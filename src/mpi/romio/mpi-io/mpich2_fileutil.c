/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpioimpl.h"

#ifdef MPICH2

int MPIR_Err_return_file( MPI_File file_ptr, const char fcname[], int errcode )
{
    /* First, check the nesting level */
    /*if (MPIR_Nest_value()) return errcode;*/ /* Is there any recursion in the MPI_File_... interface? */

    /* If the file pointer is not valid, we use the handler on
       MPI_FILE_NULL (MPI-2, section 9.7).  For now, this code assumes that 
       MPI_FILE_NULL has the default handler (return).  FIXME */
    if (MPIR_Err_is_fatal(errcode) ||
	(file_ptr != NULL && file_ptr->err_handler == MPI_ERRORS_ARE_FATAL))
    {
	if (MPIR_Err_print_stack_flag)
	{
	    fprintf( stderr, "Fatal error (code 0x%08x) in %s():\n", errcode, fcname);
	    MPIR_Err_print_stack(stderr, errcode);
	}
	else
	{
	    /* The default handler should try the following: Provide the rank in comm_world.  If the process is not in comm world,
	       use something else.  If the communicator exists and has a name, provide that name */
	    char msg[MPI_MAX_ERROR_STRING];

	    MPIR_Err_get_string( errcode, msg );
	    fprintf( stderr, "Fatal error (code 0x%08x) in %s(): %s\n", errcode, fcname, msg);
	}

	abort(); /* Change this to MPID_Abort */
    }

    return errcode;
}

/* These next two routines are used to allow MPICH2 to access/set the
   error handers in the MPI_File structure until MPICH2 knows about the
   file structure, and to handle the errhandler structure, which 
   includes a reference count.  Not currently used. */
int MPIR_ROMIO_Set_file_errhand( MPI_File file_ptr, MPI_Errhandler e )
{
    file_ptr->err_handler = e;
    return 0;
}
int MPIR_ROMIO_Get_file_errhand( MPI_File file_ptr, MPI_Errhandler *e )
{
    *e = file_ptr->err_handler;
    return 0;
}

#endif /* MPICH2 */
