#include "tcp_module.h"
#include "tcp_module_impl.h"
#include "mpidimpl.h"
#include "mpid_nem.h"
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>

#define ERROR(err...) ERROR_RET (-1, err)

#define safe_malloc(x) _safe_malloc(x, __FILE__, __LINE__)
static inline void *
_safe_malloc (size_t len, char* file, int line)
{
    void *p;

    p = MPIU_Malloc (len);
    if (p)
	return p;
    else
	FATAL_ERROR ("malloc failed at %s:%d\n", file, line);
}


static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t module_tcp_recv_queue;
MPID_nem_queue_ptr_t module_tcp_free_queue;

MPID_nem_queue_ptr_t process_recv_queue;
MPID_nem_queue_ptr_t process_free_queue;


mpid_nem_tcp_internal_t MPID_nem_tcp_internal_vars ;

static int init_tcp (MPIDI_PG_t *pg_p) 
{
    int             numprocs = MPID_nem_mem_region.ext_procs;
    unsigned int    len      = sizeof(struct sockaddr_in);
    int             port     = 0 ;  
    int             ret;
    int             grank;
    int             index;
    node_t         *MPID_nem_tcp_nodes;
   
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");    

   
    /* Allocate more than used, but fill only the external ones */
    MPID_nem_tcp_internal_vars.nodes = safe_malloc (sizeof (node_t) * MPID_nem_mem_region.num_procs);
    MPID_nem_tcp_nodes = MPID_nem_tcp_internal_vars.nodes ;
   
    /* All Masters create their sockets and put their keys w/PMI */
    for(index = 0 ; index < numprocs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if(grank > MPID_nem_mem_region.rank)
	{
	    struct sockaddr_in temp;
	    char              s[255];
	    int               len2 = 255;

	    MPID_nem_tcp_nodes[grank].desc = socket(AF_INET, SOCK_STREAM, 0);	  	    
	    temp.sin_family      = AF_INET;
	    temp.sin_addr.s_addr = htonl(INADDR_ANY);
	    temp.sin_port        = htons(port);	
	  
	    ret = bind(MPID_nem_tcp_nodes[grank].desc, (struct sockaddr *)&temp, len);	  
	    if(ret == -1)
		perror("bind");	      
	  
	    ret = getsockname(MPID_nem_tcp_nodes[grank].desc, 
			      (struct sockaddr *)&(MPID_nem_tcp_nodes[grank].sock_id), 
			      &len);	 
	    if(ret == -1)
		perror("getsockname");
	  
	    ret = listen(MPID_nem_tcp_nodes[grank].desc, SOMAXCONN);	      
	    if(ret == -1)
		perror("listen");	      
	  
	    /* Put the key (machine name, port #, src , dest) with PMI */
	    gethostname(s, len2);
#ifdef TRACE
	    fprintf(stderr,"[%i] ID :  %s_%d_%d_%d \n",MPID_nem_mem_region.rank,s,
		    ntohs(MPID_nem_tcp_nodes[grank].sock_id.sin_port),grank,MPID_nem_mem_region.rank);
#endif
	    snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%d:%s", ntohs(MPID_nem_tcp_nodes[grank].sock_id.sin_port), s);
	    snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "TCPkey[%d:%d]", MPID_nem_mem_region.rank, grank);

	    /* Put my unique id */
	    ret = PMI_KVS_Put (kvs_name, key, val);
	    if (ret != 0)
		ERROR_RET (-1, "PMI_KVS_Put failed %d", ret);
	  
	    ret = PMI_KVS_Commit (kvs_name);
	    if (ret != 0)
		ERROR_RET (-1, "PMI_KVS_commit failed %d", ret);
	}       
	else if (grank < MPID_nem_mem_region.rank)
	{
	    struct sockaddr_in temp;
	    MPID_nem_tcp_nodes[grank].desc   = socket(AF_INET, SOCK_STREAM, 0);
	    temp.sin_family      = AF_INET;
	    temp.sin_addr.s_addr = htonl(INADDR_ANY);
	    temp.sin_port        = htons(port);
	    ret = bind(MPID_nem_tcp_nodes[grank].desc, (struct sockaddr *)&temp, len);
	    if(ret == -1)
		perror("bind");

	    ret = getsockname(MPID_nem_tcp_nodes[grank].desc,
			      (struct sockaddr *)&(MPID_nem_tcp_nodes[grank].sock_id),
			      &len);
	    if(ret == -1)
		perror("getsockname");

	}
    }
    ret = PMI_Barrier();
    if (ret != 0)
	ERROR_RET (-1, "PMI_Barrier failed %d", ret);

