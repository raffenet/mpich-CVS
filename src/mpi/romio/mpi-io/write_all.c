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
#pragma weak MPI_File_write_all = PMPI_File_write_all
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_all MPI_File_write_all
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_all as PMPI_File_write_all
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

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
    int error_code;
    static char myname[] = "MPI_FILE_WRITE_ALL";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEWRITEALL, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    error_code = ADIOI_File_write_all(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf,
				      count, datatype, myname, status);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}

int ADIOI_File_write_all(MPI_File fh,
			 MPI_Offset offset,
			 int file_ptr_type,
			 void *buf,
			 int count,
			 MPI_Datatype datatype,
			 char *myname,
			 MPI_Status *status)
{
    int error_code, datatype_size;

    /* --BEGIN ERROR HANDLING-- */
#ifdef PRINT_ERR_MSG
    if ((fh <= (MPI_File) 0) || (fh->cookie != ADIOI_FILE_COOKIE)) {
	FPRINTF(stderr, "%s: Invalid file handle\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
    }
#else
    ADIOI_TEST_FILE_HANDLE(fh, myname);
#endif

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0) {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_ARG, 
					  "**iobadoffset", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif  defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Invalid offset argument\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_ARG, MPIR_ERR_OFFSET_ARG,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);	    
#endif
    }

    if (count < 0) {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_ARG,
					  "**iobadcount", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Invalid count argument\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_ARG, MPIR_ERR_COUNT_ARG,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);
#endif
    }

    if (datatype == MPI_DATATYPE_NULL) {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_TYPE,
					  "**dtypenull", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
        FPRINTF(stderr, "%s: Invalid datatype\n", myname);
        MPI_Abort(MPI_COMM_WORLD, 1);
#else
	error_code = MPIR_Err_setmsg(MPI_ERR_TYPE, MPIR_ERR_TYPE_NULL,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);	    
#endif
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    if ((count*datatype_size) % fh->etype_size != 0) {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_IO,
					  "**ioetype", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
        FPRINTF(stderr, "%s: Only an integral number of etypes can be accessed\n", myname);
        MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_IO, MPIR_ERR_ETYPE_FRACTIONAL,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);	    
#endif
    }

    if (fh->access_mode & MPI_MODE_SEQUENTIAL) {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_UNSUPPORTED_OPERATION,
					  "**ioamodeseq", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Can't use this function because file was opened with MPI_MODE_SEQUENTIAL\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_UNSUPPORTED_OPERATION, 
                        MPIR_ERR_AMODE_SEQ, myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);
#endif
    }
    /* --END ERROR HANDLING-- */

    ADIO_WriteStridedColl(fh, buf, count, datatype, file_ptr_type,
                          offset, status, &error_code);
    return error_code;
}
