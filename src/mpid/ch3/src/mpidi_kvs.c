/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidimpl.h"

/*
 * This file provides a KVS name cache.  This is needed to implement the 
 * MPI-2 Dynamic Process functions because the KVS space is used by the 
 * processes to exchange connection information, and the current PMI 
 * design does not provide a way for processes in one PMI process group to 
 * access the KVS space of processes in another PMI process group.  To 
 * work around this limitation, accesses to the KVS space (at least after 
 * MPID_Init, where the accesses for setting up COMM_WORLD are within
 * the calling processes PMI process group) are made through these 
 * routines, which provide for local caching of data.
 *
 * These routines are not needed by channels that cannot implement the 
 * dynamic process functions.
 *
 * The routines are in two basic groups: routines to provide access to the
 * cache (e.g., MPIDI_KVS_Get) and routines to manage the cache.  The routines
 * to manage the cache are currently in mpidi_pg.c, which provides two routines
 * (MPIDI_PG_To_string and MPIDI_PG_Create_from_string) to encode and 
 * decode the KVS spaces associated with a (PMI) process group.
 * 
 */

/*
 * FIXME: These routines use an #ifdef USE_WIN_MUTEX_PROTECT in many places.
 * It is not clear why a mutex is required only for Windows, and only in 
 * some cases.  Who sets USE_WIN_MUTEX_PROTECT?  What are the assumptions 
 * about the multi-threadedness of these routines?
 */
/* FIXME: These are temps until the thread code is worked out */
#ifdef USE_WIN_MUTEX_PROTECT
#define MPIDI_KVS_MUTEX_LOCK WaitForSingleObject(kvs.hKVSMutex, INFINITE)
#define MPIDI_KVS_MUTEX_UNLOCK ReleaseMutex(kvs.hKVSMutex)
#else 
/* FIXME: Define these as empty for now */
#define MPIDI_KVS_MUTEX_LOCK 
#define MPIDI_KVS_MUTEX_UNLOCK 
#endif

/*
 * FIXME: The routines that have PMI_KVS counterparts should use the
 * identical calling sequence, so that code can easily switch between 
 * the two (e.g., with #define or with a switch to using function pointers).
 * Using different calling sequences unnecessarily restricts the flexibility
 * of the code.
 */

#ifdef MPIDI_DEV_IMPLEMENTS_KVS
#include "pmi.h"

typedef struct MPIDI_KVS_database_element_t
{
    char pszKey[MPIDI_MAX_KVS_KEY_LEN];
    char pszValue[MPIDI_MAX_KVS_VALUE_LEN];
    struct MPIDI_KVS_database_element_t *pNext;
} MPIDI_KVS_database_element_t;

typedef struct MPIDI_KVS_database_node_t
{
    char pszName[MPIDI_MAX_KVS_NAME_LEN];
    MPIDI_KVS_database_element_t *pData, *pIter;
    struct MPIDI_KVS_database_node_t *pNext;
} MPIDI_KVS_database_node_t;

typedef struct MPIDI_KVS_Global_t
{
#ifdef USE_WIN_MUTEX_PROTECT
    HANDLE kvs.hKVSMutex;
#endif
    int nInitKVSRefCount;
    MPIDI_KVS_database_node_t *pDatabase;
    MPIDI_KVS_database_node_t *pDatabaseIter;
} MPIDI_KVS_Global_t;

static MPIDI_KVS_Global_t kvs = { 0 };

/* FIXME: Why is get_uuid used/needed?  Will this set of routines ever
   need to create a *new* KVS name (assuming that the congecture is true 
   that this is a KVS cache only? */

