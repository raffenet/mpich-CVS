/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

int mp_connect_next(int *parent_ptr, int *id_ptr)
{
    /* start assuming mpiexec is node 0 and we have connected to the first host, node 1 */
    static int parent = 1;
    static int id = 2;
    int bit, mask, temp;

    *parent_ptr = parent;
    *id_ptr = id;
    
    id++;

    temp = id >> 2;
    bit = 1;
    while (temp)
    {
	bit <<= 1;
	temp >>= 1;
    }
    mask = bit - 1;
    parent = bit | (id & mask);

    return SMPD_SUCCESS;
}

int mp_connect_tree(mp_host_node_t *node)
{
    int result;
    mp_host_node_t *iter;
    sock_set_t set;
    sock_t sock;
    sock_event_t event;
    smpd_context_t *context;
    int parent, id;
    smpd_command_t *cmd_ptr;

    if (node == NULL)
	return SMPD_FAIL;

    mp_enter_fn("mp_connect_tree");

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    /* create a process session with the first host, node 1 */
    set = SOCK_INVALID_SET;
    result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, node->host, SMPD_PROCESS_SESSION_STR, 1, &set, &sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to connect to smpd on %s\n", node->host);
	mp_exit_fn("mp_connect_tree");
	return result;
    }
    smpd_process.set = set;

    /* create a context for the session */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	mp_exit_fn("mp_connect_tree");
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_CHILD, set, sock, 1);
    strcpy(context->host, node->host);
    sock_set_user_ptr(sock, context);
    smpd_process.left_context = context;

    /* post a read for a possible incoming command */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
	    context->host, get_sock_error_string(result));
	mp_exit_fn("mp_connect_tree");
	return SMPD_FAIL;
    }

    iter = node->next;
    while (iter)
    {
	/* get the next parent and child ids */
	result = mp_connect_next(&parent, &id);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to get the next node id\n");
	    mp_exit_fn("mp_connect_tree");
	    return result;
	}

	/* create a connect command to be sent to the parent */
	result = smpd_create_command("connect", 0, parent, SMPD_TRUE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to create a connect command.\n");
	    mp_exit_fn("mp_connect_tree");
	    return result;
	}
	result = smpd_add_command_arg(cmd_ptr, "host", iter->host);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to add the host parameter to the connect command for host %s\n", iter->host);
	    mp_exit_fn("mp_connect_tree");
	    return result;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "id", id);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to add the id parameter to the connect command for host %s\n", iter->host);
	    mp_exit_fn("mp_connect_tree");
	    return result;
	}

	/* post a write of the command */
	result = smpd_post_write_command(context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to post a write of the connect command.\n");
	    mp_exit_fn("mp_connect_tree");
	    return result;
	}

	/* wait for the reply and handle any other commands */
	result = SMPD_SUCCESS;
	while (result == SMPD_SUCCESS)
	{
	    mp_dbg_printf("sock_waiting for next event.\n");
	    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
		smpd_close_connection(set, sock);
		mp_exit_fn("mp_connect_tree");
		return SMPD_FAIL;
	    }

	    switch (event.op_type)
	    {
	    case SOCK_OP_READ:
		result = handle_read(event.user_ptr, event.num_bytes, event.error, NULL);
		if (result == SMPD_CONNECTED)
		{
		    mp_dbg_printf("handle_read returned SMPD_CONNECTED\n");
		    break;
		}
		if (result != SMPD_SUCCESS)
		{
		    mp_err_printf("handle_read() failed.\n");
		}
		break;
	    case SOCK_OP_WRITE:
		result = handle_written(event.user_ptr, event.num_bytes, event.error);
		if (result != SMPD_SUCCESS)
		{
		    mp_err_printf("handle_written() failed.\n");
		}
		break;
	    case SOCK_OP_ACCEPT:
		mp_err_printf("unexpected accept event returned by sock_wait.\n");
		break;
	    case SOCK_OP_CONNECT:
		mp_err_printf("unexpected connect event returned by sock_wait.\n");
		break;
	    case SOCK_OP_CLOSE:
		mp_err_printf("unexpected close event returned by sock_wait.\n");
		free(context);
		mp_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    mp_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		}
		mp_exit_fn("mp_connect_tree");
		return SMPD_FAIL;
		break;
	    default:
		mp_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
		break;
	    }
	}
	if (result != SMPD_CONNECTED)
	{
	    mp_err_printf("unable to connect to host: %s\n", iter->host);
	    mp_exit_fn("mp_connect_tree");
	    return SMPD_FAIL;
	}
	iter = iter->next;
    }

    mp_exit_fn("mp_connect_tree");
    return SMPD_SUCCESS;
}
