/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#define ZERO_RANK 0x10101010

#undef SYNCHRONIZE_SHMAPPING

#ifdef HAVE_SHARED_PROCESS_READ
static void InitSharedProcesses(MPIDI_CH3I_Process_group_t *pg)
{
    int mpi_errno;
#ifndef HAVE_WINDOWS_H
    char filename[256];
#endif
    int i;
    MPIDI_CH3I_Shared_process_t *pSharedProcess;
    int nRank, nProc;

    nRank = pg->rank;
    nProc = pg->size;

    /* initialize arrays */
#ifdef HAVE_WINDOWS_H
    pg->pSharedProcessHandles = (HANDLE*)MPIU_Malloc(sizeof(HANDLE) * pg->size);
#else
    pg->pSharedProcessIDs = (int*)MPIU_Malloc(sizeof(int) * pg->size);
    pg->pSharedProcessFileDescriptors = (int*)MPIU_Malloc(sizeof(int) * pg->size);
#endif

    pSharedProcess = pg->pSHP;

#ifdef HAVE_WINDOWS_H
    pSharedProcess[nRank].nPid = GetCurrentProcessId();
#else
    pSharedProcess[nRank].nPid = getpid();
#endif
    pSharedProcess[nRank].bFinished = FALSE;
    if (nRank == 0)
	pSharedProcess[nRank].nRank = ZERO_RANK;
    else
	pSharedProcess[nRank].nRank = nRank;

    for (i=0; i<nProc; i++)
    {
        if (i != nRank)
        {
	    if (i == 0)
	    {
		while (pSharedProcess[i].nRank != ZERO_RANK)
		    MPIDU_Yield();
	    }
	    else
	    {
		while (pSharedProcess[i].nRank != i)
		    MPIDU_Yield();
	    }
#ifdef HAVE_WINDOWS_H
            /*MPIU_DBG_PRINTF(("Opening process[%d]: %d\n", i, pSharedProcess[i].nPid));*/
            pg->pSharedProcessHandles[i] =
                OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
                            FALSE, pSharedProcess[i].nPid);
            if (pg->pSharedProcessHandles[i] == NULL)
            {
                int err = GetLastError();
                mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|OpenProcess", "**ch3|shm|OpenProcess %d %d", i, err); /*"unable to open process %d, error %d\n", i, err);*/
            }
#else
            sprintf(filename, "/proc/%d/mem", pSharedProcess[i].nPid);
            pg->pSharedProcessIDs[i] = pSharedProcess[i].nPid;
            pg->pSharedProcessFileDescriptors[i] = open(filename, O_RDONLY);
            if (pg->pSharedProcessFileDescriptors[i] == -1)
	    {
                mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|open", "**ch3|shm|open %s %d", filename, pSharedProcess[i].nPid); /*"failed to open mem file, '%s', for process %d\n", filename, pSharedProcess[i].nPid);*/
	    }
#endif
        }
        else
        {
#ifdef HAVE_WINDOWS_H
            pg->pSharedProcessHandles[i] = NULL;
#else
            pg->pSharedProcessIDs[i] = 0;
            pg->pSharedProcessFileDescriptors[i] = 0;
#endif
        }
    }
    if (nRank == 0)
    {
        for (i=1; i<nProc; i++)
        {
            while (pSharedProcess[i].bFinished != TRUE)
                MPIDU_Yield();
            pSharedProcess[i].nPid = -1;
            pSharedProcess[i].bFinished = -1;
            pSharedProcess[i].nRank = -1;
        }
        pSharedProcess[0].nPid = -1;
        pSharedProcess[0].nRank = -1;
        pSharedProcess[0].bFinished = TRUE;
    }
    else
    {
        pSharedProcess[nRank].bFinished = TRUE;
        while (pSharedProcess[0].bFinished == FALSE)
            MPIDU_Yield();
    }
}
#endif

