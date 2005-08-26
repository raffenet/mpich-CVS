/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*@
   MPID_Open_port - short description

   Input Arguments:
+  MPI_Info info - info
-  char *port_name - port name

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Open_port(MPID_Info *info_ptr, char *port_name)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);

    MPIU_UNREFERENCED_ARG(info_ptr);

#   if defined(MPIDI_CH3_IMPLEMENTS_OPEN_PORT)
    {
	mpi_errno = MPIDI_CH3_Open_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   elif defined(MPIDI_DEV_IMPLEMENTS_OPEN_PORT)
    {
	mpi_errno = MPIDI_Open_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }
#   else
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					 "**notimpl %s", FCNAME);
	/* --END ERROR HANDLING-- */
    }
#   endif

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;
}
