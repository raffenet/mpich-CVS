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
    char *value = NULL;
    int value_len;
    char *key = NULL;
    int key_len;
    char sCapabilities[256] = "";

    bsocket_init();

    /* Initialize per process structure */
    memset(&MPID_Process, 0, sizeof(MPID_PerProcess));
    MPID_Thread_lock_init(MPID_Process.qlock);
    MPID_Thread_lock_init(MPID_Process.lock);

    MPIR_Process.attrs.tag_ub = 2*1024*1024;

    /*dbg_printf("+PMI_Init");*/
    PMI_Init(&spawned);
    /*dbg_printf("-\n+PMI_Get_rank:");*/
    PMI_Get_rank(&MPIR_Process.comm_world->rank);
    /*dbg_printf("%d-\n+PMI_Get_size:", MPIR_Process.comm_world->rank);*/
    PMI_Get_size(&MPIR_Process.comm_world->local_size);
    /*dbg_printf("%d-\n", MPIR_Process.comm_world->local_size);*/
    MPIR_Process.comm_world->remote_size = MPIR_Process.comm_world->local_size;
    /*dbg_printf("+PMI_KVS_Get_my_name:");*/
    PMI_KVS_Get_my_name(MPID_Process.pmi_kvsname);
    /*dbg_printf("%s-\n", MPID_Process.pmi_kvsname);*/
    MPIR_Process.comm_world->mm.pmi_kvsname = MPID_Process.pmi_kvsname;
    /*dbg_printf("+PMI_Barrier");*/
    PMI_Barrier();
    /*dbg_printf("-\n");*/

    MPID_Timer_init(MPIR_Process.comm_world->rank, MPIR_Process.comm_world->local_size);

    if (spawned)
    {
	/*dbg_printf("+PMI_KVS_Get");*/
	PMI_KVS_Get(MPID_Process.pmi_kvsname, MPICH_PARENT_PORT_KEY, pszPortName);
	/*dbg_printf("-\n");*/
	/*PMPI_Comm_connect(pszPortName, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &MPID_Process.comm_parent); */
    }
    else
    {
	MPID_Process.comm_parent = (MPID_Comm *)0;
    }

    /* single threaded for now? */
    if (provided != NULL)
    {
	*provided = MPI_THREAD_SINGLE;
    }

    mm_car_init();
    mm_vc_init();

    /* initialize the methods */
    packer_init();
    unpacker_init();
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

    /*dbg_printf("+PMI_KVS_Get_value_length_max");*/
    value_len = PMI_KVS_Get_value_length_max();
    /*dbg_printf("-\n");*/
    if (value_len < 1)
    {
	err_printf("PMI_KVS_Get_value_length_max returned %d\n", value_len);
	return -1;
    }
    value = (char*)MPIU_Malloc(value_len);
    if (value == NULL)
    {
	err_printf("MPIU_Malloc(%d) failed\n", value_len);
	return -1;
    }
    /*dbg_printf("+PMI_KVS_Get_key_length_max");*/
    key_len = PMI_KVS_Get_key_length_max();
    /*dbg_printf("-\n");*/
    if (key_len < 1)
    {
	err_printf("PMI_KVS_Get_key_length_max returned %d\n", key_len);
	return -1;
    }
    key = MPIU_Malloc(key_len);
    if (key == NULL)
    {
	err_printf("MPIU_Malloc(%d) failed\n", key_len);
	return -1;
    }

#ifdef WITH_METHOD_SHM
    strncat(sCapabilities, "shm,", 5);
    shm_get_business_card(value, value_len);
    snprintf(key, key_len, "business_card_shm:%d", MPIR_Process.comm_world->rank);
    /*dbg_printf("+PMI_KVS_Put([%s],[%s])", key, value);*/
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
    /*dbg_printf("-\n");*/
#endif
#ifdef WITH_METHOD_TCP
    strncat(sCapabilities, "tcp,", 5);
    tcp_get_business_card(value, value_len);
    snprintf(key, key_len, "business_card_tcp:%d", MPIR_Process.comm_world->rank);
    /*dbg_printf("+PMI_KVS_Put([%s],[%s])", key, value);*/
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
    /*dbg_printf("-\n");*/
#endif
#ifdef WITH_METHOD_VIA
    strncat(sCapabilities, "via,", 5);
    via_get_business_card(value, value_len);
    snprintf(key, key_len, "business_card_via:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_VIA_RDMA
    strncat(sCapabilities, "via_rdma,", 9);
    via_rdma_get_business_card(value, value_len);
    snprintf(key, key_len, "business_card_via_rdma:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif
#ifdef WITH_METHOD_NEW
    strncat(sCapabilities, "new,", 5);
    new_get_business_card(value, value_len);
    snprintf(key, key_len, "business_card_new:%d", MPIR_Process.comm_world->rank);
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, value);
#endif

    if (sCapabilities[strlen(sCapabilities)-1] == ',')
	sCapabilities[strlen(sCapabilities)-1] = '\0';

    snprintf(key, key_len, "businesscard:%d", MPIR_Process.comm_world->rank);
    /*dbg_printf("+PMI_KVS_Put([%s],[%s])", key, sCapabilities);*/
    PMI_KVS_Put(MPID_Process.pmi_kvsname, key, sCapabilities);
    /*dbg_printf("-\n+PMI_KVS_Commit");*/
    PMI_KVS_Commit(MPID_Process.pmi_kvsname);
    /*dbg_printf("-\n+PMI_Barrier");*/
    PMI_Barrier();
    /*dbg_printf("-\n");*/

    MPIU_Free(value);
    MPIU_Free(key);

    return MPI_SUCCESS;
}
