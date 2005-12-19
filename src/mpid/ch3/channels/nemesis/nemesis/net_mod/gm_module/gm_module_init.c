#include "gm.h"
#include "pm.h"
#include "gm_module_impl.h"
#include "mpid_nem.h"

#define MAX_GM_BOARDS 16
#define UNIQUE_ID_LEN 6

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

node_t *nodes;

int num_send_tokens;
int num_recv_tokens;

struct gm_port *port;

int numnodes;
int rank;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t module_gm_recv_queue;
MPID_nem_queue_ptr_t module_gm_free_queue;

MPID_nem_queue_ptr_t process_recv_queue;
MPID_nem_queue_ptr_t process_free_queue;

#define FREE_SEND_QUEUE_ELEMENTS MPID_NEM_NUM_CELLS
gm_module_send_queue_head_t gm_module_send_queue;
gm_module_send_queue_t *gm_module_send_free_queue;

static int
init_gm (int *board_id, int *port_id, unsigned char *unique_id)
{
    gm_status_t status;
    int max_gm_ports;
    
    strncpy ((char *)unique_id, UNDEFINED_UNIQUE_ID_VAL, UNIQUE_ID_LEN);
    
    status = gm_init();
    if (status != GM_SUCCESS)
    {
	ERROR_RET (-1, "gm_init() failed %d", status);
    }
    
    max_gm_ports = gm_num_ports (NULL);
    
    for (*port_id = 0; *port_id < max_gm_ports; ++*port_id)
    {
	/* skip reserved gm ports */
	if (*port_id == 0 || *port_id == 1 || *port_id == 3)
	    continue;
	for (*board_id = 0; *board_id < MAX_GM_BOARDS; ++*board_id)
	{
	    status = gm_open (&port, *board_id, *port_id, " ", GM_API_VERSION);
		
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

static int
distribute_mac_ids (unsigned port_id, unsigned char *unique_id)
{
    int errno;
    int i;
    
    nodes = safe_malloc (sizeof (node_t) * numnodes);

    memset(pmi_val, 0, pmi_val_max_sz);
    snprintf (pmi_val, pmi_val_max_sz, "{%u:%Lu}", port_id, UNIQUE_TO_UINT64 (unique_id));

    memset (pmi_key, 0, pmi_key_max_sz);
    snprintf (pmi_key, pmi_key_max_sz, "portUnique[%d]", rank);

    /* Put my unique id */
    errno = PMI_KVS_Put (pmi_kvs_name, pmi_key, pmi_val);
    if (errno != 0)
	ERROR_RET (-1, "PMI_KVS_Put failed %d", errno);
    
    errno = PMI_KVS_Commit (pmi_kvs_name);
    if (errno != 0)
	ERROR_RET (-1, "PMI_KVS_commit failed %d", errno);

    errno = PMI_Barrier();
    if (errno != 0)
	ERROR_RET (-1, "PMI_Barrier failed %d", errno);

    /* Gather unique ids */
    for (i = 0; i < numnodes; ++i)
    {
	unsigned p;
	gm_u64_t u;
	memset (pmi_key, 0, pmi_key_max_sz);
	snprintf (pmi_key, pmi_key_max_sz, "portUnique[%d]", i);
	
	errno = PMI_KVS_Get (pmi_kvs_name, pmi_key, pmi_val, pmi_val_max_sz);
	if (errno != 0)
	    ERROR_RET (-1, "PMI_KVS_Get failed %d for rank %d", errno, i);

	if (sscanf (pmi_val, "{%u:%Lu}", &p, &u) != 2)
	    ERROR_RET (-1, "unable to parse data from PMI_KVS_Get %s", pmi_val);

	nodes[i].port_id = p;
	UINT64_TO_UNIQUE (u, nodes[i].unique_id);
	errno = gm_unique_id_to_node_id (port, (char *)nodes[i].unique_id, &nodes[i].node_id);
	if (errno != GM_SUCCESS)
	    ERROR_RET (-1, "gm_unique_id_to_node_id() failed for node %d %s", i, UNIQUE_TO_STR (nodes[i].unique_id));
	
	printf_d ("  %d: %s node = %d port = %d\n", i, UNIQUE_TO_STR (nodes[i].unique_id), nodes[i].node_id, nodes[i].port_id);
    }
    return 0;
}

/*
   int  
   gm_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
gm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_recv_queue,
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart)
{
    int board_id;
    int port_id;
    unsigned char unique_id[UNIQUE_ID_LEN];
    int ret;
    gm_status_t status;
    int i;

    /* FIXME: what's the right way to get (and store) our rank and numnodes? */
    ret = PMI_Get_rank (&rank);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_rank failed %d", ret);
    
    ret = PMI_Get_size (&numnodes);
    if (ret != 0)
	ERROR_RET (-1, "PMI_Get_size failed %d", ret);

    ret = init_gm (&board_id, &port_id, unique_id);
    if (ret != 0)
	ERROR_RET (-1, "init_gm() failed");
    
    ret = distribute_mac_ids (port_id, unique_id);
    if (ret != 0)
	ERROR_RET (-1, "distribute_mac_ids() failed");
    
    process_recv_queue = proc_recv_queue;
    process_free_queue = proc_free_queue;

    status = gm_register_memory (port, proc_elements, sizeof (MPID_nem_cell_t) * num_proc_elements);
    if (status != GM_SUCCESS)
	ERROR_RET (-1, "gm_register_memory() for proc elements failed");

    status = gm_register_memory (port, module_elements, sizeof (MPID_nem_cell_t) * num_module_elements);
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
	gm_provide_receive_buffer_with_tag (port, MPID_NEM_CELL_TO_PACKET (c), PACKET_SIZE, GM_LOW_PRIORITY, 0);
	--num_recv_tokens;
    }

    *module_recv_queue = module_gm_recv_queue;
    *module_free_queue = module_gm_free_queue;

    gm_module_send_queue.head = NULL;
    gm_module_send_queue.tail = NULL;

    gm_module_send_free_queue = NULL;
    
    for (i = 0; i < FREE_SEND_QUEUE_ELEMENTS; ++i)
    {
	gm_module_send_queue_t *e;
	
	e = malloc (sizeof (gm_module_send_queue_t));
	if (!e)
	    ERROR_RET (-1, "malloc failed");
	e->next = gm_module_send_free_queue;
	gm_module_send_free_queue = e;
    }
    
    ret = gm_module_lmt_init();
    if (ret)
	ERROR_RET (-1, "gm_module_lmt_init failed");

    return 0;
}