#ifdef TRACE
    fprintf(stderr,"[%i] ---- Creating sockets done \n",MPID_nem_mem_region.rank);	  	  
#endif

    /* Connect/accept sequence */
    for(index = 0 ; index < numprocs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if(grank > MPID_nem_mem_region.rank)
	{
	    /* I am a master */
#ifdef TRACE  
	    fprintf(stderr,"MASTER accepting sockets \n");
#endif
	    MPID_nem_tcp_nodes[grank].desc = accept(MPID_nem_tcp_nodes[grank].desc,
						    (struct sockaddr *)&(MPID_nem_tcp_nodes[grank].sock_id),
						    &len);
#ifdef TRACE  
	    fprintf(stderr,"[%i] ====> ACCEPT DONE for GRANK %i\n",MPID_nem_mem_region.rank,grank);    
#endif
	}
	else if(grank < MPID_nem_mem_region.rank)
	{	      
	    /* I am the slave */
	    struct sockaddr_in  master;
	    struct hostent     *hp = NULL;
	    char                s[255];
	    int                 port_num;  
		  
	    memset(val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	    snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN,"TCPkey[%d:%d]", grank, MPID_nem_mem_region.rank);
	      
	    ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	    if (ret != 0)
		ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", ret, grank);
	  
	    ret = sscanf (val, "%d:%s", &port_num, s);
	    if ( ret != 2)
	    {
		ERROR_RET (-1, "unable to parse data from PMI_KVS_Get %s", val);
	    }

	    hp = gethostbyname(s);	  
	    master.sin_family      = AF_INET;
	    master.sin_port        = htons(port_num);
	    MPID_NEM_MEMCPY(&(master.sin_addr.s_addr), hp->h_addr, hp->h_length);

	    ret = connect(MPID_nem_tcp_nodes[grank].desc,(struct sockaddr *)&master, sizeof(master));
	    if(ret == -1)
		perror("connect");
#ifdef TRACE
	    fprintf(stderr,"====> CONNECT DONE : %i\n", ret);	      	 
#endif
	}
    }

    for(index = 0 ; index < numprocs ; index++)
    {
	int option = 1;
	grank     = MPID_nem_mem_region.ext_ranks[index];
	if(grank != MPID_nem_mem_region.rank)
	{

	    MPID_nem_tcp_nodes[grank].internal_recv_queue = (internal_queue_ptr_t)MPIU_Malloc(sizeof(internal_queue_t));
	    MPID_nem_tcp_nodes[grank].internal_free_queue = (internal_queue_ptr_t)MPIU_Malloc(sizeof(internal_queue_t));
	    MPID_nem_tcp_nodes[grank].internal_recv_queue->head = NULL;
	    MPID_nem_tcp_nodes[grank].internal_recv_queue->tail = NULL;
	    MPID_nem_tcp_nodes[grank].internal_free_queue->head = NULL;
	    MPID_nem_tcp_nodes[grank].internal_free_queue->tail = NULL;
	  
	    MPID_nem_tcp_nodes[grank].left2write     = 0;
	    MPID_nem_tcp_nodes[grank].left2read_head = 0;
	    MPID_nem_tcp_nodes[grank].left2read      = 0;
	    MPID_nem_tcp_nodes[grank].toread         = 0;

#ifdef TRACE
	    fprintf(stderr,"[%i] ----- DESC %i is %i ------ \n",
		    MPID_nem_mem_region.rank,grank,
		    MPID_nem_tcp_nodes[grank].desc);
#endif

	    FD_SET(MPID_nem_tcp_nodes[grank].desc, &MPID_nem_tcp_internal_vars.set);
	    if(MPID_nem_tcp_nodes[grank].desc > MPID_nem_tcp_internal_vars.max_fd)
		MPID_nem_tcp_internal_vars.max_fd = MPID_nem_tcp_nodes[grank].desc ;
	  
	    fcntl(MPID_nem_tcp_nodes[grank].desc, F_SETFL, O_NONBLOCK);
	    setsockopt( MPID_nem_tcp_nodes[grank].desc, 
			IPPROTO_TCP,  
			TCP_NODELAY,  
			&option, 
			sizeof(int));	  
	    option = 2 * MPID_NEM_CELL_PAYLOAD_LEN ;
	    setsockopt( MPID_nem_tcp_nodes[grank].desc, 
			SOL_SOCKET,  
			SO_RCVBUF,  
			&option, 
			sizeof(int));	  
	    setsockopt( MPID_nem_tcp_nodes[grank].desc, 
			SOL_SOCKET,  
			SO_SNDBUF,  
			&option, 
			sizeof(int));	  	  
	    /*
	      setsockopt( nodes[grank].desc, 
	      IPPROTO_TCP,  
	      TCP_MAXSEG,  
	      &option, 
	      sizeof(int));	  	 
	    */
	}
    }
    (MPID_nem_tcp_internal_vars.max_fd)++;
    return 0;
}


