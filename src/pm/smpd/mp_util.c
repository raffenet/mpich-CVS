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

int g_bUseProcessSession = 0;

#if 0
int mp_dbg_printf(char *str, ...)
{
    int n;
    va_list list;
    char *format_str;

    if (smpd_process.dbg_state == SMPD_DBG_STATE_ERROUT)
	return 0;

    va_start(list, str);
    if (smpd_process.dbg_state & SMPD_DBG_STATE_STDOUT)
    {
	fprintf(stdout, "[%d]", smpd_process.id);
	format_str = str;
	n = vfprintf(stdout, format_str, list);
    }
    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	format_str = str;
	n = vfprintf(smpd_process.dbg_fout, format_str, list);
    }
    va_end(list);

    fflush(stdout);

    return n;
}

int mp_err_printf(char *str, ...)
{
    int n = 0;
    va_list list;
    char *format_str;

    if (smpd_process.dbg_state == 0)
	return 0;

    va_start(list, str);
    if (smpd_process.dbg_state & SMPD_DBG_STATE_ERROUT)
    {
	fprintf(stdout, "[%d]", smpd_process.id);
	format_str = str;
	n = vfprintf(stdout, format_str, list);
    }
    if (smpd_process.dbg_state & SMPD_DBG_STATE_LOGFILE)
    {
	format_str = str;
	n = vfprintf(smpd_process.dbg_fout, format_str, list);
    }
    va_end(list);

    fflush(stdout);

    return n;
}

#define MP_MAX_INDENT 20
static char indent[MP_MAX_INDENT+1] = "";
static int cur_indent = 0;

int mp_enter_fn(char *fcname)
{
    mp_dbg_printf("%sentering %s\n", indent, fcname);
    if (cur_indent >= 0 && cur_indent < MP_MAX_INDENT)
    {
	indent[cur_indent] = '.';
	indent[cur_indent+1] = '\0';
    }
    cur_indent++;

    return SMPD_SUCCESS;
}

int mp_exit_fn(char *fcname)
{
    if (cur_indent > 0 && cur_indent < MP_MAX_INDENT)
    {
	indent[cur_indent-1] = '\0';
    }
    cur_indent--;
    mp_dbg_printf("%sexiting %s\n", indent, fcname);

    return SMPD_SUCCESS;
}
#endif

int mp_create_command_from_stdin(char *str, smpd_command_t **cmd_pptr)
{
    int result;
    smpd_command_t cmd;
    int tag;

    mp_enter_fn("mp_create_command_from_stdin");

    smpd_init_command(&cmd);
    strcpy(cmd.cmd, str);
    result = smpd_parse_command(&cmd);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("invalid command read from stdin, ignoring: \"%s\"\n", str);
	mp_exit_fn("mp_create_command_from_stdin");
	return SMPD_FAIL;
    }
    if (strcmp(cmd.cmd_str, "connect") == 0)
    {
	if (!smpd_get_int_arg(str, "tag", &tag))
	{
	    mp_dbg_printf("adding tag %d to connect command.\n", smpd_process.cur_tag);
	    smpd_add_command_int_arg(&cmd, "tag", smpd_process.cur_tag);
	    cmd.tag = smpd_process.cur_tag;
	    smpd_process.cur_tag++;
	}
	cmd.wait = SMPD_TRUE;
    }
    result = smpd_create_command_copy(&cmd, cmd_pptr);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to create a copy of the command read from stdin: \"%s\"\n", str);
	mp_exit_fn("mp_create_command_from_stdin");
	return SMPD_FAIL;
    }
    mp_exit_fn("mp_create_command_from_stdin");
    return SMPD_SUCCESS;
}

