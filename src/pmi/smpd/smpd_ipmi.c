/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "ipmi.h"
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* pmiimpl.h */

/* Define to prevent an smpd root thread or process from being created when there is only one process. */
/* Currently, defining this prevents the use of the spawn command. */
/*#define SINGLE_PROCESS_OPTIMIZATION*/

#define PMI_MAX_KEY_LEN          256
#define PMI_MAX_VALUE_LEN        8192
#define PMI_MAX_KVS_NAME_LENGTH  100

#define PMI_INITIALIZED 0
#define PMI_FINALIZED   1

#define PMI_TRUE        1
#define PMI_FALSE       0

typedef struct pmi_process_t
{
    int rpmi;
#ifdef HAVE_WINDOWS_H
    HANDLE hRootThread;
    HANDLE hRootThreadReadyEvent;
#else
    int root_pid;
#endif
    char root_host[100];
    int root_port;
    int local_kvs;
    char kvs_name[PMI_MAX_KVS_NAME_LENGTH];
    char domain_name[PMI_MAX_KVS_NAME_LENGTH];
    MPIDU_Sock_t sock;
    MPIDU_Sock_set_t set;
    int iproc;
    int nproc;
    int init_finalized;
    int smpd_id;
    MPIDU_SOCK_NATIVE_FD smpd_fd;
    int smpd_key;
    smpd_context_t *context;
    int clique_size;
    int *clique_ranks;
    char host[100];
    int port;
} pmi_process_t;

static int root_smpd(void *p);

/* global variables */
static pmi_process_t pmi_process =
{
    PMI_FALSE,           /* rpmi           */
#ifdef HAVE_WINDOWS_H
    NULL,                /* root thread    */
    NULL,                /* hRootThreadReadyEvent */
#else
    0,                   /* root pid       */
#endif
    "",                  /* root host      */
    0,                   /* root port      */
    PMI_FALSE,           /* local_kvs      */
    "",                  /* kvs_name       */
    "",                  /* domain_name    */
    MPIDU_SOCK_INVALID_SOCK,  /* sock           */
    MPIDU_SOCK_INVALID_SET,   /* set            */
    -1,                  /* iproc          */
    -1,                  /* nproc          */
    PMI_FINALIZED,       /* init_finalized */
    -1,                  /* smpd_id        */
    0,                   /* smpd_fd        */
    0,                   /* smpd_key       */
    NULL,                /* context        */
    0,                   /* clique_size    */
    NULL,                /* clique_ranks   */
    "",                  /* host           */
    -1                   /* port           */
};

static int pmi_err_printf(char *str, ...)
{
    int n;
    va_list list;

    printf("[%d] ", pmi_process.iproc);
    va_start(list, str);
    n = vprintf(str, list);
    va_end(list);

    fflush(stdout);

    return n;
}

static int pmi_mpi_err_printf(int mpi_errno, char *fmt, ... )
{
    int n;
    va_list list;

    /* convert the error code to a string */
    printf("mpi_errno: %d\n", mpi_errno);

    printf("[%d] ", pmi_process.iproc);
    va_start(list, fmt);
    n = vprintf(fmt, list);
    va_end(list);

    fflush(stdout);

    MPIR_Err_return_comm(NULL, "", mpi_errno);

    return n;
}

static int pmi_create_post_command(const char *command, const char *name, const char *key, const char *value)
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 1;

    if (!pmi_process.rpmi)
    {
	if (strcmp(command, "done") == 0)
	{
	    /* done commands go to the immediate smpd, not the root */
	    dest = pmi_process.smpd_id;
	}
    }

    result = smpd_create_command((char*)command, pmi_process.smpd_id, dest, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a %s command.\n", command);
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the %s command.\n", command);
	return PMI_FAIL;
    }

    if (name != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "name", (char*)name);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the kvs name('%s') to the %s command.\n", name, command);
	    return PMI_FAIL;
	}
    }

    if (key != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "key", (char*)key);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the key('%s') to the %s command.\n", key, command);
	    return PMI_FAIL;
	}
    }

    if (value != NULL)
    {
	result = smpd_add_command_arg(cmd_ptr, "value", (char*)value);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add the value('%s') to the %s command.\n", value, command);
	    return PMI_FAIL;
	}
    }

    /* post the write of the command */
    /*
    printf("posting write of dbs command to %s context, sock %d: '%s'\n",
	smpd_get_context_str(pmi_process.context), MPIDU_Sock_getid(pmi_process.context->sock), cmd_ptr->cmd);
    fflush(stdout);
    */
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the %s command.\n", command);
	return PMI_FAIL;
    }
    if (strcmp(command, "done"))
    {
	/* and post a read for the result if it is not a done command */
	result = smpd_post_read_command(pmi_process.context);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to post a read of the next command on the pmi context.\n");
	    return PMI_FAIL;
	}
    }

    /* let the state machine send the command and receive the result */
    result = smpd_enter_at_state(pmi_process.set, SMPD_WRITING_CMD);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("the state machine logic failed to get the result of the %s command.\n", command);
	return PMI_FAIL;
    }
    return PMI_SUCCESS;
}

int iPMI_Initialized(PMI_BOOL *initialized)
{
    if (initialized == NULL)
	return PMI_ERR_INVALID_ARG;
    if (pmi_process.init_finalized == PMI_INITIALIZED)
    {
	*initialized = PMI_TRUE;
    }
    else
    {
	*initialized = PMI_FALSE;
    }
    return PMI_SUCCESS;
}

