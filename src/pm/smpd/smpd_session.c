/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int handle_command(smpd_context *context)
{
    int result;

    smpd_dbg_printf("command read: '%s'\n", context->input_str);
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

int smpd_session(sock_set_t set, sock_t sock)
{
    int result = SMPD_SUCCESS;
    smpd_context *context;
    sock_event_t event;

    /* allocate and initialize a context */
    context = (smpd_context*)malloc(sizeof(smpd_context));
    if (context == NULL)
    {
	smpd_err_printf("malloc failed to allocate %d bytes for a context structure.\n", sizeof(smpd_context));
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }
    smpd_init_context(context, set, sock);

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
