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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_WINDOWS_H

int smpd_clear_process_registry()
{
    if (RegDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process") != ERROR_SUCCESS)
    {
	/* It's ok if the key does not exist */
    }
    return SMPD_SUCCESS;
}

int smpd_validate_process_registry()
{
    int error;
    HKEY tkey;
    DWORD dwLen, result;
    int i;
    DWORD dwNumSubKeys, dwMaxSubKeyLen;
    char pid_str[100];
    int pid;
    HANDLE hTemp;

    smpd_enter_fn("smpd_validate_process_registry");

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process", 0, KEY_ALL_ACCESS, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_err_printf("Unable to open the smpd\\process registry key, error %d\n", result);
	    smpd_exit_fn("smpd_validate_process_registry");
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    result = RegQueryInfoKey(tkey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_validate_process_registry");
	return SMPD_FAIL;
    }
    if (dwMaxSubKeyLen > 100)
    {
	smpd_err_printf("Error: Invalid process subkeys, max length is too large: %d\n", dwMaxSubKeyLen);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_validate_process_registry");
	return SMPD_FAIL;
    }
    if (dwNumSubKeys == 0)
    {
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_validate_process_registry");
	return SMPD_SUCCESS;
    }
    /* count backwards so keys can be removed */
    for (i=dwNumSubKeys-1; i>=0; i--)
    {
	dwLen = 100;
	result = RegEnumKeyEx(tkey, i, pid_str, &dwLen, NULL, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS)
	{
	    smpd_err_printf("Error: Unable to enumerate the %d subkey in the smpd\\process registry key\n", i);
	    RegCloseKey(tkey);
	    smpd_exit_fn("smpd_validate_process_registry");
	    return SMPD_FAIL;
	}
	pid = atoi(pid_str);
	printf("pid = %d\n", pid);fflush(stdout);
	hTemp = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (hTemp == NULL)
	{
	    error = GetLastError();
	    if (error == ERROR_INVALID_PARAMETER)
	    {
		RegDeleteKey(tkey, pid_str);
	    }
	    /*
	    else
	    {
		printf("error = %d\n", error);
	    }
	    */
	}
	else
	{
	    CloseHandle(hTemp);
	}
    }
    RegCloseKey(tkey);
    smpd_exit_fn("smpd_validate_process_registry");
    return SMPD_SUCCESS;
}

int smpd_process_to_registry(smpd_process_t *process, char *actual_exe)
{
    HKEY tkey;
    DWORD len, result;
    char name[1024];

    smpd_enter_fn("smpd_process_to_registry");

    if (process == NULL)
    {
	smpd_dbg_printf("NULL process passed to smpd_process_to_registry.\n");
	return SMPD_FAIL;
    }

    len = snprintf(name, 1024, SMPD_REGISTRY_KEY "\\process\\%d", process->pid);
    if (len < 0 || len > 1024)
    {
	smpd_dbg_printf("unable to create a string of the registry key.\n");
	return SMPD_FAIL;
    }

    result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, name,
	0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &tkey, NULL);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to open the HKEY_LOCAL_MACHINE\\%s registry key, error %d\n", name, result);
	smpd_exit_fn("smpd_process_to_registry");
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(actual_exe)+1);
    result = RegSetValueEx(tkey, "exe", 0, REG_SZ, actual_exe, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to write the process registry value 'exe:%s', error %d\n", process->exe, result);
	RegCloseKey(tkey);
	smpd_exit_fn("smpd_process_to_registry");
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn("smpd_process_to_registry");
    return SMPD_SUCCESS;
}

int smpd_process_from_registry(smpd_process_t *process)
{
    DWORD len, result;
    char name[1024];

    smpd_enter_fn("smpd_process_from_registry");

    if (process == NULL)
	return SMPD_FAIL;

    len = snprintf(name, 1024, SMPD_REGISTRY_KEY "\\process\\%d", process->pid);
    if (len < 0 || len > 1024)
	return SMPD_FAIL;

    result = RegDeleteKey(HKEY_LOCAL_MACHINE, name);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to delete the HKEY_LOCAL_MACHINE\\%s registry key, error %d\n", name, result);
	smpd_exit_fn("smpd_process_from_registry");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_process_from_registry");
    return SMPD_SUCCESS;
}

