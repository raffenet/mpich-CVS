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

int smpd_command_destination(int dest, smpd_context_t **dest_context)
{
    int src, level_bit, sub_tree_mask;

    smpd_dbg_printf("entering smpd_command_destination.\n");

    /* get the source */
    src = smpd_process.id;

    smpd_dbg_printf("determining destination context for %d -> %d\n", src, dest);
    /* determine the route and return the context */
    if (src == dest)
    {
	*dest_context = NULL;
	smpd_dbg_printf("%d -> %d : returning NULL context\n", src, dest);
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_SUCCESS;
    }

    if (src == 0)
    {
	/* this assumes that the root uses the left context for it's only child. */
	if (smpd_process.left_context == NULL)
	{
	    smpd_dbg_printf("exiting smpd_command_destination.\n");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("%d -> %d : returning left_context\n", src, dest);
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_SUCCESS;
    }

    if (dest < src)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_dbg_printf("exiting smpd_command_destination.\n");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.parent_context;
	smpd_dbg_printf("%d -> %d : returning parent_context: %d < %d\n", src, dest, dest, src);
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_SUCCESS;
    }

    level_bit = 0x1 << smpd_process.level;
    sub_tree_mask = (level_bit << 1) - 1;

    if (( src ^ level_bit ) == ( dest & sub_tree_mask ))
    {
	if (smpd_process.left_context == NULL)
	{
	    smpd_dbg_printf("exiting smpd_command_destination.\n");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("returning left_context\n", src, dest);
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_SUCCESS;
    }

    if ( src == ( dest & sub_tree_mask ) )
    {
	if (smpd_process.right_context == NULL)
	{
	    smpd_dbg_printf("exiting smpd_command_destination.\n");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.right_context;
	smpd_dbg_printf("%d -> %d : returning right_context\n", src, dest);
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_SUCCESS;
    }

    if (smpd_process.parent_context == NULL)
    {
	smpd_dbg_printf("exiting smpd_command_destination.\n");
	return SMPD_FAIL;
    }
    *dest_context = smpd_process.parent_context;
    smpd_dbg_printf("%d -> %d : returning parent_context: fall through\n", src, dest);
    smpd_dbg_printf("exiting smpd_command_destination.\n");
    return SMPD_SUCCESS;
}

int smpd_init_command(smpd_command_t *cmd)
{
    smpd_dbg_printf("entering smpd_init_command.\n");

    if (cmd == NULL)
    {
	smpd_dbg_printf("exiting smpd_init_command.\n");
	return SMPD_FAIL;
    }

    cmd->cmd_hdr_str[0] = '\0';
    cmd->cmd_str[0] = '\0';
    cmd->cmd[0] = '\0';
    cmd->dest = -1;
    cmd->src = -1;
    cmd->next = NULL;
    cmd->length = 0;
    cmd->state = SMPD_CMD_INVALID;

    smpd_dbg_printf("exiting smpd_init_command.\n");
    return SMPD_SUCCESS;
}

int smpd_parse_command(smpd_command_t *cmd_ptr)
{
    char temp_str[100];

    smpd_dbg_printf("entering smpd_parse_command.\n");

    /* get the source */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "src", temp_str, 100))
    {
	smpd_err_printf("no src flag in the command.\n");
	smpd_dbg_printf("exiting smpd_parse_command.\n");
	return SMPD_FAIL;
    }
    cmd_ptr->src = atoi(temp_str);
    if (cmd_ptr->src < 0)
    {
	smpd_err_printf("invalid command src: %d\n", cmd_ptr->src);
	smpd_dbg_printf("exiting smpd_parse_command.\n");
	return SMPD_FAIL;
    }

    /* get the destination */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "dest", temp_str, 100))
    {
	smpd_err_printf("no dest flag in the command.\n");
	smpd_dbg_printf("exiting smpd_parse_command.\n");
	return SMPD_FAIL;
    }
    cmd_ptr->dest = atoi(temp_str);
    if (cmd_ptr->dest < 0)
    {
	smpd_err_printf("invalid command dest: %d\n", cmd_ptr->dest);
	smpd_dbg_printf("exiting smpd_parse_command.\n");
	return SMPD_FAIL;
    }

    /* get the command string */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "cmd", cmd_ptr->cmd_str, SMPD_MAX_CMD_STR_LENGTH))
    {
	smpd_err_printf("no cmd string in the command.\n");
	smpd_dbg_printf("exiting smpd_parse_command.\n");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("exiting smpd_parse_command.\n");
    return SMPD_SUCCESS;
}

