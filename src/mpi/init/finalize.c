/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Finalize */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Finalize = PMPI_Finalize
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Finalize  MPI_Finalize
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Finalize as PMPI_Finalize
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#define MPI_Finalize PMPI_Finalize

/* Any internal routines can go here.  Make them static if possible */

/* The following routines provide a callback facility for modules that need 
   some code called on exit.  This method allows us to avoid forcing 
   MPI_Finalize to know the routine names a priori.  Any module that wants to 
   have a callback calls MPIR_Add_finalize( routine, extra ).
 */
typedef struct {
    int (*f)( void * );
    void *extra_data;
} Finalize_func_t;
#define MAX_FINALIZE_FUNC 16
static Finalize_func_t fstack[MAX_FINALIZE_FUNC];
static int fstack_sp = 0;

void MPIR_Add_finalize( int (*f)( void * ), void *extra_data )
{
    if (fstack_sp >= MAX_FINALIZE_FUNC) {
    }
    fstack[fstack_sp].f            = f;
    fstack[fstack_sp++].extra_data = extra_data;
}

PMPI_LOCAL void MPIR_Call_finalize_callbacks( void );
PMPI_LOCAL void MPIR_Call_finalize_callbacks( void )
{
    int i;
    for (i=fstack_sp-1; i>=0; i--) {
	if (fstack[i].f) fstack[i].f( fstack[i].extra_data );
    }
    fstack_sp = 0;
}
#else
#ifndef USE_WEAK_SYMBOLS
PMPI_LOCAL void MPIR_Call_finalize_callbacks( void );
#endif
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Finalize

/*@
   MPI_Finalize - Terminates MPI execution environment

   Notes:
   All processes must call this routine before exiting.  The number of
   processes running `after` this routine is called is undefined; 
   it is best not to perform much more than a 'return rc' after calling
   'MPI_Finalize'.

.N fortran

.N Errors
.N MPI_SUCCESS
.N ... others
@*/
int MPI_Finalize( void )
{
    static const char FCNAME[] = "MPI_Finalize";
    int mpi_errno = MPI_SUCCESS;
    MPID_MPI_FINALIZE_STATE_DECLS;


#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_INITIALIZED(mpi_errno);
            if (mpi_errno) {
                return MPIR_Err_return_comm( 0, FCNAME, mpi_errno );
	    }
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    MPID_MPI_FINALIZE_FUNC_ENTER(MPID_STATE_MPI_FINALIZE);
    
    /* ... body of routine ...  */
    MPID_Timer_finalize();
    
    MPID_Finalize();

    MPIR_Call_finalize_callbacks();

    /* If memory debugging is enabled, check the memory here */
#ifdef USE_MEMORY_TRACING
    if (1) {
	MPIU_trdump( (void *)0 );
    }
#endif

    /* ... end of body of routine ... */

    MPID_MPI_FINALIZE_FUNC_EXIT(MPID_STATE_MPI_FINALIZE);
    return MPI_SUCCESS;
}
