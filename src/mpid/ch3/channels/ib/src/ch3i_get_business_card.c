/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Get_business_card(char *value, int length)
{
#if 0
    int mpi_errno;
    int port;
    char host_description[MAX_HOST_DESCRIPTION_LEN];

    mpi_errno = MPIDU_Sock_get_host_description(host_description, MAX_HOST_DESCRIPTION_LEN);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**init_description");
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */

    port = MPIDI_CH3I_listener_port;
    mpi_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, port);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, host_description);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	if (mpi_errno == MPIU_STR_NOMEM) {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard_len");
	}
	else {
	    MPIU_ERR_SET(mpi_errno,MPI_ERR_OTHER, "**buscard");
	}
	return mpi_errno;
    }
    /* --END ERROR HANDLING-- */
#endif
    return MPI_SUCCESS;
}