int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    smpd_enter_fn("smpd_get_user_handle");

    if (domain)
	smpd_dbg_printf("LogonUser(%s\\%s)\n", domain, account);
    else
	smpd_dbg_printf("LogonUser(%s)\n", account);

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
		smpd_dbg_printf("env: %s=%s\n", name, value);
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
    if (name[0] != '\0')
    {
	smpd_dbg_printf("env: %s=%s\n", name, value);
	SetEnvironmentVariable(name, value);
    }
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
    if (name[0] != '\0')
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

#define USE_LAUNCH_THREADS

#ifdef USE_LAUNCH_THREADS

typedef struct smpd_piothread_arg_t
{
    HANDLE hIn;
    SOCKET hOut;
} smpd_piothread_arg_t;

static int smpd_easy_send(SOCKET sock, char *buffer, int length)
{
    int error;
    int num_sent;

    while ((num_sent = send(sock, buffer, length, 0)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	if (error == WSAEWOULDBLOCK)
	{
            Sleep(0);
	    continue;
	}
	if (error == WSAENOBUFS)
	{
	    /* If there is no buffer space available then split the buffer in half and send each piece separately.*/
	    if (smpd_easy_send(sock, buffer, length/2) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    if (smpd_easy_send(sock, buffer+(length/2), length - (length/2)) == SOCKET_ERROR)
		return SOCKET_ERROR;
	    return length;
	}
	WSASetLastError(error);
	return SOCKET_ERROR;
    }
    
    return length;
}

int smpd_piothread(smpd_piothread_arg_t *p)
{
    char buffer[1024];
    int num_read;
    HANDLE hIn;
    SOCKET hOut;

    hIn = p->hIn;
    hOut = p->hOut;
    free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_piothread ***\n");
    while (1)
    {
	if (!ReadFile(hIn, buffer, 1024, &num_read, NULL))
	{
	    smpd_dbg_printf("ReadFile failed, error %d\n", GetLastError());
	    break;
	}
	if (num_read < 1)
	{
	    smpd_dbg_printf("ReadFile returned %d bytes\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread read %d bytes ***\n", num_read);*/
	if (smpd_easy_send(hOut, buffer, num_read) == SOCKET_ERROR)
	{
	    smpd_dbg_printf("smpd_easy_send of %d bytes failed.\n", num_read);
	    break;
	}
	/*smpd_dbg_printf("*** smpd_piothread wrote %d bytes ***\n", num_read);*/
    }
    smpd_dbg_printf("*** smpd_piothread finishing ***\n");
    FlushFileBuffers((HANDLE)hOut);
    shutdown(hOut, SD_BOTH);
    closesocket(hOut);
    CloseHandle(hIn);
    /*smpd_dbg_printf("*** exiting smpd_piothread ***\n");*/
    return 0;
}

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    HANDLE hStdin, hStdout, hStderr;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    SOCKET hSockPmiR = INVALID_SOCKET, hSockPmiW = INVALID_SOCKET;
    /*HANDLE hPipeStdinR = NULL, hPipeStdinW = NULL;*/
    HANDLE hPipeStdoutR = NULL, hPipeStdoutW = NULL;
    HANDLE hPipeStderrR = NULL, hPipeStderrW = NULL;
    HANDLE hIn, hOut, hErr;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo;
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag;
    int nError, result;
    unsigned long blocking_flag;
    sock_t sock_in, sock_out, sock_err, sock_pmi;
    SECURITY_ATTRIBUTES saAttr;
    char str[8192], sock_str[20];
    BOOL bSuccess = TRUE;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    const char *args;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    int num_chars;

    smpd_enter_fn("smpd_launch_process");

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	args = smpd_get_string(process->exe, temp_exe, SMPD_MAX_EXE_LENGTH, &num_chars);
	smpd_dbg_printf("searching for '%s' in '%s'\n", temp_exe, process->path);
	if (smpd_search_path(process->path, temp_exe, SMPD_MAX_EXE_LENGTH, exe_data))
	{
	    if (args != NULL)
	    {
		strncat(exe_data, " ", SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		strncat(exe_data, args, SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		exe_data[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    }
	    actual_exe = exe_data;
	}
	else
	{
	    actual_exe = process->exe;
	}
    }
    else
    {
	actual_exe = process->exe;
    }

    smpd_priority_class_to_win_class(&priorityClass);
    smpd_priority_to_win_priority(&priority);

    /* Save stdin, stdout, and stderr */
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError(); /* This will only be correct if stderr failed */
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
    if (nError = smpd_make_socket_loop(&hSockPmiR, &hSockPmiW))
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;

    /* Create the pipes for stdout, stderr */
    if (!CreatePipe(&hPipeStdoutR, &hPipeStdoutW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stdout) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }
    if (!CreatePipe(&hPipeStderrR, &hPipeStderrW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stderr) failed, error %d\n", GetLastError());
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
    /*
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
    */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStdoutR, GetCurrentProcess(), &hOut, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStderrR, GetCurrentProcess(), &hErr, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockPmiR, GetCurrentProcess(), (LPHANDLE)&hSockPmiR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* prevent the socket loops from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutR, GetCurrentProcess(), (LPHANDLE)&hSockStdoutR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrR, GetCurrentProcess(), (LPHANDLE)&hSockStderrR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdoutW, GetCurrentProcess(), (LPHANDLE)&hSockStdoutW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStderrW, GetCurrentProcess(), (LPHANDLE)&hSockStderrW, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }

    /* make the ends used by the spawned process blocking */
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);
    */

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    /*
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
    */
    if (!SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)hPipeStdoutW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }
    if (!SetStdHandle(STD_ERROR_HANDLE, (HANDLE)hPipeStderrW))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto RESTORE_CLEANUP;
    }

    /* Create the process */
    memset(&saInfo, 0, sizeof(STARTUPINFO));
    saInfo.cb = sizeof(STARTUPINFO);
    saInfo.hStdInput = (HANDLE)hSockStdinR;
    /*
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.hStdError = (HANDLE)hSockStderrW;
    */
    saInfo.hStdOutput = (HANDLE)hPipeStdoutW;
    saInfo.hStdError = (HANDLE)hPipeStderrW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    sprintf(str, "%d", process->rank);
    smpd_dbg_printf("env: PMI_RANK=%s\n", str);
    SetEnvironmentVariable("PMI_RANK", str);
    sprintf(str, "%d", process->nproc);
    smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
    SetEnvironmentVariable("PMI_SIZE", str);
    sprintf(str, "%s", process->kvs_name);
    smpd_dbg_printf("env: PMI_KVS=%s\n", str);
    SetEnvironmentVariable("PMI_KVS", str);
    smpd_encode_handle(sock_str, (HANDLE)hSockPmiW);
    sprintf(str, "%s", sock_str);
    smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
    SetEnvironmentVariable("PMI_SMPD_FD", str);
    sprintf(str, "%d", smpd_process.id);
    smpd_dbg_printf("env: PMI_SMPD_ID=%s\n", str);
    SetEnvironmentVariable("PMI_SMPD_ID", str);
    sprintf(str, "%d", process->id);
    smpd_dbg_printf("env: PMI_SMPD_KEY=%s\n", str);
    SetEnvironmentVariable("PMI_SMPD_KEY", str);
    if (process->clique[0] != '\0')
    {
	sprintf(str, "%s", process->clique);
	smpd_dbg_printf("env: PMI_CLIQUE=%s\n", str);
	SetEnvironmentVariable("PMI_CLIQUE", str);
    }
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | CREATE_NO_WINDOW | priorityClass;
    if (dbg)
	launch_flag = launch_flag | DEBUG_PROCESS;

    smpd_dbg_printf("CreateProcess(%s)\n", actual_exe);
    psInfo.hProcess = INVALID_HANDLE_VALUE;
    if (CreateProcess(
	NULL,
	actual_exe,
	NULL, NULL, TRUE,
	launch_flag,
	pEnv,
	NULL,
	&saInfo, &psInfo))
    {
	SetThreadPriority(psInfo.hThread, priority);
	process->pid = psInfo.dwProcessId;
    }
    else
    {
	nError = GetLastError();
	smpd_err_printf("CreateProcess('%s') failed, error %d\n", process->exe, nError);
	/*snprintf(process->err_msg, SMPD_MAX_ERROR_LEN, "CreateProcess failed, error %d", nError);*/
	/*smpd_translate_win_error(nError, process->err_msg, SMPD_MAX_ERROR_LEN, "CreateProcess failed, error %d - ", nError);*/
	smpd_translate_win_error(nError, process->err_msg, SMPD_MAX_ERROR_LEN, "CreateProcess(%s) on '%s' failed, error %d - ",
	    process->exe, smpd_process.host, nError);
	psInfo.hProcess = INVALID_HANDLE_VALUE;
	bSuccess = FALSE;
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);
    SetEnvironmentVariable("PMI_RANK", NULL);
    SetEnvironmentVariable("PMI_SIZE", NULL);
    SetEnvironmentVariable("PMI_KVS", NULL);
    SetEnvironmentVariable("PMI_SMPD_FD", NULL);
    SetEnvironmentVariable("PMI_SMPD_ID", NULL);
    SetEnvironmentVariable("PMI_SMPD_KEY", NULL);

    if (bSuccess)
    {
	/* make sock structures out of the sockets */
	nError = sock_native_to_sock(set, hIn, NULL, &sock_in);
	if (nError != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	nError = sock_native_to_sock(set, (SOCK_NATIVE_FD)hSockStdoutR, NULL, &sock_out);
	if (nError != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	nError = sock_native_to_sock(set, (SOCK_NATIVE_FD)hSockStderrR, NULL, &sock_err);
	if (nError != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	nError = sock_native_to_sock(set, (SOCK_NATIVE_FD)hSockPmiR, NULL, &sock_pmi);
	if (nError != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}

	process->in->sock = sock_in;
	process->out->sock = sock_out;
	process->err->sock = sock_err;
	process->pmi->sock = sock_pmi;
	process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
	sock_set_user_ptr(sock_in, process->in);
	sock_set_user_ptr(sock_out, process->out);
	sock_set_user_ptr(sock_err, process->err);
	sock_set_user_ptr(sock_pmi, process->pmi);
    }
    else
    {
	/* close all the sockets and handles allocated in this function */
	CloseHandle(hIn);
	CloseHandle((HANDLE)hSockStdoutR);
	CloseHandle((HANDLE)hSockStderrR);
	CloseHandle((HANDLE)hSockPmiR);
    }

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    CloseHandle((HANDLE)hSockStdinR);
    /*
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);
    */
    CloseHandle((HANDLE)hPipeStdoutW);
    CloseHandle((HANDLE)hPipeStderrW);
    CloseHandle((HANDLE)hSockPmiW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	HANDLE hThread;
	smpd_piothread_arg_t *arg_ptr;

	arg_ptr = (smpd_piothread_arg_t*)malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hOut;
	arg_ptr->hOut = hSockStdoutW;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);
	arg_ptr = (smpd_piothread_arg_t*)malloc(sizeof(smpd_piothread_arg_t));
	arg_ptr->hIn = hErr;
	arg_ptr->hOut = hSockStderrW;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_piothread, arg_ptr, 0, NULL);
	CloseHandle(hThread);

	process->context_refcount = 3;
	process->out->read_state = SMPD_READING_STDOUT;
	result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	process->err->read_state = SMPD_READING_STDERR;
	result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	result = smpd_post_read_command(process->pmi);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the first command on the pmi control channel.\n");
	    smpd_exit_fn("smpd_launch_process");
	    return SMPD_FAIL;
	}
	process->wait = process->in->wait = process->out->wait = process->err->wait = psInfo.hProcess;
	smpd_process_to_registry(process, actual_exe);
	ResumeThread(psInfo.hThread);
	CloseHandle(psInfo.hThread);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
}

#else

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
	process->pid = psInfo.dwProcessId;
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
	process->wait = process->in->wait = process->out->wait = process->err->wait = psInfo.hProcess;
	CloseHandle(psInfo.hThread);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_SUCCESS;
    }

    smpd_exit_fn("smpd_launch_process");
    return SMPD_FAIL;
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

static void set_environment_variables(char *bEnv)
{
    char name[1024]="", value[8192]="";
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
		smpd_dbg_printf("env: %s=%s\n", name, value);
		setenv(name, value, 1);
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
    if (name[0] != '\0')
    {
	smpd_dbg_printf("env: %s=%s\n", name, value);
	setenv(name, value, 1);
    }
}

int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2];
    int  stderr_pipe_fds[2], pmi_pipe_fds[2];
    int pid;
    sock_t sock_in, sock_out, sock_err, sock_pmi;
    char args[SMPD_MAX_EXE_LENGTH];
    char *argv[1024];
    char *token;
    int i;
    char str[1024];
    const char *str_iter;
    int total, num_chars;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    const char *temp_str;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_launch_process");

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	temp_str = smpd_get_string(process->exe, temp_exe, SMPD_MAX_EXE_LENGTH, &num_chars);
	smpd_dbg_printf("searching for '%s' in '%s'\n", temp_exe, process->path);
	if (smpd_search_path(process->path, temp_exe, SMPD_MAX_EXE_LENGTH, exe_data))
	{
	    smpd_dbg_printf("found: '%s'\n", exe_data);
	    if (strstr(exe_data, " "))
	    {
		smpd_err_printf("Currently unable to handle paths with spaces in them.\n");
	    }
	    if (temp_str != NULL)
	    {
		strncat(exe_data, " ", SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		strncat(exe_data, temp_str, SMPD_MAX_EXE_LENGTH - strlen(exe_data));
		exe_data[SMPD_MAX_EXE_LENGTH-1] = '\0';
	    }
	    actual_exe = exe_data;
	}
	else
	{
	    actual_exe = process->exe;
	}
    }
    else
    {
	actual_exe = process->exe;
    }

    /* create argv from the command */
    i = 0;
    total = 0;
    /*str_iter = process->exe;*/
    str_iter = actual_exe;
    while (str_iter)
    {
	str_iter = smpd_get_string(str_iter, &args[total],
				   SMPD_MAX_EXE_LENGTH - total, &num_chars);
	argv[i] = &args[total];
	i++;
	total += num_chars+1; /* move over the null termination */
    }
    argv[i] = NULL;

    /* create pipes for redirecting I/O */
    /*
    pipe(stdin_pipe_fds);
    pipe(stdout_pipe_fds);
    pipe(stderr_pipe_fds);
    */
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdin_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stdout_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, stderr_pipe_fds);
    socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_pipe_fds);

    pid = fork();
    if (pid < 0)
    {
	smpd_err_printf("fork failed - error %d.\n", errno);
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }

    if (pid == 0)
    {
	/* child process */
	smpd_dbg_printf("client is alive and about to exec '%s'\n", argv[0]);

	sprintf(str, "%d", process->rank);
	smpd_dbg_printf("env: PMI_RANK=%s\n", str);
	setenv("PMI_RANK", str, 1);
	sprintf(str, "%d", process->nproc);
	smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
	setenv("PMI_SIZE", str, 1);
	sprintf(str, "%s", process->kvs_name);
	smpd_dbg_printf("env: PMI_KVS=%s\n", str);
	setenv("PMI_KVS", str, 1);
	sprintf(str, "%d", pmi_pipe_fds[1]);
	smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
	setenv("PMI_SMPD_FD", str, 1);
	sprintf(str, "%d", smpd_process.id);
	smpd_dbg_printf("env: PMI_SMPD_ID=%s\n", str);
	setenv("PMI_SMPD_ID", str, 1);
	sprintf(str, "%d", process->id);
	smpd_dbg_printf("env: PMI_SMPD_KEY=%s\n", str);
	setenv("PMI_SMPD_KEY", str, 1);

	set_environment_variables(process->env);

	result = dup2(stdin_pipe_fds[0], 0);   /* dup a new stdin */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stdin failed: %d\n", errno);
	}
	close(stdin_pipe_fds[0]);
	close(stdin_pipe_fds[1]);

	result = dup2(stdout_pipe_fds[1], 1);  /* dup a new stdout */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stdout failed: %d\n", errno);
	}
	close(stdout_pipe_fds[0]);
	close(stdout_pipe_fds[1]);

	result = dup2(stderr_pipe_fds[1], 2);  /* dup a new stderr */
	if (result == -1)
	{
	    smpd_err_printf("dup2 stderr failed: %d\n", errno);
	}
	close(stderr_pipe_fds[0]);
	close(stderr_pipe_fds[1]);

	close(pmi_pipe_fds[0]); /* close the other end */

	/* change the working directory */
	result = -1;
	if (process->dir[0] != '\0')
	    result = chdir( process->dir );
	if (result < 0)
	    chdir( getenv( "HOME" ) );

	/* reset the file mode creation mask */
	umask(0);

	result = execvp( argv[0], argv );

	result = errno;
	fprintf(stderr, "Unable to exec '%s'.\nError %d - %s", process->exe,
		result, strerror(result));
	sprintf(process->err_msg, "Error %d - %s", result, strerror(result));

	/* create the result command */
	result = smpd_create_command("abort", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an abort command in response to failed launch command: '%s'\n", process->exe);
	    exit(-1);
	}
	/* launch process should provide a reason for the error, for now just return FAIL */
	result = smpd_add_command_arg(cmd_ptr, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", process->exe);
	    exit(-1);
	}
	if (process->err_msg[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "error", process->err_msg);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the error field to the abort command in response to failed launch command: '%s'\n", process->exe);
		exit(-1);
	    }
	}

	/* send the result back */
	smpd_package_command(cmd_ptr);
	result = write(pmi_pipe_fds[1], cmd_ptr->cmd_hdr_str, SMPD_CMD_HDR_LENGTH);
	if (result != SMPD_CMD_HDR_LENGTH)
	{
	    smpd_err_printf("unable to write the abort command header in response to failed launch command: '%s'\n", process->exe);
	    exit(-1);
	}
	result = write(pmi_pipe_fds[1], cmd_ptr->cmd, cmd_ptr->length);
	if (result != cmd_ptr->length)
	{
	    smpd_err_printf("unable to write the abort command in response to failed launch command: '%s'\n", process->exe);
	    exit(-1);
	}

	/* send a closed message on the pmi socket? */

	exit(result);
    }

    /* parent process */
    process->pid = pid;
    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);
    close(stderr_pipe_fds[1]);
    close(pmi_pipe_fds[1]);

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
    result = sock_native_to_sock(set, pmi_pipe_fds[0], NULL, &sock_pmi);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    process->pmi->sock = sock_pmi;
    process->pid = process->in->id = process->out->id = process->err->id = pid;
    result = sock_set_user_ptr(sock_in, process->in);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_set_user_ptr(sock_out, process->out);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_set_user_ptr(sock_err, process->err);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = sock_set_user_ptr(sock_pmi, process->pmi);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }

    process->context_refcount = 3;
    process->out->read_state = SMPD_READING_STDOUT;
    result = sock_post_read(sock_out, process->out->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    process->err->read_state = SMPD_READING_STDERR;
    result = sock_post_read(sock_err, process->err->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_launch_process");
	return SMPD_FAIL;
    }
    result = smpd_post_read_command(process->pmi);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the first command on the pmi control context.\n");
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
    smpd_enter_fn("smpd_wait_process");

    if (WaitForSingleObject(wait, INFINITE) != WAIT_OBJECT_0)
    {
	smpd_err_printf("WaitForSingleObject failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn("smpd_wait_process");
	return SMPD_FAIL;
    }
    result = GetExitCodeProcess(wait, exit_code_ptr);
    if (!result)
    {
	smpd_err_printf("GetExitCodeProcess failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn("smpd_wait_process");
	return SMPD_FAIL;
    }
    CloseHandle(wait);

    smpd_exit_fn("smpd_wait_process");
    return SMPD_SUCCESS;
#else
    int status;
    smpd_enter_fn("smpd_wait_process");

    smpd_dbg_printf("waiting for process %d\n", wait);
    waitpid(wait, &status, WUNTRACED);
    if (WIFEXITED(status))
    {
	*exit_code_ptr =  WEXITSTATUS(status);
    }
    else
    {
	smpd_err_printf("WIFEXITED(%d) failed, setting exit code to -1\n", wait);
	*exit_code_ptr = -1;
    }

    smpd_exit_fn("smpd_wait_process");
    return SMPD_SUCCESS;
#endif
}

int smpd_kill_all_processes(void)
{
    smpd_process_t *iter;

    smpd_enter_fn("smpd_kill_all_processes");

    iter = smpd_process.process_list;
    while (iter)
    {
#ifdef HAVE_WINDOWS_H
	smpd_process_from_registry(iter);
	TerminateProcess(iter->wait, 8676);
#else
	kill(iter->wait, SIGKILL);
#endif
	iter = iter->next;
    }

    smpd_exit_fn("smpd_kill_all_processes");
    return SMPD_SUCCESS;
}

int smpd_exit(int exitcode)
{
    smpd_enter_fn("smpd_exit");
    smpd_kill_all_processes();
    smpd_finalize_printf();
#ifdef HAVE_WINDOWS_H
    /* This is necessary because exit() can deadlock flushing file buffers while the stdin thread is running */
    ExitProcess(exitcode);
#else
    exit(exitcode);
#endif
    smpd_exit_fn("smpd_exit");
    return SMPD_SUCCESS;
}
