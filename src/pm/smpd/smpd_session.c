/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_close_connection(sock_set_t set, sock_t sock)
{
    int result;
    sock_event_t event;

    /* close the sock and its set */
    result = sock_post_close(sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error closing socket: %s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error waiting for socket to close: %s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    if (event.op_type != SOCK_OP_CLOSE)
    {
	smpd_err_printf("incorrect sock operation returned: %d\n", event.op_type);
	return SMPD_FAIL;
    }
    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_session(sock_set_t set, sock_t sock)
{
    int result;
    char str[SMPD_MAX_CMD_LENGTH];

    /* do session stuff */
    result = smpd_read_string(set, sock, str, SMPD_MAX_CMD_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to read the session command.\n");
	smpd_close_connection(set, sock);
	return SMPD_FAIL;
    }
    smpd_dbg_printf("session read command: %s\n", str);

    smpd_close_connection(set, sock);
    return SMPD_SUCCESS;
}
