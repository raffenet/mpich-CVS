/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "tcpimpl.h"

int tcp_post_connect(MPIDI_VC *vc_ptr, char *business_card)
{
    char host[100];
    int port;
    char *token;

    if (vc_ptr->data.tcp.connected)
	return MPI_SUCCESS;
    MPID_Thread_lock(vc_ptr->lock);
    if (vc_ptr->data.tcp.connected)
    {
	MPID_Thread_unlock(vc_ptr->lock);
	return MPI_SUCCESS;
    }
    if ((business_card == NULL) || (strlen(business_card) > 100))
    {
	err_printf("tcp_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    strcpy(host, business_card);
    token = strtok(host, ":");
    if (token == NULL)
    {
	err_printf("tcp_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	err_printf("tcp_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }
    port = atoi(token);

    if (beasy_create(&vc_ptr->data.tcp.bfd, ADDR_ANY, INADDR_ANY) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_create failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    if (beasy_connect(vc_ptr->data.tcp.bfd, host, port) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_connect failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    /* what do i send the other side? what context? what rank */
    if (beasy_send(vc_ptr->data.tcp.bfd, (void*)&MPIR_Process.comm_world->rank, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_send(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }
    if (beasy_send(vc_ptr->data.tcp.bfd, (void*)&MPIR_Process.comm_world->context_id, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_send(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    /* add the vc to the active read list */
    TCP_Process.max_bfd = BFD_MAX(vc_ptr->data.tcp.bfd, TCP_Process.max_bfd);
    BFD_SET(vc_ptr->data.tcp.bfd, &TCP_Process.readset);
    vc_ptr->read_next_ptr = TCP_Process.read_list;
    TCP_Process.read_list = vc_ptr;

    vc_ptr->data.tcp.connecting = TRUE;
    vc_ptr->data.tcp.reject_received = FALSE;
    MPID_Thread_unlock(vc_ptr->lock);
	
    return MPI_SUCCESS;
}
