
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <elan.h>
#include <capability.h>
#include <elanctrl.h>
#include "mpidimpl.h"
#include "elan_module_impl.h"
#include "mpid_nem.h"
#include "elan_module.h"

#define ELAN_CONTEXT_ID_OFFSET  2
#define ELAN_ALLOC_SIZE         16 

/* elan_base is a reserved name in quadrics */
static ELAN_BASE my_elan_base;

int MPID_nem_module_elan_pendings_sends = 0;
int MPID_nem_module_elan_pendings_recvs = 0 ;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_elan_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_module_elan_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

#undef FUNCNAME
#define FUNCNAME init_elan
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int init_elan( MPIDI_PG_t *pg_p )
{
   char            capability_str[ELAN_ALLOC_SIZE];
   int             mpi_errno = MPI_SUCCESS;
   char            name[256];
   int             node_id;
   int             min_node_id;
   int             max_node_id;
   ELAN_BASE      *base      = NULL;

   /* this has to be fixed ASAP */
   gethostname(name,256);   
   if ( strncmp( name, "joe" ,strlen("joe")) == 0)
     node_id = 0;
   else if ( strncmp( name, "jack" ,strlen("jack")) == 0)
     node_id = 1 ;
   min_node_id = 0 ;   
   max_node_id = 1 ;
   
   snprintf(capability_str, ELAN_ALLOC_SIZE, "N%dC%d-%d-%dN%d-%dR1b",
	    node_id,
	    ELAN_CONTEXT_ID_OFFSET,
	    ELAN_CONTEXT_ID_OFFSET+MPID_nem_mem_region.local_rank,
	    ELAN_CONTEXT_ID_OFFSET+(MPID_nem_mem_region.num_local - 1),
	    min_node_id,
	    max_node_id);   
   /*
    size = snprintf(str, alloc_size, "N%dC%d-%d-%dN%d-%dR1b",
    node_id,
    ELAN_CONTEXT_ID_OFFSET,
    ctx_id,
    ctx_id_max,
    node_id_min,
    node_id_max);
    */   
   elan_generateCapability (capability_str);            
   //fprintf(stdout,"[%s] generate Cap done !\n",name);
   
   base = elan_baseInit(0);
   my_elan_base = *base;
   
   //fprintf(stdout,"[%s] Init done !\n",name);
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

/*
 int  
   MPID_nem_elan_module_init(MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements, int num_proc_elements,
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
#define FUNCNAME MPID_nem_mx_module_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		MPID_nem_queue_ptr_t proc_free_queue, 
		MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		MPID_nem_queue_ptr_t *module_recv_queue,
		MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		MPIDI_PG_t *pg_p, int pg_rank,
		char **bc_val_p, int *val_max_sz_p)
{   
   int mpi_errno = MPI_SUCCESS ;
   int index;
   
   if( MPID_nem_mem_region.ext_procs > 0)
     {
	init_elan(pg_p);
     }   

   mpi_errno = MPID_nem_elan_module_get_business_card (bc_val_p, val_max_sz_p);
   if (mpi_errno) MPIU_ERR_POP (mpi_errno);

   MPID_nem_process_recv_queue = proc_recv_queue;
   MPID_nem_process_free_queue = proc_free_queue;
   
   MPID_nem_module_elan_recv_queue = &_recv_queue;
   MPID_nem_module_elan_free_queue = &_free_queue;
   
   MPID_nem_queue_init (MPID_nem_module_elan_recv_queue);
   MPID_nem_queue_init (MPID_nem_module_elan_free_queue);
   
   for (index = 0; index < num_module_elements; ++index)
     {
	MPID_nem_queue_enqueue (MPID_nem_module_elan_free_queue, &module_elements[index]);
     }
   
   *module_recv_queue = MPID_nem_module_elan_recv_queue;
   *module_free_queue = MPID_nem_module_elan_free_queue;

   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
  MPID_nem_elan_module_get_business_card (char **bc_val_p, int *val_max_sz_p)
{
   int mpi_errno = MPI_SUCCESS;

   /*
   mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_ENDPOINT_KEY, local_endpoint_id);
   if (mpi_errno != MPIU_STR_SUCCESS)
     {
	if (mpi_errno == MPIU_STR_NOMEM) 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }
	else 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }
	goto fn_exit;
     }

   mpi_errno = MPIU_Str_add_binary_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_NIC_KEY, (char *)&local_nic_id, sizeof(uint64_t));
   if (mpi_errno != MPIU_STR_SUCCESS)
     {
	if (mpi_errno == MPIU_STR_NOMEM) 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard_len");
	  }
	else 
	  {
	     MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**buscard");
	  }
	goto fn_exit;
     }
   */
   
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_get_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_get_from_bc (const char *business_card,int *remote_endpoint_id,int *remote_nic_id)
{
   int mpi_errno = MPI_SUCCESS;
   int len;
   
   /*
   mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_ENDPOINT_KEY, &tmp_endpoint_id);
   if (mpi_errno != MPIU_STR_SUCCESS) 
     {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   *remote_endpoint_id = (uint32_t)tmp_endpoint_id;
   
   mpi_errno = MPIU_Str_get_binary_arg (business_card, MPIDI_CH3I_NIC_KEY, (char *)remote_nic_id, sizeof(uint64_t), &len);
   if ((mpi_errno != MPIU_STR_SUCCESS) || len != sizeof(int)) 
     {	
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
   */
   
   fn_exit:
     return mpi_errno;
   fn_fail:  
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
   int mpi_errno = MPI_SUCCESS;
   int ret;
   
   //mpi_errno = MPID_nem_mx_module_get_from_bc (business_card, &vc->ch.remote_endpoint_id, &vc->ch.remote_nic_id);
   /* --BEGIN ERROR HANDLING-- */   
   /*
   if (mpi_errno) 
     {	
	MPIU_ERR_POP (mpi_errno);
     }
    */ 
   /* --END ERROR HANDLING-- */
   
   /*
   ret = mx_connect(MPID_nem_module_mx_local_endpoint,
		    vc->ch.remote_nic_id,
		    vc->ch.remote_endpoint_id,
		    MPID_nem_module_mx_filter,
		    MPID_nem_module_mx_timeout,
		    &MPID_nem_module_mx_endpoints_addr[vc->pg_rank]);
   MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_connect", "**mx_connect %s", mx_strerror (ret));
*/
//   fprintf(stdout,"[%i] ELAN connect ================ with %i \n",MPID_nem_mem_region.rank,vc->pg_rank);
   fn_exit:
   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}
