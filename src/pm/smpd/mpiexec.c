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
    int result = SMPD_SUCCESS;
    smpd_host_node_t *host_node_ptr;
    smpd_launch_node_t *launch_node_ptr;
    smpd_context_t *context;
    MPIDU_Sock_set_t set;
    MPIDU_Sock_t sock;
    smpd_state_t state;

    smpd_enter_fn("main");

    /* catch an empty command line */
    if (argc < 2)
    {
	mp_print_options();
	exit(0);
    }

    /* initialize */
    result = PMPI_Init(&argc, &argv);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPI_Init failed,\nerror: %d\n", result);
	smpd_exit_fn("main");
	return result;
    }
    /*
    result = MPIDU_Sock_init();
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_init failed,\nsock error: %s\n",
		      get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }
    */

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	goto quit_job;
    }

    smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT;

    /* parse the command line */
    smpd_dbg_printf("parsing the command line.\n");
    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the mpiexec command arguments.\n");
	goto quit_job;
    }

    /* make sure we have a passphrase to authenticate connections to the smpds */
    if (smpd_process.passphrase[0] == '\0')
	smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_process.passphrase[0] == '\0')
    {
	if (smpd_process.noprompt)
	{
	    printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
	    result = SMPD_FAIL;
	    goto quit_job;
	}
	printf("Please specify an authentication passphrase for smpd: ");
	fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
    }

    /* print and see what we've got */
    /* debugging output *************/
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
    /* end debug output *************/

    result = MPIDU_Sock_create_set(&set);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	goto quit_job;
    }
    smpd_process.set = set;

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;
    /* set the state to create a console session or a job session */
    state = smpd_process.do_console ? SMPD_MPIEXEC_CONNECTING_SMPD : SMPD_MPIEXEC_CONNECTING_TREE;

    /* start connecting the tree by posting a connect to the first host */
    result = MPIDU_Sock_post_connect(set, NULL, smpd_process.host_list->host, smpd_process.port, &sock);
    if (result != MPI_SUCCESS)
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
    result = MPIDU_Sock_set_user_ptr(sock, context);
    if (result != MPI_SUCCESS)
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

    if ((result != SMPD_SUCCESS) && (smpd_process.mpiexec_exit_code == 0))
    {
	smpd_process.mpiexec_exit_code = -1;
    }

    /* finalize */
    /*
    smpd_dbg_printf("calling MPIDU_Sock_finalize\n");
    result = MPIDU_Sock_finalize();
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }
    */
    /* MPI_Finalize called in smpd_exit()
    smpd_dbg_printf("calling MPI_Finalize\n");
    result = PMPI_Finalize();
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPI_Finalize failed,\nerror: %d\n", result);
    }
    */

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
    return smpd_exit(smpd_process.mpiexec_exit_code);
}

