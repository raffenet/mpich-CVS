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
    smpd_host_node_t *host_node_ptr;
    smpd_launch_node_t *launch_node_ptr;
    smpd_context_t *context;
    sock_set_t set;
    sock_t sock;
    smpd_state_t state;

    smpd_enter_fn("main");

    /* catch an empty command line */
    if (argc < 2)
    {
	mp_print_options();
	exit(0);
    }

    /* initialize */
    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_init failed,\nsock error: %s\n",
		      get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	goto quit_job;
    }

    /* parse the command line */
    smpd_dbg_printf("parsing the command line.\n");
    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the command arguments.\n");
	goto quit_job;
    }

    /* print and see what we've got */
    smpd_dbg_printf("host tree:\n");
    host_node_ptr = smpd_process.host_list;
    if (!host_node_ptr)
	smpd_dbg_printf("<none>\n");
    while (host_node_ptr)
    {
	smpd_dbg_printf(" host: %s, parent: %d, id: %d\n",
	    host_node_ptr->host,
	    host_node_ptr->parent, host_node_ptr->id);
	host_node_ptr = host_node_ptr->next;
    }
    smpd_dbg_printf("launch nodes:\n");
    launch_node_ptr = smpd_process.launch_list;
    if (!launch_node_ptr)
	smpd_dbg_printf("<none>\n");
    while (launch_node_ptr)
    {
	smpd_dbg_printf(" iproc: %d, id: %d, exe: %s\n",
	    launch_node_ptr->iproc, launch_node_ptr->host_id,
	    launch_node_ptr->exe);
	launch_node_ptr = launch_node_ptr->next;
    }

    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	goto quit_job;
    }
    smpd_process.set = set;

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;
    /* set the state to create a console session or a job session */
    state = smpd_process.do_console ? SMPD_MPIEXEC_CONNECTING_SMPD : SMPD_MPIEXEC_CONNECTING_TREE;

    /* start connecting the tree by posting a connect to the first host */
    result = sock_post_connect(set, NULL, smpd_process.host_list->host, smpd_process.port, &sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("Unable to connect to '%s:%d',\nsock error: %s\n",
	    smpd_process.host_list->host, smpd_process.port, get_sock_error_string(result));
	goto quit_job;
    }
    result = smpd_create_context(SMPD_CONTEXT_LEFT_CHILD, set, sock, 1, &context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the first host in the tree.\n");
	goto quit_job;
    }
    context->state = state;
    context->connect_to = smpd_process.host_list;
    smpd_process.left_context = context;
    result = sock_set_user_ptr(sock, context);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to set the smpd sock user pointer,\nsock error: %s\n",
	    get_sock_error_string(result));
	goto quit_job;
    }
    result = smpd_enter_at_state(set, state);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
	goto quit_job;
    }

quit_job:

    /* finalize */
    smpd_dbg_printf("calling sock_finalize\n");
    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
	SetEvent(smpd_process.hCloseStdinThreadEvent);
    if (smpd_process.hStdinThread != NULL)
    {
	if (WaitForSingleObject(smpd_process.hStdinThread, 3000) != WAIT_OBJECT_0)
	{
	    TerminateThread(smpd_process.hStdinThread, 321);
	}
	CloseHandle(smpd_process.hStdinThread);
    }
    if (smpd_process.hCloseStdinThreadEvent)
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
#endif
    smpd_exit_fn("main");
    smpd_exit(0);
}

