/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

int main(int argc, char* argv[])
{
    int result;
    sock_set_t set;
    sock_t sock;
    int port = SMPD_LISTENER_PORT;
    char host[SMPD_MAX_HOST_LENGTH];

    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    result = mp_parse_command_args(argc, argv);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to parse the command arguments.\n");
	return result;
    }

    gethostname(host, SMPD_MAX_HOST_LENGTH);

    result = mp_connect_to_smpd(host, SMPD_PROCESS_SESSION_STR, &set, &sock);
    /*result = mp_connect_to_smpd(host, SMPD_SMPD_SESSION_STR, &set, &sock);*/
    if (result)
    {
	mp_err_printf("Unable to connect to smpd on %s\n", host);
	return result;
    }

    result = mp_close_connection(set, sock);

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }
    return 0;
}

