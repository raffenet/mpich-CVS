#include "mpiimpl.h"

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
