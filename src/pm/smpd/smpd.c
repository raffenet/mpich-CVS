/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include "smpd.h"
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

int main(int argc, char* argv[])
{
    int result;
#ifdef HAVE_WINDOWS_H
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(SMPD_SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)smpd_service_main },
        { NULL, NULL }
    };
#else
    char smpd_filename[SMPD_MAX_FILENAME] = "";
    char response[100] = "no";
    char *homedir;
#endif

    smpd_enter_fn("main");

    /* initialization */
    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_init failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("smpd_init_process failed.\n");
	smpd_exit_fn("main");
	return result;
    }

    /* parse the command line */
    result = smpd_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to parse the command arguments.\n");
	smpd_exit_fn("main");
	return result;
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_process.bService)
    {
	printf( "\nStartServiceCtrlDispatcher being called.\n" );
	printf( "This may take several seconds.  Please wait.\n" );
	fflush(stdout);

	/* If StartServiceCtrlDispatcher returns true the service has exited */
	result = StartServiceCtrlDispatcher(dispatchTable);
	if (result)
	{
	    smpd_exit_fn("main");
	    smpd_exit(0);
	}

	result = GetLastError();
	if (result != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
	{
	    smpd_add_error_to_message_log(TEXT("StartServiceCtrlDispatcher failed."));
	    smpd_exit_fn("main");
	    smpd_exit(0);
	}
    }
    printf("\nRunning smpd from the console, not as a service.\n");
    fflush(stdout);
    smpd_process.bService = SMPD_FALSE;
#endif

    if (smpd_process.passphrase[0] == '\0')
	smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
    if (smpd_process.passphrase[0] == '\0')
    {
	if (smpd_process.noprompt)
	{
	    printf("Error: No smpd passphrase specified through the registry or .smpd file, exiting.\n");
	    smpd_exit_fn("main");
	    return -1;
	}
	printf("Please specify an authentication passphrase for this smpd: ");
	fflush(stdout);
	smpd_get_password(smpd_process.passphrase);
#ifndef HAVE_WINDOWS_H
	homedir = getenv("HOME");
	strcpy(smpd_filename, homedir);
	if (smpd_filename[strlen(smpd_filename)-1] != '/')
	    strcat(smpd_filename, "/.smpd");
	else
	    strcat(smpd_filename, ".smpd");
	printf("Would you like to save this passphrase in '%s'? ", smpd_filename);
	fflush(stdout);
	fgets(response, 100, stdin);
	if (smpd_is_affirmative(response))
	{
	    FILE *fout;
	    umask(0077);
	    fout = fopen(smpd_filename, "w");
	    if (fout == NULL)
	    {
		printf("Error: unable to open '%s', errno = %d\n", smpd_filename, errno);
		smpd_exit_fn("main");
		return errno;
	    }
	    fprintf(fout, "phrase=%s\n", smpd_process.passphrase);
	    fclose(fout);
	}
#endif
    }

    result = smpd_entry_point(argc, argv);

    smpd_finalize_printf();

    smpd_exit(result);
    smpd_exit_fn("main");
    return result;
}

int smpd_entry_point()
{
    int result;
    sock_set_t set;
    sock_t listener;

    /* This function is called by main or by smpd_service_main in the case of a Windows service */

    smpd_enter_fn("smpd_entry_point");

#ifdef HAVE_WINDOWS_H
    /* prevent the os from bringing up debug message boxes if this process crashes */
    if (smpd_process.bService)
    {
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	if (!smpd_report_status_to_sc_mgr(SERVICE_RUNNING, NO_ERROR, 0))
	{
	    result = GetLastError();
	    smpd_err_printf("Unable to report that the service has started, error: %d\n", result);
	    smpd_exit_fn("smpd_entry_point");
	    return result;
	}
	smpd_clear_process_registry();
    }
#else
    /* put myself in the background if flag is set */
    if (smpd_process.bNoTTY)
    {
	int fd;

        if (fork() != 0)  /* parent exits; child in background */
	    exit(0);
	setsid();           /* become session leader; no controlling tty */
	smpd_signal(SIGHUP, SIG_IGN); /* make sure no sighup when leader ends */
	/* leader exits; svr4: make sure do not get another controlling tty */
        if (fork() != 0)
	    exit(0);

	/* How do I make stdout and stderr go away? */
	/* redirect stdout/err to nothing */
	fd = open("/dev/null", O_APPEND);
	if (fd != -1)
	{
	    close(1);
	    close(2);
	    dup2(fd, 1);
	    dup2(fd, 2);
	    close(fd);
	}
	/*
        freopen("/dev/null", "a", stdout);
        freopen("/dev/null", "a", stderr);
	*/
	close(0);
    }
#endif

    /* This process is the root_smpd.  All sessions are child processes of this process. */
    smpd_process.id = 0;
    smpd_process.root_smpd = SMPD_TRUE;

    smpd_set_smpd_data("binary", smpd_process.pszExe);

    result = sock_create_set(&set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_create_set failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_entry_point");
	return result;
    }
    smpd_process.set = set;
    smpd_dbg_printf("created a set for the listener: %d\n", sock_getsetid(set));
    result = sock_listen(set, NULL, &smpd_process.port, &listener); 
    if (result != SOCK_SUCCESS)
    {
	/* If another smpd is running and listening on this port, tell it to shutdown or restart? */
	smpd_err_printf("sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_entry_point");
	return result;
    }
    smpd_dbg_printf("smpd listening on port %d\n", smpd_process.port);

    result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the smpd listener.\n");
	smpd_exit_fn("smpd_entry_point");
	return result;
    }
    result = sock_set_user_ptr(listener, smpd_process.listener_context);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_entry_point");
	return result;
    }
    smpd_process.listener_context->state = SMPD_SMPD_LISTENING;

    result = smpd_enter_at_state(set, SMPD_SMPD_LISTENING);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("state machine failed.\n");
    }

    result = sock_destroy_set(set);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to destroy the set, error:\n%s\n",
	    get_sock_error_string(result));
    }

    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
    }
    smpd_exit_fn("smpd_entry_point");
    return 0;
}