static int parse_clique(const char *str_orig)
{
    int count, i;
    char *str, *token;
    int first, last;

    /* count clique */
    count = 0;
    str = strdup(str_orig);
    if (str == NULL)
	return PMI_FAIL;
    token = strtok(str, ",");
    while (token)
    {
	first = atoi(token);
	while (isdigit(*token))
	    token++;
	if (*token == '\0')
	    count++;
	else
	{
	    if (*token == '.')
	    {
		token++;
		token++;
		last = atoi(token);
		count += last - first + 1;
	    }
	    else
	    {
		pmi_err_printf("unexpected clique token: '%s'\n", token);
		free(str);
		return PMI_FAIL;
	    }
	}
	token = strtok(NULL, ",");
    }
    free(str);

    /* allocate array */
    pmi_process.clique_ranks = (int*)malloc(count * sizeof(int));
    if (pmi_process.clique_ranks == NULL)
	return PMI_FAIL;
    pmi_process.clique_size = count;
    
    /* populate array */
    count = 0;
    str = strdup(str_orig);
    if (str == NULL)
	return PMI_FAIL;
    token = strtok(str, ",");
    while (token)
    {
	first = atoi(token);
	while (isdigit(*token))
	    token++;
	if (*token == '\0')
	{
	    pmi_process.clique_ranks[count] = first;
	    count++;
	}
	else
	{
	    if (*token == '.')
	    {
		token++;
		token++;
		last = atoi(token);
		for (i=first; i<=last; i++)
		{
		    pmi_process.clique_ranks[count] = i;
		    count++;
		}
	    }
	    else
	    {
		pmi_err_printf("unexpected clique token: '%s'\n", token);
		free(str);
		return PMI_FAIL;
	    }
	}
	token = strtok(NULL, ",");
    }
    free(str);

    /*
    printf("clique: %d [", pmi_process.iproc);
    for (i=0; i<pmi_process.clique_size; i++)
    {
	printf("%d,", pmi_process.clique_ranks[i]);
    }
    printf("]\n");
    fflush(stdout);
    */
    return PMI_SUCCESS;
}

static int uPMI_ConnectToHost(char *host, int port, smpd_state_t state)
{
    int result;

    /*printf("posting a connect to %s:%d\n", pmi_process.root_host, pmi_process.root_port);*/
    result = MPIDU_Sock_post_connect(pmi_process.set, NULL, host, port, &pmi_process.sock);
    if (result != MPI_SUCCESS)
    {
	pmi_err_printf("PMI_ConnectToHost failed: unable to post a connect to %s:%d, error: %d\n", host, port, result);
	return PMI_FAIL;
    }

    result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, smpd_process.id, &pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("PMI_ConnectToHost failed: unable to create a context to connect to %s:%d with.\n", host, port);
	return PMI_FAIL;
    }
    pmi_process.context->state = state;

    result = MPIDU_Sock_set_user_ptr(pmi_process.sock, pmi_process.context);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "PMI_ConnectToHost failed: unable to set the connect sock user pointer.\n");
	return PMI_FAIL;
    }

    result = smpd_enter_at_state(pmi_process.set, state);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "PMI_ConnectToHost failed: unable to connect to %s:%d.\n", host, port);
	return PMI_FAIL;
    }

    return SMPD_SUCCESS;
}

