/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 2004 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include <stdarg.h>
#include <stdio.h>

#include "mpioimpl.h"

/* MPICH2 error handling implementation */

int MPIO_Err_create_code(int lastcode, int fatal, const char fcname[],
			 int line, int error_class, const char generic_msg[],
			 const char specific_msg[], ... )
{
    va_list Argp;
    int error_code;

    va_start(Argp, specific_msg);
	
    error_code = MPIR_Err_setmsg(error_class, 0, fcname, generic_msg,
				 specific_msg, Argp);

    vfprintf(stderr, specific_msg, Argp);
    
    va_end(Argp);

    return error_code;
}

int MPIO_Err_return_file(MPI_File mpi_fh, int error_code)
{
    return ADIOI_Error(mpi_fh, error_code, "I/O Error");
}

int MPIO_Err_return_comm(MPI_Comm mpi_comm, int error_code)
{
    return ADIOI_Error(MPI_FILE_NULL, error_code, "I/O Error");
}
