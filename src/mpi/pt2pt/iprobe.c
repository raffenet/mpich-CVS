/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Iprobe */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Iprobe = PMPI_Iprobe
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Iprobe  MPI_Iprobe
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Iprobe as PMPI_Iprobe
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Iprobe PMPI_Iprobe

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Iprobe

/*@
    MPI_Iprobe - Nonblocking test for a message

Input Parameters:
+ source - source rank, or  'MPI_ANY_SOURCE' (integer) 
. tag - tag value or  'MPI_ANY_TAG' (integer) 
- comm - communicator (handle) 

Output Parameter:
+ flag - (logical) 
- status - status object (Status) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_TAG
.N MPI_ERR_RANK

@*/
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
	       MPI_Status *status)
{
    static const char FCNAME[] = "MPI_Iprobe";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_IPROBE);

    /* Verify that MPI has been initialized */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL( flag, "flag", mpi_errno );
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */
	    
    MPID_MPI_PT2PT_FUNC_ENTER(MPID_STATE_MPI_IPROBE);
    
    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );
    
    /* Validate parameters if error checking is enabled */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    /* Validate communicator */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    MPIR_ERRTEST_RECV_TAG(tag,mpi_errno);
	    if (comm_ptr) {
		MPIR_ERRTEST_RECV_RANK(comm_ptr, source, mpi_errno);
	    }
            if (mpi_errno) {
                MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IPROBE);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* FIXME: Is this correct for intercomms? */
    mpi_errno = MPID_Iprobe(source, tag, comm_ptr, MPID_CONTEXT_INTRA_PT2PT, 
			    flag, status);
    
    MPID_MPI_PT2PT_FUNC_EXIT(MPID_STATE_MPI_IPROBE);
    if (mpi_errno == MPI_SUCCESS)
    {
	return MPI_SUCCESS;
    }
    else
    {
	return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    }
}