#undef FCNAME
#define FCNAME "get_uuid"
static void get_uuid(char *str)
{
#ifdef HAVE_WINDOWS_H
#if 0
    UUID guid;
    char *rpcstr;
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    UuidCreate(&guid);
    UuidToString(&guid, &rpcstr);
    MPIU_Strncpy(str, rpcstr, MPIDI_MAX_KVS_NAME_LEN);
    RpcStringFree(&rpcstr);
#else
    UUID guid;
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    UuidCreate(&guid);
    MPIU_Snprintf(str, MPIDI_MAX_KVS_NAME_LEN,
	"%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
#endif
#elif defined(HAVE_CFUUIDCREATE)
    CFUUIDRef       myUUID;
    CFStringRef     myUUIDString;
    char            strBuffer[100];
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    myUUID = CFUUIDCreate(kCFAllocatorDefault);
    myUUIDString = CFUUIDCreateString(kCFAllocatorDefault, myUUID);/* This is the safest way to obtain a C string from a CFString.*/
    CFStringGetCString(myUUIDString, str, MPIDI_MAX_KVS_NAME_LEN, kCFStringEncodingASCII);
    CFRelease(myUUIDString);
#elif defined(HAVE_UUID_GENERATE)
    uuid_t guid;
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    uuid_generate(guid);
    uuid_unparse(guid, str);
#elif defined(HAVE_TIME)
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    MPIU_Snprintf(str, MPIDI_MAX_KVS_NAME_LEN, "%X%X%X%X", rand(), rand(), rand(), (unsigned int)time(NULL));
#else
    MPIDI_STATE_DECL(MPID_STATE_GET_UUID);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_UUID);
    MPIU_Snprintf(str, MPIDI_MAX_KVS_NAME_LEN, "%X%X%X%X", rand(), rand(), rand(), rand());
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_GET_UUID);
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Init"
int MPIDI_KVS_Init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_INIT);
#ifdef USE_WIN_MUTEX_PROTECT
    kvs.hKVSMutex = CreateMutex(NULL, FALSE, "MPIDI_KVS_MUTEX");
    if (kvs.hKVSMutex == NULL)
    {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**fail");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_INIT);
	return mpi_errno;
    }
    MPIDI_KVS_MUTEX_LOCK;
#endif
    kvs.nInitKVSRefCount++;
    MPIDI_KVS_MUTEX_UNLOCK;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_INIT);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Finalize"
int MPIDI_KVS_Finalize()
{
    MPIDI_KVS_database_node_t *pNode, *pNext;
    MPIDI_KVS_database_element_t *pElement;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_FINALIZE);

    MPIDI_KVS_MUTEX_LOCK;
    kvs.nInitKVSRefCount--;

    if (kvs.nInitKVSRefCount == 0)
    {
	pNode = kvs.pDatabase;
	while (pNode)
	{
	    pNext = pNode->pNext;

	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		MPIU_Free(pElement);
	    }
	    MPIU_Free(pNode);

	    pNode = pNext;
	}

	kvs.pDatabase = NULL;
	kvs.pDatabaseIter = NULL;

	MPIDI_KVS_MUTEX_UNLOCK;
#ifdef USE_WIN_MUTEX_PROTECT
	CloseHandle(kvs.hKVSMutex);
	kvs.hKVSMutex = NULL;
#endif
    }
    else
    {
	MPIDI_KVS_MUTEX_UNLOCK;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_FINALIZE);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Create"
int MPIDI_KVS_Create(char *name)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_KVS_database_node_t *pNode, *pNodeTest;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_CREATE);

    MPIDI_KVS_MUTEX_LOCK;

    pNode = kvs.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (MPIDI_KVS_database_node_t*)MPIU_Malloc(sizeof(MPIDI_KVS_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	kvs.pDatabase = (MPIDI_KVS_database_node_t*)MPIU_Malloc(sizeof(MPIDI_KVS_database_node_t));
	pNode = kvs.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    do
    {
	get_uuid(pNode->pszName);
	if (pNode->pszName[0] == '\0')
	{
	    MPIDI_KVS_MUTEX_UNLOCK;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_CREATE);
	    return mpi_errno;
	}
	pNodeTest = kvs.pDatabase;
	while (strcmp(pNodeTest->pszName, pNode->pszName) != 0)
	    pNodeTest = pNodeTest->pNext;
    } while (pNodeTest != pNode);
    MPIU_Strncpy(name, pNode->pszName, MPIDI_MAX_KVS_NAME_LEN);

    MPIDI_KVS_MUTEX_UNLOCK;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_CREATE);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Create_name_in"
