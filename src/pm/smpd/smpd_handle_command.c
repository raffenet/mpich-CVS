/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int handle_stdout_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;

    smpd_enter_fn("handle_stdout_command");

    cmd = &context->read_cmd;
    if (!smpd_get_int_arg(cmd->cmd, "rank", &rank))
    {
	rank = -1;
	smpd_err_printf("no rank in the stdout command: '%s'\n", cmd->cmd);
    }
    if (smpd_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH))
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	/*printf("[%d]", rank);*/
	fwrite(data, 1, num_decoded, stdout);
	fflush(stdout);
    }
    else
    {
	smpd_err_printf("unable to get the data from the stdout command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn("handle_stdout_command");
    return SMPD_SUCCESS;
}

int handle_stderr_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;

    smpd_enter_fn("handle_stderr_command");

    cmd = &context->read_cmd;
    if (!smpd_get_int_arg(cmd->cmd, "rank", &rank))
    {
	rank = -1;
	smpd_err_printf("no rank in the stderr command: '%s'\n", cmd->cmd);
    }
    if (smpd_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH))
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	/*fprintf(stderr, "[%d]", rank);*/
	fwrite(data, 1, num_decoded, stderr);
	fflush(stderr);
    }
    else
    {
	smpd_err_printf("unable to get the data from the stderr command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn("handle_stderr_command");
    return SMPD_SUCCESS;
}

int launch_processes()
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_launch_node_t *launch_node_ptr;

    /* launch the processes */
    smpd_dbg_printf("launching the processes.\n");
    launch_node_ptr = smpd_process.launch_list;
    while (launch_node_ptr)
    {
	/* create the launch command */
	result = smpd_create_command("launch", 0, 
	    launch_node_ptr->host_id,
	    SMPD_TRUE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a launch command.\n");
	    goto launch_failure;
	}
	result = smpd_add_command_arg(cmd_ptr, "c", launch_node_ptr->exe);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the command line to the launch command: '%s'\n", launch_node_ptr->exe);
	    goto launch_failure;
	}
	if (launch_node_ptr->env[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "e", launch_node_ptr->env);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the environment variables to the launch command: '%s'\n", launch_node_ptr->env);
		goto launch_failure;
	    }
	}
	result = smpd_add_command_int_arg(cmd_ptr, "i", launch_node_ptr->iproc);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the rank to the launch command: %d\n", launch_node_ptr->iproc);
	    goto launch_failure;
	}

	/* send the launch command */
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the launch command:\n id: %d\n rank: %d\n cmd: '%s'\n",
		launch_node_ptr->host_id, launch_node_ptr->iproc, launch_node_ptr->exe);
	    goto launch_failure;
	}

	/* increment the number of launched processes */
	smpd_process.nproc++;

	/* move to the next node */
	launch_node_ptr = launch_node_ptr->next;
    }
    return SMPD_SUCCESS;
launch_failure:
    return SMPD_FAIL;
}

