/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Allreduce */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allreduce = PMPI_Allreduce
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allreduce  MPI_Allreduce
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allreduce as PMPI_Allreduce
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Allreduce PMPI_Allreduce

/* Any internal routines can go here.  Make them static if possible */
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Allreduce

/*@
MPI_Allreduce - Combines values from all processes and distribute the result
                back to all processes

Input Arguments:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Argument:
. recvbuf - starting address of receive buffer (choice) 

.N fortran

.N collops

.N Errors
.N MPI_ERR_BUFFER
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_OP
.N MPI_ERR_COMM
@*/
int MPI_Allreduce ( void *sendbuf, void *recvbuf, int count, 
		    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm )
{
    static const char FCNAME[] = "MPI_Allreduce";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = 0;
    MPID_MPI_STATE_DECLS;

    /* This is a temporary version to support the testing library */

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ALLREDUCE);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (count < 0) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
                            "**argneg", "**argneg %s %d", "count", count );
            } 
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* This is a *very* temporary implementation that allows the
       single-process test codes to use the common test runtime 
       routines */
    /* ... body of routine ...  */
    if (comm_ptr->remote_size > 1) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, 
					      "**notimpl", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
    else {
	/* count in bytes */
	/* This also assumes that the datatypes are basic */
	int dtype_size;

	if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, 
					      "**notimpl", 0 );
	    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
	    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
	}
	dtype_size = MPID_Datatype_get_size(datatype);
	
	memcpy( recvbuf, sendbuf, count*dtype_size );
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLREDUCE);
    return MPI_SUCCESS;
}
