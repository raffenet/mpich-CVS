/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <stdlib.h>

#ifdef HAVE_WINDOWS_H

#if 0
HANDLE g_hLaunchMutex = NULL;

struct CachedUserNode
{
    HANDLE hUser;
    char account[40];
    char domain[40];
    char password[100];
    SYSTEMTIME timestamp;
    CachedUserNode *pNext;
};

CachedUserNode *g_pCachedList = NULL;

static void PrintPriorities(DWORD dwClass, DWORD dwPriority)
{
    char *str;
    switch (dwClass)
    {
    case ABOVE_NORMAL_PRIORITY_CLASS:
	str = "ABOVE_NORMAL_PRIORITY_CLASS";
	break;
    case BELOW_NORMAL_PRIORITY_CLASS:
	str = "BELOW_NORMAL_PRIORITY_CLASS";
	break;
    case HIGH_PRIORITY_CLASS:
	str = "HIGH_PRIORITY_CLASS";
	break;
    case IDLE_PRIORITY_CLASS:
	str = "IDLE_PRIORITY_CLASS";
	break;
    case NORMAL_PRIORITY_CLASS:
	str = "NORMAL_PRIORITY_CLASS";
	break;
    case REALTIME_PRIORITY_CLASS:
	str = "REALTIME_PRIORITY_CLASS";
	break;
    default:
	str = "unknown priority class";
	break;
    }
    switch(dwPriority)
    {
    case THREAD_PRIORITY_ABOVE_NORMAL:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_ABOVE_NORMAL\n", str);
	break;
    case THREAD_PRIORITY_BELOW_NORMAL:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_BELOW_NORMAL\n", str);
	break;
    case THREAD_PRIORITY_HIGHEST:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_HIGHEST\n", str);
	break;
    case THREAD_PRIORITY_IDLE:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_IDLE\n", str);
	break;
    case THREAD_PRIORITY_LOWEST:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_LOWEST\n", str);
	break;
    case THREAD_PRIORITY_NORMAL:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_NORMAL\n", str);
	break;
    case THREAD_PRIORITY_TIME_CRITICAL:
	dbg_printf("class: %s, priority: THREAD_PRIORITY_TIME_CRITICAL\n", str);
	break;
    default:
	dbg_printf("class: %s, priority: unknown\n", str);
	break;
    }
}

void statCachedUsers(char *pszOutput, int length)
{
    CachedUserNode *pIter;

    *pszOutput = '\0';
    length--;

    pIter = g_pCachedList;
    while (pIter)
    {
	if (pIter->domain[0] != '\0')
	    snprintf_update(pszOutput, length, "USER: %s\\%s\n", pIter->domain, pIter->account);
	else
	    snprintf_update(pszOutput, length, "USER: %s\n", pIter->account);
	pIter = pIter->pNext;
    }
}

void CacheUserHandle(char *account, char *domain, char *password, HANDLE hUser)
{
    CachedUserNode *pNode;

    pNode = new CachedUserNode;
    strcpy(pNode->account, account);
    if (domain != NULL)
	strcpy(pNode->domain, domain);
    else
	pNode->domain[0] = '\0';
    strcpy(pNode->password, password);
    pNode->hUser = hUser;
    GetSystemTime(&pNode->timestamp);
    pNode->pNext = g_pCachedList;
    g_pCachedList = pNode;
}

void RemoveCachedUser(HANDLE hUser)
{
    CachedUserNode *pTrailer, *pIter;

    if (g_pCachedList == NULL)
	return;

    if (g_pCachedList->hUser == hUser)
    {
	pIter = g_pCachedList;
	g_pCachedList = g_pCachedList->pNext;
	CloseHandle(pIter->hUser);
	delete pIter;
	return;
    }

    pTrailer = g_pCachedList;
    pIter = g_pCachedList->pNext;
    while (pIter)
    {
	if (pIter->hUser == hUser)
	{
	    pTrailer->pNext = pIter->pNext;
	    CloseHandle(pIter->hUser);
	    delete pIter;
	    return;
	}
	pTrailer = pTrailer->pNext;
	pIter = pIter->pNext;
    }
}

