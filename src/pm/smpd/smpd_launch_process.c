/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

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
int smpd_launch_process(char *cmd, char *search_path, char *env, char *dir, int priorityClass, int priority, int dbg, sock_set_t set, sock_t *sock_in, sock_t *sock_out, sock_t *sock_err, int *pid_ptr)
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
    HANDLE hRetVal = INVALID_HANDLE_VALUE;
    DWORD launch_flag;
    int nError;

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
    if (nError = smpd_make_socket_loop(&hSockStdinR, &hSockStdinW))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop(&hSockStdoutR, &hSockStdoutW))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (nError = smpd_make_socket_loop(&hSockStderrR, &hSockStderrW))
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

    SetEnvironmentVariables(env);
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(dir);

    launch_flag = 
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    if (CreateProcess(
	NULL,
	cmd,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);

	ResumeThread(psInfo.hThread);
	hRetVal = psInfo.hProcess;
	*pid_ptr = psInfo.dwProcessId;

	CloseHandle(psInfo.hThread);
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess failed, error %d\n", nError);
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(env);

    /* make sock structures out of the sockets */
    nError = sock_native_to_sock(set, hIn, NULL, sock_in);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hOut, NULL, sock_out);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }
    nError = sock_native_to_sock(set, hErr, NULL, sock_err);
    if (nError != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
    }

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    CloseHandle((HANDLE)hSockStdinR);
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);

    if (hRetVal != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hRetVal);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }
    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
}

#if 0
HANDLE LaunchProcess(char *cmd, char *env, char *dir, int priorityClass, int priority, HANDLE *hIn, HANDLE *hOut, HANDLE *hErr, int *pdwPid, int *nError, char *pszError, bool bDebug)
{
    HANDLE hStdin, hStdout, hStderr;
    HANDLE hPipeStdinR=NULL, hPipeStdinW=NULL;
    HANDLE hPipeStdoutR=NULL, hPipeStdoutW=NULL;
    HANDLE hPipeStderrR=NULL, hPipeStderrW=NULL;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo;
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    HANDLE hRetVal = INVALID_HANDLE_VALUE;
    DWORD launch_flag;
    
    /* Launching of the client processes must be synchronized because
       stdin,out,err are redirected for the entire process, not just this thread.*/
    WaitForSingleObject(g_hLaunchMutex, INFINITE);
    
    /* Don't handle errors, just let the process die.
       In the future this will be configurable to allow various debugging options.*/
#ifdef USE_SET_ERROR_MODE
    DWORD dwOriginalErrorMode;
    dwOriginalErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
    
    /* Save stdin, stdout, and stderr */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	*nError = GetLastError();
	strcpy(pszError, "GetStdHandle failed, ");
	ReleaseMutex(g_hLaunchMutex);
	return INVALID_HANDLE_VALUE;
    }
    
    /* Set the security attributes to allow handles to be inherited */
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;
    
    /* Create pipes for stdin, stdout, and stderr */
    if (!CreatePipe(&hPipeStdinR, &hPipeStdinW, &saAttr, 0))
    {
	*nError = GetLastError();
	strcpy(pszError, "CreatePipe failed, ");
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStdoutR, &hPipeStdoutW, &saAttr, 0))
    {
	*nError = GetLastError();
	strcpy(pszError, "CreatePipe failed, ");
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStderrR, &hPipeStderrW, &saAttr, 0))
    {
	*nError = GetLastError();
	strcpy(pszError, "CreatePipe failed, ");
	goto CLEANUP;
    }
    
    /* Make the ends of the pipes that this process will use not inheritable */
    if (!DuplicateHandle(GetCurrentProcess(), hPipeStdinW, GetCurrentProcess(), hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	*nError = GetLastError();
	strcpy(pszError, "DuplicateHandle failed, ");
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), hPipeStdoutR, GetCurrentProcess(), hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	*nError = GetLastError();
	strcpy(pszError, "DuplicateHandle failed, ");
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), hPipeStderrR, GetCurrentProcess(), hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	*nError = GetLastError();
	strcpy(pszError, "DuplicateHandle failed, ");
	goto CLEANUP;
    }
    
    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    if (!SetStdHandle(STD_INPUT_HANDLE, hPipeStdinR))
    {
	*nError = GetLastError();
	strcpy(pszError, "SetStdHandle failed, ");
	goto CLEANUP;
    }
    if (!SetStdHandle(STD_OUTPUT_HANDLE, hPipeStdoutW))
    {
	*nError = GetLastError();
	strcpy(pszError, "SetStdHandle failed, ");
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, hPipeStderrW))
    {
	*nError = GetLastError();
	strcpy(pszError, "SetStdHandle failed, ");
	goto RESTORE_CLEANUP;
    }
    
    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    saInfo.hStdError = hPipeStderrW;
    saInfo.hStdInput = hPipeStdinR;
    saInfo.hStdOutput = hPipeStdoutW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;
    /*saInfo.lpDesktop = "WinSta0\\Default";*/
    /*saInfo.wShowWindow = SW_SHOW;*/
    
    SetEnvironmentVariables(env);
    pEnv = GetEnvironmentStrings();
    
    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(dir);
    
    launch_flag = 
	/*
	DETACHED_PROCESS | IDLE_PRIORITY_CLASS;
	CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS;
	CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS;
	CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP;
	DETACHED_PROCESS | IDLE_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP;
	CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS | CREATE_SUSPENDED;
	*/
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (bDebug)
	launch_flag = launch_flag | DEBUG_PROCESS;

    /*PrintPriorities(priorityClass, priority);*/
    if (CreateProcess(
	NULL,
	cmd,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);

	DWORD dwClass, dwPriority;
	dwClass = GetPriorityClass(psInfo.hProcess);
	dwPriority = GetThreadPriority(psInfo.hThread);
	PrintPriorities(dwClass, dwPriority);

	ResumeThread(psInfo.hThread);
	hRetVal = psInfo.hProcess;
	*pdwPid = psInfo.dwProcessId;

	CloseHandle(psInfo.hThread);
    }
    else
    {
	*nError = GetLastError();
	strcpy(pszError, "CreateProcess failed, ");
    }
    
    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(env);
    
RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);
    
CLEANUP:
    ReleaseMutex(g_hLaunchMutex);
    CloseHandle(hPipeStdinR);
    CloseHandle(hPipeStdoutW);
    CloseHandle(hPipeStderrW);
    
#ifdef USE_SET_ERROR_MODE
    SetErrorMode(dwOriginalErrorMode);
#endif

    return hRetVal;
}
#endif

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
int smpd_launch_process(char *cmd, char *search_path, char *env, char *dir, int priorityClass, int priority, int dbg, sock_set_t set, sock_t *sock_in, sock_t *sock_out, sock_t *sock_err, int *pid_ptr)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2], stderr_pipe_fds[2];
    int pid;

    smpd_enter_fn("smpd_launch_process");

    /* create pipes for redirecting I/O */
    pipe(stdin_pipe_fds);
    pipe(stdout_pipe_fds);
    pipe(stderr_pipe_fds);

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
	result = chdir( dir );
	if (result < 0)
	    chdir( getenv( "HOME" ) );

	result = execvp( cmd, NULL );

	smpd_err_printf("execvp failed, exiting.\n");
	smpd_exit_fn("smpd_launch_process");
	exit(-1);
    }
    else
    {
	/* parent process */
	*pid_ptr = pid;
	close(stdin_pipe_fds[0]);
	close(stdout_pipe_fds[1]);
	close(stderr_pipe_fds[1]);

	/* make sock structures out of the sockets */
	result = sock_native_to_sock(set, stdin_pipe_fds[1], NULL, sock_in);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	}
	result = sock_native_to_sock(set, stdout_pipe_fds[0], NULL, sock_out);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	}
	result = sock_native_to_sock(set, stderr_pipe_fds[0], NULL, sock_err);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	}
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_SUCCESS;
}

#endif
