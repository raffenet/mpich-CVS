/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Foo */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Foo = PMPI_Foo
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Foo  MPI_Foo
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Foo as PMPI_Foo
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Foo PMPI_Foo

/* Any internal routines can go here.  Make them static if possible */
int MPIR_Foo_util( int a, MPID_Comm *comm )
{
...
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Foo

/*@
   MPI_Foo - short description

   Input Arguments:
+  first - 
.  middle - 
-  last - 

   Output Arguments:

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Foo( MPI_Comm comm, int a ) 
{
    static const char FCNAME[] = "MPI_Foo";
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FOO);
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
            if (a < 0) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_ARG, 
                            "**negarg", "**negarg %s %d", "a", a );
            } 
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
	    /* If comm_ptr is not value, it will be reset to null */
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FOO);
                return MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* Some routines must ensure only one thread modifies a communicator
       at a time, e.g., MPI_Comm_set_attr.  */
    MPID_Comm_thread_lock( comm_ptr );
    {
        ... actual code ...
    }
    MPID_Comm_thread_unlock( comm_ptr );
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FOO);
    return MPI_SUCCESS;
}