static int rPMI_Init(int *spawned)
{
    char *p;
    int result;

    if (spawned == NULL)
	return PMI_ERR_INVALID_ARG;

    /* initialize to defaults */
    smpd_process.id = 1;
    pmi_process.smpd_id = 1;
    pmi_process.rpmi = PMI_TRUE;
    pmi_process.iproc = 0;
    pmi_process.nproc = 1;

    p = getenv("PMI_ROOT_HOST");
    if (p == NULL)
    {
	pmi_err_printf("unable to initialize the rPMI library: no PMI_ROOT_HOST specified.\n");
	return PMI_FAIL;
    }
    strncpy(pmi_process.root_host, p, 100);

    p = getenv("PMI_ROOT_PORT");
    if (p == NULL)
    {
	/* set to default port? */
	pmi_err_printf("unable to initialize the rPMI library: no PMI_ROOT_PORT specified.\n");
	return PMI_FAIL;
    }
    pmi_process.root_port = atoi(p);
    if (pmi_process.root_port < 1)
    {
	pmi_err_printf("invalid root port specified: %s\n", p);
	return PMI_FAIL;
    }
    smpd_process.port = pmi_process.root_port;
    strcpy(smpd_process.host, pmi_process.root_host);

    p = getenv("PMI_SPAWN");
    if (p)
    {
	*spawned = atoi(p);
    }
    else
    {
	*spawned = 0;
    }

    p = getenv("PMI_KVS");
    if (p != NULL)
    {
	/* use specified kvs name */
	strncpy(pmi_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	/* use default kvs name */
	strncpy(pmi_process.kvs_name, "default_mpich_kvs_name", PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.kvs_name, "default_mpich_kvs_name", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_DOMAIN");
    if (p != NULL)
    {
	strncpy(pmi_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	strncpy(pmi_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
	strncpy(smpd_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_RANK");
    if (p != NULL)
    {
	pmi_process.iproc = atoi(p);
	if (pmi_process.iproc < 0)
	{
	    pmi_err_printf("invalid rank %d\n", pmi_process.iproc);
	    return PMI_FAIL;
	}
    }

    p = getenv("PMI_SIZE");
    if (p != NULL)
    {
	pmi_process.nproc = atoi(p);
	if (pmi_process.nproc < 1)
	{
	    pmi_err_printf("invalid size %d\n", pmi_process.nproc);
	    return PMI_FAIL;
	}
    }
    smpd_process.nproc = pmi_process.nproc;
#ifdef SINGLE_PROCESS_OPTIMIZATION
/* leave this code #ifdef'd out so we can test rPMI stuff with one process */
    if (pmi_process.nproc == 1)
    {
	pmi_process.local_kvs = PMI_TRUE;
	result = smpd_dbs_init();
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to initialize the local dbs engine.\n");
	    return PMI_FAIL;
	}
	result = smpd_dbs_create(pmi_process.kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to create the process group kvs\n");
	    return PMI_FAIL;
	}
	pmi_process.init_finalized = PMI_INITIALIZED;
	return PMI_SUCCESS;
    }
#endif

    p = getenv("PMI_CLIQUE");
    if (p != NULL)
    {
	parse_clique(p);
    }

    /*
    printf("PMI_ROOT_HOST=%s PMI_ROOT_PORT=%s PMI_RANK=%s PMI_SIZE=%s PMI_KVS=%s PMI_CLIQUE=%s\n",
	getenv("PMI_ROOT_HOST"), getenv("PMI_ROOT_PORT"), getenv("PMI_RANK"), getenv("PMI_SIZE"),
	getenv("PMI_KVS"), getenv("PMI_CLIQUE"));
    fflush(stdout);
    */

    if (pmi_process.iproc == 0)
    {
	p = getenv("PMI_ROOT_LOCAL");
	if (p && strcmp(p, "1") == 0)
	{
#ifdef HAVE_WINDOWS_H
	    pmi_process.hRootThreadReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	    if (pmi_process.hRootThreadReadyEvent == NULL)
	    {
		pmi_err_printf("unable to create the root listener synchronization event, error: %d\n", GetLastError());
		return PMI_FAIL;
	    }
	    pmi_process.hRootThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)root_smpd, NULL, 0, NULL);
	    if (pmi_process.hRootThread == NULL)
	    {
		pmi_err_printf("unable to create the root listener thread: error %d\n", GetLastError());
		return PMI_FAIL;
	    }
	    if (WaitForSingleObject(pmi_process.hRootThreadReadyEvent, 60000) != WAIT_OBJECT_0)
	    {
		pmi_err_printf("the root process thread failed to initialize.\n");
		return PMI_FAIL;
	    }
#else
	    result = fork();
	    if (result == -1)
	    {
		pmi_err_printf("unable to fork the root listener, errno %d\n", errno);
		return PMI_FAIL;
	    }
	    if (result == 0)
	    {
		root_smpd(NULL);
		exit(0);
	    }
	    pmi_process.root_pid = result;
#endif
	}
    }
    else
    {
	/* set the non-root ids to iproc + 1 to aid in sorting debugging output */
	smpd_process.id = pmi_process.iproc + 1;
	pmi_process.smpd_id = smpd_process.id;
    }

    /* connect to the root */

    result = MPIDU_Sock_create_set(&pmi_process.set);
    if (result != MPI_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: unable to create a sock set, error: %d\n", result);
	return PMI_FAIL;
    }

    result = uPMI_ConnectToHost(pmi_process.root_host, pmi_process.root_port, SMPD_CONNECTING_RPMI);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed.\n");
	return PMI_FAIL;
    }
#if 0
    /*printf("posting a connect to %s:%d\n", pmi_process.root_host, pmi_process.root_port);*/
    result = MPIDU_Sock_post_connect(pmi_process.set, NULL, pmi_process.root_host, pmi_process.root_port, &pmi_process.sock);
    if (result != MPI_SUCCESS)
    {
	pmi_err_printf("PMI_Init failed: unable to post a connect to the root, error: %d\n", result);
	return PMI_FAIL;
    }

    result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, smpd_process.id, &pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a context to connect to the root with.\n");
	return PMI_FAIL;
    }
    pmi_process.context->state = SMPD_CONNECTING_RPMI;

    result = MPIDU_Sock_set_user_ptr(pmi_process.sock, pmi_process.context);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "unable to set the connect sock user pointer.\n");
	return PMI_FAIL;
    }

    result = smpd_enter_at_state(pmi_process.set, SMPD_CONNECTING_RPMI);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "PMI_Init failed: unable to connect to the root.\n");
	return PMI_FAIL;
    }
#endif

    pmi_process.init_finalized = PMI_INITIALIZED;

    return PMI_SUCCESS;
}

