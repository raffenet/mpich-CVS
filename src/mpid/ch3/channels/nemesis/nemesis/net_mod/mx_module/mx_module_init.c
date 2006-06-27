/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "myriexpress.h"
#include "mpidimpl.h"
#include "mx_module_impl.h"
#include "mpid_nem.h"
#include "mx_module.h"

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

mx_endpoint_t       MPID_nem_module_mx_local_endpoint;
mx_endpoint_addr_t *MPID_nem_module_mx_endpoints_addr;

MPID_nem_mx_cell_ptr_t MPID_nem_module_mx_send_outstanding_request;
int                    MPID_nem_module_mx_send_outstanding_request_num;
MPID_nem_mx_cell_ptr_t MPID_nem_module_mx_recv_outstanding_request;
int                    MPID_nem_module_mx_recv_outstanding_request_num;

uint32_t            MPID_nem_module_mx_filter  = 0xdeadbeef;
static uint32_t     MPID_nem_module_mx_timeout = MX_INFINITE;
int                 MPID_nem_module_mx_pendings_sends = 0;
int                 MPID_nem_module_mx_pendings_recvs = 0 ;
int                *MPID_nem_module_mx_pendings_sends_array;
int                *MPID_nem_module_mx_pendings_recvs_array;


static MPID_nem_mx_req_queue_t _mx_send_free_req_q;
static MPID_nem_mx_req_queue_t _mx_send_pend_req_q;
static MPID_nem_mx_req_queue_t _mx_recv_free_req_q;
static MPID_nem_mx_req_queue_t _mx_recv_pend_req_q;

MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_free_req_queue    = &_mx_send_free_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_send_pending_req_queue = &_mx_send_pend_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_free_req_queue    = &_mx_recv_free_req_q;
MPID_nem_mx_req_queue_ptr_t MPID_nem_module_mx_recv_pending_req_queue = &_mx_recv_pend_req_q;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_mx_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_module_mx_free_queue = 0;

MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

int init_mx( MPIDI_PG_t *pg_p )
{
   mx_return_t ret;
   uint64_t    local_nic_id;
   uint64_t    remote_nic_id;
   uint32_t    local_endpoint_id;
   uint32_t    remote_endpoint_id;
   int         count;  
   char        key[MPID_NEM_MAX_KEY_VAL_LEN];
   char        val[MPID_NEM_MAX_KEY_VAL_LEN];
   char       *kvs_name;
   int         numprocs = MPID_nem_mem_region.ext_procs;
   int         grank;
   int         index = 0;
   
   ret = mx_init();
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_init() failed");	
   
   /* Allocate more than needed but use only external processes */
   
   MPID_nem_module_mx_endpoints_addr = (mx_endpoint_addr_t *)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(mx_endpoint_addr_t));
   MPID_nem_module_mx_send_outstanding_request = (MPID_nem_mx_cell_ptr_t)MPIU_Malloc(MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));
   memset(MPID_nem_module_mx_send_outstanding_request,0,MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));  
   MPID_nem_module_mx_recv_outstanding_request = (MPID_nem_mx_cell_ptr_t)MPIU_Malloc(MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));
   memset(MPID_nem_module_mx_recv_outstanding_request,0,MPID_NEM_MX_REQ*sizeof(MPID_nem_mx_cell_t));  
   MPID_nem_module_mx_pendings_recvs_array = (int *)MPIU_Malloc(MPID_nem_mem_region.num_procs*sizeof(int));
   for (index = 0 ; index < MPID_nem_mem_region.num_procs ; index++)
     {
	MPID_nem_module_mx_pendings_recvs_array[index] = 0;
     }   
      
   /*
   ret = mx_get_info(NULL, MX_NIC_COUNT, NULL, 0, &nic_count, sizeof(int));
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_get_info() failed");	
   
   count = ++nic_count;
   mx_nics = (uint64_t *)MPIU_Malloc(count*sizeof(uint64_t));
   ret = mx_get_info(NULL, MX_NIC_IDS, NULL, 0, mx_nics, count*sizeof(uint64_t));
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_get_info() failed");	
    
    do{	     
      ret = mx_nic_id_to_board_number(mx_nics[index],&mx_board_num);
      index++;
   }while(ret != MX_SUCCESS);
   */
   
   /* mx_board_num */
   ret = mx_open_endpoint(MX_ANY_NIC,
			  MX_ANY_ENDPOINT,
			  MPID_nem_module_mx_filter,
			  NULL,0,
			  &MPID_nem_module_mx_local_endpoint);
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_open_endpoint() failed");	
      
   ret = mx_get_endpoint_addr( MPID_nem_module_mx_local_endpoint,
			       &MPID_nem_module_mx_endpoints_addr[MPID_nem_mem_region.rank]);
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_get_endpoint_addr() failed");	       
   
   ret = mx_decompose_endpoint_addr(MPID_nem_module_mx_endpoints_addr[MPID_nem_mem_region.rank],
				    &local_nic_id, &local_endpoint_id);
   if (ret != MX_SUCCESS)
     ERROR_RET (-1, "mx_decompose_endpoint_addr() failed");
   
   //fprintf(stdout,"[%i] LOCAL ADDR is local_nic_id : %Lu, local_endpt_id : %u\n",
   //	   MPID_nem_mem_region.rank,local_nic_id,local_endpoint_id);
   
   ret = PMI_Barrier();
   if (ret != 0)
     ERROR_RET (-1, "PMI_Barrier failed %d", ret);	     
   
   ret = MPIDI_PG_GetConnKVSname (&kvs_name);
   if (ret != MPI_SUCCESS)
     FATAL_ERROR ("MPIDI_PG_GetConnKVSname failed");
   
   for(index = 0 ; index < numprocs ; index++)
     {
	grank = MPID_nem_mem_region.ext_ranks[index];
	/* fix me : this is non portable and the same gm stuff should be used instead */
	snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "{%u:%Lu}", local_endpoint_id, local_nic_id);
	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "MXkey[%d:%d]", MPID_nem_mem_region.rank, grank);
	
	ret = PMI_KVS_Put (kvs_name, key, val);
	if (ret != 0)
	  ERROR_RET (-1, "PMI_KVS_Put failed %d", ret);
	
	ret = PMI_KVS_Commit (kvs_name);
	if (ret != 0)
	  ERROR_RET (-1, "PMI_KVS_commit failed %d", ret);	
     }		  
   
   ret = PMI_Barrier();
   if (ret != 0)
     ERROR_RET (-1, "PMI_Barrier failed %d", ret);	     
   
   for(index = 0 ; index < numprocs ; index++)
     {
	grank = MPID_nem_mem_region.ext_ranks[index];
	
	memset(val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN,"MXkey[%d:%d]", grank, MPID_nem_mem_region.rank);
	
	ret = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	if (ret != 0)
	  ERROR_RET (-1, "[%i] PMI_KVS_Get failed %d for rank %d", MPID_nem_mem_region.rank, ret, grank);
	
	ret = sscanf (val, "{%u:%Lu}", &remote_endpoint_id, &remote_nic_id);
	if ( ret != 2)
	  {
	     ERROR_RET (-1, "unable to parse data from PMI_KVS_Get %s", val);
	  }

	ret = mx_connect(MPID_nem_module_mx_local_endpoint,
			 remote_nic_id,
			 remote_endpoint_id,
			 MPID_nem_module_mx_filter,
			 MPID_nem_module_mx_timeout,
			 &MPID_nem_module_mx_endpoints_addr[grank]);
	
	if(ret != MX_SUCCESS)
	  ERROR_RET (-1, "mx_connect() failed");
	
	//fprintf(stdout,"[%i] SLAVE : ADDR is  remote_endpt_id : %u, remote_nic_id : %Lu\n",
	//	MPID_nem_mem_region.rank,remote_endpoint_id, remote_nic_id); 	
     }
   
   ret = PMI_Barrier();
   if (ret != 0)
     ERROR_RET (-1, "PMI_Barrier failed %d", ret);	     
   
   return 0;
}


