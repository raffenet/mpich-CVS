#include "gm.h"
#include "mpidimpl.h"
#include "gm_module_impl.h"
#include "mpid_nem.h"
#include "gm_module.h"

#define MAX_GM_BOARDS 16
#define UNIQUE_ID_LEN 6
#define MPIDI_CH3I_PORT_KEY "port"
#define MPIDI_CH3I_UNIQUE_KEY "unique"


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

#define UNIQUE_TO_UINT64(un) (((((((((((gm_u64_t)un[0] << 8)	\
				      + (gm_u64_t)un[1]) << 8)	\
				    + (gm_u64_t)un[2]) << 8)	\
				  + (gm_u64_t)un[3]) << 8)	\
				+ (gm_u64_t)un[4]) << 8)	\
			      + (gm_u64_t)un[5])    

#define UINT64_TO_UNIQUE(ll, un) do {		\
    un[5] = (ll >>  0) & 0xff;			\
    un[4] = (ll >>  8) & 0xff;			\
    un[3] = (ll >> 16) & 0xff;			\
    un[2] = (ll >> 24) & 0xff;			\
    un[1] = (ll >> 32) & 0xff;			\
    un[0] = (ll >> 40) & 0xff;			\
} while (0)

#define UNIQUE_TO_STR_TMPSTR_LEN (UNIQUE_ID_LEN * 3) /* two hex digits each, a ':' between each, and a \0 at the end */
char UNIQUE_TO_STR_TMPSTR[UNIQUE_TO_STR_TMPSTR_LEN]; 
#define UNIQUE_TO_STR(un) ({														  \
    snprintf (UNIQUE_TO_STR_TMPSTR, UNIQUE_TO_STR_TMPSTR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x", un[0], un[1], un[2], un[3], un[4], un[5]); \
    UNIQUE_TO_STR_TMPSTR;														  \
})

#define UNDEFINED_UNIQUE_ID_VAL "\0\0\0\0\0\0"

//node_t *nodes;
unsigned char unique_id[UNIQUE_ID_LEN] = UNDEFINED_UNIQUE_ID_VAL;
int port_id;

int num_send_tokens;
int num_recv_tokens;

struct gm_port *port;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t module_gm_recv_queue;
MPID_nem_queue_ptr_t module_gm_free_queue;

MPID_nem_queue_ptr_t process_recv_queue;
MPID_nem_queue_ptr_t process_free_queue;

#define FREE_SEND_QUEUE_ELEMENTS MPID_NEM_NUM_CELLS
MPID_nem_gm_module_send_queue_head_t MPID_nem_gm_module_send_queue;
MPID_nem_gm_module_send_queue_t *MPID_nem_gm_module_send_free_queue;

static int
init_gm (int *boardId, int *portId, unsigned char unique_id[])
{
    gm_status_t status;
    int max_gm_ports;
    
    //    strncpy ((char *)unique_id, UNDEFINED_UNIQUE_ID_VAL, UNIQUE_ID_LEN);
    
    status = gm_init();
    if (status != GM_SUCCESS)
    {
	ERROR_RET (-1, "gm_init() failed %d", status);
    }
    
    max_gm_ports = gm_num_ports (NULL);
    
    for (*portId = 0; *portId < max_gm_ports; ++*portId)
    {
	/* skip reserved gm ports */
	if (*portId == 0 || *portId == 1 || *portId == 3)
	    continue;
	for (*boardId = 0; *boardId < MAX_GM_BOARDS; ++*boardId)
	{
	    status = gm_open (&port, *boardId, *portId, " ", GM_API_VERSION);
		
	    switch (status)
	    {
	    case GM_SUCCESS:
		/* successfuly allocated a port */
		if (gm_get_unique_board_id (port, (char *)unique_id) != GM_SUCCESS)
		{
		    memset (unique_id, 0, UNIQUE_ID_LEN);
		    ERROR_RET (-1, "Failed to get local unique id");
		}
		return 0;
		break;
	    case GM_INCOMPATIBLE_LIB_AND_DRIVER:
		ERROR_RET (-1, "GM library and driver don't match");
		break;
	    default:
		break;
	    }
		
	}
    }

    ERROR_RET (-1, "unable to allocate a GM port\n");
}

#if 0
static int
distribute_mac_ids ()
{
    int ret;
    int i;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");

    nodes = safe_malloc (sizeof (node_t) * MPID_nem_mem_region.num_procs);

    /* Put my unique id */
    snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "{%u:%Lu}", port_id, UNIQUE_TO_UINT64 (unique_id));
    snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "portUnique[%d]", MPID_nem_mem_region.rank);

    ret = PMI_KVS_Put (kvs_name, key, val);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Put failed %d", ret);
    
    ret = PMI_KVS_Commit (kvs_name);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_commit failed %d", ret);

    ret = PMI_Barrier();
    if (ret != 0)
	ERROR_RET (-1, "PMI_Barrier failed %d", ret);

    /* Gather unique ids */
    for (i = 0; i < MPID_nem_mem_region.num_procs; ++i)
    {
	unsigned p;
	gm_u64_t u;

	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "portUnique[%d]", i);
	memset (val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	
	ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	if (ret != 0)
	    ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", ret, i);

	if (sscanf (val, "{%u:%Lu}", &p, &u) != 2)
	    ERROR_RET (-1, "unable to parse data from PMI_KVS_Get %s", val);

	nodes[i].port_id = p;
	UINT64_TO_UNIQUE (u, nodes[i].unique_id);
	ret = gm_unique_id_to_node_id (port, (char *)nodes[i].unique_id, &nodes[i].node_id);
	if (ret != GM_SUCCESS)
	    ERROR_RET (-1, "gm_unique_id_to_node_id() failed for node %d %s", i, UNIQUE_TO_STR (nodes[i].unique_id));
	
	printf_d ("  %d: %s node = %d port = %d\n", i, UNIQUE_TO_STR (nodes[i].unique_id), nodes[i].node_id, nodes[i].port_id);
    }
    return 0;
}
#endif

