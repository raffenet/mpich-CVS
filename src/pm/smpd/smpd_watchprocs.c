/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

#ifdef HAVE_WINDOWS_H

typedef struct smpd_registry_proc
{
    char pid[10];
    char exe[1024];
    SMPD_BOOL handled;
    struct smpd_registry_proc *next;
} smpd_registry_proc;

static HANDLE hQuit=NULL;
static SMPD_BOOL bPrint = SMPD_TRUE;

#undef FCNAME
#define FCNAME "smpd_watch_processes_thread"
int smpd_watch_processes_thread()
{
    HKEY hKey, hProcKey;
    char name[100];
    char value[1024];
    DWORD len;
    DWORD index;
    HANDLE hRegEvent;
    HANDLE hEvents[2];
    FILETIME t;
    DWORD result;
    smpd_registry_proc *node, *trailer, *temp_list, *list = NULL;

    smpd_enter_fn(FCNAME);

restart:

    hRegEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\MPICH\\SMPD\\process", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
	for (;;)
	{
	    result = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_NAME, hRegEvent, TRUE);
	    if (result != ERROR_SUCCESS)
	    {
		/*printf("result = %d\n", result);*/
		RegCloseKey(hKey);
		break;
	    }
	    index = 0;
	    len = 100;
	    temp_list = NULL;
	    while (1)
	    {
		result = RegEnumKeyEx(hKey, index, name, &len, NULL, NULL, NULL, &t);
		if (result == ERROR_NO_MORE_ITEMS)
		    break;
		if (result != ERROR_SUCCESS)
		    break;
		if (len > 0)
		{
		    if (RegOpenKeyEx(hKey, name, 0, KEY_READ, &hProcKey) == ERROR_SUCCESS)
		    {
			len = 1024;
			if (RegQueryValueEx(hProcKey, "exe", NULL, NULL, value, &len) == ERROR_SUCCESS)
			{
			    node = malloc(sizeof(smpd_registry_proc));
			    strcpy(node->pid, name);
			    if (len > 0)
				strcpy(node->exe, value);
			    else
				node->exe[0] = '\0';
			    node->next = temp_list;
			    temp_list = node;
			}
			RegCloseKey(hProcKey);
		    }
		}
		index++;
		len = 100;
	    }
	    node = list;
	    while (node != NULL)
	    {
		node->handled = SMPD_FALSE;
		node = node->next;
	    }
	    while (temp_list)
	    {
		node = list;
		while (node)
		{
		    if (strcmp(node->pid, temp_list->pid) == 0)
		    {
			node->handled = SMPD_TRUE;
			break;
		    }
		    node = node->next;
		}
		if (node == NULL)
		{
		    node = temp_list;
		    temp_list = temp_list->next;
		    node->next = list;
		    list = node;
		    node->handled = SMPD_TRUE;
		    if (bPrint)
		    {
			printf("+%s %s\n", node->pid, node->exe);
			fflush(stdout);
		    }
		}
		else
		{
		    node = temp_list;
		    temp_list = temp_list->next;
		    free(node);
		}
	    }
	    trailer = node = list;
	    while (node != NULL)
	    {
		if (node->handled == SMPD_FALSE)
		{
		    if (bPrint)
		    {
			printf("-%s %s\n", node->pid, node->exe);
			fflush(stdout);
		    }
		    if (trailer != node)
		    {
			trailer->next = node->next;
			free(node);
			node = trailer->next;
		    }
		    else
		    {
			list = list->next;
			trailer = list;
			free(node);
			node = list;
		    }
		}
		else
		{
		    if (trailer != node)
			trailer = trailer->next;
		    node = node->next;
		}
	    }

	    hEvents[0] = hQuit;
	    hEvents[1] = hRegEvent;
	    result = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
	    if (result < WAIT_OBJECT_0 || result > WAIT_OBJECT_0 + 2)
	    {
		RegCloseKey(hKey);
		break;
	    }
	    if (WaitForSingleObject(hQuit, 0) == WAIT_OBJECT_0)
	    {
		RegCloseKey(hKey);
		break;
	    }
	    /*
	    result = WaitForSingleObject(hRegEvent, INFINITE);
	    if (result != WAIT_OBJECT_0)
	    {
		RegCloseKey(hKey);
		break;
	    }
	    */
	    ResetEvent(hRegEvent);
	}
    }
    else
    {
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\MPICH\\SMPD", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
	    for(;;)
	    {
		result = RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_NAME, hRegEvent, TRUE);
		if (result != ERROR_SUCCESS)
		{
		    /*printf("result = %d\n", result);*/
		    RegCloseKey(hKey);
		    break;
		}
		index = 0;
		len = 100;
		while (1)
		{
		    result = RegEnumKeyEx(hKey, index, name, &len, NULL, NULL, NULL, &t);
		    if (result == ERROR_NO_MORE_ITEMS)
			break;
		    if (result != ERROR_SUCCESS)
			break;
		    if (len > 0)
		    {
			if (strcmp(name, "process") == 0)
			{
			    CloseHandle(hRegEvent);
			    RegCloseKey(hKey);
			    goto restart;
			}
		    }
		    index++;
		    len = 100;
		}

		hEvents[0] = hQuit;
		hEvents[1] = hRegEvent;
		result = WaitForMultipleObjects(3, hEvents, FALSE, INFINITE);
		if (result < WAIT_OBJECT_0 || result > WAIT_OBJECT_0 + 2)
		{
		    RegCloseKey(hKey);
		    break;
		}
		if (WaitForSingleObject(hQuit, 0) == WAIT_OBJECT_0)
		{
		    RegCloseKey(hKey);
		    break;
		}
		/*
		result = WaitForSingleObject(hRegEvent, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
		    RegCloseKey(hKey);
		    break;
		}
		*/
		ResetEvent(hRegEvent);
	    }
	}
    }

    CloseHandle(hRegEvent);

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

int smpd_watch_processes()
{
    HANDLE hThread;
    char line[1024], cmd[1024];
    int result;

    hQuit = CreateEvent(NULL, TRUE, FALSE, NULL);

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)smpd_watch_processes_thread, NULL, 0, NULL);
    if (hThread == NULL)
    {
	printf("Unable to create a thread to watch the processes, exiting...\n");
	fflush(stdout);
	goto fn_exit;
    }

    while (fgets(line, 1024, stdin) != NULL)
    {
	line[1023] = '\0';
	cmd[0] = '\0';
	result = sscanf(line, "%s", cmd);
	if (result == EOF)
	{
	    goto fn_exit;
	}
	if (stricmp(cmd, "start") == 0)
	{
	    bPrint = SMPD_TRUE;
	}
	if (stricmp(cmd, "stop") == 0)
	{
	    bPrint = SMPD_FALSE;
	}
	if ((stricmp(cmd, "quit") == 0) || (stricmp(cmd, "exit") == 0))
	{
	    SetEvent(hQuit);
	    WaitForSingleObject(hThread, INFINITE);
	    break;
	}
    }

fn_exit:
    if (hThread != NULL)
	CloseHandle(hThread);
    if (hQuit != NULL)
	CloseHandle(hQuit);
    return SMPD_SUCCESS;
}

#else /* HAVE_WINDOWS_H */

#undef FCNAME
#define FCNAME "smpd_watch_processes"
int smpd_watch_processes()
{
    smpd_enter_fn(FCNAME);
    printf("Process watching not implemented under unix systems, exiting...\n");
    fflush(stdout);
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#endif /* HAVE_WINDOWS_H */
