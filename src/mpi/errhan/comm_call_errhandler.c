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
   MPI_Comm_call_errhandler - call communicator error handler

   Arguments:
+  MPI_Comm comm - communicator
-  int errorcode - error code

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
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
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    switch (comm_ptr->errhandler->language) {
    case MPID_LANG_C:
    case MPID_LANG_CXX:
	(*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( 
	    &comm_ptr->handle, &errorcode );
	break;
    case MPID_LANG_FORTRAN90:
    case MPID_LANG_FORTRAN:
	(*comm_ptr->errhandler->errfn.F77_Handler_function)( 
	    (MPI_Fint *)&comm_ptr->handle, &errorcode );
	break;
    }
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CALL_ERRHANDLER);
    return MPI_SUCCESS;
}
