/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGACTION
#ifdef HAVE_SIGNAL_H
#include <signal.h>
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
      MPIDU_SOCK_INVALID_SET, /* set                    */
      "",               /* host                   */
      "",               /* pszExe                 */
      SMPD_FALSE,       /* bService               */
      SMPD_TRUE,        /* bNoTTY                 */
      SMPD_FALSE,       /* bPasswordProtect       */
      "",               /* SMPDPassword           */
      "",               /* passphrase             */
      SMPD_FALSE,       /* logon                  */
      "",               /* UserAccount            */
      "",               /* UserPassword           */
      0,                /* cur_tag                */
      SMPD_DBG_STATE_ERROUT, /* dbg_state         */
      /*NULL,*/             /* dbg_fout               */
      "",               /* dbg_filename           */
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
      0,                /* nproc                  */
      0,                /* nproc_launched         */
      0,                /* nproc_exited           */
      SMPD_FALSE,       /* verbose                */
      SMPD_FALSE,       /* shutdown               */
      SMPD_FALSE,       /* restart                */
      SMPD_FALSE,       /* validate               */
      SMPD_FALSE,       /* do_status              */
#ifdef HAVE_WINDOWS_H
      FALSE,            /* bOutputInitialized     */
      NULL,             /* hOutputMutex           */
#endif
#ifdef USE_WIN_MUTEX_PROTECT
      NULL,             /* hDBSMutext             */
#endif
      NULL,             /* pDatabase              */
      NULL,             /* pDatabaseIter          */
      /*0,*/                /* nNextAvailableDBSID    */
      0,                /* nInitDBSRefCount       */
      NULL,             /* barrier_list           */
#ifdef HAVE_WINDOWS_H
      {                 /* ssStatus                    */
	  SERVICE_WIN32_OWN_PROCESS, /* dwServiceType  */
	  SERVICE_STOPPED,           /* dwCurrentState */
	  SERVICE_ACCEPT_STOP,   /* dwControlsAccepted */
	  NO_ERROR,     /* dwWin32ExitCode             */
	  0,            /* dwServiceSpecificExitCode   */
	  0,            /* dwCheckPoint                */
	  3000,         /* dwWaitHint                  */
      },
      NULL,             /* sshStatusHandle         */
      NULL,             /* hBombDiffuseEvent       */
      NULL,             /* hBombThread             */
#endif
      SMPD_FALSE,       /* service_stop            */
      SMPD_FALSE,       /* noprompt                */
      "",               /* smpd_filename           */
      SMPD_FALSE,       /* stdin_toall             */
      SMPD_FALSE,       /* stdin_redirecting       */
      NULL,             /* default_host_list       */
      NULL,             /* cur_default_host        */
      0,                /* cur_default_iproc       */
#ifdef HAVE_WINDOWS_H
      NULL,             /* hSMPDDataMutex          */
#endif
      "",               /* printf_buffer           */
      SMPD_SUCCESS,     /* state_machine_ret_val   */
      SMPD_FALSE,       /* exit_on_done            */
      0,                /* tree_parent             */
      1                 /* tree_id                 */
    };

