/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_WINDOWS_H
#include "smpd_service.h"
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

void smpd_print_options(void)
{
    printf("smpd options:\n");
    printf(" -port <port> or -p <port>\n");
    printf(" -phrase <passphrase>\n");
    printf(" -debug or -d\n");
    printf(" -noprompt\n");
    printf(" -restart [hostname]\n");
    printf(" -shutdown [hostname]\n");
    printf(" -console [hostname]\n");
    printf(" -status [hostname]\n");
    printf("unix only options:\n");
    printf(" -s\n");
    printf(" -r\n");
    printf(" -smpdfile <filename>\n");
    printf("windows only options:\n");
    printf(" -install or -regserver\n");
    printf(" -remove  or -unregserver or -uninstall\n");
    printf(" -start\n");
    printf(" -stop\n");
    printf("\n");
    printf("bracketed [] items are optional\n");
    printf("\n");
    printf("\"smpd -d\" will start the smpd in debug mode.\n");
    printf("\"smpd -s\" will start the smpd in daemon mode for the current unix user.\n");
    printf("\"smpd -install\" will install and start the smpd in Windows service mode.\n");
    printf(" This must be done by a user with administrator privileges and then all\n");
    printf(" users can launch processes with mpiexec.\n");
    printf("Not yet implemented:\n");
    printf("\"smpd -r\" will start the smpd in root daemon mode for unix.\n");
}

int smpd_parse_command_args(int *argcp, char **argvp[])
{
    int result;
#ifdef HAVE_WINDOWS_H
    char str[20], read_handle_str[20], write_handle_str[20];
    int port;
    MPIDU_Sock_t listener;
    MPIDU_Sock_set_t set;
    HANDLE hWrite, hRead;
    DWORD num_written, num_read;
#endif
    int dbg_flag;

    smpd_enter_fn("smpd_parse_command_args");

    /* check for help option */
    if (
#ifndef HAVE_WINDOWS_H
	*argcp < 2 || /* unix: print the options if no arguments are supplied */
#endif
	smpd_get_opt(argcp, argvp, "-help") || smpd_get_opt(argcp, argvp, "-?"))
    {
	smpd_print_options();
	smpd_exit(0);
    }

    /* check for the service/silent option */
#ifdef HAVE_WINDOWS_H
    smpd_process.bService = SMPD_TRUE;
#else
    if (smpd_get_opt(argcp, argvp, "-s"))
	smpd_process.bNoTTY = SMPD_TRUE;
#endif

    /* check for debug option */
    if (smpd_get_opt_int(argcp, argvp, "-d", &dbg_flag))
    {
	smpd_process.dbg_state = dbg_flag;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt(argcp, argvp, "-d"))
    {
	smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt_int(argcp, argvp, "-debug", &dbg_flag))
    {
	smpd_process.dbg_state = dbg_flag;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }
    if (smpd_get_opt(argcp, argvp, "-debug"))
    {
	smpd_process.dbg_state = SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE;
	smpd_process.bNoTTY = SMPD_FALSE;
	smpd_process.bService = SMPD_FALSE;
    }

    /* check for port option */
    smpd_get_opt_int(argcp, argvp, "-p", &smpd_process.port);
    smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);

    smpd_process.noprompt = smpd_get_opt(argcp, argvp, "-noprompt");

