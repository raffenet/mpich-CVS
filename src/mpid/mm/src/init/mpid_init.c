/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpid_init.c
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

MPID_PerProcess MPID_Process;

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
    char *value;
    int value_len;
    char sCapabilities[256] = "";
    char key[100];

    /* Initialize per process structure */
    memset(&MPID_Process, 0, sizeof(MPID_PerProcess));
    MPID_Thread_lock_init(MPID_Process.qlock);
    MPID_Thread_lock_init(MPID_Process.lock);

    PMI_Init(&spawned);
    PMI_Get_rank(&MPIR_Process.comm_world->rank);
    PMI_Get_size(&MPIR_Process.comm_world->local_size);
    MPIR_Process.comm_world->remote_size = MPIR_Process.comm_world->local_size;
    PMI_KVS_Get_my_name(MPID_Process.pmi_kvsname);
    MPIR_Process.comm_world->mm.pmi_kvsname = MPID_Process.pmi_kvsname;
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
    mm_vcutil_init();

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

    value_len = PMI_KVS_Get_value_length_max();
    if (value_len < 1)
	return -1;
    value = (char*)malloc(value_len);

#ifdef WITH_METHOD_SHM
    strncat(sCapabilities, "shm ", 5);
    shm_get_business_card(value);
    snprintf(key, 100, "business_card_shm:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_TCP
    strncat(sCapabilities, "tcp ", 5);
    tcp_get_business_card(value);
    snprintf(key, 100, "business_card_tcp:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_VIA
    strncat(sCapabilities, "via ", 5);
    via_get_business_card(value);
    snprintf(key, 100, "business_card_via:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_VIA_RDMA
    strncat(sCapabilities, "via_rdma ", 9);
    via_rdma_get_business_card(value);
    snprintf(key, 100, "business_card_via_rdma:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_NEW
    strncat(sCapabilities, "new ", 5);
    new_get_business_card(value);
    snprintf(key, 100, "business_card_new:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif

    free(value);

    sprintf(key, "businesscard:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, sCapabilities);

    return MPI_SUCCESS;
}
