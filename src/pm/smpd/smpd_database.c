/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd_dbs_internal.h"

#ifdef USE_WIN_MUTEX_PROTECT
HANDLE g_hMutex = NULL;
#endif
static DatabaseNode *g_pDatabase = NULL;
static DatabaseNode *g_pDatabaseIter = NULL;
static int g_nNextAvailableID = 0;
static int s_nInitRefCount = 0;

int smpd_dbs_init()
{
    smpd_enter_fn("smpd_dbs_init");
#ifdef USE_WIN_MUTEX_PROTECT
    if (s_nInitRefCount == 0)
    {
	g_hMutex = CreateMutex(NULL, FALSE, NULL);
    }
#endif
    s_nInitRefCount++;
    smpd_exit_fn("smpd_dbs_init");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_finalize()
{
    DatabaseNode *pNode, *pNext;
    DatabaseElement *pElement;

    smpd_enter_fn("smpd_dbs_finalize");

    s_nInitRefCount--;

    if (s_nInitRefCount == 0)
    {
#ifdef USE_WIN_MUTEX_PROTECT
	WaitForSingleObject(g_hMutex, INFINITE);
#endif

	pNode = g_pDatabase;
	while (pNode)
	{
	    pNext = pNode->pNext;

	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		free(pElement);
	    }
	    free(pNode);

	    pNode = pNext;
	}

	g_pDatabase = NULL;
	g_pDatabaseIter = NULL;

#ifdef USE_WIN_MUTEX_PROTECT
	ReleaseMutex(g_hMutex);
	CloseHandle(g_hMutex);
	g_hMutex = NULL;
#endif
    }

    smpd_exit_fn("smpd_dbs_finalize");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_create(char *name)
{
    DatabaseNode *pNode, *pNodeTest;

    smpd_enter_fn("smpd_dbs_create");

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    pNode = g_pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (DatabaseNode*)malloc(sizeof(DatabaseNode));
	pNode = pNode->pNext;
    }
    else
    {
	g_pDatabase = (DatabaseNode*)malloc(sizeof(DatabaseNode));
	pNode = g_pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    do
    {
	sprintf(pNode->pszName, "%d", g_nNextAvailableID);
	g_nNextAvailableID++;
	pNodeTest = g_pDatabase;
	while (strcmp(pNodeTest->pszName, pNode->pszName) != 0)
	    pNodeTest = pNodeTest->pNext;
    } while (pNodeTest != pNode);
    strcpy(name, pNode->pszName);

#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_create");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_create_name_in(char *name)
{
    DatabaseNode *pNode;

    smpd_enter_fn("smpd_dbs_create_name_in");

    if (strlen(name) < 1 || strlen(name) > SMPD_MAX_DBS_NAME_LEN)
    {
	smpd_exit_fn("smpd_dbs_create_name_in");
	return SMPD_DBS_FAIL;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    /* Check if the name already exists */
    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
#ifdef USE_WIN_MUTEX_PROTECT
	    /* Unlock */
	    ReleaseMutex(g_hMutex);
#endif
	    /*return SMPD_DBS_FAIL;*/
	    /* Empty database? */
	    smpd_exit_fn("smpd_dbs_create_name_in");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    pNode = g_pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (DatabaseNode*)malloc(sizeof(DatabaseNode));
	pNode = pNode->pNext;
    }
    else
    {
	g_pDatabase = (DatabaseNode*)malloc(sizeof(DatabaseNode));
	pNode = g_pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    strcpy(pNode->pszName, name);
    
#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_create_name_in");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_get(char *name, char *key, char *value)
{
    DatabaseNode *pNode;
    DatabaseElement *pElement;

    smpd_enter_fn("smpd_dbs_get");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    strcpy(value, pElement->pszValue);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(g_hMutex);
#endif
		    smpd_exit_fn("smpd_dbs_get");
		    return SMPD_DBS_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_get");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_put(char *name, char *key, char *value)
{
    DatabaseNode *pNode;
    DatabaseElement *pElement;

    smpd_enter_fn("smpd_dbs_put");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    strcpy(pElement->pszValue, value);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(g_hMutex);
#endif
		    smpd_exit_fn("smpd_dbs_put");
		    return SMPD_DBS_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	    pElement = (DatabaseElement*)malloc(sizeof(DatabaseElement));
	    pElement->pNext = pNode->pData;
	    strcpy(pElement->pszKey, key);
	    strcpy(pElement->pszValue, value);
	    pNode->pData = pElement;
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(g_hMutex);
#endif
	    smpd_exit_fn("smpd_dbs_put");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_put");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_delete(char *name, char *key)
{
    DatabaseNode *pNode;
    DatabaseElement *pElement, *pElementTrailer;

    smpd_enter_fn("smpd_dbs_delete");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElementTrailer = pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    if (pElementTrailer != pElement)
			pElementTrailer->pNext = pElement->pNext;
		    else
			pNode->pData = pElement->pNext;
		    free(pElement);
#ifdef USE_WIN_MUTEX_PROTECT
		    ReleaseMutex(g_hMutex);
#endif
		    smpd_exit_fn("smpd_dbs_delete");
		    return SMPD_DBS_SUCCESS;
		}
		pElementTrailer = pElement;
		pElement = pElement->pNext;
	    }
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(g_hMutex);
#endif
	    smpd_exit_fn("smpd_dbs_delete");
	    return SMPD_DBS_FAIL;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_delete");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_destroy(char *name)
{
    DatabaseNode *pNode, *pNodeTrailer;
    DatabaseElement *pElement;

    smpd_enter_fn("smpd_dbs_destroy");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    pNodeTrailer = pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		free(pElement);
	    }
	    if (pNodeTrailer == pNode)
		g_pDatabase = g_pDatabase->pNext;
	    else
		pNodeTrailer->pNext = pNode->pNext;
	    free(pNode);
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(g_hMutex);
#endif
	    smpd_exit_fn("smpd_dbs_destroy");
	    return SMPD_DBS_SUCCESS;
	}
	pNodeTrailer = pNode;
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_destroy");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_first(char *name, char *key, char *value)
{
    DatabaseNode *pNode;

    smpd_enter_fn("smpd_dbs_first");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    if (key != NULL)
	key[0] = '\0';
    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (key != NULL)
	    {
		if (pNode->pData)
		{
		    strcpy(key, pNode->pData->pszKey);
		    strcpy(value, pNode->pData->pszValue);
		    pNode->pIter = pNode->pData->pNext;
		}
		else
		    key[0] = '\0';
	    }
	    else
	    {
		pNode->pIter = pNode->pData;
	    }
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(g_hMutex);
#endif
	    smpd_exit_fn("smpd_dbs_first");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_first");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_next(char *name, char *key, char *value)
{
    DatabaseNode *pNode;

    smpd_enter_fn("smpd_dbs_next");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    key[0] = '\0';
    pNode = g_pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (pNode->pIter)
	    {
		strcpy(key, pNode->pIter->pszKey);
		strcpy(value, pNode->pIter->pszValue);
		pNode->pIter = pNode->pIter->pNext;
	    }
	    else
		key[0] = '\0';
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(g_hMutex);
#endif
	    smpd_exit_fn("smpd_dbs_next");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_next");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_firstdb(char *name)
{
    smpd_enter_fn("smpd_dbs_firstdb");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    g_pDatabaseIter = g_pDatabase;
    if (name != NULL)
    {
	if (g_pDatabaseIter)
	    strcpy(name, g_pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_firstdb");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_nextdb(char *name)
{
    smpd_enter_fn("smpd_dbs_nextdb");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(g_hMutex, INFINITE);
#endif

    if (g_pDatabaseIter == NULL)
	name[0] = '\0';
    else
    {
	g_pDatabaseIter = g_pDatabaseIter->pNext;
	if (g_pDatabaseIter)
	    strcpy(name, g_pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(g_hMutex);
#endif

    smpd_exit_fn("smpd_dbs_nextdb");
    return SMPD_DBS_SUCCESS;
}
