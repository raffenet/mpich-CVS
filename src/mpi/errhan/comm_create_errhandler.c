/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_create_errhandler */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_create_errhandler = PMPI_Comm_create_errhandler
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_create_errhandler  MPI_Comm_create_errhandler
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_create_errhandler as PMPI_Comm_create_errhandler
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Comm_create_errhandler PMPI_Comm_create_errhandler

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_create_errhandler

/*@
   MPI_Comm_create_errhandler - create a communicator error handler

   Arguments:
+  MPI_Comm_errhandler_fn *function - function
-  MPI_Errhandler *errhandler - error handler

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Comm_create_errhandler(MPI_Comm_errhandler_fn *function, MPI_Errhandler *errhandler)
{
    static const char FCNAME[] = "MPI_Comm_create_errhandler";
    int mpi_errno = MPI_SUCCESS;
    MPID_Errhandler *errhan_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_CREATE_ERRHANDLER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_CREATE_ERRHANDLER);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_ERRHANDLER);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    errhan_ptr = (MPID_Errhandler *)MPIU_Handle_obj_alloc( &MPID_Errhandler_mem );
    if (!errhan_ptr) {
	mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "**nomem", 0 );
	MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_ERRHANDLER);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }
    *errhandler		 = errhan_ptr->handle;
    errhan_ptr->language = MPID_LANG_C;
    errhan_ptr->kind	 = MPID_COMM;
    MPIU_Object_set_ref(errhan_ptr,1);
    errhan_ptr->errfn.C_Comm_Handler_function = function;
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_CREATE_ERRHANDLER);
    return MPI_SUCCESS;
}