static int rPMI_Finalize()
{
    int result;
#ifndef HAVE_WINDOWS_H
    int status;
#endif

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    if (pmi_process.local_kvs)
    {
	smpd_dbs_finalize();
	pmi_process.init_finalized = PMI_FINALIZED;
	return PMI_SUCCESS;
    }

    if (pmi_process.iproc == 0)
    {
	/* the root process tells the root to exit when all the pmi contexts have exited */
	result = pmi_create_post_command("exit_on_done", NULL, NULL, NULL);
	if (result != PMI_SUCCESS)
	{
	    pmi_err_printf("exit_on_done command failed.\n");
	    return PMI_FAIL;
	}
	/*printf("exit_on_done command returned successfully.\n");fflush(stdout);*/
    }

    /*printf("entering finalize pmi_barrier.\n");fflush(stdout);*/
    PMI_Barrier();
    /*printf("after finalize pmi_barrier, posting done command.\n");fflush(stdout);*/

    /* post a done command to close the pmi context */
    result = pmi_create_post_command("done", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("failed.\n");
	return PMI_FAIL;
    }

    if (pmi_process.iproc == 0)
    {
#ifdef HAVE_WINDOWS_H
	WaitForSingleObject(pmi_process.hRootThread, INFINITE);
#else
	waitpid(pmi_process.root_pid, &status, WUNTRACED);
#endif
    }

    if (pmi_process.sock != MPIDU_SOCK_INVALID_SOCK)
    {
	result = MPIDU_Sock_finalize();
	if (result != MPI_SUCCESS)
	{
	    pmi_err_printf("MPIDU_Sock_finalize failed, error: %d\n", result);
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;

    return PMI_SUCCESS;
}

int iPMI_Init(int *spawned)
{
    char *p;
    int result;

    if (spawned == NULL)
	return PMI_ERR_INVALID_ARG;

    /* don't allow pmi_init to be called more than once */
    if (pmi_process.init_finalized == PMI_INITIALIZED)
	return PMI_SUCCESS;

    /* initialize to defaults */

    result = MPIDU_Sock_init();
    if (result != MPI_SUCCESS)
    {
	pmi_err_printf("MPIDU_Sock_init failed,\nsock error: %s\n", get_sock_error_string(result));
	return PMI_FAIL;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to initialize the smpd global process structure.\n");
	return PMI_FAIL;
    }

    p = getenv("PMI_ROOT_HOST");
    if (p != NULL)
    {
	return rPMI_Init(spawned);
    }

    pmi_process.iproc = 0;
    pmi_process.nproc = 1;

    p = getenv("PMI_SPAWN");
    if (p)
    {
	*spawned = atoi(p);
    }
    else
    {
	*spawned = 0;
    }

    p = getenv("PMI_KVS");
    if (p != NULL)
    {
	strncpy(pmi_process.kvs_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	pmi_process.local_kvs = PMI_TRUE;
	result = smpd_dbs_init();
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to initialize the local dbs engine.\n");
	    return PMI_FAIL;
	}
	result = smpd_dbs_create(pmi_process.kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to create the process group kvs\n");
	    return PMI_FAIL;
	}
	strncpy(pmi_process.domain_name, smpd_process.domain_name, PMI_MAX_KVS_NAME_LENGTH);
	pmi_process.init_finalized = PMI_INITIALIZED;
	return PMI_SUCCESS;
    }

    p = getenv("PMI_DOMAIN");
    if (p != NULL)
    {
	strncpy(pmi_process.domain_name, p, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	strncpy(pmi_process.domain_name, "mpich2", PMI_MAX_KVS_NAME_LENGTH);
    }

    p = getenv("PMI_RANK");
    if (p != NULL)
    {
	pmi_process.iproc = atoi(p);
	if (pmi_process.iproc < 0)
	{
	    pmi_err_printf("invalid rank %d, setting to 0\n", pmi_process.iproc);
	    pmi_process.iproc = 0;
	}
    }

    p = getenv("PMI_SIZE");
    if (p != NULL)
    {
	pmi_process.nproc = atoi(p);
	if (pmi_process.nproc < 1)
	{
	    pmi_err_printf("invalid size %d, setting to 1\n", pmi_process.nproc);
	    pmi_process.nproc = 1;
	}
    }

    p = getenv("PMI_SMPD_ID");
    if (p != NULL)
    {
	pmi_process.smpd_id = atoi(p);
	smpd_process.id = pmi_process.smpd_id;
    }

    p = getenv("PMI_SMPD_KEY");
    if (p != NULL)
    {
	pmi_process.smpd_key = atoi(p);
    }

    p = getenv("PMI_SMPD_FD");
    if (p != NULL)
    {
	result = MPIDU_Sock_create_set(&pmi_process.set);
	if (result != MPI_SUCCESS)
	{
	    pmi_err_printf("PMI_Init failed: unable to create a sock set, error:\n%s\n",
		get_sock_error_string(result));
	    return PMI_FAIL;
	}

#ifdef HAVE_WINDOWS_H
	pmi_process.smpd_fd = smpd_decode_handle(p);
#else
	pmi_process.smpd_fd = (MPIDU_SOCK_NATIVE_FD)atoi(p);
#endif
	result = MPIDU_Sock_native_to_sock(pmi_process.set, pmi_process.smpd_fd, NULL, &pmi_process.sock);
	if (result != MPI_SUCCESS)
	{
	    pmi_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	    return PMI_FAIL;
	}
	result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, pmi_process.smpd_id, &pmi_process.context);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to create a pmi context.\n");
	    return PMI_FAIL;
	}
    }
    else
    {
	p = getenv("PMI_HOST");
	if (p != NULL)
	{
	    strncpy(pmi_process.host, p, 100);
	    p = getenv("PMI_PORT");
	    if (p != NULL)
	    {
		pmi_process.port = atoi(p);

		result = MPIDU_Sock_create_set(&pmi_process.set);
		if (result != MPI_SUCCESS)
		{
		    pmi_err_printf("PMI_Init failed: unable to create a sock set, error: %d\n", result);
		    return PMI_FAIL;
		}

		result = uPMI_ConnectToHost(pmi_process.host, pmi_process.port, SMPD_CONNECTING_PMI);
		if (result != SMPD_SUCCESS)
		{
		    pmi_err_printf("PMI_Init failed.\n");
		    return PMI_FAIL;
		}
#if 0
		/*printf("posting a connect to %s:%d\n", pmi_process.host, pmi_process.port);*/
		result = MPIDU_Sock_post_connect(pmi_process.set, NULL, pmi_process.host, pmi_process.port, &pmi_process.sock);
		if (result != MPI_SUCCESS)
		{
		    pmi_err_printf("PMI_Init failed: unable to post a connect to the process manager, error: %d\n", result);
		    return PMI_FAIL;
		}

		result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, smpd_process.id, &pmi_process.context);
		if (result != SMPD_SUCCESS)
		{
		    pmi_err_printf("unable to create a context to connect to the process manager with.\n");
		    return PMI_FAIL;
		}
		pmi_process.context->state = SMPD_CONNECTING_PMI;

		result = MPIDU_Sock_set_user_ptr(pmi_process.sock, pmi_process.context);
		if (result != MPI_SUCCESS)
		{
		    pmi_mpi_err_printf(result, "unable to set the connect sock user pointer.\n");
		    return PMI_FAIL;
		}

		result = smpd_enter_at_state(pmi_process.set, SMPD_CONNECTING_PMI);
		if (result != MPI_SUCCESS)
		{
		    pmi_mpi_err_printf(result, "PMI_Init failed: unable to connect to the process manager.\n");
		    return PMI_FAIL;
		}
#endif
	    }
	}
	else
	{
	    pmi_err_printf("No mechanism specified for connecting to the process manager.\n");
	    return PMI_FAIL;
	}
    }

    p = getenv("PMI_CLIQUE");
    if (p != NULL)
    {
	parse_clique(p);
    }

    /*
    printf("PMI_RANK=%s PMI_SIZE=%s PMI_KVS=%s PMI_SMPD_ID=%s PMI_SMPD_FD=%s PMI_SMPD_KEY=%s\n PMI_SPAWN=%s",
	getenv("PMI_RANK"), getenv("PMI_SIZE"), getenv("PMI_KVS"), getenv("PMI_SMPD_ID"),
	getenv("PMI_SMPD_FD"), getenv("PMI_SMPD_KEY"), getenv("PMI_SPAWN"));
    fflush(stdout);
    */

    pmi_process.init_finalized = PMI_INITIALIZED;

    /*
    if (*spawned && pmi_process.iproc == 0)
    {
	char key[1024], val[8192];
	key[0] = '\0';
	result = PMI_KVS_Iter_first(pmi_process.kvs_name, key, 1024, val, 8192);
	if (result != PMI_SUCCESS || key[0] == '\0')
	{
	    printf("No preput values in %s\n", pmi_process.kvs_name);
	}
	while (result == PMI_SUCCESS && key[0] != '\0')
	{
	    printf("PREPUT key=%s, val=%s\n", key, val);
	    result = PMI_KVS_Iter_next(pmi_process.kvs_name, key, 1024, val, 8192);
	}
	fflush(stdout);
    }
    iPMI_Barrier();
    */

    /*printf("iPMI_Init returning success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Finalize()
{
    int result;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    /*
    printf("PMI_Finalize called.\n");
    fflush(stdout);
    */

    if (pmi_process.rpmi)
    {
	return rPMI_Finalize();
    }

    if (pmi_process.local_kvs)
    {
	smpd_dbs_finalize();
	pmi_process.init_finalized = PMI_FINALIZED;
	return PMI_SUCCESS;
    }

    PMI_Barrier();

    /* post the done command and wait for the result */
    result = pmi_create_post_command("done", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("failed.\n");
	return PMI_FAIL;
    }

    if (pmi_process.sock != MPIDU_SOCK_INVALID_SOCK)
    {
	result = MPIDU_Sock_finalize();
	if (result != MPI_SUCCESS)
	{
	    pmi_err_printf("MPIDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;

    /*printf("iPMI_Finalize success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Get_size(int *size)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (size == NULL)
	return PMI_ERR_INVALID_ARG;

    *size = pmi_process.nproc;

    return PMI_SUCCESS;
}

int iPMI_Get_rank(int *rank)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (rank == NULL)
	return PMI_ERR_INVALID_ARG;

    *rank = pmi_process.iproc;

    return PMI_SUCCESS;
}

int iPMI_Get_clique_size( int *size )
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (size == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.clique_size == 0)
	*size = 1;
    else
	*size = pmi_process.clique_size;
    return PMI_SUCCESS;
}

int iPMI_Get_clique_ranks( int ranks[], int length )
{
    int i;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (ranks == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < pmi_process.clique_size)
	return PMI_ERR_INVALID_LENGTH;

    if (pmi_process.clique_size == 0)
    {
	*ranks = 0;
    }
    else
    {
	for (i=0; i<pmi_process.clique_size; i++)
	{
	    ranks[i] = pmi_process.clique_ranks[i];
	}
    }
    return PMI_SUCCESS;
}

int iPMI_Get_id( char id_str[], int length )
{
    /*
    char kvs_name[PMI_MAX_KVS_NAME_LENGTH];
    char key[PMI_MAX_KEY_LEN] = PMI_KVS_ID_KEY;
    char value[PMI_MAX_VALUE_LEN] = "";

    strcpy(kvs_name, pmi_process.kvs_name);
    iPMI_KVS_Get(kvs_name, key, value);
    strcpy(id_str, value);

    return PMI_SUCCESS;
    */
    return iPMI_KVS_Get_my_name(id_str, length);
}

int iPMI_Get_id_length_max(int *maxlen)
{
    /*return 40;*/
    return iPMI_KVS_Get_name_length_max(maxlen);
}

int iPMI_Get_kvs_domain_id(char id_str[], int length)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (id_str == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    strncpy(id_str, pmi_process.domain_name, length);

    return PMI_SUCCESS;
}

int iPMI_Barrier()
{
    int result;
    char count_str[20];
    char str[1024];
    
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;

    if (pmi_process.nproc == 1)
	return PMI_SUCCESS;

    /*printf("entering barrier %d, %s\n", pmi_process.nproc, pmi_process.kvs_name);fflush(stdout);*/

    /* encode the size of the barrier */
    snprintf(count_str, 20, "%d", pmi_process.nproc);

    /* post the command and wait for the result */
    result = pmi_create_post_command("barrier", pmi_process.kvs_name, NULL, count_str);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_Barrier failed.\n");
	return PMI_FAIL;
    }

    /* interpret the result */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_Barrier failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Barrier failed: '%s'\n", str);
	return PMI_FAIL;
    }

    /*printf("iPMI_Barrier success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_my_name(char kvsname[], int length)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    strncpy(kvsname, pmi_process.kvs_name, length);

    /*
    printf("my kvs name is %s\n", kvsname);fflush(stdout);
    */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get_name_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_KVS_NAME_LENGTH;
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_key_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_KEY_LEN;
    return PMI_SUCCESS;
}

int iPMI_KVS_Get_value_length_max(int *maxlen)
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_MAX_VALUE_LEN;
    return PMI_SUCCESS;
}