int smpd_create_command(char *cmd, int src, int dest, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;
    char *str;
    int len;
    int result;

    smpd_dbg_printf("entering smpd_create_command.\n");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_dbg_printf("exiting smpd_create_command.\n");
	return SMPD_FAIL;
    }
    smpd_init_command(cmd_ptr);
    cmd_ptr->src = src;
    cmd_ptr->dest = dest;

    if (strlen(cmd) >= SMPD_MAX_CMD_STR_LENGTH)
    {
	smpd_err_printf("command string too long: %s\n", cmd);
	smpd_dbg_printf("exiting smpd_create_command.\n");
	return SMPD_FAIL;
    }
    strcpy(cmd_ptr->cmd_str, cmd);

    str = cmd_ptr->cmd;
    len = SMPD_MAX_CMD_LENGTH;
    result = smpd_add_string_arg(&str, &len, "cmd", cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create the command.\n");
	free(cmd_ptr);
	smpd_dbg_printf("exiting smpd_create_command.\n");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "src", src);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the src to the command.\n");
	free(cmd_ptr);
	smpd_dbg_printf("exiting smpd_create_command.\n");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "dest", dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the dest to the command.\n");
	free(cmd_ptr);
	smpd_dbg_printf("exiting smpd_create_command.\n");
	return SMPD_FAIL;
    }

    *cmd_pptr = cmd_ptr;
    smpd_dbg_printf("exiting smpd_create_command.\n");
    return SMPD_SUCCESS;
}

int smpd_create_command_copy(smpd_command_t *src_ptr, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;

    smpd_dbg_printf("entering smpd_create_command_copy.\n");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_dbg_printf("exiting smpd_create_command_copy.\n");
	return SMPD_FAIL;
    }
    
    *cmd_ptr = *src_ptr;
    *cmd_pptr = cmd_ptr;

    smpd_dbg_printf("exiting smpd_create_command_copy.\n");
    return SMPD_SUCCESS;
}

int smpd_free_command(smpd_command_t *cmd_ptr)
{
    smpd_dbg_printf("entering smpd_free_command.\n");
    if (cmd_ptr)
	free(cmd_ptr);
    smpd_dbg_printf("exiting smpd_free_command.\n");
    return SMPD_SUCCESS;
}

