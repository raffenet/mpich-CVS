/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidi_ch3_impl.h"

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

#define MPIDI_BOOTSTRAP_NAME_LEN 100

#ifdef HAVE_WINDOWS_H

typedef struct bootstrap_msg
{
    int length;
    char buffer[1024];
    struct bootstrap_msg *next;
} bootstrap_msg;

#endif /* HAVE_WINDOWS_H */

#ifdef USE_SINGLE_MSG_QUEUE
#define MPICH_MSG_QUEUE_ID 12345
#endif

typedef struct MPIDI_CH3I_BootstrapQ_struct
{
    char name[MPIDI_BOOTSTRAP_NAME_LEN];
#ifdef USE_SINGLE_MSG_QUEUE
    int pid;
    int id;
#elif defined(HAVE_MSGGET)
    int id;
#elif defined (HAVE_WINDOWS_H)
    bootstrap_msg *msg_list;
    HWND hWnd;
    HANDLE hMsgThread;
    HANDLE hReadyEvent;
    HANDLE hMutex;
    HANDLE hMessageArrivedEvent;
    int error;
#else
#error *** No bootstrapping queue capability specified ***
#endif
    struct MPIDI_CH3I_BootstrapQ_struct *next;
} MPIDI_CH3I_BootstrapQ_struct;

static MPIDI_CH3I_BootstrapQ_struct * g_queue_list = NULL;

#ifdef HAVE_WINDOWS_H

int GetNextBootstrapMsg(MPIDI_CH3I_BootstrapQ queue, bootstrap_msg ** msg_ptr, BOOL blocking)
{
    MPIDI_STATE_DECL(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    while (WaitForSingleObject(queue->hMessageArrivedEvent, blocking ? INFINITE : 0) == WAIT_OBJECT_0)
    {
	WaitForSingleObject(queue->hMutex, INFINITE);
	if (queue->msg_list)
	{
	    *msg_ptr = queue->msg_list;
	    queue->msg_list = queue->msg_list->next;
	    if (queue->msg_list == NULL)
		ResetEvent(queue->hMessageArrivedEvent);
	    ReleaseMutex(queue->hMutex);
	    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
	    return MPI_SUCCESS;
	}
	ReleaseMutex(queue->hMutex);
    }
    *msg_ptr = NULL;
    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_BOOTSTRAP_MSG);
    return MPI_SUCCESS;
}

static MPIDI_CH3I_BootstrapQ s_queue = NULL;

