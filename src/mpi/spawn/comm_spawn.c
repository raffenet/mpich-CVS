/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_spawn */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_spawn = PMPI_Comm_spawn
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_spawn  MPI_Comm_spawn
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_spawn as PMPI_Comm_spawn
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_spawn PMPI_Comm_spawn

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_spawn

/*@
   MPI_Comm_spawn - spawn up to maxprocs instances of a single mpi application

   Input Parameters:
+ command - name of program to be spawned (string, significant only at root) 
. argv - arguments to command (array of strings, significant only at root) 
. maxprocs - maximum number of processes to start (integer, significant only 
  at root) 
. info - a set of key-value pairs telling the runtime system where and how 
   to start the processes (handle, significant only at root) 
. root - rank of process in which previous arguments are examined (integer) 
- comm - intracommunicator containing group of spawning processes (handle) 

   Output Parameters:
+ intercomm - intercommunicator between original group and the 
   newly spawned group (handle) 
- array_of_errcodes - one code per process (array of integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_ARG
.N MPI_ERR_INFO
@*/
int MPI_Comm_spawn(char *command, char *argv[], int maxprocs, MPI_Info info, 
		   int root, MPI_Comm comm, MPI_Comm *intercomm,
		   int array_of_errcodes[])
{
    static const char FCNAME[] = "MPI_Comm_spawn";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SPAWN);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SPAWN);

    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /*
    MPID_Comm_thread_lock( comm_ptr );
    MPID_Comm_thread_unlock( comm_ptr );
    */

/* begin experimental code by Rusty and Ralph */ 

    mpi_errno = MPIR_Comm_create( comm_ptr, &newcomm_ptr );
    if (mpi_errno) {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN );
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }

    /* printf( "calling MPID_Comm_spawn\n" ); */
    mpi_errno = MPID_Comm_spawn(command, argv, maxprocs, info, root,
			 comm_ptr, newcomm_ptr, array_of_errcodes);
    /* printf( "back from MPID_Comm_spawn, errno = %d\n", mpi_errno ); */

    *intercomm = newcomm_ptr->handle;

/* end experimental code by Rusty and Ralph */ 

    if (mpi_errno == MPI_SUCCESS)
    {
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN);
	return MPI_SUCCESS;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SPAWN);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
}