void RemoveAllCachedUsers()
{
    CachedUserNode *pIter;
    while (g_pCachedList)
    {
	pIter = g_pCachedList;
	g_pCachedList = g_pCachedList->pNext;
	CloseHandle(pIter->hUser);
	delete pIter;
    }
}

HANDLE GetCachedUser(char *account, char *domain, char *password)
{
    CachedUserNode *pIter;
    SYSTEMTIME now;

    pIter = g_pCachedList;
    while (pIter)
    {
	if (strcmp(pIter->account, account) == 0)
	{
	    if (domain != NULL)
	    {
		if (strcmp(pIter->domain, domain) == 0)
		{
		    if (strcmp(pIter->password, password) == 0)
		    {
			GetSystemTime(&now);
			if (now.wDay != pIter->timestamp.wDay)
			{
			    /* throw away cached handles not created on the same day */
			    RemoveCachedUser(pIter->hUser);
			    return INVALID_HANDLE_VALUE;
			}
			return pIter->hUser;
		    }
		}
	    }
	    else
	    {
		if (strcmp(pIter->password, password) == 0)
		{
		    return pIter->hUser;
		}
	    }
	}
	pIter = pIter->pNext;
    }

    return INVALID_HANDLE_VALUE;
}

HANDLE GetUserHandle(char *account, char *domain, char *password, int *pError)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    /* attempt to get a cached handle */
    hUser = GetCachedUser(account, domain, password);
    if (hUser != INVALID_HANDLE_VALUE)
	return hUser;

    /* logon the user */
    while (!LogonUser(
	account,
	domain, 
	password,
	LOGON32_LOGON_INTERACTIVE, 
	LOGON32_PROVIDER_DEFAULT, 
	&hUser))
    {
	error = GetLastError();
	if (error == ERROR_NO_LOGON_SERVERS)
	{
	    if (num_tries)
		Sleep(250);
	    else
	    {
		*pError = error;
		return INVALID_HANDLE_VALUE;
	    }
	    num_tries--;
	}
	else
	{
	    *pError = error;
	    return INVALID_HANDLE_VALUE;
	}
    }

    /* cache the user handle */
    CacheUserHandle(account, domain, password, hUser);

    return hUser;
}

HANDLE GetUserHandleNoCache(char *account, char *domain, char *password, int *pError)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    /* logon the user */
    while (!LogonUser(
	account,
	domain, 
	password,
	LOGON32_LOGON_INTERACTIVE, 
	LOGON32_PROVIDER_DEFAULT, 
	&hUser))
    {
	error = GetLastError();
	if (error == ERROR_NO_LOGON_SERVERS)
	{
	    if (num_tries)
		Sleep(250);
	    else
	    {
		*pError = error;
		return INVALID_HANDLE_VALUE;
	    }
	    num_tries--;
	}
	else
	{
	    *pError = error;
	    return INVALID_HANDLE_VALUE;
	}
    }

    /* cache the user handle */
    CacheUserHandle(account, domain, password, hUser);

    return hUser;
}

#endif

int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    smpd_enter_fn("smpd_get_user_handle");

    /* logon the user */
    while (!LogonUser(
	account,
	domain,
	password,
	LOGON32_LOGON_INTERACTIVE,
	LOGON32_PROVIDER_DEFAULT,
	&hUser))
    {
	error = GetLastError();
	if (error == ERROR_NO_LOGON_SERVERS)
	{
	    if (num_tries)
		Sleep(250);
	    else
	    {
		*handle_ptr = INVALID_HANDLE_VALUE;
		smpd_exit_fn("smpd_get_user_handle");
		return error;
	    }
	    num_tries--;
	}
	else
	{
	    *handle_ptr = INVALID_HANDLE_VALUE;
	    smpd_exit_fn("smpd_get_user_handle");
	    return error;
	}
    }

    *handle_ptr = hUser;
    smpd_exit_fn("smpd_get_user_handle");
    return SMPD_SUCCESS;
}

