
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <elan.h>
#include <capability.h>
#include <elanctrl.h>
#include "mpidimpl.h"
#include "mpid_nem_impl.h"
#include "elan_module_impl.h"
#include "elan_module.h"

#define ELAN_CONTEXT_ID_OFFSET  2
#define ELAN_ALLOC_SIZE         16 
#define MPIDI_CH3I_NODE_ID_KEY  "node_id"

/* elan_base is a reserved name in quadrics */
static ELAN_BASE my_elan_base;
static int      *node_ids;  
static int       my_node_id;
static int       min_node_id;
static int       max_node_id;
static int       my_ctxt_id;

int MPID_nem_module_elan_pendings_sends = 0;
int MPID_nem_module_elan_pendings_recvs = 0 ;

static MPID_nem_queue_t _recv_queue;
static MPID_nem_queue_t _free_queue;

MPID_nem_queue_ptr_t MPID_nem_module_elan_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_module_elan_free_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_recv_queue = 0;
MPID_nem_queue_ptr_t MPID_nem_process_free_queue = 0;

int my_compar(const int *a, const int *b)
{
   if ( *a <= *b ) 
     return -1;
   else
     return 1;
}


#undef FUNCNAME
#define FUNCNAME init_elan
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int init_elan( MPIDI_PG_t *pg_p )
{
   char            capability_str[ELAN_ALLOC_SIZE];
   int             mpi_errno = MPI_SUCCESS;
   char            file_name[256];
   char            line[255]; 
   int             numprocs  = MPID_nem_mem_region.ext_procs;
   char            key[MPID_NEM_MAX_KEY_VAL_LEN];
   char            val[MPID_NEM_MAX_KEY_VAL_LEN];
   char           *kvs_name;
   FILE           *myfile;
   ELAN_BASE      *base = NULL;
   int             grank;
   int             index; 
   int             pmi_errno;
   int             ret;
   
   myfile = fopen("/proc/qsnet/elan3/device0/position","r");
   if (myfile == NULL) 
     {
	myfile = fopen("/proc/qsnet/elan4/device0/position","r");
     }
   
   if (myfile != NULL)
     {	
	ret = fscanf(myfile,"%s%i",&line,&my_node_id);
     }
   else
     {
	/* Error */
     }

   mpi_errno = MPIDI_PG_GetConnKVSname (&kvs_name);      
   node_ids = (int *)MPIU_Malloc(numprocs * sizeof(int));
   for (index = 0 ; index < numprocs ; index++)
     {	
	grank = MPID_nem_mem_region.ext_ranks[index];
	MPIU_Snprintf (val, MPID_NEM_MAX_KEY_VAL_LEN, "%i",my_node_id);
	MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN, "QsNetkey[%d:%d]", MPID_nem_mem_region.rank, grank);
	
	pmi_errno = PMI_KVS_Put (kvs_name, key, val);
	MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put", "**pmi_kvs_put %d", pmi_errno);
	
	pmi_errno = PMI_KVS_Commit (kvs_name);
	MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit", "**pmi_kvs_commit %d", pmi_errno);
     }   
   pmi_errno = PMI_Barrier();
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);
   
   for (index = 0 ; index < numprocs ; index++)
     {
	grank = MPID_nem_mem_region.ext_ranks[index];
	memset(val, 0, MPID_NEM_MAX_KEY_VAL_LEN);
	MPIU_Snprintf (key, MPID_NEM_MAX_KEY_VAL_LEN,"QsNetkey[%d:%d]", grank, MPID_nem_mem_region.rank);
	
	pmi_errno = PMI_KVS_Get (kvs_name, key, val, MPID_NEM_MAX_KEY_VAL_LEN);
	MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get", "**pmi_kvs_get %d", pmi_errno);
	
	ret = sscanf (val, "%i", &(node_ids[index]));
	MPIU_ERR_CHKANDJUMP1 (ret != 1, mpi_errno, MPI_ERR_OTHER, "**business_card", "**business_card %s", val);	
     }
   pmi_errno = PMI_Barrier();
   MPIU_ERR_CHKANDJUMP1 (pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier", "**pmi_barrier %d", pmi_errno);

   qsort(node_ids, numprocs, sizeof(int), my_compar);
   
   if (node_ids[0] < my_node_id)
     min_node_id = node_ids[0] ;
   else
     min_node_id = my_node_id ;
   
   if (node_ids[numprocs - 1] > my_node_id)
     max_node_id = node_ids[numprocs - 1] ;
   else
     max_node_id = my_node_id;
   
   MPIU_Snprintf(capability_str, ELAN_ALLOC_SIZE, "N%dC%d-%d-%dN%d-%dR1b",
		 my_node_id,
		 ELAN_CONTEXT_ID_OFFSET,
		 ELAN_CONTEXT_ID_OFFSET+MPID_nem_mem_region.local_rank,
		 ELAN_CONTEXT_ID_OFFSET+(MPID_nem_mem_region.num_local - 1),
		 min_node_id,
		 max_node_id);   
   
   elan_generateCapability (capability_str);            
   
   base = elan_baseInit(0);
   my_elan_base = *base;
   
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

   mpi_errno = MPIU_Str_add_int_arg (bc_val_p, val_max_sz_p, MPIDI_CH3I_NODE_ID_KEY, my_node_id);
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
MPID_nem_elan_module_get_from_bc (const char *business_card,int *node_id)
{
   int mpi_errno = MPI_SUCCESS;
   int tmp_node_id;
   
   mpi_errno = MPIU_Str_get_int_arg (business_card, MPIDI_CH3I_NODE_ID_KEY, &tmp_node_id);
   if (mpi_errno != MPIU_STR_SUCCESS) 
     {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**argstr_hostd");
     }
      
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
   
  // mpi_errno = MPID_nem_mx_module_get_from_bc (business_card, &(vc->ch.node_ids[vc->pg_rank]));
   /* --BEGIN ERROR HANDLING-- */   
   /*
   if (mpi_errno) 
     {	
	MPIU_ERR_POP (mpi_errno);
     }
    */
    /* --END ERROR HANDLING-- */
   
   fn_exit:   
       return mpi_errno;
   fn_fail:
       goto fn_exit;
}
