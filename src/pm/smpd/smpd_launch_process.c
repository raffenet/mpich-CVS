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

#undef FCNAME
#define FCNAME "smpd_clear_process_registry"
int smpd_clear_process_registry()
{
#if 0
    if (SHDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process") != ERROR_SUCCESS)
    {
	/* It's ok if the key does not exist */
    }
    return SMPD_SUCCESS;
#endif
    HKEY tkey;
    DWORD dwLen, result;
    int i;
    DWORD dwNumSubKeys, dwMaxSubKeyLen;
    char pid_str[256];

    smpd_enter_fn(FCNAME);

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process", 0, KEY_ALL_ACCESS, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_err_printf("Unable to open the " SMPD_REGISTRY_KEY "\\process registry key, error %d\n", result);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    result = RegQueryInfoKey(tkey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwMaxSubKeyLen > 256)
    {
	smpd_err_printf("Error: Invalid process subkeys, max length is too large: %d\n", dwMaxSubKeyLen);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwNumSubKeys == 0)
    {
	RegCloseKey(tkey);
	RegDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process");
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    /* count backwards so keys can be removed */
    for (i=dwNumSubKeys-1; i>=0; i--)
    {
	dwLen = 256;
	result = RegEnumKeyEx(tkey, i, pid_str, &dwLen, NULL, NULL, NULL, NULL);
	if (result != ERROR_SUCCESS)
	{
	    smpd_err_printf("Error: Unable to enumerate the %d subkey in the " SMPD_REGISTRY_KEY "\\process registry key\n", i);
	    RegCloseKey(tkey);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	RegDeleteKey(tkey, pid_str);
    }
    RegCloseKey(tkey);
    RegDeleteKey(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process");
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_validate_process_registry"
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

    smpd_enter_fn(FCNAME);

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SMPD_REGISTRY_KEY "\\process", 0, KEY_ALL_ACCESS, &tkey);
    if (result != ERROR_SUCCESS)
    {
	if (result != ERROR_PATH_NOT_FOUND)
	{
	    smpd_err_printf("Unable to open the smpd\\process registry key, error %d\n", result);
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    result = RegQueryInfoKey(tkey, NULL, NULL, NULL, &dwNumSubKeys, &dwMaxSubKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);
    if (result != ERROR_SUCCESS)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwMaxSubKeyLen > 100)
    {
	smpd_err_printf("Error: Invalid process subkeys, max length is too large: %d\n", dwMaxSubKeyLen);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (dwNumSubKeys == 0)
    {
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
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
	    smpd_exit_fn(FCNAME);
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
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_process_to_registry"
int smpd_process_to_registry(smpd_process_t *process, char *actual_exe)
{
    HKEY tkey;
    DWORD len, result;
    char name[1024];

    smpd_enter_fn(FCNAME);

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
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    len = (DWORD)(strlen(actual_exe)+1);
    result = RegSetValueEx(tkey, "exe", 0, REG_SZ, (const BYTE *)actual_exe, len);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to write the process registry value 'exe:%s', error %d\n", process->exe, result);
	RegCloseKey(tkey);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    RegCloseKey(tkey);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_process_from_registry"
int smpd_process_from_registry(smpd_process_t *process)
{
    DWORD len, result;
    char name[1024];

    smpd_enter_fn(FCNAME);

    if (process == NULL)
	return SMPD_FAIL;

    len = snprintf(name, 1024, SMPD_REGISTRY_KEY "\\process\\%d", process->pid);
    if (len < 0 || len > 1024)
	return SMPD_FAIL;

    result = RegDeleteKey(HKEY_LOCAL_MACHINE, name);
    if (result != ERROR_SUCCESS)
    {
	smpd_err_printf("Unable to delete the HKEY_LOCAL_MACHINE\\%s registry key, error %d\n", name, result);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_user_handle"
int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr)
{
    HANDLE hUser;
    int error;
    int num_tries = 3;

    smpd_enter_fn(FCNAME);

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
		smpd_exit_fn(FCNAME);
		return error;
	    }
	    num_tries--;
	}
	else
	{
	    *handle_ptr = INVALID_HANDLE_VALUE;
	    smpd_exit_fn(FCNAME);
	    return error;
	}
    }

    *handle_ptr = hUser;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

/*
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
*/

static void SetEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH], equals[3], value[MAX_PATH];

    for (;;)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, MAX_PATH) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, MAX_PATH) != MPIU_STR_SUCCESS)
	    break;
	smpd_dbg_printf("setting environment variable: <%s> = <%s>\n", name, value);
	SetEnvironmentVariable(name, value);
    }
}

static void RemoveEnvironmentVariables(char *bEnv)
{
    char name[MAX_PATH], equals[3], value[MAX_PATH];

    for (;;)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, MAX_PATH) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, MAX_PATH) != MPIU_STR_SUCCESS)
	    break;
	/*smpd_dbg_printf("removing environment variable <%s>\n", name);*/
	SetEnvironmentVariable(name, NULL);
    }
}

