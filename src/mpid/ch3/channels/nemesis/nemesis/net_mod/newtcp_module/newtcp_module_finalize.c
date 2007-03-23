/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"

extern sockconn_t g_lstn_sc;

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    poke_msg_t msg;
    
    MPID_nem_newtcp_module_send_finalize();
    MPID_nem_newtcp_module_poll_finalize();

#if 0     
    if (g_lstn_sc.fd)
    {
        CHECK_EINTR (ret, close(g_lstn_sc.fd));
        MPIU_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**closesocket", "**closesocket %s %d", errno, strerror (errno));
    }
#endif

    msg.type = FINALIZE;
    count = send(MPID_nem_newtcp_module_main_to_comm_fd, &msg, sizeof(poke_msg_t), 0);
    /* FIXME: Return a proper error code, instead of asserting */
    MPI_Assert(count == sizeof(poke_msg_t));

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_tcp_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_ckpt_shutdown()
{
    return MPID_nem_newtcp_module_finalize();
}

