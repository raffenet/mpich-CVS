/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"
#include <stdio.h>

/*
 * MPID_Comm_spawn()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, int root,
		    MPID_Comm *comm, MPID_Comm *intercomm, int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;
    char spawned_kvsname[128];
    int kvsnamelen = 128;
    MPIDI_CH3I_Process_group_t * pg;
    MPIDI_VC * vc_table;
    int p, rc;

    MPIDI_STATE_DECL(MPID_STATE_MPID_SPAWN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_SPAWN);

    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

/*  begin experimental Ralph and Rusty version */

    /* printf( "before call to PMI_Spawn, maxprocs=%d, intercomm=0x%x, comm=0x%x\n",
       maxprocs, intercomm, comm ); fflush(stdout); */
    if (comm->rank == root) {
	rc = PMI_Spawn(command, argv, maxprocs, spawned_kvsname, kvsnamelen );
	assert(rc == 0);
    }
    else {
	/* get some information as needed from root */
    }
    /* printf( "after call to PMI_Spawn, maxprocs=%d, intercomm=0x%x, comm=0x%x\n",
       maxprocs, intercomm, comm ); fflush(stdout); */

    /* Fill in new intercomm */
    intercomm->rank	   = comm->rank;
    intercomm->local_size  = comm->local_size;
    intercomm->remote_size = maxprocs;
    /* Point local vcr, vcrt at those of incoming intracommunicator */
    intercomm->local_vcrt = comm->vcrt;
    MPID_VCRT_Add_ref(comm->vcrt);
    intercomm->local_vcr  = comm->vcr;

    /* Allocate process group data structure for remote group and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    assert(pg != NULL);
    pg->size = maxprocs;
    pg->kvs_name = MPIU_Malloc(PMI_KVS_Get_name_length_max() + 1);
    assert(pg->kvs_name != NULL);
    MPIU_Strncpy(pg->kvs_name, spawned_kvsname, PMI_KVS_Get_name_length_max());
    pg->ref_count = 0;

    /* Allocate and initialize the VC table associated with the remote group */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg->size);
    assert(vc_table != NULL);
    pg->ref_count += pg->size;
    for (p = 0; p < pg->size; p++)
    {
	vc_table[p].ref_count = 0;
	vc_table[p].lpid = p;
	vc_table[p].tcp.pg = pg;
	vc_table[p].tcp.pg_rank = p;
	vc_table[p].tcp.sendq_head = NULL;
	vc_table[p].tcp.sendq_tail = NULL;
	vc_table[p].tcp.poll_elem = -1;
	vc_table[p].tcp.fd = -1;
	vc_table[p].tcp.state = MPIDI_CH3I_VC_STATE_UNCONNECTED;
    }
    pg->vc_table = vc_table;

    /* Set up VC reference table */
    rc = MPID_VCRT_Create(intercomm->remote_size, &intercomm->vcrt);
    assert(rc == MPI_SUCCESS);
    rc = MPID_VCRT_Get_ptr(intercomm->vcrt, &intercomm->vcr);
    assert(rc == MPI_SUCCESS);
    for (p = 0; p < pg->size; p++) {
	MPID_VCR_Dup(&vc_table[p], &intercomm->vcr[p]);
    }

/*  end experimental Ralph and Rusty version */

    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_SPAWN);
    return mpi_errno;
}