int MPIDI_KVS_Create_name_in(char *name)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_KVS_database_node_t *pNode;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_CREATE_NAME_IN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_CREATE_NAME_IN);

    if (strlen(name) < 1 || strlen(name) > MPIDI_MAX_KVS_NAME_LEN)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_CREATE_NAME_IN);
	return mpi_errno;
    }

    MPIDI_KVS_MUTEX_LOCK;

    /* Check if the name already exists */
    pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    MPIDI_KVS_MUTEX_UNLOCK;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_CREATE_NAME_IN);
	    /*return MPIDI_KVS_FAIL;*/
	    /* Empty database? */
	    return MPI_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    pNode = kvs.pDatabase;
    if (pNode)
    {
	while (pNode->pNext)
	    pNode = pNode->pNext;
	pNode->pNext = (MPIDI_KVS_database_node_t*)MPIU_Malloc(sizeof(MPIDI_KVS_database_node_t));
	pNode = pNode->pNext;
    }
    else
    {
	kvs.pDatabase = (MPIDI_KVS_database_node_t*)MPIU_Malloc(sizeof(MPIDI_KVS_database_node_t));
	pNode = kvs.pDatabase;
    }
    pNode->pNext = NULL;
    pNode->pData = NULL;
    pNode->pIter = NULL;
    MPIU_Strncpy(pNode->pszName, name, MPIDI_MAX_KVS_NAME_LEN);
    
    MPIDI_KVS_MUTEX_UNLOCK;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_CREATE_NAME_IN);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Get"
int MPIDI_KVS_Get(const char *name, const char *key, char *value)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_KVS_database_node_t *pNode;
    MPIDI_KVS_database_element_t *pElement;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_GET);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_GET);

    MPIDI_KVS_MUTEX_LOCK;

    pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    /* FIXME: This routine assume that value has length MPIDI_MAX_KVS_VALUE_LEN, but there is no easy way to check this.  This is poor coding prctice. */
		    MPIU_Strncpy(value, pElement->pszValue, MPIDI_MAX_KVS_VALUE_LEN);
		    MPIDI_KVS_MUTEX_UNLOCK;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_GET);
		    return MPI_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	}
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    pmi_errno = PMI_KVS_Get(name, key, value, MPIDI_MAX_KVS_VALUE_LEN);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get",
					 "**pmi_kvs_get %s %s %d", name, key, pmi_errno);
	/* --END ERROR HANDLING-- */
    }
    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_GET);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Put"
int MPIDI_KVS_Put(const char *name, const char *key, const char *value)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_KVS_database_node_t *pNode;
    MPIDI_KVS_database_element_t *pElement;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_PUT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_PUT);

    MPIDI_KVS_MUTEX_LOCK;

    pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    pElement = pNode->pData;
	    while (pElement)
	    {
		if (strcmp(pElement->pszKey, key) == 0)
		{
		    MPIU_Strncpy(pElement->pszValue, value, MPIDI_MAX_KVS_VALUE_LEN);
		    MPIDI_KVS_MUTEX_LOCK;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_PUT);
		    return MPI_SUCCESS;
		}
		pElement = pElement->pNext;
	    }
	    pElement = (MPIDI_KVS_database_element_t*)MPIU_Malloc(sizeof(MPIDI_KVS_database_element_t));
	    pElement->pNext = pNode->pData;
	    MPIU_Strncpy(pElement->pszKey, key, MPIDI_MAX_KVS_KEY_LEN);
	    MPIU_Strncpy(pElement->pszValue, value, MPIDI_MAX_KVS_VALUE_LEN);
	    pNode->pData = pElement;
	    MPIDI_KVS_MUTEX_UNLOCK;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_PUT);
	    return MPI_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    pmi_errno = PMI_KVS_Put(name, key, value);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_put",
					 "**pmi_kvs_put %s %s %s %d", name, key, value, pmi_errno);
	/* --END ERROR HANDLING-- */
    }
    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_PUT);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Delete"
