/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "pmiimpl.h"
#include "mpdutil.h"
#include "mpd.h"
#include "bsocket.h"
#include "redirectio.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifndef err_printf
#define err_printf printf
#endif

char g_pszKVSName[PMI_MAX_KVS_NAME_LENGTH] = "";
char g_pszMPDHost[100] = "";
char g_pszPMIAccount[100] = "";
char g_pszPMIPassword[100] = "";
int g_nMPDPort = MPD_DEFAULT_PORT;
char g_pszMPDPhrase[MPD_PASSPHRASE_MAX_LENGTH] = MPD_DEFAULT_PASSPHRASE;
int g_bfdMPD = BFD_INVALID_SOCKET;
int g_nIproc = 0;
int g_nNproc = 1;
int g_bInitFinalized = PMI_FINALIZED;
BOOL g_bPMIFinalizeWaiting = FALSE;

int PMI_Init(int *spawned)
{
    char *p;
    int error;

    p = getenv("PMI_SPAWN");
    *spawned = (p != NULL) ? 1 : 0;

    if (g_bInitFinalized == PMI_INITIALIZED)
	return PMI_SUCCESS;

    bsocket_init();
    InitRedirection();

    p = getenv("PMI_KVS");
    if (p != NULL)
	strncpy(g_pszKVSName, p, PMI_MAX_KVS_NAME_LENGTH);

    p = getenv("PMI_MPD");
    if (p != NULL)
    {
	strncpy(g_pszMPDHost, p, 100);
	p = strtok(g_pszMPDHost, ":");
	if (p != NULL)
	{
	    p = strtok(NULL, " \t\n");
	    if (p != NULL)
		g_nMPDPort = atoi(p);
	}
    }
    else
    {
	p = getenv("PMI_MPD_HOST");
	if (p != NULL)
	    strncpy(g_pszMPDHost, p, 100);
	else
	{
	    gethostname(g_pszMPDHost, 100);
	}
	p = getenv("PMI_MPD_PORT");
	if (p != NULL)
	    g_nMPDPort = atoi(p);
    }

    p = getenv("PMI_PHRASE");
    if (p != NULL)
    {
	strncpy(g_pszMPDPhrase, p, MPD_PASSPHRASE_MAX_LENGTH);
	putenv("PMI_PHRASE="); /* erase the phrase from the environment*/
    }

    p = getenv("PMI_RANK");
    if (p != NULL)
	g_nIproc = atoi(p);

    p = getenv("PMI_SIZE");
    if (p != NULL)
	g_nNproc = atoi(p);

    p = getenv("PMI_USER");
    if (p != NULL)
    {
	strncpy(g_pszPMIAccount, p, 100);
	putenv("PMI_USER="); /* erase the user name from the environment*/
    }

    p = getenv("PMI_PWD");
    if (p != NULL)
    {
	strncpy(g_pszPMIPassword, p, 100);
	putenv("PMI_PWD="); /* erase the password from the environment*/
    }

    error = ConnectToMPD(g_pszMPDHost, g_nMPDPort, g_pszMPDPhrase, &g_bfdMPD);
    if (error)
    {
	return PMI_FAIL;
    }

    g_hSpawnMutex = CreateMutex(NULL, FALSE, NULL);

    g_bInitFinalized = PMI_INITIALIZED;

    return PMI_SUCCESS;
}

int PMI_Finalize()
{
    if (g_bInitFinalized == PMI_FINALIZED)
	return PMI_SUCCESS;

    /* Close the connection to the mpd, insuring no further spawn calls*/
    WaitForSingleObject(g_hSpawnMutex, 10000);
    WriteString(g_bfdMPD, "done");
    beasy_closesocket(g_bfdMPD);
    g_bfdMPD = BFD_INVALID_SOCKET;
    g_bPMIFinalizeWaiting = TRUE;
    ReleaseMutex(g_hSpawnMutex);

    /* Wait for all spawned jobs to complete*/
    if (g_nNumJobThreads > 0)
    {
	WaitForMultipleObjects(g_nNumJobThreads, g_hJobThreads, TRUE, INFINITE);
    }

    CloseHandle(g_hSpawnMutex);
    g_hSpawnMutex = NULL;

    bsocket_finalize();
    FinalizeRedirection();

    g_bInitFinalized = PMI_FINALIZED;

    return PMI_SUCCESS;
}

int PMI_Get_size(int *size)
{
    if (g_bInitFinalized == PMI_FINALIZED || size == NULL)
	return PMI_FAIL;

    *size = g_nNproc;

    return PMI_SUCCESS;
}

