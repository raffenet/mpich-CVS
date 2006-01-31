#include "mpid_nem.h"
#include <unistd.h>
#include <errno.h>
#include "pm.h"

void MPID_nem_seg_create(MPID_nem_seg_ptr_t memory, int size, int num_local, int local_rank)
{
    int ret;

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
	memset (pmi_val, 0, pmi_val_max_sz);
	snprintf (pmi_val, pmi_val_max_sz, "%s", memory->file_name);
	memset (pmi_key, 0, pmi_key_max_sz);
	ASSERT(MPID_nem_mem_region.local_procs[0] == MPID_nem_mem_region.rank);
	snprintf (pmi_key, pmi_key_max_sz, "sharedFilename[%i]",MPID_nem_mem_region.rank);
	ret = PMI_KVS_Put (pmi_kvs_name, pmi_key, pmi_val);
	if (ret != 0)
	{
	    unlink (memory->file_name);
	    FATAL_ERROR ("PMI_KVS_Put failed %d", ret);
	}
	ret = PMI_KVS_Commit (pmi_kvs_name);
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
	snprintf (pmi_key, pmi_key_max_sz, "sharedFilename[%i]",MPID_nem_mem_region.local_procs[0]);
	memset (pmi_val, 0, pmi_val_max_sz);
	ret = PMI_KVS_Get (pmi_kvs_name, pmi_key, pmi_val, pmi_val_max_sz);
	if (ret != 0)
	{
	    FATAL_ERROR ("PMI_KVS_Get failed %d", ret);
	}

	MPIU_Strncpy (memory->file_name, pmi_val, MPID_NEM_MAX_FNAME_LEN);

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
   assert( memory->size_left >= size );
  
   seg->addr = memory->current_addr;
   seg->size = size ;
   
   memory->size_left    -= size;
   memory->current_addr  = (char *)(memory->current_addr) + size;
   
   assert( (int)(memory->current_addr) <=  (int) (memory->max_addr) );   
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
