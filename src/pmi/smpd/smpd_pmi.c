/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifdef USE_WINCONF_H
#include "winpmiconf.h"
#else
#include "pmiconf.h" 
#endif

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#include "pmi.h"
#include "sock.h"
#include "smpd.h"

/* pmiimpl.h */
#define PMI_MAX_KEY_LEN          256
#define PMI_MAX_VALUE_LEN        1024
#define PMI_MAX_KVS_NAME_LENGTH  100

#define PMI_SUCCESS     0
#define PMI_FAIL       -1

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
    void *smpd_key;
    smpd_context_t *context;
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
    NULL,               /* smpd_key       */
    NULL                /* context        */
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

int PMI_Init(int *spawned)
{
    char *p;
    int result;

    /* don't allow pmi_init to be called more than once */
    if (pmi_process.init_finalized == PMI_INITIALIZED)
	return PMI_SUCCESS;

    /* initialize to defaults */
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
    }

    p = getenv("PMI_SMPD_KEY");
    if (p != NULL)
    {
	pmi_process.smpd_key = (void*)atol(p);
    }

    p = getenv("PMI_SMPD_FD");
    if (p != NULL)
    {
	result = sock_init();
	if (result != SOCK_SUCCESS)
	{
	    pmi_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	    return PMI_FAIL;
	}
	pmi_process.smpd_fd = (SOCK_NATIVE_FD)atol(p);
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

    pmi_process.init_finalized = PMI_INITIALIZED;

    return PMI_SUCCESS;
}

int PMI_Finalize()
{
    int result;

    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    if (pmi_process.sock != SOCK_INVALID_SOCK)
    {
	result = sock_finalize();
	if (result != SOCK_SUCCESS)
	{
	    pmi_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
	}
    }

    pmi_process.init_finalized = PMI_FINALIZED;

    return PMI_SUCCESS;
}

int PMI_Get_size(int *size)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || size == NULL)
	return PMI_FAIL;

    *size = pmi_process.nproc;

    return PMI_SUCCESS;
}

int PMI_Get_rank(int *rank)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || rank == NULL)
	return PMI_FAIL;

    *rank = pmi_process.iproc;

    return PMI_SUCCESS;
}

int PMI_Barrier()
{
    char pszStr[256];
    
    if (pmi_process.init_finalized == PMI_FINALIZED)
	return PMI_FAIL;

    /*
    snprintf(pszStr, 256, "barrier name=%s count=%d", pmi_process.kvs_name, pmi_process.nproc);
    if (WriteString(g_bfdMPD, pszStr) == SOCKET_ERROR)
    {
	pmi_err_printf("PMI_Barrier: WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
	return PMI_FAIL;
    }
    if (!ReadString(g_bfdMPD, pszStr))
    {
	pmi_err_printf("PMI_Barrier: ReadString failed, error %d\n", WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(pszStr, "SUCCESS", 8) == 0)
	return PMI_SUCCESS;
    */

    pmi_err_printf("PMI_Barrier returned: '%s'\n", pszStr);
    return PMI_FAIL;
}

int PMI_KVS_Get_my_name(char *kvsname)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    strcpy(kvsname, pmi_process.kvs_name);

    return PMI_SUCCESS;
}

int PMI_KVS_Get_name_length_max()
{
    return PMI_MAX_KVS_NAME_LENGTH;
}

int PMI_KVS_Get_key_length_max()
{
    return PMI_MAX_KEY_LEN;
}

int PMI_KVS_Get_value_length_max()
{
    return PMI_MAX_VALUE_LEN;
}