#ifdef HAVE_WINDOWS_H

    /* check for service options */
    if (smpd_get_opt(argcp, argvp, "-remove") ||
	smpd_get_opt(argcp, argvp, "-unregserver") ||
	smpd_get_opt(argcp, argvp, "-uninstall") ||
	smpd_get_opt(argcp, argvp, "/Remove") ||
	smpd_get_opt(argcp, argvp, "/Uninstall"))
    {
	/*RegDeleteKey(HKEY_CURRENT_USER, MPICHKEY);*/
	smpd_remove_service(SMPD_TRUE);
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-install") ||
	smpd_get_opt(argcp, argvp, "-regserver") ||
	smpd_get_opt(argcp, argvp, "/Install") ||
	smpd_get_opt(argcp, argvp, "/install") ||
	smpd_get_opt(argcp, argvp, "/RegServer"))
    {
	char phrase[SMPD_PASSPHRASE_MAX_LENGTH]="", port_str[12]="";
	char version[100]="";

	if (smpd_remove_service(SMPD_FALSE) == SMPD_FALSE)
	{
	    printf("Unable to remove the previous installation, install failed.\n");
	    ExitProcess(0);
	}
	
	if (smpd_get_opt_string(argcp, argvp, "-phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH) ||
	    smpd_get_win_opt_string(argcp, argvp, "/phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH))
	{
	    smpd_set_smpd_data("phrase", phrase);
	}
	if (smpd_get_opt(argcp, argvp, "-getphrase"))
	{
	    printf("passphrase for smpd: ");fflush(stdout);
	    smpd_get_password(phrase);
	    smpd_set_smpd_data("phrase", phrase);
	}
	if (smpd_get_opt_string(argcp, argvp, "-port", port_str, 10))
	{
	    smpd_set_smpd_data("port", port_str);
	}
	/*ParseRegistry(true);*/
	smpd_install_service(SMPD_FALSE, SMPD_TRUE);
	/*
	GetMPDVersion(version, 100);
	WriteMPDRegistry("version", version);
	*/
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-start"))
    {
	smpd_start_service();
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-stop"))
    {
	smpd_stop_service();
	ExitProcess(0);
    }

    if (smpd_get_opt(argcp, argvp, "-mgr"))
    {
	smpd_process.bService = SMPD_FALSE;
	if (!smpd_get_opt_string(argcp, argvp, "-read", read_handle_str, 20))
	{
	    smpd_err_printf("manager started without a read pipe handle.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!smpd_get_opt_string(argcp, argvp, "-write", write_handle_str, 20))
	{
	    smpd_err_printf("manager started without a write pipe handle.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	hRead = smpd_decode_handle(read_handle_str);
	hWrite = smpd_decode_handle(write_handle_str);

	smpd_dbg_printf("manager creating listener and session sets.\n");

	result = MPIDU_Sock_create_set(&set);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_create_set(listener) failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_process.set = set;
	smpd_dbg_printf("created set for manager listener, %d\n", MPIDU_Sock_get_sock_set_id(set));
	port = 0;
	result = MPIDU_Sock_listen(set, NULL, &port, &listener); 
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("smpd manager listening on port %d\n", port);

	result = smpd_create_context(SMPD_CONTEXT_LISTENER, set, listener, -1, &smpd_process.listener_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a context for the smpd listener.\n");
	    smpd_exit_fn("smpd_parse_command_args");
	    return result;
	}
	result = MPIDU_Sock_set_user_ptr(listener, smpd_process.listener_context);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return result;
	}
	smpd_process.listener_context->state = SMPD_MGR_LISTENING;

	memset(str, 0, 20);
	snprintf(str, 20, "%d", port);
	smpd_dbg_printf("manager writing port back to smpd.\n");
	if (!WriteFile(hWrite, str, 20, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	CloseHandle(hWrite);
	if (num_written != 20)
	{
	    smpd_err_printf("wrote only %d bytes of 20\n", num_written);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("manager reading account and password from smpd.\n");
	if (!ReadFile(hRead, smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_MAX_ACCOUNT_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_MAX_ACCOUNT_LENGTH);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.UserPassword, SMPD_MAX_PASSWORD_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_MAX_PASSWORD_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_MAX_PASSWORD_LENGTH);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != SMPD_PASSPHRASE_MAX_LENGTH)
	{
	    smpd_err_printf("read only %d bytes of %d\n", num_read, SMPD_PASSPHRASE_MAX_LENGTH);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_process.credentials_prompt = SMPD_FALSE;

	result = smpd_enter_at_state(set, SMPD_MGR_LISTENING);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("state machine failed.\n");
	}

	result = MPIDU_Sock_finalize();
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
	}
	smpd_exit(0);
	smpd_exit_fn("smpd_parse_command_args (ExitProcess)");
	ExitProcess(0);
    }
#endif

    /* check for the status option */
    if (smpd_get_opt_string(argcp, argvp, "-status", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.do_status = 1;
    }
    else if (smpd_get_opt(argcp, argvp, "-status"))
    {
	gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.do_status = 1;
    }

    /* check for console options */
    if (smpd_get_opt_string(argcp, argvp, "-console", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
    }
    else if (smpd_get_opt(argcp, argvp, "-console"))
    {
	gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
    }
    if (smpd_process.do_console)
    {
	/* This may need to be changed to avoid conflict */
	if (smpd_get_opt(argcp, argvp, "-p"))
	{
	    smpd_process.use_process_session = 1;
	}
    }

    if (smpd_get_opt_string(argcp, argvp, "-shutdown", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.shutdown = 1;
    }
    else if (smpd_get_opt(argcp, argvp, "-shutdown"))
    {
	gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.shutdown = 1;
    }

    if (smpd_get_opt_string(argcp, argvp, "-restart", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_process.do_console = 1;
	smpd_process.restart = 1;
    }
    else if (smpd_get_opt(argcp, argvp, "-restart"))
    {
#ifdef HAVE_WINDOWS_H
	printf("restarting the smpd service...\n");
	smpd_stop_service();
	Sleep(1000);
	smpd_start_service();
	smpd_exit(0);
#else
	gethostname(smpd_process.console_host, SMPD_MAX_HOST_LENGTH);
	smpd_process.do_console = 1;
	smpd_process.restart = 1;
#endif
    }

    smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);

    if (smpd_get_opt_string(argcp, argvp, "-smpdfile", smpd_process.smpd_filename, SMPD_MAX_FILENAME))
    {
	struct stat s;

	if (stat(smpd_process.smpd_filename, &s) == 0)
	{
	    if (s.st_mode & 00077)
	    {
		printf(".smpd file cannot be readable by anyone other than the current user.\n");
		smpd_exit_fn("smpd_parse_command_args");
		return SMPD_FAIL;
	    }
	}
    }

    if (smpd_process.do_console)
    {
	result = smpd_do_console();
	smpd_exit_fn("smpd_parse_command_args");
	return result;
    }

    smpd_exit_fn("smpd_parse_command_args");
    return SMPD_SUCCESS;
}
