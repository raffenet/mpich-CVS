/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpioimpl.h"

#ifdef HAVE_WEAK_SYMBOLS

#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_File_write_ordered = PMPI_File_write_ordered
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_File_write_ordered MPI_File_write_ordered
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_File_write_ordered as PMPI_File_write_ordered
/* end of weak pragmas */
#endif

/* Include mapping from MPI->PMPI */
#define MPIO_BUILD_PROFILING
#include "mpioprof.h"
#endif

/* status object not filled currently */

/*@
    MPI_File_write_ordered - Collective write using shared file pointer

Input Parameters:
. fh - file handle (handle)
. buf - initial address of buffer (choice)
. count - number of elements in buffer (nonnegative integer)
. datatype - datatype of each buffer element (handle)

Output Parameters:
. status - status object (Status)

.N fortran
@*/
int MPI_File_write_ordered(MPI_File mpi_fh, void *buf, int count, 
			   MPI_Datatype datatype, MPI_Status *status)
{
    int error_code, datatype_size, nprocs, myrank, incr;
    int source, dest, dummy;
    static char myname[] = "MPI_FILE_WRITE_ORDERED";
    ADIO_Offset shared_fp=0, new_shared_fp=0;
    ADIO_File fh;

    MPIU_THREAD_SINGLE_CS_ENTER("io");
    MPIR_Nest_incr();

    fh = MPIO_File_resolve(mpi_fh);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_FILE_HANDLE(fh, myname, error_code);
    MPIO_CHECK_COUNT(fh, count, myname, error_code);
    MPIO_CHECK_DATATYPE(fh, datatype, myname, error_code);
    /* --END ERROR HANDLING-- */

    MPI_Type_size(datatype, &datatype_size);

    /* --BEGIN ERROR HANDLING-- */
    MPIO_CHECK_INTEGRAL_ETYPE(fh, count, datatype_size, myname, error_code);
    /* --END ERROR HANDLING-- */

    ADIOI_TEST_DEFERRED(fh, myname, &error_code);

    MPI_Comm_size(fh->comm, &nprocs);
    MPI_Comm_rank(fh->comm, &myrank);

    incr = (count*datatype_size)/fh->etype_size;

    /* use the "ordered mode with RMA operations" algorithm outlined in the
     * shared file pointer paper: rank 0 gets its offset value from the RMA
     * window.  all other ranks know their offset after MPI_Scan */

    if (myrank == 0) {
	    ADIOI_MPIMUTEX_FP_Get(mpi_fh->fp_mutex, &shared_fp);
	    MPI_Scan(&shared_fp, &new_shared_fp, 1, MPI_INT, MPI_SUM, 
			    MPI_COMM_WORLD);
    } else {
	    MPI_Scan( &incr, &new_shared_fp, 1, MPI_INT, MPI_SUM, 
			    MPI_COMM_WORLD);
	    shared_fp = new_shared_fp;
    }
    if (myrank == nprocs - 1) {
	    ADIOI_MPIMUTEX_FP_Set(mpi_fh->fp_mutex, new_shared_fp + incr);
    }

    /* weak syncronization to prevent one process from racing ahead before rank
     * N-1 has updated shared fp value */
    MPI_Bcast(&dummy, 1, MPI_INT, nprocs -1, mpi_fh->comm);

#if 0
    /* Use a message as a 'token' to order the operations */
    source = myrank - 1;
    dest   = myrank + 1;
    if (source < 0) source = MPI_PROC_NULL;
    if (dest >= nprocs) dest = MPI_PROC_NULL;
    MPI_Recv(NULL, 0, MPI_BYTE, source, 0, fh->comm, MPI_STATUS_IGNORE);

    ADIO_Get_shared_fp(fh, incr, &shared_fp, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS) {
	error_code = MPIO_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL,
					  myname, __LINE__, MPI_ERR_INTERN, 
					  "**iosharedfailed", 0);
	error_code = MPIO_Err_return_file(fh, error_code);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPI_Send(NULL, 0, MPI_BYTE, dest, 0, fh->comm);
#endif

    ADIO_WriteStridedColl(fh, buf, count, datatype, ADIO_EXPLICIT_OFFSET,
                          shared_fp, status, &error_code);

    /* --BEGIN ERROR HANDLING-- */
    if (error_code != MPI_SUCCESS)
	error_code = MPIO_Err_return_file(fh, error_code);
    /* --END ERROR HANDLING-- */

fn_exit:
    MPIR_Nest_decr();
    MPIU_THREAD_SINGLE_CS_EXIT("io");

    /* FIXME: Check for error code from WriteStridedColl? */
    return error_code;
}

