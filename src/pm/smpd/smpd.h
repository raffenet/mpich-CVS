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

#define SMPD_FREE_COOKIE           0x00BEEF00

#define SMPD_DBG_STATE_STDOUT             0x1
#define SMPD_DBG_STATE_ERROUT             0x2
#define SMPD_DBG_STATE_LOGFILE            0x4
#define SMPD_DBG_STATE_PREPEND_RANK       0x8

#define DBS_SUCCESS_STR	                  "DBS_SUCCESS"
#define DBS_FAIL_STR	                  "DBS_FAIL"
#define DBS_END_STR	                  "DBS_END"

#define SMPD_CONSOLE_STR_LENGTH         10*SMPD_MAX_CMD_LENGTH

#define SMPD_DEFAULT_TIMEOUT               45
#define SMPD_SHORT_TIMEOUT                 20

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
    struct smpd_command_t *next;
    int freed; /* debugging to see if freed more than once */
} smpd_command_t;

typedef struct smpd_context_t
{
    smpd_context_type_t type;
    char host[SMPD_MAX_HOST_LENGTH];
    int id, rank;
    sock_set_t set;
    sock_t sock;
    smpd_command_t read_cmd;
    smpd_command_t *write_list;
    smpd_command_t *wait_list;
    struct smpd_context_t *next;
} smpd_context_t;

/* If you add elements to the process structure you must add the appropriate
   initializer in smpd_connect.c where the global variable, smpd_process, lives */
typedef struct smpd_process_t
{
    int id, parent_id;
    int level;
    smpd_context_t *left_context, *right_context, *parent_context, *context_list;
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
} smpd_process_t;

extern smpd_process_t smpd_process;

/* smpd */
int smpd_parse_command_args(int *argcp, char **argvp[]);
int smpd_session(sock_set_t set, sock_t sock);
int smpd_start_mgr(sock_set_t set, sock_t sock);
#ifdef HAVE_WINDOWS_H
char *smpd_encode_handle(char *str, HANDLE h);
HANDLE smpd_decode_handle(char *str);
#endif

/* smpd_util */
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
int smpd_authenticate(sock_set_t set, sock_t sock, int type);
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
int smpd_server_authenticate(sock_set_t set, sock_t sock, char *passphrase);
int smpd_client_authenticate(sock_set_t set, sock_t sock, char *passphrase);
int smpd_getpid();
char * get_sock_error_string(int error);
int smpd_close_connection(sock_set_t set, sock_t sock);
int smpd_connect_to_smpd(sock_set_t parent_set, sock_t parent_sock, char *host, char *session_type, int session_id, sock_set_t *set_ptr, sock_t *sock_ptr);
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
#endif
int smpd_generate_session_header(char *str, int session_id);
int smpd_interpret_session_header(char *str);
int smpd_add_string_arg(char **str_ptr, int *maxlen_ptr, char *flag, char *val);
int smpd_add_int_arg(char **str_ptr, int *maxlen_ptr, char *flag, int val);
int smpd_get_string_arg(char *str, char *flag, char *val, int maxlen);
int smpd_get_int_arg(char *str, char *flag, int *val_ptr);
int smpd_command_destination(int dest, smpd_context_t **dest_context);
int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest);
int smpd_launch_process(char *cmd, char *search_path, char *env, char *dir, int priorityClass, int priority, int dbg, sock_set_t set, sock_t *sock_in, sock_t *sock_out, sock_t *sock_err, int *pid_ptr);
int smpd_encode_buffer(char *dest, int dest_length, char *src, int src_length, int *num_encoded);
int smpd_decode_buffer(char *str, char *dest, int length, int *num_decoded);

#endif