int handle_result(smpd_context_t *context)
{
    int result, ret_val;
    char str[1024];
    smpd_command_t *iter, *trailer;
    int match_tag;

    mp_enter_fn("handle_result");

    if (!smpd_get_int_arg(context->read_cmd.cmd, "cmd_tag", &match_tag))
    {
	mp_err_printf("result command received without a cmd_tag field: '%s'\n", context->read_cmd.cmd);
	mp_exit_fn("handle_result");
	return SMPD_FAIL;
    }

    trailer = iter = context->wait_list;
    while (iter)
    {
	if (iter->tag == match_tag)
	{
	    ret_val = SMPD_SUCCESS;
	    if (smpd_get_string_arg(context->read_cmd.cmd, "result", str, 1024))
	    {
		if (strcmp(iter->cmd_str, "connect") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
			ret_val = SMPD_CONNECTED;
		    else
		    {
			mp_err_printf("connect failed: %s\n", str);
			ret_val = SMPD_FAIL;
		    }
		}
		else if (strcmp(iter->cmd_str, "launch") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			mp_dbg_printf("successfully launched: '%s'\n", iter->cmd);
		    }
		    else
		    {
			mp_err_printf("launch failed: %s\n", str);
			ret_val = SMPD_FAIL;
		    }
		}
		else
		{
		    mp_err_printf("result returned for unhandled command:\n command: '%s'\n result: '%s'\n", iter->cmd, str);
		}
	    }
	    else
	    {
		mp_err_printf("no result string in the result command.\n");
	    }
	    if (trailer == iter)
	    {
		context->wait_list = context->wait_list->next;
	    }
	    else
	    {
		trailer->next = iter->next;
	    }
	    result = smpd_free_command(iter);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to free command in the wait_list\n");
	    }
	    mp_exit_fn("handle_result");
	    return ret_val;
	}
	else
	{
	    mp_dbg_printf("tag %d != match_tag %d\n", iter->tag, match_tag);
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    if (context->wait_list == NULL)
    {
	mp_err_printf("result command received but the wait_list is empty.\n");
    }
    else
    {
	mp_err_printf("result command did not match any commands in the wait_list.\n");
    }
    mp_exit_fn("handle_result");
    return SMPD_FAIL;
}

