/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#define ZERO_RANK 0x10101010

#undef USE_SYNCHRONIZE_SHMAPPING

#ifdef HAVE_SHARED_PROCESS_READ
#undef FUNCNAME
#define FUNCNAME InitSharedProcesses
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int InitSharedProcesses(MPIDI_CH3I_Process_group_t *pg)
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
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", i, err); /*"unable to open process %d, error %d\n", i, err);*/
            }
#else
            sprintf(filename, "/proc/%d/mem", pSharedProcess[i].nPid);
            pg->pSharedProcessIDs[i] = pSharedProcess[i].nPid;
            pg->pSharedProcessFileDescriptors[i] = open(filename, O_RDONLY);
            if (pg->pSharedProcessFileDescriptors[i] == -1)
	    {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**open", "**open %s %d %d", filename, pSharedProcess[i].nPid, errno);
		return mpi_errno;
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
    return MPI_SUCCESS;
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
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Get_mem(MPIDI_CH3I_Process_group_t *pg, int nTotalSize, int nRank, int nNproc, BOOL bUseShm)
{
    int mpi_errno;
#ifdef HAVE_SHARED_PROCESS_READ
    int shp_offset;
#endif
#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
    HANDLE hSyncEvent1, hSyncEvent2;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

#ifdef HAVE_SHARED_PROCESS_READ
    /* add room at the end of the shard memory region for the shared process information */
    shp_offset = nTotalSize;
    nTotalSize += nNproc * sizeof(MPIDI_CH3I_Shared_process_t);
#endif

#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
    hSyncEvent1 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent1");
    hSyncEvent2 = CreateEvent(NULL, TRUE, FALSE, "mpich2shmsyncevent2");
#endif

    if (nTotalSize < 1)
    {
	/*MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "unable to allocate %d bytes of shared memory: must be greater than zero.\n", nTotalSize);*/
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	pg->addr = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    if (bUseShm)
    {
	/* Create the shared memory object */
#ifdef USE_POSIX_SHM
	pg->id = shm_open(pg->key, O_RDWR | O_CREAT, 0600);
	if (pg->id == -1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pg->key, errno);
	    pg->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
	ftruncate(pg->id, nTotalSize);
#elif defined (USE_SYSV_SHM)
	pg->id = shmget(pg->key, nTotalSize, IPC_CREAT | SHM_R | SHM_W);
	if (pg->id == -1) 
	{
	    if (errno == EINVAL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmsize", "**shmsize %d", nTotalSize);
	    }
	    else
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", errno);
	    }
	    pg->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined (USE_WINDOWS_SHM)
	pg->id = CreateFileMapping(
	    INVALID_HANDLE_VALUE,
	    NULL,
	    PAGE_READWRITE,
	    0, 
	    nTotalSize,
	    pg->key);
	if (pg->id == NULL) 
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", GetLastError()); /*"Error in CreateFileMapping, %d\n", GetLastError());*/
	    pg->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#else
#error No shared memory subsystem defined
#endif

	/* Get the shmem pointer */
#if defined(HAVE_WINDOWS_H) && defined(USE_SYNCHRONIZE_SHMAPPING)
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
#ifdef USE_POSIX_SHM
	pg->addr = mmap(NULL, nTotalSize, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pg->id, 0);
	if (pg->addr == MAP_FAILED)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	    pg->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined (USE_SYSV_SHM)
	pg->addr = shmat(pg->id, NULL, SHM_RND);
	if (pg->addr == (void*)-1)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", errno); /*"Error from shmat %d\n", errno);*/
	    pg->addr = NULL;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	    return mpi_errno;
	}
#elif defined(USE_WINDOWS_SHM)
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
	    /*MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "Error in MapViewOfFileEx, %d\n", GetLastError());*/
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", 0);
	    pg->addr = NULL;
	    return mpi_errno;
	}
#else
#error No shared memory subsystem defined
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
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Process_group_t *pg, BOOL bUseShm)
{
#ifdef HAVE_SHARED_PROCESS_READ
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
    if (bUseShm)
    {
#ifdef USE_POSIX_SHM
	close(pg->id);
#elif defined (USE_SYSV_SHM)
        shmdt(pg->addr);
#elif defined (USE_WINDOWS_SHM)
        UnmapViewOfFile(pg->addr);
        pg->addr = NULL;
        CloseHandle(pg->id);
        pg->id = NULL;
#else
#error No shared memory subsystem defined
#endif
#ifdef HAVE_SHARED_PROCESS_READ
#ifdef USE_WINDOWS_SHM
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
    return MPI_SUCCESS;
}