int smpd_priority_class_to_win_class(int *priorityClass)
{
    switch (*priorityClass)
    {
    case 0:
	*priorityClass = IDLE_PRIORITY_CLASS;
	break;
    case 1:
	*priorityClass = BELOW_NORMAL_PRIORITY_CLASS;
	break;
    case 2:
	*priorityClass = NORMAL_PRIORITY_CLASS;
	break;
    case 3:
	*priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
	break;
    case 4:
	*priorityClass = HIGH_PRIORITY_CLASS;
	break;
    default:
	*priorityClass = NORMAL_PRIORITY_CLASS;
	break;
    }
    return SMPD_SUCCESS;
}

int smpd_priority_to_win_priority(int *priority)
{
    switch (*priority)
    {
    case 0:
	*priority = THREAD_PRIORITY_IDLE;
	break;
    case 1:
	*priority = THREAD_PRIORITY_LOWEST;
	break;
    case 2:
	*priority = THREAD_PRIORITY_BELOW_NORMAL;
	break;
    case 3:
	*priority = THREAD_PRIORITY_NORMAL;
	break;
    case 4:
	*priority = THREAD_PRIORITY_ABOVE_NORMAL;
	break;
    case 5:
	*priority = THREAD_PRIORITY_HIGHEST;
	break;
    default:
	*priority = THREAD_PRIORITY_NORMAL;
	break;
    }
    return SMPD_SUCCESS;
}

/* Windows code */

typedef struct smpd_piothread_arg_t
{
    HANDLE hIn;
    SOCKET hOut;
} smpd_piothread_arg_t;