int smpd_post_abort_command(char *fmt, ...)
{
    int result;
    char error_str[2048] = "";
    smpd_command_t *cmd_ptr;
    smpd_context_t *context;
    va_list list;

    smpd_enter_fn("smpd_post_abort_command");

    va_start(list, fmt);
    vsnprintf(error_str, 2048, fmt, list);
    va_end(list);

    result = smpd_create_command("abort", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an abort command.\n");
	smpd_exit_fn("smpd_post_abort_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "error", error_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("Unable to add the error string to the abort command.\n");
	smpd_exit_fn("smpd_post_abort_command");
	return SMPD_FAIL;
    }
    smpd_command_destination(0, &context);
    if (context == NULL)
    {
	if (smpd_process.left_context == NULL)
	{
	    printf("Aborting: %s\n", error_str);
	    fflush(stdout);
	    smpd_exit_fn("smpd_post_abort_command");
	    smpd_exit(-1);
	}

	smpd_process.closing = SMPD_TRUE;
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_enter_at_state");
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_enter_at_state");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_dbg_printf("sending abort command to %s context: \"%s\"\n", smpd_get_context_str(context), cmd_ptr->cmd);
	result = smpd_post_write_command(context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the abort command to the %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_post_abort_command");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_post_abort_command");
    return SMPD_SUCCESS;
}

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
#else
    char *homedir;
    struct stat s;
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
    smpd_process.set = MPIDU_SOCK_INVALID_SET;

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
    memset(&act, 0, sizeof(act));
    act.sa_handler = smpd_child_handler;
#ifdef SA_NODEFER
    act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
#else
    act.sa_flags = SA_NOCLDSTOP;
#endif
    sigaction(SIGCHLD, &act, NULL);
#endif

#ifdef HAVE_WINDOWS_H
    smpd_process.hBombDiffuseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#else
    homedir = getenv("HOME");
    strcpy(smpd_process.smpd_filename, homedir);
    if (smpd_process.smpd_filename[strlen(smpd_process.smpd_filename)-1] != '/')
	strcat(smpd_process.smpd_filename, "/.smpd");
    else
	strcat(smpd_process.smpd_filename, ".smpd");
    if (stat(smpd_process.smpd_filename, &s) == 0)
    {
	if (s.st_mode & 00077)
	{
	    /*printf("smpd file, %s, cannot be readable by anyone other than the current user, please set the permissions accordingly (0600).\n", smpd_process.smpd_filename);*/
	    smpd_process.smpd_filename[0] = '\0';
	}
    }
    else
    {
	smpd_process.smpd_filename[0] = '\0';
    }
#endif

    smpd_get_smpd_data("phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);

    smpd_exit_fn("smpd_init_process");
    return SMPD_SUCCESS;
}

int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, MPIDU_Sock_set_t set, MPIDU_Sock_t sock, int id)
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

    if (sock != MPIDU_SOCK_INVALID_SOCK)
    {
	result = MPIDU_Sock_set_user_ptr(sock, context);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to set the sock user ptr while initializing context,\nsock error: %s\n",
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
    result = MPIU_Str_add_int_arg(&str, &len, "id", session_id);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding session id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = MPIU_Str_add_int_arg(&str, &len, "parent", smpd_process.id);
    if (result != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to create session header, adding parent id failed.\n");
	smpd_exit_fn("smpd_generate_session_header");
	return SMPD_FAIL;
    }
    result = MPIU_Str_add_int_arg(&str, &len, "level", smpd_process.level + 1);
    if (result != MPIU_STR_SUCCESS)
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

int smpd_get_credentials_from_parent(MPIDU_Sock_set_t set, MPIDU_Sock_t sock)
{
    smpd_enter_fn("smpd_get_credentials_from_parent");
    smpd_exit_fn("smpd_get_credentials_from_parent");
    return SMPD_FAIL;
}

int smpd_get_smpd_password_from_parent(MPIDU_Sock_set_t set, MPIDU_Sock_t sock)
{
    smpd_enter_fn("smpd_get_smpd_password_from_parent");
    smpd_exit_fn("smpd_get_smpd_password_from_parent");
    return SMPD_FAIL;
}

int smpd_get_default_hosts()
{
    char hosts[8192];
    char *host;
    smpd_host_node_t *cur_host, *iter;
#ifdef HAVE_WINDOWS_H
    DWORD len;
#else
    int dynamic = SMPD_FALSE;
    char myhostname[SMPD_MAX_HOST_LENGTH];
    int found;
#endif

    smpd_enter_fn("smpd_get_default_hosts");

    if (smpd_process.default_host_list != NULL && smpd_process.cur_default_host != NULL)
    {
	smpd_dbg_printf("default list already populated, returning success.\n");
	smpd_exit_fn("smpd_get_default_hosts");
	return SMPD_SUCCESS;
    }

    if (smpd_get_smpd_data("hosts", hosts, 8192) != SMPD_SUCCESS)
    {
#ifdef HAVE_WINDOWS_H
	len = 8192;
	if (GetComputerName(hosts, &len))
	{
	    smpd_process.default_host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	    if (smpd_process.default_host_list == NULL)
	    {
		smpd_exit_fn("smpd_get_default_hosts");
		return SMPD_FAIL;
	    }
	    strcpy(smpd_process.default_host_list->host, hosts);
	    smpd_process.default_host_list->nproc = 1;
	    smpd_process.default_host_list->next = smpd_process.default_host_list;
	    smpd_process.cur_default_host = smpd_process.default_host_list;
	    smpd_exit_fn("smpd_get_default_hosts");
	    return SMPD_SUCCESS;
	}
	smpd_exit_fn("smpd_get_default_hosts");
	return SMPD_FAIL;
#else
	smpd_lock_smpd_data();
	if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
	{
	    smpd_unlock_smpd_data();
	    if (gethostname(hosts, 8192) == 0)
	    {
		smpd_process.default_host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
		if (smpd_process.default_host_list == NULL)
		{
		    smpd_exit_fn("smpd_get_default_hosts");
		    return SMPD_FAIL;
		}
		strcpy(smpd_process.default_host_list->host, hosts);
		smpd_process.default_host_list->nproc = 1;
		smpd_process.default_host_list->next = smpd_process.default_host_list;
		smpd_process.cur_default_host = smpd_process.default_host_list;
		/* add this host to the dynamic_hosts key */
		strcpy(myhostname, hosts);
		smpd_lock_smpd_data();
		hosts[0] = '\0';
		smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192);
		if (strlen(hosts) > 0)
		{
		    /* FIXME this could overflow */
		    strcat(hosts, " ");
		    strcat(hosts, myhostname);
		}
		else
		{
		    strcpy(hosts, myhostname);
		}
		smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
		smpd_unlock_smpd_data();
		smpd_exit_fn("smpd_get_default_hosts");
		return SMPD_SUCCESS;
	    }
	    smpd_exit_fn("smpd_get_default_hosts");
	    return SMPD_FAIL;
	}
	smpd_unlock_smpd_data();
	if (gethostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
	{
	    dynamic = SMPD_FALSE;
	    myhostname[0] = '\0';
	}
	else
	{
	    dynamic = SMPD_TRUE;
	}
#endif
    }

    /* Insert code here to parse a compressed host string */
    /* For now, just use a space separated list of host names */

    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	cur_host = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	if (cur_host != NULL)
	{
	    strcpy(cur_host->host, host);
	    cur_host->nproc = 1;
	    cur_host->next = NULL;
	    if (smpd_process.default_host_list == NULL)
	    {
		smpd_process.default_host_list = cur_host;
	    }
	    else
	    {
		iter = smpd_process.default_host_list;
		while (iter->next)
		    iter = iter->next;
		iter->next = cur_host;
	    }
	}
	host = strtok(NULL, " \t\r\n");
    }
    if (smpd_process.default_host_list)
    {
#ifndef HAVE_WINDOWS_H
	if (dynamic)
	{
	    found = SMPD_FALSE;
	    iter = smpd_process.default_host_list;
	    while (iter)
	    {
		if (strcmp(iter->host, myhostname) == 0)
		{
		    found = SMPD_TRUE;
		    break;
		}
		iter = iter->next;
	    }
	    if (!found)
	    {
		/* add this host to the dynamic_hosts key */
		smpd_lock_smpd_data();
		hosts[0] = '\0';
		smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192);
		if (strlen(hosts) > 0)
		{
		    /* FIXME this could overflow */
		    strcat(hosts, " ");
		    strcat(hosts, myhostname);
		}
		else
		{
		    strcpy(hosts, myhostname);
		}
		smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
		smpd_unlock_smpd_data();
	    }
	}
#endif
	/* make the default list into a ring */
	iter = smpd_process.default_host_list;
	while (iter->next)
	    iter = iter->next;
	iter->next = smpd_process.default_host_list;
	/* point the cur_default_host to the first node in the ring */
	smpd_process.cur_default_host = smpd_process.default_host_list;
    }

    smpd_exit_fn("smpd_get_default_hosts");
    return SMPD_SUCCESS;
}

