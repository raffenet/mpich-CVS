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
#include <stdio.h>
#include "mpichinfo.h"
#include "redirectio.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifndef err_printf
#define err_printf printf
#endif

typedef struct Spawn_node_st
{
    int pid;
    int launchid;
    char fwd_host[100];
    int fwd_port;
} Spawn_node;

typedef struct Spawn_struct_st
{
    int m_nNproc;
    Spawn_node *m_pNode;
    int m_bfd;
    int m_bfdStop;
    HANDLE m_hRedirectIOThread;
    HANDLE m_hThread;
    struct Spawn_struct_st *m_pNext;
} Spawn_struct;

typedef struct HostNode_st
{
    char pszHost[100];
    int nSMPProcs;
    struct HostNode_st *pNext;
} HostNode;

Spawn_struct *g_pSpawnList = NULL;
char g_pszIOHost[100] = "";
int g_nIOPort = 0;
HANDLE g_hSpawnMutex = NULL;
HANDLE g_hJobThreads[100];
int g_nNumJobThreads = 0;

Spawn_struct* CreateSpawn_struct()
{
    Spawn_struct *p = (Spawn_struct*)malloc(sizeof(Spawn_struct));
    p->m_bfd = BFD_INVALID_SOCKET;
    p->m_bfdStop = BFD_INVALID_SOCKET;
    p->m_hRedirectIOThread = NULL;
    p->m_nNproc = 0;
    p->m_pNode = NULL;
    p->m_pNext = NULL;
    p->m_hThread = NULL;
    return p;
}

Spawn_struct* CreateSpawn_structn(int n)
{
    int i;
    Spawn_struct *p = CreateSpawn_struct();
    p->m_nNproc = n;
    p->m_pNode = (Spawn_node*)malloc(sizeof(Spawn_node)*n);
    for (i=0; i<n; i++)
    {
	p->m_pNode[i].fwd_host[0] = '\0';
	p->m_pNode[i].fwd_port = 0;
	p->m_pNode[i].launchid = -1;
	p->m_pNode[i].pid = 0;
    }
    return p;
}

void FreeSpawn_struct(Spawn_struct *p)
{
    p->m_nNproc = 0;
    if (p->m_pNode)
	free(p->m_pNode);
    p->m_pNode = NULL;
    if (p->m_hRedirectIOThread != NULL)
    {
	if (p->m_bfdStop != BFD_INVALID_SOCKET)
	{
	    beasy_send(p->m_bfdStop, "x", 1);
	    if (WaitForSingleObject(p->m_hRedirectIOThread, 10000) == WAIT_TIMEOUT)
		TerminateThread(p->m_hRedirectIOThread, 0);
	}
	else
	    TerminateThread(p->m_hRedirectIOThread, 0);
	CloseHandle(p->m_hRedirectIOThread);
    }
    p->m_hRedirectIOThread = NULL;
    if (p->m_bfd != BFD_INVALID_SOCKET)
    {
	beasy_closesocket(p->m_bfd);
	p->m_bfd = BFD_INVALID_SOCKET;
    }
    if (p->m_bfdStop != BFD_INVALID_SOCKET)
    {
	beasy_closesocket(p->m_bfdStop);
	p->m_bfdStop = BFD_INVALID_SOCKET;
    }
    p->m_pNext = NULL;

    /* don't touch the m_hThread member because PMI_Finalize may be waiting on it?*/
    if (!g_bPMIFinalizeWaiting && p->m_hThread != NULL)
    {
	CloseHandle(p->m_hThread);
	p->m_hThread = NULL;
    }
    free(p);
}

