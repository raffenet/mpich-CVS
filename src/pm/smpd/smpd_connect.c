/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_SIGACTION
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#endif

smpd_global_t smpd_process = 
    { SMPD_IDLE,        /* state                  */
      -1,               /* id                     */
      -1,               /* parent_id              */
      -1,               /* level                  */
      NULL,             /* left_context           */
      NULL,             /* right_context          */
      NULL,             /* parent_context         */
      NULL,             /* context_list           */
      NULL,             /* listener_context       */
      NULL,             /* process_list           */
      SMPD_FALSE,       /* closing                */
      SMPD_FALSE,       /* root_smpd              */
      SOCK_INVALID_SET, /* set                    */
      "",               /* host                   */
      "",               /* pszExe                 */
      SMPD_FALSE,       /* bService               */
      SMPD_FALSE,       /* bPasswordProtect       */
      "",               /* SMPDPassword           */
      "",               /* UserAccount            */
      "",               /* UserPassword           */
      0,                /* cur_tag                */
      SMPD_DBG_STATE_ERROUT, /* dbg_state         */
      NULL,             /* dbg_fout               */
      SMPD_FALSE,       /* have_dbs               */
      "",               /* kvs_name               */
#ifdef HAVE_WINDOWS_H
      NULL,             /* hCloseStdinThreadEvent */
      NULL,             /* hStdinThread           */
#endif
      SMPD_FALSE,       /* do_console             */
      SMPD_LISTENER_PORT, /* smpd port            */
      "",               /* console_host           */
      NULL,             /* host_list              */
      NULL,             /* launch_list            */
      SMPD_TRUE,        /* credentials_prompt     */
      SMPD_TRUE,        /* do_multi_color_output  */
      SMPD_FALSE,       /* no_mpi                 */
      SMPD_FALSE,       /* output_exit_codes      */
      SMPD_FALSE,       /* local_root             */
      SMPD_FALSE,       /* use_iproot             */
      SMPD_FALSE,       /* use_process_session    */
      SMPD_FALSE,       /* shutdown_console       */
      0,                /* nproc                  */
      SMPD_FALSE,       /* verbose                */
      SMPD_FALSE,       /* shutdown               */
      SMPD_FALSE,       /* restart                */
#ifdef HAVE_WINDOWS_H
      FALSE,            /* bOutputInitialized     */
      NULL,             /* hOutputMutex           */
#endif
#ifdef USE_WIN_MUTEX_PROTECT
      NULL,             /* hDBSMutext             */
#endif
      NULL,             /* pDatabase              */
      NULL,             /* pDatabaseIter          */
      0,                /* nNextAvailableDBSID    */
      0,                /* nInitDBSRefCount       */
      NULL              /* barrier_list           */
    };

#ifdef HAVE_SIGACTION
void smpd_child_handler(int code)
{
    int status;
    int pid;

    if (smpd_process.root_smpd && code == SIGCHLD)
    {
	/*pid = waitpid(-1, &status, WNOHANG);*/
	pid = waitpid(-1, &status, 0);
	if (pid < 0)
	{
	    fprintf(stderr, "waitpid failed, error %d\n", errno);
	}
	/*
	else
	{
	    printf("process %d exited with code: %d\n", pid, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
	    fflush(stdout);
	}
	*/
    }
}
#endif

#ifdef HAVE_WINDOWS_H
int smpd_make_socket_loop(SOCKET *pRead, SOCKET *pWrite)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;

    /* Create a listener */

    /* create the socket */
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);
    
    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	smpd_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    gethostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	smpd_err_printf("WSASocket failed, error %d\n", error);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	closesocket(*pWrite);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    closesocket(sock);
    return 0;
}

