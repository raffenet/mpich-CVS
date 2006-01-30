#include "mpid_nem.h"
#include "mpid_nem_nets.h"
#include "pm.h"

int MPID_nem_finalize()
{
    int rank     = MPID_nem_mem_region.rank;
    int ret;
    /* this test is not the right one */
    while (! MPID_nem_queue_empty( MPID_nem_mem_region.RecvQ[rank] ))
    {
	MPID_nem_dump_queue( MPID_nem_mem_region.RecvQ[rank] );
	exit(0);
	SKIP;
    }
    net_module_finalize();
    munmap(MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.max_size);    

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
  net_module_ckpt_shutdown();
  munmap (MPID_nem_mem_region.memory.base_addr, MPID_nem_mem_region.memory.max_size);    
  return 0;
}
