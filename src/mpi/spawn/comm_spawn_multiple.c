/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_spawn_multiple */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_spawn_multiple = PMPI_Comm_spawn_multiple
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_spawn_multiple  MPI_Comm_spawn_multiple
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_spawn_multiple as PMPI_Comm_spawn_multiple
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_spawn_multiple PMPI_Comm_spawn_multiple

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_spawn_multiple

/*@
   MPI_Comm_spawn_multiple - short description

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

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char* *array_of_argv[], int array_of_maxprocs[], MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]) 
{
    static const char FCNAME[] = "MPI_Comm_spawn_multiple";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_Comm *intercomm_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_Comm_thread_lock( comm_ptr );

    MPID_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm_ptr, &intercomm_ptr, array_of_errcodes);
    //*intercomm = MPID_Comm_ptr_to_MPI_Comm(intercomm_ptr);

    MPID_Comm_thread_unlock( comm_ptr );

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN_MULTIPLE);
    return MPI_SUCCESS;
}