int smpd_make_socket_loop_choose(SOCKET *pRead, int read_overlapped, SOCKET *pWrite, int write_overlapped)
{
    SOCKET sock;
    char host[100];
    int port;
    int len;
    /*LINGER linger;*/
    BOOL b;
    SOCKADDR_IN sockAddr;
    int error;
    DWORD flag;

    /* Create a listener */

    /* create the socket */
    flag = read_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (sock == INVALID_SOCKET)
    {
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return WSAGetLastError();
    }

    memset(&sockAddr,0,sizeof(sockAddr));
    
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY;
    sockAddr.sin_port = htons((unsigned short)ADDR_ANY);

    if (bind(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	smpd_err_printf("bind failed: error %d\n", error);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }
    
    /* listen */
    listen(sock, 2);

    /* get the host and port where we're listening */
    len = sizeof(sockAddr);
    getsockname(sock, (struct sockaddr*)&sockAddr, &len);
    port = ntohs(sockAddr.sin_port);
    gethostname(host, 100);

    /* Connect to myself */

    /* create the socket */
    flag = write_overlapped ? WSA_FLAG_OVERLAPPED : 0;
    *pWrite = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, flag);
    if (*pWrite == INVALID_SOCKET)
    {
	error = WSAGetLastError();
	smpd_err_printf("WSASocket failed, error %d\n", error);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* set the nodelay option */
    b = TRUE;
    setsockopt(*pWrite, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(BOOL));

    /* Set the linger on close option */
    /*
    linger.l_onoff = 1 ;
    linger.l_linger = 60;
    setsockopt(*pWrite, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger));
    */

    sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* connect to myself */
    if (connect(*pWrite, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
    {
	error = WSAGetLastError();
	closesocket(*pWrite);
	closesocket(sock);
	*pRead = INVALID_SOCKET;
	*pWrite = INVALID_SOCKET;
	return error;
    }

    /* Accept the connection from myself */
    len = sizeof(sockAddr);
    *pRead = accept(sock, (SOCKADDR*)&sockAddr, &len);

    closesocket(sock);
    return 0;
}
#endif

int smpd_init_process(void)
{
#ifdef HAVE_WINDOWS_H
    HMODULE hModule;
#endif
#ifdef HAVE_SIGACTION
    struct sigaction act;
#endif

    smpd_enter_fn("smpd_init_process");

    /* initialize the debugging output print engine */
    smpd_init_printf();

    /* tree data */
    smpd_process.parent_id = -1;
    smpd_process.id = -1;
    smpd_process.level = -1;
    smpd_process.left_context = NULL;
    smpd_process.right_context = NULL;
    smpd_process.parent_context = NULL;
    smpd_process.set = SOCK_INVALID_SET;

    /* local data */
#ifdef HAVE_WINDOWS_H
    hModule = GetModuleHandle(NULL);
    if (!GetModuleFileName(hModule, smpd_process.pszExe, SMPD_MAX_EXE_LENGTH)) 
	smpd_process.pszExe[0] = '\0';
#else
    smpd_process.pszExe[0] = '\0';
#endif
    strcpy(smpd_process.SMPDPassword, SMPD_DEFAULT_PASSWORD);
    smpd_process.bPasswordProtect = SMPD_FALSE;
    smpd_process.bService = SMPD_FALSE;
    gethostname(smpd_process.host, SMPD_MAX_HOST_LENGTH);
    smpd_process.UserAccount[0] = '\0';
    smpd_process.UserPassword[0] = '\0';
    smpd_process.closing = SMPD_FALSE;
    smpd_process.root_smpd = SMPD_FALSE;

    srand(smpd_getpid());

#ifdef HAVE_SIGACTION
    act.sa_handler = smpd_child_handler;
    act.sa_flags = SA_NOCLDSTOP | SA_NOMASK;
    sigaction(SIGCHLD, &act, NULL);
#endif

    smpd_exit_fn("smpd_init_process");
    return SMPD_SUCCESS;
}

int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, sock_set_t set, sock_t sock, int id)
{
    int result;

    smpd_enter_fn("smpd_init_context");
    context->type = type;
    context->host[0] = '\0';
    context->id = id;
    context->rank = 0;
    context->write_list = NULL;
    context->wait_list = NULL;
    smpd_init_command(&context->read_cmd);
    context->next = NULL;
    context->set = set;
    context->sock = sock;
    context->state = SMPD_IDLE;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    context->account[0] = '\0';
    context->connect_return_id = -1;
    context->connect_return_tag = -1;
    context->connect_to = NULL;
    context->cred_request[0] = '\0';
    context->password[0] = '\0';
    context->port_str[0] = '\0';
    context->pszChallengeResponse[0] = '\0';
    context->pszCrypt[0] = '\0';
    context->pwd_request[0] = '\0';
    context->session[0] = '\0';
    context->session_header[0] = '\0';
    context->smpd_pwd[0] = '\0';
    context->wait = SMPD_FALSE;
    context->process = NULL;

    if (sock != SOCK_INVALID_SOCK)
    {
	result = sock_set_user_ptr(sock, context);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("unable to set the sock user ptr while initializing context, sock error:\n%s\n",
		get_sock_error_string(result));
	}
    }

    smpd_exit_fn("smpd_init_context");
    return SMPD_SUCCESS;
}

int smpd_generate_session_header(char *str, int session_id)
{
    char * str_orig;
    int result;
    int len;

    smpd_enter_fn("smpd_generate_session_header");

    str_orig = str;
    *str = '\0';
    len = SMPD_MAX_SESSION_HEADER_LENGTH;

    /* add header fields */
    result = smpd_add_int_arg(&str, &len, "id", session_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "parent", smpd_process.id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding parent id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "level", smpd_process.level + 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session level failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }

    /* remove the trailing space */
    str--;
    *str = '\0';

    smpd_dbg_printf("session header: (%s)\n", str_orig);
    smpd_exit_fn("smpd_generate_session_header");
    return SMPD_SUCCESS;
}

void smpd_get_password(char *password)
{
    char ch = 0;
    int index = 0;
#ifdef HAVE_WINDOWS_H
    HANDLE hStdin;
    DWORD dwMode;
#else
    struct termios terminal_settings, original_settings;
#endif
    size_t len;

    smpd_enter_fn("smpd_get_password");

#ifdef HAVE_WINDOWS_H

    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hStdin, &dwMode))
	dwMode = ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT;
    SetConsoleMode(hStdin, dwMode & ~ENABLE_ECHO_INPUT);
    *password = '\0';
    fgets(password, 100, stdin);
    SetConsoleMode(hStdin, dwMode);

    fprintf(stderr, "\n");