LRESULT CALLBACK BootstrapQWndProc(
    HWND hwnd,        /* handle to window */
    UINT uMsg,        /* message identifier */
    WPARAM wParam,    /* first message parameter */
    LPARAM lParam)    /* second message parameter */
{
    PCOPYDATASTRUCT p;
    bootstrap_msg *bsmsg_ptr, *msg_iter;
    /*MPIDI_STATE_DECL(MPID_STATE_BOOTSTRAPQWNDPROC);
    MPIDI_FUNC_ENTER(MPID_STATE_BOOTSTRAPQWNDPROC);*/
    switch (uMsg)
    {
    case WM_DESTROY:
	PostQuitMessage(0);
	break;
    case WM_COPYDATA:
	/*printf("WM_COPYDATA received\n");fflush(stdout);*/
	p = (PCOPYDATASTRUCT) lParam;
	if (WaitForSingleObject(s_queue->hMutex, INFINITE) == WAIT_OBJECT_0)
	{
	    bsmsg_ptr = MPIU_Malloc(sizeof(bootstrap_msg));
	    if (bsmsg_ptr != NULL)
	    {
		memcpy(bsmsg_ptr->buffer, p->lpData, p->cbData);
		bsmsg_ptr->length = p->cbData;
		bsmsg_ptr->next = NULL;
		msg_iter = s_queue->msg_list;
		if (msg_iter == NULL)
		    s_queue->msg_list = bsmsg_ptr;
		else
		{
		    while (msg_iter->next != NULL)
			msg_iter = msg_iter->next;
		    msg_iter->next = bsmsg_ptr;
		}
		/*printf("bootstrap message received: %d bytes\n", bsmsg_ptr->length);fflush(stdout);*/
		SetEvent(s_queue->hMessageArrivedEvent);
	    }
	    else
	    {
		MPIU_Error_printf("MPIU_Malloc failed\n");
	    }
	    ReleaseMutex(s_queue->hMutex);
	}
	else
	{
	    MPIU_Error_printf("Error waiting for s_queue mutex, error %d\n", GetLastError());
	}
	/*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
	return 101;
    default:
	/*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    /*MPIDI_FUNC_EXIT(MPID_STATE_BOOTSTRAPQWNDPROC);*/
    return 0;
}

void MessageQueueThreadFn(MPIDI_CH3I_BootstrapQ_struct *queue)
{
    MSG msg;
    BOOL bRet;
    WNDCLASS wc;
    UUID guid;

    UuidCreate(&guid);
    sprintf(queue->name, "%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
	guid.Data1, guid.Data2, guid.Data3,
	guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) BootstrapQWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "MainMenu";
    wc.lpszClassName = queue->name;
    
    if (!RegisterClass(&wc))
    {
	queue->error = GetLastError();
	SetEvent(queue->hReadyEvent);
	return;
    }

    /* Create the hidden window. */
 
    queue->hWnd = CreateWindow(
	queue->name, /* window class */
	queue->name, /* window name */
	WS_OVERLAPPEDWINDOW,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	(HWND) HWND_MESSAGE, 
	(HMENU) NULL,
	0,
	(LPVOID) NULL); 
 
    if (!queue->hWnd)
    {
	queue->error = GetLastError();
	SetEvent(queue->hReadyEvent);
        return;
    }
 
    ShowWindow(queue->hWnd, SW_HIDE); 
 
    /* initialize the synchronization objects */
    queue->msg_list = NULL;
    queue->hMessageArrivedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    queue->hMutex = CreateMutex(NULL, FALSE, NULL);

    assert(s_queue == NULL); /* we can only handle one message queue */
    s_queue = queue;

    /*printf("signalling queue is ready\n");fflush(stdout);*/
    /* signal that the queue is ready */
    SetEvent(queue->hReadyEvent);

    /* Start handling messages. */
    while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
    { 
        if (bRet == -1)
        {
	    MPIU_Error_printf("MessageQueueThreadFn window received a WM_QUIT message, exiting...\n");
	    return;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
	    /*printf("window message: %d\n", msg.message);fflush(stdout);*/
	}
    } 
}
#endif /* HAVE_WINDOWS_H */

#ifdef USE_SINGLE_MSG_QUEUE

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create(MPIDI_CH3I_BootstrapQ *queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_BootstrapQ_struct *queue;
    int id, key;
    int nb;
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);

    if (g_queue_list)
    {
	*queue_ptr = g_queue_list;
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return MPI_SUCCESS;
    }

    queue = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->next = NULL;
    g_queue_list = queue;

    key = MPICH_MSG_QUEUE_ID;
    id = msgget(key, IPC_CREAT | 0666);
    if (id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgget", "**msgget %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->id = id;
    queue->pid = getpid();
    sprintf(queue->name, "%d", getpid());

    /* drain any stale messages in the queue */
    nb = 0;
    while (nb != -1)
    {
	msg.mtype = queue->pid;
	nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, queue->pid, IPC_NOWAIT);
    }

    *queue_ptr = queue;

    MPIU_DBG_PRINTF(("Created bootstrap queue, %d -> %d:%s\n", key, queue->id, queue->name));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    return MPI_SUCCESS;
}

#else

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_create(MPIDI_CH3I_BootstrapQ *queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_BootstrapQ_struct *queue;
#ifdef HAVE_MSGGET
    int id, key;
#elif defined(HAVE_WINDOWS_H)
    DWORD dwThreadID;
#endif
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);

    /* allocate a queue structure and add it to the global list */
    queue = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (queue == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    queue->next = g_queue_list;
    g_queue_list = queue;

#ifdef HAVE_MSGGET

    srand(time(0));
    key = rand();
    id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    while (id == -1)
    {
	if (errno != EEXIST)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgget", "**msgget %d", errno);
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	    return mpi_errno;
	}
	key = rand();
	id = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
    }
    queue->id = id;
    sprintf(queue->name, "%d", key);
    MPIU_DBG_PRINTF(("created message queue: %s\n", queue->name));