int MPIDI_KVS_Delete(const char *name, const char *key)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_KVS_database_node_t *pNode;
    MPIDI_KVS_database_element_t *pElement, *pElementTrailer;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_DELETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_DELETE);

    MPIDI_KVS_MUTEX_LOCK;

    pNode = kvs.pDatabase;
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
		    {
			pElementTrailer->pNext = pElement->pNext;
		    }
		    else
		    {
			pNode->pData = pElement->pNext;
		    }
		    MPIU_Free(pElement);
		    MPIDI_KVS_MUTEX_UNLOCK;
		    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_DELETE);
		    return MPI_SUCCESS;
		}
		pElementTrailer = pElement;
		pElement = pElement->pNext;
	    }
	    MPIDI_KVS_MUTEX_UNLOCK;
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_DELETE);
	    return mpi_errno;
	}
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_DELETE);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Destroy"
int MPIDI_KVS_Destroy(const char *name)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_KVS_database_node_t *pNode, *pNodeTrailer;
    MPIDI_KVS_database_element_t *pElement;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_DESTROY);

    MPIDI_KVS_MUTEX_LOCK;

    pNodeTrailer = pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    while (pNode->pData)
	    {
		pElement = pNode->pData;
		pNode->pData = pNode->pData->pNext;
		MPIU_Free(pElement);
	    }
	    if (pNodeTrailer == pNode)
	    {
		kvs.pDatabase = kvs.pDatabase->pNext;
	    }
	    else
	    {
		pNodeTrailer->pNext = pNode->pNext;
	    }
	    MPIU_Free(pNode);
	    MPIDI_KVS_MUTEX_UNLOCK;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_DESTROY);
	    return MPI_SUCCESS;
	}
	pNodeTrailer = pNode;
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    pmi_errno = PMI_KVS_Destroy(name);
    if (pmi_errno != PMI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_destroy",
					 "**pmi_kvs_destroy %d", pmi_errno);
	/* --END ERROR HANDLING-- */
    }
    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_DESTROY);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_First"
