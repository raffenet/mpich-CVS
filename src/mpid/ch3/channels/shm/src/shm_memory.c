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
#ifdef HAVE_WINDOWS_H
extern HANDLE *g_pSharedProcessHandles;
#else
extern int *g_pSharedProcessIDs;
extern int *g_pSharedProcessFileDescriptors;
#endif
#endif

#ifdef USE_GARBAGE_COLLECTING
/*@
*MPIDI_CH3I_SHM_Alloc - 

  Parameters:
  +  int size
  
    Notes:
@*/
void *MPIDI_CH3I_SHM_Alloc(unsigned int size)
{
/* allocate size bytes out of this process's shared memory pool
    return null if not available. */
    
    MPIDI_Shm_mem_obj_hdr *mem_obj_hdr, *next_mem_obj_hdr, **mem_obj_hdr_ptr,
        **next_mem_obj_hdr_ptr;
    int i, j, k;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
    
    /* find next largest size supported */
    for (i = 0; i < SHM_NSIZES; i++) 
    {
        if (size <= MPIDI_CH3I_Process.free_list[i].size)
            break;
    }
    
    if (i == SHM_NSIZES) 
    {
        printf("MPIDI_CH3I_SHM_Alloc: Memory requested, %d, is larger than maximum size allowed, %d\n", size, MPIDI_CH3I_Process.free_list[SHM_NSIZES-1].size);
        fflush(stdout);
        exit(0);
    }

    size = MPIDI_CH3I_Process.free_list[i].size; /* allocate an entire bucket size, regardless of what was passed in. */
    
    /* every few times, do garbage collection first */
    /* maybe lock is not needed */
    MPIDU_Process_lock(&MPIDI_CH3I_Process.gc_lock);
    k = MPIDI_CH3I_Process.gc_count;
    if (k == MPIDI_SHM_GC_COUNT_MAX)
        MPIDI_CH3I_Process.gc_count = 0;
    else
        MPIDI_CH3I_Process.gc_count++;
    MPIDU_Process_unlock(&MPIDI_CH3I_Process.gc_lock);    
    
    /* Note: can't use the variable i in garbage collection code because it points to 
    the correct size bucket for the current request */
    if (k == MPIDI_SHM_GC_COUNT_MAX) 
    {
        /*printf("SHM Garbage collecting.\n"); fflush(stdout); */
        for (j = 0; j < SHM_NSIZES; j++) 
        {
            MPIDU_Process_lock(&(MPIDI_CH3I_Process.inuse_list[j].thr_lock));
            mem_obj_hdr = MPIDI_CH3I_Process.inuse_list[j].ptr;
            mem_obj_hdr_ptr = &(MPIDI_CH3I_Process.inuse_list[j].ptr);
            while (mem_obj_hdr) 
            {
                next_mem_obj_hdr = mem_obj_hdr->next;
                next_mem_obj_hdr_ptr = &(mem_obj_hdr->next);
                if (mem_obj_hdr->inuse == 0) 
                {  /* free */
                    /* remove from this list */
                    *mem_obj_hdr_ptr = mem_obj_hdr->next;
                    MPIDI_CH3I_Process.inuse_list[j].count--;
                    /* add to free list */
                    MPIDU_Process_lock(&(MPIDI_CH3I_Process.free_list[j].thr_lock));
                    
                    mem_obj_hdr->next = MPIDI_CH3I_Process.free_list[j].ptr;
                    MPIDI_CH3I_Process.free_list[j].ptr = mem_obj_hdr;
                    MPIDI_CH3I_Process.free_list[j].count++;
                    MPIDI_CH3I_Process.free_list[j].max_count = 
                        MPIDU_MAX(MPIDI_CH3I_Process.free_list[j].count, MPIDI_CH3I_Process.free_list[j].max_count);
                    
                    MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[j].thr_lock));
                }
                else 
                {
                    mem_obj_hdr_ptr = next_mem_obj_hdr_ptr;
                }
                mem_obj_hdr = next_mem_obj_hdr;
            }
            MPIDU_Process_unlock(&(MPIDI_CH3I_Process.inuse_list[j].thr_lock));
        }
    }
    
    
    /* see if available in free list first */
    MPIDU_Process_lock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
    mem_obj_hdr = MPIDI_CH3I_Process.free_list[i].ptr;
    
    /* printf("SZ %d, mem_obj_hdr %ld\n", size, mem_obj_hdr); */
    if (mem_obj_hdr) 
    {  /* take from free list */
        MPIDI_CH3I_Process.free_list[i].ptr = mem_obj_hdr->next;
        MPIDI_CH3I_Process.free_list[i].count--;
        MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
        mem_obj_hdr->inuse = 1;
    }
    else 
    {   /* take out of shared memory pool */
        MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
        /*printf("size %d, MPIDI_CH3I_Process.my_rem_size %d\n", size, MPIDI_CH3I_Process.my_rem_size); */
        MPIDU_Process_lock(&MPIDI_CH3I_Process.addr_lock);
        if (size + (int)sizeof(MPIDI_Shm_mem_obj_hdr) <= MPIDI_CH3I_Process.my_rem_size) 
        {
            mem_obj_hdr = (MPIDI_Shm_mem_obj_hdr *) MPIDI_CH3I_Process.my_curr_addr;
            
            /*printf("[%d] shmem %d -> %d\n", MPID_COMM_WORLD->rank, MPIDI_CH3I_Process.my_rem_size, MPIDI_CH3I_Process.my_rem_size - size - sizeof(MPIDI_Shm_mem_obj_hdr)); fflush(stdout); */
            MPIDI_CH3I_Process.my_rem_size = MPIDI_CH3I_Process.my_rem_size - size -
                sizeof(MPIDI_Shm_mem_obj_hdr);
            MPIDI_CH3I_Process.my_curr_addr = ((char *) MPIDI_CH3I_Process.my_curr_addr) + size
                + sizeof(MPIDI_Shm_mem_obj_hdr);
            MPIDU_Process_unlock(&MPIDI_CH3I_Process.addr_lock);
            
            mem_obj_hdr->inuse = 1;
        }
        else
        {
            MPIDU_Process_unlock(&MPIDI_CH3I_Process.addr_lock);
            printf("Unable to allocate a shared memory block of size: %d\n", size);
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
            return NULL;
        }
    }
    
    /* add to inuse list */
    MPIDU_Process_lock(&(MPIDI_CH3I_Process.inuse_list[i].thr_lock));
    mem_obj_hdr->next = MPIDI_CH3I_Process.inuse_list[i].ptr;
    MPIDI_CH3I_Process.inuse_list[i].ptr = mem_obj_hdr;
    MPIDI_CH3I_Process.inuse_list[i].count++;
    MPIDI_CH3I_Process.inuse_list[i].max_count = 
        MPIDU_MAX(MPIDI_CH3I_Process.inuse_list[i].count, MPIDI_CH3I_Process.inuse_list[i].max_count);
    MPIDU_Process_unlock(&(MPIDI_CH3I_Process.inuse_list[i].thr_lock));
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
    return (void *) (mem_obj_hdr + 1);
}

