/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem.h"
#include "mpidimpl.h"
#include "mpid_nem_nets.h"

#ifdef MEM_REGION_IN_HEAP
MPID_nem_mem_region_t *MPID_nem_mem_region_ptr = 0;
#else /* MEM_REGION_IN_HEAP */
MPID_nem_mem_region_t MPID_nem_mem_region = {0};
#endif /* MEM_REGION_IN_HEAP */

char MPID_nem_hostname[MAX_HOSTNAME_LEN] = "UNKNOWN";
char MPID_nem_err_str[MAX_ERR_STR_LEN] = "";

#define MIN( a , b ) ((a) >  (b)) ? (b) : (a)
#define MAX( a , b ) ((a) >= (b)) ? (a) : (b)

#define ERROR(err...) ERROR_RET (-1, err)

static int intcompar (const void *a, const void *b) { return *(int *)a - *(int *)b; }

char *MPID_nem_asymm_base_addr = 0;

static int get_local_procs (int rank, int num_procs, int *num_local, int **local_procs, int *local_rank);

int
MPID_nem_init (int rank, MPIDI_PG_t *pg_p)
{
    return  _MPID_nem_init (rank, pg_p, 0);
}

int
_MPID_nem_init (int pg_rank, MPIDI_PG_t *pg_p, int ckpt_restart)
{
    int num_procs = pg_p->size;
    pid_t            my_pid;
    int              ret;
    int              num_local;
    int             *local_procs;
    int              local_rank;
    int              global_size;
    int              index, index2, size;
    int i;
    char *publish_bc_orig = NULL;
    char *bc_val = NULL;
    int val_max_remaining;

    /* Initialize the business card */
    ret = MPIDI_CH3I_BCInit( &bc_val, &val_max_remaining );
    if (ret)
	FATAL_ERROR ("MPIDI_CH3I_BCInit failed \n");
    publish_bc_orig = bc_val;
    
    gethostname (MPID_nem_hostname, MAX_HOSTNAME_LEN);
    MPID_nem_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    ret = get_local_procs (pg_rank, num_procs, &num_local, &local_procs, &local_rank);
    if (ret != 0)
    	FATAL_ERROR ("get_local_procs() failed");
    

#ifdef MEM_REGION_IN_HEAP
    MPID_nem_mem_region_ptr = MPIU_Malloc (sizeof(MPID_nem_mem_region_t));
    if (!MPID_nem_mem_region_ptr)
	FATAL_ERROR ("failed to allocate mem_region");
#endif /* MEM_REGION_IN_HEAP */
    
    MPID_nem_mem_region.num_seg        = 6;
    MPID_nem_mem_region.seg            = (MPID_nem_seg_info_ptr_t)MPIU_Malloc (MPID_nem_mem_region.num_seg * sizeof(MPID_nem_seg_info_t));
    if (MPID_nem_mem_region.seg == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.pid            = (pid_t *)MPIU_Malloc (num_local * sizeof(pid_t));
    if (MPID_nem_mem_region.pid == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.rank           = pg_rank;
    MPID_nem_mem_region.num_local      = num_local;
    MPID_nem_mem_region.num_procs      = num_procs;
    MPID_nem_mem_region.local_procs    = local_procs;
    MPID_nem_mem_region.local_rank     = local_rank;
    MPID_nem_mem_region.local_ranks    = (int *)MPIU_Malloc (num_procs* sizeof(int));
    if (MPID_nem_mem_region.local_ranks == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.ext_procs      = num_procs - num_local ; 
    MPID_nem_mem_region.ext_ranks      = (int *)MPIU_Malloc (MPID_nem_mem_region.ext_procs * sizeof(int));
    if (MPID_nem_mem_region.ext_ranks == NULL)
	FATAL_ERROR ("malloc failed");
    MPID_nem_mem_region.next           = NULL;
    
    for (index = 0 ; index < num_procs; index++)
    {
	MPID_nem_mem_region.local_ranks[index] = MPID_NEM_NON_LOCAL;
    }
    for (index = 0; index < num_local; index++)
    {
	index2 = local_procs[index];
	MPID_nem_mem_region.local_ranks[index2] = index;
    }

    index2 = 0;
    for(index = 0 ; index < num_procs ; index++)
    {
	if( ! MPID_NEM_IS_LOCAL (index))
	{
	    MPID_nem_mem_region.ext_ranks[index2++] = index;
	}
    }

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
    /*fprintf(stderr,"[%i] -- address shift ok \n",pg_rank); */
#endif  /*FORCE_ASYM */

    /*     if (num_local > 1) */
    /* 	MPID_nem_mem_region.map_lock = make_sem (local_rank, num_local, 0); */
    
    MPID_nem_seg_create (&(MPID_nem_mem_region.memory), global_size, num_local, local_rank, pg_p);
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
    
    ret = PMI_Barrier();
    if (ret != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", ret);

    my_pid = getpid();
    MPID_NEM_MEMCPY (&(((pid_t *)(MPID_nem_mem_region.seg[0].addr))[local_rank]), &my_pid, sizeof(pid_t));
   
    /* syncro part */  
    MPID_nem_barrier (num_local, local_rank);   
    for (index = 0 ; index < num_local ; index ++)
    {
	MPID_nem_mem_region.pid[index] = (((pid_t *)MPID_nem_mem_region.seg[0].addr)[index]);
    }
    MPID_nem_barrier (num_local, local_rank);   

    /* SHMEM QS */
    MPID_nem_mem_region.Elements =
	(MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[1].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));    
    MPID_nem_mem_region.FreeQ = (MPID_nem_queue_ptr_t *)MPIU_Malloc (num_procs * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.RecvQ = (MPID_nem_queue_ptr_t *)MPIU_Malloc (num_procs * sizeof(MPID_nem_queue_ptr_t));
    MPID_nem_mem_region.net_elements =
	(MPID_nem_cell_ptr_t) (MPID_nem_mem_region.seg[2].addr + (local_rank * (MPID_NEM_NUM_CELLS) * sizeof(MPID_nem_cell_t)));

    MPID_nem_mem_region.FreeQ[pg_rank] =
	(MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + local_rank * sizeof(MPID_nem_queue_t)));
    
    MPID_nem_mem_region.RecvQ[pg_rank] = 
	(MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + local_rank) * sizeof(MPID_nem_queue_t)));

    /* Free Q init and building*/
    MPID_nem_queue_init (MPID_nem_mem_region.FreeQ[pg_rank] );
    for (index = 0; index < MPID_NEM_NUM_CELLS; ++index)
    {         
	MPID_nem_cell_init (&(MPID_nem_mem_region.Elements[index]));       
	MPID_nem_queue_enqueue (MPID_nem_mem_region.FreeQ[pg_rank], &(MPID_nem_mem_region.Elements[index]));
    }

    /* Recv Q init only*/
    MPID_nem_queue_init (MPID_nem_mem_region.RecvQ[pg_rank]);
    close (MPID_nem_mem_region.memory.base_descs);   

    /* Initialize generic net module pointers */
    MPID_nem_net_init();
    
    /* network init */
    if (MPID_NEM_NET_MODULE != MPID_NEM_NO_MODULE)
    {
	ret = MPID_nem_net_module_init (MPID_nem_mem_region.RecvQ[pg_rank],
					MPID_nem_mem_region.FreeQ[pg_rank],
					MPID_nem_mem_region.Elements, 
					MPID_NEM_NUM_CELLS,
					MPID_nem_mem_region.net_elements, 
					MPID_NEM_NUM_CELLS, 
					&MPID_nem_mem_region.net_recv_queue, 
					&MPID_nem_mem_region.net_free_queue,
					ckpt_restart, pg_p, pg_rank,
					&bc_val, &val_max_remaining);
	if (ret != 0)
	    FATAL_ERROR ("net_module_init() failed");
    }
    else{
	if (pg_rank == 0)
	{
	    MPID_nem_mem_region.net_recv_queue = NULL;
	    MPID_nem_mem_region.net_free_queue = NULL;
	}
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
	    (MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + index * sizeof(MPID_nem_queue_t)));
	MPID_nem_mem_region.RecvQ[index2] =
	    (MPID_nem_queue_ptr_t)(((char *)MPID_nem_mem_region.seg[3].addr + (num_local + index) * sizeof(MPID_nem_queue_t)));
	MPIU_Assert (MPID_NEM_ALIGNED (MPID_nem_mem_region.FreeQ[index2], MPID_NEM_CACHE_LINE_LEN));
	MPIU_Assert (MPID_NEM_ALIGNED (MPID_nem_mem_region.RecvQ[index2], MPID_NEM_CACHE_LINE_LEN));

    }

    /* make pointers to our queues global so we don't have to dereference the array */
    MPID_nem_mem_region.my_freeQ = MPID_nem_mem_region.FreeQ[MPID_nem_mem_region.rank];
    MPID_nem_mem_region.my_recvQ = MPID_nem_mem_region.RecvQ[MPID_nem_mem_region.rank];
    
    
    MPID_nem_barrier (num_local, local_rank);

    /* POboxes stuff */
    MPID_nem_mem_region.mailboxes.in  = (MPID_nem_fastbox_t **)MPIU_Malloc((num_local)*sizeof(MPID_nem_fastbox_t *));
    MPID_nem_mem_region.mailboxes.out = (MPID_nem_fastbox_t **)MPIU_Malloc((num_local)*sizeof(MPID_nem_fastbox_t *));
    
    MPIU_Assert (num_local > 0);

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
	    MPID_nem_mem_region.mailboxes.in [i] = ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (i, local_rank);
	    MPID_nem_mem_region.mailboxes.out[i] = ((MPID_nem_fastbox_t *)MPID_nem_mem_region.seg[0].addr) + MAILBOX_INDEX (local_rank, i);
	    MPID_nem_mem_region.mailboxes.in [i]->common.flag.value  = 0;
	    MPID_nem_mem_region.mailboxes.out[i]->common.flag.value  = 0;	   
	}
    }