int smpd_add_command_arg(smpd_command_t *cmd_ptr, char *param, char *value)
{
    char *str;
    int len;
    int result;

    len = (int)(SMPD_MAX_CMD_LENGTH - strlen(cmd_ptr->cmd));
    str = &cmd_ptr->cmd[strlen(cmd_ptr->cmd)];

    result = smpd_add_string_arg(&str, &len, param, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the command parameter: %s=%s\n", param, value);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_add_command_int_arg(smpd_command_t *cmd_ptr, char *param, int value)
{
    char *str;
    int len;
    int result;

    len = (int)(SMPD_MAX_CMD_LENGTH - strlen(cmd_ptr->cmd));
    str = &cmd_ptr->cmd[strlen(cmd_ptr->cmd)];

    result = smpd_add_int_arg(&str, &len, param, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the command parameter: %s=%d\n", param, value);
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest)
{
    int result;
    smpd_command_t *cmd;

    smpd_dbg_printf("entering smpd_forward_command.\n");

    result = smpd_create_command_copy(&src->read_cmd, &cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a copy of the command to forward.\n");
	smpd_dbg_printf("exiting smpd_forward_command.\n");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("posting write of forwarded command: \"%s\"\n", cmd->cmd);
    result = smpd_post_write_command(dest, cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of a forwarded command.\n");
	smpd_dbg_printf("exiting smpd_forward_command.\n");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("exiting smpd_forward_command.\n");
    return SMPD_SUCCESS;
}

int smpd_post_read_command(smpd_context_t *context)
{
    int result;

    smpd_dbg_printf("entering smpd_post_read_command.\n");

    /* post a read for the next command header */
    smpd_dbg_printf("posting read for a command header on sock %d: %d bytes\n", sock_getid(context->sock), SMPD_CMD_HDR_LENGTH);
    context->read_cmd.state = SMPD_CMD_READING_HDR;
    result = sock_post_read(context->sock, context->read_cmd.cmd_hdr_str, SMPD_CMD_HDR_LENGTH, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command header, sock error:\n%s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_post_read_command.\n");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("exiting smpd_post_read_command.\n");
    return SMPD_SUCCESS;
}

int smpd_post_write_command(smpd_context_t *context, smpd_command_t *cmd)
{
    int result;
    smpd_command_t *iter;

    smpd_dbg_printf("entering smpd_post_write_command.\n");

    smpd_package_command(cmd);
    cmd->next = NULL;
    cmd->state = SMPD_CMD_WRITING_CMD;

    if (!context->write_list)
    {
	context->write_list = cmd;
    }
    else
    {
	smpd_dbg_printf("enqueueing write at the end of the list.\n");
	iter = context->write_list;
	while (iter->next)
	    iter = iter->next;
	iter->next = cmd;
	smpd_dbg_printf("exiting smpd_post_write_command.\n");
	return SMPD_SUCCESS;
    }

    cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
    cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
    cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
    cmd->iov[1].SOCK_IOV_LEN = cmd->length;
    smpd_dbg_printf("smpd_post_write_command on sock %d: %d bytes for command: \"%s\"\n",
	sock_getid(context->sock), cmd->iov[0].SOCK_IOV_LEN + cmd->iov[1].SOCK_IOV_LEN, cmd->cmd);
    result = sock_post_writev(context->sock, cmd->iov, 2, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_post_write_command.\n");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("exiting smpd_post_write_command.\n");
    return SMPD_SUCCESS;
}

#if 0
int smpd_read_command(smpd_context_t *context)
{
    int result;
    int length;
    char *str;
    int num_read;

    /* get the length of the command from the header */
    length = atoi(context->input_cmd_hdr_str);
    if (length < 1)
    {
	smpd_err_printf("unable to read the command, invalid command length: %d\n", length);
	return SMPD_FAIL;
    }

    /* read the command */
    str = context->input_str;
    while (length)
    {
	result = sock_read(context->sock, str, length, &num_read);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("unable to read the command, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	if (num_read < 0)
	{
	    smpd_err_printf("sock_read returned invalid num_bytes: %d\n", num_read);
	    return SMPD_FAIL;
	}
	length -= num_read;
	str += num_read;
    }

    smpd_dbg_printf("read command on sock %d: '%s'\n", sock_getid(context->sock), context->input_str);

    return SMPD_SUCCESS;
}
#endif

int smpd_package_command(smpd_command_t *cmd)
{
    int length;

    smpd_dbg_printf("entering smpd_package_command.\n");

    /* create the command header - for now it is simply the length of the command string */
    length = (int)strlen(cmd->cmd) + 1;
    if (length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("unable to package invalid command of length %d\n", length);
	smpd_dbg_printf("exiting smpd_package_command.\n");
	return SMPD_FAIL;
    }
    snprintf(cmd->cmd_hdr_str, SMPD_CMD_HDR_LENGTH, "%d", length);
    cmd->length = length;

    smpd_dbg_printf("exiting smpd_package_command.\n");
    return SMPD_SUCCESS;
}

#if 0
int smpd_write_command(smpd_context_t *context)
{
    int length;
    int num_written;
    char *buf;
    int result;

    /* verify the header*/
    length = atoi(context->output_cmd_hdr_str);
    if (length < 1)
    {
	smpd_err_printf("unable to write command, invalid length: %d\n", length);
	return SMPD_FAIL;
    }

    smpd_dbg_printf("writing command from %d to %d on sock %d: \"%s\"\n", smpd_process.id, context->id, sock_getid(context->sock), context->output_str);

    /* write the header */
    length = SMPD_CMD_HDR_LENGTH;
    buf = context->output_cmd_hdr_str;
    while (length)
    {
	result = sock_write(context->sock, buf, length, &num_written);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("unable to write the command header, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	if (num_written < 0)
	{
	    smpd_err_printf("sock_write returned invalid num_bytes: %d\n", num_written);
	    return SMPD_FAIL;
	}
	length -= num_written;
	buf += num_written;
    }

    /* write the data */
    length = atoi(context->output_cmd_hdr_str);
    buf = context->output_str;
    while (length)
    {
	result = sock_write(context->sock, buf, length, &num_written);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("unable to write the command, sock error:\n%s\n", get_sock_error_string(result));
	    return SMPD_FAIL;
	}
	if (num_written < 0)
	{
	    smpd_err_printf("sock_write returned invalid num_bytes: %d\n", num_written);
	    return SMPD_FAIL;
	}
	length -= num_written;
	buf += num_written;
    }

    return SMPD_SUCCESS;
}
#endif
