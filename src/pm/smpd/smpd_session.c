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

    smpd_dbg_printf("command read: '%s'\n", context->input_str);
    result = smpd_command_destination(context, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination.\n");
	return SMPD_FAIL;
    }
    if (dest)
    {
	smpd_forward_command(context, dest);
    }
    if (strncmp(context->input_str, "close", 5) == 0)
    {
	smpd_dbg_printf("replying with close command.\n");
	strcpy(context->output_str, "close");
	result = smpd_package_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to package the close command.\n");
	}
	result = smpd_write_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the close command back to the sender.\n");
	}
	smpd_dbg_printf("close command written.\n");
	return SMPD_CLOSE;
    }
    else if (strncmp(context->input_str, "launch", 6) == 0)
    {
	smpd_dbg_printf("launch command, whahoo!\n");
    }
    else if (strncmp(context->input_str, "stat", 4) == 0)
    {
	strcpy(context->output_str, "all ok");
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
    else if (strncmp(context->input_str, "shutdown", 8) == 0)
    {
	smpd_dbg_printf("replying with close command.\n");
	strcpy(context->output_str, "close");
	result = smpd_package_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to package the close command.\n");
	}
	result = smpd_write_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the close command back to the sender.\n");
	}
	smpd_dbg_printf("close command written.\n");
	smpd_close_connection(context->set, context->sock);
	smpd_dbg_printf("shutting down.\n");
	exit(0);
    }
    else
    {
	smpd_err_printf("ignoring unknown session command: '%s'\n", context->input_str);
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

    /* allocate and initialize a context */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	smpd_err_printf("malloc failed to allocate %d bytes for a context structure.\n", sizeof(smpd_context_t));
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }
    smpd_init_context(context, set, sock);
    context->type = SMPD_CONTEXT_PARENT;
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