typedef struct smpd_pinthread_arg_t
{
    SOCKET hIn;
    HANDLE hOut;
} smpd_pinthread_arg_t;

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
    char buffer[8192];
    DWORD num_read;
    HANDLE hIn;
    SOCKET hOut;
    DWORD error;

    hIn = p->hIn;
    hOut = p->hOut;
    free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_piothread ***\n");
    for (;;)
    {
	num_read = 0;
	if (!ReadFile(hIn, buffer, 8192, &num_read, NULL))
	{
	    error = GetLastError();
	    /* If there was an error but some bytes were read, send those bytes before exiting */
	    if (num_read > 0)
	    {
		if (smpd_easy_send(hOut, buffer, num_read) == SOCKET_ERROR)
		{
		    smpd_dbg_printf("smpd_easy_send of %d bytes failed.\n", num_read);
		    break;
		}
	    }
	    smpd_dbg_printf("ReadFile failed, error %d\n", error);
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

/* one line at a time version */
int smpd_pinthread(smpd_pinthread_arg_t *p)
{
    char str [SMPD_MAX_CMD_LENGTH];
    int index;
    DWORD num_written;
    SOCKET hIn;
    HANDLE hOut;
    /*int i;*/

    hIn = p->hIn;
    hOut = p->hOut;
    free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_pinthread ***\n");
    index = 0;
    for (;;)
    {
	if (recv(hIn, &str[index], 1, 0) == SOCKET_ERROR)
	{
	    if (index > 0)
	    {
		/* write any buffered data before exiting */
		if (!WriteFile(hOut, str, index, &num_written, NULL))
		{
		    smpd_dbg_printf("WriteFile failed, error %d\n", GetLastError());
		    break;
		}
	    }
	    smpd_dbg_printf("recv from stdin socket failed, error %d.\n", WSAGetLastError());
	    break;
	}
	if (str[index] == '\n' || index == SMPD_MAX_CMD_LENGTH-1)
	{
	    smpd_dbg_printf("writing %d bytes to the process's stdin\n", index+1);
	    if (!WriteFile(hOut, str, index+1, &num_written, NULL))
	    {
		smpd_dbg_printf("WriteFile failed, error %d\n", GetLastError());
		break;
	    }
	    /*
	    smpd_dbg_printf("wrote: ");
	    for (i=0; i<=index; i++)
	    {
		smpd_dbg_printf("(%d)'%c'", (int)str[i], str[i]);
	    }
	    smpd_dbg_printf("\n");
	    */
	    index = 0;
	}
	else
	{
	    smpd_dbg_printf("read character(%d)'%c'\n", (int)str[index], str[index]);
	    index++;
	}
    }
    smpd_dbg_printf("*** smpd_pinthread finishing ***\n");
    FlushFileBuffers(hOut);
    closesocket(hIn);
    CloseHandle(hOut);
    /*smpd_dbg_printf("*** exiting smpd_pinthread ***\n");*/
    return 0;
}
#if 0
/* 1 byte at a time version */
int smpd_pinthread(smpd_pinthread_arg_t *p)
{
    char ch;
    DWORD num_written;
    SOCKET hIn;
    HANDLE hOut;

    hIn = p->hIn;
    hOut = p->hOut;
    free(p);
    p = NULL;

    smpd_dbg_printf("*** entering smpd_pinthread ***\n");
    for (;;)
    {
	if (recv(hIn, &ch, 1, 0) == SOCKET_ERROR)
	{
	    smpd_dbg_printf("recv from stdin socket failed, error %d.\n", WSAGetLastError());
	    break;
	}
	if (!WriteFile(hOut, &ch, 1, &num_written, NULL))
	{
	    smpd_dbg_printf("WriteFile failed, error %d\n", GetLastError());
	    break;
	}
    }
    smpd_dbg_printf("*** smpd_pinthread finishing ***\n");
    FlushFileBuffers(hOut);
    closesocket(hIn);
    CloseHandle(hOut);
    /*smpd_dbg_printf("*** exiting smpd_pinthread ***\n");*/
    return 0;
}
#endif

#undef FCNAME
#define FCNAME "smpd_launch_process"
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, MPIDU_Sock_set_t set)
{
    HANDLE hStdin = INVALID_HANDLE_VALUE, hStdout = INVALID_HANDLE_VALUE, hStderr = INVALID_HANDLE_VALUE;
    SOCKET hSockStdinR = INVALID_SOCKET, hSockStdinW = INVALID_SOCKET;
    SOCKET hSockStdoutR = INVALID_SOCKET, hSockStdoutW = INVALID_SOCKET;
    SOCKET hSockStderrR = INVALID_SOCKET, hSockStderrW = INVALID_SOCKET;
    SOCKET hSockPmiR = INVALID_SOCKET, hSockPmiW = INVALID_SOCKET;
    HANDLE hPipeStdinR = NULL, hPipeStdinW = NULL;
    HANDLE hPipeStdoutR = NULL, hPipeStdoutW = NULL;
    HANDLE hPipeStderrR = NULL, hPipeStderrW = NULL;
    HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE, hErr = INVALID_HANDLE_VALUE;
    STARTUPINFO saInfo;
    PROCESS_INFORMATION psInfo = { 0 };
    void *pEnv=NULL;
    char tSavedPath[MAX_PATH] = ".";
    DWORD launch_flag = 0;
    int nError = 0, result = 0;
    unsigned long blocking_flag = 0;
    MPIDU_Sock_t sock_in = MPIDU_SOCK_INVALID_SOCK, sock_out = MPIDU_SOCK_INVALID_SOCK, sock_err = MPIDU_SOCK_INVALID_SOCK, sock_pmi = MPIDU_SOCK_INVALID_SOCK;
    SECURITY_ATTRIBUTES saAttr;
    char str[8192], sock_str[20];
    BOOL bSuccess = TRUE;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    char *args;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    MPIDU_Sock_t sock_pmi_listener;
    smpd_context_t *listener_context;
    int listener_port = 0;
    char host_description[256];

    smpd_enter_fn(FCNAME);

    /* Initialize the psInfo structure in case there is an error an we jump to CLEANUP before psInfo is set */
    psInfo.hProcess = INVALID_HANDLE_VALUE;

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	args = process->exe;
	result = MPIU_Str_get_string(&args, temp_exe, SMPD_MAX_EXE_LENGTH);
	if (result != MPIU_STR_SUCCESS)
	{
	}
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
    WaitForSingleObject(smpd_process.hLaunchProcessMutex, INFINITE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStderr = GetStdHandle(STD_ERROR_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE  || hStderr == INVALID_HANDLE_VALUE)
    {
	nError = GetLastError(); /* This will only be correct if stderr failed */
	ReleaseMutex(smpd_process.hLaunchProcessMutex);
	smpd_err_printf("GetStdHandle failed, error %d\n", nError);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;;
    }

    /* Create sockets for stdin, stdout, and stderr */
    nError = smpd_make_socket_loop_choose(&hSockStdinR, SMPD_FALSE, &hSockStdinW, SMPD_TRUE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    nError = smpd_make_socket_loop_choose(&hSockStdoutR, SMPD_TRUE, &hSockStdoutW, SMPD_FALSE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    nError = smpd_make_socket_loop_choose(&hSockStderrR, SMPD_TRUE, &hSockStderrW, SMPD_FALSE);
    if (nError)
    {
	smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
	goto CLEANUP;
    }
    if (process->pmi != NULL)
    {
	if (smpd_process.use_inherited_handles)
	{
	    nError = smpd_make_socket_loop(&hSockPmiR, &hSockPmiW);
	    if (nError)
	    {
		smpd_err_printf("smpd_make_socket_loop failed, error %d\n", nError);
		goto CLEANUP;
	    }
	}
	else
	{
	    nError = MPIDU_Sock_listen(set, NULL, &listener_port, &sock_pmi_listener); 
	    if (nError != MPI_SUCCESS)
	    {
		/* If another smpd is running and listening on this port, tell it to shutdown or restart? */
		smpd_err_printf("MPIDU_Sock_listen failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    smpd_dbg_printf("pmi listening on port %d\n", listener_port);

	    nError = smpd_create_context(SMPD_CONTEXT_PMI_LISTENER, set, sock_pmi_listener, -1, &listener_context);
	    if (nError != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a context for the pmi listener.\n");
		goto CLEANUP;
	    }
	    nError = MPIDU_Sock_set_user_ptr(sock_pmi_listener, listener_context);
	    if (nError != MPI_SUCCESS)
	    {
		smpd_err_printf("MPIDU_Sock_set_user_ptr failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    listener_context->state = SMPD_PMI_LISTENING;
	    nError = MPIDU_Sock_get_host_description(host_description, 256);
	    if (nError != MPI_SUCCESS)
	    {
		smpd_err_printf("MPIDU_Sock_get_host_description failed,\nsock error: %s\n", get_sock_error_string(nError));
		goto CLEANUP;
	    }
	    /* save the listener sock in the slot reserved for the client sock so we can match the listener
	    to the process structure when the client sock is accepted */
	    process->pmi->sock = sock_pmi_listener;
	}
    }

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = NULL;
    saAttr.bInheritHandle = TRUE;

    /* Create the pipes for stdout, stderr */
    if (!CreatePipe(&hPipeStdinR, &hPipeStdinW, &saAttr, 0))
    {
	smpd_err_printf("CreatePipe(stdin) failed, error %d\n", GetLastError());
	goto CLEANUP;
    }
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
    /*
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), &hIn, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hPipeStdinW, GetCurrentProcess(), &hIn, 
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
    if (process->pmi != NULL && smpd_process.use_inherited_handles)
    {
	if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockPmiR, GetCurrentProcess(), (LPHANDLE)&hSockPmiR, 
	    0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
	{
	    nError = GetLastError();
	    smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	    goto CLEANUP;
	}
    }

    /* prevent the socket loops from being inherited */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinR, GetCurrentProcess(), (LPHANDLE)&hSockStdinR, 
	0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS))
    {
	nError = GetLastError();
	smpd_err_printf("DuplicateHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
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
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)hSockStdinW, GetCurrentProcess(), (LPHANDLE)&hSockStdinW, 
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
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdinR, FIONBIO, &blocking_flag);
    */
    /*
    blocking_flag = 0;
    ioctlsocket(hSockStdoutW, FIONBIO, &blocking_flag);
    blocking_flag = 0;
    ioctlsocket(hSockStderrW, FIONBIO, &blocking_flag);
    */

    /* Set stdin, stdout, and stderr to the ends of the pipe the created process will use */
    /*
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hSockStdinR))
    {
	nError = GetLastError();
	smpd_err_printf("SetStdHandle failed, error %d\n", nError);
	goto CLEANUP;
    }
    */
    if (!SetStdHandle(STD_INPUT_HANDLE, (HANDLE)hPipeStdinR))
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
    /*saInfo.hStdInput = (HANDLE)hSockStdinR;*/
    saInfo.hStdInput = (HANDLE)hPipeStdinR;
    /*
    saInfo.hStdOutput = (HANDLE)hSockStdoutW;
    saInfo.hStdError = (HANDLE)hSockStderrW;
    */
    saInfo.hStdOutput = (HANDLE)hPipeStdoutW;
    saInfo.hStdError = (HANDLE)hPipeStderrW;
    saInfo.dwFlags = STARTF_USESTDHANDLES;

    SetEnvironmentVariables(process->env);
    if (process->pmi != NULL)
    {
	sprintf(str, "%d", process->rank);
	smpd_dbg_printf("env: PMI_RANK=%s\n", str);
	SetEnvironmentVariable("PMI_RANK", str);
	sprintf(str, "%d", process->nproc);
	smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
	SetEnvironmentVariable("PMI_SIZE", str);
	sprintf(str, "%s", process->kvs_name);
	smpd_dbg_printf("env: PMI_KVS=%s\n", str);
	SetEnvironmentVariable("PMI_KVS", str);
	sprintf(str, "%s", process->domain_name);
	smpd_dbg_printf("env: PMI_DOMAIN=%s\n", str);
	SetEnvironmentVariable("PMI_DOMAIN", str);
	if (smpd_process.use_inherited_handles)
	{
	    smpd_encode_handle(sock_str, (HANDLE)hSockPmiW);
	    sprintf(str, "%s", sock_str);
	    smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
	    SetEnvironmentVariable("PMI_SMPD_FD", str);
	}
	else
	{
	    smpd_dbg_printf("env: PMI_HOST=%s\n", host_description);
	    SetEnvironmentVariable("PMI_HOST", host_description);
	    sprintf(str, "%d", listener_port);
	    smpd_dbg_printf("env: PMI_PORT=%s\n", str);
	    SetEnvironmentVariable("PMI_PORT", str);
	}
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
	sprintf(str, "%d", process->spawned);
	smpd_dbg_printf("env: PMI_SPAWN=%s\n", str);
	SetEnvironmentVariable("PMI_SPAWN", str);
	sprintf(str, "%d", process->appnum);
	smpd_dbg_printf("env: PMI_APPNUM=%s\n", str);
	SetEnvironmentVariable("PMI_APPNUM", str);
    }
    pEnv = GetEnvironmentStrings();

    GetCurrentDirectory(MAX_PATH, tSavedPath);
    SetCurrentDirectory(process->dir);

    launch_flag = 
	CREATE_SUSPENDED | /*CREATE_NEW_CONSOLE*/ /*DETACHED_PROCESS*/ CREATE_NO_WINDOW | priorityClass;
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
	smpd_translate_win_error(nError, process->err_msg, SMPD_MAX_ERROR_LEN, "CreateProcess(%s) on '%s' failed, error %d - ",
	    process->exe, smpd_process.host, nError);
	psInfo.hProcess = INVALID_HANDLE_VALUE;
	bSuccess = FALSE;
    }

    FreeEnvironmentStrings((TCHAR*)pEnv);
    SetCurrentDirectory(tSavedPath);
    RemoveEnvironmentVariables(process->env);
    if (process->pmi != NULL)
    {
	SetEnvironmentVariable("PMI_RANK", NULL);
	SetEnvironmentVariable("PMI_SIZE", NULL);
	SetEnvironmentVariable("PMI_KVS", NULL);
	SetEnvironmentVariable("PMI_DOMAIN", NULL);
	if (smpd_process.use_inherited_handles)
	{
	    SetEnvironmentVariable("PMI_SMPD_FD", NULL);
	}
	else
	{
	    SetEnvironmentVariable("PMI_HOST", NULL);
	    SetEnvironmentVariable("PMI_PORT", NULL);
	}
	SetEnvironmentVariable("PMI_SMPD_ID", NULL);
	SetEnvironmentVariable("PMI_SMPD_KEY", NULL);
	SetEnvironmentVariable("PMI_SPAWN", NULL);
    }

    if (bSuccess)
    {
	/* make sock structures out of the sockets */
	/*
	nError = MPIDU_Sock_native_to_sock(set, hIn, NULL, &sock_in);
	if (nError != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	*/
	nError = MPIDU_Sock_native_to_sock(set, (MPIDU_SOCK_NATIVE_FD)hSockStdinW, NULL, &sock_in);
	if (nError != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	nError = MPIDU_Sock_native_to_sock(set, (MPIDU_SOCK_NATIVE_FD)hSockStdoutR, NULL, &sock_out);
	if (nError != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	nError = MPIDU_Sock_native_to_sock(set, (MPIDU_SOCK_NATIVE_FD)hSockStderrR, NULL, &sock_err);
	if (nError != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	}
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	{
	    nError = MPIDU_Sock_native_to_sock(set, (MPIDU_SOCK_NATIVE_FD)hSockPmiR, NULL, &sock_pmi);
	    if (nError != MPI_SUCCESS)
	    {
		smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(nError));
	    }
	}

	process->in->sock = sock_in;
	process->out->sock = sock_out;
	process->err->sock = sock_err;
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    process->pmi->sock = sock_pmi;
	process->pid = process->in->id = process->out->id = process->err->id = psInfo.dwProcessId;
	MPIDU_Sock_set_user_ptr(sock_in, process->in);
	MPIDU_Sock_set_user_ptr(sock_out, process->out);
	MPIDU_Sock_set_user_ptr(sock_err, process->err);
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    MPIDU_Sock_set_user_ptr(sock_pmi, process->pmi);
    }
    else
    {
	/* close all the sockets and handles allocated in this function */
	/*CloseHandle(hIn);*/
	CloseHandle((HANDLE)hSockStdinW);
	CloseHandle((HANDLE)hSockStdoutR);
	CloseHandle((HANDLE)hSockStderrR);
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    CloseHandle((HANDLE)hSockPmiR);
    }

RESTORE_CLEANUP:
    /* Restore stdin, stdout, stderr */
    SetStdHandle(STD_INPUT_HANDLE, hStdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
    SetStdHandle(STD_ERROR_HANDLE, hStderr);

CLEANUP:
    ReleaseMutex(smpd_process.hLaunchProcessMutex);
    /*CloseHandle((HANDLE)hSockStdinR);*/
    CloseHandle((HANDLE)hPipeStdinR);
    /*
    CloseHandle((HANDLE)hSockStdoutW);
    CloseHandle((HANDLE)hSockStderrW);
    */
    CloseHandle((HANDLE)hPipeStdoutW);
    CloseHandle((HANDLE)hPipeStderrW);
    if (process->pmi != NULL && smpd_process.use_inherited_handles)
	CloseHandle((HANDLE)hSockPmiW);

    if (psInfo.hProcess != INVALID_HANDLE_VALUE)
    {
	HANDLE hThread;
	smpd_piothread_arg_t *arg_ptr;
	smpd_pinthread_arg_t *in_arg_ptr;

	in_arg_ptr = (smpd_pinthread_arg_t*)malloc(sizeof(smpd_pinthread_arg_t));
	in_arg_ptr->hIn = hSockStdinR;
	in_arg_ptr->hOut = hPipeStdinW;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_pinthread, in_arg_ptr, 0, NULL);
	CloseHandle(hThread);
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

	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	    process->context_refcount = 3;
	else
	    process->context_refcount = 2;
	process->out->read_state = SMPD_READING_STDOUT;
	result = MPIDU_Sock_post_read(sock_out, process->out->read_cmd.cmd, 1, 1, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	process->err->read_state = SMPD_READING_STDERR;
	result = MPIDU_Sock_post_read(sock_err, process->err->read_cmd.cmd, 1, 1, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
	if (process->pmi != NULL && smpd_process.use_inherited_handles)
	{
	    result = smpd_post_read_command(process->pmi);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a read of the first command on the pmi control channel.\n");
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
	    }
	}
	process->wait.hProcess = process->in->wait.hProcess = process->out->wait.hProcess = process->err->wait.hProcess = psInfo.hProcess;
	process->wait.hThread = process->in->wait.hThread = process->out->wait.hThread = process->err->wait.hThread = psInfo.hThread;
	smpd_process_to_registry(process, actual_exe);
	ResumeThread(psInfo.hThread);
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_parse_account_domain"
void smpd_parse_account_domain(char *domain_account, char *account, char *domain)
{
    char *pCh, *pCh2;

    smpd_enter_fn(FCNAME);

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

    smpd_exit_fn(FCNAME);
}

#else

/* Unix code */

/*
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
*/

static void set_environment_variables(char *bEnv)
{
    char name[1024], equals[3], value[8192];

    while (1)
    {
	name[0] = '\0';
	equals[0] = '\0';
	value[0] = '\0';
	if (MPIU_Str_get_string(&bEnv, name, 1024) != MPIU_STR_SUCCESS)
	    break;
	if (name[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, equals, 3) != MPIU_STR_SUCCESS)
	    break;
	if (equals[0] == '\0')
	    break;
	if (MPIU_Str_get_string(&bEnv, value, 8192) != MPIU_STR_SUCCESS)
	    break;
	setenv(name, value, 1);
    }
}

#undef FCNAME
#define FCNAME "smpd_launch_process"
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, MPIDU_Sock_set_t set)
{
    int result;
    int stdin_pipe_fds[2], stdout_pipe_fds[2];
    int  stderr_pipe_fds[2], pmi_pipe_fds[2];
    int pid;
    MPIDU_Sock_t sock_in, sock_out, sock_err, sock_pmi;
    char args[SMPD_MAX_EXE_LENGTH];
    char *argv[1024];
    char *token;
    int i;
    char str[1024];
    char *str_iter;
    int total, num_chars;
    char *actual_exe, exe_data[SMPD_MAX_EXE_LENGTH];
    char *temp_str;
    char temp_exe[SMPD_MAX_EXE_LENGTH];
    smpd_command_t *cmd_ptr;

    smpd_enter_fn(FCNAME);

    /* resolve the executable name */
    if (process->path[0] != '\0')
    {
	temp_str = process->exe;
	result = MPIU_Str_get_string(&temp_str, temp_exe, SMPD_MAX_EXE_LENGTH);
	if (result != MPIU_STR_SUCCESS)
	{
	}
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
    while (str_iter && i<1024)
    {
	result = MPIU_Str_get_string(&str_iter, &args[total],
				   SMPD_MAX_EXE_LENGTH - total);
	argv[i] = &args[total];
	i++;
	total += strlen(&args[total])+1; /* move over the null termination */
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
    if (process->pmi != NULL)
    {
	socketpair(AF_UNIX, SOCK_STREAM, 0, pmi_pipe_fds);
    }

    pid = fork();
    if (pid < 0)
    {
	smpd_err_printf("fork failed - error %d.\n", errno);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    if (pid == 0)
    {
	/* child process */
	smpd_dbg_printf("client is alive and about to exec '%s'\n", argv[0]);

	if (process->pmi != NULL)
	{
	    sprintf(str, "%d", process->rank);
	    smpd_dbg_printf("env: PMI_RANK=%s\n", str);
	    setenv("PMI_RANK", str, 1);
	    sprintf(str, "%d", process->nproc);
	    smpd_dbg_printf("env: PMI_SIZE=%s\n", str);
	    setenv("PMI_SIZE", str, 1);
	    sprintf(str, "%s", process->kvs_name);
	    smpd_dbg_printf("env: PMI_KVS=%s\n", str);
	    setenv("PMI_KVS", str, 1);
	    sprintf(str, "%s", process->domain_name);
	    smpd_dbg_printf("env: PMI_DOMAIN=%s\n", str);
	    setenv("PMI_DOMAIN", str, 1);
	    sprintf(str, "%d", pmi_pipe_fds[1]);
	    smpd_dbg_printf("env: PMI_SMPD_FD=%s\n", str);
	    setenv("PMI_SMPD_FD", str, 1);
	    sprintf(str, "%d", smpd_process.id);
	    smpd_dbg_printf("env: PMI_SMPD_ID=%s\n", str);
	    setenv("PMI_SMPD_ID", str, 1);
	    sprintf(str, "%d", process->id);
	    smpd_dbg_printf("env: PMI_SMPD_KEY=%s\n", str);
	    setenv("PMI_SMPD_KEY", str, 1);
	    sprintf(str, "%d", process->spawned);
	    smpd_dbg_printf("env: PMI_SPAWN=%s\n", str);
	    setenv("PMI_SPAWN", str, 1);
	    sprintf(str, "%d", process->appnum);
	    smpd_dbg_printf("env: PMI_APPNUM=%s\n", str);
	    setenv("PMI_APPNUM", str, 1);
	}
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

	if (process->pmi != NULL)
	{
	    close(pmi_pipe_fds[0]); /* close the other end */
	}

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
	/*fprintf(stderr, "Unable to exec '%s'.\nError %d - %s\n", process->exe, result, strerror(result));*/
	sprintf(process->err_msg, "Error %d - %s", result, strerror(result));

	if (process->pmi != NULL)
	{
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
	}

	exit(result);
    }

    /* parent process */
    process->pid = pid;
    close(stdin_pipe_fds[0]);
    close(stdout_pipe_fds[1]);
    close(stderr_pipe_fds[1]);
    if (process->pmi != NULL)
    {
	close(pmi_pipe_fds[1]);
    }

    /* make sock structures out of the sockets */
    result = MPIDU_Sock_native_to_sock(set, stdin_pipe_fds[1], NULL, &sock_in);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = MPIDU_Sock_native_to_sock(set, stdout_pipe_fds[0], NULL, &sock_out);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    result = MPIDU_Sock_native_to_sock(set, stderr_pipe_fds[0], NULL, &sock_err);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
    }
    if (process->pmi != NULL)
    {
	result = MPIDU_Sock_native_to_sock(set, pmi_pipe_fds[0], NULL, &sock_pmi);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_native_to_sock failed, error %s\n", get_sock_error_string(result));
	}
    }
    process->in->sock = sock_in;
    process->out->sock = sock_out;
    process->err->sock = sock_err;
    if (process->pmi != NULL)
    {
	process->pmi->sock = sock_pmi;
    }
    process->pid = process->in->id = process->out->id = process->err->id = pid;
    result = MPIDU_Sock_set_user_ptr(sock_in, process->in);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = MPIDU_Sock_set_user_ptr(sock_out, process->out);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    result = MPIDU_Sock_set_user_ptr(sock_err, process->err);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("MPIDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
    }
    if (process->pmi != NULL)
    {
	result = MPIDU_Sock_set_user_ptr(sock_pmi, process->pmi);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_set_user_ptr failed, error %s\n", get_sock_error_string(result));
	}
    }

    process->context_refcount = (process->pmi != NULL) ? 3 : 2;
    process->out->read_state = SMPD_READING_STDOUT;
    result = MPIDU_Sock_post_read(sock_out, process->out->read_cmd.cmd, 1, 1, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("posting first read from stdout context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    process->err->read_state = SMPD_READING_STDERR;
    result = MPIDU_Sock_post_read(sock_err, process->err->read_cmd.cmd, 1, 1, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("posting first read from stderr context failed, sock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    if (process->pmi != NULL)
    {
	result = smpd_post_read_command(process->pmi);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the first command on the pmi control context.\n");
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	}
    }
    process->wait = process->in->wait = process->out->wait = process->err->wait = pid;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#endif

#undef FCNAME
#define FCNAME "smpd_wait_process"
int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr)
{
#ifdef HAVE_WINDOWS_H
    int result;
    DWORD exit_code;
    smpd_enter_fn(FCNAME);

    if (wait.hProcess == INVALID_HANDLE_VALUE || wait.hProcess == NULL)
    {
	smpd_dbg_printf("No process to wait for.\n");
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }
    if (WaitForSingleObject(wait.hProcess, INFINITE) != WAIT_OBJECT_0)
    {
	smpd_err_printf("WaitForSingleObject failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = GetExitCodeProcess(wait.hProcess, &exit_code);
    if (!result)
    {
	smpd_err_printf("GetExitCodeProcess failed, error %d\n", GetLastError());
	*exit_code_ptr = -1;
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    CloseHandle(wait.hProcess);
    CloseHandle(wait.hThread);

    *exit_code_ptr = exit_code;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int status;
    smpd_enter_fn(FCNAME);

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

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_suspend_process"
int smpd_suspend_process(smpd_process_t *process)
{
#ifdef HAVE_WINDOWS_H
    int result = SMPD_SUCCESS;
    smpd_enter_fn(FCNAME);

    if (SuspendThread(process->wait.hThread) == -1)
    {
	result = GetLastError();
	smpd_err_printf("SuspendThread failed with error %d for process %d:%s:'%s'\n",
	    result, process->rank, process->kvs_name, process->exe);
    }

    smpd_exit_fn(FCNAME);
    return result;
#else
    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("stopping process %d\n", process->wait);
    kill(process->wait, SIGSTOP);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#ifdef HAVE_WINDOWS_H
static BOOL SafeTerminateProcess(HANDLE hProcess, UINT uExitCode)
{
    DWORD dwTID, dwCode, dwErr = 0;
    HANDLE hProcessDup = INVALID_HANDLE_VALUE;
    HANDLE hRT = NULL;
    HINSTANCE hKernel = GetModuleHandle("Kernel32");
    BOOL bSuccess = FALSE;

    BOOL bDup = DuplicateHandle(GetCurrentProcess(),
	hProcess,
	GetCurrentProcess(),
	&hProcessDup,
	PROCESS_ALL_ACCESS,
	FALSE,
	0);

    if (GetExitCodeProcess((bDup) ? hProcessDup : hProcess, &dwCode) &&
	(dwCode == STILL_ACTIVE))
    {
	FARPROC pfnExitProc;

	pfnExitProc = GetProcAddress(hKernel, "ExitProcess");

	if (pfnExitProc)
	{
	    hRT = CreateRemoteThread((bDup) ? hProcessDup : hProcess,
		NULL,
		0,
		// This relies on the probability that Kernel32.dll is mapped to the same place on all processes
		// If it gets relocated, this function will produce spurious results
		(LPTHREAD_START_ROUTINE)pfnExitProc,
		UintToPtr(uExitCode)/*(LPVOID)uExitCode*/, 0, &dwTID);
	}
	
	if (hRT == NULL)
	    dwErr = GetLastError();
    }
    else
    {
	dwErr = ERROR_PROCESS_ABORTED;
    }

    if (hRT)
    {
	if (WaitForSingleObject((bDup) ? hProcessDup : hProcess, 30000) == WAIT_OBJECT_0)
	    bSuccess = TRUE;
	else
	{
	    dwErr = ERROR_TIMEOUT;
	    bSuccess = FALSE;
	}
	CloseHandle(hRT);
    }

    if (bDup)
	CloseHandle(hProcessDup);

    if (!bSuccess)
	SetLastError(dwErr);

    return bSuccess;
}
#endif

#undef FCNAME
#define FCNAME "smpd_kill_process"
int smpd_kill_process(smpd_process_t *process, int exit_code)
{
#ifdef HAVE_WINDOWS_H
    smpd_enter_fn(FCNAME);

    smpd_process_from_registry(process);
    if (!SafeTerminateProcess(process->wait.hProcess, exit_code))
    {
	if (GetLastError() != ERROR_PROCESS_ABORTED)
	{
	    TerminateProcess(process->wait.hProcess, exit_code);
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#else
    int status;
    smpd_enter_fn(FCNAME);

    smpd_dbg_printf("killing process %d\n", process->wait);
    kill(process->wait, /*SIGTERM*/SIGKILL);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
#endif
}

#undef FCNAME
#define FCNAME "smpd_kill_all_processes"
int smpd_kill_all_processes(void)
{
    smpd_process_t *iter;

    smpd_enter_fn(FCNAME);

    if (smpd_process.rsh_mpiexec)
    {
	int i;
	int count = 0;
	iter = smpd_process.process_list;
	while (iter)
	{
	    count++;
	    iter = iter->next;
	}
	if (count > 0)
	{
	    smpd_pwait_t *wait_array;
	    wait_array = (smpd_pwait_t*)malloc(sizeof(smpd_pwait_t) * count);
	    for (i=0, iter = smpd_process.process_list; i<count; i++)
	    {
		wait_array[i] = iter->wait;
		iter = iter->next;
	    }
	    for (i=0; i<count; i++)
	    {
#ifdef HAVE_WINDOWS_H
		if (!SafeTerminateProcess(wait_array[i].hProcess, 123))
		{
		    if (GetLastError() != ERROR_PROCESS_ABORTED)
		    {
			TerminateProcess(wait_array[i].hProcess, 255);
		    }
		}
#else
		kill(wait_array[i], /*SIGTERM*/SIGKILL);
#endif
	    }
	}
    }
    else
    {
	iter = smpd_process.process_list;
	while (iter)
	{
#ifdef HAVE_WINDOWS_H
	    /*DWORD dwProcessId;*/
	    smpd_process_from_registry(iter);
	    /* For some reason break signals don't work on processes created by smpd_launch_process
	    printf("ctrl-c process: %s\n", iter->exe);fflush(stdout);
	    dwProcessId = GetProcessId(iter->wait.hProcess);
	    GenerateConsoleCtrlEvent(CTRL_C_EVENT, dwProcessId);
	    if (WaitForSingleObject(iter->wait.hProcess, 1000) != WAIT_OBJECT_0)
	    {
	    printf("breaking process: %s\n", iter->exe);fflush(stdout);
	    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, dwProcessId);
	    if (WaitForSingleObject(iter->wait.hProcess, 1000) != WAIT_OBJECT_0)
	    {
	    */
	    if (!SafeTerminateProcess(iter->wait.hProcess, 123))
	    {
		if (GetLastError() != ERROR_PROCESS_ABORTED)
		{
		    TerminateProcess(iter->wait.hProcess, 255);
		}
	    }
	    /*
	    }
	    }
	    */
#else
	    kill(iter->wait, /*SIGTERM*/SIGKILL);
#endif
	    iter = iter->next;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_exit"
int smpd_exit(int exitcode)
{
    smpd_enter_fn(FCNAME);
    smpd_kill_all_processes();
    smpd_finalize_drive_maps();
    smpd_finalize_printf();
    PMPI_Finalize();
    /* If we're exiting due to a user abort, use the exit code supplied by the abort call */
    if (smpd_process.use_abort_exit_code)
	exitcode = smpd_process.abort_exit_code;
#ifdef HAVE_WINDOWS_H
    if (smpd_process.hCloseStdinThreadEvent)
    {
	CloseHandle(smpd_process.hCloseStdinThreadEvent);
    }
    /* This is necessary because exit() can deadlock flushing file buffers while the stdin thread is running */
    ExitProcess(exitcode);
#else
    exit(exitcode);
    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
#endif
}
