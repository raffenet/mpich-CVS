/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "mpidimpl.h"

/* allocate_shared_memory allocates a shared mem region of size "length" and attaches to it.  "handle" points to a string
   descriptor for the region to be passed in to attach_shared_memory.  "handle" is dynamically allocated and should be
   freed by the caller.*/
static int allocate_shared_memory (char **buf_p, const int length, char *handle[]);
/* attach_shared_memory attaches to shared memory previously allocated by allocate_shared_memory */
static int attach_shared_memory (char **buf_p, const int length, const char const handle[]);
/* remove_shared_memory removes the OS descriptor associated with the handle.  Once all processes detatch from the region
   the OS resource will be destroyed. */
static int remove_shared_memory (const char const handle[]);
/* detach_shared_memory detaches the shared memory region from this process */
static int detach_shared_memory (const char *buf_p, const int length);

void MPID_nem_seg_create(MPID_nem_seg_ptr_t memory, int size, int num_local, int local_rank, MPIDI_PG_t *pg_p)
{
    int ret;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    char *handle;
 
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");
    
    memory->max_size = size;
    
    if (local_rank == 0)
    {
        ret = allocate_shared_memory (&memory->base_addr, memory->max_size, &handle);
        if (ret != MPI_SUCCESS)
            FATAL_ERROR ("allocate_shared_memory failed");        
	
	/* post name of shared file */
	MPIU_Assert (MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);
	MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "sharedFilename[%i]", MPID_nem_mem_region.rank);
	ret = PMI_KVS_Put (kvs_name, key, handle);
	if (ret != MPI_SUCCESS)
	{
            remove_shared_memory (handle);
	    FATAL_ERROR ("PMI_KVS_Put failed %d", ret);
	}
        
	ret = PMI_KVS_Commit (kvs_name);
	if (ret != 0)
	{
            remove_shared_memory (handle);
	    FATAL_ERROR ("PMI_KVS_Commit failed %d", ret);
	}

	ret = PMI_Barrier();
	if (ret != 0)
	{
            remove_shared_memory (handle);
	    FATAL_ERROR ("PMI_Barrier failed %d", ret);
	}
    }
    else
    {
	ret = PMI_Barrier();
	if (ret != 0)
	{
	    FATAL_ERROR ("PMI_Barrier failed %d", ret);
	}

	/* get name of shared file */
	MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "sharedFilename[%i]", MPID_nem_mem_region.local_procs[0]);
	ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	if (ret != MPI_SUCCESS)
	{
	    FATAL_ERROR ("PMI_KVS_Get failed %d", ret);
	}

        handle = val;

	ret = attach_shared_memory (&memory->base_addr, memory->max_size, handle);
	if (ret == -1)
	{
            remove_shared_memory (handle);
            FATAL_ERROR ("attach_shared_memory failed");
	}
    }    
    
    ret = PMI_Barrier();
    if (ret != 0)
    {
        remove_shared_memory (handle);
	FATAL_ERROR ("PMI_Barrier failed %d", ret);
    }
    
    if (local_rank == 0)
    {
        ret = remove_shared_memory (handle);
        if (ret != MPI_SUCCESS)
        {
            FATAL_ERROR ("remove_shared_memory failed");
        }

        MPIU_Free (handle);
    }
    
    memory->current_addr = memory->base_addr;
    memory->max_addr     = (char *)(memory->base_addr) + memory->max_size;
    memory->size_left    = memory->max_size;
    memory->symmetrical  = 0 ;   
}




void MPID_nem_seg_alloc( MPID_nem_seg_ptr_t memory, MPID_nem_seg_info_ptr_t seg, int size)
{
   MPIU_Assert( memory->size_left >= size );
  
   seg->addr = memory->current_addr;
   seg->size = size ;
   
   memory->size_left    -= size;
   memory->current_addr  = (char *)(memory->current_addr) + size;
   
   MPIU_Assert( (MPI_Aint)(memory->current_addr) <=  (MPI_Aint) (memory->max_addr) );   
}