int PMI_KVS_Create(char * kvsname)
{
    int result;
    smpd_command_t *cmd_ptr;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    /* This isn't going to work because the command id created here will conflict with
       command ids created by the smpd process? */

    /* create a command to send to the root smpd manager */
    result = smpd_create_command("dbcreate", pmi_process.smpd_id, 0, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a dbcreate command.\n");
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", (int)pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the dbcreate command.\n");
	return PMI_FAIL;
    }
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the dbcreate command.\n");
	return PMI_FAIL;
    }
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
	pmi_err_printf("the state machine logic failed to get the result of the dbcreate command.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR) == 0)
    {
	if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "kvs_name", str, 1024))
	{
	    pmi_err_printf("no kvs_name in the dbcreate result command.\n");
	    return PMI_FAIL;
	}
	strncpy(kvsname, str, PMI_MAX_KVS_NAME_LENGTH);
    }
    else
    {
	pmi_err_printf("dbcreate failed: %s\n", str);
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int PMI_KVS_Destroy(const char * kvsname)
{
    int result;
    smpd_command_t *cmd_ptr;
    char str[1024];

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    /* create a command to send to the root smpd manager */
    result = smpd_create_command("dbdestroy", pmi_process.smpd_id, 0, SMPD_TRUE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to create a dbdestroy command.\n");
	return PMI_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "ctx_key", (int)pmi_process.smpd_key);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the key to the dbdestroy command.\n");
	return PMI_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "kvs_name", (char*)kvsname);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to add the kvsname to the dbdestroy command.\n");
	return PMI_FAIL;
    }
    result = smpd_post_write_command(pmi_process.context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	pmi_err_printf("unable to post a write of the dbdestroy command.\n");
	return PMI_FAIL;
    }
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
	pmi_err_printf("the state machine logic failed to get the result of the dbdestroy command.\n");
	return PMI_FAIL;
    }

    /* parse the result of the command */
    if (!smpd_get_string_arg(pmi_process.context->read_cmd.cmd, "result", str, 1024))
    {
	pmi_err_printf("no result string in the result command.\n");
	return PMI_FAIL;
    }
    if (strcmp(str, DBS_SUCCESS_STR))
    {
	pmi_err_printf("dbdestroy failed: %s\n", str);
	return PMI_FAIL;
    }

    return PMI_FAIL;
}

int PMI_KVS_Put(const char *kvsname, const char *key, const char *value)
{
    char str[SMPD_MAX_CMD_LENGTH];
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    /*
    snprintf(str, SMPD_MAX_CMD_LENGTH, "dbput name=%s key='%s' value='%s'", kvsname, key, value);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	pmi_err_printf("PMI_KVS_Put: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	pmi_err_printf("PMI_KVS_Put('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (stricmp(str, DBS_SUCCESS_STR) == 0)
	return PMI_SUCCESS;
    */

    return PMI_SUCCESS;
}

int PMI_KVS_Commit(const char *kvsname)
{
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    return PMI_SUCCESS;
}

int PMI_KVS_Get(const char *kvsname, const char *key, char *value)
{
    char str[SMPD_MAX_CMD_LENGTH];
    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    /*
    snprintf(str, SMPD_MAX_CMD_LENGTH, "dbget name=%s key='%s'", kvsname, key);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	pmi_err_printf("PMI_KVS_Get: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, value))
    {
	pmi_err_printf("PMI_KVS_Get('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(value, DBS_FAIL_STR, strlen(DBS_FAIL_STR)+1) == 0)
	return PMI_FAIL;
    */

    return PMI_SUCCESS;
}

int PMI_KVS_Iter_first(const char *kvsname, char *key, char *value)
{
    char str[SMPD_MAX_CMD_LENGTH];
    char *token;

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    /*
    snprintf(str, SMPD_MAX_CMD_LENGTH, "dbfirst %s", kvsname);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	pmi_err_printf("PMI_KVS_Iter_first: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	pmi_err_printf("PMI_KVS_Iter_first('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(str, DBS_FAIL_STR, strlen(DBS_FAIL_STR)+1) == 0)
	return PMI_FAIL;
    
    *key = '\0';
    *value = '\0';
    if (strncmp(str, DBS_END_STR, strlen(DBS_END_STR)+1) == 0)
	return PMI_SUCCESS;
    token = strtok(str, "=");
    if (token == NULL)
	return PMI_FAIL;

    strcpy(key, str);
    strcpy(value, token);
    */

    return PMI_SUCCESS;
}

int PMI_KVS_Iter_next(const char *kvsname, char *key, char *value)
{
    char str[SMPD_MAX_CMD_LENGTH];
    char *token;

    if (pmi_process.init_finalized == PMI_FINALIZED || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    /*
    snprintf(str, SMPD_MAX_CMD_LENGTH, "dbnext %s", kvsname);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	pmi_err_printf("PMI_KVS_Iter_next: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	pmi_err_printf("PMI_KVS_Iter_next('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(str, DBS_FAIL_STR, strlen(DBS_FAIL_STR)+1) == 0)
	return PMI_FAIL;
    
    *key = '\0';
    *value = '\0';
    if (strncmp(str, DBS_END_STR, strlen(DBS_END_STR)+1) == 0)
	return PMI_SUCCESS;
    token = strtok(str, "=");
    if (token == NULL)
	return PMI_FAIL;

    strcpy(key, str);
    strcpy(value, token);
    */

    return PMI_SUCCESS;
}

int PMI_Spawn_multiple(int count, const char *cmds[], const char **argvs[], 
                       const int *maxprocs, const void *info, int *errors, 
                       int *same_domain, const void *preput_info)
{
    return PMI_FAIL;
}

int PMI_Args_to_info(int *argcp, char ***argvp, void *infop)
{
    return PMI_SUCCESS;
}
