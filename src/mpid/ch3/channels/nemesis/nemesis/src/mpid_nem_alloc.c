#include "mpid_nem.h"
#include <unistd.h>
#include <errno.h>
#include "pm.h"

void MPID_nem_seg_create(MPID_nem_seg_ptr_t memory, int size, int num_local, int local_rank)
{
    char name[MPID_NEM_MAX_FNAME_LEN]  = "/tmp/shmem.map_";
    char *user           = getenv("USER");
    int ret;
    int errno;
    
    strcat (name, user);
    strncpy (memory->file_name, name, MPID_NEM_MAX_FNAME_LEN);
    memory->max_size     = size ;
    memory->base_descs   = open (memory->file_name, O_RDWR | O_CREAT, S_IRWXU);
    if (memory->base_descs == -1)
    {
	perror ("Error opening shared file.");
	exit (-1);
    }
    
    ret = ftruncate(memory->base_descs, memory->max_size);
    if (ret == -1)
    {
	perror ("Resizing shared file failed.");
	printf ("max size = %d\n", memory->max_size);
	unlink (memory->file_name);
	exit (-1);
    }
    
    memory->base_addr = mmap (NULL, memory->max_size, PROT_READ | PROT_WRITE, MAP_SHARED, memory->base_descs, 0);
    if (memory->base_addr == MAP_FAILED)
    {
	perror ("Error mmap()ing shared memory region.");
	unlink (memory->file_name);
	exit (-1);
    }
    
    do 
    {
	ret = close (memory->base_descs);
    }
    while (ret == EINTR);
    if (ret)
	perror ("Error closing shared memory file.");
    
    memset (memory->base_addr, 0, memory->max_size);
    
    errno = PMI_Barrier();
    if (errno != 0)
    {
	unlink (memory->file_name);
	FATAL_ERROR ("PMI_Barrier failed %d", errno);
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
    int errno;
    
    errno = PMI_Barrier();
    if (errno != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", errno);

    address =  ((MPID_nem_addr_t)(MPID_nem_mem_region.memory.current_addr));
    MPID_NEM_MEMCPY(&(((int*)(MPID_nem_mem_region.memory.current_addr))[rank]),
	   &address,
	   sizeof(MPID_nem_addr_t));

    errno = PMI_Barrier();
    if (errno != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", errno);

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
    errno = PMI_Barrier();
    if (errno != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", errno);

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