/*
   int  
   MPID_nem_gm_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
MPID_nem_gm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_recv_queue,
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{
    int board_id;
    int ret;
    gm_status_t status;
    int i;

    ret = init_gm (&board_id, &port_id, unique_id);
    if (ret != 0)
	ERROR_RET (-1, "init_gm() failed");

/* using business cards now */
/*     ret = distribute_mac_ids (); */
/*     if (ret != 0) */
/* 	ERROR_RET (-1, "distribute_mac_ids() failed"); */
    ret = MPID_nem_gm_module_get_business_card (bc_val_p, val_max_sz_p);
    if (ret != 0)
	ERROR_RET (-1, "get_businesscard() failed");

    process_recv_queue = proc_recv_queue;
    process_free_queue = proc_free_queue;

    status = gm_register_memory (port, (void *)proc_elements, sizeof (MPID_nem_cell_t) * num_proc_elements);
    if (status != GM_SUCCESS)
	ERROR_RET (-1, "gm_register_memory() for proc elements failed");

    status = gm_register_memory (port, (void *)module_elements, sizeof (MPID_nem_cell_t) * num_module_elements);
    if (status != GM_SUCCESS)
	ERROR_RET (-1, "gm_register_memory() for module elements failed");

    module_gm_recv_queue = &_recv_queue;
    module_gm_free_queue = &_free_queue;

    MPID_nem_queue_init (module_gm_recv_queue);
    MPID_nem_queue_init (module_gm_free_queue);

    num_send_tokens = gm_num_send_tokens (port);
    num_recv_tokens = gm_num_receive_tokens (port);

    for (i = 0; i < num_module_elements; ++i)
    {
	MPID_nem_queue_enqueue (module_gm_free_queue, &module_elements[i]);
    }

    while (num_recv_tokens && !MPID_nem_queue_empty (module_gm_free_queue))
    {
	MPID_nem_cell_ptr_t c;
	MPID_nem_queue_dequeue (module_gm_free_queue, &c);
	gm_provide_receive_buffer_with_tag (port, (void *)MPID_NEM_CELL_TO_PACKET (c), PACKET_SIZE, GM_LOW_PRIORITY, 0);
	--num_recv_tokens;
    }

    *module_recv_queue = module_gm_recv_queue;
    *module_free_queue = module_gm_free_queue;

    MPID_nem_gm_module_send_queue.head = NULL;
    MPID_nem_gm_module_send_queue.tail = NULL;

    MPID_nem_gm_module_send_free_queue = NULL;
    
    for (i = 0; i < FREE_SEND_QUEUE_ELEMENTS; ++i)
    {
	MPID_nem_gm_module_send_queue_t *e;
	
	e = safe_malloc (sizeof (MPID_nem_gm_module_send_queue_t));
	e->next = MPID_nem_gm_module_send_free_queue;
	MPID_nem_gm_module_send_free_queue = e;
    }
    
    ret = MPID_nem_gm_module_lmt_init();
    if (ret)
	ERROR_RET (-1, "MPID_nem_gm_module_lmt_init failed");

    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_gm_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_gm_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port_id);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_UNIQUE_KEY, (char *)unique_id, UNIQUE_ID_LEN);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }

    return mpi_errno;
}