int iPMI_KVS_Create(char kvsname[], int length)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (length < PMI_MAX_KVS_NAME_LENGTH)
	return PMI_ERR_INVALID_LENGTH;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_create(kvsname);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbcreate", NULL, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: unable to create a pmi kvs space.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Create failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "name", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Create failed: no kvs name in the dbcreate result command.\n");
	return PMI_FAIL;
    }
    strncpy(kvsname, str, PMI_MAX_KVS_NAME_LENGTH);

    /*printf("iPMI_KVS_Create success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Destroy(const char kvsname[])
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_destroy(kvsname);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbdestroy", kvsname, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Destroy failed: unable to destroy the pmi kvs space named '%s'.\n", kvsname);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Destroy failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Destroy failed: %s\n", str);
	return PMI_FAIL;
    }

    return PMI_FAIL;
}

int iPMI_KVS_Put(const char kvsname[], const char key[], const char value[])
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;

    /*printf("putting <%s><%s><%s>\n", kvsname, key, value);fflush(stdout);*/

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_put(kvsname, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbput", kvsname, key, value);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Put failed: unable to put '%s:%s:%s'\n", kvsname, key, value);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Put failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Put failed: '%s'\n", str);
	return PMI_FAIL;
    }

    /*printf("iPMI_KVS_Put success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_KVS_Commit(const char kvsname[])
{
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	return PMI_SUCCESS;
    }

    /* Make the puts return when the commands are written but not acknowledged.
       Then have this function wait until all outstanding puts are acknowledged.
       */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get(const char kvsname[], const char key[], char value[], int length)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_get(kvsname, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbget", kvsname, key, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: unable to get '%s:%s'\n", kvsname, key);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Get failed: '%s'\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Get failed: no value in the result command for the get: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    /*
    printf("iPMI_KVS_Get success.\n");fflush(stdout);
    printf("get <%s><%s><%s>\n", kvsname, key, value);
    fflush(stdout);
    */
    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_first(const char kvsname[], char key[], int key_len, char value[], int val_len)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (key_len < PMI_MAX_KEY_LEN)
	return PMI_ERR_INVALID_KEY_LENGTH;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;
    if (val_len < PMI_MAX_VALUE_LEN)
	return PMI_ERR_INVALID_VAL_LENGTH;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_first(kvsname, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbfirst", kvsname, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: unable to get the first key/value pair from '%s'\n", kvsname);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no key in the result command for the pmi iter_first: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_END_STR) == 0)
    {
	*key = '\0';
	*value = '\0';
	return PMI_SUCCESS;
    }
    strcpy(key, str);
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no value in the result command for the pmi iter_first: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_next(const char kvsname[], char key[], int key_len, char value[], int val_len)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (kvsname == NULL)
	return PMI_ERR_INVALID_ARG;
    if (key == NULL)
	return PMI_ERR_INVALID_KEY;
    if (key_len < PMI_MAX_KEY_LEN)
	return PMI_ERR_INVALID_KEY_LENGTH;
    if (value == NULL)
	return PMI_ERR_INVALID_VAL;
    if (val_len < PMI_MAX_VALUE_LEN)
	return PMI_ERR_INVALID_VAL_LENGTH;

    if (pmi_process.local_kvs)
    {
	result = smpd_dbs_next(kvsname, key, value);
	return (result == SMPD_SUCCESS) ? PMI_SUCCESS : PMI_FAIL;
    }

    result = pmi_create_post_command("dbnext", kvsname, NULL, NULL);
    if (result != PMI_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: unable to get the next key/value pair from '%s'\n", kvsname);
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: %s\n", str);
	return PMI_FAIL;
    }
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no key in the result command for the pmi iter_next: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_END_STR) == 0)
    {
	*key = '\0';
	*value = '\0';
	return PMI_SUCCESS;
    }
    strcpy(key, str);
    if (MPIU_Str_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN) != MPIU_STR_SUCCESS)
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no value in the result command for the pmi iter_next: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 0;
    char buffer[SMPD_MAX_CMD_LENGTH];
    char keyval_buf[SMPD_MAX_CMD_LENGTH];
    char key[100];
    char *iter, *iter2;
    int i, j, maxlen, maxlen2;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_ERR_INIT;
    if (count < 1 || cmds == NULL || maxprocs == NULL || preput_keyval_size < 0)
	return PMI_ERR_INVALID_ARG;

    if (pmi_process.local_kvs)
    {
	return PMI_FAIL;
    }

    /*printf("creating spawn command.\n");fflush(stdout);*/
    result = smpd_create_command("spawn", pmi_process.smpd_id, dest, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a spawn command.\n");
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the spawn command.\n");
	return PMI_FAIL;
    }

    /* add the number of commands */
    result = smpd_add_command_int_arg(cmd_ptr, "ncmds", count);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the ncmds field to the spawn command.\n");
	return PMI_FAIL;
    }
    /* add the commands and their argv arrays */
    for (i=0; i<count; i++)
    {
	sprintf(key, "cmd%d", i);
	result = smpd_add_command_arg(cmd_ptr, key, (char*)cmds[i]);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, cmds[i]);
	    return PMI_FAIL;
	}
	if (argvs)
	{
	    buffer[0] = '\0';
	    iter = buffer;
	    maxlen = SMPD_MAX_CMD_LENGTH;
	    if (argvs[i] != NULL)
	    {
		for (j=0; argvs[i][j] != NULL; j++)
		{
		    result = MPIU_Str_add_string(&iter, &maxlen, argvs[i][j]);
		}
	    }
	    sprintf(key, "argv%d", i);
	    result = smpd_add_command_arg(cmd_ptr, key, buffer);
	    if (result != SMPD_SUCCESS)
	    {
		pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
		return PMI_FAIL;
	    }
	}
    }
    /* add the maxprocs array */
    buffer[0] = '\0';
    for (i=0; i<count; i++)
    {
	if (i < count-1)
	    sprintf(key, "%d ", maxprocs[i]);
	else
	    sprintf(key, "%d", maxprocs[i]);
	strcat(buffer, key);
    }
    result = smpd_add_command_arg(cmd_ptr, "maxprocs", buffer);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add maxprocs(%s) to the spawn command.\n", buffer);
	return PMI_FAIL;
    }
    /* add the keyval sizes array */
    if (info_keyval_sizes)
    {
	buffer[0] = '\0';
	for (i=0; i<count; i++)
	{
	    if (i < count-1)
		sprintf(key, "%d ", info_keyval_sizes[i]);
	    else
		sprintf(key, "%d", info_keyval_sizes[i]);
	    strcat(buffer, key);
	}
	result = smpd_add_command_arg(cmd_ptr, "nkeyvals", buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add nkeyvals(%s) to the spawn command.\n", buffer);
	    return PMI_FAIL;
	}
    }
    /* add the keyvals */
    if (info_keyval_sizes && info_keyval_vectors)
    {
	for (i=0; i<count; i++)
	{
	    buffer[0] = '\0';
	    iter = buffer;
	    maxlen = SMPD_MAX_CMD_LENGTH;
	    for (j=0; j<info_keyval_sizes[i]; j++)
	    {
		keyval_buf[0] = '\0';
		iter2 = keyval_buf;
		maxlen2 = SMPD_MAX_CMD_LENGTH;
		result = MPIU_Str_add_string_arg(&iter2, &maxlen2, info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val);
		if (result != MPIU_STR_SUCCESS)
		{
		    pmi_err_printf("unable to add %s=%s to the spawn command.\n", info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val);
		    return PMI_FAIL;
		}
		iter2--;
		*iter2 = '\0'; /* remove the trailing space */
		sprintf(key, "%d", j);
		result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
		if (result != MPIU_STR_SUCCESS)
		{
		    pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
		    return PMI_FAIL;
		}
	    }
	    sprintf(key, "keyvals%d", i);
	    result = smpd_add_command_arg(cmd_ptr, key, buffer);
	    if (result != SMPD_SUCCESS)
	    {
		pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
		return PMI_FAIL;
	    }
	}
    }
    /* add the pre-put keyvals */
    result = smpd_add_command_int_arg(cmd_ptr, "npreput", preput_keyval_size);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add npreput=%d to the spawn command.\n", preput_keyval_size);
	return PMI_FAIL;
    }
    if (preput_keyval_size > 0 && preput_keyval_vector)
    {
	buffer[0] = '\0';
	iter = buffer;
	maxlen = SMPD_MAX_CMD_LENGTH;
	for (i=0; i<preput_keyval_size; i++)
	{
	    keyval_buf[0] = '\0';
	    iter2 = keyval_buf;
	    maxlen2 = SMPD_MAX_CMD_LENGTH;
	    result = MPIU_Str_add_string_arg(&iter2, &maxlen2, preput_keyval_vector[i].key, preput_keyval_vector[i].val);
	    if (result != MPIU_STR_SUCCESS)
	    {
		pmi_err_printf("unable to add %s=%s to the spawn command.\n", preput_keyval_vector[i].key, preput_keyval_vector[i].val);
		return PMI_FAIL;
	    }
	    iter2--;
	    *iter2 = '\0'; /* remove the trailing space */
	    sprintf(key, "%d", i);
	    result = MPIU_Str_add_string_arg(&iter, &maxlen, key, keyval_buf);
	    if (result != MPIU_STR_SUCCESS)
	    {
		pmi_err_printf("unable to add %s=%s to the spawn command.\n", key, keyval_buf);
		return PMI_FAIL;
	    }
	}
	result = smpd_add_command_arg(cmd_ptr, "preput", buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add preput(%s) to the spawn command.\n", buffer);
	    return PMI_FAIL;
	}
    }	

    /*printf("spawn command: <%s>\n", cmd_ptr->cmd);*/

    /* post the write of the command */
    /*
    printf("posting write of spawn command to %s context, sock %d: '%s'\n",
	smpd_get_context_str(pmi_process.context), MPIDU_Sock_get_sock_id(pmi_process.context->sock), cmd_ptr->cmd);
    fflush(stdout);
    */
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the spawn command.\n");
	return PMI_FAIL;
    }

    /* post a read for the result*/
    result = smpd_post_read_command(pmi_process.context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a read of the next command on the pmi context.\n");
	return PMI_FAIL;
    }

    /* let the state machine send the command and receive the result */
    result = smpd_enter_at_state(pmi_process.set, SMPD_WRITING_CMD);
    if (result != SMPD_SUCCESS)
    {
	/*printf("PMI_Spawn_multiple returning failure.\n");fflush(stdout);*/
	pmi_err_printf("the state machine logic failed to get the result of the spawn command.\n");
	return PMI_FAIL;
    }
    /*printf("PMI_Spawn_multiple returning success.\n");fflush(stdout);*/
    return PMI_SUCCESS;
}