/*@
   MPIDI_CH3I_SHM_Free - 

   Parameters:
+  void *address

   Notes:
@*/
void MPIDI_CH3I_SHM_Free(void *address)
{
    MPIDI_Shm_mem_obj_hdr *mem_obj_hdr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_FREE);
    mem_obj_hdr = ((MPIDI_CH3I_Process.mem_obj_hdr *) address) - 1;
    mem_obj_hdr->inuse = 0;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_FREE);
}

#else /* don't USE_GARBAGE_COLLECTING */

/*@
*MPIDI_CH3I_SHM_Alloc - 

  Parameters:
  +  int size
  
    Notes:
@*/
void *MPIDI_CH3I_SHM_Alloc(unsigned int size)
{
/* allocate size bytes out of this process's shared memory pool
    return null if not available. */
    
    MPIDI_Shm_mem_obj_hdr *mem_obj_hdr;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
    
    /* find next largest size supported */
    for (i = 0; i < SHM_NSIZES; i++) 
    {
        if (size <= MPIDI_CH3I_Process.free_list[i].size)
            break;
    }
    
    if (i == SHM_NSIZES) 
    {
        printf("MPIDI_CH3I_SHM_Alloc: Memory requested, %d, is larger than maximum size allowed, %d\n", size, MPIDI_CH3I_Process.free_list[SHM_NSIZES-1].size);
        fflush(stdout);
        exit(0);
    }
    
    size = MPIDI_CH3I_Process.free_list[i].size; /* allocate an entire bucket size, regardless of what was passed in. */
    
    /* see if available in free list first */
    MPIDU_Process_lock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
    mem_obj_hdr = MPIDI_CH3I_Process.free_list[i].ptr;
    
    /* printf("SZ %d, mem_obj_hdr %ld\n", size, mem_obj_hdr); */
    if (mem_obj_hdr) 
    {  /* take from free list */
        MPIDI_CH3I_Process.free_list[i].ptr = mem_obj_hdr->next;
        MPIDI_CH3I_Process.free_list[i].count--;
        //mem_obj_hdr->inuse = 1;
        MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
    }
    else 
    {   /* take out of shared memory pool */
        MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[i].thr_lock));
        /*printf("size %d, MPIDI_CH3I_Process.my_rem_size %d\n", size, MPIDI_CH3I_Process.my_rem_size); */
        MPIDU_Process_lock(&MPIDI_CH3I_Process.addr_lock);
        if (size + (unsigned int)sizeof(MPIDI_Shm_mem_obj_hdr) <= MPIDI_CH3I_Process.my_rem_size) 
        {
            mem_obj_hdr = (MPIDI_Shm_mem_obj_hdr *) MPIDI_CH3I_Process.my_curr_addr;
            
            /*printf("[%d] shmem %d -> %d\n", MPID_COMM_WORLD->rank, MPIDI_CH3I_Process.my_rem_size, MPIDI_CH3I_Process.my_rem_size - size - sizeof(MPIDI_Shm_mem_obj_hdr)); fflush(stdout); */
            MPIDI_CH3I_Process.my_rem_size = MPIDI_CH3I_Process.my_rem_size - size -
                sizeof(MPIDI_Shm_mem_obj_hdr);
            MPIDI_CH3I_Process.my_curr_addr = ((char *) MPIDI_CH3I_Process.my_curr_addr) + size
                + sizeof(MPIDI_Shm_mem_obj_hdr);
            MPIDU_Process_unlock(&MPIDI_CH3I_Process.addr_lock);
            
            mem_obj_hdr->index = i;
        }
        else
        {
            MPIDU_Process_unlock(&MPIDI_CH3I_Process.addr_lock);
            printf("Unable to allocate a shared memory block of size: %d\n", size);
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
            return NULL;
        }
    }
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ALLOC);
    return (void *) (mem_obj_hdr + 1);
}

