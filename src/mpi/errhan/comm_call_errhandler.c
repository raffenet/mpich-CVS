/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_call_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_call_errhandler = PMPI_Comm_call_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_call_errhandler  MPI_Comm_call_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_call_errhandler as PMPI_Comm_call_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_call_errhandler PMPI_Comm_call_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_call_errhandler

/*@
   MPI_Comm_call_errhandler - Call the error handler installed on a 
   communicator

 Input Parameters:
+ comm - communicator with error handler (handle) 
- errorcode - error code (integer) 

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    static const char FCNAME[] = "MPI_Comm_call_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);

            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
	    if (!mpi_errno) {
		MPIR_ERRTEST_ERRHANDLER(comm_ptr->errhandler,mpi_errno);
	    }
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Check for predefined error handlers */
    if (!comm_ptr->errhandler || 
	comm_ptr->errhandler->handle == MPI_ERRORS_ARE_FATAL) {
	return MPIR_Err_return_comm( comm_ptr, "MPI_Comm_call_errhandler", 
				     errorcode );
    }

    if (comm_ptr->errhandler->handle == MPI_ERRORS_RETURN) {
	return errorcode;
    }

    /* Process any user-defined error handling function */
    switch (comm_ptr->errhandler->language) {
    case MPID_LANG_C:
#ifdef HAVE_CXX_BINDING
    case MPID_LANG_CXX:
#endif
	(*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
	    &comm_ptr->handle, &errorcode );
	break;
#ifdef HAVE_FORTRAN_BINDING
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*comm_ptr->errhandler->errfn.F77_Handler_function)( 
	    (MPI_Fint *)&comm_ptr->handle, &errorcode );
	break;
#endif
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_comm_call_errhandler", "**mpi_comm_call_errhandler %C %d", comm, errorcode);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
