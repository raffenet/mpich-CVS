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

    if (beasy_connect(vc_ptr->data.tcp.bfd, host, port) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_connect failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    if (beasy_send(vc_ptr->data.tcp.bfd, (void*)&vc_ptr->rank, sizeof(int)) == SOCKET_ERROR)
    {
	TCP_Process.error = beasy_getlasterror();
	beasy_error_to_string(TCP_Process.error, TCP_Process.err_msg, TCP_ERROR_MSG_LENGTH);
	err_printf("tcp_post_connect: beasy_send(rank) failed, error %d: %s\n", TCP_Process.error, TCP_Process.err_msg);
	MPID_Thread_unlock(vc_ptr->lock);
	return -1;
    }

    vc_ptr->data.tcp.connecting = TRUE;
    MPID_Thread_unlock(vc_ptr->lock);
	
    return MPI_SUCCESS;
}
