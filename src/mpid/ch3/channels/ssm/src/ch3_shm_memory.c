/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static void generate_shm_string(char *str)
{
#ifdef HAVE_MAPVIEWOFFILE
    UUID guid;
    UuidCreate(&guid);
    sprintf(str, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    MPIU_DBG_PRINTF(("GUID = %s\n", str));
#else
    sprintf(str, "%d", getpid());
#endif
}

#if 0
/* Here's a clip of the SHARED_PROCESS_READ code */
/* allocation */
#ifdef HAVE_WINDOWS_H
            pSharedProcessHandles[i] =
                OpenProcess(STANDARD_RIGHTS_REQUIRED | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, 
                            FALSE, pSharedProcess[i].nPid);
            if (pSharedProcessHandles[i] == NULL)
            {
                int err = GetLastError();
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**OpenProcess", "**OpenProcess %d %d", i, err); /*"unable to open process %d, error %d", i, err);*/
            }
#else
            sprintf(filename, "/proc/%d/mem", pSharedProcess[i].nPid);
            pSharedProcessIDs[i] = pSharedProcess[i].nPid;
            pSharedProcessFileDescriptors[i] = open(filename, O_RDONLY);
            if (pSharedProcessFileDescriptors[i] == -1)
	    {
                mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**open", "**open %s %d %d", filename, pSharedProcess[i].nPid, errno); /*"failed to open mem file, '%s', for process %d", filename, pSharedProcess[i].nPid);*/
	    }
#endif

/* deallocation */
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

#endif

/*@
   MPIDI_CH3I_SHM_Get_mem - allocate and get the address and size of a shared memory block

   Parameters:
+  int size - size
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Get_mem(int size, MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_SHMGET
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE )
    {
	MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "Error: unable to allocate %u bytes of shared memory.n", size);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return -1;
    }

    /* Create the shared memory object */
#ifdef HAVE_SHMGET
    srand(getpid());
    for (i=0; i<10; i++)
    {
	pOutput->key = rand();
	pOutput->id = shmget(pOutput->key, size, IPC_EXCL | IPC_CREAT | SHM_R | SHM_W);
	if (pOutput->id != -1)
	    break;
    }
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", pOutput->error); /*"Error in shmget, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#elif defined (HAVE_CREATEFILEMAPPING)
    generate_shm_string(pOutput->key);
    pOutput->id = CreateFileMapping(
	INVALID_HANDLE_VALUE,
	NULL,
	PAGE_READWRITE,
	0, 
	size,
	pOutput->key);
    if (pOutput->id == NULL) 
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", pOutput->error); /*"Error in CreateFileMapping, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#else
#error *** No shared memory allocation function specified ***
#endif

    pOutput->addr = NULL;
#ifdef HAVE_SHMAT
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#elif defined(HAVE_MAPVIEWOFFILE)
    pOutput->addr = MapViewOfFileEx(
	pOutput->id,
	FILE_MAP_WRITE,
	0, 0,
	size,
	NULL
	);
    if (pOutput->addr == NULL)
    {
	pOutput->error = GetLastError();
	MPIDI_err_printf("MPIDI_CH3I_SHM_Get_mem", "Error in MapViewOfFileEx, %d\n", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return -1;
    }
#else
#error *** No shared memory mapping function specified ***
#endif

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Attach_to_mem - attach to an existing shmem queue

   Parameters:
+  MPIDI_CH3I_Shmem_block_request_result* pInput - input
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
@*/
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput, MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    /* Create the shared memory object */
#ifdef HAVE_SHMGET
    pOutput->id = shmget(pInput->key, pInput->size, SHM_R | SHM_W);
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", pOutput->error); /*"Error in shmget, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#elif defined (HAVE_CREATEFILEMAPPING)
    MPIU_DBG_PRINTF(("MPIDI_CH3I_SHM_Attach_to_mem: Creating file mapping of size %d named %s\n", pInput->size, pInput->key));
    pOutput->id = CreateFileMapping(
	INVALID_HANDLE_VALUE,
	NULL,
	PAGE_READWRITE,
	0, 
	pInput->size,
	pInput->key);
    if (pOutput->id == NULL) 
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", pOutput->error); /*"Error in CreateFileMapping, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#else
#error *** No shared memory allocation function specified ***
#endif

    pOutput->addr = NULL;
#ifdef HAVE_SHMAT
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#elif defined(HAVE_MAPVIEWOFFILE)
    pOutput->addr = MapViewOfFileEx(
	pOutput->id,
	FILE_MAP_WRITE,
	0, 0,
	pInput->size,
	NULL
	);
    if (pOutput->addr == NULL)
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error); /*"MPIDI_CH3I_SHM_Get_mem: Error in MapViewOfFileEx, %d", pOutput->error);*/
	MPIDI_ERR_PRINTF(("MPIDI_CH3I_SHM_Get_mem", "Error in MapViewOfFileEx, %d\n", pOutput->error));
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#else
#error *** No shared memory mapping function specified ***
#endif

    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    
#ifdef HAVE_SHMDT
    shmdt(p->addr);
#endif
#ifdef HAVE_SHMCTL
    shmctl(p->id, IPC_RMID, NULL);
#endif
#ifdef HAVE_UNMAPVIEWOFFILE
    UnmapViewOfFile(p->addr);
#endif
#ifdef HAVE_CREATEFILEMAPPING
    CloseHandle(p->id);
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
