/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * has_args, int * has_env, int * has_parent, MPIDI_PG_t ** pg_p, int * pg_rank_p,
                    char **publish_bc_p, char **bc_key_p, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pg_size;
    int p;
    MPIDI_STATE_DECL(MPID_STATE_MPID_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CH3_INIT);

    /* initialize aspects specific to sockets.  do NOT publish business card yet  */
    mpi_errno = MPIDI_CH3U_Init_sock(has_args, has_env, has_parent, pg_p, pg_rank_p,
                               NULL, bc_key_p, bc_val_p, val_max_sz_p);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_fail;
    }
    /* --END ERROR HANDLING-- */

    /* initialize aspects specific to sshm.  now publish business card   */
    mpi_errno = MPIDI_CH3U_Init_sshm(has_args, has_env, has_parent, pg_p, pg_rank_p,
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
    for (p = 0; p < pg_size; p++)
    {
	(*pg_p)->vct[p].ch.bShm = FALSE;
    }

fn_exit:

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CH3_INIT);
    return mpi_errno;

fn_fail:
    if ((*pg_p) != NULL)
    {
	MPIDI_PG_Destroy(*pg_p);
    }
    goto fn_exit;
}