/*
 int  
   MPID_nem_mx_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
MPID_nem_mx_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_recv_queue,
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{   
   int         index;
   
   if( MPID_nem_mem_region.ext_procs > 0)
     {
	init_mx(pg_p);
     }   
   
   MPID_nem_process_recv_queue = proc_recv_queue;
   MPID_nem_process_free_queue = proc_free_queue;
   
   MPID_nem_module_mx_recv_queue = &_recv_queue;
   MPID_nem_module_mx_free_queue = &_free_queue;
   
   MPID_nem_queue_init (MPID_nem_module_mx_recv_queue);
   MPID_nem_queue_init (MPID_nem_module_mx_free_queue);
   
   for (index = 0; index < num_module_elements; ++index)
     {
	MPID_nem_queue_enqueue (MPID_nem_module_mx_free_queue, &module_elements[index]);
     }
   
   *module_recv_queue = MPID_nem_module_mx_recv_queue;
   *module_free_queue = MPID_nem_module_mx_free_queue;

   MPID_nem_module_mx_send_free_req_queue->head    = NULL;
   MPID_nem_module_mx_send_free_req_queue->tail    = NULL;
   MPID_nem_module_mx_send_pending_req_queue->head = NULL;
   MPID_nem_module_mx_send_pending_req_queue->tail = NULL;
   
   MPID_nem_module_mx_recv_free_req_queue->head    = NULL;
   MPID_nem_module_mx_recv_free_req_queue->tail    = NULL;
   MPID_nem_module_mx_recv_pending_req_queue->head = NULL;
   MPID_nem_module_mx_recv_pending_req_queue->tail = NULL;
   
   for (index = 0; index < MPID_NEM_MX_REQ ; ++index)
     {
	MPID_nem_mx_req_queue_enqueue (MPID_nem_module_mx_send_free_req_queue, 
				       &MPID_nem_module_mx_send_outstanding_request[index]);
	MPID_nem_mx_req_queue_enqueue (MPID_nem_module_mx_recv_free_req_queue, 
				       &MPID_nem_module_mx_recv_outstanding_request[index]);
     }

   return 0;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}

int
MPID_nem_mx_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   int mpi_errno = MPI_SUCCESS;
   return mpi_errno;
}

int
MPID_nem_mx_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}
