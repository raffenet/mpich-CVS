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

    smpd_enter_fn("smpd_command_destination");

    /* get the source */
    src = smpd_process.id;

    /*smpd_dbg_printf("determining destination context for %d -> %d\n", src, dest);*/

    /* determine the route and return the context */
    if (src == dest)
    {
	*dest_context = NULL;
	smpd_dbg_printf("%d -> %d : returning NULL context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (src == 0)
    {
	/* this assumes that the root uses the left context for it's only child. */
	if (smpd_process.left_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("%d -> %d : returning left_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (dest < src)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.parent_context;
	smpd_dbg_printf("%d -> %d : returning parent_context: %d < %d\n", src, dest, dest, src);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    level_bit = 0x1 << smpd_process.level;
    sub_tree_mask = (level_bit << 1) - 1;

    if (( src ^ level_bit ) == ( dest & sub_tree_mask ))
    {
	if (smpd_process.left_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("returning left_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if ( src == ( dest & sub_tree_mask ) )
    {
	if (smpd_process.right_context == NULL)
	{
	    smpd_exit_fn("smpd_command_destination");
	    return SMPD_FAIL;
	}
	*dest_context = smpd_process.right_context;
	smpd_dbg_printf("%d -> %d : returning right_context\n", src, dest);
	smpd_exit_fn("smpd_command_destination");
	return SMPD_SUCCESS;
    }

    if (smpd_process.parent_context == NULL)
    {
	smpd_exit_fn("smpd_command_destination");
	return SMPD_FAIL;
    }
    *dest_context = smpd_process.parent_context;
    smpd_dbg_printf("%d -> %d : returning parent_context: fall through\n", src, dest);
    smpd_exit_fn("smpd_command_destination");
    return SMPD_SUCCESS;
}

int smpd_init_command(smpd_command_t *cmd)
{
    smpd_enter_fn("smpd_init_command");

    if (cmd == NULL)
    {
	smpd_exit_fn("smpd_init_command");
	return SMPD_FAIL;
    }

    cmd->cmd_hdr_str[0] = '\0';
    cmd->cmd_str[0] = '\0';
    cmd->cmd[0] = '\0';
    cmd->dest = -1;
    cmd->src = -1;
    cmd->tag = -1;
    cmd->next = NULL;
    cmd->length = 0;
    cmd->wait = SMPD_FALSE;
    cmd->state = SMPD_CMD_INVALID;

    smpd_exit_fn("smpd_init_command");
    return SMPD_SUCCESS;
}

#if 0
int smpd_parse_command(smpd_command_t *cmd_ptr)
{
    char temp_str[100];

    smpd_enter_fn("smpd_parse_command");

    /* get the source */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "src", temp_str, 100))
    {
	smpd_err_printf("no src flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    cmd_ptr->src = atoi(temp_str);
    if (cmd_ptr->src < 0)
    {
	smpd_err_printf("invalid command src: %d\n", cmd_ptr->src);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the destination */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "dest", temp_str, 100))
    {
	smpd_err_printf("no dest flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    cmd_ptr->dest = atoi(temp_str);
    if (cmd_ptr->dest < 0)
    {
	smpd_err_printf("invalid command dest: %d\n", cmd_ptr->dest);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the command string */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "cmd", cmd_ptr->cmd_str, SMPD_MAX_CMD_STR_LENGTH))
    {
	smpd_err_printf("no cmd string in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the tag */
    if (smpd_get_string_arg(cmd_ptr->cmd, "tag", temp_str, 100))
    {
	cmd_ptr->tag = atoi(temp_str);
    }

    smpd_exit_fn("smpd_parse_command");
    return SMPD_SUCCESS;
}
#endif

int smpd_parse_command(smpd_command_t *cmd_ptr)
{
    smpd_enter_fn("smpd_parse_command");

    /* get the source */
    if (!smpd_get_int_arg(cmd_ptr->cmd, "src", &cmd_ptr->src))
    {
	smpd_err_printf("no src flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    if (cmd_ptr->src < 0)
    {
	smpd_err_printf("invalid command src: %d\n", cmd_ptr->src);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the destination */
    if (!smpd_get_int_arg(cmd_ptr->cmd, "dest", &cmd_ptr->dest))
    {
	smpd_err_printf("no dest flag in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }
    if (cmd_ptr->dest < 0)
    {
	smpd_err_printf("invalid command dest: %d\n", cmd_ptr->dest);
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the command string */
    if (!smpd_get_string_arg(cmd_ptr->cmd, "cmd", cmd_ptr->cmd_str, SMPD_MAX_CMD_STR_LENGTH))
    {
	smpd_err_printf("no cmd string in the command.\n");
	smpd_exit_fn("smpd_parse_command");
	return SMPD_FAIL;
    }

    /* get the tag */
    smpd_get_int_arg(cmd_ptr->cmd, "tag", &cmd_ptr->tag);

    smpd_exit_fn("smpd_parse_command");
    return SMPD_SUCCESS;
}

int smpd_create_command(char *cmd, int src, int dest, int want_reply, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;
    char *str;
    int len;
    int result;

    smpd_enter_fn("smpd_create_command");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    smpd_init_command(cmd_ptr);
    cmd_ptr->src = src;
    cmd_ptr->dest = dest;
    cmd_ptr->tag = smpd_process.cur_tag++;

    if (strlen(cmd) >= SMPD_MAX_CMD_STR_LENGTH)
    {
	smpd_err_printf("command string too long: %s\n", cmd);
	smpd_exit_fn("smpd_create_command");
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
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "src", src);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the src to the command.\n");
	free(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "dest", dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the dest to the command.\n");
	free(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    result = smpd_add_int_arg(&str, &len, "tag", cmd_ptr->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the command.\n");
	free(cmd_ptr);
	smpd_exit_fn("smpd_create_command");
	return SMPD_FAIL;
    }
    if (want_reply)
	cmd_ptr->wait = SMPD_TRUE;

    *cmd_pptr = cmd_ptr;
    smpd_exit_fn("smpd_create_command");
    return SMPD_SUCCESS;
}

int smpd_create_command_copy(smpd_command_t *src_ptr, smpd_command_t **cmd_pptr)
{
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_create_command_copy");

    cmd_ptr = (smpd_command_t*)malloc(sizeof(smpd_command_t));
    if (cmd_ptr == NULL)
    {
	smpd_err_printf("unable to allocate memory for a command.\n");
	smpd_exit_fn("smpd_create_command_copy");
	return SMPD_FAIL;
    }
    
    *cmd_ptr = *src_ptr;
    *cmd_pptr = cmd_ptr;

    smpd_exit_fn("smpd_create_command_copy");
    return SMPD_SUCCESS;
}

int smpd_free_command(smpd_command_t *cmd_ptr)
{
    smpd_enter_fn("smpd_free_command");
    if (cmd_ptr)
	free(cmd_ptr);
    smpd_exit_fn("smpd_free_command");
    return SMPD_SUCCESS;
}

int smpd_add_command_arg(smpd_command_t *cmd_ptr, char *param, char *value)
{
    char *str;
    int len;
    int result;
    int cmd_length;

    cmd_length = (int)strlen(cmd_ptr->cmd);

    len = (int)(SMPD_MAX_CMD_LENGTH - cmd_length);
    str = &cmd_ptr->cmd[cmd_length];

    /* make sure there is a space after the last parameter in the command */
    if (cmd_length > 0)
    {
	if (cmd_ptr->cmd[cmd_length-1] != ' ')
	{
	    if (len < 2)
	    {
		smpd_err_printf("unable to add the command parameter: %s=%s\n", param, value);
		return SMPD_FAIL;
	    }
	    cmd_ptr->cmd[cmd_length] = ' ';
	    len--;
	    str++;
	}
    }

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
    int cmd_length;

    cmd_length = (int)strlen(cmd_ptr->cmd);

    len = (int)(SMPD_MAX_CMD_LENGTH - cmd_length);
    str = &cmd_ptr->cmd[cmd_length];

    /* make sure there is a space after the last parameter in the command */
    if (cmd_length > 0)
    {
	if (cmd_ptr->cmd[cmd_length-1] != ' ')
	{
	    if (len < 2)
	    {
		smpd_err_printf("unable to add the command parameter: %s=%d\n", param, value);
		return SMPD_FAIL;
	    }
	    cmd_ptr->cmd[cmd_length] = ' ';
	    len--;
	    str++;
	}
    }

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

    smpd_enter_fn("smpd_forward_command");

    result = smpd_create_command_copy(&src->read_cmd, &cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a copy of the command to forward.\n");
	smpd_exit_fn("smpd_forward_command");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("posting write of forwarded command: \"%s\"\n", cmd->cmd);
    result = smpd_post_write_command(dest, cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of a forwarded command.\n");
	smpd_exit_fn("smpd_forward_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_forward_command");
    return SMPD_SUCCESS;
}

int smpd_post_read_command(smpd_context_t *context)
{
    int result;
    char *str;

    smpd_enter_fn("smpd_post_read_command");

    /* post a read for the next command header */
    if (context == smpd_process.parent_context)
	str = "parent";
    else if (context == smpd_process.left_context)
	str = "left";
    else if (context == smpd_process.right_context)
	str = "right";
    else
	str = "unknown";
    smpd_dbg_printf("posting read for a command header on %s context, sock %d: %d bytes\n", str, sock_getid(context->sock), SMPD_CMD_HDR_LENGTH);
    context->read_cmd.state = SMPD_CMD_READING_HDR;
    result = sock_post_read(context->sock, context->read_cmd.cmd_hdr_str, SMPD_CMD_HDR_LENGTH, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command header, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_post_read_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_post_read_command");
    return SMPD_SUCCESS;
}

int smpd_post_write_command(smpd_context_t *context, smpd_command_t *cmd)
{
    int result;
    smpd_command_t *iter;

    smpd_enter_fn("smpd_post_write_command");

    smpd_package_command(cmd);
    smpd_dbg_printf("command after packaging: \"%s\"\n", cmd->cmd);
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
	smpd_exit_fn("smpd_post_write_command");
	return SMPD_SUCCESS;
    }

    cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
    cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
    cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
    cmd->iov[1].SOCK_IOV_LEN = cmd->length;
    smpd_dbg_printf("command at this moment: \"%s\"\n", cmd->cmd);
    smpd_dbg_printf("smpd_post_write_command on sock %d: %d bytes for command: \"%s\"\n",
	sock_getid(context->sock),
	cmd->iov[0].SOCK_IOV_LEN + cmd->iov[1].SOCK_IOV_LEN,
	cmd->cmd);
    result = sock_post_writev(context->sock, cmd->iov, 2, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_post_write_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_post_write_command");
    return SMPD_SUCCESS;
}

int smpd_package_command(smpd_command_t *cmd)
{
    int length;

    smpd_enter_fn("smpd_package_command");

    /* create the command header - for now it is simply the length of the command string */
    length = (int)strlen(cmd->cmd) + 1;
    if (length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("unable to package invalid command of length %d\n", length);
	smpd_exit_fn("smpd_package_command");
	return SMPD_FAIL;
    }
    snprintf(cmd->cmd_hdr_str, SMPD_CMD_HDR_LENGTH, "%d", length);
    cmd->length = length;

    smpd_exit_fn("smpd_package_command");
    return SMPD_SUCCESS;
}