/*@
   MPIDI_CH3I_SHM_Free - 

   Parameters:
+  void *address

   Notes:
@*/
void MPIDI_CH3I_SHM_Free(void *address)
{
    MPIDI_Shm_mem_obj_hdr *mem_obj_hdr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_FREE);

    mem_obj_hdr = ((MPIDI_Shm_mem_obj_hdr *) address) - 1;

    MPIDU_Process_lock(&(MPIDI_CH3I_Process.free_list[mem_obj_hdr->index].thr_lock));
    mem_obj_hdr->next = MPIDI_CH3I_Process.free_list[mem_obj_hdr->index].ptr;
    MPIDI_CH3I_Process.free_list[mem_obj_hdr->index].ptr = mem_obj_hdr;
    MPIDU_Process_unlock(&(MPIDI_CH3I_Process.free_list[mem_obj_hdr->index].thr_lock));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_FREE);
}
#endif /* USE_GARBAGE_COLLECTING */

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
    Set the global variables MPIDI_CH3I_Process.addr, MPIDI_CH3I_Process.size, MPIDI_CH3I_Process.id
    Ensure that MPIDI_CH3I_Process.addr is the same across all processes that share memory.
@*/
void *MPIDI_CH3I_SHM_Get_mem_sync(int nTotalSize, int nRank, int nNproc)
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
        printf("Error: unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);
        MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
        return NULL;
    }
    
    //if (MPIDI_bUseShm)
    if (TRUE)
    {
        /* Create the shared memory object */
#ifdef HAVE_SHMGET
        MPIDI_CH3I_Process.id = shmget(MPIDI_CH3I_Process.key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
        if (MPIDI_CH3I_Process.id == -1) 
        {
            printf("Error in shmget\n");
            MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
            exit(0);
        }
#elif defined (HAVE_CREATEFILEMAPPING)
        MPIDI_CH3I_Process.id = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0, 
            nTotalSize,
            MPIDI_CH3I_Process.key);
        if (MPIDI_CH3I_Process.id == NULL) 
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
                MPIDI_CH3I_Process.addr = NULL;
                MPIU_DBG_PRINTF(("[%d] mapping shared memory\n", nRank));
#ifdef HAVE_SHMAT
                do 
                {
                    MPIDI_CH3I_Process.addr = shmat(MPIDI_CH3I_Process.id, pNextAddr, SHM_RND);
                    pNextAddr = (void*)((((unsigned long)pNextAddr + nPageSize) / nPageSize) * nPageSize);
                    MPIU_DBG_PRINTF(("%d", nRank));
                } 
                while (MPIDI_CH3I_Process.addr == (void*)-1);
#elif defined(HAVE_MAPVIEWOFFILE)
                do
                {
                    MPIDI_CH3I_Process.addr = MapViewOfFileEx(
                        MPIDI_CH3I_Process.id,
                        FILE_MAP_WRITE,
                        0, 0,
                        nTotalSize,
                        pNextAddr
                        );
                    pNextAddr = (void*)((((unsigned long)pNextAddr + nPageSize) / nPageSize) * nPageSize);
                    MPIU_DBG_PRINTF(("."));
                } while (MPIDI_CH3I_Process.addr == NULL);
#else
#error *** No shared memory mapping function specified ***
#endif
                MPIU_DBG_PRINTF(("\n[%d] finished mapping shared memory: addr:%x\n", nRank, MPIDI_CH3I_Process.addr));
            }
            else
            {
                MPIU_DBG_PRINTF(("[%d] shm[%d] not remapping\n", nRank, nRank));
            }
