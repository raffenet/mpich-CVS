#include "mpid_nem.h"
#include "gm_module.h"
#include "tcp_module.h"
#include "pm.h"

/*#define TRACE */
int MPID_nem_finalize()
{
    int rank     = MPID_nem_mem_region.rank;
    int ret;

#ifdef TRACE
    fprintf(stderr,"[%i] Waiting for empty RecvQ \n",rank);    
#endif
    /* this test is not the right one */
    while (! MPID_nem_queue_empty( MPID_nem_mem_region.RecvQ[rank] ))
    {
	MPID_nem_dump_queue( MPID_nem_mem_region.RecvQ[rank] );
	exit(0);
	SKIP;
    }
#ifdef TRACE
    fprintf(stderr,"[%i] empty Recv Q !!! \n",rank);
#endif

    switch (MPID_NEM_NET_MODULE)
    {
    case MPID_NEM_GM_MODULE:
	gm_module_finalize();
	break;
    case MPID_NEM_TCP_MODULE:
        tcp_module_finalize();
	break;
    default:
	break;
    }

    munmap(MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.max_size);    

/*     MPID_nem_barrier(MPID_nem_mem_region.num_local, MPID_nem_mem_region.local_rank); */

/*     if ((MPID_nem_mem_region.num_local > 1) && (MPID_nem_mem_region.local_rank == ROOT_NUM)) */
/*     {	 */
/* 	del_sem(MPID_nem_mem_region.map_lock, MPID_nem_mem_region.num_local); */
/*     } */
    
#ifdef PAPI_MONITOR
    my_papi_close();
#endif /*PAPI_MONITOR */
   
    ret = PMI_Barrier();
    if (ret != 0)
	FATAL_ERROR ("PMI_Barrier failed %d", ret);

    pm_finalize();

    return MPID_NEM_RET_OK;
}

int
MPID_nem_ckpt_shutdown()
{
    switch (MPID_NEM_NET_MODULE)
    {
    case MPID_NEM_GM_MODULE:
	gm_module_ckpt_shutdown();
	break;
    case MPID_NEM_TCP_MODULE:
        tcp_module_ckpt_shutdown();
	break;
    default:
	break;
    }

    munmap (MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.max_size);    

    return 0;
}
