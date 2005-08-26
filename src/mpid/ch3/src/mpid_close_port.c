/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Close_port - close port

   Arguments:
.  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Close_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Close_port(char *port_name)
{
    int mpi_errno=MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);

#   if defined(MPIDI_CH3_IMPLEMENTS_CLOSE_PORT)
    {
	mpi_errno = MPIDI_CH3_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   elif defined(MPIDI_DEV_IMPLEMENTS_CLOSE_PORT)
    {
	mpi_errno = MPIDI_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   else
    {
	MPIU_UNREFERENCED_ARG(port_name);
	/* --BEGIN ERROR HANDLING-- */
	/* FIXME: For now leave this unimplemented without producing an error */
	/*
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					 "**notimpl %s", FCNAME);
	*/
	/* --END ERROR HANDLING-- */
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;
}
