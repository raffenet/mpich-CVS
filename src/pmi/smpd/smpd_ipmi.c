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
#define PMI_MAX_KEY_LEN          256
#define PMI_MAX_VALUE_LEN        8192
#define PMI_MAX_KVS_NAME_LENGTH  100

#define PMI_INITIALIZED 0
#define PMI_FINALIZED   1

#define PMI_TRUE        1
#define PMI_FALSE       0

typedef struct pmi_process_t
{
    int local_kvs;
    char kvs_name[PMI_MAX_KVS_NAME_LENGTH];
    sock_t sock;
    sock_set_t set;
    int iproc;
    int nproc;
    int init_finalized;
    int smpd_id;
    SOCK_NATIVE_FD smpd_fd;
    int smpd_key;
    smpd_context_t *context;
    int clique_size;
    int *clique_ranks;
} pmi_process_t;

/* global variables */
pmi_process_t pmi_process =
{
    PMI_FALSE,          /* local_kvs      */
    "",                 /* kvs_name       */
    SOCK_INVALID_SOCK,  /* sock           */
    SOCK_INVALID_SET,   /* set            */
    -1,                 /* iproc          */
    -1,                 /* nproc          */
    PMI_FINALIZED,      /* init_finalized */
    -1,                 /* smpd_id        */
    0,                  /* smpd_fd        */
    0,                  /* smpd_key       */
    NULL,               /* context        */
    0,                  /* clique_size    */
    NULL                /* clique_ranks   */
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

static int pmi_create_post_command(const char *command, const char *name, const char *key, const char *value)
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 1;

    if (strcmp(command, "done") == 0)
    {
	/* done commands go to the immediate smpd, not the root */
	dest = pmi_process.smpd_id;
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
	smpd_get_context_str(pmi_process.context), sock_getid(pmi_process.context->sock), cmd_ptr->cmd);
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

int iPMI_Initialized()
{
    if (pmi_process.init_finalized == PMI_INITIALIZED)
	return 1;
    return 0;
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

int iPMI_Init(int *spawned)
{
    char *p;
    int result;

    /* don't allow pmi_init to be called more than once */
    if (pmi_process.init_finalized == PMI_INITIALIZED)
	return PMI_SUCCESS;

    /* initialize to defaults */

    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	pmi_err_printf("sock_init failed,\nsock error: %s\n", get_sock_error_string(result));
	return PMI_FAIL;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to initialize the smpd global process structure.\n");
	return PMI_FAIL;
    }

    pmi_process.iproc = 0;
    pmi_process.nproc = 1;

    p = getenv("PMI_SPAWN");
    *spawned = (p != NULL) ? 1 : 0;

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
	pmi_process.init_finalized = PMI_INITIALIZED;
	return PMI_SUCCESS;
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
	result = sock_create_set(&pmi_process.set);
	if (result != SOCK_SUCCESS)
	{
	    pmi_err_printf("PMI_Init failed: unable to create a sock set, error:\n%s\n",
		get_sock_error_string(result));
	    return PMI_FAIL;
	}

#ifdef HAVE_WINDOWS_H
	pmi_process.smpd_fd = smpd_decode_handle(p);
#else
	pmi_process.smpd_fd = (SOCK_NATIVE_FD)atoi(p);
#endif
	result = sock_native_to_sock(pmi_process.set, pmi_process.smpd_fd, NULL, &pmi_process.sock);
	if (result != SOCK_SUCCESS)
	{
	    pmi_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	    return PMI_FAIL;
	}
	result = smpd_create_context(SMPD_CONTEXT_PMI, pmi_process.set, pmi_process.sock, pmi_process.smpd_id, &pmi_process.context);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to create a pmi context.\n");
	    return PMI_FAIL;
	}
    }

    p = getenv("PMI_CLIQUE");
    if (p != NULL)
    {
	parse_clique(p);
    }

    /*
    printf("PMI_RANK=%s PMI_SIZE=%s PMI_KVS=%s PMI_SMPD_ID=%s PMI_SMPD_FD=%s PMI_SMPD_KEY=%s\n",
	getenv("PMI_RANK"), getenv("PMI_SIZE"), getenv("PMI_KVS"), getenv("PMI_SMPD_ID"),
	getenv("PMI_SMPD_FD"), getenv("PMI_SMPD_KEY"));
    fflush(stdout);
    */

    pmi_process.init_finalized = PMI_INITIALIZED;

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

    if (pmi_process.sock != SOCK_INVALID_SOCK)
    {
	result = sock_finalize();
	if (result != SOCK_SUCCESS)
	{
	    pmi_err_printf("sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;

    return PMI_SUCCESS;
}

int iPMI_Get_size(int *size)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || size == NULL)
	return PMI_FAIL;

    *size = pmi_process.nproc;

    return PMI_SUCCESS;
}

