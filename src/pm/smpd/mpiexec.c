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
mp_host_node_t *g_host_list = NULL;

int mp_parse_command_args(int *argcp, char **argvp[])
{
    mp_host_node_t *node;
    char host[MP_MAX_HOST_LENGTH];
    int found;
    int cur_rank;
    int n, parent, id;
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;

    mp_enter_fn("mp_parse_command_args");

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
    if (smpd_get_opt(argcp, argvp, "-p"))
    {
	g_bUseProcessSession = 1;
    }
    if (smpd_get_opt_int(argcp, argvp, "-tree", &n))
    {
	while (n)
	{
	    mp_connect_next(&parent, &id);
	    printf("connect: %d -> %d\n", parent, id);
	    n--;
	}
	exit(0);
    }
    
    /* check for mpi options */
    /*
     * Required:
     * -n <maxprocs>
     * -host <hostname>
     * -soft <Fortran90 triple> - represents allowed number of processes up to maxprocs
     *        a or a:b or a:b:c where
     *        1) a = a
     *        2) a:b = a, a+1, a+2, ..., b
     *        3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc
     *           where a+kc <= b if c>0
     *                 a+kc >= b if c<0
     * -wdir <working directory>
     * -path <search path for executable>
     * -arch <architecture> - sun, linux, rs6000, ...
     * -file <filename> - each line contains a complete set of mpiexec options, #commented
     *
     * Extensions:
     * -env <variable=value>
     * -env <variable=value;variable2=value2;...>
     * -hosts <n host1 host2 ... hostn>
     * -hosts <n host1 m1 host2 m2 ... hostn mn>
     * -machinefile <filename> - one host per line, #commented
     * -localonly <numprocs>
     * -nompi - don't require processes to be MPI processes (don't have to call MPI_Init or PMI_Init)
     * -exitcodes - print the exit codes of processes as they exit
     * 
     * Windows extensions:
     * -map <drive:\\host\share>
     * -pwdfile <filename> - account on the first line and password on the second
     * -nomapping - don't copy the current directory mapping on the remote nodes
     * -dbg - debug
     * -noprompt - don't prompt for user credentials, fail with an error message
     * -logon - force the prompt for user credentials
     * -priority <class[:level]> - set the process startup priority class and optionally level.
     *            class = 0,1,2,3,4   = idle, below, normal, above, high
     *            level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest
     * -localroot - launch the root process without smpd if the host is local.
     *              (This allows the root process to create windows and be debugged.)
     *
     * Backwards compatibility
     * -np <numprocs>
     * -dir <working directory>
     */

    cur_rank = 1;
    next_argc = *argcp;
    next_argv = *argvp + 1;
    exe_ptr = **argvp;
    do
    {
	/* calculate the current argc and find the next argv */
	argc = 1;
	while ( (*next_argv) != NULL && (**next_argv) != ':')
	{
	    argc++;
	    next_argc--;
	    next_argv++;
	}
	if ( (*next_argv) != NULL && (**next_argv) == ':')
	{
	    (*next_argv) = NULL;
	    next_argv++;
	}
	argcp = &argc;

	/* parse the current block */
	while (smpd_get_opt_string(argcp, argvp, "-host", host, MP_MAX_HOST_LENGTH))
	{
	    node = g_host_list;
	    found = 0;
	    while (node)
	    {
		if (strcmp(node->host, host) == 0)
		{
		    found = 1;
		    break;
		}
		if (node->next == NULL)
		    break;
		node = node->next;
	    }
	    if (!found)
	    {
		if (node != NULL)
		{
		    node->next = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
		    node = node->next;
		}
		else
		{
		    node = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
		    g_host_list = node;
		}
		if (node == NULL)
		{
		    mp_err_printf("malloc failed to allocate a host node structure of size %d\n", sizeof(mp_host_node_t));
		    mp_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(node->host, host);
		node->ip_str[0] = '\0';
		node->id = cur_rank;
		cur_rank++;
		node->next = NULL;
	    }
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    mp_exit_fn("mp_parse_command_args");
    return SMPD_SUCCESS;
}

int main(int argc, char* argv[])
{
    int result;
    int port = SMPD_LISTENER_PORT;
    smpd_command_t *cmd_ptr;
    sock_event_t event;

    mp_enter_fn("main");

    /* initialize */
    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	mp_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_init_process failed.\n");
	mp_exit_fn("main");
	return result;
    }

    /* parse the command line */
    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to parse the command arguments.\n");
	mp_exit_fn("main");
	return result;
    }

    /* handle a console session or a job session */
    if (g_bDoConsole)
    {
	/* do a console session */

	result = mp_console(g_pszConsoleHost);
    }
    else
    {
	/* do mpi job */

	/* connect to all the hosts in the job */
	result = mp_connect_tree(g_host_list);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to connect all the hosts in the job.\n");
	    goto quit_job;
	}

	/* initialize the pmi database engine on the root node */

	/* create the list of processes to launch */

	/* launch the processes */

	/* wait for them all to exit */

	/* close the tree */
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to create the close command to tear down the job tree.\n");
	    goto quit_job;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to post a write of the close command to tear down the job tree.\n");
	    goto quit_job;
	}
	result = SMPD_SUCCESS;
	while (result == SMPD_SUCCESS)
	{
	    mp_dbg_printf("sock_waiting for next event.\n");
	    result = sock_wait(smpd_process.set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
		smpd_close_connection(smpd_process.set, smpd_process.left_context->sock);
		goto quit_job;
	    }

	    switch (event.op_type)
	    {
	    case SOCK_OP_READ:
		result = handle_read(event.user_ptr, event.num_bytes, event.error, NULL);
		if (result == SMPD_CLOSE)
		{
		    mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
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
		free(smpd_process.left_context);
		mp_dbg_printf("closing the session.\n");
		result = sock_destroy_set(smpd_process.set);
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
    }

quit_job: /* use a goto label to avoid deep indenting in the above code */

    /* finalize */
    mp_dbg_printf("calling sock_finalize\n");
    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }

#ifdef HAVE_WINDOWS_H
    if (g_hCloseStdinThreadEvent)
	SetEvent(g_hCloseStdinThreadEvent);
    if (g_hStdinThread != NULL)
    {
	WaitForSingleObject(g_hStdinThread, 3000);
	CloseHandle(g_hStdinThread);
    }
    if (g_hCloseStdinThreadEvent)
	CloseHandle(g_hCloseStdinThreadEvent);
#endif
    mp_exit_fn("main");
    smpd_exit(0);
}