int handle_result(smpd_context_t *context)
{
    int result, ret_val;
    char str[1024];
    smpd_command_t *iter, *trailer;
    int match_tag;

    smpd_enter_fn("handle_result");

    if (!smpd_get_int_arg(context->read_cmd.cmd, "cmd_tag", &match_tag))
    {
	smpd_err_printf("result command received without a cmd_tag field: '%s'\n", context->read_cmd.cmd);
	smpd_exit_fn("handle_result");
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
		    {
			ret_val = SMPD_CONNECTED;
			switch (context->state)
			{
			case SMPD_MPIEXEC_CONNECTING_TREE:
			    smpd_dbg_printf("successful connect, state: connecting tree.\n");
			    break;
			case SMPD_MPIEXEC_CONNECTING_SMPD:
			    smpd_dbg_printf("successful connect, state: connecting smpd.\n");
			    break;
			case SMPD_CONNECTING:
			    smpd_dbg_printf("successful connect, state: connecting.\n");
			    break;
			default:
			    break;
			}
		    }
		    else
		    {
			smpd_err_printf("connect failed: %s\n", str);
			ret_val = SMPD_FAIL;
		    }
		}
		else if (strcmp(iter->cmd_str, "launch") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			smpd_dbg_printf("successfully launched: '%s'\n", iter->cmd);
		    }
		    else
		    {
			smpd_err_printf("launch failed: %s\n", str);
			ret_val = SMPD_FAIL;
		    }
		}
		else if (strcmp(iter->cmd_str, "start_dbs") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			if (smpd_get_string_arg(context->read_cmd.cmd, "kvs_name", smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN))
			{
			    smpd_dbg_printf("start_dbs succeeded, kvs_name: '%s'\n", smpd_process.kvs_name);
			    ret_val = launch_processes();
			}
			else
			{
			    smpd_err_printf("invalid start_dbs result returned, no kvs_name specified: '%s'\n", context->read_cmd.cmd);
			    ret_val = SMPD_FAIL;
			}
		    }
		    else
		    {
			smpd_err_printf("start_dbs failed: %s\n", str);
			ret_val = SMPD_FAIL;
		    }
		}
		else
		{
		    smpd_err_printf("result returned for unhandled command:\n command: '%s'\n result: '%s'\n", iter->cmd, str);
		}
	    }
	    else
	    {
		smpd_err_printf("no result string in the result command.\n");
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
		smpd_err_printf("unable to free command in the wait_list\n");
	    }
	    smpd_exit_fn("handle_result");
	    return ret_val;
	}
	else
	{
	    smpd_dbg_printf("tag %d != match_tag %d\n", iter->tag, match_tag);
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }
    if (context->wait_list == NULL)
    {
	smpd_err_printf("result command received but the wait_list is empty.\n");
    }
    else
    {
	smpd_err_printf("result command did not match any commands in the wait_list.\n");
    }
    smpd_exit_fn("handle_result");
    return SMPD_FAIL;
}

int handle_dbs_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_dbs_command");

    cmd = &context->read_cmd;

    if (!smpd_process.have_dbs)
    {
	/* create a failure reply because this node does not have an initialized database */
	smpd_dbg_printf("sending a failure reply because this node does not have an initialized database.\n");
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_dbs_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_dbs_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - smpd does not have an initialized database.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_dbs_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn("handle_dbs_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("handle_dbs_command");
	return SMPD_SUCCESS;
    }

    /* create a reply */
    smpd_dbg_printf("sending reply to dbs command '%s'.\n", cmd->cmd);
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_dbs_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_dbs_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_dbs_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn("handle_dbs_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("handle_dbs_command");
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
    result = smpd_create_process_struct(iproc, &process);
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
    smpd_get_string_arg(cmd->cmd, "e", process->env, SMPD_MAX_ENV_LENGTH);
    smpd_get_string_arg(cmd->cmd, "d", process->dir, SMPD_MAX_EXE_LENGTH);
    smpd_get_string_arg(cmd->cmd, "p", process->path, SMPD_MAX_EXE_LENGTH);

    /* launch the process */
    smpd_dbg_printf("launching: '%s'\n", process->exe);
    result = smpd_launch_process(process, 2, 3, SMPD_FALSE, context->set);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("launch_process failed.\n");

	/* create the result command */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}
	/* launch process should provide a reason for the error, for now just return FAIL */
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result field to the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}

	/* send the result back */
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command in response to launch command: '%s'\n", cmd->cmd);
	    smpd_free_process_struct(process);
	    smpd_exit_fn("handle_launch_command");
	    return SMPD_FAIL;
	}

	/* free the failed process structure */
	smpd_free_process_struct(process);
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

