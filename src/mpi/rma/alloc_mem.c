/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Alloc_mem */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Alloc_mem = PMPI_Alloc_mem
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Alloc_mem  MPI_Alloc_mem
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Alloc_mem as PMPI_Alloc_mem
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Alloc_mem PMPI_Alloc_mem

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Alloc_mem

/*@
   MPI_Alloc_mem - allocate memory

   Arguments:
+  MPI_Aint size - size
.  MPI_Info info - info
-  void *baseptr - base pointer

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr)
{
    static const char FCNAME[] = "MPI_Alloc_mem";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_ALLOC_MEM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_ALLOC_MEM);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLOC_MEM);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
            if (size < 0)
                mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_ARG,
                               "**argneg", "**argneg %s %d", "size", size);  
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLOC_MEM);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    baseptr = MPIU_Malloc(size);
    if (!baseptr) {
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, MPI_ERR_NO_MEM, "**allocmem", 0 );
        return mpi_errno;
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_ALLOC_MEM);
    return MPI_SUCCESS;
}
