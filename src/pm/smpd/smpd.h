/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef SMPD_H
#define SMPD_H

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else
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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "smpd_database.h"

#define SMPD_LISTENER_PORT               8676

#define SMPD_SUCCESS                        0
#define SMPD_FAIL                          -1
#define SMPD_ERR_INVALID_USER              -2
#define SMPD_CLOSE                          2
#define SMPD_CONNECTED                      3
#define SMPD_EXIT                           4
#define SMPD_DBS_RETURN                     5
#define SMPD_ABORT                          6

typedef int SMPD_BOOL;
#define SMPD_TRUE                           1
#define SMPD_FALSE                          0

#define SMPD_SERVER_AUTHENTICATION          0
#define SMPD_CLIENT_AUTHENTICATION          1

#define SMPD_MAX_NAME_LENGTH              256
#define SMPD_MAX_VALUE_LENGTH            1024
#define SMPD_MAX_FILENAME                1024
#define SMPD_MAX_STDOUT_LENGTH           1024
#define SMPD_MAX_SESSION_HEADER_LENGTH   1024
#define SMPD_MAX_ERROR_LEN               1024
#define SMPD_CMD_HDR_LENGTH                13
#define SMPD_MAX_CMD_LENGTH	         8192
#define SMPD_MAX_CMD_STR_LENGTH           100
#define SMPD_MAX_HOST_LENGTH	           64
#define SMPD_MAX_EXE_LENGTH              1024
#define SMPD_MAX_ENV_LENGTH              1024
#define SMPD_MAX_DIR_LENGTH              1024
#define SMPD_MAX_PATH_LENGTH             1024
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
#define SMPD_OUTPUT_MUTEXNAME             "SMPD_OUTPUT_MUTEX"
#define SMPD_FREE_COOKIE           0xDDBEEFDD

#define SMPD_DBG_STATE_STDOUT            0x01
#define SMPD_DBG_STATE_ERROUT            0x02
#define SMPD_DBG_STATE_LOGFILE           0x04
#define SMPD_DBG_STATE_PREPEND_RANK      0x08
#define SMPD_DBG_STATE_TRACE             0x10
#define SMPD_DBG_STATE_ALL               (SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_LOGFILE | SMPD_DBG_STATE_PREPEND_RANK | SMPD_DBG_STATE_TRACE)

#define SMPD_QUOTE_CHAR                   '\"'
#define SMPD_DELIM_CHAR                   '='
#define SMPD_DELIM_STR                    "="
#define SMPD_ESCAPE_CHAR                  '\\'

#define DBS_SUCCESS_STR	                  "DBS_SUCCESS"
#define DBS_FAIL_STR	                  "DBS_FAIL"
#define DBS_END_STR	                  "DBS_END"

#define SMPD_CONSOLE_STR_LENGTH         10*SMPD_MAX_CMD_LENGTH

#define SMPD_DEFAULT_TIMEOUT              120
#define SMPD_SHORT_TIMEOUT                 60

#ifdef HAVE_WINDOWS_H
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define smpd_get_last_error GetLastError
#else
#define smpd_get_last_error() errno
#endif

