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

int smpd_parse_command_args(int *argcp, char **argvp[])
{
    int result;
#ifdef HAVE_WINDOWS_H
    char str[20], read_handle_str[20], write_handle_str[20];
    int port;
    sock_t listener;
    sock_set_t set;
    HANDLE hWrite, hRead;
    DWORD num_written, num_read;
#endif
    int dbg_flag;

    smpd_enter_fn("smpd_parse_command_args");

#ifdef HAVE_WINDOWS_H
    smpd_process.bService = SMPD_TRUE;
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
    if (smpd_get_opt(argcp, argvp, "-remove") || smpd_get_opt(argcp, argvp, "-unregserver") || smpd_get_opt(argcp, argvp, "-uninstall"))
    {
	/*RegDeleteKey(HKEY_CURRENT_USER, MPICHKEY);*/
	smpd_remove_service(SMPD_TRUE);
	ExitProcess(0);
    }
    if (smpd_get_opt(argcp, argvp, "-install") || smpd_get_opt(argcp, argvp, "-regserver"))
    {
	char phrase[SMPD_PASSPHRASE_MAX_LENGTH]="", port_str[12]="";
	char version[100]="";

	if (smpd_remove_service(SMPD_FALSE) == SMPD_FALSE)
	{
	    printf("Unable to remove the previous installation, install failed.\n");
	    ExitProcess(0);
	}
	
	if (smpd_get_opt_string(argcp, argvp, "-phrase", phrase, SMPD_PASSPHRASE_MAX_LENGTH))
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

	result = sock_create_set(&set);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_create_set(listener) failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	smpd_process.set = set;
	smpd_dbg_printf("created set for manager listener, %d\n", sock_getsetid(set));
	port = 0;
	result = sock_listen(set, NULL, &port, &listener); 
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_listen failed,\nsock error: %s\n", get_sock_error_string(result));
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
	result = sock_set_user_ptr(listener, smpd_process.listener_context);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(result));
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
	if (!ReadFile(hRead, smpd_process.UserAccount, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (!ReadFile(hRead, smpd_process.UserPassword, 100, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile failed, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}
	if (num_read != 100)
	{
	    smpd_err_printf("read only %d bytes of 100\n", num_read);
	    smpd_exit_fn("smpd_parse_command_args");
	    return SMPD_FAIL;
	}

	result = smpd_enter_at_state(set, SMPD_MGR_LISTENING);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("state machine failed.\n");
	}

	result = sock_finalize();
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_finalize failed,\nsock error: %s\n", get_sock_error_string(result));
	}
	smpd_exit(0);
	smpd_exit_fn("smpd_parse_command_args (ExitProcess)");
	ExitProcess(0);
    }
#endif

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
	FILE *fin;
	char line[SMPD_PASSPHRASE_MAX_LENGTH+3];
	struct stat s;

	if (stat(smpd_process.smpd_filename, &s) == 0)
	{
	    if (s.st_mode & 00077)
	    {
		printf(".smpd file cannot be readable by anyone other than the current user.\n");
		smpd_exit_fn("smpd_parse_command_args");
		return SMPD_FAIL;
	    }
	    fin = fopen(smpd_process.smpd_filename, "r");
	    if (fin != NULL)
	    {
		fgets(line, SMPD_PASSPHRASE_MAX_LENGTH+2, fin);
		line[SMPD_PASSPHRASE_MAX_LENGTH] = '\0';
		if (strlen(line) > 0)
		{
		    while (strlen(line) > 0 && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
			line[strlen(line)-1] = '\0';
		    if (strlen(line) > 0)
		    {
			strcpy(smpd_process.passphrase, line);
		    }
		}
		fclose(fin);
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