#undef MAILBOX_INDEX    

    ret = MPIDI_PG_SetConnInfo (pg_rank, (const char *)publish_bc_orig);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_SetConnInfo failed\n");

    MPID_nem_barrier (num_local, local_rank);
    MPID_nem_mpich2_init (ckpt_restart);
    MPID_nem_barrier (num_local, local_rank);

#ifdef ENABLED_CHECKPOINTING
    MPID_nem_ckpt_init (ckpt_restart);
#endif
    

#ifdef PAPI_MONITOR
    my_papi_start( pg_rank );
#endif /*PAPI_MONITOR   */ 

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
get_local_procs (int global_rank, int num_global, int *num_local, int **local_procs, int *local_rank)
{
#if 0 /* PMI_Get_clique_(size)|(ranks) don't work with mpd */
#warning PMI_Get_clique doesnt work with mpd
    int ret;
    int *lrank_p;

    /* get an array of all processes on this node */
    ret = PMI_Get_clique_size (num_local);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_clique_size failed %d", ret);
    
    *local_procs = MPIU_Malloc (*num_local * sizeof (int));
    if (*local_procs == NULL)
	ERROR_RET (-1, "malloc failed");

    ret = PMI_Get_clique_ranks (*local_procs, *num_local);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_clique_ranks failed %d", ret);

    /* make sure it's sorted  so that ranks are consistent between processes */
    qsort (*local_procs, *num_local, sizeof (**local_procs), intcompar);

    /* find our local rank */
    lrank_p = bsearch (&global_rank, *local_procs, *num_local, sizeof (**local_procs), intcompar);
    if (lrank_p == NULL)
	ERROR_RET (-1, "Can't find our rank in local ranks");
    *local_rank = lrank_p - *local_procs;

    return 0;
    
#else

    int ret;
    int *procs;
    int i;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");

    /* Put my hostname id */
    memset (key, 0, MPID_NEM_MAX_KEY_VAL_LEN);
    snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "hostname[%d]", global_rank);

    ret = PMI_KVS_Put (kvs_name, key, MPID_nem_hostname);
    if (ret != MPI_SUCCESS)
	ERROR_RET (-1, "PMI_KVS_Put failed %d", ret);

    ret = PMI_Barrier();
    if (ret != 0)
	ERROR_RET (-1, "PMI_Barrier failed %d", ret);

    /* Gather hostnames */
    procs = MPIU_Malloc (num_global * sizeof (int));
    if (!procs)
	ERROR_RET (-1, "malloc failed");
    *num_local = 0;

    for (i = 0; i < num_global; ++i)
    {
	memset (val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	memset (key, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "hostname[%d]", i);

	ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	if (ret != MPI_SUCCESS)
	    ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", ret, i);
	
	if (!strncmp (MPID_nem_hostname, val, MPID_NEM_MAX_KEY_VAL_LEN))
	{
	    if (i == global_rank)
		*local_rank = *num_local;
	    procs[*num_local] = i;
	    ++*num_local;
	}
	
	printf_d ("  %d: %s\n", i, val);
    }
	
    /* copy over local procs into smaller array */
    *local_procs = MPIU_Malloc (*num_local * sizeof (int));
    if (!local_procs)
	ERROR_RET (-1, "malloc failed");
    MPID_NEM_MEMCPY (*local_procs, procs, *num_local * sizeof (int));

    MPIU_Free (procs);
    
    return 0;
#endif
}