int handle_close_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_close_command");

    cmd = &context->read_cmd;

    smpd_process.closing = SMPD_TRUE;
    if (smpd_process.left_context || smpd_process.right_context)
    {
	if (smpd_process.left_context)
	{
	    result = smpd_create_command("close", smpd_process.id, smpd_process.left_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a close command for the left context.\n");
		smpd_exit_fn("handle_close_command");
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending close command to left child: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of a close command for the left context.\n");
		smpd_exit_fn("handle_close_command");
		return SMPD_FAIL;
	    }
	}
	if (smpd_process.right_context)
	{
	    result = smpd_create_command("close", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a close command for the right context.\n");
		smpd_exit_fn("handle_close_command");
		return SMPD_FAIL;
	    }
	    smpd_dbg_printf("sending close command to right child: \"%s\"\n", temp_cmd->cmd);
	    result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of a close command for the right context.\n");
		smpd_exit_fn("handle_close_command");
		return SMPD_FAIL;
	    }
	}
	smpd_exit_fn("handle_close_command");
	/* If we return success here, a post_read will be made for the next command header. */
	return SMPD_SUCCESS;
	/*return SMPD_CLOSE;*/
    }
    result = smpd_create_command("closed", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed command for the parent context.\n");
	smpd_exit_fn("handle_close_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending closed command to parent: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed command to the parent context.\n");
	smpd_exit_fn("handle_close_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("posted closed command.\n");

    smpd_exit_fn("handle_close_command");
    return SMPD_CLOSE;
}

int handle_closed_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_closed_command");

    cmd = &context->read_cmd;

    if (context == smpd_process.left_context)
    {
	smpd_dbg_printf("closed command received from left child, closing sock.\n");
	smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.left_context->sock));
	smpd_process.left_context->state = SMPD_CLOSING;
	sock_post_close(smpd_process.left_context->sock);
	if (smpd_process.right_context)
	{
	    smpd_exit_fn("handle_closed_command");
	    return SMPD_CLOSE;
	}
    }
    else if (context == smpd_process.right_context)
    {
	smpd_dbg_printf("closed command received from right child, closing sock.\n");
	smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.right_context->sock));
	smpd_process.right_context->state = SMPD_CLOSING;
	sock_post_close(smpd_process.right_context->sock);
	if (smpd_process.left_context)
	{
	    smpd_exit_fn("handle_closed_command");
	    return SMPD_CLOSE;
	}
    }
    else if (context == smpd_process.parent_context)
    {
	smpd_dbg_printf("closed command received from parent, closing sock.\n");
	smpd_dbg_printf("sock_post_close(%d)\n", sock_getid(smpd_process.parent_context->sock));
	smpd_process.parent_context->state = SMPD_CLOSING;
	sock_post_close(smpd_process.parent_context->sock);
	smpd_exit_fn("handle_closed_command");
	return SMPD_EXITING;
    }
    else
    {
	smpd_err_printf("closed command received from unknown context.\n");
	smpd_exit_fn("handle_closed_command");
	return SMPD_FAIL;
    }
    if (smpd_process.parent_context == NULL)
    {
	context->state = SMPD_EXITING;
	smpd_dbg_printf("received a closed at node with no parent context, assuming root, returning SMPD_EXITING.\n");
	smpd_exit_fn("handle_closed_command");
	return SMPD_EXITING;
    }
    result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed_request command for the parent context.\n");
	smpd_exit_fn("handle_closed_command");
	return SMPD_FAIL;
    }
    /*smpd_dbg_printf("posting write of closed_request command to parent: \"%s\"\n", temp_cmd->cmd);*/
    result = smpd_post_write_command(smpd_process.parent_context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
	smpd_exit_fn("handle_closed_command");
	return SMPD_FAIL;
    }
	
    smpd_exit_fn("handle_closed_command");
    return SMPD_CLOSE;
}