#elif defined(HAVE_WINDOWS_H)

    queue->hReadyEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    queue->hMsgThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MessageQueueThreadFn, queue, 0, &dwThreadID);
    if (queue->hMsgThread == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**CreateThread", "**CreateThread %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    if (WaitForSingleObject(queue->hReadyEvent, 60000) != WAIT_OBJECT_0)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**winwait", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
	return mpi_errno;
    }
    CloseHandle(queue->hReadyEvent);
    queue->hReadyEvent = NULL;

#endif

    *queue_ptr = queue;

    MPIU_DBG_PRINTF(("Created bootstrap queue: %s\n", queue->name));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE);
    return MPI_SUCCESS;
}

#endif

int MPIDI_CH3I_BootstrapQ_tostring(MPIDI_CH3I_BootstrapQ queue, char *name, int length)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSRAPQ_TOSTRING);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSRAPQ_TOSTRING);
    MPIU_Strncpy(name, queue->name, length);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSRAPQ_TOSTRING);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_destroy(MPIDI_CH3I_BootstrapQ queue)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef USE_SINGLE_MSG_QUEUE
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
#elif defined(HAVE_MSGGET)
    int result;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    result = msgctl(queue->id, IPC_RMID, NULL);
    if (result == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgctl", "**msgctl %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
	return mpi_errno;
    }
#elif defined(HAVE_WINDOWS_H)
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    PostMessage(queue->hWnd, WM_DESTROY, 0, 0);
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY);
    return MPI_SUCCESS;
}

#ifdef USE_SINGLE_MSG_QUEUE

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_attach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_attach(char *name, MPIDI_CH3I_BootstrapQ * queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int id, key;
    MPIDI_CH3I_BootstrapQ_struct *iter;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);

    if (g_queue_list == NULL)
    {
	MPIDI_CH3I_BootstrapQ temp_queue;
	mpi_errno = MPIDI_CH3I_BootstrapQ_create(&temp_queue);
    }
    if (g_queue_list == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**boot_attach", "**boot_attach %s", "queue list is empty");
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }

    iter = g_queue_list;
    while (iter->next)
    {
	iter = iter->next;
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
    }
    iter->next = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    iter = iter->next;
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->next = NULL;
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    iter->pid = atoi(name);
    iter->id = g_queue_list->id;

    *queue_ptr = iter;
    MPIU_DBG_PRINTF(("attached to message queue: %s\n", name));

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    return MPI_SUCCESS;
}

#else

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_attach
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_attach(char *name, MPIDI_CH3I_BootstrapQ * queue_ptr)
{
    int mpi_errno = MPI_SUCCESS;
#ifdef HAVE_MSGGET

    int id, key;
    MPIDI_CH3I_BootstrapQ_struct *iter = g_queue_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);

    while (iter)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    iter = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    key = atoi(name);
    id = msgget(key, IPC_CREAT | 0666);
    if (id == -1)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgget", "**msgget %d", errno());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->id = id;
    *queue_ptr = iter;
    /*printf("attached to message queue: %s\n", name);fflush(stdout);*/

