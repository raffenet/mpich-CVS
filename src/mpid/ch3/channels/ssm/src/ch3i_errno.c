/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

int MPIDI_CH3I_sock_errno_to_mpi_errno(int sock_errno, const char *fcname)
{
    int mpi_errno;
    
    switch(sock_errno)
    {
	case SOCK_EOF:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**connclose", 0 );
	    break;
	case SOCK_ERR_NOMEM:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    break;
	case SOCK_ERR_HOST_LOOKUP:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**hostlookup", 0 );
	    break;
	case SOCK_ERR_CONN_REFUSED:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**connrefused", 0 );
	    break;
	case SOCK_ERR_CONN_FAILED:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**connterm", 0 );
	    break;
	case SOCK_ERR_BAD_SOCK:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_INTERN, "**badsock", 0 );
	    break;
	case SOCK_ERR_BAD_BUFFER:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_BUFFER, "**buffer", 0 );
	    break;
	default:
	    mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, fcname, __LINE__, MPI_ERR_OTHER, "**failure", "**failure %d", sock_errno );
	    break;
    }

    return mpi_errno;
}
