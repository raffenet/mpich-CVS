/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#define MPIDU_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MPIDU_MIN(a,b)    (((a) < (b)) ? (a) : (b))

#undef SYNCHRONIZE_SHMAPPING

#ifdef HAVE_SHARED_PROCESS_READ
static void InitSharedProcesses(void *pShmem, int nRank, int nProc)
{
#ifndef HAVE_WINDOWS_H
    char filename[256];
#endif
    int i;
    struct SharedProcessStruct
    {
        int nRank;
#ifdef HAVE_WINDOWS_H
        DWORD nPid;
#else
        int nPid;
#endif
        BOOL bFinished;
    } *pSharedProcess;

    pSharedProcess = (struct SharedProcessStruct *)pShmem;

#ifdef HAVE_WINDOWS_H
    pSharedProcess[nRank].nPid = GetCurrentProcessId();
#else
    pSharedProcess[nRank].nPid = getpid();
#endif
    pSharedProcess[nRank].bFinished = FALSE;
    pSharedProcess[nRank].nRank = nRank;

    for (i=0; i<nProc; i++)
    {
        if (i != nRank)
        {
            while (pSharedProcess[i].nRank != i)
                MPIDU_Yield();
#ifdef HAVE_WINDOWS_H
            /*printf("Opening process[%d]: %d\n", i, pSharedProcess[i].nPid);*/
            g_pSharedProcessHandles[i] =
                OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
                            FALSE, pSharedProcess[i].nPid);
            if (g_pSharedProcessHandles[i] == NULL)
            {
                int err = GetLastError();
                printf("unable to open process %d, error %d\n", i, err);
            }
#else
            sprintf(filename, "/proc/%d/mem", pSharedProcess[i].nPid);
            g_pSharedProcessIDs[i] = pSharedProcess[i].nPid;
            g_pSharedProcessFileDescriptors[i] = open(filename, O_RDONLY);
            if (g_pSharedProcessFileDescriptors[i] == -1)
                printf("failed to open mem file, '%s', for process %d\n", filename, pSharedProcess[i].nPid);
#endif
        }
        else
        {
#ifdef HAVE_WINDOWS_H
            g_pSharedProcessHandles[i] = NULL;
#else
            g_pSharedProcessIDs[i] = 0;
            g_pSharedProcessFileDescriptors[i] = 0;
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
void *MPIDI_CH3I_SHM_Get_mem_sync(MPIDI_CH3I_Process_group_t *pg, int nTotalSize, int nRank, int nNproc)
{
    struct ShmemRankAndAddressStruct
    {
        int nRank;
        void *pAddress, *pNextAddressAttempt;
    } *pRankAddr;
    int i;
    void *pHighAddr, *pLastAddr, *pNextAddr = NULL;
    BOOL bAllEqual, bRepeat, bDoRemapping = TRUE;
#ifdef HAVE_SHARED_PROCESS_READ
    BOOL bFirst = TRUE;
#endif
    unsigned long nPageSize = 65536;
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    HANDLE hSyncEvent1, hSyncEvent2;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
    
    /* Setup and check parameters */
#ifdef HAVE_WINDOWS_H
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    nPageSize = sysInfo.dwAllocationGranularity;
    MPIU_DBG_PRINTF(("[%d] nPageSize: %d\n", nRank, nPageSize));
#endif

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
    
#if defined(HAVE_WINDOWS_H) && defined(SYNCHRONIZE_SHMAPPING)
    hSyncEvent1 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent1");
    hSyncEvent2 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent2");
#endif
    /* Uncomment this line to test shmem pointer alignment algorithm */
    /*if (nRank == 0) pLastAddr = malloc(1024000); */
    if (g_bGetMemSyncCalled)
    {
        printf("Error: Global shared memory initializer called more than once.\n");
    }
    g_bGetMemSyncCalled = TRUE;
    if (nTotalSize < 1)
    {
        err_printf("Error: unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);
        MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
        return NULL;
    }
    
    //if (MPIDI_bUseShm)
    if (TRUE)
    {
        /* Create the shared memory object */
#ifdef HAVE_SHMGET
        pg->id = shmget(pg->key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
        if (pg->id == -1) 
        {
            printf("Error in shmget\n");
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
            exit(0);
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
            printf("Error in CreateFileMapping, %d\n", GetLastError());
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
            exit(0);
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
                InitSharedProcesses(pg->addr, nRank, nNproc);
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
                    /*printf("pHighAddr = %x\n", pHighAddr); */
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
void MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process_group_t *pg)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
    //if (MPIDI_bUseShm)
    if (TRUE)
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
    }
    else
    {
        MPIU_Free(pg->addr);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
}
