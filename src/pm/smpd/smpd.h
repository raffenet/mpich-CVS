/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_H
#define SMPD_H

#ifndef HAVE_WINDOWS_H
#include "smpdconf.h"
#endif
#include "sock.h"
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "smpd_database.h"

#define SMPD_LISTENER_PORT               8676

#define SMPD_SUCCESS                        0
#define SMPD_FAIL                          -1
#define SMPD_CLOSE                          2
#define SMPD_CONNECTED                      3
#define SMPD_EXIT                           4

#define SMPD_TRUE                           1
#define SMPD_FALSE                          0

#define SMPD_SERVER_AUTHENTICATION          0
#define SMPD_CLIENT_AUTHENTICATION          1

#define SMPD_MAX_NAME_LENGTH              256
#define SMPD_MAX_VALUE_LENGTH            1024
#define SMPD_MAX_FILENAME                1024
#define SMPD_MAX_STDOUT_LENGTH           1024
#define SMPD_MAX_SESSION_HEADER_LENGTH   1024
#define SMPD_CMD_HDR_LENGTH                13
#define SMPD_MAX_CMD_LENGTH	         8192
#define SMPD_MAX_CMD_STR_LENGTH           100
#define SMPD_MAX_HOST_LENGTH	           64
#define SMPD_MAX_EXE_LENGTH              1024
#define SMPD_MAX_ENV_LENGTH              1024
#define SMPD_MAX_ACCOUNT_LENGTH           100
#define SMPD_MAX_PASSWORD_LENGTH          100
#define SMPD_MAX_CRED_REQUEST_LENGTH      100
#define SMPD_MAX_PWD_REQUEST_LENGTH       100
#define SMPD_MAX_PORT_STR_LENGTH           20
#define SMPD_PASSPHRASE_MAX_LENGTH        256
#define SMPD_SALT_VALUE                   "14"
#define SMPD_SESSION_REQUEST_LEN          100
#define SMPD_AUTHENTICATION_STR_LEN       256
#define SMPD_AUTHENTICATION_REPLY_LENGTH  100
#define SMPD_AUTHENTICATION_REJECTED_STR  "FAIL"
#define SMPD_AUTHENTICATION_ACCEPTED_STR  "SUCCESS"
#define SMPD_SMPD_SESSION_STR             "smpd"
#define SMPD_PROCESS_SESSION_STR          "process"
#define SMPD_DEFAULT_PASSPHRASE           "behappy" /* must be less than 13 characers */
#define SMPD_DEFAULT_PASSWORD             "gastroduodenostomy"
#define SMPD_REGISTRY_KEY                 "SOFTWARE\\MPICH\\SMPD"
#define SMPD_CRED_REQUEST                 "credentials"
#define SMPD_NO_CRED_REQUEST              "nocredentials"
#define SMPD_PWD_REQUEST                  "pwd"
#define SMPD_NO_PWD_REQUEST               "nopwd"
#define SMPD_NO_RECONNECT_PORT_STR        "-1"
#define SMPD_SUCCESS_STR                  "SUCCESS"
#define SMPD_FAIL_STR                     "FAIL"

#define SMPD_FREE_COOKIE           0xDDBEEFDD

#define SMPD_DBG_STATE_STDOUT             0x1
#define SMPD_DBG_STATE_ERROUT             0x2
#define SMPD_DBG_STATE_LOGFILE            0x4
#define SMPD_DBG_STATE_PREPEND_RANK       0x8

#define DBS_SUCCESS_STR	                  "DBS_SUCCESS"
#define DBS_FAIL_STR	                  "DBS_FAIL"
#define DBS_END_STR	                  "DBS_END"

#define SMPD_CONSOLE_STR_LENGTH         10*SMPD_MAX_CMD_LENGTH

#define SMPD_DEFAULT_TIMEOUT              120
#define SMPD_SHORT_TIMEOUT                 60

#ifdef HAVE_WINDOWS_H
#define snprintf _snprintf
#endif

#ifdef HAVE_WINDOWS_H
/* This is necessary because exit() can deadlock flushing file buffers while the stdin thread is running */
/* The correct solution is to signal the thread to exit */
#define smpd_exit ExitProcess
#else
#define smpd_exit exit
#endif