/*
   int  
   MPID_nem_tcp_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
                            MPID_nem_cell_ptr_t module_elements, int num_module_elements, MPID_nem_queue_ptr_t *module_recv_queue,
                            MPID_nem_queue_ptr_t *module_free_queue)

   IN
       proc_recv_queue -- main recv queue for the process
       proc_free_queue -- main free queueu for the process
       proc_elements -- pointer to the process' queue elements
       num_proc_elements -- number of process' queue elements
       module_elements -- pointer to queue elements to be used by this module
       num_module_elements -- number of queue elements for this module
       ckpt_restart -- true if this is a restart from a checkpoint.  In a restart, the network needs to be brought up again, but
                       we want to keep things like sequence numbers.
   OUT
   
       recv_queue -- pointer to the recv queue for this module.  The process will add elements to this
                     queue for the module to send
       free_queue -- pointer to the free queue for this module.  The process will return elements to
                     this queue
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_init (MPID_nem_queue_ptr_t  proc_recv_queue, 
			  MPID_nem_queue_ptr_t  proc_free_queue, 
			  MPID_nem_cell_ptr_t    proc_elements,   
			  int num_proc_elements,
			  MPID_nem_cell_ptr_t    module_elements, 
			  int num_module_elements, 
			  MPID_nem_queue_ptr_t *module_recv_queue,
			  MPID_nem_queue_ptr_t *module_free_queue,
			  int ckpt_restart, MPIDI_PG_t *pg_p, int pg_rank,
			  char **bc_val_p, int *val_max_sz_p)
{
    int ret;
    int index;
   
    MPID_nem_tcp_internal_vars.n_pending_send  = 0;
    MPID_nem_tcp_internal_vars.n_pending_recv  = 0;
    MPID_nem_tcp_internal_vars.outstanding     = 0;
    MPID_nem_tcp_internal_vars.poll_freq       = TCP_POLL_FREQ_NO;
    MPID_nem_tcp_internal_vars.old_poll_freq   = TCP_POLL_FREQ_NO;

    MPID_nem_tcp_internal_vars.n_pending_sends = (int *)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(int));    
    for(index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
    {
	MPID_nem_tcp_internal_vars.n_pending_sends[index] = 0;
    }

    if( MPID_nem_mem_region.ext_procs > 0)
    {
	ret = init_tcp (pg_p);
	if (ret != 0)
	    ERROR_RET (-1, "init_tcp() failed");
       
	if(MPID_nem_mem_region.num_local == 0)
	    MPID_nem_tcp_internal_vars.poll_freq = TCP_POLL_FREQ_ALONE ;
	else
	    MPID_nem_tcp_internal_vars.poll_freq = TCP_POLL_FREQ_MULTI ;
	MPID_nem_tcp_internal_vars.old_poll_freq = MPID_nem_tcp_internal_vars.poll_freq;	
    }

    process_recv_queue = proc_recv_queue;
    process_free_queue = proc_free_queue;

    module_tcp_recv_queue = &_recv_queue;
    module_tcp_free_queue = &_free_queue;

    MPID_nem_queue_init (module_tcp_recv_queue);
    MPID_nem_queue_init (module_tcp_free_queue);

    for (index = 0; index < num_module_elements; ++index)
    {
	MPID_nem_queue_enqueue (module_tcp_free_queue, &module_elements[index]);
    }

    *module_recv_queue = module_tcp_recv_queue;
    *module_free_queue = module_tcp_free_queue;

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_tcp_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    struct MPIDI_CH3_VC_TCP *t = &vc->ch.net.tcp;

    t->lmt_desc = -1;
    t->lmt_cookie = NULL;
    t->lmt_s_len = 0;
    t->lmt_connected = 0;
    
    vc->ch.lmt_pre_send = MPID_nem_tcp_module_lmt_pre_send;
    vc->ch.lmt_pre_recv = MPID_nem_tcp_module_lmt_pre_recv;
    vc->ch.lmt_start_send = MPID_nem_tcp_module_lmt_start_send;
    vc->ch.lmt_start_recv = MPID_nem_tcp_module_lmt_start_recv;
    vc->ch.lmt_post_send = MPID_nem_tcp_module_lmt_post_send;
    vc->ch.lmt_post_recv = MPID_nem_tcp_module_lmt_post_recv;

    return MPI_SUCCESS;
}



