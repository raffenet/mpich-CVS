#include "mpiimpl.h"

/* void for now until error handlers are defined */
int MPIR_Return( void  *eh, int errcode )
{
    if (!eh) {
	;
    }
    return errcode;
}

int MPIR_Err_create_code( int class, const char def_string[], ... )
{
    return class;
}
