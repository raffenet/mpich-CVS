/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

char * smpd_get_context_str(smpd_context_t *context)
{
    switch (context->type)
    {
    case SMPD_CONTEXT_INVALID:
	return "invalid";
    case SMPD_CONTEXT_STDIN:
	return "stdin";
    case SMPD_CONTEXT_STDOUT:
	return "stdout";
    case SMPD_CONTEXT_STDERR:
	return "stderr";
    case SMPD_CONTEXT_PARENT:
	return "parent";
    case SMPD_CONTEXT_LEFT_CHILD:
	return "left";
    case SMPD_CONTEXT_RIGHT_CHILD:
	return "right";
    case SMPD_CONTEXT_CHILD:
	return "child";
    case SMPD_CONTEXT_FREED:
	return "freed";
    default:
	return "unknown";
    }
    return "error";
}

int smpd_create_process(int rank, smpd_process_t **process_ptr)
{
    int result;
    smpd_process_t *p;

    smpd_enter_fn("smpd_create_process");

    p = (smpd_process_t*)malloc(sizeof(smpd_process_t));
    if (p == NULL)
    {
	*process_ptr = NULL;
	smpd_exit_fn("smpd_create_process");
	return SMPD_FAIL;
    }
    p->rank = rank;
    p->exe[0] = '\0';
    p->env[0] = '\0';
    p->dir[0] = '\0';
    p->path[0] = '\0';
    p->next = NULL;
    result = smpd_create_context(SMPD_CONTEXT_STDIN, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->in);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdin context.\n");
	smpd_exit_fn("smpd_create_process");
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDOUT, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->out);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdout context.\n");
	smpd_exit_fn("smpd_create_process");
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDERR, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->err);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stderr context.\n");
	smpd_exit_fn("smpd_create_process");
	return SMPD_FAIL;
    }
    p->in->rank = rank;
    p->out->rank = rank;
    p->err->rank = rank;
    p->num_valid_contexts = 3;
    p->exitcode = 0;
    p->next = NULL;

    *process_ptr = p;

    smpd_exit_fn("smpd_create_process");
    return SMPD_SUCCESS;
}

int smpd_free_process(smpd_process_t *process)
{
    smpd_enter_fn("smpd_free_process");
    if (process == NULL)
    {
	smpd_dbg_printf("smpd_free_process passed NULL process pointer.\n");
	smpd_exit_fn("smpd_free_process");
	return SMPD_SUCCESS;
    }
    if (process->in)
	smpd_free_context(process->in);
    process->in = NULL;
    if (process->out)
	smpd_free_context(process->out);
    process->out = NULL;
    if (process->err)
	smpd_free_context(process->err);
    process->err = NULL;
    process->dir[0] = '\0';
    process->env[0] = '\0';
    process->exe[0] = '\0';
    process->path[0] = '\0';
    process->pid = -1;
    process->rank = -1;
    process->next = NULL;
    free(process);
    smpd_exit_fn("smpd_free_process");
    return SMPD_SUCCESS;
}