int handle_stdout_command(smpd_context_t *context)
{
    int rank;
    char data[MP_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;

    mp_enter_fn("handle_stdout_command");

    cmd = &context->read_cmd;
    if (!smpd_get_int_arg(cmd->cmd, "rank", &rank))
    {
	rank = -1;
	mp_err_printf("no rank in the stdout command: '%s'\n", cmd->cmd);
    }
    if (smpd_get_string_arg(cmd->cmd, "data", data, MP_MAX_STDOUT_LENGTH))
    {
	smpd_decode_buffer(data, data, MP_MAX_STDOUT_LENGTH, &num_decoded);
	printf("[%d]", rank);
	fwrite(data, 1, num_decoded, stdout);
	fflush(stdout);
    }
    else
    {
	mp_err_printf("unable to get the data from the stdout command: '%s'\n", cmd->cmd);
    }

    mp_exit_fn("handle_stdout_command");
    return SMPD_SUCCESS;
}

int handle_stderr_command(smpd_context_t *context)
{
    int rank;
    char data[MP_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;

    mp_enter_fn("handle_stderr_command");

    cmd = &context->read_cmd;
    if (!smpd_get_int_arg(cmd->cmd, "rank", &rank))
    {
	rank = -1;
	mp_err_printf("no rank in the stderr command: '%s'\n", cmd->cmd);
    }
    if (smpd_get_string_arg(cmd->cmd, "data", data, MP_MAX_STDOUT_LENGTH))
    {
	smpd_decode_buffer(data, data, MP_MAX_STDOUT_LENGTH, &num_decoded);
	fprintf(stderr, "[%d]", rank);
	fwrite(data, 1, num_decoded, stderr);
	fflush(stderr);
    }
    else
    {
	mp_err_printf("unable to get the data from the stderr command: '%s'\n", cmd->cmd);
    }

    mp_exit_fn("handle_stderr_command");
    return SMPD_SUCCESS;
}

int handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    smpd_command_t *cmd, *temp_cmd;

    mp_enter_fn("handle_command");

    cmd = &context->read_cmd;

    mp_dbg_printf("handle_command:\n");
    mp_dbg_printf(" src  = %d\n", cmd->src);
    mp_dbg_printf(" dest = %d\n", cmd->dest);
    mp_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    mp_dbg_printf(" tag  = %d\n", cmd->tag);
    mp_dbg_printf(" str  = %s\n", cmd->cmd);
    mp_dbg_printf(" len  = %d\n", cmd->length);
    if (context == smpd_process.left_context)
	mp_dbg_printf(" context = left\n");
    else if (context == smpd_process.right_context)
	mp_dbg_printf(" context = right\n");
    else if (context == smpd_process.parent_context)
	mp_dbg_printf(" context = parent\n");
    else
	mp_dbg_printf(" context = unknown\n");

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("invalid command received, unable to determine the destination.\n");
	mp_exit_fn("handle_command");
	return SMPD_FAIL;
    }
    if (dest)
    {
	mp_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to forward the command.\n");
	    mp_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	mp_exit_fn("handle_command");
	return SMPD_SUCCESS;
    }
    if ((strcmp(cmd->cmd_str, "closed") == 0) || (strcmp(cmd->cmd_str, "down") == 0))
    {
	mp_dbg_printf("handling '%s' command, posting close of the sock.\n", cmd->cmd_str);
	smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(context->sock));
	result = sock_post_close(context->sock);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("unable to post a close of the sock after receiving a '%s' command, sock error:\n%s\n",
		cmd->cmd_str, get_sock_error_string(result));
	    mp_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	mp_exit_fn("handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "closed_request") == 0)
    {
	result = smpd_create_command("closed", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the context.\n");
	    mp_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the context.\n");
	    mp_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed command to context.\n");
	mp_exit_fn("handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "result") == 0)
    {
	result = handle_result(context);
	mp_exit_fn("handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "exit") == 0)
    {
	int exitcode, iproc;
	
	if (!smpd_get_int_arg(cmd->cmd, "code", &exitcode))
	{
	    mp_err_printf("no exit code in exit command: '%s'\n", cmd->cmd);
	}
	if (!smpd_get_int_arg(cmd->cmd, "rank", &iproc))
	{
	    mp_err_printf("no iproc in exit command: '%s'\n", cmd->cmd);
	}
	printf("process %d exited with exit code %d\n", iproc, exitcode);
	fflush(stdout);
	mp_process.nproc--;
	if (mp_process.nproc == 0)
	{
	    mp_dbg_printf("last process exited, returning SMPD_CLOSE.\n");
	    mp_exit_fn("handle_command");
	    return SMPD_EXIT;
	}
	mp_exit_fn("handle_command");
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd->cmd_str, "stdout") == 0)
    {
	result = handle_stdout_command(context);
	mp_exit_fn("handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "stderr") == 0)
    {
	result = handle_stderr_command(context);
	mp_exit_fn("handle_command");
	return result;
    }

    mp_err_printf("ignoring unknown command from the session: '%s'\n", cmd->cmd);

    mp_exit_fn("handle_command");
    return SMPD_SUCCESS;
}

int handle_read(smpd_context_t *context, int num_read, int error, smpd_context_t *session_context)
{
    int result;
    static int read_offset = 0;
    smpd_command_t *cmd_ptr;
    int ret_val = SMPD_SUCCESS;
    char *type_str;

    mp_enter_fn("handle_read");

    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	{
	    switch (context->type)
	    {
	    case SMPD_CONTEXT_INVALID:
		type_str = "invalid";
		break;
	    case SMPD_CONTEXT_STDIN:
		type_str = "stdin";
		break;
	    case SMPD_CONTEXT_STDOUT:
		type_str = "stdout";
		break;
	    case SMPD_CONTEXT_STDERR:
		type_str = "stderr";
		break;
	    case SMPD_CONTEXT_PARENT:
		type_str = "parent";
		break;
	    case SMPD_CONTEXT_LEFT_CHILD:
		type_str = "left child";
		break;
	    case SMPD_CONTEXT_RIGHT_CHILD:
		type_str = "right child";
		break;
	    case SMPD_CONTEXT_CHILD:
		type_str = "child";
		break;
	    default:
		type_str = "unknown";
		break;
	    }
	    mp_err_printf("sock read error on %s context connected to '%s': %s\n", type_str, context->host, get_sock_error_string(error));
	}
	else
	    mp_err_printf("sock read error:\n%s\n", get_sock_error_string(error));
	mp_exit_fn("handle_read");
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: read on a NULL context of %d bytes\n", num_read);
	mp_exit_fn("handle_read");
	return SMPD_FAIL;
    }
    if (num_read < 1)
    {
	mp_err_printf("Error: read %d bytes from '%s'\n", num_read, context->host);
	mp_exit_fn("handle_read");
	return SMPD_FAIL;
    }

    if (context->type == SMPD_CONTEXT_STDIN)
    {
	/* handle data read from stdin */
	if (context->read_cmd.cmd[read_offset] == '\n')
	{
	    context->read_cmd.cmd[read_offset] = '\0'; /* remove the \n character */
	    result = mp_create_command_from_stdin(context->read_cmd.cmd, &cmd_ptr);
	    if (result == SMPD_SUCCESS)
	    {
		mp_dbg_printf("command read from stdin, forwarding to smpd\n");
		result = smpd_post_write_command(session_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    mp_err_printf("unable to post a write of the command read from stdin: \"%s\"\n", cmd_ptr->cmd);
		    smpd_free_command(cmd_ptr);
		    mp_exit_fn("handle_read");
		    return SMPD_FAIL;
		}
		mp_dbg_printf("posted write of command: \"%s\"\n", cmd_ptr->cmd);
	    }
	    else
	    {
		mp_err_printf("unable to create a command from user input, ignoring.\n");
	    }	    
	    read_offset = 0;
	}
	else
	{
	    read_offset++;
	}
	result = sock_post_read(context->sock, &context->read_cmd.cmd[read_offset], 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("unable to post a read on the stdin sock, error:\n%s\n", get_sock_error_string(result));
	    mp_exit_fn("handle_read");
	    return SMPD_FAIL;
	}
    }
    else
    {
	switch (context->read_cmd.state)
	{
	case SMPD_CMD_INVALID:
	    mp_err_printf("data read on a command in the invalid state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_HDR:
	    context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
	    if (context->read_cmd.length < 1)
	    {
		mp_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
		ret_val = SMPD_FAIL;
		break;
	    }
	    mp_dbg_printf("read command header, posting read of data: %d bytes\n", context->read_cmd.length);
	    context->read_cmd.state = SMPD_CMD_READING_CMD;
	    ret_val = sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, NULL);
	    if (ret_val == SOCK_SUCCESS)
		ret_val = SMPD_SUCCESS;
	    else
	    {
		mp_err_printf("unable to post a read for the command string, sock error:\n%s\n", get_sock_error_string(ret_val));
		ret_val = SMPD_FAIL;
	    }
	    break;
	case SMPD_CMD_READING_CMD:
	    ret_val = smpd_parse_command(&context->read_cmd);
	    if (ret_val != SMPD_SUCCESS)
	    {
		mp_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
		break;
	    }
	    mp_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
	    ret_val = handle_command(context);
	    if (ret_val == SMPD_SUCCESS || ret_val == SMPD_CONNECTED || ret_val == SMPD_EXIT)
	    {
		result = smpd_post_read_command(context);
		if (result != SMPD_SUCCESS)
		    ret_val = result;
	    }
	    else
	    {
		if (ret_val != SMPD_CLOSE)
		{
		    mp_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
		}
	    }
	    break;
	case SMPD_CMD_WRITING_CMD:
	    mp_err_printf("data read on a command in the writing_cmd state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READY:
	    mp_err_printf("data read on a command in the ready state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_HANDLED:
	    mp_err_printf("data read on a command in the handled state\n");
	    ret_val = SMPD_FAIL;
	    break;
	default:
	    mp_err_printf("data read on a command in an invalid state: %d\n", context->read_cmd.state);
	    ret_val = SMPD_FAIL;
	    break;
	}
	mp_exit_fn("handle_read");
	return ret_val;
    }
    mp_exit_fn("handle_read");
    return SMPD_SUCCESS;
}

int handle_written(smpd_context_t *context, int num_written, int error)
{
    smpd_command_t *cmd, *iter;
    int ret_val = SMPD_SUCCESS;

    mp_enter_fn("handle_written");

    if (error != SOCK_SUCCESS)
    {
	if (context != NULL)
	    mp_err_printf("sock write error on sock connected to '%s':\n%s\n", context->host, get_sock_error_string(error));
	else
	    mp_err_printf("sock write error:\n%s\n", get_sock_error_string(error));
	mp_exit_fn("handle_written");
	return SMPD_FAIL;
    }
    if (context == NULL)
    {
	mp_err_printf("Error: write on a NULL context of %d bytes\n", num_written);
	mp_exit_fn("handle_written");
	return SMPD_FAIL;
    }
    if (context->host[0] == '\0')
    {
	mp_err_printf("Error: unexpected write finished on stdin context.\n");
    }
    else
    {
	if (context->write_list == NULL)
	{
	    mp_err_printf("data written on a context with no write command posted.\n");
	    mp_exit_fn("handle_written");
	    return SMPD_FAIL;
	}
	switch (context->write_list->state)
	{
	case SMPD_CMD_INVALID:
	    mp_err_printf("data written on a command in the invalid state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_HDR:
	    mp_err_printf("data written on a command in the reading_hdr state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_READING_CMD:
	    mp_err_printf("data written on a command in the reading_cmd state.\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_WRITING_CMD:
	    /* get the currently written command out of the write list */
	    cmd = context->write_list;
	    context->write_list = context->write_list->next;

	    /* check to see if the sock needs to be closed */
	    if (strcmp(cmd->cmd_str, "closed") == 0)
	    {
		/* after writing a closed command, close the socket */
		mp_dbg_printf("closed command written, posting close of the sock.\n");
		mp_dbg_printf("sock_post_close(%d)\n", sock_getid(context->sock));
		ret_val = sock_post_close(context->sock);
		if (ret_val != SOCK_SUCCESS)
		{
		    mp_err_printf("unable to post a close of the sock after writing a 'closed' command, sock error:\n%s\n",
			get_sock_error_string(ret_val));
		    smpd_free_command(cmd);
		    ret_val = SMPD_FAIL;
		    break;
		}
	    }

	    if (cmd->wait)
	    {
		/* If this command expects a reply, move it to the wait list */
		mp_dbg_printf("moving '%s' command to the wait_list.\n", cmd->cmd_str);
		if (context->wait_list)
		{
		    iter = context->wait_list;
		    while (iter->next)
			iter = iter->next;
		    iter->next = cmd;
		}
		else
		{
		    context->wait_list = cmd;
		}
		cmd->next = NULL;
	    }
	    else
	    {
		/* otherwise free the command immediately. */
		mp_dbg_printf("freeing written command: '%s'\n", cmd->cmd);
		ret_val = smpd_free_command(cmd);
		if (ret_val != SMPD_SUCCESS)
		{
		    mp_err_printf("unable to free the written command.\n");
		    break;
		}
	    }

	    /* move to the next command in the write list an post a write for it */
	    cmd = context->write_list;
	    if (cmd)
	    {
		cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
		cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
		cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
		cmd->iov[1].SOCK_IOV_LEN = cmd->length;
		ret_val = sock_post_writev(context->sock, cmd->iov, 2, NULL);
		if (ret_val != SOCK_SUCCESS)
		{
		    mp_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(ret_val));
		    ret_val = SMPD_FAIL;
		}
	    }
	    break;
	case SMPD_CMD_READY:
	    mp_err_printf("data written on a command in the ready state\n");
	    ret_val = SMPD_FAIL;
	    break;
	case SMPD_CMD_HANDLED:
	    mp_err_printf("data written on a command in the handled state\n");
	    ret_val = SMPD_FAIL;
	    break;
	default:
	    mp_err_printf("data written on a command in an invalid state: %d\n", context->write_list->state);
	    ret_val = SMPD_FAIL;
	    break;
	}
    }
    mp_dbg_printf("wrote %d bytes\n", num_written);
    mp_exit_fn("handle_written");
    return SMPD_SUCCESS;
}

#ifdef HAVE_WINDOWS_H
HANDLE g_hCloseStdinThreadEvent = NULL;
HANDLE g_hStdinThread = NULL;
void StdinThread(SOCKET hWrite)
{
    DWORD len;
    char str[SMPD_MAX_CMD_LENGTH];
    HANDLE h[2];
    int result;

    h[0] = GetStdHandle(STD_INPUT_HANDLE);
    if (h[0] == NULL)
    {
	mp_err_printf("Unable to get the stdin handle.\n");
	return;
    }
    h[1] = g_hCloseStdinThreadEvent;
    while (1)
    {
	/*mp_dbg_printf("waiting for input from stdin\n");*/
	result = WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
	    fgets(str, SMPD_MAX_CMD_LENGTH, stdin);
	    len = (DWORD)strlen(str);
	    mp_dbg_printf("forwarding stdin: '%s'\n", str);
	    if (send(hWrite, str, len, 0) == SOCKET_ERROR)
	    {
		mp_err_printf("unable to forward stdin, WriteFile failed, error %d\n", GetLastError());
		return;
	    }
	    /*
	    if (strncmp(str, "close", 5) == 0)
	    {
		shutdown(hWrite, SD_BOTH);
		closesocket(hWrite);
		mp_dbg_printf("closing stdin reader thread.\n");
		return;
	    }
	    */
	}
	else if (result == WAIT_OBJECT_0 + 1)
	{
	    shutdown(hWrite, SD_BOTH);
	    closesocket(hWrite);
	    mp_dbg_printf("g_hCloseStdinThreadEvent signalled, closing stdin reader thread.\n");
	    return;
	}
	else
	{
	    mp_err_printf("stdin wait failed, error %d\n", GetLastError());
	    return;
	}
    }
}

#endif

int shutdown_console(sock_set_t set, sock_t sock, smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd_ptr;
    sock_event_t event;

    /* create a shutdown command */
    result = smpd_create_command("shutdown", 0, context->id, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to create a shutdown command.\n");
	smpd_close_connection(set, sock);
	mp_exit_fn("shutdown_console");
	return result;
    }

    /* post a write for the command */
    result = smpd_post_write_command(context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to post a write of the shutdown command.\n");
	smpd_close_connection(set, sock);
	mp_exit_fn("shutdown_console");
	return result;
    }

    /* wait until it finishes */
    result = SMPD_SUCCESS;
    while (result == SMPD_SUCCESS)
    {
	mp_dbg_printf("sock_waiting for next event.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    mp_exit_fn("shutdown_console");
	    return SMPD_FAIL;
	}

	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    result = handle_read(event.user_ptr, event.num_bytes, event.error, context);
	    if (result == SMPD_CLOSE)
	    {
		mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
		result = SMPD_SUCCESS;
		break;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_read() failed.\n");
	    }
	    break;
	case SOCK_OP_WRITE:
	    result = handle_written(event.user_ptr, event.num_bytes, event.error);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_written() failed.\n");
	    }
	    break;
	case SOCK_OP_ACCEPT:
	    mp_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    mp_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    if (event.user_ptr == smpd_process.left_context)
	    {
		mp_dbg_printf("child context closed.\n");
		free(smpd_process.left_context);
		mp_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    mp_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		    mp_exit_fn("shutdown_console");
		    return SMPD_FAIL;
		}
		mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
		mp_exit_fn("shutdown_console");
		return SMPD_SUCCESS;
	    }
	    else
	    {
		mp_err_printf("unexpected close event returned by sock_wait.\n");
	    }
	    break;
	default:
	    mp_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    mp_dbg_printf("closing smpd console session connection.\n");
    result = smpd_close_connection(set, sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to close the connection to smpd\n");
	mp_exit_fn("shutdown_console");
	return result;
    }
    mp_exit_fn("shutdown_console");
    return SMPD_SUCCESS;
}

int mp_console(char *host)
{
    int result;
    sock_set_t set;
    sock_t sock, insock;
    sock_event_t event;
    SOCK_NATIVE_FD stdin_fd;
    smpd_context_t *context;
    smpd_context_t *session_context;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif

    mp_enter_fn("mp_console");

    /* set the id of the mpiexec node to zero */
    smpd_process.id = 0;

    /* create a session with the host */
    set = SOCK_INVALID_SET;
    if (mp_process.use_process_session)
    {
	result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, host, SMPD_PROCESS_SESSION_STR, 1, &set, &sock);
    }
    else
    {
	result = smpd_connect_to_smpd(SOCK_INVALID_SET, SOCK_INVALID_SOCK, host, SMPD_SMPD_SESSION_STR, 1, &set, &sock);
    }
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to connect to smpd on %s\n", host);
	mp_exit_fn("mp_console");
	return result;
    }

    /* create a context for the session */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_CHILD, set, sock, 1);
    strcpy(context->host, host);
    sock_set_user_ptr(sock, context);
    session_context = context;
    smpd_process.left_context = context;

    /* post a read for a possible incoming command */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
	    context->host, get_sock_error_string(result));
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }

    if (mp_process.shutdown_console)
    {
	result = shutdown_console(set, sock, context);
	mp_exit_fn("mp_console");
	return result;
    }

    /* create a context for reading from stdin */
    context = (smpd_context_t*)malloc(sizeof(smpd_context_t));
    if (context == NULL)
    {
	mp_err_printf("malloc failed to allocate an smpd_context_t, size %d\n", sizeof(smpd_context_t));
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }

    /* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
    result = smpd_make_socket_loop(&stdin_fd, &hWrite);
    if (result)
    {
	mp_err_printf("Unable to make a local socket loop to forward stdin.\n");
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }
#else
    stdin_fd = fileno(stdin);
#endif

    /* convert the native handle to a sock */
    result = sock_native_to_sock(set, stdin_fd, context, &insock);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to create a sock from stdin, sock error:\n%s\n", get_sock_error_string(result));
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }
    smpd_init_context(context, SMPD_CONTEXT_STDIN, set, insock, -1);

#ifdef HAVE_WINDOWS_H
    /* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
       stdin to a sock */
    g_hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_hCloseStdinThreadEvent == NULL)
    {
	mp_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }
    g_hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StdinThread, (void*)hWrite, 0, &dwThreadID);
    if (g_hStdinThread == NULL)
    {
	mp_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }
#endif

    /* post a read for a user command from stdin */
    result = sock_post_read(insock, context->read_cmd.cmd, 1, NULL);
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
	    get_sock_error_string(result));
	mp_exit_fn("mp_console");
	return SMPD_FAIL;
    }

    /* wait for and handle commands */
    result = SMPD_SUCCESS;
    while (result == SMPD_SUCCESS)
    {
	mp_dbg_printf("sock_waiting for next event.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    mp_exit_fn("mp_console");
	    return SMPD_FAIL;
	}

	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    result = handle_read(event.user_ptr, event.num_bytes, event.error, session_context);
	    if (result == SMPD_CLOSE)
	    {
		mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
		result = SMPD_SUCCESS;
		break;
	    }
	    if (result == SMPD_EXIT)
	    {
		mp_dbg_printf("handle_read returned SMPD_EXIT\n");
		result = SMPD_SUCCESS;
		break;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_read() failed.\n");
	    }
	    break;
	case SOCK_OP_WRITE:
	    result = handle_written(event.user_ptr, event.num_bytes, event.error);
	    if (result != SMPD_SUCCESS)
	    {
		mp_err_printf("handle_written() failed.\n");
	    }
	    break;
	case SOCK_OP_ACCEPT:
	    mp_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    mp_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    if (event.user_ptr == smpd_process.left_context)
	    {
		mp_dbg_printf("child context closed.\n");
		free(smpd_process.left_context);
		mp_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    mp_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		    mp_exit_fn("mp_console");
		    return SMPD_FAIL;
		}
		mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
		mp_exit_fn("mp_console");
		return SMPD_SUCCESS;
	    }
	    else
	    {
		mp_err_printf("unexpected close event returned by sock_wait.\n");
	    }
	    break;
	default:
	    mp_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    mp_dbg_printf("closing smpd console session connection.\n");
    result = smpd_close_connection(set, sock);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to close the connection to smpd\n");
	mp_exit_fn("mp_console");
	return result;
    }
    mp_dbg_printf("mp_console returning SMPD_SUCCESS\n");
    mp_exit_fn("mp_console");
    return SMPD_SUCCESS;
}

void mp_get_password(char *password)
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
	mp_err_printf("\nunable to turn off the terminal echo\n");
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
}

void mp_get_account_and_password(char *account, char *password)
{
    size_t len;

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

    mp_get_password(password);
}
