/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpid_init.c
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

MPID_PerProcess_t MPID_Process;

/*@
   MPID_Init - Initialize the mm device

   Parameters:
+  int *argcp
.  char ***argvp
.  int requested
.  int *provided
.  int *flag_args
-  int *flag_env

   Notes:
@*/
int MPID_Init(int *argcp, char ***argvp, int requested, int *provided, int *flag_args, int *flag_env)
{
    char pszPortName[MPI_MAX_PORT_NAME];
    int spawned;

    MPID_Process.pmi_kvsname[0] = '\0';
    MPID_Process.comm_parent = (MPID_Comm *)0;
    MPID_Process.port_list = (OpenPortNode_t *)0;

    PMI_Init(&spawned);
    PMI_Get_rank(&MPIR_Process.comm_world->rank);
    PMI_Get_size(&MPIR_Process.comm_world->size);
    PMI_KVS_Get_my_name(MPID_Process.pmi_kvsname);
    PMI_Barrier();

    if (spawned)
    {
	PMI_KVS_Get(MPID_Process.pmi_kvsname, MPICH_PARENT_PORT_KEY, pszPortName);
	/*PMPI_Comm_connect(pszPortName, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &MPID_Process.comm_parent); */
    }
    else
    {
	MPID_Process.comm_parent = (MPID_Comm *)0;
    }

    mm_car_init();
    mm_vctable_init();

#ifdef WITH_METHOD_SHM
    shm_init();
#endif
#ifdef WITH_METHOD_TCP
    tcp_init();
#endif
#ifdef WITH_METHOD_VIA
    via_init();
#endif
#ifdef WITH_METHOD_VIA_RDMA
    via_rdma_init();
#endif
#ifdef WITH_METHOD_NEW
    new_method_init();
#endif

    return MPI_SUCCESS;
}
