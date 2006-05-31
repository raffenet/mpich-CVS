/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"
#include <unistd.h>
#include <errno.h>
#include "mpidimpl.h"

void MPID_nem_seg_create(MPID_nem_seg_ptr_t memory, int size, int num_local, int local_rank, MPIDI_PG_t *pg_p)
{
    int ret;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");
    
    memory->max_size = size;
    
    if (local_rank == 0)
    {
	char filename[MPID_NEM_MAX_FNAME_LEN] = "/dev/shm/nemesis_shar_tmpXXXXXX";
	memory->base_descs = mkstemp (filename);
	if (memory->base_descs == -1)
	{
	    fprintf (stderr, "%d: Error opening shared file \"%s\": %s \n", local_rank, memory->file_name, strerror(errno));
	    exit (-1);
	}
	MPIU_Strncpy (memory->file_name, filename, MPID_NEM_MAX_FNAME_LEN);

	ret = ftruncate(memory->base_descs, memory->max_size);
	if (ret == -1)
	{
	    fprintf (stderr, "%d: Error resizing shared file to %d: %s \n", local_rank, memory->max_size, strerror(errno));
	    unlink (memory->file_name);
	    exit (-1);
	}
	
	/* post name of shared file */
	MPIU_Assert (MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);
	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "sharedFilename[%i]", MPID_nem_mem_region.rank);
	ret = PMI_KVS_Put (kvs_name, key, memory->file_name);
	if (ret != MPI_SUCCESS)
	{
	    unlink (memory->file_name);
	    FATAL_ERROR ("PMI_KVS_Put failed %d", ret);
	}
	ret = PMI_KVS_Commit (kvs_name);
	if (ret != 0)
	{
	    unlink (memory->file_name);
	    FATAL_ERROR ("PMI_KVS_Commit failed %d", ret);
	}

	ret = PMI_Barrier();
	if (ret != 0)
	{
	    unlink (memory->file_name);
	    FATAL_ERROR ("PMI_Barrier failed %d", ret);
	}
    }
    else
    {
	ret = PMI_Barrier();
	if (ret != 0)
	{
	    unlink (memory->file_name);
	    FATAL_ERROR ("PMI_Barrier failed %d", ret);
	}

	/* get name of shared file */
	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "sharedFilename[%i]", MPID_nem_mem_region.local_procs[0]);
	ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	if (ret != MPI_SUCCESS)
	{
	    FATAL_ERROR ("PMI_KVS_Get failed %d", ret);
	}

	MPIU_Strncpy (memory->file_name, val, MPID_NEM_MAX_FNAME_LEN);

	memory->base_descs = open (memory->file_name, O_RDWR);
	if (memory->base_descs == -1)
	{
	    fprintf (stderr, "%d: Error opening shared file \"%s\": %s \n", local_rank, memory->file_name, strerror(errno));
	    exit (-1);
	}
    }
    
    memory->base_addr = mmap (NULL, memory->max_size, PROT_READ | PROT_WRITE, MAP_SHARED, memory->base_descs, 0);
    if (memory->base_addr == MAP_FAILED)
    {
	fprintf (stderr, "%d: Error mmap()ing shared memory region: %s \n", local_rank, strerror(errno));
	unlink (memory->file_name);
	exit (-1);
    }
    
    do 
    {
	ret = close (memory->base_descs);
    }
    while (errno == EINTR);
    if (ret)
	fprintf (stderr, "%d: Error closing shared memory file: %s \n", local_rank, strerror(errno));
    
    memset (memory->base_addr, 0, memory->max_size);
    
    ret = PMI_Barrier();
    if (ret != 0)
    {
	unlink (memory->file_name);
	FATAL_ERROR ("PMI_Barrier failed %d", ret);
    }
    
    unlink (memory->file_name);
    
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