static BOOL GetHostsFromFile(char *pszFileName, HostNode **ppNode, int nNumWanted)
{
    FILE *fin;
    char buffer[1024] = "";
    char *pChar, *pChar2;
    HostNode *node = NULL, *list = NULL, *cur_node;
    int num_left;
    HostNode *n, *target;

    /* check the parameters*/
    if ((nNumWanted < 1) || (ppNode == NULL))
	return FALSE;

    /* open the file*/
    fin = fopen(pszFileName, "r");
    if (fin == NULL)
    {
	err_printf("Error: unable to open file '%s', error %d\n", pszFileName, GetLastError());
	return FALSE;
    }
    
    /* Read the host names from the file*/
    while (fgets(buffer, 1024, fin))
    {
	pChar = buffer;
	
	/* Advance over white space*/
	while (*pChar != '\0' && isspace(*pChar))
	    pChar++;
	if (*pChar == '#' || *pChar == '\0')
	    continue;
	
	/* Trim trailing white space*/
	pChar2 = &buffer[strlen(buffer)-1];
	while (isspace(*pChar2) && (pChar >= pChar))
	{
	    *pChar2 = '\0';
	    pChar2--;
	}
	
	/* If there is anything left on the line, consider it a host name*/
	if (strlen(pChar) > 0)
	{
	    node = (HostNode*)malloc(sizeof(HostNode));
	    node->nSMPProcs = 1;
	    node->pNext = NULL;
	    
	    /* Copy the host name*/
	    pChar2 = node->pszHost;
	    while (*pChar != '\0' && !isspace(*pChar))
	    {
		*pChar2 = *pChar;
		pChar++;
		pChar2++;
	    }
	    *pChar2 = '\0';
	    pChar2 = strtok(node->pszHost, ":");
	    pChar2 = strtok(NULL, "\n");
	    if (pChar2 != NULL)
	    {
		node->nSMPProcs = atoi(pChar2);
		if (node->nSMPProcs < 1)
		    node->nSMPProcs = 1;
	    }
	    else
	    {
		/* Advance over white space*/
		while (*pChar != '\0' && isspace(*pChar))
		    pChar++;
		/* Get the number of SMP processes*/
		if (*pChar != '\0')
		{
		    node->nSMPProcs = atoi(pChar);
		    if (node->nSMPProcs < 1)
			node->nSMPProcs = 1;
		}
	    }

	    if (list == NULL)
	    {
		list = node;
		cur_node = node;
	    }
	    else
	    {
		cur_node->pNext = node;
		cur_node = node;
	    }
	}
    }

    fclose(fin);

    if (list == NULL)
	return FALSE;

    /* Allocate the first host node*/
    node = (HostNode*)malloc(sizeof(HostNode));
    num_left = nNumWanted;
    n = list;
    target = node;

    /* add the nodes to the target list, cycling if necessary*/
    while (num_left)
    {
	target->pNext = NULL;
	strncpy(target->pszHost, n->pszHost, 100);
	if (num_left <= n->nSMPProcs)
	{
	    target->nSMPProcs = num_left;
	    num_left = 0;
	}
	else
	{
	    target->nSMPProcs = n->nSMPProcs;
	    num_left = num_left - n->nSMPProcs;
	}

	if (num_left)
	{
	    target->pNext = (HostNode*)malloc(sizeof(HostNode));
	    target = target->pNext;
	}

	n = n->pNext;
	if (n == NULL)
	    n = list;
    }
    
    /* free the list created from the file*/
    while (list)
    {
	n = list;
	list = list->pNext;
	free(n);
    }

    /* add the generated list to the end of the list passed in*/
    if (*ppNode == NULL)
    {
	*ppNode = node;
    }
    else
    {
	cur_node = *ppNode;
	while (cur_node->pNext != NULL)
	    cur_node = cur_node->pNext;
	cur_node->pNext = node;
    }

    return TRUE;
}

static void FreeHosts(HostNode *pNode)
{
    HostNode *n;
    while (pNode)
    {
	n = pNode;
	pNode = pNode->pNext;
	free(n);
    }
}

static void GetHost(HostNode *pList, int nRank, char *pszHost)
{
    nRank++;
    while (nRank > 0)
    {
	if (pList == NULL)
	    return;
	nRank = nRank - pList->nSMPProcs;
	if (nRank > 0)
	    pList = pList->pNext;
    }
    if (pList == NULL)
	return;
    strncpy(pszHost, pList->pszHost, 100);
}

