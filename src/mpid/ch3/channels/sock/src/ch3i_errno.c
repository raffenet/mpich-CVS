/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

int MPIDI_CH3I_sock_errno_to_mpi_errno(int sock_errno)
{
    int mpi_errno;
    
    switch(sock_errno)
    {
	case SOCK_EOF:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] active connection unexpectedly closed", 0 );
	    break;
	case SOCK_ERR_NOMEM:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] memory allocation failure", 0 );
	    break;
	case SOCK_ERR_HOST_LOOKUP:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] hostname lookup failed", 0 );
	    break;
	case SOCK_ERR_CONN_REFUSED:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] connection refused", 0 );
	    break;
	case SOCK_ERR_CONN_FAILED:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_OTHER, "[ch3:sock] active connection unexpectedly terminated", 0 );
	    break;
	case SOCK_ERR_BAD_SOCK:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_INTERN, "[ch3:sock] internal error - bad sock", 0 );
	    break;
	case SOCK_ERR_BAD_BUFFER:
	    mpi_errno = MPIR_Err_create_code( MPI_ERR_BUFFER, "[ch3:sock] bad user buffer", 0 );
	    break;
	default:
	    mpi_errno = MPI_ERR_OTHER;
	    break;
    }

    return mpi_errno;
}
