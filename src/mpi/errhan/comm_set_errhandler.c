/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_set_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_set_errhandler = PMPI_Comm_set_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_set_errhandler  MPI_Comm_set_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_set_errhandler as PMPI_Comm_set_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_set_errhandler PMPI_Comm_set_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_set_errhandler

/*@
   MPI_Comm_set_errhandler - Set the error handler for a communicator

   Input Parameters:
+ comm - communicator (handle) 
- errhandler - new error handler for communicator (handle) 


   Arguments:
+  MPI_Comm comm - communicator
-  MPI_Errhandler errhandler - error handler

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_OTHER
@*/
int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    static const char FCNAME[] = "MPI_Comm_set_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int in_use;
    MPID_Errhandler *errhan_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_SET_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_SET_ERRHANDLER);
    /* Get handles to MPI objects. */
    MPID_Comm_get_ptr( comm, comm_ptr );
    MPID_Errhandler_get_ptr( errhandler, errhan_ptr );
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not valid, it will be reset to null */

	    if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
		MPID_Errhandler_valid_ptr( errhan_ptr, mpi_errno );
	    }
	    /* FIXME: Add a check that this is a comm errhandler.
	       This test should be a macro so that it can be used
	       in all calls, along with similar calls for win and file */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ERRHANDLER);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (comm_ptr->errhandler != NULL) {
	if (HANDLE_GET_KIND(errhandler) != HANDLE_KIND_BUILTIN) {
	    MPIU_Object_release_ref(comm_ptr->errhandler,&in_use);
	    if (!in_use) {
		MPID_Errhandler_free( comm_ptr->errhandler );
	    }
	}
    }
    
    MPIU_Object_add_ref(errhan_ptr);
    comm_ptr->errhandler = errhan_ptr;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_SET_ERRHANDLER);
    return MPI_SUCCESS;
}