typedef enum smpd_state_t
{
    SMPD_IDLE,
    SMPD_EXITING,
    SMPD_DONE,
    SMPD_CLOSING,
    SMPD_SMPD_LISTENING,
    SMPD_MGR_LISTENING,
    SMPD_MPIEXEC_CONNECTING_TREE,
    SMPD_MPIEXEC_CONNECTING_SMPD,
    SMPD_CONNECTING,
    SMPD_RECONNECTING,
    SMPD_READING_CHALLENGE_STRING,
    SMPD_WRITING_CHALLENGE_STRING,
    SMPD_READING_CHALLENGE_RESPONSE,
    SMPD_WRITING_CHALLENGE_RESPONSE,
    SMPD_READING_CONNECT_RESULT,
    SMPD_WRITING_CONNECT_RESULT,
    SMPD_READING_STDIN,
    SMPD_READING_STDOUT,
    SMPD_READING_STDERR,
    SMPD_READING_CMD_HEADER,
    SMPD_READING_CMD,
    SMPD_WRITING_CMD,
    SMPD_READING_SESSION_REQUEST,
    SMPD_WRITING_SMPD_SESSION_REQUEST,
    SMPD_WRITING_PROCESS_SESSION_REQUEST,
    SMPD_READING_PWD_REQUEST,
    SMPD_WRITING_PWD_REQUEST,
    SMPD_WRITING_NO_PWD_REQUEST,
    SMPD_READING_SMPD_PASSWORD,
    SMPD_WRITING_SMPD_PASSWORD,
    SMPD_READING_CRED_REQUEST,
    SMPD_WRITING_CRED_REQUEST,
    SMPD_WRITING_NO_CRED_REQUEST,
    SMPD_READING_ACCOUNT,
    SMPD_WRITING_ACCOUNT,
    SMPD_READING_PASSWORD,
    SMPD_WRITING_PASSWORD,
    SMPD_READING_RECONNECT_REQUEST,
    SMPD_WRITING_RECONNECT_REQUEST,
    SMPD_WRITING_NO_RECONNECT_REQUEST,
    SMPD_READING_SESSION_HEADER,
    SMPD_WRITING_SESSION_HEADER,
    SMPD_READING_SMPD_RESULT,
    SMPD_WRITING_SESSION_ACCEPT,
    SMPD_WRITING_SESSION_REJECT,
    SMPD_READING_PROCESS_RESULT,
    SMPD_WRITING_PROCESS_SESSION_ACCEPT,
    SMPD_WRITING_PROCESS_SESSION_REJECT,
    SMPD_END_MARKER_NUM_STATES
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
    SMPD_CONTEXT_PMI,
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
    int id;
    int num_valid_contexts;
    smpd_context_t *in, *out, *err, *pmi;
    int context_refcount;
    int pid;
    char exe[SMPD_MAX_EXE_LENGTH];
    char env[SMPD_MAX_ENV_LENGTH];
    char dir[SMPD_MAX_DIR_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    int rank;
    int nproc;
    smpd_pwait_t wait;
    int exitcode;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
    char err_msg[SMPD_MAX_ERROR_LEN];
    struct smpd_process_t *next;
} smpd_process_t;

typedef struct smpd_map_drive_node_t
{
    int ref_count;
    char drive;
    char share[SMPD_MAX_EXE_LENGTH];
    struct smpd_map_drive_node_t *next;
} smpd_map_drive_node_t;

typedef struct smpd_launch_node_t
{
    char exe[SMPD_MAX_EXE_LENGTH];
    char *env, env_data[SMPD_MAX_ENV_LENGTH];
    char dir[SMPD_MAX_DIR_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    smpd_map_drive_node_t *map_list;
    int host_id;
    int iproc;
    int nproc;
    struct smpd_launch_node_t *next;
} smpd_launch_node_t;

typedef struct smpd_env_node_t
{
    char name[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];
    struct smpd_env_node_t *next;
} smpd_env_node_t;

typedef struct smpd_database_element_t
{
    char pszKey[SMPD_MAX_DBS_KEY_LEN];
    char pszValue[SMPD_MAX_DBS_VALUE_LEN];
    struct smpd_database_element_t *pNext;
} smpd_database_element_t;

typedef struct smpd_database_node_t
{
    char pszName[SMPD_MAX_DBS_NAME_LEN];
    smpd_database_element_t *pData, *pIter;
    struct smpd_database_node_t *pNext;
} smpd_database_node_t;

typedef struct smpd_barrier_in_t
{
    smpd_context_t *context;
    int dest;
    int cmd_tag;
    char ctx_key[100];
} smpd_barrier_in_t;

typedef struct smpd_barrier_node_t
{
    char name[SMPD_MAX_DBS_NAME_LEN];
    int count;
    int in;
    smpd_barrier_in_t *in_array;
    struct smpd_barrier_node_t *next;
} smpd_barrier_node_t;

/* If you add elements to the process structure you must add the appropriate
   initializer in smpd_connect.c where the global variable, smpd_process, lives */
typedef struct smpd_global_t
{
    smpd_state_t state;
    int  id, parent_id;
    int  level;
    smpd_context_t *left_context, *right_context, *parent_context, *context_list;
    smpd_context_t *listener_context;
    smpd_process_t *process_list;
    int  closing;
    int  root_smpd;
    sock_set_t set;
    char host[SMPD_MAX_HOST_LENGTH];
    char pszExe[SMPD_MAX_EXE_LENGTH];
    int  bService;
    int  bNoTTY;
    int  bPasswordProtect;
    char SMPDPassword[100];
    char UserAccount[100];
    char UserPassword[100];
    int  cur_tag;
    int  dbg_state;
    FILE *dbg_fout;
    int  have_dbs;
    char kvs_name[SMPD_MAX_DBS_NAME_LEN];
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
    int nproc;
    int verbose;
    int shutdown, restart; /* built in commands */
#ifdef HAVE_WINDOWS_H
    BOOL bOutputInitialized;
    HANDLE hOutputMutex;
#endif
#ifdef USE_WIN_MUTEX_PROTECT
    HANDLE hDBSMutex;
#endif
    smpd_database_node_t *pDatabase;
    smpd_database_node_t *pDatabaseIter;
    int nNextAvailableDBSID;
    int nInitDBSRefCount;
    smpd_barrier_node_t *barrier_list;
#ifdef HAVE_WINDOWS_H
    SERVICE_STATUS ssStatus;
    SERVICE_STATUS_HANDLE sshStatusHandle;
#endif
} smpd_global_t;

extern smpd_global_t smpd_process;


/* function prototypes */

int smpd_parse_command_args(int *argcp, char **argvp[]);
#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h);
HANDLE smpd_decode_handle(char *str);
#endif
int smpd_entry_point();
int smpd_enter_at_state(sock_set_t set, smpd_state_t state);
int smpd_wait_process(smpd_pwait_t wait, int *exit_code_ptr);
int smpd_init_process(void);
int smpd_init_printf(void);
int smpd_finalize_printf(void);
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
SMPD_BOOL smpd_option_on(const char *option);
int smpd_get_user_data(const char *key, char *value, int value_len);
int smpd_get_smpd_data(const char *key, char *value, int value_len);
int smpd_get_user_data_default(const char *key, char *value, int value_len);
int smpd_get_smpd_data_default(const char *key, char *value, int value_len);
int smpd_set_user_data(const char *key, const char *value);
int smpd_set_smpd_data(const char *key, const char *value);
int smpd_getpid(void);
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
int smpd_get_string_arg(const char *str, char *flag, char *val, int maxlen);
int smpd_get_int_arg(const char *str, char *flag, int *val_ptr);
int smpd_add_string(char *str, int maxlen, const char *val);
const char * smpd_get_string(const char *str, char *val, int maxlen, int *num_chars);
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
int smpd_handle_barrier_command(smpd_context_t *context);
int smpd_post_abort_command(char *fmt, ...);
int smpd_kill_all_processes(void);
int smpd_exit(int exitcode);
#ifdef HAVE_WINDOWS_H
void smpd_translate_win_error(int error, char *msg, int maxlen, char *prepend, ...);
#else
typedef void smpd_sig_fn_t( int );
smpd_sig_fn_t *smpd_signal( int signo, smpd_sig_fn_t func );
#endif
SMPD_BOOL smpd_get_full_path_name(const char *exe, int maxlen, char *path, char **namepart);
SMPD_BOOL smpd_search_path(const char *path, const char *exe, int maxlen, char *str);

#endif