static void SetEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH]="", value[MAX_PATH]="";
    char *pChar;
    
    pChar = name;
    while (*bEnv != '\0')
    {
	if (*bEnv == '=')
	{
	    *pChar = '\0';
	    pChar = value;
	}
	else
	{
	    if (*bEnv == ';')
	    {
		*pChar = '\0';
		pChar = name;
		SetEnvironmentVariable(name, value);
	    }
	    else
	    {
		*pChar = *bEnv;
		pChar++;
	    }
	}
	bEnv++;
    }
    *pChar = '\0';
    SetEnvironmentVariable(name, value);
}

static void RemoveEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH]="", value[MAX_PATH]="";
    char *pChar;
    
    pChar = name;
    while (*bEnv != '\0')
    {
	if (*bEnv == '=')
	{
	    *pChar = '\0';
	    pChar = value;
	}
	else
	{
	    if (*bEnv == ';')
	    {
		*pChar = '\0';
		pChar = name;
		SetEnvironmentVariable(name, NULL);
	    }
	    else
	    {
		*pChar = *bEnv;
		pChar++;
	    }
	}
	bEnv++;
    }
    *pChar = '\0';
    SetEnvironmentVariable(name, NULL);
}

int smpd_priority_class_to_win_class(int *priorityClass)
{
    *priorityClass = NORMAL_PRIORITY_CLASS;
    return SMPD_SUCCESS;
}

int smpd_priority_to_win_priority(int *priority)
{
    *priority = THREAD_PRIORITY_NORMAL;
    return SMPD_SUCCESS;
}

/* Windows code */

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    HANDLE hStdin, hStdout, hStderr;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    HANDLE hIn, hOut, hErr;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo;
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag;
    int nError, result;
    unsigned long blocking_flag;
    sock_t sock_in, sock_out, sock_err;

    smpd_enter_fn("smpd_launch_process");

    smpd_priority_class_to_win_class(&priorityClass);
    smpd_priority_to_win_priority(&priority);

    /* Save stdin, stdout, and stderr */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError();
	smpd_err_printf("GetStdHandle failed, error %d\n", nError);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;;
    }

    /* Create sockets for stdin, stdout, and stderr */
    if (nError = smpd_make_socket_loop_choose(&hSockStdinR, SMPD_FALSE, &hSockStdinW, SMPD_TRUE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStdoutR, SMPD_TRUE, &hSockStdoutW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop_choose(&hSockStderrR, SMPD_TRUE, &hSockStderrW, SMPD_FALSE))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* Make the ends of the pipes that this process will use not inheritable */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* make the ends used by the spawned process blocking */
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hSockStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hSockStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }

    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    saInfo.hStdError = (HANDLE)hSockStderrW;
    saInfo.hStdInput = (HANDLE)hSockStdinR;
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    psInfo.hProcess = INVALID_HANDLE_VALUE;
    if (CreateProcess(
	NULL,
	process->exe,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);

	/*
	ResumeThread(psInfo.hThread);
	psInfo.hProcess;
	*/
	process->pid = psInfo.dwProcessId;

	/*CloseHandle(psInfo.hThread);*/
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess failed, error %d\n", nError);
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);

    /* make sock structures out of the sockets */
    nError = sock_native_to_sock(set, hIn, NULL, &sock_in);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hOut, NULL, &sock_out);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hErr, NULL, &sock_err);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }

    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
    sock_set_user_ptr(sock_in, process->in);
    sock_set_user_ptr(sock_out, process->out);
    sock_set_user_ptr(sock_err, process->err);

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    CloseHandle((HANDLE)hSockStdinR);
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	ResumeThread(psInfo.hThread);
        /*CloseHandle(psInfo.hProcess);*/
	process->wait = process->in->wait = process->out->wait = process->err->wait = psInfo.hProcess;
	CloseHandle(psInfo.hThread);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
}