#elif defined(HAVE_WINDOWS_H)

    MPIDI_CH3I_BootstrapQ_struct *iter = g_queue_list;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    while (iter)
    {
	if (strcmp(iter->name, name) == 0)
	{
	    *queue_ptr = iter;
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	    return MPI_SUCCESS;
	}
	iter = iter->next;
    }
    iter = (MPIDI_CH3I_BootstrapQ_struct*)MPIU_Malloc(sizeof(MPIDI_CH3I_BootstrapQ_struct));
    if (iter == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    MPIU_Strncpy(iter->name, name, MPIDI_BOOTSTRAP_NAME_LEN);
    /*printf("looking for window %s\n", name);fflush(stdout);*/
    iter->hWnd = FindWindowEx(HWND_MESSAGE, NULL, name, name);
    /*if (iter->hWnd != NULL) { printf("FindWindowEx found the window\n"); fflush(stdout); }*/
    if (iter->hWnd == NULL)
    {
	iter->hWnd = FindWindow(name, name);
	/*if (iter->hWnd != NULL) { printf("FindWindow found the window\n"); fflush(stdout); }*/
    }
    if (iter->hWnd == NULL)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**FindWindowEx", "**FindWindowEx %d", GetLastError());
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
	return mpi_errno;
    }
    iter->hMutex = CreateMutex(NULL, FALSE, NULL);
    iter->hReadyEvent = NULL;
    iter->msg_list = NULL;
    iter->hMsgThread = NULL;
    iter->hMessageArrivedEvent = NULL;
    *queue_ptr = iter;
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH);
    return MPI_SUCCESS;
}

#endif

int MPIDI_CH3I_BootstrapQ_detach(MPIDI_CH3I_BootstrapQ queue)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_send_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_send_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length)
{
    int mpi_errno = MPI_SUCCESS;
#if defined(HAVE_MSGGET) || defined(USE_SINGLE_MSG_QUEUE)
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);

    assert(length <= BOOTSTRAP_MAX_MSG_SIZE);

#ifdef USE_SINGLE_MSG_QUEUE
    msg.mtype = queue->pid;
#else
    msg.mtype = 100;
#endif
    memcpy(msg.data, buffer, length);
    MPIU_DBG_PRINTF(("sending message %d on queue %d\n", msg.mtype, queue->id));
    if (msgsnd(queue->id, &msg, length, 0) == -1)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgsnd", "**msgsnd %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
	return mpi_errno;
    }
    MPIU_DBG_PRINTF(("message sent: %d bytes\n", length));

#elif defined(HAVE_WINDOWS_H)
    COPYDATASTRUCT data;
    LRESULT rc;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    data.dwData = 0; /* immediate data field */
    data.cbData = length;
    data.lpData = buffer;
    rc = SendMessage(queue->hWnd, WM_COPYDATA, 0, (LPARAM)&data);
    /*printf("SendMessage returned %d\n", rc);fflush(stdout);*/
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_BootstrapQ_recv_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_BootstrapQ_recv_msg(MPIDI_CH3I_BootstrapQ queue, void *buffer, int length, int *num_bytes_ptr, BOOL blocking)
{
    int mpi_errno = MPI_SUCCESS;
#if defined(HAVE_MSGGET) || defined(USE_SINGLE_MSG_QUEUE)
    int nb;
    struct msgbuf {
	long mtype;
	char data[BOOTSTRAP_MAX_MSG_SIZE];
    } msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);

    assert(length <= BOOTSTRAP_MAX_MSG_SIZE);

#ifdef USE_SINGLE_MSG_QUEUE
    msg.mtype = queue->pid;
    nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, queue->pid, blocking ? 0 : IPC_NOWAIT);
#else
    msg.mtype = 100;
    nb = msgrcv(queue->id, &msg, BOOTSTRAP_MAX_MSG_SIZE, 0, blocking ? 0 : IPC_NOWAIT);
#endif
    if (nb == -1)
    {
	*num_bytes_ptr = 0;
	if (errno == EAGAIN || errno == ENOMSG)
	{
	    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	    return MPI_SUCCESS;
	}
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**msgrcv", "**msgrcv %d", errno);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }
    memcpy(buffer, msg.data, nb);
    *num_bytes_ptr = nb;
    MPIU_DBG_PRINTF(("message %d received: %d bytes\n", msg.mtype, nb));

#elif defined(HAVE_WINDOWS_H)

    bootstrap_msg * msg;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);

    mpi_errno = GetNextBootstrapMsg(queue, &msg, blocking);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nextbootmsg", 0);
	MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
	return mpi_errno;
    }
    if (msg)
    {
	memcpy(buffer, msg->buffer, min(length, msg->length));
	*num_bytes_ptr = min(length, msg->length);
	MPIU_Free(msg);
    }
    else
    {
	*num_bytes_ptr = 0;
    }
#endif
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG);
    return MPI_SUCCESS;
}