static void CreateCommand(int count, const int *maxprocs, const char **cmds, const char ***argvs, int nIproc, char *pszCmd)
{
    int i = 0;
    const char **ppArg;

    nIproc++;
    while (nIproc > 0)
    {
	if (i >= count)
	    return;
	nIproc = nIproc - maxprocs[i];
	if (nIproc > 0)
	    i++;
    }
    if (i >= count)
	return;
    snprintf(pszCmd, 1024, "\"%s\"", cmds[i]);

    if (argvs == NULL)
	return;

    ppArg = argvs[i];
    while (ppArg)
    {
	strncat(pszCmd, " ", 2);
	strncat(pszCmd, *ppArg, 1024-strlen(pszCmd));
	ppArg++;
    }
}

static void RemoveSpawnThread(Spawn_struct *pSpawn)
{
    int i;
    if (g_bPMIFinalizeWaiting)
	return;
    WaitForSingleObject(g_hSpawnMutex, INFINITE);
    for (i=0; i<g_nNumJobThreads; i++)
    {
	if (g_hJobThreads[i] == pSpawn->m_hThread)
	{
	    g_nNumJobThreads--;
	    g_hJobThreads[i] = g_hJobThreads[g_nNumJobThreads];
	    CloseHandle(pSpawn->m_hThread);
	    pSpawn->m_hThread = NULL;
	    break;
	}
    }
    ReleaseMutex(g_hSpawnMutex);
}

static void RemoveSpawnStruct(Spawn_struct *pSpawn)
{
    Spawn_struct *p, *pTrailer;
    WaitForSingleObject(g_hSpawnMutex, INFINITE);

    if (pSpawn == g_pSpawnList)
    {
	g_pSpawnList = g_pSpawnList->m_pNext;
	ReleaseMutex(g_hSpawnMutex);
	return;
    }
    pTrailer = g_pSpawnList;
    p = g_pSpawnList->m_pNext;
    while (p)
    {
	if (p == pSpawn)
	{
	    pTrailer->m_pNext = p->m_pNext;
	    ReleaseMutex(g_hSpawnMutex);
	    return;
	}
	pTrailer = pTrailer->m_pNext;
	p = p->m_pNext;
    }
    ReleaseMutex(g_hSpawnMutex);
}

