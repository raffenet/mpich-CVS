#include "mpiimpl.h"
#include <stdio.h>

/* void for now until error handlers are defined */
int MPIR_Err_return_comm( MPID_Comm  *comm_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the communicator */
    if (comm_ptr && comm_ptr->errhandler) {
	switch (comm_ptr->errhandler->language) {
	case MPID_LANG_C:
	case MPID_LANG_CXX:
	    (*comm_ptr->errhandler->errfn.C_Comm_Handler_function)( &comm_ptr->id, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*comm_ptr->errhandler->errfn.F77_Handler_function)( (MPI_Fint *)&comm_ptr->id, &errcode );
	    break;
	}
    }
    else {
	/* No communicator, so errors are fatal */
	printf( "Fatal error %d\n", errcode );
	exit(1); /* Change this to MPID_Abort */
    }
    return errcode;
}

int MPIR_Err_return_win( MPID_Win  *win_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the communicator */
    if (win_ptr && win_ptr->errhandler) {
	switch (win_ptr->errhandler->language) {
	case MPID_LANG_C:
	case MPID_LANG_CXX:
	    (*win_ptr->errhandler->errfn.C_Comm_Handler_function)( &win_ptr->id, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*win_ptr->errhandler->errfn.F77_Handler_function)( (MPI_Fint *)&win_ptr->id, &errcode );
	    break;
	}
    }
    else {
	/* No communicator, so errors are fatal */
	printf( "Fatal error %d\n", errcode );
	exit(1); /* Change this to MPID_Abort */
    }
    return errcode;
}

int MPIR_Err_return_file( MPID_File  *file_ptr, const char fcname[], 
			  int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the communicator */
    if (file_ptr && file_ptr->errhandler) {
	switch (file_ptr->errhandler->language) {
	case MPID_LANG_C:
	case MPID_LANG_CXX:
	    (*file_ptr->errhandler->errfn.C_Comm_Handler_function)( &file_ptr->id, &errcode );
	    break;
	case MPID_LANG_FORTRAN90:
	case MPID_LANG_FORTRAN:
	    (*file_ptr->errhandler->errfn.F77_Handler_function)( (MPI_Fint *)&file_ptr->id, &errcode );
	    break;
	}
    }
    else {
	/* No communicator, so errors are fatal */
	printf( "Fatal error %d\n", errcode );
	exit(1); /* Change this to MPID_Abort */
    }
    return errcode;
}

int MPIR_Err_create_code( int class, const char def_string[], ... )
{
    return class;
}

/* 
   Nesting level for routines.
   Note that since these use per-thread data, no locks or atomic update
   routines are required 
 */
void MPIR_Nest_incr( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    p->nest_count++;
}
void MPIR_Nest_decr( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    p->nest_count--;
}
int MPIR_Nest_value( void )
{
    MPICH_PerThread_t *p;
    MPID_GetPerThread(p);
    return p->nest_count;
}
