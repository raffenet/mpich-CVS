#include "mpid_nem.h"
#include "pm.h"
#include "gm_module.h"
#include "tcp_module.h"

#if MEM_REGION_IN_HEAP
MPID_nem_mem_region_t *MPID_nem_mem_region_ptr;
#else
MPID_nem_mem_region_t MPID_nem_mem_region;
#endif

char MPID_nem_err_str[MAX_ERR_STR_LEN] = "";

#define MIN( a , b ) ((a) >  (b)) ? (b) : (a)
#define MAX( a , b ) ((a) >= (b)) ? (a) : (b)

#define ERROR(err...) ERROR_RET (-1, err)

char *MPID_nem_asymm_base_addr;
char *MPID_nem_asymm_null_var;

int get_local_procs (int *num_local, int **local_procs, int *local_rank);
int get_MPID_nem_key();

int
MPID_nem_init (int argc, char **argv, int *myrank, int *num_procs)
{
    return  _MPID_nem_init (argc, argv, myrank, num_procs, 0);
}

int
_MPID_nem_init (int argc, char **argv, int *myrank, int *num_procs, int ckpt_restart)
{   
    char             name[MPID_NEM_MAX_FNAME_LEN]  = "/tmp/shmem.map_";
    pid_t            my_pid;
    int              num_processes;
    int              rank;
    int              errno;
    int              num_local;
    int             *local_procs;
    int              local_rank;
    int              global_size;
    int              index, index2, size;
    int i;

    errno = pm_init (&num_processes, &rank);
    if (errno != 0)
	FATAL_ERROR ("pm_init() failed");

    errno = get_local_procs (&num_local, &local_procs, &local_rank);
    if (errno != 0)
	FATAL_ERROR ("get_local_procs() failed");

#if MEM_REGION_IN_HEAP
    MPID_nem_mem_region_ptr = malloc (sizeof(MPID_nem_mem_region_t));
    if (!MPID_nem_mem_region_ptr)
	FATAL_ERROR ("failed to allocate mem_region");
#endif

    
    MPID_nem_mem_region.num_seg        = 6;
    MPID_nem_mem_region.seg            = (MPID_nem_seg_info_ptr_t)MALLOC (MPID_nem_mem_region.num_seg * sizeof(MPID_nem_seg_info_t));
    if (MPID_nem_mem_region.seg == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.pid            = (pid_t *)MALLOC (num_local * sizeof(pid_t));
    if (MPID_nem_mem_region.pid == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.rank           = rank;
    MPID_nem_mem_region.num_local      = num_local;
    MPID_nem_mem_region.num_procs      = num_processes;
    MPID_nem_mem_region.local_procs    = local_procs;
    MPID_nem_mem_region.local_rank     = local_rank;
    MPID_nem_mem_region.local_ranks    = (int *)MALLOC (num_processes* sizeof(int));
    if (MPID_nem_mem_region.local_ranks == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.ext_procs      = num_processes - num_local ; 
    MPID_nem_mem_region.ext_ranks      = (int *)MALLOC (MPID_nem_mem_region.ext_procs * sizeof(int));
    if (MPID_nem_mem_region.ext_ranks == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.next           = NULL;
    
    for (index = 0 ; index < num_processes; index++)
    {
	MPID_nem_mem_region.local_ranks[index] = MPID_NEM_NON_LOCAL;
    }
    for (index = 0; index < num_local; index++)
    {
	index2 = local_procs[index];
	MPID_nem_mem_region.local_ranks[index2] = index;
    }

    index2 = 0;
    for(index = 0 ; index < num_processes ; index++)
    {
	if( ! MPID_NEM_IS_LOCAL (index))
	{
	    MPID_nem_mem_region.ext_ranks[index2++] = index;
	}
    }
    
    /*
      fprintf(stderr,"=========== TOPOLOGY 1 for [%i]==============\n",rank);
      for(index = 0 ; index < num_processes ; index ++)
      {
      fprintf(stderr," Proc grank %i  is  lrank %i \n", 
      index,
      MPID_nem_mem_region.local_ranks[index]);
      }
      fprintf(stderr,"=========== TOPOLOGY 2 for [%i]==============\n",rank);
      for(index = 0 ; index < num_local ; index ++)
      {
      fprintf(stderr," Proc lrank %i  is  grank %i \n", 
      index,
      MPID_nem_mem_region.local_procs[index]);
      }
      fprintf(stderr,"=========== TOPOLOGY 3 for [%i ]==============\n",rank);
      for(index = 0 ; index < MPID_nem_mem_region.ext_procs ; index ++)
      {
      fprintf(stderr," Ext proc index %i  is  grank %i \n", 
      index,
      MPID_nem_mem_region.ext_ranks[index]);
      }    
    */

    /* Global size for the segment */
    /* Data cells + Header Qs + control blocks + Net data cells + POBoxes */
    global_size = ((num_local * (((MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t))  +
				 (2 * sizeof(MPID_nem_queue_t))            +
				 (sizeof(int))                       +
				 ((MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t))))+   
		   (MAX((num_local * ((num_local-1) * sizeof(MPID_nem_fastbox_t))) , MPID_NEM_ASYMM_NULL_VAL)) +
		   sizeof (MPID_nem_barrier_t));
    
#ifdef FORCE_ASYM
    {
	char name[MPID_NEM_MAX_FNAME_LEN] = "/tmp/shmem.map2_";
	char *user              = getenv("USER");
	char  file_name[MPID_NEM_MAX_FNAME_LEN];
	int   descs; 
	int size                 = (local_rank * 65536) + 65536;
	char *base_addr;

	strcat (name, user);
	strncpy (file_name, name, MPID_NEM_MAX_FNAME_LEN);
	descs   = open(file_name,O_RDWR | O_CREAT, S_IRWXU);
	ftruncate(descs, size);
	base_addr    = (char *)mmap(NULL,
				    size,
				    PROT_READ | PROT_WRITE ,
				    MAP_SHARED ,
				    descs,0);
    }
    //fprintf(stderr,"[%i] -- address shift ok \n",rank);
#endif  //FORCE_ASYM

/*     if (num_local > 1) */
/* 	MPID_nem_mem_region.map_lock = make_sem (local_rank, num_local, 0); */
    
    MPID_nem_seg_create (&(MPID_nem_mem_region.memory), global_size, name, num_local, local_rank);
    MPID_nem_check_alloc (num_local);    

    /* Fastpath boxes */
    size =  MAX((num_local*((num_local-1)*sizeof(MPID_nem_fastbox_t))), MPID_NEM_ASYMM_NULL_VAL);
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[0]), size);      

    /* Data cells */
    size =  num_local * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t);
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[1]), size);

    /* Network data cells */
    size =  num_local  * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t);
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[2]), size);

    /* Header Qs */
    size = num_local * (2 * sizeof(MPID_nem_queue_t));
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[3]), size);

    /* Control blocks */
    size = num_local * (sizeof(int));
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[4]), size);

    /* Barrier data */
    size = sizeof(MPID_nem_barrier_t);
    MPID_nem_seg_alloc (&(MPID_nem_mem_region.memory), &(MPID_nem_mem_region.seg[5]), size);

    /* set up barrier region */
    MPID_nem_barrier_init ((MPID_nem_barrier_t *)(MPID_nem_mem_region.seg[5].addr));	
    
    errno = PMI_Barrier();
    if (errno != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", errno);

    my_pid = getpid();
    MPID_NEM_MEMCPY(&(((pid_t *)(MPID_nem_mem_region.seg[0].addr))[local_rank]),
	   &my_pid,
	   sizeof(pid_t));
   
    /* syncro part */  
    MPID_nem_barrier (num_local, local_rank);   
    for (index = 0 ; index < num_local ; index ++)
    {
	MPID_nem_mem_region.pid[index] = (((pid_t *)MPID_nem_mem_region.seg[0].addr)[index]);
    }
    MPID_nem_barrier (num_local, local_rank);   

    /*
      for (index = 0 ; index < num_local ; index ++)
      {
      fprintf(stderr,"[%i] PID[%i] is %i \n", rank, index, MPID_nem_mem_region.pid[index]);
      }   
      MPID_nem_barrier (num_local, local_rank);   
    */

    /* SHMEM QS */
    MPID_nem_mem_region.Elements = (MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[1].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));    
    MPID_nem_mem_region.FreeQ = (MPID_nem_queue_ptr_t *)MALLOC (num_processes * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.RecvQ =( MPID_nem_queue_ptr_t *)MALLOC (num_processes * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.net_elements = (MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[2].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));

    MPID_nem_mem_region.FreeQ[rank] = 
	(MPID_nem_queue_ptr_t)(MPID_NEM_ABS_TO_REL(((char *)MPID_nem_mem_region.seg[3].addr + local_rank * sizeof(MPID_nem_queue_t))));
    
    MPID_nem_mem_region.RecvQ[rank] = 
	(MPID_nem_queue_ptr_t)(MPID_NEM_ABS_TO_REL(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + local_rank) * sizeof(MPID_nem_queue_t))));

    /* Free Q init and building*/
    MPID_nem_rel_queue_init (MPID_nem_mem_region.FreeQ[rank] );
    for (index = 0; index < MPID_NEM_NUM_CELLS; ++index)
    {         
	MPID_nem_rel_cell_init (&(MPID_nem_mem_region.Elements[index]));       
	MPID_nem_rel_queue_enqueue (MPID_nem_mem_region.FreeQ[rank], 
			   MPID_NEM_ABS_TO_REL(&(MPID_nem_mem_region.Elements[index])));
    }

    /* Recv Q init only*/
    MPID_nem_rel_queue_init (MPID_nem_mem_region.RecvQ[rank]);

    close (MPID_nem_mem_region.memory.base_descs);   

    /* network init */
    switch (MPID_NEM_NET_MODULE)
    {
    case MPID_NEM_GM_MODULE:
	if (rank == 0)
	{
	    //fprintf (stderr, "Using GM module\n");
	}
	errno = gm_module_init ( MPID_NEM_REL_TO_ABS(MPID_nem_mem_region.RecvQ[rank]), 
				 MPID_NEM_REL_TO_ABS(MPID_nem_mem_region.FreeQ[rank]), 
				 MPID_nem_mem_region.Elements, 
				 MPID_NEM_NUM_CELLS,
				 MPID_nem_mem_region.net_elements, 
				 MPID_NEM_NUM_CELLS, 
				 &MPID_nem_mem_region.net_recv_queue, 
				 &MPID_nem_mem_region.net_free_queue,
				 ckpt_restart);
	if (errno != 0)
	    FATAL_ERROR ("gm_module_init() failed");
	break;
    case MPID_NEM_TCP_MODULE:
	{
	    if (rank == 0)
	      {
		  //fprintf (stderr, "Using TCP module\n");
	      }
	    errno = tcp_module_init (MPID_NEM_REL_TO_ABS(MPID_nem_mem_region.RecvQ[rank]), 
				     MPID_NEM_REL_TO_ABS(MPID_nem_mem_region.FreeQ[rank]), 
				     MPID_nem_mem_region.Elements, 
				     MPID_NEM_NUM_CELLS,
				     MPID_nem_mem_region.net_elements, 
				     MPID_NEM_NUM_CELLS, 
				     &MPID_nem_mem_region.net_recv_queue, 
				     &MPID_nem_mem_region.net_free_queue,
				     ckpt_restart);
	    if (errno != 0)
		FATAL_ERROR ("tcp_module_init() failed");
	}
	break;
    default:
	if (rank == 0)
	    //fprintf (stderr, "Using no network module\n");
	MPID_nem_mem_region.net_recv_queue = NULL;
	MPID_nem_mem_region.net_free_queue = NULL;
	break;
    }

    /* set default route for only external processes through network */
    for (index = 0 ; index < MPID_nem_mem_region.ext_procs ; index++)
    {
	index2 = MPID_nem_mem_region.ext_ranks[index];
	MPID_nem_mem_region.FreeQ[index2] = MPID_nem_mem_region.net_free_queue;
	MPID_nem_mem_region.RecvQ[index2] = MPID_nem_mem_region.net_recv_queue;
    }

    
    /* set route for local procs through shmem */   
    for (index = 0; index < num_local; index++)
    {
	index2 = local_procs[index];
	MPID_nem_mem_region.FreeQ[index2] = 
	    (MPID_nem_queue_ptr_t)(MPID_NEM_ABS_TO_REL(((char *)MPID_nem_mem_region.seg[3].addr + index * sizeof(MPID_nem_queue_t))));	
	MPID_nem_mem_region.RecvQ[index2] =
	    (MPID_nem_queue_ptr_t)(MPID_NEM_ABS_TO_REL(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + index) * sizeof(MPID_nem_queue_t))));
	assert (MPID_NEM_ALIGNED (MPID_NEM_REL_TO_ABS (MPID_nem_mem_region.FreeQ[index2]), MPID_NEM_CACHE_LINE_LEN));
	assert (MPID_NEM_ALIGNED (MPID_NEM_REL_TO_ABS (MPID_nem_mem_region.RecvQ[index2]), MPID_NEM_CACHE_LINE_LEN));

    }
    MPID_nem_barrier (num_local, local_rank);

    /* POboxes stuff */
    MPID_nem_mem_region.mailboxes.in  = (MPID_nem_fastbox_t **)MALLOC((num_local)*sizeof(MPID_nem_fastbox_t *));
    MPID_nem_mem_region.mailboxes.out = (MPID_nem_fastbox_t **)MALLOC((num_local)*sizeof(MPID_nem_fastbox_t *));

    if (num_local > 1)
    {

#define MAILBOX_INDEX(sender, receiver) ( ((sender) > (receiver)) ? ((num_local-1) * (sender) + (receiver)) :		\
                                         (((sender) < (receiver)) ? ((num_local-1) * (sender) + ((receiver)-1)) : 0) )

	for (i = 0; i < num_local; ++i)
	{
	    if (i == local_rank)
	    {
		MPID_nem_mem_region.mailboxes.in [i] = NULL ;
		MPID_nem_mem_region.mailboxes.out[i] = NULL ;
	    }
	    else
	    {
		MPID_nem_mem_region.mailboxes.in [i] =  ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (i, local_rank);
		MPID_nem_mem_region.mailboxes.out[i] =  ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (local_rank, i);
		MPID_nem_mem_region.mailboxes.in [i]->common.flag.value  = 0;
		MPID_nem_mem_region.mailboxes.out[i]->common.flag.value  = 0;	   
	    }
	}