int smpd_insert_into_dynamic_hosts(void)
{
#ifndef HAVE_WINDOWS_H
    char myhostname[SMPD_MAX_HOST_LENGTH];
    char hosts[8192];
    char *host;
#endif

    smpd_enter_fn("smpd_insert_into_dynamic_hosts");

#ifndef HAVE_WINDOWS_H
    smpd_lock_smpd_data();
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	if (gethostname(hosts, 8192) == 0)
	{
	    /* add this host to the dynamic_hosts key */
	    smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
	    smpd_unlock_smpd_data();
	    smpd_exit_fn("smpd_insert_into_dynamic_hosts");
	    return SMPD_SUCCESS;
	}
	smpd_unlock_smpd_data();
	smpd_exit_fn("smpd_insert_into_dynamic_hosts");
	return SMPD_FAIL;
    }
    smpd_unlock_smpd_data();

    if (gethostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
    {
	smpd_exit_fn("smpd_insert_into_dynamic_hosts");
	return SMPD_FAIL;
    }

    /* check to see if the host is already in the key */
    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	if (strcmp(host, myhostname) == 0)
	{
	    smpd_exit_fn("smpd_insert_into_dynamic_hosts");
	    return SMPD_SUCCESS;
	}
	host = strtok(NULL, " \t\r\n");
    }
    
    /* add this host to the dynamic_hosts key */
    smpd_lock_smpd_data();

    hosts[0] = '\0';
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	smpd_unlock_smpd_data();
	smpd_exit_fn("smpd_insert_into_dynamic_hosts");
	return SMPD_FAIL;
    }
    if (strlen(hosts) > 0)
    {
	/* FIXME this could overflow */
	strcat(hosts, " ");
	strcat(hosts, myhostname);
    }
    else
    {
	strcpy(hosts, myhostname);
    }
    smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts);
    smpd_unlock_smpd_data();
