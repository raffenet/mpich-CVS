/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifdef MPICH2
#define ADIOI_TEST_FILE_HANDLE(fh, myname) \
{if (!(fh)) { \
	error_code = MPIR_Err_create_code(MPI_ERR_FILE, "**iofilenull", 0);\
	return MPIR_Err_return_file(MPI_FILE_NULL, myname, error_code);}\
 else if ((fh)->cookie != ADIOI_FILE_COOKIE) { \
    error_code = MPIR_Err_create_code(MPI_ERR_FILE, "**iofilecorrupt", 0);\
    return MPIR_Err_return_file(MPI_FILE_NULL, myname, error_code); } }
#else /* MPICH-1 */
#define ADIOI_TEST_FILE_HANDLE(fh, myname) \
{if (!(fh)) { \
    error_code = MPIR_Err_setmsg(MPI_ERR_FILE, MPIR_ERR_FILE_NULL, myname, (char *) 0, (char *) 0); \
    return ADIOI_Error(MPI_FILE_NULL, error_code, myname); } \
 else if ((fh)->cookie != ADIOI_FILE_COOKIE) { \
    error_code = MPIR_Err_setmsg(MPI_ERR_FILE, MPIR_ERR_FILE_CORRUPT, myname, (char *) 0, (char *) 0); \
    return ADIOI_Error(MPI_FILE_NULL, error_code, myname); } }
#endif
