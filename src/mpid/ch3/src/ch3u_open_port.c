/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

#ifdef MPIDI_DEV_IMPLEMENTS_OPEN_PORT

/*
 * MPIDI_Open_port()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_Open_port
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Open_port(char *port_name)
{
    int mpi_errno = MPI_SUCCESS;
    int len;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_OPEN_PORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_OPEN_PORT);

    len = MPI_MAX_PORT_NAME;
    mpi_errno = MPIU_Str_add_int_arg(&port_name, &len, MPIDI_CH3I_PORT_NAME_TAG_KEY, MPIDI_Process.port_name_tag);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", "**fail %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    MPIDI_Process.port_name_tag++;    
    mpi_errno = MPIDI_CH3I_Get_business_card(port_name, len);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_OPEN_PORT);
    return mpi_errno;
}

#endif