#endif
    smpd_exit_fn("smpd_insert_into_dynamic_hosts");
    return SMPD_SUCCESS;
}

int smpd_remove_from_dynamic_hosts(void)
{
#ifndef HAVE_WINDOWS_H
    char myhostname[SMPD_MAX_HOST_LENGTH];
    char hosts[8192];
    char hosts_less_me[8192];
    char *host;
#endif

    smpd_enter_fn("smpd_remove_from_dynamic_hosts");
#ifndef HAVE_WINDOWS_H
    if (gethostname(myhostname, SMPD_MAX_HOST_LENGTH) != 0)
    {
	smpd_err_printf("gethostname failed, errno = %d\n", errno);
	smpd_exit_fn("smpd_remove_from_dynamic_hosts");
	return SMPD_FAIL;
    }

    smpd_lock_smpd_data();

    hosts[0] = '\0';
    if (smpd_get_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts, 8192) != SMPD_SUCCESS)
    {
	smpd_unlock_smpd_data();
	smpd_dbg_printf("not removing host because "SMPD_DYNAMIC_HOSTS_KEY" does not exist\n");
	smpd_exit_fn("smpd_remove_from_dynamic_hosts");
	return SMPD_FAIL;
    }

    /* create a new hosts lists without this host name in it */
    hosts_less_me[0] = '\0';
    host = strtok(hosts, " \t\r\n");
    while (host)
    {
	if (strcmp(host, myhostname))
	{
	    if (hosts_less_me[0] != '\0')
		strcat(hosts_less_me, " ");
	    strcat(hosts_less_me, host);
	}
	host = strtok(NULL, " \t\r\n");
    }

    if (hosts_less_me[0] == '\0')
    {
	smpd_dbg_printf("removing "SMPD_DYNAMIC_HOSTS_KEY"\n");
	smpd_delete_smpd_data(SMPD_DYNAMIC_HOSTS_KEY);
    }
    else
    {
	smpd_dbg_printf("setting new "SMPD_DYNAMIC_HOSTS_KEY": %s\n", hosts_less_me);
	smpd_set_smpd_data(SMPD_DYNAMIC_HOSTS_KEY, hosts_less_me);
    }
    smpd_unlock_smpd_data();
#endif
    smpd_exit_fn("smpd_remove_from_dynamic_hosts");
    return SMPD_SUCCESS;
}