int iPMI_Parse_option(int num_args, char *args[], int *num_parsed, PMI_keyval_t **keyvalp, int *size)
{
    if (num_args < 1)
	return PMI_ERR_INVALID_NUM_ARGS;
    if (args == NULL)
	return PMI_ERR_INVALID_ARGS;
    if (num_parsed == NULL)
	return PMI_ERR_INVALID_NUM_PARSED;
    if (keyvalp == NULL)
	return PMI_ERR_INVALID_KEYVALP;
    if (size == NULL)
	return PMI_ERR_INVALID_SIZE;
    *num_parsed = 0;
    *keyvalp = NULL;
    *size = 0;
    return PMI_SUCCESS;
}

int iPMI_Args_to_keyval(int *argcp, char *((*argvp)[]), PMI_keyval_t **keyvalp, int *size)
{
    if (argcp == NULL || argvp == NULL || keyvalp == NULL || size == NULL)
	return PMI_ERR_INVALID_ARG;
    return PMI_SUCCESS;
}

int iPMI_Free_keyvals(PMI_keyval_t keyvalp[], int size)
{
    if (keyvalp == NULL || size < 0)
	return PMI_ERR_INVALID_ARG;
    if (size == 0)
	return PMI_SUCCESS;
    /* free stuff */
    return PMI_SUCCESS;
}