#undef MAILBOX_INDEX
    }


    MPID_nem_barrier (num_local, local_rank);
    MPID_nem_mpich2_init (ckpt_restart);
    MPID_nem_barrier (num_local, local_rank);

#if MPID_NEM_CKPT_ENABLED
    MPID_nem_ckpt_init (ckpt_restart);
#endif
    

#ifdef PAPI_MONITOR
    my_papi_start( rank );
#endif //PAPI_MONITOR  
 
    *myrank = rank;
    *num_procs = num_processes;


    return 0;
}



/* get_local_procs() determines which processes are local and should use shared memory

   OUT
     num_local -- number of local processes
     local_procs -- array of global ranks of local processes
     local_rank -- our local rank

   This uses PMI to get all of the processes that have the same
   hostname, and puts them into local_procs sorted by global rank.
*/
int
get_local_procs (int *num_local, int **local_procs, int *local_rank)
{
    int errno;
    int global_rank;
    int global_size;
    int *procs;
    int i;
    
    errno = PMI_Get_rank (&global_rank);
    if (errno != 0)
	ERROR_RET (-1, "PMI_Get_rank failed %d", errno);
    
    errno = PMI_Get_size (&global_size);
    if (errno != 0)
	ERROR_RET (-1, "PMI_Get_size failed %d", errno);

    memset(pmi_val, 0, pmi_val_max_sz);
    snprintf (pmi_val, pmi_val_max_sz, "%s", MPID_nem_hostname);

    memset (pmi_key, 0, pmi_key_max_sz);
    snprintf (pmi_key, pmi_key_max_sz, "hostname[%d]", global_rank);

    /* Put my hostname id */
    errno = PMI_KVS_Put (pmi_kvs_name, pmi_key, pmi_val);
    if (errno != 0)
	ERROR_RET (-1, "PMI_KVS_Put failed %d", errno);
    
    errno = PMI_KVS_Commit (pmi_kvs_name);
    if (errno != 0)
	ERROR_RET (-1, "PMI_KVS_commit failed %d", errno);

    errno = PMI_Barrier();
    if (errno != 0)
	ERROR_RET (-1, "PMI_Barrier failed %d", errno);

    procs = MALLOC (global_size * sizeof (int));
    if (!procs)
	ERROR_RET (-1, "malloc failed");
    *num_local = 0;

    /* Gather hostnames */
    for (i = 0; i < global_size; ++i)
    {
	memset (pmi_key, 0, pmi_key_max_sz);
	snprintf (pmi_key, pmi_key_max_sz, "hostname[%d]", i);
	
	errno = PMI_KVS_Get (pmi_kvs_name, pmi_key, pmi_val, pmi_val_max_sz);
	if (errno != 0)
	    ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", errno, i);

	if (!strncmp (MPID_nem_hostname, pmi_val, pmi_val_max_sz))
	{
	    if (i == global_rank)
		*local_rank = *num_local;
	    procs[*num_local] = i;
	    ++*num_local;
	}
	
	printf_d ("  %d: %s\n", i, pmi_val);
    }

    /* copy over local procs into smaller array */
    *local_procs = MALLOC (*num_local * sizeof (int));
    if (!local_procs)
	ERROR_RET (-1, "malloc failed");
    MPID_NEM_MEMCPY (*local_procs, procs, *num_local * sizeof (int));

    FREE (procs);
    
    return 0;
}


