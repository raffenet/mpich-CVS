/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "mpidu_sock.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Finalize()
{


    int mpi_errno = MPI_SUCCESS;
/*     int rc; */

    MPIDI_DBG_PRINTF((50, FCNAME, "entering"));
#if 0 /* brad : code common to every channel so moved into mpid_finalize.c  */
    /* Shutdown the progress engine */
    mpi_errno = MPIDI_CH3I_Progress_finalize();
    if (mpi_errno != MPI_SUCCESS)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
					 "**ch3|sock|progress_finalize", 0);
	/* --END ERROR HANDLING-- */
    }
#endif
    /* brad : this is only in the sock channel but is this the way it is supposed to be? */
    /* brad : may later add feature about bizcards and move this up to mpid_finalize.c */
    MPIDI_CH3I_Bizcard_cache_free();
#if 0 /* brad : common */
    /* Let PMI know the process is about to exit */
    rc = PMI_Finalize();
    if (rc != 0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
					 MPI_ERR_OTHER, "**ch3|sock|pmi_finalize", "**ch3|sock|pmi_finalize %d", rc);
    }

    mpi_errno = MPIDI_PG_Finalize();  /* brad : currently this function does nothing */
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
					 MPI_ERR_OTHER, "**ch3|sock|pg_finalize", 0);
    }
#endif
    MPIDI_DBG_PRINTF((50, FCNAME, "exiting"));
    return mpi_errno;
}