int
MPID_nem_gm_module_get_port_unique_from_bc (const char *business_card, int *port_id, unsigned char *unique_id)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    
    mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_PORT_KEY, port_id);
    if (mpi_errno != MPIU_STR_SUCCESS) {
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

    mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_UNIQUE_KEY, unique_id, UNIQUE_ID_LEN, &len);
    if (mpi_errno != MPIU_STR_SUCCESS || len != UNIQUE_ID_LEN) {
	/* FIXME: create a real error string for this */
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int
MPID_nem_gm_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    /* In GM, once the VC is initialized there's nothing extra that we need to do to establish a connection */
    return MPI_SUCCESS;
}

int
MPID_nem_gm_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
#if 0 /* old PMI version */ 
    int ret;
    char key[MPID_NEM_MAX_KEY_VAL_LEN];
    char val[MPID_NEM_MAX_KEY_VAL_LEN];
    char *kvs_name;
    
    unsigned p;
    gm_u64_t u;
    
    ret = MPIDI_PG_GetConnKVSname (&kvs_name);
    if (ret != MPI_SUCCESS)
	FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");

    snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "portUnique[%d]", vc->pg_rank);
    memset (val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	
    ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
    if (ret != 0)
	ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", ret, vc->pg_rank);

    if (sscanf (val, "{%u:%Lu}", &p, &u) != 2)
	ERROR_RET (-1, "unable to parse data from PMI_KVS_Get %s", val);

    vc->ch.port_id = p;
    UINT64_TO_UNIQUE (u, vc->ch.unique_id);
    ret = gm_unique_id_to_node_id (port, (char *)vc->ch.unique_id, &vc->ch.node_id);
    if (ret != GM_SUCCESS)
	ERROR_RET (-1, "gm_unique_id_to_node_id() failed for node %d %s", vc->pg_rank, UNIQUE_TO_STR (vc->ch.unique_id));
	
    printf_d ("gm info for %d: %s node = %d port = %d\n", vc->pg_rank, UNIQUE_TO_STR (vc->ch.unique_id), vc->ch.node_id, vc->ch.port_id);
#endif
    
    int mpi_errno = MPI_SUCCESS;
    int ret;

    mpi_errno = MPID_nem_gm_module_get_port_unique_from_bc (business_card, &vc->ch.port_id, vc->ch.unique_id);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno) {
	MPIU_ERR_POP (mpi_errno);
    }
    /* --END ERROR HANDLING-- */

    ret = gm_unique_id_to_node_id (port, (char *)vc->ch.unique_id, &vc->ch.node_id);
    /* --BEGIN ERROR HANDLING-- */
    if (ret != GM_SUCCESS)
    {
	mpi_errno = MPI_ERR_INTERN;
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

 fn_fail:
    return mpi_errno;
}
