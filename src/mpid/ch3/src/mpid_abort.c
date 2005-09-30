/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifdef MPIDI_DEV_IMPLEMENTS_ABORT
#include "pmi.h"
static int MPIDI_CH3I_PMI_Abort(int exit_code, char *error_msg);
#endif

#ifdef HAVE_WINDOWS_H
/* exit can hang if libc fflushes output while in/out/err buffers are locked
   (this must be a bug in exit?).  ExitProcess does not hang (what does this
   mean about the state of the locked buffers?). */
#define exit(_e) ExitProcess(_e)
#endif

#undef FUNCNAME
#define FUNCNAME MPID_Abort
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Abort(MPID_Comm * comm, int mpi_errno, int exit_code, char *error_msg)
{
    int rank;
    char msg[MPI_MAX_ERROR_STRING] = "";
    char error_str[MPI_MAX_ERROR_STRING + 100];
    MPIDI_STATE_DECL(MPID_STATE_MPID_ABORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_ABORT);
    MPIDI_DBG_PRINTF((10, FCNAME, "entering"));

    if (comm)
    {
	rank = comm->rank;
    }
    else
    {
	if (MPIR_Process.comm_world != NULL)
	{
	    rank = MPIR_Process.comm_world->rank;
	}
	else
	{
	    rank = -1;
	}
    }

    if (error_msg != NULL)
    {
#       ifdef MPIDI_CH3_IMPLEMENTS_ABORT
	{
	    MPIDI_CH3_Abort(exit_code, error_msg);
	}
#       elif defined(MPIDI_DEV_IMPLEMENTS_ABORT)
	{
	    MPIDI_CH3I_PMI_Abort(exit_code, error_msg);
	}
#       else
	{
	    MPIU_Error_printf("%s", error_msg);
	    fflush(stderr);

	    exit(exit_code);
	}
#       endif
    }
    else
    {
	if (mpi_errno != MPI_SUCCESS)
	{
	    /*MPIR_Err_print_mpi_string(mpi_errno, msg, MPI_MAX_ERROR_STRING);*/
	    MPIR_Err_get_string(mpi_errno, msg, MPI_MAX_ERROR_STRING, NULL);
	    MPIU_Snprintf(error_str, MPI_MAX_ERROR_STRING + 100, "internal ABORT - process %d: %s", rank, msg);
	}
	else
	{
	    MPIU_Snprintf(error_str, MPI_MAX_ERROR_STRING + 100, "internal ABORT - process %d", rank);
	}

#       ifdef MPIDI_CH3_IMPLEMENTS_ABORT
	{
	    MPIDI_CH3_Abort(exit_code, error_str);
	}
#       elif defined(MPIDI_DEV_IMPLEMENTS_ABORT)
	{
	    MPIDI_CH3I_PMI_Abort(exit_code, error_str);
	}
#       else
	{
	    MPIU_Error_printf("%s", error_str);
	    fflush(stderr);
	    exit(exit_code);
	}
#       endif
    }

    /* ch3_abort should not return but if it does, exit here */
    exit(exit_code);
    
    MPIDI_DBG_PRINTF((10, FCNAME, "exiting"));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_ABORT);
    return MPI_ERR_INTERN;
}


#ifdef MPIDI_DEV_IMPLEMENTS_ABORT
static int MPIDI_CH3I_PMI_Abort(int exit_code, char *error_msg)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ABORT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ABORT);

    PMI_Abort(exit_code, error_msg);

    /* if abort returns for some reason, exit here */

    MPIU_Error_printf("%s", error_msg);
    fflush(stderr);
    exit(exit_code);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ABORT);
    return MPI_ERR_INTERN;    
}
#endif