typedef enum smpd_state_t
{
    SMPD_IDLE,
    /* mpiexec process states */
    SMPD_MPIEXEC_CONNECTING_TREE,
    SMPD_MPIEXEC_DBS_STARTING,
    SMPD_MPIEXEC_LAUNCHING,
    SMPD_MPIEXEC_EXIT_WAITING,
    SMPD_MPIEXEC_CONNECTING_SMPD,
    /* smpd states */
    SMPD_CONNECTING,
    SMPD_READING_CHALLENGE_STRING,
    SMPD_WRITING_CHALLENGE_RESPONSE,
    SMPD_READING_CONNECT_RESULT,
    SMPD_WRITING_CHALLENGE_STRING,
    SMPD_READING_CHALLENGE_RESPONSE,
    SMPD_WRITING_CONNECT_RESULT,
    SMPD_READING_STDIN,
    SMPD_READING_STDOUT,
    SMPD_READING_STDERR,
    SMPD_READING_CMD_HEADER,
    SMPD_READING_CMD,
    SMPD_WRITING_CMD,
    SMPD_SMPD_LISTENING,
    SMPD_MGR_LISTENING,
    SMPD_READING_SESSION_REQUEST,
    SMPD_WRITING_SMPD_SESSION_REQUEST,
    SMPD_WRITING_PROCESS_SESSION_REQUEST,
    SMPD_WRITING_PWD_REQUEST,
    SMPD_WRITING_NO_PWD_REQUEST,
    SMPD_READING_PWD_REQUEST,
    SMPD_READING_SMPD_PASSWORD,
    SMPD_WRITING_SMPD_PASSWORD,
    SMPD_WRITING_CRED_REQUEST,
    SMPD_READING_ACCOUNT,
    SMPD_READING_PASSWORD,
    SMPD_WRITING_ACCOUNT,
    SMPD_WRITING_PASSWORD,
    SMPD_WRITING_NO_CRED_REQUEST,
    SMPD_READING_CRED_REQUEST,
    SMPD_WRITING_RECONNECT_REQUEST,
    SMPD_WRITING_NO_RECONNECT_REQUEST,
    SMPD_READING_RECONNECT_REQUEST,
    SMPD_READING_SESSION_HEADER,
    SMPD_WRITING_SESSION_HEADER,
    SMPD_READING_SMPD_RESULT,
    SMPD_WRITING_SESSION_ACCEPT,
    SMPD_WRITING_SESSION_REJECT,
    SMPD_RECONNECTING,
    SMPD_EXITING,
    SMPD_CLOSING
} smpd_state_t;

typedef enum smpd_context_type_t
{
    SMPD_CONTEXT_INVALID,
    SMPD_CONTEXT_STDIN,
    SMPD_CONTEXT_STDOUT,
    SMPD_CONTEXT_STDERR,
    SMPD_CONTEXT_PARENT,
    SMPD_CONTEXT_LEFT_CHILD,
    SMPD_CONTEXT_RIGHT_CHILD,
    SMPD_CONTEXT_CHILD,
    SMPD_CONTEXT_LISTENER,
    SMPD_CONTEXT_SMPD,
    SMPD_CONTEXT_UNDETERMINED,
    SMPD_CONTEXT_FREED
} smpd_context_type_t;

typedef enum smpd_command_state_t
{
    SMPD_CMD_INVALID,
    SMPD_CMD_READING_HDR,
    SMPD_CMD_READING_CMD,
    SMPD_CMD_WRITING_CMD,
    SMPD_CMD_READY,
    SMPD_CMD_HANDLED
} smpd_command_state_t;

typedef struct smpd_command_t
{
    smpd_command_state_t state;
    char cmd_hdr_str[SMPD_CMD_HDR_LENGTH];
    char cmd_str[SMPD_MAX_CMD_STR_LENGTH];
    char cmd[SMPD_MAX_CMD_LENGTH];
    SOCK_IOV iov[2];
    int length;
    int src, dest, tag;
    int wait;
    int stdin_read_offset;
    struct smpd_command_t *next;
    int freed; /* debugging to see if freed more than once */
} smpd_command_t;

#ifdef HAVE_WINDOWS_H
typedef HANDLE smpd_pwait_t;
#else
typedef pid_t smpd_pwait_t;
#endif

typedef struct smpd_host_node_t
{
    int id, parent;
    char host[SMPD_MAX_HOST_LENGTH];
    int nproc;
    struct smpd_host_node_t *next;
} smpd_host_node_t;

