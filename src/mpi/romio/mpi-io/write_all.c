/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/* status object not filled currently */

/*@
    MPI_File_write_all - Collective write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_all(MPI_File fh, void *buf, int count, 
                       MPI_Datatype datatype, MPI_Status *status)
{
    int error_code, datatype_size;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEWRITEALL, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_write_all: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (buf < (void *) 0) {
        printf("MPI_File_write_all: buf is not a valid address\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (count < 0) {
	printf("MPI_File_write_all: Invalid count argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (datatype == MPI_DATATYPE_NULL) {
        printf("MPI_File_write_all: Invalid datatype\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Type_size(datatype, &datatype_size);
    if ((count*datatype_size) % fh->etype_size != 0) {
        printf("MPI_File_write_all: Only an integral number of etypes can be accessed\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
	printf("MPI_File_write_all: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIO_WriteStridedColl(fh, buf, count, datatype, ADIO_INDIVIDUAL,
                          0, status, &error_code);
#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}
