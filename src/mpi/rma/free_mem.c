/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Free_mem */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Free_mem = PMPI_Free_mem
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Free_mem  MPI_Free_mem
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Free_mem as PMPI_Free_mem
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Free_mem PMPI_Free_mem

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Free_mem

/*@
   MPI_Free_mem - Free memory allocated with MPI_Alloc_mem

   Input Parameter:
.  base - initial address of memory segment allocated by 'MPI_ALLOC_MEM' 
       (choice) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Free_mem(void *base)
{
    static const char FCNAME[] = "MPI_Free_mem";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_FREE_MEM);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_FREE_MEM);
    MPIR_ERRTEST_INITIALIZED_FIRSTORJUMP;

    MPIU_Free(base);

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FREE_MEM);
    return MPI_SUCCESS;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
#ifdef HAVE_ERROR_CHECKING
    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, 
				     FCNAME, __LINE__, MPI_ERR_OTHER,
	"**mpi_free_mem", "**mpi_free_mem %p", base);
#endif
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_FREE_MEM);
    return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    /* --END ERROR HANDLING-- */
}