typedef struct smpd_context_t
{
    smpd_context_type_t type;
    char host[SMPD_MAX_HOST_LENGTH];
    int id, rank;
    smpd_pwait_t wait;
    sock_set_t set;
    sock_t sock;
    smpd_state_t state;
    smpd_state_t read_state;
    smpd_command_t read_cmd;
    smpd_state_t write_state;
    smpd_command_t *write_list;
    smpd_command_t *wait_list;
    smpd_host_node_t *connect_to;
    char pszCrypt[SMPD_AUTHENTICATION_STR_LEN];
    char pszChallengeResponse[SMPD_AUTHENTICATION_STR_LEN];
    char port_str[SMPD_MAX_PORT_STR_LENGTH];
    char session[SMPD_SESSION_REQUEST_LEN];
    char pwd_request[SMPD_MAX_PWD_REQUEST_LENGTH];
    char cred_request[SMPD_MAX_CRED_REQUEST_LENGTH];
    char account[SMPD_MAX_ACCOUNT_LENGTH];
    char password[SMPD_MAX_PASSWORD_LENGTH];
    char smpd_pwd[SMPD_MAX_PASSWORD_LENGTH];
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];
    int connect_return_id, connect_return_tag;
    struct smpd_process_t *process;
    struct smpd_context_t *next;
} smpd_context_t;

typedef struct smpd_process_t
{
    int num_valid_contexts;
    smpd_context_t *in, *out, *err;
    int context_refcount;
    int pid;
    char exe[SMPD_MAX_EXE_LENGTH];
    char env[SMPD_MAX_ENV_LENGTH];
    char dir[SMPD_MAX_EXE_LENGTH];
    char path[SMPD_MAX_EXE_LENGTH];
    int rank;
    smpd_pwait_t wait;
    int exitcode;
    struct smpd_process_t *next;
} smpd_process_t;

typedef struct smpd_launch_node_t
{
    char exe[SMPD_MAX_EXE_LENGTH];
    char *env, env_data[SMPD_MAX_ENV_LENGTH];
    int host_id;
    long iproc;
    struct smpd_launch_node_t *next;
} smpd_launch_node_t;

typedef struct smpd_env_node
{
    char name[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];
    struct smpd_env_node *next;
} smpd_env_node;

typedef struct smpd_map_drive_node
{
    char drive;
    char share[SMPD_MAX_EXE_LENGTH];
    struct smpd_map_drive_node *next;
} smpd_map_drive_node;

/* If you add elements to the process structure you must add the appropriate
   initializer in smpd_connect.c where the global variable, smpd_process, lives */
typedef struct smpd_global_t
{
    /* stuff used by smpd */
    smpd_state_t state;
    int id, parent_id;
    int level;
    smpd_context_t *left_context, *right_context, *parent_context, *context_list;
    smpd_context_t *listener_context;
    smpd_process_t *process_list;
    int closing;
    int root_smpd;
    sock_set_t set;
    char host[SMPD_MAX_HOST_LENGTH];
    char pszExe[SMPD_MAX_EXE_LENGTH];
    int  bService;
    int  bPasswordProtect;
    char SMPDPassword[100];
    char UserAccount[100];
    char UserPassword[100];
    int cur_tag;
    int dbg_state;
    FILE *dbg_fout;
    int have_dbs;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    /* stuff used by mpiexec */
#ifdef HAVE_WINDOWS_H
    HANDLE hCloseStdinThreadEvent;
    HANDLE hStdinThread;
#endif
    int do_console;
    int port;
    char console_host[SMPD_MAX_HOST_LENGTH];
    smpd_host_node_t *host_list;
    smpd_launch_node_t *launch_list;
    int credentials_prompt;
    int do_multi_color_output;
    int no_mpi;
    int output_exit_codes;
    int local_root;
    int use_iproot;
    int use_process_session;
    int shutdown_console;
    int nproc;
    int verbose;
} smpd_global_t;

extern smpd_global_t smpd_process;


/* smpd */
int smpd_parse_command_args(int *argcp, char **argvp[]);
#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h);
HANDLE smpd_decode_handle(char *str);
#endif

