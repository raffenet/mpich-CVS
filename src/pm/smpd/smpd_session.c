/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

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