#ifdef HAVE_SHARED_PROCESS_READ
            if (bFirst)
            {
                InitSharedProcesses(MPIDI_CH3I_Process.addr, nRank, nNproc);
                bFirst = FALSE;
            }
#endif
            /* Write the value and index at the index for this process */
            MPIU_DBG_PRINTF(("[%d] writing address and rank\n", nRank));
            pRankAddr = (struct ShmemRankAndAddressStruct *)MPIDI_CH3I_Process.addr;
            pRankAddr[nRank].pAddress = MPIDI_CH3I_Process.addr;
            if (nRank != 0)
            {
                while (pRankAddr[0].nRank != 0)
                    MPIDU_Yield();
            }
            pRankAddr[nRank].nRank = nRank;
            if (nRank == 0)
            {
                bAllEqual = TRUE;
                pHighAddr = MPIDI_CH3I_Process.addr;
                /* Wait for everyone to write their base address to shared memory */
                MPIU_DBG_PRINTF(("[%d] waiting for all ranks to be valid\n", nRank));
                for (i = 0; i<nNproc; i++)
                {
                    while (pRankAddr[i].nRank != i)
                        MPIDU_Yield();
                    if (pRankAddr[i].pAddress != MPIDI_CH3I_Process.addr)
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
                    if (pNextAddr == MPIDI_CH3I_Process.addr)
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
                    shmdt(MPIDI_CH3I_Process.addr);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
                    UnmapViewOfFile(MPIDI_CH3I_Process.addr);
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
                    if (MPIDI_CH3I_Process.addr == pNextAddr)
                        bDoRemapping = FALSE;
                    else
                    {
                        MPIU_DBG_PRINTF(("[%d] unmapping shared memory\n", nRank));
#ifdef HAVE_SHMDT
                        shmdt(MPIDI_CH3I_Process.addr);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
                        UnmapViewOfFile(MPIDI_CH3I_Process.addr);
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
        MPIDI_CH3I_Process.addr = MPIU_Malloc(nTotalSize);
    }
    
    MPIDI_CH3I_Process.size = nTotalSize;
    
    MPIU_DBG_PRINTF(("[%d] made it: shm address: %x\n", nRank, MPIDI_CH3I_Process.addr));
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC);
    return MPIDI_CH3I_Process.addr;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
void MPIDI_CH3I_SHM_Release_mem()
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
    //if (MPIDI_bUseShm)
    if (TRUE)
    {
#ifdef HAVE_SHMDT
        shmdt(MPIDI_CH3I_Process.addr);
#endif
#ifdef HAVE_SHMCTL
        shmctl(MPIDI_CH3I_Process.id, IPC_RMID, NULL);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
        UnmapViewOfFile(MPIDI_CH3I_Process.addr);
        MPIDI_CH3I_Process.addr = NULL;
#endif
#ifdef HAVE_CREATEFILEMAPPING
        CloseHandle(MPIDI_CH3I_Process.id);
        MPIDI_CH3I_Process.id = NULL;
#endif
    }
    else
    {
        MPIU_Free(MPIDI_CH3I_Process.addr);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
}
