/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_seek - Updates the individual file pointer

Input Parameters:
. fh - file handle (handle)
. offset - file offset (integer)
. whence - update mode (state)

.N fortran
@*/
int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence)
{
    int error_code;
    MPI_Offset curr_offset, eof_offset;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILESEEK, TRDTBLOCK, fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_seek: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
	printf("MPI_File_seek: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    switch(whence) {
    case MPI_SEEK_SET:
	if (offset < 0) {
	    printf("MPI_File_seek: Invalid offset argument\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	break;
    case MPI_SEEK_CUR:
	/* find offset corr. to current location of file pointer */
	ADIOI_Get_position(fh, &curr_offset);
	offset += curr_offset;
	if (offset < 0) {
	    printf("MPI_File_seek: offset points to a negative location in the file\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	break;
    case MPI_SEEK_END:
	/* find offset corr. to end of file */
	ADIOI_Get_eof_offset(fh, &eof_offset);
	offset += eof_offset;
	if (offset < 0) {
	    printf("MPI_File_seek: offset points to a negative location in the file\n");
	    MPI_Abort(MPI_COMM_WORLD, 1);
	}
	break;
    default:
	printf("MPI_File_seek: Invalid whence argument\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    ADIO_SeekIndividual(fh, offset, ADIO_SEEK_SET, &error_code);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */
    return error_code;
}
