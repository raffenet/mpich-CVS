/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_accept_connection()
{
    int bfd;
    int remote_rank;
    MPIDI_VC *vc_ptr;
    char ack;

    /* accept new connection */
    bfd = beasy_accept(TCP_Process.listener);
    if (bfd == BFD_INVALID_SOCKET)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_cq_test: beasy_accpet failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	return -1;
    }
    if (beasy_receive(bfd, (void*)&remote_rank, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_cq_test: beasy_receive(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	return -1;
    }
    vc_ptr = mm_vc_get(remote_rank);
    
    MPID_Thread_lock(vc_ptr->lock);

    if (vc_ptr->method == MM_UNBOUND_METHOD)
    {
	vc_ptr->method = MM_TCP_METHOD;
	vc_ptr->data.tcp.bfd = BFD_INVALID_SOCKET;
	vc_ptr->data.tcp.connected = FALSE;
	vc_ptr->data.tcp.connecting = TRUE;
	vc_ptr->post_read = tcp_post_read;
	vc_ptr->merge_post_read = tcp_merge_post_read;
	vc_ptr->post_write = tcp_post_write;

	/* send a keep acknowledgement */
	ack = TCP_ACCEPT_CONNECTION;
	beasy_send(bfd, &ack, 1);
	/* add the new connection to the read set */
	TCP_Process.max_bfd = BFD_MAX(bfd, TCP_Process.max_bfd);
	BFD_SET(bfd, &TCP_Process.readset);
	vc_ptr->read_next_ptr = TCP_Process.read_list;
	TCP_Process.read_list = vc_ptr;

	/* change the state of the vc to connected */
	vc_ptr->data.tcp.connected = TRUE;
	vc_ptr->data.tcp.connecting = FALSE;
	
	MPID_Thread_unlock(vc_ptr->lock);

	/* post the first packet read on the newly connected vc */
	mm_post_read_pkt(vc_ptr);
    }
    else
    {
	if (vc_ptr->method != MM_TCP_METHOD)
	{
	    err_printf("Error:tcp_accept_connection: vc is already connected with method %d\n", vc_ptr->method);
	    MPID_Thread_unlock(vc_ptr->lock);
	    return -1;
	}
	if (!vc_ptr->data.tcp.connecting || vc_ptr->data.tcp.connected)
	{
	    err_printf("Error:tcp_accept_connection: vc is already connected.\n");
	    MPID_Thread_unlock(vc_ptr->lock);
	    return -1;
	}
	if (remote_rank > MPIR_Process.comm_world->rank)
	{
	    /* close the old socket and keep the new */
	    if (BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.readset))
		BFD_CLR(vc_ptr->data.tcp.bfd, &TCP_Process.readset);
	    if (BFD_ISSET(vc_ptr->data.tcp.bfd, &TCP_Process.writeset))
		BFD_CLR(vc_ptr->data.tcp.bfd, &TCP_Process.writeset);
	    beasy_closesocket(vc_ptr->data.tcp.bfd);
	    vc_ptr->data.tcp.bfd = bfd;
	    /* add the new connection to the read set */
	    TCP_Process.max_bfd = BFD_MAX(bfd, TCP_Process.max_bfd);
	    BFD_SET(bfd, &TCP_Process.readset);
	    vc_ptr->read_next_ptr = TCP_Process.read_list;
	    TCP_Process.read_list = vc_ptr;
	}
	else
	{
	    /* close the new socket and keep the old */
	    beasy_closesocket(bfd);
	}
	/* change the state of the vc to connected */
	vc_ptr->data.tcp.connected = TRUE;
	vc_ptr->data.tcp.connecting = FALSE;
	
	MPID_Thread_unlock(vc_ptr->lock);
    }

    return MPI_SUCCESS;
}

int tcp_cq_test()
{
    int nready = 0;
    struct timeval tv;
    MPIDI_VC *vc_iter;
    bfd_set readset, writeset;
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    readset = TCP_Process.readset;
    writeset = TCP_Process.writeset;

    nready = bselect(TCP_Process.max_bfd, &readset, &writeset, NULL, &tv);

    if (nready == 0)
	return MPI_SUCCESS;

    vc_iter = TCP_Process.read_list;
    while (vc_iter)
    {
	if (BFD_ISSET(vc_iter->data.tcp.bfd, &readset))
	{
	    /* read data */
	    tcp_read(vc_iter);
	    nready--;
	}
	if (nready == 0)
	    return MPI_SUCCESS;
	vc_iter = vc_iter->read_next_ptr;
    }

    vc_iter = TCP_Process.write_list;
    while (vc_iter)
    {
	if (BFD_ISSET(vc_iter->data.tcp.bfd, &writeset))
	{
	    /* write data */
	    tcp_write(vc_iter);
	    nready--;
	}
	if (nready == 0)
	    return MPI_SUCCESS;
	vc_iter = vc_iter->write_next_ptr;
    }

    if (nready == 0)
	return MPI_SUCCESS;

    if (BFD_ISSET(TCP_Process.listener, &readset))
    {
	nready--;
	if (tcp_accept_connection() != MPI_SUCCESS)
	    return -1;
    }

    if (nready)
    {
	err_printf("Error: %d sockets still signalled after traversing read_list, write_list and listener.");
	/* return some error */
	return -1;
    }

    return MPI_SUCCESS;
}