void smpd_parse_account_domain(char *domain_account, char *account, char *domain)
{
    char *pCh, *pCh2;

    smpd_enter_fn("smpd_parse_account_domain");

    pCh = domain_account;
    pCh2 = domain;
    while ((*pCh != '\\') && (*pCh != '\0'))
    {
	*pCh2 = *pCh;
	pCh++;
	pCh2++;
    }
    if (*pCh == '\\')
    {
	pCh++;
	strcpy(account, pCh);
	*pCh2 = L'\0';
    }
    else
    {
	strcpy(account, domain_account);
	domain[0] = '\0';
    }

    smpd_exit_fn("smpd_parse_account_domain");
}

#else

/* Unix code */

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2], stderr_pipe_fds[2];
    int pid;
    sock_t sock_in, sock_out, sock_err;

    smpd_enter_fn("smpd_launch_process");

    /* create pipes for redirecting I/O */
    /*
    pipe(stdin_pipe_fds);
    pipe(stdout_pipe_fds);
    pipe(stderr_pipe_fds);
    */
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stderr_pipe_fds);

    pid = fork();
    if (pid < 0)
    {
	smpd_err_printf("fork failed.\n");
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }

    if (pid == 0)
    {
	/* child process */
	smpd_dbg_printf("client is alive and about to redirect io\n");

	close(0); 		  /* close stdin     */
	dup(stdin_pipe_fds[0]);   /* dup a new stdin */
	close(stdin_pipe_fds[0]);
	close(stdin_pipe_fds[1]);

	close(1);		  /* close stdout     */
	dup(stdout_pipe_fds[1]);  /* dup a new stdout */
	close(stdout_pipe_fds[0]);
	close(stdout_pipe_fds[1]);

	close(2);		  /* close stderr     */
	dup(stderr_pipe_fds[1]);  /* dup a new stderr */
	close(stderr_pipe_fds[0]);
	close(stderr_pipe_fds[1]);

	/*sprintf( env_path_for_exec, "PATH=%s", getenv( "PATH" ) );*/
	/*strcpy( pwd_for_exec, getenv( "PWD" ) );*/
	result = chdir( process->dir );
	if (result < 0)
	    chdir( getenv( "HOME" ) );

	result = execvp( process->exe, NULL );

	smpd_err_printf("execvp failed, exiting.\n");
	smpd_exit_fn("smpd_launch_process");
	exit(-1);
    }

    /* parent process */
    process->pid = pid;
    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);
    close(stderr_pipe_fds[1]);

    /* make sock structures out of the sockets */
    result = sock_native_to_sock(set, stdin_pipe_fds[1], NULL, &sock_in);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_native_to_sock(set, stdout_pipe_fds[0], NULL, &sock_out);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_native_to_sock(set, stderr_pipe_fds[0], NULL, &sock_err);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pid = process->in->id = process->out->id = process->err->id = pid;
    sock_set_user_ptr(sock_in, process->in);
    sock_set_user_ptr(sock_out, process->out);
    sock_set_user_ptr(sock_err, process->err);

    result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    process->wait = process->in->wait = process->out->wait = process->err->wait = pid;

    smpd_exit_fn("smpd_launch_process");
    return SMPD_SUCCESS;
}

#endif

int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    WaitForSingleObject(wait, INFINITE);
    result = GetExitCodeProcess(wait, exit_code_ptr);
    CloseHandle(wait);
    if (result)
	return SMPD_SUCCESS;
    *exit_code_ptr = -1;
    return SMPD_FAIL;
#else
    int status;
    waitpid(wait, &status, 0);
    *exit_code_ptr = (WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
    return SMPD_SUCCESS;
#endif
}
