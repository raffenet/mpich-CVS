/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd_dbs_internal.h"

int smpd_dbs_init()
{
    smpd_enter_fn("smpd_dbs_init");
#ifdef USE_WIN_MUTEX_PROTECT
    if (smpd_process.nInitDBSRefCount == 0)
    {
	smpd_process.hDBSMutex = CreateMutex(NULL, FALSE, NULL);
    }
#endif
    smpd_process.nInitDBSRefCount++;
    smpd_exit_fn("smpd_dbs_init");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_finalize()
{
    smpd_database_node_t *pNode, *pNext;
    smpd_database_element_t *pElement;

    smpd_enter_fn("smpd_dbs_finalize");

    smpd_process.nInitDBSRefCount--;

    if (smpd_process.nInitDBSRefCount == 0)
    {
#ifdef USE_WIN_MUTEX_PROTECT
	WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

	pNode = smpd_process.pDatabase;
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

	smpd_process.pDatabase = NULL;
	smpd_process.pDatabaseIter = NULL;

#ifdef USE_WIN_MUTEX_PROTECT
	ReleaseMutex(smpd_process.hDBSMutex);
	CloseHandle(smpd_process.hDBSMutex);
	smpd_process.hDBSMutex = NULL;
#endif
    }

    smpd_exit_fn("smpd_dbs_finalize");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_create(char *name)
{
    smpd_database_node_t *pNode, *pNodeTest;

    smpd_enter_fn("smpd_dbs_create");

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	smpd_process.pDatabase = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = smpd_process.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    do
    {
	sprintf(pNode->pszName, "%d", smpd_process.nNextAvailableDBSID);
	smpd_process.nNextAvailableDBSID++;
	pNodeTest = smpd_process.pDatabase;
	while (strcmp(pNodeTest->pszName, pNode->pszName) != 0)
	    pNodeTest = pNodeTest->pNext;
    } while (pNodeTest != pNode);
    strcpy(name, pNode->pszName);

#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_create");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_create_name_in(char *name)
{
    smpd_database_node_t *pNode;

    smpd_enter_fn("smpd_dbs_create_name_in");

    if (strlen(name) < 1 || strlen(name) > SMPD_MAX_DBS_NAME_LEN)
    {
	smpd_exit_fn("smpd_dbs_create_name_in");
	return SMPD_DBS_FAIL;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    /* Lock */
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    /* Check if the name already exists */
    pNode = smpd_process.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
#ifdef USE_WIN_MUTEX_PROTECT
	    /* Unlock */
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    /*return SMPD_DBS_FAIL;*/
	    /* Empty database? */
	    smpd_exit_fn("smpd_dbs_create_name_in");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    pNode = smpd_process.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	smpd_process.pDatabase = (smpd_database_node_t*)malloc(sizeof(smpd_database_node_t));
	pNode = smpd_process.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    strcpy(pNode->pszName, name);
    
#ifdef USE_WIN_MUTEX_PROTECT
    /* Unlock */
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_create_name_in");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_get(char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement;

    smpd_enter_fn("smpd_dbs_get");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
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
		    ReleaseMutex(smpd_process.hDBSMutex);
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
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_get");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_put(char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement;

    smpd_enter_fn("smpd_dbs_put");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
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
		    ReleaseMutex(smpd_process.hDBSMutex);
#endif
		    smpd_exit_fn("smpd_dbs_put");
		    return SMPD_DBS_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	    pElement = (smpd_database_element_t*)malloc(sizeof(smpd_database_element_t));
	    pElement->pNext = pNode->pData;
	    strcpy(pElement->pszKey, key);
	    strcpy(pElement->pszValue, value);
	    pNode->pData = pElement;
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn("smpd_dbs_put");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_put");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_delete(char *name, char *key)
{
    smpd_database_node_t *pNode;
    smpd_database_element_t *pElement, *pElementTrailer;

    smpd_enter_fn("smpd_dbs_delete");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNode = smpd_process.pDatabase;
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
		    ReleaseMutex(smpd_process.hDBSMutex);
#endif
		    smpd_exit_fn("smpd_dbs_delete");
		    return SMPD_DBS_SUCCESS;
		}
		pElementTrailer = pElement;
		pElement = pElement->pNext;
	    }
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn("smpd_dbs_delete");
	    return SMPD_DBS_FAIL;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_delete");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_destroy(char *name)
{
    smpd_database_node_t *pNode, *pNodeTrailer;
    smpd_database_element_t *pElement;

    smpd_enter_fn("smpd_dbs_destroy");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    pNodeTrailer = pNode = smpd_process.pDatabase;
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
		smpd_process.pDatabase = smpd_process.pDatabase->pNext;
	    else
		pNodeTrailer->pNext = pNode->pNext;
	    free(pNode);
#ifdef USE_WIN_MUTEX_PROTECT
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn("smpd_dbs_destroy");
	    return SMPD_DBS_SUCCESS;
	}
	pNodeTrailer = pNode;
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_destroy");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_first(char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;

    smpd_enter_fn("smpd_dbs_first");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    if (key != NULL)
	key[0] = '\0';
    pNode = smpd_process.pDatabase;
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
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn("smpd_dbs_first");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_first");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_next(char *name, char *key, char *value)
{
    smpd_database_node_t *pNode;

    smpd_enter_fn("smpd_dbs_next");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    key[0] = '\0';
    pNode = smpd_process.pDatabase;
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
	    ReleaseMutex(smpd_process.hDBSMutex);
#endif
	    smpd_exit_fn("smpd_dbs_next");
	    return SMPD_DBS_SUCCESS;
	}
	pNode = pNode->pNext;
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_next");
    return SMPD_DBS_FAIL;
}

int smpd_dbs_firstdb(char *name)
{
    smpd_enter_fn("smpd_dbs_firstdb");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    smpd_process.pDatabaseIter = smpd_process.pDatabase;
    if (name != NULL)
    {
	if (smpd_process.pDatabaseIter)
	    strcpy(name, smpd_process.pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_firstdb");
    return SMPD_DBS_SUCCESS;
}

int smpd_dbs_nextdb(char *name)
{
    smpd_enter_fn("smpd_dbs_nextdb");

#ifdef USE_WIN_MUTEX_PROTECT
    WaitForSingleObject(smpd_process.hDBSMutex, INFINITE);
#endif

    if (smpd_process.pDatabaseIter == NULL)
	name[0] = '\0';
    else
    {
	smpd_process.pDatabaseIter = smpd_process.pDatabaseIter->pNext;
	if (smpd_process.pDatabaseIter)
	    strcpy(name, smpd_process.pDatabaseIter->pszName);
	else
	    name[0] = '\0';
    }

#ifdef USE_WIN_MUTEX_PROTECT
    ReleaseMutex(smpd_process.hDBSMutex);
#endif

    smpd_exit_fn("smpd_dbs_nextdb");
    return SMPD_DBS_SUCCESS;
}
