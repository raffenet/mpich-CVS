/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static void generate_shm_string(char *str)
{
#ifdef USE_WINDOWS_SHM
    UUID guid;
    UuidCreate(&guid);
    sprintf(str, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    MPIU_DBG_PRINTF(("GUID = %s\n", str));
#elif defined (USE_POSIX_SHM)
    sprintf(str, "/mpich_shm_%d", rand());
#elif defined (USE_SYSV_SHM)
    sprintf(str, "%d", getpid());
#else
#error No shared memory subsystem defined
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
#if defined (USE_POSIX_SHM) || defined (USE_SYSV_SHM)
    int i;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE )
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }

    /* Create the shared memory object */
#ifdef USE_POSIX_SHM
    srand(getpid());
    for (i=0; i<10; i++)
    {
	generate_shm_string(pOutput->key);
	pOutput->id = shm_open(pOutput->key, O_RDWR | O_CREAT, 0600);
	if (pOutput->id != -1)
	    break;
    }
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pOutput->key, pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
    strcpy(pOutput->name, pOutput->key);
    if (ftruncate(pOutput->id, size) == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ftruncate", "**ftruncate %s %d %d", pOutput->key, size, pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#elif defined (USE_SYSV_SHM)
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
#elif defined (USE_WINDOWS_SHM)
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
#error No shared memory subsystem defined
#endif

    pOutput->addr = NULL;
#ifdef USE_POSIX_SHM
    pOutput->addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pOutput->id, 0);
    if (pOutput->addr == MAP_FAILED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	pOutput->addr = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#elif defined (USE_SYSV_SHM)
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#elif defined(USE_WINDOWS_SHM)
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
	return mpi_errno;
    }
#else
#error No shared memory subsystem defined
#endif

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Get_mem - allocate and get the address and size of a shared memory block

   Parameters:
+  int size - size
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Get_mem_named
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Get_mem_named(int size, MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno = MPI_SUCCESS;
#if defined (USE_POSIX_SHM)
    int i;
#elif defined (USE_SYSV_SHM)
    int i;
    FILE *fout;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);

    if (size == 0 || size > MPIDU_MAX_SHM_BLOCK_SIZE )
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**arg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }

    /* Create the shared memory object */
