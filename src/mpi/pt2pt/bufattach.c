/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Buffer_attach */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Buffer_attach = PMPI_Buffer_attach
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Buffer_attach  MPI_Buffer_attach
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Buffer_attach as PMPI_Buffer_attach
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Buffer_attach PMPI_Buffer_attach

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Buffer_attach

/*@
  MPI_Buffer_attach - Attaches a user-defined buffer for sending 

Input Parameters:
+ buffer - initial buffer address (choice) 
- size - buffer size, in bytes (integer) 

Notes:
The size given should be the sum of the sizes of all outstanding Bsends that
you intend to have, plus 'MPI_BSEND_OVERHEAD' for each Bsend that you do.
For the purposes of calculating size, you should use 'MPI_Pack_size'. 
In other words, in the code
.vb
     MPI_Buffer_attach( buffer, size );
     MPI_Bsend( ..., count=20, datatype=type1,  ... );
     ...
     MPI_Bsend( ..., count=40, datatype=type2, ... );
.ve
the value of 'size' in the 'MPI_Buffer_attach' call should be greater than
the value computed by
.vb
     MPI_Pack_size( 20, type1, comm, &s1 );
     MPI_Pack_size( 40, type2, comm, &s2 );
     size = s1 + s2 + 2 * MPI_BSEND_OVERHEAD;
.ve    
The 'MPI_BSEND_OVERHEAD' gives the maximum amount of space that may be used in 
the buffer for use by the BSEND routines in using the buffer.  This value 
is in 'mpi.h' (for C) and 'mpif.h' (for Fortran).

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_BUFFER
.N MPI_ERR_INTERN

.seealso: MPI_Buffer_detach, MPI_Bsend
@*/
int MPI_Buffer_attach(void *buffer, int size)
{
    static const char FCNAME[] = "MPI_Buffer_attach";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BUFFER_ATTACH);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_BUFFER_ATTACH);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNEG(size,"size",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BUFFER_ATTACH);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Bsend_attach( buffer, size );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BUFFER_ATTACH);
    if (mpi_errno)
    {
	/* --BEGIN ERROR HANDLING-- */
	/* FIXME: This is wrong, since the internal routine returns the
	   correct message for the user */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
	    "**mpi_buffer_attach", "**mpi_buffer_attach %p %d", buffer, size);
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	/* --END ERROR HANDLING-- */
    }	
    /* ... end of body of routine ... */
    return MPI_SUCCESS;
}