void SpawnWaitThread(Spawn_struct *pSpawn)
{
    int i,j;
    char pszStr[1024];
    int nPid;
    char *token;

    for (i=0; i<pSpawn->m_nNproc; i++)
    {
	snprintf(pszStr, 1024, "getexitcodewait %d", pSpawn->m_pNode[i].launchid);
	if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
	{
	    err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
	    RemoveSpawnThread(pSpawn);
	    RemoveSpawnStruct(pSpawn);
	    FreeSpawn_struct(pSpawn);
	    return;
	}
    }

    for (i=0; i<pSpawn->m_nNproc; i++)
    {
	if (!ReadString(pSpawn->m_bfd, pszStr))
	{
	    err_printf("ReadString(exitcode) failed, error %d\n", WSAGetLastError());
	    RemoveSpawnThread(pSpawn);
	    RemoveSpawnStruct(pSpawn);
	    FreeSpawn_struct(pSpawn);
	    return;
	}
	token = strtok(pszStr, ":");
	if (token != NULL)
	{
	    token = strtok(NULL, "\n");
	    if (token != NULL)
	    {
		nPid = atoi(token);
		for (j=0; j<pSpawn->m_nNproc; j++)
		{
		    if (pSpawn->m_pNode[j].pid == nPid)
		    {
			if ((j > 0) && ((pSpawn->m_nNproc/2) > j))
			{
			    snprintf(pszStr, 1024, "stopforwarder host=%s port=%d abort=no", pSpawn->m_pNode[j].fwd_host, pSpawn->m_pNode[j].fwd_port);
			    if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
			    {
				err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
				RemoveSpawnThread(pSpawn);
				RemoveSpawnStruct(pSpawn);
				FreeSpawn_struct(pSpawn);
				return;
			    }
			}
			snprintf(pszStr, 1024, "freeprocess %d", pSpawn->m_pNode[j].launchid);
			if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
			{
			    err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
			    RemoveSpawnThread(pSpawn);
			    RemoveSpawnStruct(pSpawn);
			    FreeSpawn_struct(pSpawn);
			    return;
			}
		    }
		}
	    }
	}
    }

    if (pSpawn->m_bfdStop != BFD_INVALID_SOCKET)
    {
	/* tell the redirection thread to stop*/
	pszStr[0] = 0;
	beasy_send(pSpawn->m_bfdStop, pszStr, 1);
    }

    if (pSpawn->m_hRedirectIOThread != NULL)
    {
	if (WaitForSingleObject(pSpawn->m_hRedirectIOThread, 10000) != WAIT_OBJECT_0)
	{
	    TerminateThread(pSpawn->m_hRedirectIOThread, 0);
	}
	CloseHandle(pSpawn->m_hRedirectIOThread);
	pSpawn->m_hRedirectIOThread = NULL;
    }

    WriteString(pSpawn->m_bfd, "done");
    beasy_closesocket(pSpawn->m_bfd);

    RemoveSpawnThread(pSpawn);
    RemoveSpawnStruct(pSpawn);
    FreeSpawn_struct(pSpawn);
}

int info_keyval_get(const int size, const PMI_keyval_t *array, const char *key, const int len, char *val, int *flag)
{
    int i;
    for (i=0; i<size; i++)
    {
	if (strcmp(array[i].key, key) == 0)
	{
	    if ((int)strlen(array[i].val) >= len)
	    {
		*flag = 0;
		return -1;
	    }
	    strcpy(val, array[i].val);
	    *flag = 1;
	    return 0;
	}
    }
    *flag = 0;
    return 0;
}