#else

    /* save the current terminal settings */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    original_settings = terminal_settings;

    /* turn off echo */
    terminal_settings.c_lflag &= ~ECHO;
    terminal_settings.c_lflag |= ECHONL;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal_settings);

    /* check that echo is off */
    tcgetattr(STDIN_FILENO, &terminal_settings);
    if (terminal_settings.c_lflag & ECHO)
    {
	smpd_err_printf("\nunable to turn off the terminal echo\n");
	tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

    fgets(password, 100, stdin);

    /* restore the original settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);

#endif

    while ((len = strlen(password)) > 0)
    {
	if (password[len-1] == '\r' || password[len-1] == '\n')
	    password[len-1] = '\0';
	else
	    break;
    }
    smpd_exit_fn("smpd_get_password");
}

void smpd_get_account_and_password(char *account, char *password)
{
    size_t len;

    smpd_enter_fn("smpd_get_account_and_password");

    fprintf(stderr, "credentials needed to launch processes:\n");
    do
    {
	fprintf(stderr, "account (domain\\user): ");
	fflush(stderr);
	*account = '\0';
	fgets(account, 100, stdin);
	while (strlen(account))
	{
	    len = strlen(account);
	    if (account[len-1] == '\r' || account[len-1] == '\n')
		account[len-1] = '\0';
	    else
		break;
	}
    } 
    while (strlen(account) == 0);
    
    fprintf(stderr, "password: ");
    fflush(stderr);

    smpd_get_password(password);
    smpd_exit_fn("smpd_get_account_and_password");
}

int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock)
{
    smpd_enter_fn("smpd_get_credentials_from_parent");
    smpd_exit_fn("smpd_get_credentials_from_parent");
    return SMPD_FAIL;
}

int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock)
{
    smpd_enter_fn("smpd_get_smpd_password_from_parent");
    smpd_exit_fn("smpd_get_smpd_password_from_parent");
    return SMPD_FAIL;
}