int handle_closed_request_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_closed_request_command");

    cmd = &context->read_cmd;

    result = smpd_create_command("closed", smpd_process.id, context->id, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a closed command for the context.\n");
	smpd_exit_fn("handle_closed_request_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending closed command to context: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the closed command to the context.\n");
	smpd_exit_fn("handle_closed_request_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("posted closed command to context.\n");
    smpd_exit_fn("handle_closed_request_command");
    return SMPD_CLOSE;
}

int handle_connect_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    smpd_context_t *dest;
    sock_set_t dest_set;
    sock_t dest_sock;
    int dest_id;
    char host[SMPD_MAX_HOST_LENGTH];

    smpd_enter_fn("handle_connect_command");

    cmd = &context->read_cmd;

    if (smpd_process.root_smpd)
    {
	smpd_err_printf("the root smpd is not allowed to connect to other smpds.\n");
	/* send connect failed return command */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - root smpd is not allowed to connect to other smpds.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (smpd_process.closing)
    {
	smpd_err_printf("connect command received while session is closing, ignoring connect.\n");
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - connect command received while closing.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (!smpd_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH))
    {
	smpd_err_printf("connect command does not have a target host argument, discarding: \"%s\"\n", cmd->cmd);
	/* return failure result */
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (!smpd_get_int_arg(cmd->cmd, "id", &dest_id))
    {
	smpd_err_printf("connect command does not have a target id argument, discarding: \"%s\"\n", cmd->cmd);
	/* return failure result */
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (dest_id < smpd_process.id)
    {
	smpd_dbg_printf("connect command has an invalid id, discarding: %d\n", dest_id);
	/* return failure result */
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (smpd_process.left_context != NULL && smpd_process.right_context != NULL)
    {
	smpd_err_printf("unable to connect to a new session, left and right sessions already exist, discarding.\n");
	/* return failure result */
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("now connecting to %s\n", host);
    /* create a new context */
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, context->set, SOCK_INVALID_SOCK, dest_id, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a new context.\n");
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }

    dest_set = context->set; /*smpd_process.set;*/

    /* start the connection logic here */
    result = sock_post_connect(dest_set, dest, host, SMPD_LISTENER_PORT, &dest_sock);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a connect to start the connect command, sock error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }

#if 0
    /* This failure block needs to be called everywhere that failures during the connect logic occur */
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to connect to %s\n", host);
	/* send fail connect command back */
	result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command for the connect request.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - unable to connect to smpd.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to context: \"%s\"\n", temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context.\n");
	    smpd_exit_fn("handle_connect_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (dest_set != smpd_process.set)
    {
	smpd_err_printf("connect_to_smpd returned a new set instead of adding the new sock to the provided set.\n");
	smpd_err_printf("dest_set:%d != process_set:%d\n", sock_getsetid(dest_set), sock_getsetid(smpd_process.set));
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }
#endif
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
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }
    dest->state = SMPD_CONNECTING;
    dest->connect_to = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
    if (dest->connect_to == NULL)
    {
	smpd_err_printf("unable to allocate a host node structure.\n");
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }
    strcpy(dest->connect_to->host, host);
    dest->connect_to->id = dest_id;
    dest->connect_to->nproc = 1;
    dest->connect_to->parent = smpd_process.id;
    dest->connect_to->next = NULL;
    dest->connect_return_id = cmd->src;
    dest->connect_return_tag = cmd->tag;

    smpd_exit_fn("handle_connect_command");
    return SMPD_SUCCESS;
}

int handle_start_dbs_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_start_dbs_command");

    cmd = &context->read_cmd;

    if (smpd_process.have_dbs == SMPD_FALSE)
    {
	smpd_dbs_init();
	smpd_process.have_dbs = SMPD_TRUE;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_start_dbs_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_start_dbs_command");
	return SMPD_FAIL;
    }

    /* create a db */
    result = smpd_dbs_create(smpd_process.kvs_name);
    if (result == SMPD_DBS_SUCCESS)
    {
	/* send the name back to the root */
	result = smpd_add_command_arg(temp_cmd, "kvs_name", smpd_process.kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the kvs_name string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_start_dbs_command");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_start_dbs_command");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("unable to create a db\n");
	/* Shoud we send a bogus kvs_name field in the command so the reader won't blow up or complain that it is not there?
	result = smpd_add_command_arg(temp_cmd, "kvs_name", "0");
	if (result != SMPD_SUCCESS)
	{
	smpd_err_printf("unable to add the kvs_name string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("handle_start_dbs_command");
	return SMPD_FAIL;
	}
	*/
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("handle_start_dbs_command");
	    return SMPD_FAIL;
	}
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn("handle_start_dbs_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("handle_start_dbs_command");
    return result;
}

int handle_print_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_print_command");

    cmd = &context->read_cmd;

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
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the 'print' command to the left context.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
    }
    if (smpd_process.right_context)
    {
	result = smpd_create_command("print", smpd_process.id, smpd_process.right_context->id, SMPD_FALSE, &temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a 'print' command for the right context.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.right_context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the 'print' command to the right context.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("handle_print_command");
    return result;
}

int handle_stat_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_stat_command");

    cmd = &context->read_cmd;

    result = smpd_create_command("all ok", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an 'all ok' command for the context.\n");
	smpd_exit_fn("handle_stat_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("replying to stat command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the 'all ok' command to the context.\n");
	smpd_exit_fn("handle_stat_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("handle_stat_command");
    return result;
}

#if 0
/* use this template to add new command handler functions */
int handle__command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle__command");

    cmd = &context->read_cmd;

    smpd_exit_fn("handle__command");
    return result;
}
#endif

int smpd_handle_command(smpd_context_t *context)
{
    int result;
    smpd_context_t *dest;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("smpd_handle_command");

    cmd = &context->read_cmd;

    smpd_dbg_printf("handling command:\n");
    smpd_dbg_printf(" src  = %d\n", cmd->src);
    smpd_dbg_printf(" dest = %d\n", cmd->dest);
    smpd_dbg_printf(" cmd  = %s\n", cmd->cmd_str);
    smpd_dbg_printf(" tag  = %d\n", cmd->tag);
    smpd_dbg_printf(" ctx  = %s\n", smpd_get_context_str(context));
    smpd_dbg_printf(" len  = %d\n", cmd->length);
    smpd_dbg_printf(" str  = %s\n", cmd->cmd);

    /* set the command state to handled */
    context->read_cmd.state = SMPD_CMD_HANDLED;

    result = smpd_command_destination(cmd->dest, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("invalid command received, unable to determine the destination.\n");
	smpd_exit_fn("smpd_handle_command");
	return SMPD_SUCCESS;
    }
    if (dest)
    {
	smpd_dbg_printf("forwarding command to %d\n", dest->id);
	result = smpd_forward_command(context, dest);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to forward the command.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_SUCCESS;
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_SUCCESS;
    }
    if (strcmp(cmd->cmd_str, "close") == 0)
    {
	result = handle_close_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "closed") == 0)
    {
	result = handle_closed_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "closed_request") == 0)
    {
	result = handle_closed_request_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "result") == 0)
    {
	result = handle_result(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "exit") == 0)
    {
	int exitcode, iproc;

	if (!smpd_get_int_arg(cmd->cmd, "code", &exitcode))
	{
	    smpd_err_printf("no exit code in exit command: '%s'\n", cmd->cmd);
	}
	if (!smpd_get_int_arg(cmd->cmd, "rank", &iproc))
	{
	    smpd_err_printf("no iproc in exit command: '%s'\n", cmd->cmd);
	}
	if (smpd_process.output_exit_codes)
	{
	    printf("process %d exited with exit code %d\n", iproc, exitcode);
	    fflush(stdout);
	}
	smpd_process.nproc--;
	if (smpd_process.nproc == 0)
	{
	    smpd_dbg_printf("last process exited, returning SMPD_EXIT.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_EXIT;
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd->cmd_str, "stdout") == 0)
    {
	result = handle_stdout_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "stderr") == 0)
    {
	result = handle_stderr_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "launch") == 0)
    {
	result = handle_launch_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "connect") == 0)
    {
	result = handle_connect_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "print") == 0)
    {
	result = handle_print_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "start_dbs") == 0)
    {
	result = handle_start_dbs_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    /*else if (strncmp(cmd->cmd_str, "db", 2) == 0)*/
    else if ((cmd->cmd_str[0] == 'd') && (cmd->cmd_str[1] == 'b'))
    {
	/* handle database command */
	result = handle_dbs_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "down") == 0)
    {
	context->state = SMPD_EXITING;
	result = sock_post_close(context->sock);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on sock %d, sock error:\n%s\n",
		sock_getid(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_EXITING;
    }
    else
    {
	/* handle root commands */
	if (smpd_process.root_smpd)
	{
	    if (strcmp(cmd->cmd_str, "shutdown") == 0)
	    {
		result = smpd_create_command("down", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a closed command for the context.\n");
		    smpd_exit_fn("smpd_handle_command");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("shutdown received, replying with down command: \"%s\"\n", temp_cmd->cmd);
		result = smpd_post_write_command(context, temp_cmd);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the closed command to the context.\n");
		    smpd_exit_fn("smpd_handle_command");
		    return SMPD_FAIL;
		}
		smpd_exit_fn("smpd_handle_command");
		return SMPD_EXITING; /* return close to prevent posting another read on this context */
	    }
	    else if (strcmp(cmd->cmd_str, "stat") == 0)
	    {
		result = handle_stat_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
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

    smpd_exit_fn("smpd_handle_command");
    return SMPD_SUCCESS;
}
