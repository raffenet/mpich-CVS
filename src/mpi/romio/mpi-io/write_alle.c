/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   $Id$    
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_all_end = PMPI_File_write_all_end
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_all_end MPI_File_write_all_end
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_all_end as PMPI_File_write_all_end
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_write_all_end - Complete a split collective write using individual file pointer

Input Parameters:
. fh - file handle (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_all_end(MPI_File mpi_fh, void *buf, MPI_Status *status)
{
    int error_code;
    static char myname[] = "MPI_FILE_WRITE_ALL_END";

    error_code = MPIOI_File_write_all_end(mpi_fh, buf, myname, status);

    return error_code;
}

int MPIOI_File_write_all_end(MPI_File mpi_fh,
			     void *buf,
			     char *myname,
			     MPI_Status *status)
{
    int error_code;
    ADIO_File fh;

    fh = MPIO_File_resolve(mpi_fh);

    /* --BEGIN ERROR HANDLING-- */
#ifdef PRINT_ERR_MSG
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE))
    {
	FPRINTF(stderr, "%s: Invalid file handle\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#else
    ADIOI_TEST_FILE_HANDLE(fh, myname);
#endif

    if (!(fh->split_coll_count))
    {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_IO, 
					  "**iosplitcollnone", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
        FPRINTF(stderr, "%s: Does not match a previous MPI_File_write_at_all_begin\n", myname);
        MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ERR_NO_SPLIT_COLL,
                              myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);
#endif
    }
    /* --END ERROR HANDLING-- */

#ifdef HAVE_STATUS_SET_BYTES
    /* FIXME - we should really ensure that the split_datatype remains
       valid by incrementing the ref count in the write_allb.c routine
       and decrement it here after setting the bytes */
    if (status != MPI_STATUS_IGNORE)
       *status = fh->split_status;
#endif
    fh->split_coll_count = 0;

    return MPI_SUCCESS;
}
