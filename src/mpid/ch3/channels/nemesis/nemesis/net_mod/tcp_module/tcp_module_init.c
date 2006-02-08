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

    p = MALLOC (len);
    if (p)
	return p;
    else
	FATAL_ERROR ("malloc failed at %s:%d\n", file, line);
}



/*****************************/
fd_set  set;
node_t *nodes;

int     max_fd;

int  n_pending_send  = 0;
int *n_pending_sends = NULL;
int  n_pending_recv  = 0;
int  outstanding     = 0;

int poll_freq      = TCP_POLL_FREQ_NO;
int old_poll_freq  = TCP_POLL_FREQ_NO;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t module_tcp_recv_queue;
MPID_nem_queue_ptr_t module_tcp_free_queue;

MPID_nem_queue_ptr_t process_recv_queue;
MPID_nem_queue_ptr_t process_free_queue;

static int init_tcp (MPIDI_PG_t *pg_p) 
{
    int    numprocs = MPID_nem_mem_region.ext_procs;
    unsigned int    len      = sizeof(struct sockaddr_in);
    int    port     = 0 ;  
    int    ret;
    int    grank;
    int    index;

    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");

    /* Allocate more than used, but fill only the external ones */
    nodes = safe_malloc (sizeof (node_t) * MPID_nem_mem_region.num_procs);

    /* All Masters create their sockets and put their keys w/PMI */
    for(index = 0 ; index < numprocs ; index++)
    {
	grank = MPID_nem_mem_region.ext_ranks[index];
	if(grank > MPID_nem_mem_region.rank)
	{
	    struct sockaddr_in temp;
	    char              s[255];
	    int               len2 = 255;

	    nodes[grank].desc = socket(AF_INET, SOCK_STREAM, 0);	  	    
	    temp.sin_family      = AF_INET;
	    temp.sin_addr.s_addr = htonl(INADDR_ANY);
	    temp.sin_port        = htons(port);	
	  
	    ret = bind(nodes[grank].desc, (struct sockaddr *)&temp, len);	  
	    if(ret == -1)
		perror("bind");	      
	  
	    ret = getsockname(nodes[grank].desc, 
			      (struct sockaddr *)&(nodes[grank].sock_id), 
			      &len);	 
	    if(ret == -1)
		perror("getsockname");
	  
	    ret = listen(nodes[grank].desc, SOMAXCONN);	      
	    if(ret == -1)
		perror("listen");	      
	  
	    /* Put the key (machine name, port #, src , dest) with PMI */
	    gethostname(s, len2);
#ifdef TRACE
	    fprintf(stderr,"[%i] ID :  %s_%d_%d_%d \n",MPID_nem_mem_region.rank,s,
		    ntohs(nodes[grank].sock_id.sin_port),grank,MPID_nem_mem_region.rank);
#endif
	    snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%d:%s", ntohs(nodes[grank].sock_id.sin_port), s);
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
	    nodes[grank].desc   = socket(AF_INET, SOCK_STREAM, 0);
	    temp.sin_family      = AF_INET;
	    temp.sin_addr.s_addr = htonl(INADDR_ANY);
	    temp.sin_port        = htons(port);
	    ret = bind(nodes[grank].desc, (struct sockaddr *)&temp, len);
	    if(ret == -1)
		perror("bind");

	    ret = getsockname(nodes[grank].desc,
			      (struct sockaddr *)&(nodes[grank].sock_id),
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
	    nodes[grank].desc = accept(nodes[grank].desc,
				       (struct sockaddr *)&(nodes[grank].sock_id),
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

	    ret = connect(nodes[grank].desc,(struct sockaddr *)&master, sizeof(master));
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

	    nodes[grank].internal_recv_queue = (internal_queue_ptr_t)MALLOC(sizeof(internal_queue_t));
	    nodes[grank].internal_free_queue = (internal_queue_ptr_t)MALLOC(sizeof(internal_queue_t));
	    nodes[grank].internal_recv_queue->head = NULL;
	    nodes[grank].internal_recv_queue->tail = NULL;
	    nodes[grank].internal_free_queue->head = NULL;
	    nodes[grank].internal_free_queue->tail = NULL;
	  
	    nodes[grank].left2write     = 0;
	    nodes[grank].left2read_head = 0;
	    nodes[grank].left2read      = 0;
	    nodes[grank].toread         = 0;

#ifdef TRACE
	    fprintf(stderr,"[%i] ----- DESC %i is %i ------ \n",
		    MPID_nem_mem_region.rank,grank,
		    nodes[grank].desc);
#endif

	    FD_SET(nodes[grank].desc, &set);
	    if(nodes[grank].desc > max_fd)
		max_fd = nodes[grank].desc ;
	  
	    fcntl(nodes[grank].desc, F_SETFL, O_NONBLOCK);
	    setsockopt( nodes[grank].desc, 
			IPPROTO_TCP,  
			TCP_NODELAY,  
			&option, 
			sizeof(int));	  
	    option = 2 * MPID_NEM_CELL_PAYLOAD_LEN ;
	    setsockopt( nodes[grank].desc, 
			SOL_SOCKET,  
			SO_RCVBUF,  
			&option, 
			sizeof(int));	  
	    setsockopt( nodes[grank].desc, 
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
    max_fd++;
    return 0;
}


/*
   int  
   tcp_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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

int
tcp_module_init (MPID_nem_queue_ptr_t  proc_recv_queue, 
     		 MPID_nem_queue_ptr_t  proc_free_queue, 
		 MPID_nem_cell_ptr_t    proc_elements,   
		 int num_proc_elements,
		 MPID_nem_cell_ptr_t    module_elements, 
		 int num_module_elements, 
		 MPID_nem_queue_ptr_t *module_recv_queue,
		 MPID_nem_queue_ptr_t *module_free_queue,
		 int ckpt_restart, MPIDI_PG_t *pg_p)
{
    int ret;
    int index;
    
    n_pending_sends = (int *)MALLOC(MPID_nem_mem_region.num_procs*sizeof(int));    
    for(index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
    {
	n_pending_sends[index] = 0;
    }

    if( MPID_nem_mem_region.ext_procs > 0)
    {
	ret = init_tcp (pg_p);
	if (ret != 0)
	    ERROR_RET (-1, "init_tcp() failed");

	if(MPID_nem_mem_region.num_local == 0)
	    poll_freq = TCP_POLL_FREQ_ALONE ;
	else
	    poll_freq = TCP_POLL_FREQ_MULTI ;
	old_poll_freq = poll_freq;	
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
#define FUNCNAME tcp_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
tcp_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME tcp_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
tcp_module_connect_to_root (const char *business_card, const int lpid)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME tcp_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
tcp_module_vc_init (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}