int iPMI_Get_rank(int *rank)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || rank == NULL)
	return PMI_FAIL;

    *rank = pmi_process.iproc;

    return PMI_SUCCESS;
}

int iPMI_Get_clique_size( int *size )
{
    if (pmi_process.clique_size == 0)
	*size = 1;
    else
	*size = pmi_process.clique_size;
    return PMI_SUCCESS;
}

int iPMI_Get_clique_ranks( int *ranks )
{
    int i;
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

int iPMI_Get_id( char *id_str )
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
    return iPMI_KVS_Get_my_name(id_str);
}

int iPMI_Get_id_length_max()
{
    /*return 40;*/
    return iPMI_KVS_Get_name_length_max();
}

int iPMI_Barrier()
{
    int result;
    char count_str[20];
    char str[1024];
    
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_FAIL;

    if (pmi_process.nproc == 1)
	return PMI_SUCCESS;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_Barrier failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_Barrier failed: '%s'\n", str);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Get_my_name(char *kvsname)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    strcpy(kvsname, pmi_process.kvs_name);

    /*
    printf("my kvs name is %s\n", kvsname);fflush(stdout);
    */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get_name_length_max()
{
    return PMI_MAX_KVS_NAME_LENGTH;
}

int iPMI_KVS_Get_key_length_max()
{
    return PMI_MAX_KEY_LEN;
}

int iPMI_KVS_Get_value_length_max()
{
    return PMI_MAX_VALUE_LEN;
}

int iPMI_KVS_Create(char * kvsname)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Create failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Create failed: %s\n", str);
	return PMI_FAIL;
    }
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "name", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Create failed: no kvs name in the dbcreate result command.\n");
	return PMI_FAIL;
    }
    strncpy(kvsname, str, PMI_MAX_KVS_NAME_LENGTH);

    return PMI_SUCCESS;
}

int iPMI_KVS_Destroy(const char * kvsname)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
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

int iPMI_KVS_Put(const char *kvsname, const char *key, const char *value)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    /*
    printf("putting <%s:%s:%s>\n", kvsname, key, value);
    fflush(stdout);
    */

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Put failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Put failed: '%s'\n", str);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Commit(const char *kvsname)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    if (pmi_process.local_kvs)
    {
	return PMI_SUCCESS;
    }

    /* Make the puts return when the commands are written but not acknowledged.
       Then have this function wait until all outstanding puts are acknowledged.
       */

    return PMI_SUCCESS;
}

