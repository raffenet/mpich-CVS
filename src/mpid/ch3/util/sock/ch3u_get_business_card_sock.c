/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "mpidi_ch3_impl.h"
#include "pmi.h"


/* FIXME: All of the listener port routines should be in one place.
   It looks like this should be a socket utility function called by
   ch3_progress.c in sock and ssm (and essm if that is still valid), 
   as part of the progress init function.  Note that those progress
   engines also set the port to 0 when shutting down the progress engine,
   though it doesn't look like the port is closed. */

int MPIDI_CH3I_listener_port = 0;

/*  MPIDI_CH3U_Get_business_card_sock - does socket specific portion of getting a business card
 *     bc_val_p     - business card value buffer pointer, updated to the next available location or
 *                    freed if published.
 *     val_max_sz_p - ptr to maximum value buffer size reduced by the number of characters written
 *                               
 */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Get_business_card_sock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Get_business_card_sock(char **bc_val_p, int *val_max_sz_p)
{
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
    return MPI_SUCCESS;
}

/*
 * FIXME: This is old code that can be used to discover the available 
 * interfaces, and is left here for reference purposes only
 */
#if 0
    int32_t local_ip[MAX_NUM_NICS];
    unsigned int a, b, c, d;
    int num_nics, i;
    char *value_orig;
    struct hostent *h;
    int port;

    /*snprintf(value, length, "%s:%d", host, port);*/

    value_orig = value;
    num_nics = GetLocalIPs(local_ip, MAX_NUM_NICS);
    for (i=0; i<num_nics; i++)
    {
	a = (unsigned char)(((unsigned char *)(&local_ip[i]))[0]);
	b = (unsigned char)(((unsigned char *)(&local_ip[i]))[1]);
	c = (unsigned char)(((unsigned char *)(&local_ip[i]))[2]);
	d = (unsigned char)(((unsigned char *)(&local_ip[i]))[3]);

	if (a != 127)
	{
	    h = gethostbyaddr((const char *)&local_ip[i], sizeof(int), AF_INET);
	    if (h && h->h_name != NULL)
		value += MPIU_Snprintf(value, MPI_MAX_PORT_NAME, 
				 "%s:%u.%u.%u.%u:%d:", 
				 h->h_name, 
				 a, b, c, d,
				 port);
	    else
		value += MPIU_Snprintf(value, MPI_MAX_PORT_NAME, 
				 "%u.%u.%u.%u:%u.%u.%u.%u:%d:", 
				 a, b, c, d, 
				 a, b, c, d,
				 port);
	}
    }
#endif