static int root_smpd(void *p)
{
    int result;
    MPIDU_Sock_set_t set;
    MPIDU_Sock_t listener;

    smpd_process.id = 1;
    smpd_process.root_smpd = SMPD_FALSE;

    result = MPIDU_Sock_create_set(&set);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "MPIDU_Sock_create_set failed.\n");
	return PMI_FAIL;
    }
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", MPIDU_Sock_get_sock_set_id(set));
    result = MPIDU_Sock_listen(set, NULL, &pmi_process.root_port, &listener); 
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "MPIDU_Sock_listen failed.\n");
	return PMI_FAIL;
    }
    smpd_dbg_printf("smpd listening on port %d\n", pmi_process.root_port);

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a context for the smpd listener.\n");
	return PMI_FAIL;
    }
    result = MPIDU_Sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "MPIDU_Sock_set_user_ptr failed.\n");
	return PMI_FAIL;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    smpd_dbs_init();
    smpd_process.have_dbs = SMPD_TRUE;
    result = smpd_dbs_create_name_in(smpd_process.kvs_name);
    if (result != SMPD_DBS_SUCCESS)
    {
	pmi_err_printf("unable to create a kvs database: name = <%s>.\n", smpd_process.kvs_name);
	return PMI_FAIL;
    }

#ifdef HAVE_WINDOWS_H
    SetEvent(pmi_process.hRootThreadReadyEvent);
#endif

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("state machine failed.\n");
	return PMI_FAIL;
    }

    result = MPIDU_Sock_destroy_set(set);
    if (result != MPI_SUCCESS)
    {
	pmi_mpi_err_printf(result, "unable to destroy the set.\n");
    }

    return PMI_SUCCESS;
}
