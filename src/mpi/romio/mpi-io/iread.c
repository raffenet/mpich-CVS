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
#pragma weak MPI_File_iread = PMPI_File_iread
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_iread MPI_File_iread
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_iread as PMPI_File_iread
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/*@
    MPI_File_iread - Nonblocking read using individual file pointer

Input Parameters:
. fh - file handle (handle)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. buf - initial address of buffer (choice)
. request - request object (handle)

.N fortran
@*/
#ifdef HAVE_MPI_GREQUEST
#include "mpiu_greq.h"

int MPI_File_iread(MPI_File fh, void *buf, int count, 
		   MPI_Datatype datatype, MPIO_Request *request)
{
	MPI_Status *status;
	int errcode;

	status = (MPI_Status *) ADIOI_Malloc(sizeof(MPI_Status));

	/* for now, no threads or anything fancy. 
	 * just call the blocking version */
	errcode = MPI_File_read(fh, buf, count, datatype, status); 
	/* ROMIO-1 doesn't do anything with status.MPI_ERROR */
	status->MPI_ERROR = errcode;

	/* kick off the request */
	MPI_Grequest_start(MPIU_Greq_query_fn, MPIU_Greq_free_fn, 
			   MPIU_Greq_cancel_fn, status, request);
	/* but we did all the work already */
	MPI_Grequest_complete(*request);

	/* passed the buck to the blocking version...*/
	return MPI_SUCCESS;
}
#else
int MPI_File_iread(MPI_File fh, void *buf, int count, 
                   MPI_Datatype datatype, MPIO_Request *request)
{
    int error_code;
    static char myname[] = "MPI_FILE_IREAD";
#ifdef MPI_hpux
    int fl_xmpi;

    HPMP_IO_START(fl_xmpi, BLKMPIFILEIREAD, TRDTSYSTEM, fh, datatype, count);
#endif /* MPI_hpux */

    error_code = ADIOI_File_iread(fh, (MPI_Offset) 0, ADIOI_INDIVIDUAL, buf,
				  count, datatype, myname, request);
    
#ifdef MPI_hpux
    HPMP_IO_END(fl_xmpi, fh, datatype, count);
#endif /* MPI_hpux */
    return error_code;
}
#endif

#ifndef HAVE_MPI_GREQUEST
int ADIOI_File_iread(MPI_File fh,
		     MPI_Offset offset,
		     int file_ptr_type,
		     void *buf,
		     int count,
		     MPI_Datatype datatype,
		     char *myname,
		     MPIO_Request *request)
{
    int error_code, bufsize, buftype_is_contig, filetype_is_contig;
    int datatype_size;
    ADIO_Status status;
    ADIO_Offset off;

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
#elif defined(PRINT_ERR_MSG)
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
#else /* MPICH-1 */
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

    if (fh->access_mode & MPI_MODE_WRONLY) {
#ifdef MPICH2
	error_code=  MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
					  myname, __LINE__, MPI_ERR_ACCESS, 
					  "**ioneedrd", 0);
	return MPIR_Err_return_file(fh, myname, error_code);
#elif defined(PRINT_ERR_MSG)
	FPRINTF(stderr, "%s: Can't read from a file opened with MPI_MODE_WRONLY\n", myname);
	MPI_Abort(MPI_COMM_WORLD, 1);
#else /* MPICH-1 */
	error_code = MPIR_Err_setmsg(MPI_ERR_UNSUPPORTED_OPERATION, 
 		MPIR_ERR_MODE_WRONLY, myname, (char *) 0, (char *) 0);
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

    ADIOI_Datatype_iscontig(datatype, &buftype_is_contig);
    ADIOI_Datatype_iscontig(fh->filetype, &filetype_is_contig);

    ADIOI_TEST_DEFERRED(fh, myname, &error_code);

    if (buftype_is_contig && filetype_is_contig) {
	/* convert count and offset to bytes */
	bufsize = datatype_size * count;

	if (file_ptr_type == ADIO_EXPLICIT_OFFSET) {
	    off = fh->disp + fh->etype_size * offset;
	}
	else {
	    off = fh->fp_ind;
	}

        if (!(fh->atomicity))
	    ADIO_IreadContig(fh, buf, count, datatype, file_ptr_type,
			off, request, &error_code); 
        else {
            /* to maintain strict atomicity semantics with other concurrent
              operations, lock (exclusive) and call blocking routine */

            *request = ADIOI_Malloc_request();
            (*request)->optype = ADIOI_READ;
            (*request)->fd = fh;
            (*request)->datatype = datatype;
            (*request)->queued = 0;
	    (*request)->handle = 0;

            if ((fh->file_system != ADIO_PIOFS) && 
              (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS)
	      && (fh->file_system != ADIO_PVFS2))
	    {
                ADIOI_WRITE_LOCK(fh, off, SEEK_SET, bufsize);
	    }

            ADIO_ReadContig(fh, buf, count, datatype, file_ptr_type, 
			    off, &status, &error_code);

            if ((fh->file_system != ADIO_PIOFS) && 
               (fh->file_system != ADIO_NFS) && (fh->file_system != ADIO_PVFS)
	       && (fh->file_system != ADIO_PVFS2))
	    {
                ADIOI_UNLOCK(fh, off, SEEK_SET, bufsize);
	    }

            fh->async_count++;
            /* status info. must be linked to the request structure, so that it
               can be accessed later from a wait */
        }
    }
    else ADIO_IreadStrided(fh, buf, count, datatype, file_ptr_type,
			   offset, request, &error_code); 

    return error_code;
}
#endif
