/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bsendutil.h"

/* -- Begin Profiling Symbol Block for routine MPI_Buffer_detach */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Buffer_detach = PMPI_Buffer_detach
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Buffer_detach  MPI_Buffer_detach
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Buffer_detach as PMPI_Buffer_detach
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Buffer_detach PMPI_Buffer_detach

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Buffer_detach

/*@
  MPI_Buffer_detach - Removes an existing buffer (for use in MPI_Bsend etc)

Output Parameters:
+ buffer - initial buffer address (choice) 
- size - buffer size, in bytes (integer) 

Notes:
    The reason that 'MPI_Buffer_detach' returns the address and size of the
buffer being detached is to allow nested libraries to replace and restore
the buffer.  For example, consider

.vb
    int size, mysize, idummy;
    void *ptr, *myptr, *dummy;     
    MPI_Buffer_detach( &ptr, &size );
    MPI_Buffer_attach( myptr, mysize );
    ...
    ... library code ...
    ...
    MPI_Buffer_detach( &dummy, &idummy );
    MPI_Buffer_attach( ptr, size );
.ve

This is much like the action of the Unix signal routine and has the same
strengths (it is simple) and weaknesses (it only works for nested usages).

Note that for this approach to work, MPI_Buffer_detach must return MPI_SUCCESS
even when there is no buffer to detach.  In that case, it returns a size of
zero.  The MPI 1.1 standard for 'MPI_BUFFER_DETACH' contains the text

.vb
   The statements made in this section describe the behavior of MPI for
   buffered-mode sends. When no buffer is currently associated, MPI behaves 
   as if a zero-sized buffer is associated with the process.
.ve

This could be read as applying only to the various Bsend routines.  This 
implementation takes the position that this applies to 'MPI_BUFFER_DETACH'
as well.

.N Fortran

    The Fortran binding for this routine is different.  Because Fortran 
    does not have pointers, it is impossible to provide a way to use the
    output of this routine to exchange buffers.  In this case, only the
    size field is set.

Notes for C:
    Even though the 'bufferptr' argument is declared as 'void *', it is
    really the address of a void pointer.  See the rationale in the 
    standard for more details. 
@*/
int MPI_Buffer_detach(void *buffer, int *size)
{
    static const char FCNAME[] = "MPI_Buffer_detach";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BUFFER_DETACH);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_BUFFER_DETACH);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
	    MPIR_ERRTEST_ARGNULL(size,"size",mpi_errno);
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BUFFER_DETACH);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Bsend_detach( buffer, size );
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_BUFFER_DETACH);
    if (mpi_errno) {
	return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
    }	
    return MPI_SUCCESS;
}

