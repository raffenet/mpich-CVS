/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Get_address */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Get_address = PMPI_Get_address
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Get_address  MPI_Get_address
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Get_address as PMPI_Get_address
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Get_address PMPI_Get_address

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Get_address

/*@
   MPI_Get_address - get address

   Arguments:
+  void *location - location
-  MPI_Aint *address - address

   Notes:
    This routine is provided for both the Fortran and C programmers.
    On many systems, the address returned by this routine will be the same
    as produced by the C '&' operator, but this is not required in C and
    may not be true of systems with word- rather than byte-oriented 
    instructions or systems with segmented address spaces.  

    This routine should be used instead of 'MPI_Address'. 
.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPI_Get_address(void *location, MPI_Aint *address)
{
    static const char FCNAME[] = "MPI_Get_address";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_GET_ADDRESS);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_GET_ADDRESS);
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (MPIR_Process.initialized != MPICH_WITHIN_MPI) {
                mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER,
                            "**initialized", 0 );
            }
            if (mpi_errno) {
                MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ADDRESS);
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    /* SX_4 needs to set CHAR_PTR_IS_ADDRESS 
       The reason is that it computes the different in two pointers in
       an "int", and addresses typically have the high (bit 31) bit set;
       thus the difference, when cast as MPI_Aint (long), is sign-extended, 
       making the absolute address negative.  Without a copy of the C 
       standard, I can't tell if this is a compiler bug or a language bug.
    */
#ifdef CHAR_PTR_IS_ADDRESS
    *address = (MPI_Aint) ((char *)location);
#else
    /* Note that this is the "portable" way to generate an address.
       The difference of two pointers is the number of elements
       between them, so this gives the number of chars between location
       and ptr.  As long as sizeof(char) represents one byte, 
       of bytes from 0 to location */
    *address = (MPI_Aint) ((char *)location - (char *)MPI_BOTTOM);
#endif
    /* The same code is used in MPI_Address */
    /* ... end of body of routine ... */

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_GET_ADDRESS);
    return MPI_SUCCESS;
}
