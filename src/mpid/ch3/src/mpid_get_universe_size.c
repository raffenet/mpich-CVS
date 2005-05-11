/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"


/*
 * MPID_Get_universe_size()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Get_universe_size
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get_universe_size(int  * universe_size)
{
    int mpi_errno = MPI_SUCCESS;

#   if defined(MPIDI_CH3_IMPLEMENTS_GET_UNIVERSE_SIZE)
    {
	mpi_errno = MPIDI_CH3_Get_universe_size(universe_size);
        if (mpi_errno) goto fn_fail;
    }
#   else
    {
	*universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    }
#   endif
    
  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
#if defined(MPIDI_CH3_IMPLEMENTS_GET_UNIVERSE_SIZE)
  fn_fail:
#endif
    *universe_size = MPIR_UNIVERSE_SIZE_NOT_AVAILABLE;
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
