/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define MPIO_CHECK_FILE_HANDLE(fh, myname, error_code)          \
if ((fh <= (MPI_File) 0) ||					\
    ((fh)->cookie != ADIOI_FILE_COOKIE)) {			\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,		\
				      MPIR_ERR_RECOVERABLE,	\
				      myname, __LINE__,		\
				      MPI_ERR_ARG,		\
				      "**iobadfh", 0);		\
    return MPIO_Err_return_file(MPI_FILE_NULL, error_code);	\
}

#define MPIO_CHECK_COUNT(fh, count, myname, error_code)         \
if (count < 0) {						\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,		\
				      MPIR_ERR_RECOVERABLE,	\
				      myname, __LINE__,		\
				      MPI_ERR_ARG, 		\
				      "**iobadcount", 0);	\
    return MPIO_Err_return_file(fh, error_code);		\
}

#define MPIO_CHECK_DATATYPE(fh, datatype, myname, error_code)   \
if (datatype == MPI_DATATYPE_NULL) {				\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,		\
				      MPIR_ERR_RECOVERABLE,	\
				      myname, __LINE__,		\
				      MPI_ERR_TYPE, 		\
				      "**dtypenull", 0);	\
    return MPIO_Err_return_file(fh, error_code);		\
}

#define MPIO_CHECK_READABLE(fh, myname, error_code)		\
if (fh->access_mode & MPI_MODE_WRONLY) {			\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,		\
				      MPIR_ERR_RECOVERABLE,	\
				      myname, __LINE__,		\
				      MPI_ERR_ACCESS, 		\
				      "**iowronly", 0);		\
    return MPIO_Err_return_file(fh, error_code);                \
}

#define MPIO_CHECK_WRITABLE(fh, myname, error_code)		\
if (fh->access_mode & MPI_MODE_RDONLY) {			\
    error_code = MPIR_Err_create_code(MPI_SUCCESS,		\
				      MPIR_ERR_RECOVERABLE,	\
				      myname, __LINE__,		\
				      MPI_ERR_READ_ONLY,	\
				      "**iordonly",		\
				      0);			\
    return MPIO_Err_return_file(fh, error_code);		\
}

#define MPIO_CHECK_NOT_SEQUENTIAL_MODE(fh, myname, error_code)		\
if (fh->access_mode & MPI_MODE_SEQUENTIAL) {				\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,			\
				      MPIR_ERR_RECOVERABLE,		\
				      myname, __LINE__,			\
				      MPI_ERR_UNSUPPORTED_OPERATION,	\
				      "**ioamodeseq", 0);		\
    return MPIO_Err_return_file(fh, error_code);                        \
}

#define MPIO_CHECK_INTEGRAL_ETYPE(fh, count, dtype_size, myname, error_code) \
if ((count*dtype_size) % fh->etype_size != 0) {				     \
    error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,     \
				      myname, __LINE__, MPI_ERR_IO, 	     \
				      "**ioetype", 0);			     \
    return MPIO_Err_return_file(fh, error_code);			     \
}

#define MPIO_CHECK_FS_SUPPORTS_SHARED(fh, myname, error_code)		\
if ((fh->file_system == ADIO_PIOFS) ||					\
    (fh->file_system == ADIO_PVFS) || 					\
    (fh->file_system == ADIO_PVFS2))					\
{									\
    error_code = MPIO_Err_create_code(MPI_SUCCESS,			\
				      MPIR_ERR_RECOVERABLE,		\
				      myname, __LINE__,			\
				      MPI_ERR_UNSUPPORTED_OPERATION,	\
				      "**iosharedunsupported", 0);	\
    return MPIO_Err_return_file(fh, error_code);			\
}


/* TODO: handle the independent io case more gracefully  */
#define ADIOI_TEST_DEFERRED(fh, myname, error_code)\
    if(! (fh)->is_open ) {\
	    ADIO_ImmediateOpen((fh), (error_code)); }
