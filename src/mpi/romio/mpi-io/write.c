/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write = PMPI_File_write
#pragma weak MPI_File_write_at = PMPI_File_write_at
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write MPI_File_write
#pragma _HP_SECONDARY_DEF PMPI_File_write_at MPI_File_write_at
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write as PMPI_File_write
#pragma _CRI duplicate MPI_File_write_at as PMPI_File_write_at
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

static int file_write(MPI_File fh,
		      MPI_Offset offset,
		      int file_ptr_type,
		      void *buf,
		      int count,
		      MPI_Datatype datatype,
		      char *myname,
		      MPI_Status *status);

/*@
    MPI_File_write - Write using individual file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write(MPI_File fh, void *buf, int count, 
                   MPI_Datatype datatype, MPI_Status *status)
{
    int error_code;
#if defined(MPICH2) || !defined(PRINT_ERR_MSG)
    static char myname[] = "MPI_FILE_WRITE";
#endif
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEWRITE, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    error_code = file_write(fh, (MPI_Offset) 0, ADIO_INDIVIDUAL, buf,
			    count, datatype, myname, status);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}

/*@
    MPI_File_write_at - Write using explict offset

Input Parameters:
. fh - file handle (handle)
. offset - file offset (nonnegative integer)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, void *buf,
                      int count, MPI_Datatype datatype, 
                      MPI_Status *status)
{
    int error_code;
#if defined(MPICH2) || !defined(PRINT_ERR_MSG)
    static char myname[] = "MPI_FILE_WRITE_AT";
#endif
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEWRITEAT, TRDTBLOCK, fh, datatype, count);
#endif /* MPI_hpux */

    error_code = file_write(fh, offset, ADIO_EXPLICIT_OFFSET, buf, count,
			    datatype, myname, status);

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}

static int file_write(MPI_File fh,
		      MPI_Offset offset,
		      int file_ptr_type,
		      void *buf,
		      int count,
		      MPI_Datatype datatype,
		      char *myname,
		      MPI_Status *status)
{		      
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size;
    ADIO_Offset off;

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

    if (file_ptr_type == ADIO_EXPLICIT_OFFSET && offset < 0)
    {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_ARG, 
					  "**iobadoffset", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Invalid offset argument\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_ARG, MPIR_ERR_OFFSET_ARG,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);	    
#endif
    }

    if (count < 0)
    {
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

    if (datatype == MPI_DATATYPE_NULL)
    {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_TYPE,
					  "**dtypenull", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
        FPRINTF(stderr, "%s: Invalid datatype\n", myname);
        MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_TYPE, MPIR_ERR_TYPE_NULL,
				     myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);	    
#endif
    }
    /* --END ERROR HANDLING-- */

    MPI_Type_size(datatype, &datatype_size);
    if (count*datatype_size == 0)
    {
#ifdef MPI_hpux
	HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
#ifdef HAVE_STATUS_SET_BYTES
       MPIR_Status_set_bytes(status, datatype, 0);
#endif
	
	return MPI_SUCCESS;
    }

    /* --BEGIN ERROR HANDLING-- */
    if ((count*datatype_size) % fh->etype_size != 0)
    {
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

    if (fh->access_mode & MPI_MODE_SEQUENTIAL)
    {
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

    if (fh->access_mode & MPI_MODE_RDONLY)
    {
#ifdef MPICH2
	error_code = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_READ_ONLY,
					  "**filerdonly", "**filerdonly %s",
					  fh->filename );
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Can't use this function because file was opened with MPI_MODE_RDONLY\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_READ_ONLY, 
                        0, myname, (char *) 0, (char *) 0);
	return ADIOI_Error(fh, error_code, myname);
#endif
    }
    /* --END ERROR HANDLING-- */

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(fh, myname, &error_code);

    /* contiguous or strided? */

    if (buftype_is_contig && filetype_is_contig)
    {
    /* convert bufocunt and offset to bytes */
	bufsize = datatype_size * count;
	if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	    off = fh->disp + fh->etype_size * offset;
	}
	else /* ADIO_INDIVIDUAL */ {
	    off = fh->fp_ind;
	}

        /* if atomic mode requested, lock (exclusive) the region, because there
           could be a concurrent noncontiguous request. Locking doesn't 
           work on PIOFS and PVFS, and on NFS it is done in the ADIO_WriteContig.*/

        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
            (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS) &&
	    	(fh->file_system != ADIO_PVFS2))
            ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);

	ADIO_WriteContig(fh, buf, count, datatype, file_ptr_type,
		     off, status, &error_code); 

        if ((fh->atomicity) && (fh->file_system != ADIO_PIOFS) && 
            (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS)&&
	    	(fh->file_system != ADIO_PVFS2))
            ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);
    }
    else
    {
	ADIO_WriteStrided(fh, buf, count, datatype, file_ptr_type,
			 offset, status, &error_code);
	/* For strided and atomic mode, locking is done in ADIO_WriteStrided */
    }

#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}
