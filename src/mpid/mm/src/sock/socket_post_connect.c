/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "socketimpl.h"

int socket_post_connect(MPIDI_VC *vc_ptr, char *business_card)
{
    char host[100];
    int port;
    char *token;
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_POST_CONNECT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_POST_CONNECT);

    MPIU_dbg_printf("socket_post_connect\n");

    MPID_Thread_lock(vc_ptr->lock);
    if (vc_ptr->data.socket.state & SOCKET_CONNECTED)
    {
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return MPI_SUCCESS;
    }
    if ((business_card == NULL) || (strlen(business_card) > 100))
    {
	err_printf("socket_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }

    strcpy(host, business_card);
    token = strtok(host, ":");
    if (token == NULL)
    {
	err_printf("socket_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }
    token = strtok(NULL, ":");
    if (token == NULL)
    {
	err_printf("socket_post_connect: invalid business card\n");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }
    port = atoi(token);

    SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTING);
    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_POSTING_CONNECT);

    if ((error = sock_post_connect(SOCKET_Process.set, vc_ptr, host, port, &vc_ptr->data.socket.sock)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_post_connect: sock_post_connect failed.");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
	return -1;
    }

    MPID_Thread_unlock(vc_ptr->lock);

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_POST_CONNECT);
    return MPI_SUCCESS;
}

int socket_handle_connect(MPIDI_VC *vc_ptr)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_CONNECT);

    MPID_Thread_lock(vc_ptr->lock);

    MPIU_dbg_printf("socket_handle_connect - %d\n", sock_getid(vc_ptr->data.socket.sock));

    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_POSTING_CONNECT);
    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_CONNECT_PKT);

    MPIU_dbg_printf("sock_post_write(%d:MPID_Connect_pkt,%d)\n", sock_getid(vc_ptr->data.socket.sock), MPIR_Process.comm_world->rank);
    vc_ptr->pkt_car.msg_header.pkt.u.connect.rank = MPIR_Process.comm_world->rank;
    vc_ptr->pkt_car.msg_header.pkt.u.connect.context = MPIR_Process.comm_world->context_id;
    if ((error = sock_post_write(vc_ptr->data.socket.sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect, sizeof(MPID_Connect_pkt), NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_post_write(MPID_Connect_pkt) failed.");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
	return -1;
    }

    MPID_Thread_unlock(vc_ptr->lock);
    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
    return MPI_SUCCESS;
}

int socket_handle_written_ack(MPIDI_VC *vc_ptr, int num_written)
{
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);

    MPIU_dbg_printf("socket_handle_written_ack(%d ???) - %d bytes\n", sock_getid(vc_ptr->data.socket.sock), num_written);

    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_ACK);
    SOCKET_CLR_BIT(vc_ptr->data.socket.state, SOCKET_ACCEPTING);

    switch (vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_out)
    {
    case SOCKET_ACCEPT_CONNECTION:
	SOCKET_SET_BIT(vc_ptr->data.socket.state, SOCKET_CONNECTED);
	socket_post_read_pkt(vc_ptr);
	socket_write_aggressive(vc_ptr);
	break;
    case SOCKET_REJECT_CONNECTION:
	break;
    default:
	break;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_ACK);
    return MPI_SUCCESS;
}

int socket_handle_written_connect_pkt(MPIDI_VC *vc_ptr, int num_written)
{
    int error;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONNECT_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONNECT_PKT);

    MPIU_dbg_printf("socket_handle_written_connect_pkt(%d) - %d bytes of %d\n", sock_getid(vc_ptr->data.socket.sock), num_written, sizeof(MPID_Connect_pkt));

    SOCKET_CLR_BIT(vc_ptr->data.socket.connect_state, SOCKET_WRITING_CONNECT_PKT);
    SOCKET_SET_BIT(vc_ptr->data.socket.connect_state, SOCKET_READING_ACK);

    /* post a read of the ack */
    MPIU_dbg_printf("sock_post_read(%d:ack)\n", sock_getid(vc_ptr->data.socket.sock));
    if ((error = sock_post_read(vc_ptr->data.socket.sock, (void*)&vc_ptr->pkt_car.msg_header.pkt.u.connect.ack_in, 1, NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_post_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONNECT_PKT);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_WRITTEN_CONNECT_PKT);
    return MPI_SUCCESS;
}

#if 0
int socket_handle_connect(MPIDI_VC *vc_ptr)
{
    int error;
    int num_written;
    MPID_Connect_pkt connect;
    MPIDI_STATE_DECL(MPID_STATE_SOCKET_HANDLE_CONNECT);

    MPID_Thread_lock(vc_ptr->lock);

    MPIU_dbg_printf("socket_handle_connect - %d\n", sock_getid(vc_ptr->data.socket.sock));

    MPIU_dbg_printf("sock_easy_send(%d:MPID_Connect_pkt,%d)\n", sock_getid(vc_ptr->data.socket.sock), MPIR_Process.comm_world->rank);
    connect.rank = MPIR_Process.comm_world->rank;
    connect.context = MPIR_Process.comm_world->context_id;
    if ((error = sock_easy_send(vc_ptr->data.socket.sock, (void*)&connect, sizeof(MPID_Connect_pkt), &num_written)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_easy_send(MPID_Connect_pkt) failed.");
	MPID_Thread_unlock(vc_ptr->lock);
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
	return -1;
    }

    MPID_Thread_unlock(vc_ptr->lock);

    /* post a read of the ack */
    MPIU_dbg_printf("sock_post_read(%d:ack)\n", sock_getid(vc_ptr->data.socket.sock));
    if ((error = sock_post_read(vc_ptr->data.socket.sock, &vc_ptr->pkt_car.msg_header.pkt.u.ack.ack, 1, NULL)) != SOCK_SUCCESS)
    {
	socket_print_sock_error(error, "socket_handle_connect: sock_post_read failed.");
	MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
	return -1;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_SOCKET_HANDLE_CONNECT);
    return MPI_SUCCESS;
}
#endif