int handle_launch_command(smpd_context_t *context)
{
    int result;
    int iproc;
    smpd_command_t *cmd, *temp_cmd;
    smpd_process_t *process;

    smpd_enter_fn("handle_launch_command");

    cmd = &context->read_cmd;

    /* parse the command */
    if (smpd_get_int_arg(cmd->cmd, "i", &iproc) == SMPD_FALSE)
	iproc = 0;
    result = smpd_create_process(iproc, &process);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a process structure.\n");
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }

    if (smpd_get_string_arg(cmd->cmd, "c", process->exe, SMPD_MAX_EXE_LENGTH) == SMPD_FALSE)
    {
	smpd_err_printf("launch command received with no executable: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("i'm everywhere.\n");
    smpd_get_string_arg(cmd->cmd, "e", process->env, SMPD_MAX_ENV_LENGTH);
    smpd_get_string_arg(cmd->cmd, "d", process->dir, SMPD_MAX_EXE_LENGTH);
    smpd_get_string_arg(cmd->cmd, "p", process->path, SMPD_MAX_EXE_LENGTH);

    /* launch the process */
    smpd_dbg_printf("launching: '%s'\n", process->exe);
    result = smpd_launch_process(process, 2, 3, SMPD_FALSE, smpd_process.set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("launch_process failed.\n");

	/* create the result command */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}
	/* launch process should provide a reason for the error, for now just return FAIL */
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}

	/* send the result back */
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}

	/* free the failed process structure */
	smpd_free_process(process);
	process = NULL;

	smpd_exit_fn("handle_launch_command");
	return SMPD_SUCCESS;
    }

    /* save the new process in the list */
    process->next = smpd_process.process_list;
    smpd_process.process_list = process;

    /* create the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }

    /* send the result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command in response to launch command: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("handle_launch_command");
    return SMPD_SUCCESS;
}

int handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    char host[SMPD_MAX_HOST_LENGTH];
    sock_set_t dest_set;
    sock_t dest_sock;
    int dest_id;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_command");

    cmd = &context->read_cmd;

    smpd_dbg_printf("handle_command:\n");
    smpd_dbg_printf(" src  = %d\n", cmd->src);
    smpd_dbg_printf(" dest = %d\n", cmd->dest);
    smpd_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    smpd_dbg_printf(" tag  = %d\n", cmd->tag);
    smpd_dbg_printf(" str  = %s\n", cmd->cmd);
    smpd_dbg_printf(" len  = %d\n", cmd->length);
    if (context == smpd_process.left_context)
	smpd_dbg_printf(" context = left\n");
    else if (context == smpd_process.right_context)
	smpd_dbg_printf(" context = right\n");
    else if (context == smpd_process.parent_context)
	smpd_dbg_printf(" context = parent\n");
    else
	smpd_dbg_printf(" context = unknown\n");

    /* set the command state to handled */
    context->read_cmd.state = SMPD_CMD_HANDLED;

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination.\n");
	smpd_exit_fn("handle_command");
	return SMPD_SUCCESS;
    }
    if (dest)
    {
	smpd_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to forward the command.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	smpd_exit_fn("handle_command");
	return SMPD_SUCCESS;
    }
    if (strcmp(cmd->cmd_str, "close") == 0)
    {
	smpd_process.closing = SMPD_TRUE;
	if (smpd_process.left_context || smpd_process.right_context)
	{
	    if (smpd_process.left_context)
	    {
		result = smpd_create_command("close", smpd_process.id, smpd_process.left_context->id, SMPD_FALSE, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a close command for the left context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("sending close command to left child: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of a close command for the left context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
	    }
	    if (smpd_process.right_context)
	    {
		result = smpd_create_command("close", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a close command for the right context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("sending close command to right child: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of a close command for the right context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
	    }
	    smpd_exit_fn("handle_command");
	    /* If we return success here, a post_read will be made for the next command header. */
	    return SMPD_SUCCESS;
	    /*return SMPD_CLOSE;*/
	}
	result = smpd_create_command("closed", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the parent context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed command to parent: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the parent context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed command.\n");
	smpd_exit_fn("handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "closed") == 0)
    {
	if (smpd_process.parent_context == NULL)
	{
	    smpd_dbg_printf("received a closed command but I have no parent context to forward it to.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (context == smpd_process.left_context)
	{
	    smpd_dbg_printf("closed command received from left child, closing sock.\n");
	    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.left_context->sock));
	    sock_post_close(smpd_process.left_context->sock);
	    if (smpd_process.right_context)
	    {
		smpd_exit_fn("handle_command");
		return SMPD_CLOSE;
	    }
	}
	else if (context == smpd_process.right_context)
	{
	    smpd_dbg_printf("closed command received from right child, closing sock.\n");
	    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.right_context->sock));
	    sock_post_close(smpd_process.right_context->sock);
	    if (smpd_process.left_context)
	    {
		smpd_exit_fn("handle_command");
		return SMPD_CLOSE;
	    }
	}
	else if (context == smpd_process.parent_context)
	{
	    smpd_dbg_printf("closed command received from parent, closing sock.\n");
	    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.parent_context->sock));
	    sock_post_close(smpd_process.parent_context->sock);
	    smpd_exit_fn("handle_command");
	    return SMPD_CLOSE;
	}
	else
	{
	    smpd_err_printf("closed command received from unknown context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed_request command for the parent context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed_request command to parent: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed_request command to parent.\n");
	smpd_exit_fn("handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "closed_request") == 0)
    {
	result = smpd_create_command("closed", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a closed command for the context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending closed command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the closed command to the context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("posted closed command to context.\n");
	smpd_exit_fn("handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "launch") == 0)
    {
	result = handle_launch_command(context);
	smpd_exit_fn("handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "connect") == 0)
    {
	if (smpd_process.root_smpd)
	{
	    smpd_err_printf("the root smpd is not allowed to connect to other smpds.\n");
	    /* send connect failed return command */
	    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a result command for the connect request.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the tag to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - root smpd is not allowed to connect to other smpds.");
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result string to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the result command to the context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (smpd_process.closing)
	{
	    smpd_err_printf("connect command received while session is closing, ignoring connect.\n");
	    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a result command for the connect request.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the tag to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - connect command received while closing.");
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result string to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the result command to the context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (!smpd_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH))
	{
	    smpd_err_printf("connect command does not have a target host argument, discarding: \"%s\"\n", cmd->cmd);
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (!smpd_get_int_arg(cmd->cmd, "id", &dest_id))
	{
	    smpd_err_printf("connect command does not have a target id argument, discarding: \"%s\"\n", cmd->cmd);
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (dest_id < smpd_process.id)
	{
	    smpd_dbg_printf("connect command has an invalid id, discarding: %d\n", dest_id);
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (smpd_process.left_context != NULL && smpd_process.right_context != NULL)
	{
	    smpd_err_printf("unable to connect to a new session, left and right sessions already exist, discarding.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	smpd_dbg_printf("now connecting to %s\n", host);
	/* create a new context */
	result = smpd_create_context(SMPD_CONTEXT_INVALID, SOCK_INVALID_SET, SOCK_INVALID_SOCK, -1, &dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a new context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}

	dest_set = smpd_process.set;
	result = smpd_connect_to_smpd(smpd_process.parent_context->set, smpd_process.parent_context->sock,
	    host, SMPD_PROCESS_SESSION_STR, dest_id, &dest_set, &dest_sock);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to connect to %s\n", host);
	    /* send fail connect command back */
	    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a result command for the connect request.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the tag to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - unable to connect to smpd.");
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result string to the result command.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the result command to the context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    smpd_exit_fn("handle_command");
	    return SMPD_SUCCESS;
	}
	if (dest_set != smpd_process.set)
	{
	    smpd_err_printf("connect_to_smpd returned a new set instead of adding the new sock to the provided set.\n");
	    smpd_err_printf("dest_set:%d != process_set:%d\n", sock_getsetid(dest_set), sock_getsetid(smpd_process.set));
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	if (smpd_process.left_context == NULL)
	{
	    smpd_dbg_printf("adding new left child context\n");
	    smpd_init_context(dest, SMPD_CONTEXT_LEFT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.left_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
	else if (smpd_process.right_context == NULL)
	{
	    smpd_dbg_printf("adding new right child context\n");
	    smpd_init_context(dest, SMPD_CONTEXT_RIGHT_CHILD, dest_set, dest_sock, dest_id);
	    smpd_process.right_context = dest;
	    sock_set_user_ptr(dest_sock, dest);
	}
	else
	{
	    smpd_err_printf("impossible to be here, both left and right contexts are non-NULL.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	/* post a read for the next command to come over the new context */
	result = smpd_post_read_command(dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on the newly connected context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	/* write a success result back to the connect requester */
	result = smpd_create_command("result", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn("handle_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("handle_command");
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd->cmd_str, "print") == 0)
    {
	smpd_dbg_printf("PRINT: node %s:%d, level %d, parent = %d, left = %d, right = %d\n",
	    smpd_process.host,
	    smpd_process.id,
	    smpd_process.level,
	    /*smpd_process.parent_context ? smpd_process.parent_context->id : -1,*/
	    smpd_process.parent_id,
	    smpd_process.left_context ? smpd_process.left_context->id : -1,
	    smpd_process.right_context ? smpd_process.right_context->id : -1);
	if (smpd_process.left_context)
	{
	    result = smpd_create_command("print", smpd_process.id, smpd_process.left_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a 'print' command for the left context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write for the 'print' command to the left context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	}
	if (smpd_process.right_context)
	{
	    result = smpd_create_command("print", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a 'print' command for the right context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write for the 'print' command to the right context.\n");
		smpd_exit_fn("handle_command");
		return SMPD_FAIL;
	    }
	}
    }
    else
    {
	/* handle root commands */
	if (smpd_process.root_smpd)
	{
	    if (strcmp(cmd->cmd_str, "shutdown") == 0)
	    {
		result = smpd_create_command("down", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a closed command for the context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("shutdown received, replying with down command: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the closed command to the context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
		smpd_exit_fn("handle_command");
		return SMPD_CLOSE; /* return close to prevent posting another read on this context */
	    }
	    else if (strcmp(cmd->cmd_str, "stat") == 0)
	    {
		result = smpd_create_command("all ok", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create an 'all ok' command for the context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("replying to stat command: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the 'all ok' command to the context.\n");
		    smpd_exit_fn("handle_command");
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		smpd_err_printf("ignoring unknown session command: \"%s\"\n", cmd->cmd);
	    }
	}
	else
	{
	    smpd_err_printf("ignoring unknown session command: \"%s\"\n", cmd->cmd);
	}
    }
    smpd_exit_fn("handle_command");
    return SMPD_SUCCESS;
}

int smpd_interpret_session_header(char *str)
{
    char temp_str[100];

    smpd_enter_fn("smpd_interpret_session_header");

    smpd_dbg_printf("interpreting session header: \"%s\"\n", str);

    /* get my id */
    if (smpd_get_string_arg(str, "id", temp_str, 100))
    {
	smpd_dbg_printf(" id = %s\n", temp_str);
	smpd_process.id = atoi(temp_str);
	if (smpd_process.id < 0)
	{
	    smpd_err_printf("invalid id passed in session header: %d\n", smpd_process.id);
	    smpd_process.id = 0;
	}
    }

    /* get my parent's id */
    if (smpd_get_string_arg(str, "parent", temp_str, 100))
    {
	smpd_dbg_printf(" parent = %s\n", temp_str);
	smpd_process.parent_id = atoi(temp_str);
	if (smpd_process.parent_id < 0)
	{
	    smpd_err_printf("invalid parent id passed in session header: %d\n", smpd_process.parent_id);
	    smpd_process.parent_id = -1;
	}
    }

    /* get my level */
    if (smpd_get_string_arg(str, "level", temp_str, 100))
    {
	smpd_dbg_printf(" level = %s\n", temp_str);
	smpd_process.level = atoi(temp_str);
	if (smpd_process.level < 0)
	{
	    smpd_err_printf("invalid session level passed in session header: %d\n", smpd_process.level);
	    smpd_process.level = 0;
	}
    }

    smpd_exit_fn("smpd_interpret_session_header");
    return SMPD_SUCCESS;
}

int smpd_handle_read(smpd_context_t *context)
{
    int result;
    int ret_val = SMPD_SUCCESS;

    smpd_enter_fn("smpd_handle_read");

    /* handle the two special contexts */
    if (context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR)
    {
	sock_size_t num_read;
	smpd_command_t *temp_cmd;
	char buffer[SMPD_MAX_CMD_LENGTH];
	int num_encoded;

	/* one byte read, attempt to read up to the buffer size */
	result = sock_read(context->sock, &context->read_cmd.cmd[1], SMPD_MAX_CMD_LENGTH-1, &num_read);
	if (result != SOCK_SUCCESS)
	{
	    num_read = 0;
	    smpd_dbg_printf("sock_read(%d) failed (%s), assuming %s is closed.\n",
		sock_getid(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
	}
	smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));
	smpd_encode_buffer(buffer, SMPD_MAX_CMD_LENGTH, context->read_cmd.cmd, num_read+1, &num_encoded);
	buffer[num_encoded*2] = '\0';
	smpd_dbg_printf("encoded %d characters: %d '%s'\n", num_encoded, strlen(buffer), buffer);

	/* create an output command */
	result = smpd_create_command(
	    smpd_get_context_str(context),
	    smpd_process.id, 0 /* output always goes to node 0? */, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an output command.\n");
	    smpd_exit_fn("smpd_handle_read");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "rank", context->rank);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the rank to the stdout command.\n");
	    smpd_exit_fn("smpd_handle_read");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "data", buffer);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the data to the stdout command.\n");
	    smpd_exit_fn("smpd_handle_read");
	    return SMPD_FAIL;
	}

	/* send the stdout command */
	result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the stdout command.\n");
	    smpd_exit_fn("smpd_handle_read");
	    return SMPD_FAIL;
	}

	/* post a read for the next byte of data */
	result = sock_post_read(context->sock, &context->read_cmd.cmd, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_dbg_printf("sock_post_read failed (%s), assuming %s is closed, calling sock_post_close(%d).\n",
		get_sock_error_string(result), smpd_get_context_str(context), sock_getid(context->sock));
	    /*
	    smpd_err_printf("unable to post a read of the next byte of data from %s, error: %s\n",
		smpd_get_context_str(context),
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_handle_read");
	    return SMPD_FAIL;
	    */
	    result = sock_post_close(context->sock);
	    if (result != SOCK_SUCCESS)
	    {
		smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		smpd_exit_fn("smpd_handle_read");
		return SMPD_FAIL;
	    }
	}

	smpd_exit_fn("smpd_handle_read");
	return SMPD_SUCCESS;
    }

    /* check for an invalid context */
    if (context->type == SMPD_CONTEXT_STDIN)
    {
	smpd_err_printf("illegal read signalled on stdin context.\n");
	smpd_exit_fn("smpd_handle_read");
	return SMPD_FAIL;
    }

    /* handle a traditional context */
    switch (context->read_cmd.state)
    {
    case SMPD_CMD_INVALID:
	smpd_err_printf("data read on a command in the invalid state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_HDR:
	context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
	if (context->read_cmd.length < 1)
	{
	    smpd_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
	    ret_val = SMPD_FAIL;
	    break;
	}
	smpd_dbg_printf("command header read, posting read for data: %d bytes\n", context->read_cmd.length);
	context->read_cmd.state = SMPD_CMD_READING_CMD;
	ret_val = sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, NULL);
	if (ret_val == SOCK_SUCCESS)
	    ret_val = SMPD_SUCCESS;
	else
	{
	    smpd_err_printf("unable to post a read for the command string, sock error:\n%s\n", get_sock_error_string(ret_val));
	    ret_val = SMPD_FAIL;
	}
	break;
    case SMPD_CMD_READING_CMD:
	ret_val = smpd_parse_command(&context->read_cmd);
	if (ret_val != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
	    break;
	}
	smpd_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
	context->read_cmd.state = SMPD_CMD_READY;
	ret_val = handle_command(context);
	if (ret_val == SMPD_SUCCESS)
	{
	    ret_val = smpd_post_read_command(context);
	}
	else if (ret_val == SMPD_CLOSE)
	{
	    smpd_dbg_printf("not posting read for another command because SMPD_CLOSE returned\n");
	    ret_val = SMPD_SUCCESS;
	}
	else
	{
	    smpd_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
	}
	break;
    case SMPD_CMD_WRITING_CMD:
	smpd_err_printf("data read on a command in the writing_cmd state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READY:
	smpd_err_printf("data read on a command in the ready state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_HANDLED:
	smpd_err_printf("data read on a command in the handled state\n");
	ret_val = SMPD_FAIL;
	break;
    default:
	smpd_err_printf("data read on a command in an invalid state: %d\n", context->read_cmd.state);
	ret_val = SMPD_FAIL;
	break;
    }

    smpd_exit_fn("smpd_handle_read");
    return ret_val;
}

int smpd_handle_written(smpd_context_t *context)
{
    smpd_command_t *cmd, *iter;
    int ret_val = SMPD_SUCCESS;

    smpd_enter_fn("smpd_handle_written");

    if (context->write_list == NULL)
    {
	smpd_err_printf("data written on a context with no write command posted.\n");
	smpd_exit_fn("smpd_handle_written");
	return SMPD_FAIL;
    }
    switch (context->write_list->state)
    {
    case SMPD_CMD_INVALID:
	smpd_err_printf("data written on a command in the invalid state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_HDR:
	smpd_err_printf("data written on a command in the reading_hdr state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_READING_CMD:
	smpd_err_printf("data written on a command in the reading_cmd state.\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_WRITING_CMD:
	cmd = context->write_list;
	context->write_list = context->write_list->next;
	smpd_dbg_printf("command written: \"%s\"\n", cmd->cmd);
	if (strcmp(cmd->cmd_str, "closed") == 0)
	{
	    smpd_dbg_printf("closed command written, posting close of the sock.\n");
	    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(context->sock));
	    ret_val = sock_post_close(context->sock);
	    if (ret_val != SOCK_SUCCESS)
	    {
		smpd_err_printf("unable to post a close of the sock after writing a 'closed' command, sock error:\n%s\n",
		    get_sock_error_string(ret_val));
		smpd_free_command(cmd);
		ret_val = SMPD_FAIL;
		break;
	    }
	}
	else if (strcmp(cmd->cmd_str, "down") == 0)
	{
	    smpd_dbg_printf("down command written, exiting.\n");
	    smpd_close_connection(context->set, context->sock);
	    smpd_dbg_printf("shutting down.\n");
	    smpd_free_command(cmd);
	    smpd_exit(0);
	}

	if (cmd->wait)
	{
	    /* If this command expects a reply, move it to the wait list */
	    smpd_dbg_printf("moving '%s' command to the wait_list.\n", cmd->cmd_str);
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
	    ret_val = smpd_free_command(cmd);
	    if (ret_val != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to free the written command.\n");
		break;
	    }
	}

	cmd = context->write_list;
	if (cmd)
	{
	    cmd->iov[0].SOCK_IOV_BUF = cmd->cmd_hdr_str;
	    cmd->iov[0].SOCK_IOV_LEN = SMPD_CMD_HDR_LENGTH;
	    cmd->iov[1].SOCK_IOV_BUF = cmd->cmd;
	    cmd->iov[1].SOCK_IOV_LEN = cmd->length;
	    smpd_dbg_printf("smpd_handle_written: posting write(%d bytes) for command: \"%s\"\n",
		cmd->iov[0].SOCK_IOV_LEN + cmd->iov[1].SOCK_IOV_LEN, cmd->cmd);
	    ret_val = sock_post_writev(context->sock, cmd->iov, 2, NULL);
	    if (ret_val != SOCK_SUCCESS)
	    {
		smpd_err_printf("unable to post a write for the next command, sock error:\n%s\n", get_sock_error_string(ret_val));
		ret_val = SMPD_FAIL;
	    }
	}
	break;
    case SMPD_CMD_READY:
	smpd_err_printf("data written on a command in the ready state\n");
	ret_val = SMPD_FAIL;
	break;
    case SMPD_CMD_HANDLED:
	smpd_err_printf("data written on a command in the handled state\n");
	ret_val = SMPD_FAIL;
	break;
    default:
	smpd_err_printf("data written on a command in an invalid state: %d\n", context->write_list->state);
	ret_val = SMPD_FAIL;
	break;
    }

    smpd_exit_fn("smpd_handle_written");
    return ret_val;
}

int smpd_post_close_context(smpd_context_t *context)
{
    int result;

    smpd_enter_fn("smpd_post_close_context");

    if (context == NULL)
    {
	smpd_exit_fn("smpd_post_close_context");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(context->sock));
    result = sock_post_close(context->sock);
    if (result != SOCK_SUCCESS)
    {
	if (context == smpd_process.left_context)
	{
	    smpd_free_context(smpd_process.left_context);
	    smpd_process.left_context = NULL;
	}
	else if (context == smpd_process.right_context)
	{
	    smpd_free_context(smpd_process.right_context);
	    smpd_process.right_context = NULL;
	}
	else if (context == smpd_process.parent_context)
	{
	    smpd_free_context(smpd_process.parent_context);
	    smpd_exit_fn("smpd_post_close_context");
	    return SMPD_FAIL;
	}
	if (smpd_process.parent_context == NULL)
	{
	    smpd_exit_fn("smpd_post_close_context");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_post_close_context");
    return SMPD_SUCCESS;
}

int smpd_session(sock_set_t set, sock_t sock)
{
    int result = SMPD_SUCCESS;
    smpd_context_t *context;
    smpd_process_t *trailer, *iter;
    sock_event_t event;
    char session_header[SMPD_MAX_SESSION_HEADER_LENGTH];
    smpd_command_t *temp_cmd;

    smpd_enter_fn("smpd_session");

    /* read and interpret the session header */
    smpd_dbg_printf("reading and interpreting the session header on sock %d\n", sock_getid(sock));
    result = smpd_read(sock, session_header, SMPD_MAX_SESSION_HEADER_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to read the session header string.\n");
	smpd_close_connection(set, sock);
	smpd_exit_fn("smpd_session");
	return SMPD_FAIL;
    }
    result = smpd_interpret_session_header(session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to interpret the session header string.\n");
	smpd_close_connection(set, sock);
	smpd_exit_fn("smpd_session");
	return SMPD_FAIL;
    }

    /* the set used in this initial parent child connection will be the set used for the two
       child sessions also */
    smpd_dbg_printf("setting smpd_process.set to set: %d\n", sock_getsetid(set));
    smpd_process.set = set;

    /* allocate and initialize a context */
    result = smpd_create_context(SMPD_CONTEXT_PARENT, set, sock, smpd_process.parent_id, &context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a new context.\n");
	smpd_exit_fn("smpd_session");
	return SMPD_FAIL;
    }
    smpd_process.parent_context = context;
    sock_set_user_ptr(sock, context);

    /* post a read for the first command header */
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the first command header of the session.\n");
	smpd_close_connection(set, sock);
	smpd_exit_fn("smpd_session");
	return SMPD_FAIL;
    }

    while (1)
    {
	smpd_dbg_printf("sock_waiting for the next event.\n");
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("session failure, sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_close_connection(set, sock);
	    smpd_exit_fn("smpd_session");
	    return SMPD_FAIL;
	}
	context = (smpd_context_t*)event.user_ptr;
	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    /* check for an error */
	    if (event.error != SOCK_SUCCESS)
	    {
		if (event.error != SOCK_EOF)
		{
		    smpd_err_printf("read failure, sock %d error: %s\n", sock_getid(context->sock),
			get_sock_error_string(event.error));
		}
		if (context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR)
		{
		    smpd_dbg_printf("rank %d %s closed.\n", context->rank,
			smpd_get_context_str(context));

#if 0
		    /* insert code to figure out when to send a process exited command */
		    if (context->type == SMPD_CONTEXT_STDOUT)
		    {
			/* create the process exited command */
			smpd_dbg_printf("creating an exit command in response to stdout closing.\n");
			result = smpd_create_command("exit", smpd_process.id, 0, SMPD_FALSE, &temp_cmd);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to create an exit command for rank %d\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}
			result = smpd_add_command_int_arg(temp_cmd, "rank", context->rank);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the rank %d to the exit command.\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}
			result = smpd_add_command_int_arg(temp_cmd, "code", 0);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the exit code to the exit command for rank %d\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}

			/* send the exit command */
			result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to post a write of the exit command for rank %d\n", context->rank);
			    smpd_exit_fn("handle_launch_command");
			    return SMPD_FAIL;
			}
		    }
#endif
		}

		result = smpd_post_close_context(context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_exit_fn("smpd_session");
		    return result;
		}
		break;
	    }
	    /*smpd_dbg_printf("calling smpd_handle_read.\n");*/
	    result = smpd_handle_read(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to handle read data.\n");
		result = smpd_post_close_context(context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_exit_fn("smpd_session");
		    return result;
		}
	    }
	    break;
	case SOCK_OP_WRITE:
	    /* check for an error */
	    if (event.error != SOCK_SUCCESS)
	    {
		if (event.error != SOCK_EOF)
		{
		    smpd_err_printf("write failure, sock %d error: %s\n", sock_getid(context->sock),
			get_sock_error_string(event.error));
		}
		if (context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR)
		{
		    smpd_dbg_printf("rank %d %s closed.\n", context->rank,
			smpd_get_context_str(context));

#if 0
		    /* insert code to figure out when to send a process exited command */
		    if (context->type == SMPD_CONTEXT_STDOUT)
		    {
			/* create the process exited command */
			smpd_dbg_printf("creating an exit command in response to stdout closing.\n");
			result = smpd_create_command("exit", smpd_process.id, 0, SMPD_FALSE, &temp_cmd);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to create an exit command for rank %d\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}
			result = smpd_add_command_int_arg(temp_cmd, "rank", context->rank);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the rank %d to the exit command.\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}
			result = smpd_add_command_int_arg(temp_cmd, "code", 0);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to add the exit code to the exit command for rank %d\n", context->rank);
			    smpd_exit_fn("smpd_session");
			    return SMPD_FAIL;
			}

			/* send the exit command */
			result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
			if (result != SMPD_SUCCESS)
			{
			    smpd_err_printf("unable to post a write of the exit command for rank %d\n", context->rank);
			    smpd_exit_fn("handle_launch_command");
			    return SMPD_FAIL;
			}
		    }
#endif
		}

		result = smpd_post_close_context(context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_exit_fn("smpd_session");
		    return result;
		}
		break;
	    }
	    /*smpd_dbg_printf("calling smpd_handle_written.\n");*/
	    result = smpd_handle_written(context);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to handle written data.\n");
		result = smpd_post_close_context(context);
		if (result != SMPD_SUCCESS)
		{
		    smpd_exit_fn("smpd_session");
		    return result;
		}
	    }
	    break;
	case SOCK_OP_ACCEPT:
	    smpd_err_printf("unexpected accept event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CONNECT:
	    smpd_err_printf("unexpected connect event returned by sock_wait.\n");
	    break;
	case SOCK_OP_CLOSE:
	    if (context == smpd_process.parent_context)
	    {
		smpd_dbg_printf("sock_wait returned op_close, freeing parent context\n");
		smpd_free_context(context);
		smpd_dbg_printf("closing the session.\n");
		result = sock_destroy_set(set);
		if (result != SOCK_SUCCESS)
		{
		    smpd_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		    smpd_exit_fn("smpd_session");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("**** EXITING SMPD_SESSION ****\n");
		smpd_exit_fn("smpd_session");
		return SMPD_SUCCESS;
	    }
	    else if (context == smpd_process.left_context)
	    {
		smpd_dbg_printf("sock_wait returned op_close, freeing left context\n");
		smpd_free_context(context);
		smpd_process.left_context = NULL;
	    }
	    else if (context == smpd_process.right_context)
	    {
		smpd_dbg_printf("sock_wait returned op_close, freeing right context\n");
		smpd_free_context(context);
		smpd_process.right_context = NULL;
	    }
	    else
	    {
		/*
		trailer = iter = smpd_process.context_list;
		while (iter)
		{
		    if (context == iter)
		    {
			smpd_dbg_printf("sock_wait returned op_close, freeing %s context\n", smpd_get_context_str(context));
			if (iter == smpd_process.context_list)
			{
			    smpd_process.context_list = smpd_process.context_list->next;
			}
			else
			{
			    trailer->next = iter->next;
			}
			smpd_free_context(context);
			context = NULL;
		    }
		    else
		    {
			if (trailer != iter)
			    trailer = trailer->next;
			iter = iter->next;
		    }
		}
		if (context)
		{
		    smpd_err_printf("sock_wait returned op_close for an unlisted %s context, freeing context\n",
			smpd_get_context_str(context));
		    smpd_free_context(context);
		}
		*/
		trailer = iter = smpd_process.process_list;
		while (iter)
		{
		    if (context == iter->in || context == iter->out || context == iter->err)
		    {
			smpd_dbg_printf("sock_wait returned op_close, releasing %s context\n", smpd_get_context_str(context));
			iter->num_valid_contexts--;
			if (context->type == SMPD_CONTEXT_STDOUT)
			{
			    smpd_dbg_printf("stdout closed, posting close of stdin.\n");
			    result = smpd_post_close_context(iter->in);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to post a close of the stdin context.\n");
			    }
			}
			if (iter->num_valid_contexts == 0)
			{
			    if (iter == smpd_process.process_list)
			    {
				smpd_process.process_list = smpd_process.process_list->next;
			    }
			    else
			    {
				trailer->next = iter->next;
			    }

			    /* create the process exited command */
			    smpd_dbg_printf("creating an exit command.\n");
			    result = smpd_create_command("exit", smpd_process.id, 0, SMPD_FALSE, &temp_cmd);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to create an exit command for rank %d\n", iter->rank);
				smpd_exit_fn("smpd_session");
				return SMPD_FAIL;
			    }
			    result = smpd_add_command_int_arg(temp_cmd, "rank", iter->rank);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to add the rank %d to the exit command.\n", iter->rank);
				smpd_exit_fn("smpd_session");
				return SMPD_FAIL;
			    }
			    result = smpd_wait_process(iter->wait, &iter->exitcode);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("waiting for the exited process failed.\n");
				smpd_exit_fn("smpd_session");
				return SMPD_FAIL;
			    }
			    result = smpd_add_command_int_arg(temp_cmd, "code", iter->exitcode);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to add the exit code to the exit command for rank %d\n", iter->rank);
				smpd_exit_fn("smpd_session");
				return SMPD_FAIL;
			    }

			    /* send the exit command */
			    result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
			    if (result != SMPD_SUCCESS)
			    {
				smpd_err_printf("unable to post a write of the exit command for rank %d\n", iter->rank);
				smpd_exit_fn("handle_launch_command");
				return SMPD_FAIL;
			    }

			    /* free the process structure */
			    smpd_free_process(iter);
			    iter = NULL;
			}
			context = NULL;
		    }
		    else
		    {
			if (trailer != iter)
			    trailer = trailer->next;
			iter = iter->next;
		    }
		}
		if (context)
		{
		    smpd_err_printf("sock_wait returned op_close for an unlisted %s context, freeing context\n",
			smpd_get_context_str(context));
		    smpd_free_context(context);
		}
	    }
	    if (smpd_process.closing)
	    {
		if (smpd_process.parent_context == NULL)
		{
		    smpd_err_printf("parent context is NULL during close command processing.\n");
		    smpd_close_connection(set, sock);
		    smpd_exit_fn("smpd_session");
		    return SMPD_FAIL;
		}
		if (smpd_process.left_context == NULL && smpd_process.right_context == NULL)
		{
		    /* all children have closed, send a 'closed' command to the parent */
		    result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a closed_request command for the parent context.\n");
			smpd_close_connection(set, sock);
			smpd_exit_fn("smpd_session");
			return SMPD_FAIL;
		    }
		    smpd_dbg_printf("sending 'closed_request' command to parent: \"%s\"\n", temp_cmd->cmd);
		    result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
			smpd_close_connection(set, sock);
			smpd_exit_fn("smpd_session");
			return SMPD_FAIL;
		    }
		    smpd_dbg_printf("posted 'closed_request' command to parent.\n");
		}
	    }
	    break;
	default:
	    smpd_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
	    break;
	}
    }

    smpd_close_connection(set, sock);
    smpd_exit_fn("smpd_session");
    return result;
}
