/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef USE_MPIU_DBG_PRINT_VC

/* VC state printing debugging functions */

static const char * VCState2(MPIDI_VC_State_t state)
{
    static char unknown_state[32];
    switch (state)
    {
    case MPIDI_VC_STATE_INACTIVE:
	return "INACTIVE";
    case MPIDI_VC_STATE_ACTIVE:
	return "ACTIVE";
    case MPIDI_VC_STATE_LOCAL_CLOSE:
	return "LOCAL_CLOSE";
    case MPIDI_VC_STATE_REMOTE_CLOSE:
	return "REMOTE_CLOSE";
    case MPIDI_VC_STATE_CLOSE_ACKED:
	return "CLOSE_ACKED";
    }
    MPIU_Snprintf(unknown_state, 32, "STATE_%d", state);
    return unknown_state;
}

static const char * VCState(MPIDI_VC_t *vc)
{
    return VCState2(vc->state);
}

void MPIU_DBG_PrintVC(MPIDI_VC_t *vc)
{
    printf("vc.state : %s\n", VCState(vc));
    printf("vc.pg_rank : %d\n", vc->pg_rank);
    printf("vc.ref_count: %d\n", vc->ref_count);
    printf("vc.lpid : %d\n", vc->lpid);
    if (vc->pg == NULL)
    {
	printf("vc.pg : NULL\n");
    }
    else
    {
	printf("vc.pg->id : %s\n", vc->pg->id);
	printf("vc.pg->size : %d\n", vc->pg->size);
	printf("vc.pg->ref_count : %d\n", vc->pg->ref_count);
    }
    fflush(stdout);
}

void MPIU_DBG_PrintVCState2(MPIDI_VC_t *vc, MPIDI_VC_State_t new_state)
{
    printf("[%s%d]vc%d.state = %s->%s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), VCState2(new_state), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");
    fflush(stdout);
}

void MPIU_DBG_PrintVCState(MPIDI_VC_t *vc)
{
    printf("[%s%d]vc%d.state = %s (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, vc->pg_rank, VCState(vc), (vc->pg && vc->pg_rank >= 0) ? vc->pg->id : "?");
    fflush(stdout);
}

#endif /* USE_MPIU_DBG_PRINT_VC */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pg_size;
    int p;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    /* initialize aspects specific to sockets.  do NOT publish business card yet  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_parent, pg_p, pg_rank,
                               NULL, bc_key_p, bc_val_p, val_max_sz_p);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* initialize aspects specific to sshm.  now publish business card   */
    mpi_errno = MPIDI_CH3U_Init_sshm(has_parent, pg_p, pg_rank,
                               publish_bc_p, bc_key_p, bc_val_p, val_max_sz_p);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = PMI_Get_size(&pg_size);
    if (mpi_errno != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_size", "**pmi_get_size %d", mpi_errno);
	return mpi_errno;
    }

    /* Allocate and initialize the VC table associated with this process group (and thus COMM_WORLD) */
    /* FIXME: This doesn't allocate and only inits one field.  Is this
       now part of the channel-specific hook for channel-specific VC info? */
    for (p = 0; p < pg_size; p++)
    {
	pg_p->vct[p].ch.bShm = FALSE;
    }

fn_exit:

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;

fn_fail:
    /* FIXME: Does this routine still allocated pg_p? */
    if (pg_p != NULL)
    {
	MPIDI_PG_Destroy(pg_p);
    }
    goto fn_exit;
}
