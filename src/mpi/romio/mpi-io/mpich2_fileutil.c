#include "adio.h"

#ifdef MPICH2

int MPIR_Err_return_file( MPI_File file_ptr, const char fcname[], int errcode )
{
    /* First, check the nesting level */
    if (MPIR_Nest_value()) return errcode;

    /* Now, invoke the error handler for the file */
    if (file_ptr && file_ptr->err_handler) {
	/* Now, invoke the error handler for the communicator */
        if (file_ptr->err_handler == MPI_ERRORS_ARE_FATAL) {
	    /* Try to print the associated message */
	    const char *p = MPIR_Err_get_string( errcode );
	    
	    /* The default handler should try the following:
	       Provide the rank in comm_world.  If the process is not
	       in comm world, use something else.  If the communicator
	       exists and has a name, provide that name */
	    if (p) {
		fprintf( stderr, "Fatal error: %s in %s\n", p, fcname );
	    }
	    else
	    {
		fprintf( stderr, "Fatal error (code %d) in %s\n", errcode, fcname );
	    }
	    abort(); /* Change this to MPID_Abort */
	}
        else if (file_ptr->err_handler == MPI_ERRORS_RETURN) {
	    return errcode;
	}
	else {
	    fprintf( stderr, "Unhandled err_handler type: %d\n", file_ptr->err_handler );
	}
    }
    else {
	/* No file, so errors return */
	return errcode;
    }
    return errcode;
}

#endif /* MPICH2 */
