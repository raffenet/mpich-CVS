/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

int smpd_start_mgr(sock_set_t set, sock_t sock)
{
    char password[100];
    int result;
#ifdef HAVE_WINDOWS_H
    char read_handle_str[20], write_handle_str[20];
    char domainaccount[100], account[100], domain[100], port_str[20];
    char *pszDomain;
    HANDLE user_handle;
    int num_tries;
    char cmd[8192];
    PROCESS_INFORMATION pInfo;
    STARTUPINFO sInfo;
    SECURITY_ATTRIBUTES security;
    HANDLE hRead, hWrite, hReadRemote, hWriteRemote;
    DWORD num_read, num_written;
#endif
    char session[100];

    smpd_read_string(set, sock, session, 100);
    if (strcmp(session, SMPD_SMPD_SESSION_STR) == 0)
    {
	/* send password request or not */
	if (smpd_process.bPasswordProtect)
	{
	    result = smpd_write_string(set, sock, SMPD_PWD_REQUEST);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    result = smpd_read_string(set, sock, password, 100);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    if (strcmp(password, smpd_process.SMPDPassword) == 0)
		result = smpd_write_string(set, sock, SMPD_AUTHENTICATION_ACCEPTED_STR);
	    else
	    {
		result = smpd_write_string(set, sock, SMPD_AUTHENTICATION_REJECTED_STR);
		smpd_close_connection(set, sock);
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	}
	else
	{
	    result = smpd_write_string(set, sock, SMPD_NO_PWD_REQUEST);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	}
	smpd_session(set, sock);
    }
    else if (strcmp(session, SMPD_PROCESS_SESSION_STR) == 0)
    {
#ifdef HAVE_WINDOWS_H

	/************ Windows code to spawn the manager ************/

	domainaccount[0] = '\0';
	password[0] = '\0';
	if (smpd_process.bService)
	{
	    result = smpd_write_string(set, sock, SMPD_CRED_REQUEST);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    result = smpd_read_string(set, sock, domainaccount, 100);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	    result = smpd_read_string(set, sock, password, 100);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	}
	else
	{
	    result = smpd_write_string(set, sock, SMPD_NO_CRED_REQUEST);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_close_connection(set, sock);
		return SMPD_FAIL;
	    }
	}

	/* start the manager */
	security.bInheritHandle = TRUE;
	security.lpSecurityDescriptor = NULL;
	security.nLength = sizeof(security);
	/* create a pipe to send the listening port information through */
	if (!CreatePipe(&hRead, &hWriteRemote, &security, 0))
	{
	    smpd_err_printf("CreatePipe failed, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}
	/* prevent the local read end of the pipe from being inherited */
	if (!DuplicateHandle(GetCurrentProcess(), hRead, GetCurrentProcess(), &hRead, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
	{
	    smpd_err_printf("Unable to duplicate the read end of the pipe, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hRead);
	    CloseHandle(hWriteRemote);
	    return SMPD_FAIL;
	}
	/* create a pipe to send the account information through */
	if (!CreatePipe(&hReadRemote, &hWrite, &security, 0))
	{
	    smpd_err_printf("CreatePipe failed, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hRead);
	    CloseHandle(hWriteRemote);
	    return SMPD_FAIL;
	}
	/* prevent the local write end of the pipe from being inherited */
	if (!DuplicateHandle(GetCurrentProcess(), hWrite, GetCurrentProcess(), &hWrite, 0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
	{
	    smpd_err_printf("Unable to duplicate the read end of the pipe, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hRead);
	    CloseHandle(hWrite);
	    CloseHandle(hReadRemote);
	    CloseHandle(hWriteRemote);
	    return SMPD_FAIL;
	}
	/* encode the pipes on the command line */
	snprintf(cmd, 8192, "\"%s\" -mgr -read %s -write %s", smpd_process.pszExe, 
	    smpd_encode_handle(read_handle_str, hReadRemote), 
	    smpd_encode_handle(write_handle_str, hWriteRemote));
	smpd_dbg_printf("starting command:\n%s\n", cmd);
	GetStartupInfo(&sInfo);
	if (smpd_process.bService)
	{
	    smpd_parse_account_domain(domainaccount, account, domain);
	    if (strlen(domain) < 1)
		pszDomain = NULL;
	    else
		pszDomain = domain;

	    result = smpd_get_user_handle(account, pszDomain, password, &user_handle);
	    if (user_handle == INVALID_HANDLE_VALUE)
	    {
		smpd_err_printf("smpd_get_user_handle failed, error %d.\n", result);
		smpd_close_connection(set, sock);
		CloseHandle(hRead);
		CloseHandle(hWrite);
		CloseHandle(hReadRemote);
		CloseHandle(hWriteRemote);
		return SMPD_FAIL;
	    }

	    result = SMPD_SUCCESS;
	    if (!ImpersonateLoggedOnUser(user_handle))
	    {
		result = GetLastError();
		smpd_err_printf("ImpersonateLoggedOnUser failed, error %d\n", result);
		smpd_close_connection(set, sock);
		CloseHandle(hRead);
		CloseHandle(hWrite);
		CloseHandle(hReadRemote);
		CloseHandle(hWriteRemote);
		return SMPD_FAIL;
	    }
	}
	
	num_tries = 4;
	do
	{
	    if (smpd_process.bService)
	    {
		result = CreateProcessAsUser(
		    user_handle,
		    NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo);
	    }
	    else
	    {
		result = CreateProcess(
		    NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &sInfo, &pInfo);
	    }

	    if (result)
	    {
		result = SMPD_SUCCESS;
		num_tries = 0;
	    }
	    else
	    {
		result = GetLastError();
		if (result == ERROR_REQ_NOT_ACCEP)
		{
		    Sleep(1000);
		    num_tries--;
		    if (num_tries == 0)
		    {
			smpd_err_printf("%s failed, error %d\n", smpd_process.bService ? "CreateProcessAsUser" : "CreateProcess", result);
		    }
		}
		else
		{
		    smpd_err_printf("%s failed, error %d\n", smpd_process.bService ? "CreateProcessAsUser" : "CreateProcess", result);
		    num_tries = 0;
		}
	    }
	} while (num_tries);

	if (smpd_process.bService)
	    RevertToSelf();
	if (result != SMPD_SUCCESS)
	{
	    smpd_close_connection(set, sock);
	    CloseHandle(hRead);
	    CloseHandle(hWrite);
	    CloseHandle(hReadRemote);
	    CloseHandle(hWriteRemote);
	    return SMPD_FAIL;
	}

	CloseHandle(pInfo.hThread);
	CloseHandle(pInfo.hProcess);
	CloseHandle(hReadRemote);
	CloseHandle(hWriteRemote);

	smpd_dbg_printf("smpd reading the port string from the manager\n");
	/* read the listener port from the pipe to the manager */
	if (!ReadFile(hRead, port_str, 20, &num_read, NULL))
	{
	    smpd_err_printf("ReadFile() failed, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hRead);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	CloseHandle(hRead);
	if (num_read != 20)
	{
	    smpd_err_printf("parital port string read, %d bytes of 20\n", num_read);
	    smpd_close_connection(set, sock);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	/* send the account and password to the manager */
	smpd_dbg_printf("smpd sending the account and password to the manager\n");
	if (!WriteFile(hWrite, domainaccount, 100, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile('%s') failed to write the account, error %d\n", domainaccount, GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	if (num_written != 100)
	{
	    smpd_err_printf("parital account string written, %d bytes of 100\n", num_written);
	    smpd_close_connection(set, sock);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	if (!WriteFile(hWrite, password, 100, &num_written, NULL))
	{
	    smpd_err_printf("WriteFile() failed to write the password, error %d\n", GetLastError());
	    smpd_close_connection(set, sock);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	if (num_written != 100)
	{
	    smpd_err_printf("parital password string written, %d bytes of 100\n", num_written);
	    smpd_close_connection(set, sock);
	    CloseHandle(hWrite);
	    return SMPD_FAIL;
	}
	CloseHandle(hWrite);

	/* write the port to reconnect to back to mpiexec */
	smpd_dbg_printf("smpd writing reconnect request: port %s\n", port_str);
	result = smpd_write_string(set, sock, port_str);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("Unable to write the re-connect port number back to mpiexec.\n");
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}

	smpd_dbg_printf("smpd closing the sock and set.\n");
	smpd_close_connection(set, sock);
#else

	/************ Unix code to spawn the manager ************/

	/* don't request user credentials */
	result = smpd_write_string(set, sock, SMPD_NO_CRED_REQUEST);
	if (result != SMPD_SUCCESS)
	{
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}

	/* don't reconnect to a new sock */
	result = smpd_write_string(set, sock, SMPD_NO_RECONNECT_PORT_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}

	result = fork();
	if (result == -1)
	{
	    smpd_err_printf("fork failed, errno %d\n", errno);
	    smpd_close_connection(set, sock);
	    return SMPD_FAIL;
	}
	if (result == 0)
	{
	    /* I'm the child so handle a session and then exit */
	    result = smpd_session(set, sock);
	    if (result != SMPD_SUCCESS)
		smpd_err_printf("smpd_session() failed.\n");
	    exit(result);
	}
	/* I'm the parent so close my copy of the connection and return success */
	smpd_close_connection(set, sock);
#endif
    }
    else
    {
	smpd_err_printf("invalid session request: '%s'\n", session);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}
