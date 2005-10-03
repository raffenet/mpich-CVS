/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/* FIXME: Correct description of function */
/*@
   MPID_Comm_spawn_multiple - 

   Input Arguments:
+  int count - count
.  char *array_of_commands[] - commands
.  char* *array_of_argv[] - arguments
.  int array_of_maxprocs[] - maxprocs
.  MPI_Info array_of_info[] - infos
.  int root - root
-  MPI_Comm comm - communicator

   Output Arguments:
+  MPI_Comm *intercomm - intercommunicator
-  int array_of_errcodes[] - error codes

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Comm_spawn_multiple(int count, char *array_of_commands[], char ** array_of_argv[], int array_of_maxprocs[],
			     MPID_Info * array_of_info_ptrs[], int root, MPID_Comm * comm_ptr, MPID_Comm ** intercomm,
			     int array_of_errcodes[]) 
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);

/* FIXME: This is silly.  There should be one implementation of (most of)
   comm_spawn_multiple */
    /* FIXME: See ch3u_comm_spawn_multiple, where the DEV_IMPLEMENTS 
       branch of this is placed */
#   if defined(MPIDI_CH3_IMPLEMENTS_COMM_SPAWN_MULTIPLE)
    {
	mpi_errno = MPIDI_CH3_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info_ptrs,
						  root, comm_ptr, intercomm, array_of_errcodes);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	    /* --END ERROR HANDLING-- */
	}
    }
#   elif defined(MPIDI_DEV_IMPLEMENTS_COMM_SPAWN_MULTIPLE)
    {
	mpi_errno = MPIDI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info_ptrs,
						  root, comm_ptr, intercomm, array_of_errcodes);
	if (mpi_errno != MPI_SUCCESS)
	{
	    /* --BEGIN ERROR HANDLING-- */
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", NULL);
	    /* --END ERROR HANDLING-- */
	}
    }
#   else
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl",
					 "**notimpl %s", FCNAME);
	/* --END ERROR HANDLING-- */
    }
#   endif
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_COMM_SPAWN_MULTIPLE);
    return mpi_errno;
}