void MPID_nem_check_alloc (int num_processes)
{
    int rank    = MPID_nem_mem_region.local_rank;
    int address = 0;
    int base, found, index;
    int ret;
    
    ret = PMI_Barrier();
    if (ret != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", ret);

    address =  ((MPID_nem_addr_t)(MPID_nem_mem_region.memory.current_addr));
    MPID_NEM_MEMCPY(&(((int*)(MPID_nem_mem_region.memory.current_addr))[rank]),
	   &address,
	   sizeof(MPID_nem_addr_t));

    ret = PMI_Barrier();
    if (ret != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", ret);

    base  = ((int *)MPID_nem_mem_region.memory.current_addr)[rank];
    found = 1 ;
    for (index = 0 ; index < num_processes ; index ++) 
      {	   
	if (index != rank )
	  {		  
	    if( (base - (MPID_nem_addr_t)(((int *)(MPID_nem_mem_region.memory.current_addr))[index])) == 0)
	      {		       
		found++;
	      }		  
	  }	     
      }
    ret = PMI_Barrier();
    if (ret != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", ret);

    if (found == num_processes)
      {	     
	/*fprintf(stderr,"[%i] ===  Symmetrical Alloc ...  \n",rank);	 */
	MPID_nem_mem_region.memory.symmetrical = 1;
	MPID_nem_asymm_base_addr = NULL;
      }	
    else 
      {	     
	/*fprintf(stderr,"[%i] ===  ASymmetrical Alloc !!!  \n",rank);	 */
	MPID_nem_mem_region.memory.symmetrical = 0;
	MPID_nem_asymm_base_addr = MPID_nem_mem_region.memory.base_addr;
#ifdef MPID_NEM_SYMMETRIC_QUEUES
	fprintf(stderr,"[%i] ===  Expecting symmetric...aborting \n",rank);	
	exit (-1);
#endif
      }
}

#ifdef HAVE_SYSV_SHARED_MEM
/* SYSV shared memory */

#define MAX_INT_STR_LEN 12
#undef FUNCNAME
#define FUNCNAME allocate_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
allocate_shared_memory (char **buf_p, const int length, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int shmid;
    static int key = 0;
    void *buf;
    struct shmid_ds ds;
    int ret;
    MPIU_CHKPMEM_DECL(1);
    
    do
    {
        ++key;
        shmid = shmget (key, length, IPC_CREAT | IPC_EXCL | S_IRWXU);
    }
    while (shmid == -1 && errno == EEXIST);
    MPIU_ERR_CHKANDJUMP2 (shmid == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "shmget", strerror (errno));

    buf = 0;
    buf = shmat (shmid, buf, 0);
    MPIU_ERR_CHKANDJUMP2 ((MPI_Aint)buf == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "shmat", strerror (errno));

    *buf_p = buf;
    
    MPIU_CHKPMEM_MALLOC (*handle, char *, MAX_INT_STR_LEN, mpi_errno, "shared memory handle");
    MPIU_Snprintf (*handle, MAX_INT_STR_LEN, "%d", shmid);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    shmctl (shmid, IPC_RMID, &ds);  /* try to remove region */
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME attach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
attach_shared_memory (char **buf_p, const int length, const char const handle[])
{
    int mpi_errno = MPI_SUCCESS;
    void *buf;
    int ret;
    int shmid;
    struct shmid_ds ds;

    shmid = atoi (handle);
    
    buf = 0;
    buf = shmat (shmid, buf, 0);
    MPIU_ERR_CHKANDJUMP2 ((MPI_Aint)buf == -1, mpi_errno, MPI_ERR_OTHER, "**attach_shar_mem", "**attach_shar_mem %s %s", "shmat", strerror (errno));
    
    *buf_p = buf;

 fn_exit:
    return mpi_errno;
 fn_fail:
    shmctl (shmid, IPC_RMID, &ds); /* try to remove region */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME remove_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
remove_shared_memory (const char const handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int shmid;
    struct shmid_ds ds;

    shmid = atoi (handle);

    ret = shmctl (shmid, IPC_RMID, &ds);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem", "**remove_shar_mem %s %s", "shmctl", strerror (errno));

 fn_exit:
    return mpi_errno;
 fn_fail:
    shmctl (shmid, IPC_RMID, &ds); /* try to remove region */
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME detach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
detach_shared_memory (const char *buf_p, const int length)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    ret = shmdt (buf_p);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem", "**detach_shar_mem %s %s", "shmdt", strerror (errno));

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#else /* HAVE_SYSV_SHARED_MEM */
/* Using memory mapped files */

#define MAX_INT_STR_LEN 12 /* chars needed to store largest integer including /0 */

#undef FUNCNAME
#define FUNCNAME allocate_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
allocate_shared_memory (char **buf_p, const int length, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int fd;
    struct stat buf;
    int ret;
    MPIU_CHKPMEM_DECL(1);


    /* create a file */
    /* use /dev/shm if it's there, otherwise put file in /tmp */
    if (stat ("/dev/shm", &buf) == 0 && S_ISDIR (buf.st_mode))
    {
        const char fname[] = "/dev/shm/nemesis_shar_tmpXXXXXX";
        MPIU_CHKPMEM_MALLOC (*handle, char *, sizeof (fname), mpi_errno, "shared memory handle");
        memcpy (*handle, fname, sizeof (fname));
    }
    else
    {
        const char fname[] = "/tmp/nemesis_shar_tmpXXXXXX";
        MPIU_CHKPMEM_MALLOC (*handle, char *, sizeof (fname), mpi_errno, "shared memory handle");
        memcpy (*handle, fname, sizeof (fname));
    }
    
    fd = mkstemp (*handle);
    MPIU_ERR_CHKANDJUMP2 (fd == -1, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "mkstmp", strerror (errno));

    /* set file to "length" bytes */
    ret = lseek (fd, length-1, SEEK_SET);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "lseek", strerror (errno));
    do
    {
        ret = write (fd, "", 1);
    }
    while (ret == -1 && errno == EINTR || ret == 0);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "lseek", strerror (errno));

    /* mmap the file */
    *buf_p = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    MPIU_ERR_CHKANDSTMT2 (*buf_p == MAP_FAILED, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "mmap", strerror (errno));

    /* close the file */
    do
    {
        ret = close (fd);
    }
    while (ret == -1 && errno == EINTR);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**alloc_shar_mem", "**alloc_shar_mem %s %s", "close", strerror (errno));
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_close_fail:
    close (fd);
    unlink (*handle);
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME attach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
attach_shared_memory (char **buf_p, const int length, const char const handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int fd;

    fd = open (handle, O_RDWR);
    MPIU_ERR_CHKANDJUMP2 (fd == -1, mpi_errno, MPI_ERR_OTHER,  "**attach_shar_mem", "**attach_shar_mem %s %s", "open", strerror (errno));
    
     /* mmap the file */
    *buf_p = mmap (NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    MPIU_ERR_CHKANDSTMT2 (*buf_p == MAP_FAILED, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**attach_shar_mem", "**attach_shar_mem %s %s", "open", strerror (errno));

    /* close the file */
    do
    {
        ret = close (fd);
    }
    while (ret == -1 && errno == EINTR);
    MPIU_ERR_CHKANDSTMT2 (ret == -1, mpi_errno, MPI_ERR_OTHER, goto fn_close_fail, "**attach_shar_mem", "**attach_shar_mem %s %s", "close", strerror (errno));
    
 fn_exit:
    return mpi_errno;
 fn_close_fail:
    close (fd);
 fn_fail:
    unlink (handle);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME remove_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
remove_shared_memory (const char const handle[])
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    ret = unlink (handle);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER,  "**remove_shar_mem", "**remove_shar_mem %s %s", "unlink", strerror (errno));

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME detach_shared_memory
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int
detach_shared_memory (const char *buf_p, const int length)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;

    ret = munmap ((void *)buf_p, length);
    MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER,  "**detach_shar_mem", "**detach_shar_mem %s %s", "munmap", strerror (errno));
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#endif /* HAVE_SYSV_SHARED_MEM */