int PMI_Get_rank(int *rank)
{
    if (g_bInitFinalized == PMI_FINALIZED || rank == NULL)
	return PMI_FAIL;

    *rank = g_nIproc;

    return PMI_SUCCESS;
}

int PMI_Barrier()
{
    char pszStr[256];
    
    if (g_bInitFinalized == PMI_FINALIZED)
	return PMI_FAIL;

    snprintf(pszStr, 256, "barrier name=%s count=%d", g_pszKVSName, g_nNproc);
    if (WriteString(g_bfdMPD, pszStr) == SOCKET_ERROR)
    {
	err_printf("PMI_Barrier: WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
	return PMI_FAIL;
    }
    if (!ReadString(g_bfdMPD, pszStr))
    {
	err_printf("PMI_Barrier: ReadString failed, error %d\n", WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(pszStr, "SUCCESS", 8) == 0)
	return PMI_SUCCESS;

    err_printf("PMI_Barrier returned: '%s'\n", pszStr);
    return PMI_FAIL;
}

int PMI_KVS_Get_my_name(char *kvsname)
{
    if (g_bInitFinalized == PMI_FINALIZED || kvsname == NULL)
	return PMI_FAIL;

    strcpy(kvsname, g_pszKVSName);

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
    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL)
	return PMI_FAIL;

    if (WriteString(g_bfdMPD, "dbcreate") == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Create: WriteString('dbcreate') failed, error %d\n", WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, kvsname))
    {
	err_printf("PMI_KVS_Create: ReadString failed, error %d\n", WSAGetLastError());
	return PMI_FAIL;
    }

    return PMI_SUCCESS;
}

int PMI_KVS_Destroy(const char * kvsname)
{
    char str[PMI_MAX_KVS_NAME_LENGTH+20];

    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL)
	return PMI_FAIL;

    snprintf(str, PMI_MAX_KVS_NAME_LENGTH+20, "dbdestroy %s", kvsname);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Destroy: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	err_printf("PMI_KVS_Destroy('%s'): ReadString failed, error %d\n", kvsname, WSAGetLastError());
	return PMI_FAIL;
    }
    if (stricmp(str, DBS_SUCCESS_STR) == 0)
	return PMI_SUCCESS;

    return PMI_FAIL;
}

int PMI_KVS_Put(const char *kvsname, const char *key, const char *value)
{
    char str[MAX_CMD_LENGTH];
    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    snprintf(str, MAX_CMD_LENGTH, "dbput name=%s key='%s' value='%s'", kvsname, key, value);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Put: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	err_printf("PMI_KVS_Put('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (stricmp(str, DBS_SUCCESS_STR) == 0)
	return PMI_SUCCESS;

    return PMI_SUCCESS;
}

int PMI_KVS_Commit(const char *kvsname)
{
    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL)
	return PMI_FAIL;

    return PMI_SUCCESS;
}

int PMI_KVS_Get(const char *kvsname, const char *key, char *value)
{
    char str[MAX_CMD_LENGTH];
    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    snprintf(str, MAX_CMD_LENGTH, "dbget name=%s key='%s'", kvsname, key);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Get: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, value))
    {
	err_printf("PMI_KVS_Get('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }
    if (strncmp(value, DBS_FAIL_STR, strlen(DBS_FAIL_STR)+1) == 0)
	return PMI_FAIL;

    return PMI_SUCCESS;
}

int PMI_KVS_Iter_first(const char *kvsname, char *key, char *value)
{
    char str[MAX_CMD_LENGTH];
    char *token;

    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    snprintf(str, MAX_CMD_LENGTH, "dbfirst %s", kvsname);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Iter_first: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	err_printf("PMI_KVS_Iter_first('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
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

    return PMI_SUCCESS;
}

int PMI_KVS_Iter_next(const char *kvsname, char *key, char *value)
{
    char str[MAX_CMD_LENGTH];
    char *token;

    if (g_bInitFinalized == PMI_FINALIZED || g_bfdMPD == BFD_INVALID_SOCKET || kvsname == NULL || key == NULL || value == NULL)
	return PMI_FAIL;

    snprintf(str, MAX_CMD_LENGTH, "dbnext %s", kvsname);
    if (WriteString(g_bfdMPD, str) == SOCKET_ERROR)
    {
	err_printf("PMI_KVS_Iter_next: WriteString('%s') failed, error %d\n", str, WSAGetLastError());
	return PMI_FAIL;
    }

    if (!ReadString(g_bfdMPD, str))
    {
	err_printf("PMI_KVS_Iter_next('%s'): ReadString failed, error %d\n", str, WSAGetLastError());
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

    return PMI_SUCCESS;
}
