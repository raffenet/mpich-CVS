/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

#ifdef HAVE_WINDOWS_H
void timeout_thread(void *p)
{
    MPIU_Size_t num_written;
    char ch = -1;
    Sleep(smpd_process.timeout * 1000);
    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
    if (smpd_process.timeout_sock != MPIDU_SOCK_INVALID_SOCK)
    {
	MPIDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	Sleep(30000); /* give the engine 30 seconds to shutdown and then force an exit */
    }
    ExitProcess(-1);
}
#else
#ifdef SIGALRM
void timeout_function(int signo)
{
    MPIU_Size_t num_written;
    char ch = -1;
    static int alarmed = 0;
    if (signo == SIGALRM)
    {
	if (smpd_process.timeout_sock != MPIDU_SOCK_INVALID_SOCK && alarmed == 0)
	{
	    alarmed = 1;
	    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
	    MPIDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	    alarm(30); /* give the engine 30 seconds to shutdown and then force an exit */
	}
	else
	{
	    if (alarmed == 0)
	    {
		smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
	    }
	    exit(-1);
	}
    }
}
#else
#ifdef HAVE_PTHREAD_H
void *timeout_thread(void *p)
{
    MPIU_Size_t num_written;
    char ch = -1;
    sleep(smpd_process.timeout);
    smpd_err_printf("\nmpiexec terminated job due to %d second timeout.\n", smpd_process.timeout);
    if (smpd_process.timeout_sock != MPIDU_SOCK_INVALID_SOCK)
    {
	MPIDU_Sock_write(smpd_process.timeout_sock, &ch, 1, &num_written);
	sleep(30); /* give the engine 30 seconds to shutdown and then force an exit */
    }
    exit(-1);
}
#endif
#endif
#endif

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

    smpd_process.mpiexec_argv0 = argv[0];

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

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    result = MPIDU_Sock_create_set(&set);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	goto quit_job;
    }
    smpd_process.set = set;

    /* Check to see if the user wants to use a remote shell mechanism for launching the processes
     * instead of using the smpd process managers.
     */
    if (smpd_process.rsh_mpiexec == SMPD_TRUE)
    {
	/* Do rsh or localonly stuff */
	result = mpiexec_rsh();

	/* skip over the non-rsh code and go to the cleanup section */
	goto quit_job;
    }

    /* Start the timeout mechanism if specified */
    /* This code occurs after the rsh_mpiexec option check because the rsh code implementes timeouts differently */
    if (smpd_process.timeout > 0)
    {
#ifdef HAVE_WINDOWS_H
	/* create a Windows thread to sleep until the timeout expires */
	if (smpd_process.timeout_thread == NULL)
	{
	    smpd_process.timeout_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)timeout_thread, NULL, 0, NULL);
	    if (smpd_process.timeout_thread == NULL)
	    {
		printf("Error: unable to create a timeout thread, errno %d.\n", GetLastError());
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
	    }
	}
#elif defined(SIGALRM)
	/* create an alarm to signal mpiexec when the timeout expires */
	smpd_signal(SIGALRM, timeout_function);
	alarm(smpd_process.timeout);
#elif defined(HAVE_PTHREAD_H)
	/* create a pthread to sleep until the timeout expires */
	result = pthread_create(&smpd_process.timeout_thread, NULL, timeout_thread, NULL);
	if (result != 0)
	{
	    printf("Error: unable to create a timeout thread, errno %d.\n", result);
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
#else
	/* no timeout mechanism available */
#endif
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
	/* close stdin so the input thread will exit */
	CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
	if (WaitForSingleObject(smpd_process.hStdinThread, 3000) != WAIT_OBJECT_0)
	{
	    TerminateThread(smpd_process.hStdinThread, 321);
	}
	CloseHandle(smpd_process.hStdinThread);
    }
    if (smpd_process.hCloseStdinThreadEvent)
    {
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
	smpd_process.hCloseStdinThreadEvent = NULL;
    }
#endif
    smpd_exit_fn("main");
    return smpd_exit(smpd_process.mpiexec_exit_code);
}