/* MPID_nem_vc_init initialize nemesis' part of the vc */
#undef FUNCNAME
#define FUNCNAME MPID_nem_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int ret = MPI_SUCCESS;
    MPIDI_STATE_DECL (MPID_STATE_MPID_NEM_VC_INIT);

    MPIDI_FUNC_ENTER (MPID_STATE_MPID_NEM_VC_INIT);
    vc->ch.send_seqno = 0;
    
    /* We do different things for vcs in the COMM_WORLD pg vs other pgs
       COMM_WORLD vcs may use shared memory, and already have queues allocated
    */
    if (vc->lpid < MPID_nem_mem_region.num_procs) 
    {
	/* This vc is in COMM_WORLD */
	vc->ch.is_local = MPID_NEM_IS_LOCAL (vc->lpid);
	vc->ch.free_queue = MPID_nem_mem_region.FreeQ[vc->lpid]; /* networks and local procs have free queues */    
    }
    else
    {
	/* this vc is the result of a connect */
	vc->ch.is_local = 0;
	vc->ch.free_queue = MPID_nem_mem_region.net_free_queue;
    }
    
    if (vc->ch.is_local)
    {
	vc->ch.fbox_out = &MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
	vc->ch.fbox_in = &MPID_nem_mem_region.mailboxes.in[MPID_nem_mem_region.local_ranks[vc->lpid]]->mpich2;
	vc->ch.recv_queue = MPID_nem_mem_region.RecvQ[vc->lpid];
    }
    else
    {
	vc->ch.fbox_out = NULL;
	vc->ch.fbox_in = NULL;
	vc->ch.recv_queue = NULL;

	ret = MPID_nem_net_module_vc_init (vc, business_card);
	if (ret != MPI_SUCCESS)
	    MPIU_ERR_POP(ret);
    }
    
    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    vc->ch.sendq_head = NULL;
    
 fn_exit:
    MPIDI_FUNC_EXIT (MPID_STATE_MPID_NEM_VC_INIT);
    return ret;
 fn_fail:
    goto fn_exit;
}


int
MPID_nem_get_business_card (char *value, int length)
{
    return MPID_nem_net_module_get_business_card (&value, &length);    
}

int MPID_nem_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    return MPID_nem_net_module_connect_to_root (business_card, new_vc);
}
