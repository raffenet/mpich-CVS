/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Open_port()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Open_port(char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_OPEN_PORT);

    mpi_errno = MPIDI_CH3I_Get_business_card(port_name, MPI_MAX_PORT_NAME, NULL);
    /* We pass NULL as the pg_ptr because the pg_ptr is not used in
       setting up the connection between an MPI_Connect and MPI_Accept */

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_OPEN_PORT);
    return mpi_errno;
}