int iPMI_KVS_Get(const char *kvsname, const char *key, char *value)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Get failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Get failed: '%s'\n", str);
	return PMI_FAIL;
    }
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN))
    {
	pmi_err_printf("PMI_KVS_Get failed: no value in the result command for the get: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_first(const char *kvsname, char *key, char *value)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: %s\n", str);
	return PMI_FAIL;
    }
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN))
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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN))
    {
	pmi_err_printf("PMI_KVS_Iter_first failed: no value in the result command for the pmi iter_first: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_KVS_Iter_next(const char *kvsname, char *key, char *value)
{
    int result;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: %s\n", str);
	return PMI_FAIL;
    }
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "key", str, PMI_MAX_KEY_LEN))
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
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "value", value, PMI_MAX_VALUE_LEN))
    {
	pmi_err_printf("PMI_KVS_Iter_next failed: no value in the result command for the pmi iter_next: '%s'\n", pmi_process.context->read_cmd.cmd);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int iPMI_Spawn_multiple(int count,
                       const char ** cmds,
                       const char *** argvs,
                       const int * maxprocs,
                       const int * info_keyval_sizes,
                       const PMI_keyval_t ** info_keyval_vectors,
                       int preput_keyval_size,
                       const PMI_keyval_t * preput_keyval_vector,
                       int * errors)
{
    int result;
    smpd_command_t *cmd_ptr;
    int dest = 0;
    char buffer[SMPD_MAX_CMD_LENGTH];
    char keyval_buf[SMPD_MAX_CMD_LENGTH];
    char key[100];
    char *iter, *iter2;
    int i, j, maxlen, maxlen2;
    int num_chars;

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
	buffer[0] = '\0';
	iter = buffer;
	maxlen = SMPD_MAX_CMD_LENGTH;
	for (j=0; argvs[i][j] != NULL; j++)
	{
	    num_chars = smpd_add_string(iter, maxlen, argvs[i][j]);
	    maxlen -= num_chars;
	    iter += num_chars;
	}
	sprintf(key, "argv%d", i);
	result = smpd_add_command_arg(cmd_ptr, key, buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
	    return PMI_FAIL;
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
    /* add the keyvals */
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
	    smpd_add_string_arg(&iter2, &maxlen2, info_keyval_vectors[i][j].key, info_keyval_vectors[i][j].val);
	    iter2--;
	    *iter2 = '\0'; /* remove the trailing space */
	    sprintf(key, "%d", j);
	    smpd_add_string_arg(&iter, &maxlen, key, keyval_buf);
	}
	sprintf(key, "keyvals%d", i);
	result = smpd_add_command_arg(cmd_ptr, key, buffer);
	if (result != SMPD_SUCCESS)
	{
	    pmi_err_printf("unable to add %s(%s) to the spawn command.\n", key, buffer);
	    return PMI_FAIL;
	}
    }
    /* add the pre-put keyvals */
    result = smpd_add_command_int_arg(cmd_ptr, "npreput", preput_keyval_size);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add npreput=%d to the spawn command.\n", preput_keyval_size);
	return PMI_FAIL;
    }
    buffer[0] = '\0';
    iter = buffer;
    maxlen = SMPD_MAX_CMD_LENGTH;
    for (i=0; i<preput_keyval_size; i++)
    {
	keyval_buf[0] = '\0';
	iter2 = keyval_buf;
	maxlen2 = SMPD_MAX_CMD_LENGTH;
	smpd_add_string_arg(&iter2, &maxlen2, preput_keyval_vector[i].key, preput_keyval_vector[i].val);
	iter2--;
	*iter2 = '\0'; /* remove the trailing space */
	sprintf(key, "%d", i);
	smpd_add_string_arg(&iter, &maxlen, key, keyval_buf);
    }
    result = smpd_add_command_arg(cmd_ptr, "preput", buffer);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add preput(%s) to the spawn command.\n", buffer);
	return PMI_FAIL;
    }
	


    /* post the write of the command */
    /*
    printf("posting write of dbs command to %s context, sock %d: '%s'\n",
	smpd_get_context_str(pmi_process.context), sock_getid(pmi_process.context->sock), cmd_ptr->cmd);
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
	pmi_err_printf("the state machine logic failed to get the result of the spawn command.\n");
	return PMI_FAIL;
    }
    return PMI_FAIL;
}

int iPMI_Args_to_keyval(int *argcp, char ***argvp, PMI_keyval_t *keyvalp, int *size)
{
    return PMI_SUCCESS;
}