int PMI_Spawn_multiple(int count,
                       const char ** cmds,
                       const char *** argvs,
                       const int * maxprocs,
                       const int * info_keyval_sizes,
                       const PMI_keyval_t ** info_keyval_vectors,
                       int preput_keyval_size,
                       const PMI_keyval_t * preput_keyval_vector,
                       int * errors)
{
    char pszStr[4096];
    int nNproc, nIproc;
    int i, j, error;
    int nNumHostsNeeded = 0;
    char pszHost[100];
    char pszHostFile[MAX_PATH];
    HostNode *pHosts = NULL;
    int flag;
    Spawn_struct *pSpawn = NULL;
    char pszCmd[1024];
    char pszDb[100];
    char pszTemp[1024];
    HostNode *n;
    DWORD dwThreadId;

    /* should the user and password be passed in by info?*/
    /* should this information be in each info allowing for multiple user credentials?*/
    if (count > 0 && info_keyval_sizes && info_keyval_vectors)
    {
	if (info_keyval_get(info_keyval_sizes[0], info_keyval_vectors[0], "user", 100, g_pszPMIAccount, &flag))
	/*if (MPICH_Info_get(((MPICH_Info*)info)[0], "user", 100, g_pszPMIAccount, &flag) != MPICH_SUCCESS)*/
	{
	    err_printf("Error: info_keyval_get('user') failed\n");
	    return PMI_FAIL;
	}
	if (info_keyval_get(info_keyval_sizes[0], info_keyval_vectors[0], "password", 100, g_pszPMIPassword, &flag))
	/*if (MPICH_Info_get(((MPICH_Info*)info)[0], "password", 100, g_pszPMIPassword, &flag) != MPICH_SUCCESS)*/
	{
	    err_printf("Error: info_keyval_get('password') failed\n");
	    return PMI_FAIL;
	}
    }

    nNproc = 0;
    for (i=0; i<count; i++)
    {
	if (maxprocs[i] < 1)
	{
	    FreeHosts(pHosts);
	    return PMI_FAIL;
	}
	nNproc += maxprocs[i];
	flag = 0;
	if (info_keyval_get(info_keyval_sizes[i], info_keyval_vectors[i], "host", 100, pszHost, &flag))
	/*if (MPICH_Info_get(((MPICH_Info*)info)[i], "host", 100, pszHost, &flag) != MPICH_SUCCESS)*/
	{
	    err_printf("Error: info_keyval_get failed\n");
	    FreeHosts(pHosts);
	    return PMI_FAIL;
	}
	if (flag)
	{
	    /* user specified a single host*/
	    HostNode *n;
	    if (pHosts == NULL)
		pHosts = n = (HostNode*)malloc(sizeof(HostNode));
	    else
	    {
		n = pHosts;
		while (n->pNext != NULL)
		    n = n->pNext;
		n->pNext = (HostNode*)malloc(sizeof(HostNode));
		n = n->pNext;
	    }
	    for (j=0; j<maxprocs[i]; j++)
	    {
		n->nSMPProcs = 1;
		strncpy(n->pszHost, pszHost, 100);
		if (j<maxprocs[i]-1)
		{
		    n->pNext = (HostNode*)malloc(sizeof(HostNode));
		    n = n->pNext;
		}
		else
		    n->pNext = NULL;
	    }
	}
	else
	{
	    flag = 0;
	    if (info_keyval_get(info_keyval_sizes[i], info_keyval_vectors[i], "hostfile", MAX_PATH, pszHostFile, &flag))
	    /*if (MPICH_Info_get(((MPICH_Info*)info)[i], "hostfile", MAX_PATH, pszHostFile, &flag) != MPICH_SUCCESS)*/
	    {
		err_printf("Error: MPICH_Info_get failed\n");
		FreeHosts(pHosts);
		return PMI_FAIL;
	    }
	    if (flag)
	    {
		/* user specified a host file*/
		if (!GetHostsFromFile(pszHostFile, &pHosts, maxprocs[i]))
		{
		    FreeHosts(pHosts);
		    return PMI_FAIL;
		}
	    }
	    else
	    {
		/* user did not specify any hosts*/
		/* create a list of blank host nodes to be filled in later*/
		nNumHostsNeeded += maxprocs[i];
		if (pHosts == NULL)
		    pHosts = n = (HostNode*)malloc(sizeof(HostNode));
		else
		{
		    n = pHosts;
		    while (n->pNext != NULL)
			n = n->pNext;
		    n->pNext = (HostNode*)malloc(sizeof(HostNode));
		    n = n->pNext;
		}
		for (j=0; j<maxprocs[i]; j++)
		{
		    n->nSMPProcs = 1;
		    n->pszHost[0] = '\0';
		    if (j<maxprocs[i]-1)
		    {
			n->pNext = (HostNode*)malloc(sizeof(HostNode));
			n = n->pNext;
		    }
		    else
			n->pNext = NULL;
		}
	    }
	}
    }

    /* fill in the blank host nodes*/
    if (nNumHostsNeeded > 0)
    {
	snprintf(pszStr, 4096, "next %d", nNumHostsNeeded);
	if (WriteString(g_bfdMPD, pszStr) == SOCKET_ERROR)
	{
	    err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
	    FreeHosts(pHosts);
	    return PMI_FAIL;
	}
	n = pHosts;
	for (i=0; i<nNumHostsNeeded; i++)
	{
	    while (n->pszHost[0] != '\0')
		n = n->pNext;
	    if (!ReadString(g_bfdMPD, n->pszHost))
	    {
		err_printf("ReadString(next host) failed, error %d\n", WSAGetLastError());
		FreeHosts(pHosts);
		return PMI_FAIL;
	    }
	}
    }

    /* allocate a spawn structure to hold all the data structures for this spawn call*/
    pSpawn = CreateSpawn_structn(nNproc);
    if (pSpawn == NULL)
    {
	FreeHosts(pHosts);
	return PMI_FAIL;
    }

    /* give this spawn its own connection to the mpd*/
    error = ConnectToMPD(g_pszMPDHost, g_nMPDPort, g_pszMPDPhrase, &pSpawn->m_bfd);
    if (error)
    {
	FreeHosts(pHosts);
	return PMI_FAIL;
    }

    /* if there isn't already a host to redirect io to, create one*/
    if (g_pszIOHost[0] == '\0')
    {
	DWORD dwThreadId;
	HANDLE hEvent;
	RedirectIOArg *pArg = (RedirectIOArg*)malloc(sizeof(RedirectIOArg));
	pArg->hReadyEvent = hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	pArg->m_pbfdStopIOSignalSocket = &pSpawn->m_bfdStop;
	pSpawn->m_hRedirectIOThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RedirectIOThread, pArg, 0, &dwThreadId);
	if (pSpawn->m_hRedirectIOThread == NULL)
	{
	    err_printf("Error: Unable to create the redirect io thread, error %d\n", GetLastError());
	    FreeHosts(pHosts);
	    CloseHandle(hEvent);
	    free(pArg);
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	if (WaitForSingleObject(hEvent, 10000) == WAIT_TIMEOUT)
	{
	    err_printf("Error: timed out waiting for io redirection thread to initialize\n");
	    FreeHosts(pHosts);
	    CloseHandle(hEvent);
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	CloseHandle(hEvent);
    }
    strncpy(pSpawn->m_pNode[0].fwd_host, g_pszIOHost, 100);
    pSpawn->m_pNode[0].fwd_port = g_nIOPort;

    /* create a database for the spawned processes*/
    if (WriteString(pSpawn->m_bfd, "dbcreate") == SOCKET_ERROR)
    {
	err_printf("WriteString('dbcreate') failed, error %d\n", WSAGetLastError());
	FreeHosts(pHosts);
	FreeSpawn_struct(pSpawn);
	return PMI_FAIL;
    }
    if (!ReadString(pSpawn->m_bfd, pszDb))
    {
	err_printf("ReadString(db) failed, error %d\n", WSAGetLastError());
	FreeHosts(pHosts);
	FreeSpawn_struct(pSpawn);
	return PMI_FAIL;
    }

    /* pre-put any data provided into the spawnee's database*/
    for (i=0; i<preput_keyval_size; i++)
    {
	PMI_KVS_Put(pszDb, preput_keyval_vector[i].key, preput_keyval_vector[i].val);
    }

    /* launch each process*/
    for (nIproc = 0; nIproc < nNproc; nIproc++)
    {
	/* get the host name for this process*/
	GetHost(pHosts, nIproc, pszHost);
	/* create the command*/
	CreateCommand(count, maxprocs, cmds, argvs, nIproc, pszCmd);
	/* possibly start an io forwarder*/
	if ((nIproc > 0) && ((nNproc/2) > nIproc))
	{
	    snprintf(pszStr, 4096, "createforwarder host=%s forward=%s:%d",
		pszHost, pSpawn->m_pNode[(nIproc-1)/2].fwd_host, pSpawn->m_pNode[(nIproc-1)/2].fwd_port);
	    if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
	    {
		err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
		FreeHosts(pHosts);
		FreeSpawn_struct(pSpawn);
		return PMI_FAIL;
	    }
	    if (!ReadString(pSpawn->m_bfd, pszStr))
	    {
		err_printf("ReadString(forwarder port) failed, error %d\n", WSAGetLastError());
		FreeHosts(pHosts);
		FreeSpawn_struct(pSpawn);
		return PMI_FAIL;
	    }
	    strncpy(pSpawn->m_pNode[nIproc].fwd_host, pszHost, 100);
	    pSpawn->m_pNode[nIproc].fwd_port = atoi(pszStr);
	}
	/* create the command line*/
	snprintf(pszStr, 4096, "launch h=%s c='%s' 12=%s:%d k=%d e='PMI_SPAWN=yes|PMI_RANK=%d|PMI_SIZE=%d|PMI_KVS=%s|PMI_MPD=%s|PMI_IO=%s:%d",
	    pszHost, pszCmd, 
	    pSpawn->m_pNode[(nIproc-1)/2].fwd_host, pSpawn->m_pNode[(nIproc-1)/2].fwd_port,
	    nIproc, nIproc, nNproc, pszDb, pszHost, 
	    pSpawn->m_pNode[(nIproc-1)/2].fwd_host, pSpawn->m_pNode[(nIproc-1)/2].fwd_port);
	if (strlen(g_pszPMIAccount))
	{
	    snprintf(pszTemp, 1024, "|PMI_USER=%s|PMI_PWD=%s' a=%s p=%s",
		g_pszPMIAccount, g_pszPMIPassword, g_pszPMIAccount, g_pszPMIPassword);
	    strncat(pszStr, pszTemp, 4096-strlen(pszStr));
	}
	else
	{
	    strncat(pszStr, "'", 2);
	}
	/* write the launch command*/
	if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
	{
	    err_printf("WriteString('launch h=%s c='%s' ...') failed, error %d\n", pszHost, pszCmd, WSAGetLastError());
	    FreeHosts(pHosts);
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	if (!ReadString(pSpawn->m_bfd, pszStr))
	{
	    err_printf("ReadString(launchid) failed, error %d\n", WSAGetLastError());
	    FreeHosts(pHosts);
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	pSpawn->m_pNode[nIproc].launchid = atoi(pszStr);
    }
    FreeHosts(pHosts);
    pHosts = NULL;

    /* get the process ids*/
    for (i=0; i<nNproc; i++)
    {
	snprintf(pszStr, 4096, "getpid %d", pSpawn->m_pNode[i].launchid);
	if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
	{
	    err_printf("WriteString('%s') failed, error %d\n", pszStr, WSAGetLastError());
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	if (!ReadString(pSpawn->m_bfd, pszStr))
	{
	    err_printf("ReadString(pid) failed, error %d\n", WSAGetLastError());
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
	pSpawn->m_pNode[i].pid = atoi(pszStr);
	if (pSpawn->m_pNode[i].pid == -1)
	{
	    snprintf(pszStr, 4096, "geterror %d", pSpawn->m_pNode[i].launchid);
	    if (WriteString(pSpawn->m_bfd, pszStr) == SOCKET_ERROR)
	    {
		err_printf("Error: launching process %d failed, unable to determine the error.\nWriting the request for the error message failed, error %d", i, WSAGetLastError());
		FreeSpawn_struct(pSpawn);
		return PMI_FAIL;
	    }
	    if (!ReadString(pSpawn->m_bfd, pszStr))
	    {
		err_printf("Error: launching process %d failed, unable to determine the error.\nReading the error message failed, error %d", i, WSAGetLastError());
		FreeSpawn_struct(pSpawn);
		return PMI_FAIL;
	    }
	    err_printf("Error: launching process %d failed, %s\n", i, pszStr);
	    FreeSpawn_struct(pSpawn);
	    return PMI_FAIL;
	}
    }

    /* Start a thread to monitor the spawned processes until they all exit and all output has been redirected*/
    /* Add the spawn data structure to the global list*/
    WaitForSingleObject(g_hSpawnMutex, INFINITE);
    
    pSpawn->m_pNext = g_pSpawnList;
    g_pSpawnList = pSpawn;

    pSpawn->m_hThread = g_hJobThreads[g_nNumJobThreads] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SpawnWaitThread, pSpawn, 0, &dwThreadId);
    if (g_hJobThreads[g_nNumJobThreads] == NULL)
    {
	err_printf("Error: Unable to create the job wait thread, error %d\n", GetLastError());
	g_pSpawnList = pSpawn->m_pNext;
	ReleaseMutex(g_hSpawnMutex);
	FreeSpawn_struct(pSpawn);
	return PMI_FAIL;
    }
    g_nNumJobThreads++;

    ReleaseMutex(g_hSpawnMutex);

    return PMI_SUCCESS;
}

static void StripArgs(int *argc, char ***argv, int n)
{
    int i;
    if (n+1 > *argc)
    {
	err_printf("Error: cannot strip %d args, only %d left.\n", n, *argc-1);
    }
    for (i=n+1; i<=*argc; i++)
    {
	(*argv)[i-n] = (*argv)[i];
    }
    *argc -= n;
}

static void GetMPDPassPhrase(char *phrase)
{
    HANDLE hStdin;
    DWORD dwMode;

    fprintf(stderr, "mpd password: ");
    fflush(stderr);
    
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    gets(phrase);
    SetConsoleMode(hStdin, dwMode);
    
    fprintf(stderr, "\n");
}

int PMI_Args_to_info(int *argcp, char ***argvp, void *infop)
{
    int nArgsToStrip;
    int argc;
    char **argv;
    char phrase[MPD_PASSPHRASE_MAX_LENGTH + 1];/* = MPD_DEFAULT_PASSPHRASE;*/

    argc = *argcp;
    argv = *argvp;
    while (argv[1] && (argv[1][0] == '-' || argv[1][0] == '/'))
    {
	nArgsToStrip = 1;
	if (strncmp(&argv[1][1], "localonly", 10) == 0)
	{
	    MPICH_Info_set(infop, "localonly", "true");
	}
	else if (strncmp(&argv[1][1], "machinefile", 12) == 0)
	{
	    if (argc < 3)
	    {
		err_printf("Error: no filename specified after -machinefile option.\n");
		return 0;
	    }
	    MPICH_Info_set(infop, "machinefile", argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "map", 4) == 0)
	{
	    if (argc < 3)
	    {
		err_printf("Error: no drive specified after -map option.\n");
		return 0;
	    }
	    if ((strlen(argv[2]) > 2) && argv[2][1] == ':')
	    {
		/* add code to read the map key and append this value to it to allow for multiple mappings */
		MPICH_Info_set(infop, "map", argv[2]);
	    }
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "env", 4) == 0)
	{
	    if (argc < 3)
	    {
		err_printf("Error: no environment variables after -env option\n");
		return 0;
	    }
	    MPICH_Info_set(infop, "env", argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strncmp(&argv[1][1], "logon", 6) == 0)
	{
	    MPICH_Info_set(infop, "logon", "true");
	}
	else if (strncmp(&argv[1][1], "tcp", 4) == 0)
	{
	    MPICH_Info_set(infop, "smp", "false");
	}
	else if (strncmp(&argv[1][1], "getphrase", 10) == 0)
	{
	    GetMPDPassPhrase(phrase);
	    MPICH_Info_set(infop, "phrase", phrase);
	}
	else if (strncmp(&argv[1][1], "nocolor", 8) == 0)
	{
	    MPICH_Info_set(infop, "color", "false");
	}
	else if (strncmp(&argv[1][1], "nompi", 6) == 0)
	{
	    MPICH_Info_set(infop, "nompi", "true");
	}
	else if (strncmp(&argv[1][1], "nodots", 7) == 0)
	{
	    MPICH_Info_set(infop, "logondots", "false");
	}
	else if (strncmp(&argv[1][1], "nomapping", 10) == 0)
	{
	    MPICH_Info_set(infop, "mapping", "false");
	}
	else
	{
	    /* skip over unknown argument */
	    argc--;
	    argc--;
	    argv++;
	    argv++;
	    continue;
	}
	StripArgs(argcp, argvp, nArgsToStrip);
	argc -= nArgsToStrip;
	argv = *argvp;
    }

    return PMI_SUCCESS;
}