int MPIDI_KVS_First(const char *name, char *key, char *value)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_KVS_database_node_t *pNode;
    char *pmi_key = NULL, *pmi_value = NULL;
    int pmi_key_len_max, pmi_value_len_max;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_FIRST);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_FIRST);

    MPIDI_KVS_MUTEX_LOCK;

    if (key != NULL)
    {
	key[0] = '\0';
    }
    pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (key != NULL)
	    {
		if (pNode->pData)
		{
		    MPIU_Strncpy(key, pNode->pData->pszKey, MPIDI_MAX_KVS_KEY_LEN);
		    MPIU_Strncpy(value, pNode->pData->pszValue, MPIDI_MAX_KVS_VALUE_LEN);
		    pNode->pIter = pNode->pData->pNext;
		}
		else
		    key[0] = '\0';
	    }
	    else
	    {
		pNode->pIter = pNode->pData;
	    }
	    MPIDI_KVS_MUTEX_UNLOCK;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_FIRST);
	    return MPI_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    pmi_errno = PMI_KVS_Get_key_length_max(&pmi_key_len_max);
    pmi_errno = PMI_KVS_Get_value_length_max(&pmi_value_len_max);
    pmi_key = (char*)MPIU_Malloc(pmi_key_len_max);
    if (pmi_key == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
    pmi_value = (char*)MPIU_Malloc(pmi_value_len_max);
    if (pmi_value == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
    pmi_errno = PMI_KVS_Iter_first(name, pmi_key, pmi_key_len_max, pmi_value, pmi_value_len_max);
    if (pmi_errno == PMI_SUCCESS)
    {
	/* FIXME: check for truncation */
	MPIU_Strncpy(key, pmi_key, MPIDI_MAX_KVS_KEY_LEN);
	MPIU_Strncpy(value, pmi_value, MPIDI_MAX_KVS_VALUE_LEN);
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_iter_first",
					 "**pmi_kvs_iter_first %s %d", name, pmi_errno);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
fn_exit:
    if (pmi_key != NULL)
    {
	MPIU_Free(pmi_key);
    }
    if (pmi_value != NULL)
    {
	MPIU_Free(pmi_value);
    }
    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_FIRST);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Next"
int MPIDI_KVS_Next(const char *name, char *key, char *value)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_KVS_database_node_t *pNode;
    char *pmi_key = NULL, *pmi_value = NULL;
    int pmi_key_len_max, pmi_value_len_max;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_NEXT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_NEXT);

    MPIDI_KVS_MUTEX_LOCK;

    key[0] = '\0';
    pNode = kvs.pDatabase;
    while (pNode)
    {
	if (strcmp(pNode->pszName, name) == 0)
	{
	    if (pNode->pIter)
	    {
		MPIU_Strncpy(key, pNode->pIter->pszKey, MPIDI_MAX_KVS_KEY_LEN);
		MPIU_Strncpy(value, pNode->pIter->pszValue, MPIDI_MAX_KVS_VALUE_LEN);
		pNode->pIter = pNode->pIter->pNext;
	    }
	    else
	    {
		key[0] = '\0';
	    }
	    MPIDI_KVS_MUTEX_UNLOCK;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_NEXT);
	    return MPI_SUCCESS;
	}
	pNode = pNode->pNext;
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    pmi_errno = PMI_KVS_Get_key_length_max(&pmi_key_len_max);
    pmi_errno = PMI_KVS_Get_value_length_max(&pmi_value_len_max);
    pmi_key = (char*)MPIU_Malloc(pmi_key_len_max);
    if (pmi_key == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
    pmi_value = (char*)MPIU_Malloc(pmi_value_len_max);
    if (pmi_value == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
    pmi_errno = PMI_KVS_Iter_next(name, pmi_key, pmi_key_len_max, pmi_value, pmi_value_len_max);
    if (pmi_errno == PMI_SUCCESS)
    {
	/* FIXME: check for truncation */
	MPIU_Strncpy(key, pmi_key, MPIDI_MAX_KVS_KEY_LEN);
	MPIU_Strncpy(value, pmi_value, MPIDI_MAX_KVS_VALUE_LEN);
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_iter_next",
					 "**pmi_kvs_iter_next %s %d", name, pmi_errno);
	goto fn_exit;
	/* --END ERROR HANDLING-- */
    }
fn_exit:
    if (pmi_key != NULL)
    {
	MPIU_Free(pmi_key);
    }
    if (pmi_value != NULL)
    {
	MPIU_Free(pmi_value);
    }
    /*mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_NEXT);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Firstkvs"
int MPIDI_KVS_Firstkvs(char *name)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_FIRSTKVS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_FIRSTKVS);
    MPIDI_KVS_MUTEX_LOCK;

    kvs.pDatabaseIter = kvs.pDatabase;
    if (name != NULL)
    {
	if (kvs.pDatabaseIter)
	{
	    MPIU_Strncpy(name, kvs.pDatabaseIter->pszName, MPIDI_MAX_KVS_NAME_LEN);
	}
	else
	{
	    name[0] = '\0';
	}
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_FIRSTKVS);
    return MPI_SUCCESS;
}

#undef FCNAME
#define FCNAME "MPIDI_KVS_Nextkvs"
int MPIDI_KVS_Nextkvs(char *name)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_KVS_NEXTKVS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_KVS_NEXTKVS);
    MPIDI_KVS_MUTEX_LOCK;

    if (kvs.pDatabaseIter == NULL)
    {
	name[0] = '\0';
    }
    else
    {
	kvs.pDatabaseIter = kvs.pDatabaseIter->pNext;
	if (kvs.pDatabaseIter)
	{
	    MPIU_Strncpy(name, kvs.pDatabaseIter->pszName, MPIDI_MAX_KVS_NAME_LEN);
	}
	else
	{
	    name[0] = '\0';
	}
    }

    MPIDI_KVS_MUTEX_UNLOCK;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_KVS_NEXTKVS);
    return MPI_SUCCESS;
}

#endif