#ifdef USE_POSIX_SHM
    /*printf("[%d] creating a named shm object: '%s'\n", MPIR_Process.comm_world->rank, pOutput->name);*/
    pOutput->id = shm_open(pOutput->name, O_RDWR | O_CREAT, 0600);
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pOutput->name, pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
    if (ftruncate(pOutput->id, size) == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ftruncate", "**ftruncate %s %d %d", pOutput->name, size, pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#elif defined (USE_SYSV_SHM)
    /* Insert code here to convert the name into a key */
    fout = fopen(pOutput->name, "w");
    pOutput->key = ftok(pOutput->name, 12345);
    fclose(fout);
    if (pOutput->key == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**ftok", "**ftok %s %d %d", pOutput->name, 12345, pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
    pOutput->id = shmget(pOutput->key, size, IPC_EXCL | IPC_CREAT | SHM_R | SHM_W);
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", pOutput->error); /*"Error in shmget, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#elif defined (USE_WINDOWS_SHM)
    pOutput->id = CreateFileMapping(
	INVALID_HANDLE_VALUE,
	NULL,
	PAGE_READWRITE,
	0, 
	size,
	pOutput->name);
    if (pOutput->id == NULL) 
    {
	pOutput->error = GetLastError();
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateFileMapping", "**CreateFileMapping %d", pOutput->error); /*"Error in CreateFileMapping, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#else
#error No shared memory subsystem defined
#endif

    /*printf("[%d] mmapping the shared memory object\n", MPIR_Process.comm_world->rank);fflush(stdout);*/
    pOutput->addr = NULL;
#ifdef USE_POSIX_SHM
    pOutput->addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pOutput->id, 0);
    if (pOutput->addr == MAP_FAILED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	pOutput->addr = NULL;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#elif defined (USE_SYSV_SHM)
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#elif defined(USE_WINDOWS_SHM)
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
	return mpi_errno;
    }
#else
#error No shared memory subsystem defined
#endif

    pOutput->size = size;
    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Attach_to_mem - attach to an existing shmem queue

   Parameters:
+  MPIDI_CH3I_Shmem_block_request_result* pInput - input
-  MPIDI_CH3I_Shmem_block_request_result* pOutput - output

   Notes:
   The shared memory segment is unlinked after attaching so only one process can call this function.
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Attach_to_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Attach_to_mem(MPIDI_CH3I_Shmem_block_request_result *pInput, MPIDI_CH3I_Shmem_block_request_result *pOutput)
{
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);

    /* Create the shared memory object */
#ifdef USE_POSIX_SHM
    pOutput->id = shm_open(pInput->key, O_RDWR, 0600);
    if (pOutput->id == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_open", "**shm_open %s %d", pInput->key, errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    /*ftruncate(pOutput->id, size);*/ /* The sender/creator set the size */
#elif defined (USE_SYSV_SHM)
    pOutput->id = shmget(pInput->key, pInput->size, SHM_R | SHM_W);
    if (pOutput->id == -1)
    {
	pOutput->error = errno;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmget", "**shmget %d", pOutput->error); /*"Error in shmget, %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#elif defined (USE_WINDOWS_SHM)
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
#error No shared memory subsystem defined
#endif

    pOutput->addr = NULL;
#ifdef USE_POSIX_SHM
    pOutput->addr = mmap(NULL, pInput->size, PROT_READ | PROT_WRITE, MAP_SHARED /* | MAP_NORESERVE*/, pOutput->id, 0);
    if (pOutput->addr == MAP_FAILED)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**mmap", "**mmap %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    shm_unlink(pInput->key);
#elif defined (USE_SYSV_SHM)
    pOutput->addr = shmat(pOutput->id, NULL, SHM_RND);
    if (pOutput->addr == (void*)-1)
    {
	pOutput->error = errno;
	pOutput->addr = NULL;
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmat", "**shmat %d", pOutput->error); /*"Error from shmat %d", pOutput->error);*/
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
    shmctl(pOutput->id, IPC_RMID, NULL);
#elif defined(USE_WINDOWS_SHM)
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
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**MapViewOfFileEx", "**MapViewOfFileEx %d", pOutput->error);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
	return mpi_errno;
    }
#else
#error No shared memory subsystem defined
#endif

    pOutput->error = MPI_SUCCESS;

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM);
    return MPI_SUCCESS;
}

/*@
   MPIDI_CH3I_SHM_Unlink_mem - 

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Unlink_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Unlink_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    int ret_val, mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);

#ifdef USE_POSIX_SHM
    /*printf("unlinking '%s'\n", p->name);*/
    ret_val = shm_unlink(p->name);
    if (ret_val == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shm_unlink", "**shm_unlink %s %d", p->key, ret_val);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
	return mpi_errno;
    }
#elif defined (USE_SYSV_SHM)
    ret_val = shmctl(p->id, IPC_RMID, NULL);
    if (ret_val == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**shmctl", "**shmctl %d %d", p->id, ret_val);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
	return mpi_errno;
    }
#elif defined (USE_WINDOWS_SHM)
#else
#error No shared memory subsystem defined
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM);
    return mpi_errno;
}

/*@
   MPIDI_CH3I_SHM_Release_mem - 

   Notes:
@*/
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_SHM_Release_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_SHM_Release_mem(MPIDI_CH3I_Shmem_block_request_result *p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);

#ifdef USE_POSIX_SHM
    close(p->id);
#elif defined (USE_SYSV_SHM)
    shmdt(p->addr);
#elif defined (USE_WINDOWS_SHM)
    UnmapViewOfFile(p->addr);
    CloseHandle(p->id);
#else
#error No shared memory subsystem defined
#endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM);
    return MPI_SUCCESS;
}
