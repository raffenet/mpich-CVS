/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

int g_bDoConsole = 0;
char g_pszConsoleHost[SMPD_MAX_HOST_LENGTH];

int mp_parse_command_args(int *argcp, char **argvp[])
{
    /* check for console option */
    if (smpd_get_opt_string(argcp, argvp, "-console", g_pszConsoleHost, SMPD_MAX_HOST_LENGTH))
    {
	g_bDoConsole = 1;
    }
    if (smpd_get_opt(argcp, argvp, "-console"))
    {
	g_bDoConsole = 1;
	gethostname(g_pszConsoleHost, SMPD_MAX_HOST_LENGTH);
    }
    
    /* check for mpi options */

    return SMPD_SUCCESS;
}

int main(int argc, char* argv[])
{
    int result;
    int port = SMPD_LISTENER_PORT;

    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	return result;
    }

    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to parse the command arguments.\n");
	return result;
    }

    if (g_bDoConsole)
    {
	result = mp_console(g_pszConsoleHost);
    }
    else
    {
	/* do mpi job */
    }

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }
    return 0;
}

