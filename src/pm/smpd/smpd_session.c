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
    char host[SMPD_MAX_HOST_LENGTH];
    sock_set_t dest_set;
    sock_t dest_sock;
    int dest_id;
    smpd_command_t *cmd, *temp_cmd;

    smpd_dbg_printf("entering handle_command.\n");

    cmd = &context->read_cmd;

    smpd_dbg_printf("handle_command:\n");
    smpd_dbg_printf(" src  = %d\n", cmd->src);
    smpd_dbg_printf(" dest = %d\n", cmd->dest);
    smpd_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    smpd_dbg_printf(" str = %s\n", cmd->cmd);
    smpd_dbg_printf(" len = %d\n", cmd->length);
    if (context == smpd_process.left_context)
	smpd_dbg_printf(" context = left\n");
    else if (context == smpd_process.right_context)
	smpd_dbg_printf(" context = right\n");
    else if (context == smpd_process.parent_context)
	smpd_dbg_printf(" context = parent\n");
    else
	smpd_dbg_printf(" context = unknown\n");

    /* set the command state to handled */
    context->read_cmd.state = SMPD_CMD_HANDLED;

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination.\n");
	smpd_dbg_printf("exiting handle_command.\n");
	return SMPD_FAIL;
    }
    if (dest)
    {
	smpd_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to forward the command.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("exiting handle_command.\n");
	return SMPD_SUCCESS;
    }
    if (strcmp(cmd->cmd_str, "close") == 0)
    {
	smpd_process.closing = SMPD_TRUE;
	if (smpd_process.left_context || smpd_process.right_context)
	{
	    if (smpd_process.left_context)
	    {
		result = smpd_create_command("close", smpd_process.id, smpd_process.left_context->id, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a close command for the left context.\n");
		    smpd_dbg_printf("exiting handle_command.\n");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("sending close command to left child: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of a close command for the left context.\n");
		    smpd_dbg_printf("exiting handle_command.\n");
		    return SMPD_FAIL;
		}
	    }
	    if (smpd_process.right_context)
	    {
		result = smpd_create_command("close", smpd_process.id, smpd_process.right_context->id, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a close command for the right context.\n");
		    smpd_dbg_printf("exiting handle_command.\n");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("sending close command to right child: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of a close command for the right context.\n");
		    smpd_dbg_printf("exiting handle_command.\n");
		    return SMPD_FAIL;
		}
	    }
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_SUCCESS;
	}
	result = smpd_create_command("closed", smpd_process.id, context->id, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the parent context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed command to parent: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the parent context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed command.\n");
	smpd_dbg_printf("exiting handle_command.\n");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "closed") == 0)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_dbg_printf("received a closed command but I have no parent context to forward it to.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_SUCCESS;
	}
	if (context == smpd_process.left_context)
	{
	    smpd_dbg_printf("closed command received from left child, closing sock.\n");
	    sock_post_close(smpd_process.left_context->sock);
	    /*free(smpd_process.left_context);*/ /* can't free the context because it is used when sock_wait returns sock_op_close */
	    smpd_process.left_context = NULL;
	    if (smpd_process.right_context)
	    {
		smpd_dbg_printf("exiting handle_command.\n");
		return SMPD_SUCCESS;
	    }
	}
	else if (context == smpd_process.right_context)
	{
	    smpd_dbg_printf("closed command received from right child, closing sock.\n");
	    sock_post_close(smpd_process.right_context->sock);
	    /*free(smpd_process.right_context);*/ /* can't free the context because it is used when sock_wait returns sock_op_close */
	    smpd_process.right_context = NULL;
	    if (smpd_process.left_context)
	    {
		smpd_dbg_printf("exiting handle_command.\n");
		return SMPD_SUCCESS;
	    }
	}
	else
	{
	    smpd_err_printf("closed command received from non-child context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	result = smpd_create_command("closed", smpd_process.id, smpd_process.parent_context->id, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the parent context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed command to parent: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the parent context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed command to parent.\n");
    }
    else if (strcmp(cmd->cmd_str, "launch") == 0)
    {
	smpd_dbg_printf("launch command, whahoo!\n");
    }
    else if (strcmp(cmd->cmd_str, "stat") == 0)
    {
	result = smpd_create_command("all ok", smpd_process.id, cmd->src, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an 'all ok' command for the context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("replying to stat command: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the 'all ok' command to the context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
    }
    else if (strcmp(cmd->cmd_str, "shutdown") == 0)
    {
	result = smpd_create_command("down", smpd_process.id, context->id, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("shutdown received, replying with down command: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the context.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
    }
    else if (strcmp(cmd->cmd_str, "connect") == 0)
    {
	if (smpd_process.closing)
	{
	    smpd_err_printf("connect command received while session is closing, ignoring connect.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_SUCCESS;
	}
	if (!smpd_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH))
	{
	    smpd_err_printf("connect command does not have a target host argument: \"%s\"\n", cmd->cmd);
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	if (!smpd_get_int_arg(cmd->cmd, "id", &dest_id))
	{
	    smpd_err_printf("connect command does not have a target id argument: \"%s\"\n", cmd->cmd);
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	if (dest_id < smpd_process.id)
	{
	    smpd_dbg_printf("connecting to an smpd with an invalid id: %d\n", dest_id);
	}
	if (smpd_process.left_context != NULL && smpd_process.right_context != NULL)
	{
	    smpd_err_printf("unable to connect to a new session, left and right sessions already exist.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("now connecting to %s\n", host);
	/* create a new context */
	dest = (smpd_context_t*)malloc(sizeof(smpd_context_t));
	if (dest == NULL)
	{
	    smpd_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	dest_set = smpd_process.set;
	result = smpd_connect_to_smpd(smpd_process.parent_context->set, smpd_process.parent_context->sock,
	    host, SMPD_PROCESS_SESSION_STR, dest_id, &dest_set, &dest_sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect to %s\n", host);
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	if (dest_set != smpd_process.set)
	{
	    smpd_err_printf("connect_to_smpd returned a new set instead of adding the new sock to the provided set.\n");
	    smpd_err_printf("dest_set:%d != process_set:%d\n", sock_getsetid(dest_set), sock_getsetid(smpd_process.set));
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	if (smpd_process.left_context == NULL)
	{
	    smpd_dbg_printf("adding new left child context\n");
	    smpd_init_context(dest, SMPD_CONTEXT_LEFT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.left_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
	else if (smpd_process.right_context == NULL)
	{
	    smpd_dbg_printf("adding new right child context\n");
	    smpd_init_context(dest, SMPD_CONTEXT_RIGHT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.right_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
	else
	{
	    smpd_err_printf("impossible to be here, both left and right contexts are non-NULL.\n");
	    smpd_dbg_printf("exiting handle_command.\n");
	    return SMPD_FAIL;
	}
	/* post a read for the next command to come over the new context */
	result = smpd_post_read_command(dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on the newly connected context.\n");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("ignoring unknown session command: \"%s\"\n", cmd->cmd);
    }
    smpd_dbg_printf("exiting handle_command.\n");
    return SMPD_SUCCESS;
}

int smpd_interpret_session_header(char *str)
{
    char temp_str[100];

    smpd_dbg_printf("entering smpd_interpret_session_header.\n");

    smpd_dbg_printf("interpreting session header: \"%s\"\n", str);

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

    smpd_dbg_printf("exiting smpd_interpret_session_header.\n");
    return SMPD_SUCCESS;
}

int smpd_handle_read(smpd_context_t *context)
{
    int ret_val = SMPD_SUCCESS;

    smpd_dbg_printf("entering smpd_handle_read.\n");

    switch (context->read_cmd.state)
    {
    case SMPD_CMD_INVALID:
	smpd_err_printf("data read on a command in the invalid state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_HDR:
	context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
	if (context->read_cmd.length < 1)
	{
	    smpd_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
	    ret_val = SMPD_FAIL;
	    break;
	}
	smpd_dbg_printf("command header read, posting read for data: %d bytes\n", context->read_cmd.length);
	context->read_cmd.state = SMPD_CMD_READING_CMD;
	ret_val = sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, NULL);
	if (ret_val == SOCK_SUCCESS)
	    ret_val = SMPD_SUCCESS;
	else
	{
	    smpd_err_printf("unable to post a read for the command string, sock error:\n%s\n", get_sock_error_string(ret_val));
	    ret_val = SMPD_FAIL;
	}
	break;
    case SMPD_CMD_READING_CMD:
	ret_val = smpd_parse_command(&context->read_cmd);
	if (ret_val != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
	    break;
	}
	smpd_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
	context->read_cmd.state = SMPD_CMD_READY;
	ret_val = handle_command(context);
	if (ret_val == SMPD_SUCCESS)
	{
	    ret_val = smpd_post_read_command(context);
	}
	else if (ret_val == SMPD_CLOSE)
	{
	    smpd_dbg_printf("not posting read for another command because SMPD_CLOSE returned\n");
	    ret_val = SMPD_SUCCESS;
	}
	else
	{
	    smpd_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
	}
	break;
    case SMPD_CMD_WRITING_CMD:
	smpd_err_printf("data read on a command in the writing_cmd state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READY:
	smpd_err_printf("data read on a command in the ready state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_HANDLED:
	smpd_err_printf("data read on a command in the handled state\n");
	ret_val = SMPD_FAIL;
	break;
    default:
	smpd_err_printf("data read on a command in an invalid state: %d\n", context->read_cmd.state);
	ret_val = SMPD_FAIL;
	break;
    }
    smpd_dbg_printf("exiting smpd_handle_read.\n");
    return ret_val;
}

int smpd_handle_written(smpd_context_t *context)
{
    smpd_command_t *cmd;
    int ret_val = SMPD_SUCCESS;

    smpd_dbg_printf("entering smpd_handle_written.\n");

    if (context->write_list == NULL)
    {
	smpd_err_printf("data written on a context with no write command posted.\n");
	smpd_dbg_printf("exiting smpd_handle_written.\n");
	return SMPD_FAIL;
    }
    switch (context->write_list->state)
    {
    case SMPD_CMD_INVALID:
	smpd_err_printf("data written on a command in the invalid state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_HDR:
	smpd_err_printf("data written on a command in the reading_hdr state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_CMD:
	smpd_err_printf("data written on a command in the reading_cmd state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_WRITING_CMD:
	cmd = context->write_list;
	context->write_list = context->write_list->next;
	smpd_dbg_printf("command written: \"%s\"\n", cmd->cmd);
	if (strcmp(cmd->cmd_str, "closed") == 0)
	{
	    smpd_dbg_printf("closed command written, posting close of the sock.\n");
	    ret_val = sock_post_close(context->sock);
	    if (ret_val != SOCK_SUCCESS)
	    {
		smpd_err_printf("unable to post a close of the sock after writing a 'closed' command, sock error:\n%s\n",
		    get_sock_error_string(ret_val));
		smpd_free_command(cmd);
		ret_val = SMPD_FAIL;
		break;
	    }
	}
	else if (strcmp(cmd->cmd_str, "down") == 0)
	{
	    smpd_dbg_printf("down command written, exiting.\n");
	    smpd_close_connection(context->set, context->sock);
	    smpd_dbg_printf("shutting down.\n");
	    smpd_free_command(cmd);
	    smpd_exit(0);
	}
	ret_val = smpd_free_command(cmd);
	if (ret_val != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to free the written command.\n");
	    break;
	}
	cmd = context->write_list;
	if (cmd)
	{
	    cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
	    cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
	    cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
	    cmd->iov[1].SOCK_IOV_LEN = cmd->length;
	    smpd_dbg_printf("smpd_handle_written: posting write(%d bytes) for command: \"%s\"\n",
		cmd->iov[0].SOCK_IOV_LEN + cmd->iov[1].SOCK_IOV_LEN, cmd->cmd);
	    ret_val = sock_post_writev(context->sock, cmd->iov, 2, NULL);
	    if (ret_val != SOCK_SUCCESS)
	    {
		smpd_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(ret_val));
		ret_val = SMPD_FAIL;
	    }
	}
	break;
    case SMPD_CMD_READY:
	smpd_err_printf("data written on a command in the ready state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_HANDLED:
	smpd_err_printf("data written on a command in the handled state\n");
	ret_val = SMPD_FAIL;
	break;
    default:
	smpd_err_printf("data written on a command in an invalid state: %d\n", context->write_list->state);
	ret_val = SMPD_FAIL;
	break;
    }

    smpd_dbg_printf("exiting smpd_handle_written.\n");
    return ret_val;
}

int smpd_session(sock_set_t set, sock_t sock)
{
    int result = SMPD_SUCCESS;
    smpd_context_t *context;
    sock_event_t event;
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];

    smpd_dbg_printf("entering smpd_session.\n");

    /* read and interpret the session header */
    smpd_dbg_printf("reading and interpreting the session header on sock %d\n", sock_getid(sock));
    result = smpd_read_string(sock, session_header, SMPD_MAX_SESSION_HEADER_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the session header string.\n");
	smpd_close_connection(set, sock);
	smpd_dbg_printf("exiting smpd_session.\n");
	return SMPD_FAIL;
    }
    result = smpd_interpret_session_header(session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to interpret the session header string.\n");
	smpd_close_connection(set, sock);
	smpd_dbg_printf("exiting smpd_session.\n");
	return SMPD_FAIL;
    }

    /* the set used in this initial parent child connection will be the set used for the two
       child sessions also */
    smpd_dbg_printf("setting smpd_process.set to set: %d\n", sock_getsetid(set));
    smpd_process.set = set;

    /* allocate and initialize a context */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	smpd_err_printf("malloc failed to allocate %d bytes for a context structure.\n", sizeof(smpd_context_t));
	smpd_close_connection(set, sock);
	smpd_dbg_printf("exiting smpd_session.\n");
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
	smpd_dbg_printf("exiting smpd_session.\n");
	return SMPD_FAIL;
    }

    while (1)
    {
	smpd_dbg_printf("sock_waiting for the next event.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("session failure, sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    smpd_dbg_printf("exiting smpd_session.\n");
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
		smpd_dbg_printf("exiting smpd_session.\n");
		return SMPD_FAIL;
	    }
	    /*smpd_dbg_printf("calling smpd_handle_read.\n");*/
	    result = smpd_handle_read(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to handle read data.\n");
		smpd_close_connection(set, sock);
		smpd_dbg_printf("exiting smpd_session.\n");
		return SMPD_FAIL;
	    }
	    break;
	case SOCK_OP_WRITE:
	    /*smpd_dbg_printf("calling smpd_handle_written.\n");*/
	    result = smpd_handle_written(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to handle written data.\n");
		smpd_close_connection(set, sock);
		smpd_dbg_printf("exiting smpd_session.\n");
		return SMPD_FAIL;
	    }
	    break;
	case SOCK_OP_ACCEPT:
	    smpd_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    smpd_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    if (context == smpd_process.parent_context)
	    {
		smpd_dbg_printf("parent context closed.\n");
		free(context);
		smpd_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		    smpd_dbg_printf("exiting smpd_session.\n");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("exiting smpd_session.\n");
		return SMPD_SUCCESS;
	    }
	    else if (context == smpd_process.left_context)
	    {
		smpd_err_printf("op_close returned, freeing left context");
		free(context);
		smpd_process.left_context = NULL;
	    }
	    else if (context == smpd_process.right_context)
	    {
		smpd_err_printf("op_close returned, freeing right context");
		free(context);
		smpd_process.right_context = NULL;
	    }
	    else
	    {
		smpd_err_printf("op_close returned, freeing context");
		free(context);
	    }
	    break;
	default:
	    smpd_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    smpd_close_connection(set, sock);
    smpd_dbg_printf("exiting smpd_session.\n");
    return result;
}