/* smpd_util */
int smpd_enter_at_state(sock_set_t set, smpd_state_t state);
int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr);
int smpd_init_process(void);
int smpd_init_printf(void);
int smpd_init_context(smpd_context_t *context, smpd_context_type_t type, sock_set_t set, sock_t sock, int id);
int smpd_init_command(smpd_command_t *cmd);
int smpd_create_context(smpd_context_type_t type, sock_set_t set, sock_t sock, int id, smpd_context_t **context_pptr);
int smpd_create_command(char *cmd_str, int src, int dest, int want_reply, smpd_command_t **cmd_pptr);
int smpd_create_command_copy(smpd_command_t *src_ptr, smpd_command_t **cmd_pptr);
int smpd_free_command(smpd_command_t *cmd_ptr);
int smpd_free_context(smpd_context_t *context);
int smpd_add_command_arg(smpd_command_t *cmd_ptr, char *param, char *value);
int smpd_add_command_int_arg(smpd_command_t *cmd_ptr, char *param, int value);
int smpd_parse_command(smpd_command_t *cmd_ptr);
int smpd_post_read_command(smpd_context_t *context);
int smpd_post_write_command(smpd_context_t *context, smpd_command_t *cmd);
int smpd_package_command(smpd_command_t *cmd);
int smpd_read_string(sock_t sock, char *str, int maxlen);
int smpd_write_string(sock_t sock, char *str);
int smpd_read(sock_t sock, void *buf, sock_size_t len);
int smpd_write(sock_t sock, void *buf, sock_size_t len);
int smpd_dbg_printf(char *str, ...);
int smpd_err_printf(char *str, ...);
int smpd_enter_fn(char *fcname);
int smpd_exit_fn(char *fcname);
int smpd_get_user_data(char *key, char *value, int value_len);
int smpd_get_smpd_data(char *key, char *value, int value_len);
int smpd_get_user_data_default(char *key, char *value, int value_len);
int smpd_get_smpd_data_default(char *key, char *value, int value_len);
int smpd_set_user_data(char *key, char *value);
int smpd_set_smpd_data(char *key, char *value);
int smpd_getpid();
char * get_sock_error_string(int error);
void smpd_get_password(char *password);
void smpd_get_account_and_password(char *account, char *password);
int smpd_get_credentials_from_parent(sock_set_t set, sock_t sock);
int smpd_get_smpd_password_from_parent(sock_set_t set, sock_t sock);
int smpd_get_opt(int *argc, char ***argv, char * flag);
int smpd_get_opt_int(int *argc, char ***argv, char * flag, int *n);
int smpd_get_opt_long(int *argc, char ***argv, char * flag, long *n);
int smpd_get_opt_double(int *argc, char ***argv, char * flag, double *d);
int smpd_get_opt_string(int *argc, char ***argv, char * flag, char *str, int len);
#ifdef HAVE_WINDOWS_H
void smpd_parse_account_domain(char *domain_account, char *account, char *domain);
int smpd_get_user_handle(char *account, char *domain, char *password, HANDLE *handle_ptr);
int smpd_make_socket_loop(SOCKET *pRead, SOCKET *pWrite);
int smpd_make_socket_loop_choose(SOCKET *pRead, int read_overlapped, SOCKET *pWrite, int write_overlapped);
#endif
int smpd_generate_session_header(char *str, int session_id);
int smpd_interpret_session_header(char *str);
int smpd_add_string_arg(char **str_ptr, int *maxlen_ptr, char *flag, char *val);
int smpd_add_int_arg(char **str_ptr, int *maxlen_ptr, char *flag, int val);
int smpd_get_string_arg(char *str, char *flag, char *val, int maxlen);
int smpd_get_int_arg(char *str, char *flag, int *val_ptr);
int smpd_command_destination(int dest, smpd_context_t **dest_context);
int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest);
int smpd_launch_process(smpd_process_t *process, int priorityClass, int priority, int dbg, sock_set_t set);
int smpd_encode_buffer(char *dest, int dest_length, char *src, int src_length, int *num_encoded);
int smpd_decode_buffer(char *str, char *dest, int length, int *num_decoded);
int smpd_create_process_struct(int rank, smpd_process_t **process_ptr);
int smpd_free_process_struct(smpd_process_t *process);
char * smpd_get_context_str(smpd_context_t *context);
int smpd_gen_authentication_strings(char *phrase, char *append, char *crypted);
#ifdef HAVE_WINDOWS_H
int smpd_start_win_mgr(smpd_context_t *context);
#else
int smpd_start_unx_mgr(smpd_context_t *context);
#endif
#ifdef HAVE_WINDOWS_H
void StdinThread(SOCKET hWrite);
#endif
int smpd_handle_command(smpd_context_t *context);
int smpd_create_command_from_stdin(char *str, smpd_command_t **cmd_pptr);

#endif
