/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

/*@
    MPI_File_preallocate - Preallocates storage space for a file

Input Parameters:
. fh - file handle (handle)
. size - size to preallocate (nonnegative integer)

.N fortran
@*/
int MPI_File_preallocate(MPI_File fh, MPI_Offset size)
{
    ADIO_Fcntl_t *fcntl_struct;
    int error_code, mynod;
    MPI_Offset tmp_sz;
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEPREALLOCATE, TRDTBLOCK,
		  fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */

    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	printf("MPI_File_preallocate: Invalid file handle\n");
	MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (size < 0) {
        printf("MPI_File_preallocate: Invalid size argument\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    tmp_sz = size;
    MPI_Bcast(&tmp_sz, 1, ADIO_OFFSET, 0, fh->comm);

    if (tmp_sz != size) {
        printf("MPI_File_preallocate: size argument must be the same on all processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (size == 0) return MPI_SUCCESS;

    MPI_Comm_rank(fh->comm, &mynod);
    if (!mynod) {
	fcntl_struct = (ADIO_Fcntl_t *) ADIOI_Malloc(sizeof(ADIO_Fcntl_t));
	fcntl_struct->diskspace = size;
	ADIO_Fcntl(fh, ADIO_FCNTL_SET_DISKSPACE, fcntl_struct, &error_code);
	ADIOI_Free(fcntl_struct);
    }
    MPI_Barrier(fh->comm);
    
#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, MPI_DATATYPE_NULL, -1);
#endif /* MPI_hpux */
    if (!mynod) return error_code;
    else return MPI_SUCCESS;
}
