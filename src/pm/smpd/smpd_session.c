/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    char cmd[SMPD_MAX_CMD_STR_LENGTH];
    char host[SMPD_MAX_HOST_LENGTH];
    char int_str[12];
    char *str;
    int len;
    sock_set_t dest_set;
    sock_t dest_sock;
    int dest_id;

    smpd_dbg_printf("command read: <%s>\n", context->input_str);
    result = smpd_command_destination(context, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination.\n");
	return SMPD_FAIL;
    }
    if (dest)
    {
	smpd_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to forward the command.\n");
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }
    if (!smpd_get_string_arg(context->input_str, "cmd", cmd, SMPD_MAX_CMD_STR_LENGTH))
    {
	smpd_err_printf("no command specified in the command string: <%s>\n", context->input_str);
	return SMPD_FAIL;
    }
    if (strcmp(cmd, "close") == 0)
    {
	smpd_process.closing = SMPD_TRUE;
	if (smpd_process.left_context || smpd_process.right_context)
	{
	    /*smpd_err_printf("close command received while left child %d and right child %d still connected.\n",
		smpd_process.left_context->id, smpd_process.right_context->id);*/
	    if (smpd_process.left_context)
	    {
		str = smpd_process.left_context->output_str;
		len = SMPD_MAX_CMD_LENGTH;
		result = smpd_add_string_arg(&str, &len, "cmd", "close");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create the left context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_add_int_arg(&str, &len, "src", smpd_process.id);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the src to the left context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_add_int_arg(&str, &len, "dest", smpd_process.left_context->id);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dest to the left context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_package_command(smpd_process.left_context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to package the left context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_write_command(smpd_process.left_context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to write the left context close command.\n");
		    return SMPD_FAIL;
		}
	    }
	    if (smpd_process.right_context)
	    {
		str = smpd_process.right_context->output_str;
		len = SMPD_MAX_CMD_LENGTH;
		result = smpd_add_string_arg(&str, &len, "cmd", "close");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create the right context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_add_int_arg(&str, &len, "src", smpd_process.id);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the src to the right context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_add_int_arg(&str, &len, "dest", smpd_process.right_context->id);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dest to the right context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_package_command(smpd_process.right_context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to package the right context close command.\n");
		    return SMPD_FAIL;
		}
		result = smpd_write_command(smpd_process.right_context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to write the right context close command.\n");
		    return SMPD_FAIL;
		}
	    }
	    return SMPD_SUCCESS;
	}
	str = context->output_str;
	len = SMPD_MAX_CMD_LENGTH;
	result = smpd_add_string_arg(&str, &len, "cmd", "closed");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to add the command 'closed' to the command string.\n");
	    return SMPD_FAIL;
	}
	result = smpd_add_int_arg(&str, &len, "src", smpd_process.id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the src flag to the command.\n");
	    return SMPD_FAIL;
	}
	result = smpd_add_int_arg(&str, &len, "dest", context->id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the destination flag to the command.\n");
	    return SMPD_FAIL;
	}
	result = smpd_package_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to package the closed command.\n");
	    return SMPD_FAIL;
	}
	result = smpd_write_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the closed command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("closed command written.\n");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd, "closed") == 0)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_dbg_printf("received a closed command but I have no parent context to forward it to.\n");
	    return SMPD_SUCCESS;
	}
	if (context == smpd_process.left_context)
	{
	    sock_post_close(smpd_process.left_context->sock);
	    /*free(smpd_process.left_context);*/ /* can't free the context because it is used when sock_wait returns sock_op_close */
	    smpd_process.left_context = NULL;
	    if (smpd_process.right_context)
		return SMPD_SUCCESS;
	}
	else if (context == smpd_process.right_context)
	{
	    sock_post_close(smpd_process.right_context->sock);
	    /*free(smpd_process.right_context);*/ /* can't free the context because it is used when sock_wait returns sock_op_close */
	    smpd_process.right_context = NULL;
	    if (smpd_process.left_context)
		return SMPD_SUCCESS;
	}
	else
	{
	    smpd_err_printf("closed command received from non-child context.\n");
	    return SMPD_FAIL;
	}
	str = smpd_process.parent_context->output_str;
	len = SMPD_MAX_CMD_LENGTH;
	result = smpd_add_string_arg(&str, &len, "cmd", "closed");
	result = smpd_add_int_arg(&str, &len, "src", smpd_process.id);
	result = smpd_add_int_arg(&str, &len, "dest", smpd_process.parent_context->id);
	result = smpd_package_command(smpd_process.parent_context);
	result = smpd_write_command(smpd_process.parent_context);
    }
    else if (strcmp(cmd, "launch") == 0)
    {
	smpd_dbg_printf("launch command, whahoo!\n");
    }
    else if (strcmp(cmd, "stat") == 0)
    {
	str = context->output_str;
	len = SMPD_MAX_CMD_LENGTH;
	result = smpd_add_string_arg(&str, &len, "cmd", "all ok");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to add the command name to the command string.\n");
	}
	result = smpd_add_int_arg(&str, &len, "dest", context->id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the destination flag to the command.\n");
	}
	result = smpd_package_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to package the stat command.\n");
	}
	result = smpd_write_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the stat command back to the sender.\n");
	}
    }
    else if (strcmp(cmd, "shutdown") == 0)
    {
	smpd_dbg_printf("replying with closed command.\n");
	str = context->output_str;
	len = SMPD_MAX_CMD_LENGTH;
	result = smpd_add_string_arg(&str, &len, "cmd", "closed");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to add the command name to the command string.\n");
	}
	result = smpd_add_int_arg(&str, &len, "dest", context->id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the destination flag to the command.\n");
	}
	result = smpd_package_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to package the closed command.\n");
	}
	result = smpd_write_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the closed command back to the sender.\n");
	}
	smpd_dbg_printf("closed command written.\n");
	smpd_close_connection(context->set, context->sock);
	smpd_dbg_printf("shutting down.\n");
	smpd_exit(0);
    }
    else if (strcmp(cmd, "connect") == 0)
    {
	if (smpd_process.closing)
	{
	    smpd_err_printf("connect command received while session is closing, ignoring.\n");
	    return SMPD_SUCCESS;
	}
	if (!smpd_get_string_arg(context->input_str, "host", host, SMPD_MAX_HOST_LENGTH))
	{
	    smpd_err_printf("connect command does not have a target host argument: <%s>\n", context->input_str);
	    return SMPD_FAIL;
	}
	if (!smpd_get_string_arg(context->input_str, "id", int_str, 12))
	{
	    smpd_err_printf("connect command does not have a target id argument: <%s>\n", context->input_str);
	    return SMPD_FAIL;
	}
	dest_id = atoi(int_str);
	if (dest_id < smpd_process.id)
	{
	    smpd_dbg_printf("connecting to an smpd with an invalid id: %d\n", dest_id);
	}
	if (smpd_process.left_context != NULL && smpd_process.right_context != NULL)
	{
	    smpd_err_printf("unable to connect to a new session, left and right sessions already exist.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("now connecting to %s\n", host);
	/* create a new context */
	dest = (smpd_context_t*)malloc(sizeof(smpd_context_t));
	if (dest == NULL)
	{
	    smpd_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	    return SMPD_FAIL;
	}
	dest_set = smpd_process.set;
	result = smpd_connect_to_smpd(smpd_process.parent_context->set, smpd_process.parent_context->sock,
	    host, SMPD_PROCESS_SESSION_STR, dest_id, &dest_set, &dest_sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect to %s\n", host);
	    return SMPD_FAIL;
	}
	if (smpd_process.left_context == NULL)
	{
	    smpd_init_context(dest, SMPD_CONTEXT_LEFT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.left_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
	else if (smpd_process.right_context == NULL)
	{
	    smpd_init_context(dest, SMPD_CONTEXT_RIGHT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.right_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
    }
    else
    {
	smpd_err_printf("ignoring unknown session command: <%s>\n", context->input_str);
    }
    return SMPD_SUCCESS;
}

int smpd_interpret_session_header(char *str)
{
    char temp_str[100];

    smpd_dbg_printf("interpreting session header: <%s>\n", str);

    /* get my id */
    if (smpd_get_string_arg(str, "id", temp_str, 100))
    {
	smpd_dbg_printf(" id = %s\n", temp_str);
	smpd_process.id = atoi(temp_str);
	if (smpd_process.id < 0)
	{
	    smpd_err_printf("invalid id passed in session header: %d\n", smpd_process.id);
	    smpd_process.id = 0;
	}
    }

    /* get my parent's id */
    if (smpd_get_string_arg(str, "parent", temp_str, 100))
    {
	smpd_dbg_printf(" parent = %s\n", temp_str);
	smpd_process.parent_id = atoi(temp_str);
	if (smpd_process.parent_id < 0)
	{
	    smpd_err_printf("invalid parent id passed in session header: %d\n", smpd_process.parent_id);
	    smpd_process.parent_id = -1;
	}
    }

    /* get my level */
    if (smpd_get_string_arg(str, "level", temp_str, 100))
    {
	smpd_dbg_printf(" level = %s\n", temp_str);
	smpd_process.level = atoi(temp_str);
	if (smpd_process.level < 0)
	{
	    smpd_err_printf("invalid session level passed in session header: %d\n", smpd_process.level);
	    smpd_process.level = 0;
	}
    }

    /*
    smpd_dbg_printf("from session header:\n myid = %d\n mylevel = %d\n parent = %d\n",
	smpd_process.id, smpd_process.level, smpd_process.parent_id);
    */
    return SMPD_SUCCESS;
}

int smpd_session(sock_set_t set, sock_t sock)
{
    int result = SMPD_SUCCESS;
    smpd_context_t *context;
    sock_event_t event;
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];

    /* read and interpret the session header */
    smpd_dbg_printf("reading and interpreting the session header on sock %d\n", sock_getid(sock));
    result = smpd_read_string(set, sock, session_header, SMPD_MAX_SESSION_HEADER_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the session header string.\n");
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }
    result = smpd_interpret_session_header(session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to interpret the session header string.\n");
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }

    /* the set used in this initial parent child connection will be the set used for the two
       child sessions also */
    smpd_process.set = set;

    /* allocate and initialize a context */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	smpd_err_printf("malloc failed to allocate %d bytes for a context structure.\n", sizeof(smpd_context_t));
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_PARENT, set, sock, smpd_process.parent_id);
    smpd_process.parent_context = context;
    sock_set_user_ptr(sock, context);

    /* post a read for the first command header */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the first command header of the session.\n");
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }

    while (1)
    {
	smpd_dbg_printf("sock_waiting for the next command.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("session failure, sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}
	context = (smpd_context_t*)event.user_ptr;
	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    /* check for an error */
	    if (event.error != SOCK_SUCCESS)
	    {
		smpd_err_printf("read failure, sock error:\n%s\n", get_sock_error_string(result));
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    /* read the command */
	    result = smpd_read_command(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to read the command.\n");
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    /* handle the command */
	    result = handle_command(context);
	    if (result == SMPD_CLOSE)
	    {
		smpd_dbg_printf("closing the session.\n");
		smpd_close_connection(set, sock);
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to handle the command:\n'%s'\n", context->input_str);
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    /* post a read for the next command header */
	    result = smpd_post_read_command(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a read for the next command header.\n");
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    break;
	case SOCK_OP_WRITE:
	    smpd_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_ACCEPT:
	    smpd_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    smpd_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    smpd_err_printf("unexpected close event returned by sock_wait.\n");
	    break;
	default:
	    smpd_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    smpd_close_connection(set, sock);
    return result;
}
