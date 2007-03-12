/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newtcp_module_impl.h"
#include <errno.h>

#define RECV_MAX_PKT_LEN 1024

static char *recv_buf;

#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(recv_buf, char*, RECV_MAX_PKT_LEN, mpi_errno, "NewTCP temporary buffer");
    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


int MPID_nem_newtcp_module_poll_finalize()
{
    return MPI_SUCCESS;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_recv_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_recv_handler (struct pollfd *pfd, sockconn_t *sc)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;

    CHECK_EINTR (bytes_recvd, recv (sc->fd, recv_buf, RECV_MAX_PKT_LEN, 0));
    if (bytes_recvd <= 0)
    {
	if (bytes_recvd == -1 && errno == EAGAIN) /* handle this fast */
	    goto fn_exit;

	if (bytes_recvd == 0)
	{
	    MPIU_ERR_SETANDJUMP (mpi_errno, MPI_ERR_OTHER, "**sock_closed");
	}
	else
	    MPIU_ERR_SETANDJUMP1 (mpi_errno, MPI_ERR_OTHER, "**read", "**read %s", strerror (errno));
    }
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "recv %d", bytes_recvd);
    mpi_errno = MPID_nem_handle_pkt(sc->vc, recv_buf, bytes_recvd);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newtcp_module_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newtcp_module_poll (MPID_nem_poll_dir_t in_or_out)
{
    int mpi_errno = MPI_SUCCESS;

#if 0
    mpi_errno = MPID_nem_newtcp_module_send_progress();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
#endif

    mpi_errno = MPID_nem_newtcp_module_connpoll();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