/*@
   MPIDI_CH3I_SHM_Get_mem - allocate and get address and size of memory shared by all processes. 

   Parameters:
+  int nTotalSize
.  int nRank
-  int nNproc

   Notes:
    Set the global variables pg->addr, pg->size, pg->id
@*/
void *MPIDI_CH3I_SHM_Get_mem(MPIDI_CH3I_Process_group_t *pg, int nTotalSize, int nRank, int nNproc, BOOL bUseShm)
{
    int mpi_errno;
#ifdef HAVE_SHARED_PROCESS_READ
    int shp_offset;
#endif
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    HANDLE hSyncEvent1, hSyncEvent2;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

#ifdef HAVE_SHARED_PROCESS_READ
    /* add room at the end of the shard memory region for the shared process information */
    shp_offset = nTotalSize;
    nTotalSize += nNproc * sizeof(MPIDI_CH3I_Shared_process_t);
#endif

#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    hSyncEvent1 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent1");
    hSyncEvent2 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent2");
#endif

    if (nTotalSize < 1)
    {
	MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return NULL;
    }

    if (bUseShm)
    {
	/* Create the shared memory object */
#ifdef HAVE_SHMGET
	pg->id = shmget(pg->key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
	if (pg->id == -1) 
	{
	    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|shmget", "**ch3|shm|shmget %d", errno); /*"Error in shmget\n");*/
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined (HAVE_CREATEFILEMAPPING)
	pg->id = CreateFileMapping(
	    INVALID_HANDLE_VALUE,
	    NULL,
	    PAGE_READWRITE,
	    0, 
	    nTotalSize,
	    pg->key);
	if (pg->id == NULL) 
	{
	    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|CreateFileMapping", "**ch3|shm|CreateFileMapping %d", GetLastError()); /*"Error in CreateFileMapping, %d\n", GetLastError());*/
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#else
#error *** No shared memory allocation function specified ***
#endif

	/* Get the shmem pointer */
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
	if (nRank == 0)
	{
	    ResetEvent(hSyncEvent2);
	    SetEvent(hSyncEvent1);
	    WaitForSingleObject(hSyncEvent2, INFINITE);
	}
	else
	{
	    WaitForSingleObject(hSyncEvent1, INFINITE);
	    ResetEvent(hSyncEvent1);
	    SetEvent(hSyncEvent2);
	}
#endif
	pg->addr = NULL;
	MPIU_DBG_PRINTF(("[%d] mapping shared memory\n", nRank));
#ifdef HAVE_SHMAT
	pg->addr = shmat(pg->id, NULL, SHM_RND);
	if (pg->addr == (void*)-1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|shmat", "**ch3|shm|shmat %d", errno); /*"Error from shmat %d\n", errno);*/
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return NULL;
	}
#elif defined(HAVE_MAPVIEWOFFILE)
	pg->addr = MapViewOfFileEx(
	    pg->id,
	    FILE_MAP_WRITE,
	    0, 0,
	    nTotalSize,
	    NULL
	    );
	MPIU_DBG_PRINTF(("."));
	if (pg->addr == NULL)
	{
	    MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "Error in MapViewOfFileEx, %d\n", GetLastError());
	}
#else
#error *** No shared memory mapping function specified ***
#endif
	MPIU_DBG_PRINTF(("\n[%d] finished mapping shared memory: addr:%x\n", nRank, pg->addr));

#ifdef HAVE_SHARED_PROCESS_READ

	pg->pSHP = (MPIDI_CH3I_Shared_process_t*)((char*)pg->addr + shp_offset);
	InitSharedProcesses(pg);
#endif
    }
    else
    {
	pg->addr = MPIU_Malloc(nTotalSize);
    }

    MPIU_DBG_PRINTF(("[%d] made it: shm address: %x\n", nRank, pg->addr));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return pg->addr;
}

static BOOL g_bGetMemSyncCalled = FALSE;
/*@
   MPIDI_CH3I_SHM_Get_mem_sync - allocate and get address and size of memory shared by all processes. 

   Parameters:
+  int nTotalSize
.  int nRank
-  int nNproc

   Notes:
    Set the global variables pg->addr, pg->size, pg->id
    Ensure that pg->addr is the same across all processes that share memory.
@*/
void *MPIDI_CH3I_SHM_Get_mem_sync(MPIDI_CH3I_Process_group_t *pg, int nTotalSize, int nRank, int nNproc, BOOL bUseShm)
{
    int mpi_errno;
    struct ShmemRankAndAddressStruct
    {
        int nRank;
        void *pAddress, *pNextAddressAttempt;
    } *pRankAddr;
    int i;
    void *pHighAddr, *pLastAddr, *pNextAddr = NULL;
    BOOL bAllEqual, bRepeat, bDoRemapping = TRUE;
#ifdef HAVE_SHARED_PROCESS_READ
    int shp_offset;
    BOOL bFirst = TRUE;
#endif
#ifdef HAVE_WINDOWS_H
    SYSTEM_INFO sysInfo;
#endif
    unsigned long nPageSize = 65536;
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    HANDLE hSyncEvent1, hSyncEvent2;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);

#ifdef HAVE_SHARED_PROCESS_READ
    /* add room at the end of the shard memory region for the shared process information */
    shp_offset = nTotalSize;
    nTotalSize += nNproc * sizeof(MPIDI_CH3I_Shared_process_t);
#endif

    /* Setup and check parameters */
#ifdef HAVE_WINDOWS_H
    GetSystemInfo(&sysInfo);
    nPageSize = sysInfo.dwAllocationGranularity;
    MPIU_DBG_PRINTF(("[%d] nPageSize: %d\n", nRank, nPageSize));
#endif

#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    hSyncEvent1 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent1");
    hSyncEvent2 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent2");
#endif
    /* Uncomment this line to test shmem pointer alignment algorithm */
    /*if (nRank == 0) pLastAddr = malloc(1024000); */
    if (g_bGetMemSyncCalled)
    {
        mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|GetMemTwice", 0); /*"Error: Global shared memory initializer called more than once.\n");*/
    }
    g_bGetMemSyncCalled = TRUE;
    if (nTotalSize < 1)
    {
        MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem_sync", "unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);
        MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
        return NULL;
    }
    
    if (bUseShm)
    {
        /* Create the shared memory object */
#ifdef HAVE_SHMGET
        pg->id = shmget(pg->key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
        if (pg->id == -1) 
        {
            mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|shmget", "**ch3|shm|shmget %d", errno); /*"Error in shmget\n");*/
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
            return mpi_errno;
        }
#elif defined (HAVE_CREATEFILEMAPPING)
        pg->id = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0, 
            nTotalSize,
            pg->key);
        if (pg->id == NULL) 
        {
            mpi_errno = MPIR_Err_create_code(MPI_ERR_OTHER, "**ch3|shm|CreateFileMapping", "**ch3|shm|CreateFileMapping %d", GetLastError()); /*"Error in CreateFileMapping, %d\n", GetLastError());*/
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
            return mpi_errno;
        }
#else
#error *** No shared memory allocation function specified ***
#endif
        
        /* Map the object into the users process space, ensuring that all processes have the same address */
        do
        {
            bRepeat = FALSE;
            /* Get the shmem pointer */
            pLastAddr = pNextAddr;
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
            MPIU_DBG_PRINTF(("<"));
            if (nRank == 0)
            {
                ResetEvent(hSyncEvent2);
                SetEvent(hSyncEvent1);
                WaitForSingleObject(hSyncEvent2, INFINITE);
            }
            else
            {
                WaitForSingleObject(hSyncEvent1, INFINITE);
                ResetEvent(hSyncEvent1);
                SetEvent(hSyncEvent2);
            }
            MPIU_DBG_PRINTF((">"));
#endif
            if (bDoRemapping)
            {
                pg->addr = NULL;
                MPIU_DBG_PRINTF(("[%d] mapping shared memory\n", nRank));
#ifdef HAVE_SHMAT
                do 
                {
                    pg->addr = shmat(pg->id, pNextAddr, SHM_RND);
                    pNextAddr = (void*)((((unsigned long)pNextAddr + nPageSize) / nPageSize) * nPageSize);
                    MPIU_DBG_PRINTF(("%d", nRank));
                } 
                while (pg->addr == (void*)-1);
#elif defined(HAVE_MAPVIEWOFFILE)
                do
                {
                    pg->addr = MapViewOfFileEx(
                        pg->id,
                        FILE_MAP_WRITE,
                        0, 0,
                        nTotalSize,
                        pNextAddr
                        );
                    pNextAddr = (void*)((((unsigned long)pNextAddr + nPageSize) / nPageSize) * nPageSize);
                    MPIU_DBG_PRINTF(("."));
                } while (pg->addr == NULL);
#else
#error *** No shared memory mapping function specified ***
#endif
                MPIU_DBG_PRINTF(("\n[%d] finished mapping shared memory: addr:%x\n", nRank, pg->addr));
            }
            else
            {
                MPIU_DBG_PRINTF(("[%d] shm[%d] not remapping\n", nRank, nRank));
            }
#ifdef HAVE_SHARED_PROCESS_READ
            if (bFirst)
            {
		pg->pSHP = (MPIDI_CH3I_Shared_process_t*)((char*)pg->addr + shp_offset);
                InitSharedProcesses(pg);
                bFirst = FALSE;
            }
#endif
            /* Write the value and index at the index for this process */
            MPIU_DBG_PRINTF(("[%d] writing address and rank\n", nRank));
            pRankAddr = (struct ShmemRankAndAddressStruct *)pg->addr;
            pRankAddr[nRank].pAddress = pg->addr;
            if (nRank != 0)
            {
                while (pRankAddr[0].nRank != 0)
                    MPIDU_Yield();
            }
            pRankAddr[nRank].nRank = nRank;
            if (nRank == 0)
            {
                bAllEqual = TRUE;
                pHighAddr = pg->addr;
                /* Wait for everyone to write their base address to shared memory */
                MPIU_DBG_PRINTF(("[%d] waiting for all ranks to be valid\n", nRank));
                for (i = 0; i<nNproc; i++)
                {
                    while (pRankAddr[i].nRank != i)
                        MPIDU_Yield();
                    if (pRankAddr[i].pAddress != pg->addr)
                        bAllEqual = FALSE;
                    /*pLowAddr = MPIDU_MIN(pLowAddr, pRankAddr[i].pAddress); */
                    pHighAddr = MPIDU_MAX(pHighAddr, pRankAddr[i].pAddress);
                    /*MPIU_DBG_PRINTF(("pHighAddr = %x\n", pHighAddr)); */
                    MPIU_DBG_PRINTF(("[%d] shm[%d].pAddress = %x\n", nRank, i, pRankAddr[i].pAddress));
                }
                MPIU_DBG_PRINTF(("[%d] all ranks valid\n", nRank));
                if (bAllEqual == FALSE)
                {
                    MPIU_DBG_PRINTF(("[%d] base pointers not equal\n", nRank));
                    bRepeat = TRUE;
                    if (pHighAddr != pLastAddr)
                    {
                        pRankAddr[0].pNextAddressAttempt = pHighAddr;
                        MPIU_DBG_PRINTF(("[%d] next address attempt, high: %x\n", nRank, pHighAddr));
                    }
                    else
                    {
                        pRankAddr[0].pNextAddressAttempt = (void*)((((unsigned long)pLastAddr + nPageSize) / nPageSize) * nPageSize);
                        MPIU_DBG_PRINTF(("[%d] next address attempt, calculated: %x\n", nRank, pRankAddr[0].pNextAddressAttempt));
                    }
                    pNextAddr = pRankAddr[0].pNextAddressAttempt;
                    pRankAddr[0].pAddress = NULL;
                    if (pNextAddr == pg->addr)
                        bDoRemapping = FALSE;
                    else
                        bDoRemapping = TRUE;
                }
                /* Set rank zero's index to -1 */
                MPIU_DBG_PRINTF(("[%d] setting rank 0 to -1\n", nRank));
                pRankAddr[0].nRank = -1;
                /* Wait for everyone to set their rank to -1 */
                MPIU_DBG_PRINTF(("[%d] waiting for each index to be set to -1\n", nRank));
                for (i = 1; i<nNproc; i++)
                {
                    while (pRankAddr[i].nRank != -1)
                        MPIDU_Yield();
                }
                if (bAllEqual == FALSE && bDoRemapping)
                {
                    MPIU_DBG_PRINTF(("[%d] unmapping shared memory\n", nRank));
#ifdef HAVE_SHMDT
                    shmdt(pg->addr);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
                    UnmapViewOfFile(pg->addr);
#endif
                }
            }
            else /* if (nRank == 0) */
            {
                MPIU_DBG_PRINTF(("[%d] waiting for rank 0 to be set to -1\n", nRank));
                while (pRankAddr[0].nRank != -1)
                    MPIDU_Yield();
                if (pRankAddr[0].pAddress == NULL)
                {
                    MPIU_DBG_PRINTF(("[%d] bRepeat = TRUE, getting next address and setting rank[%d] to -1\n", nRank, nRank));
                    bRepeat = TRUE;
                    pNextAddr = pRankAddr[0].pNextAddressAttempt;
                    pRankAddr[nRank].nRank = -1;
                    if (pg->addr == pNextAddr)
                        bDoRemapping = FALSE;
                    else
                    {
                        MPIU_DBG_PRINTF(("[%d] unmapping shared memory\n", nRank));
#ifdef HAVE_SHMDT
                        shmdt(pg->addr);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
                        UnmapViewOfFile(pg->addr);
#endif
                        bDoRemapping = TRUE;
                    }
                }
                else
                {
                    MPIU_DBG_PRINTF(("[%d] setting rank[%d] to -1\n", nRank, nRank));
                    pRankAddr[nRank].nRank = -1;
                }
            }
        } 
        while (bRepeat == TRUE);
    }
    else
    {
        pg->addr = MPIU_Malloc(nTotalSize);
    }

    MPIU_DBG_PRINTF(("[%d] made it: shm address: %x\n", nRank, pg->addr));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
    return pg->addr;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
void MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process_group_t *pg, BOOL bUseShm)
{
#ifdef HAVE_SHARED_PROCESS_READ
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
    if (bUseShm)
    {
#ifdef HAVE_SHMDT
        shmdt(pg->addr);
#endif
#ifdef HAVE_SHMCTL
        shmctl(pg->id, IPC_RMID, NULL);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
        UnmapViewOfFile(pg->addr);
        pg->addr = NULL;
#endif
#ifdef HAVE_CREATEFILEMAPPING
        CloseHandle(pg->id);
        pg->id = NULL;
#endif
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef HAVE_WINDOWS_H
	for (i=0; i<pg->size; i++)
	    CloseHandle(pg->pSharedProcessHandles[i]);
	MPIU_Free(pg->pSharedProcessHandles);
	pg->pSharedProcessHandles = NULL;
#else
	for (i=0; i<pg->size; i++)
	    close(pg->pSharedProcessFileDescriptors[i]);
	MPIU_Free(pg->pSharedProcessFileDescriptors);
	MPIU_Free(pg->pSharedProcessIDs);
	pg->pSharedProcessFileDescriptors = NULL;
	pg->pSharedProcessIDs = NULL;
#endif
#endif
    }
    else
    {
        MPIU_Free(pg->addr);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
}
