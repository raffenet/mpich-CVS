/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

mp_process_t mp_process =
{
    SMPD_FALSE,  /* do_console            */
    "",          /* console_host          */
    NULL,        /* host_list             */
    NULL,        /* launch_list           */
    SMPD_TRUE,   /* credentials_prompt    */
    SMPD_TRUE,   /* do_multi_color_output */
    SMPD_FALSE,  /* no_mpi                */
    SMPD_FALSE,  /* output_exit_codes     */
    SMPD_FALSE,  /* local_root            */
    SMPD_FALSE,  /* use_iproot            */
    SMPD_FALSE   /* use_process_session   */
};

int main(int argc, char* argv[])
{
    int result;
    int port = SMPD_LISTENER_PORT;
    smpd_command_t *cmd_ptr;
    sock_event_t event;
    /*
    mp_host_node_t *host_node_ptr;
    mp_launch_node_t *launch_node_ptr;
    */

    mp_enter_fn("main");

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
    if (mp_process.do_console)
    {
	/* do a console session */

	result = mp_console(mp_process.console_host);
    }
    else
    {
	/* do an mpi job */

#if 0
	/* print and see what we've got */
	printf("host tree:\n");
	host_node_ptr = mp_process.host_list;
	while (host_node_ptr)
	{
	    printf(" host: %s, parent: %d, id: %d\n", host_node_ptr->host, host_node_ptr->parent, host_node_ptr->id);
	    host_node_ptr = host_node_ptr->next;
	}
	printf("launch nodes:\n");
	launch_node_ptr = mp_process.launch_list;
	while (launch_node_ptr)
	{
	    printf(" iproc: %d, id: %d, exe: %s\n",
		launch_node_ptr->iproc, launch_node_ptr->host_id, launch_node_ptr->exe);
	    launch_node_ptr = launch_node_ptr->next;
	}
	fflush(stdout);
	goto quit_job;
#endif

	/* connect to all the hosts in the job */
	result = mp_connect_tree(mp_process.host_list);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to connect all the hosts in the job.\n");
	    goto quit_job;
	}

	if (!mp_process.no_mpi)
	{
	    /* initialize the pmi database engine on the root node */
	}

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

