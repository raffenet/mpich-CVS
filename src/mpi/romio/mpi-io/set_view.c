/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_set_view - Sets the file view

Input Parameters:
. fh - file handle (handle)
. disp - displacement (nonnegative integer)
. etype - elementary datatype (handle)
. filetype - filetype (handle)
. datarep - data representation (string)
. info - info object (handle)

.N fortran
@*/
int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
		 MPI_Datatype filetype, char *datarep, MPI_Info info)
{
    ADIO_Fcntl_t *fcntl_struct;
    int filetype_size, etype_size, error_code;

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_set_view: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (disp < 0) {
	printf("MPI_File_set_view: Invalid disp argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    /* rudimentary checks for incorrect etype/filetype.*/
    if (etype == MPI_DATATYPE_NULL) {
	printf("MPI_File_set_view: Invalid etype\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
    if (filetype == MPI_DATATYPE_NULL) {
	printf("MPI_File_set_view: Invalid filetype\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(filetype, &filetype_size);
    MPI_Type_size(etype, &etype_size);
    if (filetype_size % etype_size != 0) {
	printf("MPI_File_set_view: Filetype must be constructed out of one or more etypes\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (strcmp(datarep, "native") && strcmp(datarep, "NATIVE")) {
	printf("MPI_File_set_view: Only \"native\" data representation currently supported\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
    fcntl_struct->disp = disp;
    fcntl_struct->etype = etype;
    fcntl_struct->filetype = filetype;
    fcntl_struct->info = info;
    fcntl_struct->iomode = fh->iomode;

    ADIO_Fcntl(fh, ADIO_FCNTL_SET_VIEW, fcntl_struct, &error_code);

    ADIOI_Free(fcntl_struct);
    return error_code;
}
