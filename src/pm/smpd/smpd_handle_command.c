/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_handle_stdin_command(smpd_context_t *context)
{
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    smpd_process_t *piter;
    smpd_stdin_write_node_t *node, *iter;
    int result;
    MPIDU_Sock_size_t num_written, num_decoded;
    int nd;

    smpd_enter_fn("handle_stdin_command");

    cmd = &context->read_cmd;
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &nd);
	num_decoded = nd;
	/*printf("[%d]", rank);*/
	piter = smpd_process.process_list;
	while (piter)
	{
	    if (piter->rank == 0 || smpd_process.stdin_toall)
	    {
		if (piter->stdin_write_list != NULL)
		{
		    node = (smpd_stdin_write_node_t*)malloc(sizeof(smpd_stdin_write_node_t));
		    if (node == NULL)
			smpd_write(piter->in->sock, data, num_decoded);
		    else
		    {
			node->buffer = (char*)malloc(num_decoded);
			if (node->buffer == NULL)
			{
			    free(node);
			    smpd_write(piter->in->sock, data, num_decoded);
			}
			else
			{
			    /* add the node to the end of the write list */
			    node->length = num_decoded;
			    memcpy(node->buffer, data, num_decoded);
			    node->next = NULL;
			    iter = piter->stdin_write_list;
			    while (iter->next != NULL)
				iter = iter->next;
			    iter->next = node;
			}
		    }
		}
		else
		{
		    /* attempt to write the data immediately */
		    num_written = 0;
		    result = MPIDU_Sock_write(piter->in->sock, data, num_decoded, &num_written);
		    if (result != MPI_SUCCESS)
		    {
			smpd_err_printf("unable to write data to the stdin context of process %d\n", piter->rank);
		    }
		    else
		    {
			if (num_written != num_decoded)
			{
			    /* If not all the data is written, copy it to a temp buffer and post a write for the remaining data */
			    node = (smpd_stdin_write_node_t*)malloc(sizeof(smpd_stdin_write_node_t));
			    if (node == NULL)
				smpd_write(piter->in->sock, &data[num_written], num_decoded-num_written);
			    else
			    {
				node->buffer = (char*)malloc(num_decoded-num_written);
				if (node->buffer == NULL)
				{
				    free(node);
				    smpd_write(piter->in->sock, &data[num_written], num_decoded-num_written);
				}
				else
				{
				    /* add the node to write list */
				    node->length = num_decoded - num_written;
				    memcpy(node->buffer, &data[num_written], num_decoded-num_written);
				    node->next = NULL;
				    piter->stdin_write_list = node;
				    piter->in->write_state = SMPD_WRITING_DATA_TO_STDIN;
				    result = MPIDU_Sock_post_write(piter->in->sock, node->buffer, node->length, node->length, NULL);
				    if (result != MPI_SUCCESS)
				    {
					smpd_err_printf("unable to post a write of %d bytes to stdin for rank %d\n",
					    node->length, piter->rank);
				    }
				}
			    }
			}
		    }
		}
	    }
	    piter = piter->next;
	}
    }
    else
    {
	smpd_err_printf("unable to get the data from the stdin command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn("handle_stdin_command");
    return SMPD_SUCCESS;
}

int smpd_handle_stdout_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;
#ifdef HAVE_WINDOWS_H
    HANDLE hStdout;
    DWORD num_written;
#endif

    smpd_enter_fn("handle_stdout_command");

    cmd = &context->read_cmd;
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	rank = -1;
	smpd_err_printf("no rank in the stdout command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	/*printf("[%d]", rank);*/
#ifdef HAVE_WINDOWS_H
	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteFile(hStdout, data, num_decoded, &num_written, NULL);
#else
	fwrite(data, 1, num_decoded, stdout);
	fflush(stdout);
#endif
    }
    else
    {
	smpd_err_printf("unable to get the data from the stdout command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn("handle_stdout_command");
    return SMPD_SUCCESS;
}

int smpd_handle_stderr_command(smpd_context_t *context)
{
    int rank;
    char data[SMPD_MAX_STDOUT_LENGTH];
    smpd_command_t *cmd;
    int num_decoded;
#ifdef HAVE_WINDOWS_H
    HANDLE hStderr;
    DWORD num_written;
#endif

    smpd_enter_fn("handle_stderr_command");

    cmd = &context->read_cmd;
    if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &rank) != MPIU_STR_SUCCESS)
    {
	rank = -1;
	smpd_err_printf("no rank in the stderr command: '%s'\n", cmd->cmd);
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "data", data, SMPD_MAX_STDOUT_LENGTH) == MPIU_STR_SUCCESS)
    {
	smpd_decode_buffer(data, data, SMPD_MAX_STDOUT_LENGTH, &num_decoded);
	/*fprintf(stderr, "[%d]", rank);*/
#ifdef HAVE_WINDOWS_H
	hStderr = GetStdHandle(STD_ERROR_HANDLE);
	WriteFile(hStderr, data, num_decoded, &num_written, NULL);
#else
	fwrite(data, 1, num_decoded, stderr);
	fflush(stderr);
#endif
    }
    else
    {
	smpd_err_printf("unable to get the data from the stderr command: '%s'\n", cmd->cmd);
    }

    smpd_exit_fn("handle_stderr_command");
    return SMPD_SUCCESS;
}

int smpd_launch_processes()
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_launch_node_t *launch_node_ptr;
    smpd_map_drive_node_t *map_iter;
    char drive_map_str[SMPD_MAX_EXE_LENGTH];

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
	if (launch_node_ptr->dir[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "d", launch_node_ptr->dir);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the working directory to the launch command: '%s'\n", launch_node_ptr->dir);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->path[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "p", launch_node_ptr->path);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the search path to the launch command: '%s'\n", launch_node_ptr->path);
		goto launch_failure;
	    }
	}
	if (launch_node_ptr->clique[0] != '\0')
	{
	    result = smpd_add_command_arg(cmd_ptr, "q", launch_node_ptr->clique);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the clique string to the launch command: '%s'\n", launch_node_ptr->clique);
		goto launch_failure;
	    }
	}
	/*printf("creating launch command for rank %d\n", launch_node_ptr->iproc);*/
	result = smpd_add_command_int_arg(cmd_ptr, "i", launch_node_ptr->iproc);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the rank to the launch command: %d\n", launch_node_ptr->iproc);
	    goto launch_failure;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "n", launch_node_ptr->nproc);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the nproc field to the launch command: %d\n", launch_node_ptr->nproc);
	    goto launch_failure;
	}
	result = smpd_add_command_arg(cmd_ptr, "k", smpd_process.kvs_name);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the kvs name('%s') to the launch command\n", smpd_process.kvs_name);
	    goto launch_failure;
	}
	map_iter = launch_node_ptr->map_list;
	while (map_iter)
	{
	    sprintf(drive_map_str, "%c:%s", map_iter->drive, map_iter->share);
	    result = smpd_add_command_arg(cmd_ptr, "m", drive_map_str);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the drive mapping to the launch command: '%s'\n", drive_map_str);
		goto launch_failure;
	    }
	    map_iter = map_iter->next;
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

int smpd_handle_result(smpd_context_t *context)
{
    int result, ret_val;
    char str[SMPD_MAX_CMD_LENGTH];
    char err_msg[SMPD_MAX_ERROR_LEN];
    smpd_command_t *iter, *trailer, *cmd_ptr;
    int match_tag;
    char ctx_key[100];
    int process_id;
    smpd_context_t *pmi_context;
    smpd_process_t *piter;
    int rank;
    MPIDU_Sock_t insock;
    MPIDU_SOCK_NATIVE_FD stdin_fd;
    smpd_context_t *context_in;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif

    smpd_enter_fn("handle_result");

    if (context->type != SMPD_CONTEXT_PMI && MPIU_Str_get_string_arg(context->read_cmd.cmd, "ctx_key", ctx_key, 100) == MPIU_STR_SUCCESS)
    {
	process_id = atoi(ctx_key);
	smpd_dbg_printf("forwarding the dbs result command to the pmi context %d.\n", process_id);
	pmi_context = NULL;
	piter = smpd_process.process_list;
	while (piter)
	{
	    if (piter->id == process_id)
	    {
		pmi_context = piter->pmi;
		break;
	    }
	    piter = piter->next;
	}
	if (pmi_context == NULL)
	{
	    smpd_err_printf("received dbs result for a pmi context that doesn't exist: unmatched id = %d\n", process_id);
	    smpd_exit_fn("smpd_handle_result");
	    return SMPD_SUCCESS;
	}

	result = smpd_forward_command(context, pmi_context);
	smpd_exit_fn("smpd_handle_result");
	return result;
    }

    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "cmd_tag", &match_tag) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("result command received without a cmd_tag field: '%s'\n", context->read_cmd.cmd);
	smpd_exit_fn("smpd_handle_result");
	return SMPD_FAIL;
    }

    trailer = iter = context->wait_list;
    while (iter)
    {
	if (iter->tag == match_tag)
	{
	    ret_val = SMPD_SUCCESS;
	    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "result", str, SMPD_MAX_CMD_LENGTH) == MPIU_STR_SUCCESS)
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
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "launch") == 0)
		{
		    smpd_process.nproc_launched++;
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			smpd_dbg_printf("successfully launched: '%s'\n", iter->cmd);
			if (!smpd_process.stdin_redirecting)
			{
			    rank = 0;
			    MPIU_Str_get_int_arg(iter->cmd, "i", &rank);
			    if (rank == 0)
			    {
				smpd_dbg_printf("root process launched, starting stdin redirection.\n");
				/* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
				result = smpd_make_socket_loop((SOCKET*)&stdin_fd, &hWrite);
				if (result)
				{
				    smpd_err_printf("Unable to make a local socket loop to forward stdin.\n");
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
#else
				stdin_fd = fileno(stdin);
#endif

				/* convert the native handle to a sock */
				result = MPIDU_Sock_native_to_sock(smpd_process.set, stdin_fd, NULL, &insock);
				if (result != MPI_SUCCESS)
				{
				    smpd_err_printf("unable to create a sock from stdin,\nsock error: %s\n", get_sock_error_string(result));
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
				/* create a context for reading from stdin */
				result = smpd_create_context(SMPD_CONTEXT_MPIEXEC_STDIN, smpd_process.set, insock, -1, &context_in);
				if (result != SMPD_SUCCESS)
				{
				    smpd_err_printf("unable to create a context for stdin.\n");
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
				MPIDU_Sock_set_user_ptr(insock, context_in);

#ifdef HAVE_WINDOWS_H
				/* unfortunately, we cannot use stdin directly as a sock.  So, use a thread to read and forward
				   stdin to a sock */
				smpd_process.hCloseStdinThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				if (smpd_process.hCloseStdinThreadEvent == NULL)
				{
				    smpd_err_printf("Unable to create the stdin thread close event, error %d\n", GetLastError());
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
				smpd_process.hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StdinThread, (void*)hWrite, 0, &dwThreadID);
				if (smpd_process.hStdinThread == NULL)
				{
				    smpd_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
#endif
				/* set this variable first before posting the first read to avoid a race condition? */
				smpd_process.stdin_redirecting = SMPD_TRUE;
				/* post a read for a user command from stdin */
				context_in->read_state = SMPD_READING_STDIN;
				result = MPIDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
				if (result != MPI_SUCCESS)
				{
				    smpd_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
					get_sock_error_string(result));
				    smpd_exit_fn("smpd_handle_result");
				    return SMPD_FAIL;
				}
			    }
			}
		    }
		    else
		    {
			smpd_process.nproc_exited++;
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "error", err_msg, SMPD_MAX_ERROR_LEN) == MPIU_STR_SUCCESS)
			{
			    smpd_err_printf("launch failed: %s\n", err_msg);
			}
			else
			{
			    smpd_err_printf("launch failed: %s\n", str);
			}
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "start_dbs") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "kvs_name", smpd_process.kvs_name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
			{
			    smpd_dbg_printf("start_dbs succeeded, kvs_name: '%s'\n", smpd_process.kvs_name);
			    ret_val = smpd_launch_processes();
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
			ret_val = SMPD_ABORT;
		    }
		}
		else if (strcmp(iter->cmd_str, "spawn") == 0)
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (iter->cmd_str[0] == 'd' && iter->cmd_str[1] == 'b')
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (strcmp(iter->cmd_str, "barrier") == 0)
		{
		    smpd_dbg_printf("%s command result = %s\n", iter->cmd_str, str);
		    ret_val = SMPD_DBS_RETURN;
		}
		else if (strcmp(iter->cmd_str, "validate") == 0)
		{
		    /* print the result of the validate command */
		    printf("%s\n", str);
		    /* close the session */
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "status") == 0)
		{
		    /* print the result of the status command */
		    printf("smpd running on %s\n", smpd_process.console_host);
		    printf("dynamic hosts: %s\n", str);
		    ret_val = smpd_create_command("done", smpd_process.id, context->id, SMPD_FALSE, &cmd_ptr);
		    if (ret_val == SMPD_SUCCESS)
		    {
			ret_val = smpd_post_write_command(context, cmd_ptr);
			if (ret_val == SMPD_SUCCESS)
			{
			    ret_val = SMPD_CLOSE;
			}
			else
			{
			    smpd_err_printf("unable to post a write of a done command.\n");
			}
		    }
		    else
		    {
			smpd_err_printf("unable to create a done command.\n");
		    }
		}
		else if (strcmp(iter->cmd_str, "get") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			/* print the result of the get command */
			char value[SMPD_MAX_VALUE_LENGTH];
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "value", value, SMPD_MAX_VALUE_LENGTH) == MPIU_STR_SUCCESS)
			{
			    printf("%s\n", value);
			}
			else
			{
			    printf("Error: unable to get the value from the result command - '%s'\n", context->read_cmd.cmd);
			}
		    }
		    else
		    {
			printf("%s\n", str);
		    }
		}
		else if (strcmp(iter->cmd_str, "set") == 0 || strcmp(iter->cmd_str, "delete") == 0)
		{
		    /* print the result of the set command */
		    printf("%s\n", str);
		}
		else if (strcmp(iter->cmd_str, "stat") == 0)
		{
		    /* print the result of the stat command */
		    printf("%s\n", str);
		}
		else if (strcmp(iter->cmd_str, "cred_request") == 0)
		{
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "account", smpd_process.UserAccount, SMPD_MAX_ACCOUNT_LENGTH) == MPIU_STR_SUCCESS &&
			    MPIU_Str_get_string_arg(context->read_cmd.cmd, "password", smpd_process.UserPassword, SMPD_MAX_PASSWORD_LENGTH) == MPIU_STR_SUCCESS)
			{
			    smpd_dbg_printf("cred_request succeeded, account: '%s'\n", smpd_process.UserAccount);
			    strcpy(iter->context->account, smpd_process.UserAccount);
			    strcpy(iter->context->password, smpd_process.UserPassword);
			    strcpy(iter->context->cred_request, "yes");
			    iter->context->read_state = SMPD_IDLE;
			    iter->context->write_state = SMPD_WRITING_CRED_ACK_YES;
			    result = MPIDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			    ret_val = result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
			}
			else
			{
			    smpd_err_printf("invalid cred_request result returned, no account and password specified: '%s'\n", context->read_cmd.cmd);
			    ret_val = SMPD_FAIL;
			}
		    }
		    else
		    {
			strcpy(iter->context->cred_request, "no");
			iter->context->read_state = SMPD_IDLE;
			iter->context->write_state = SMPD_WRITING_CRED_ACK_NO;
			result = MPIDU_Sock_post_write(iter->context->sock, iter->context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
			ret_val = result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
		    }
		}
		else if (strcmp(iter->cmd_str, "exit_on_done") == 0)
		{
		    ret_val = SMPD_DBS_RETURN;
		    /*
		    if (strcmp(str, SMPD_SUCCESS_STR) == 0)
		    {
			ret_val = SMPD_SUCCESS;
		    }
		    else
		    {
			smpd_err_printf("exit_on_done failed: %s\n", str);
			ret_val = SMPD_ABORT;
		    }
		    */
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
	    smpd_exit_fn("smpd_handle_result");
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
    smpd_exit_fn("smpd_handle_result");
    return SMPD_FAIL;
}

static int get_name_key_value(char *str, char *name, char *key, char *value)
{
    if (str == NULL)
	return SMPD_FAIL;

    if (name != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "name", name, SMPD_MAX_DBS_NAME_LEN) != MPIU_STR_SUCCESS)
	{
	    return SMPD_FAIL;
	}
    }
    if (key != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "key", key, SMPD_MAX_DBS_KEY_LEN) != MPIU_STR_SUCCESS)
	{
	    return SMPD_FAIL;
	}
    }
    if (value != NULL)
    {
	if (MPIU_Str_get_string_arg(str, "value", value, SMPD_MAX_DBS_VALUE_LEN) != MPIU_STR_SUCCESS)
	{
	    return SMPD_FAIL;
	}
    }
    return SMPD_SUCCESS;
}

int smpd_handle_dbs_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char name[SMPD_MAX_DBS_NAME_LEN+1] = "";
    char key[SMPD_MAX_DBS_KEY_LEN+1] = "";
    char value[SMPD_MAX_DBS_VALUE_LEN+1] = "";
    char ctx_key[100];
    char *result_str;

    smpd_enter_fn("smpd_handle_dbs_command");

    cmd = &context->read_cmd;

    /*
    printf("handling dbs command on %s context, sock %d.\n", smpd_get_context_str(context), MPIDU_Sock_get_sock_id(context->sock));
    fflush(stdout);
    */

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the db command: '%s'\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }

    /* check to make sure this node is running the dbs */
    if (!smpd_process.have_dbs)
    {
	/*
	printf("havd_dbs is false for this process %s context.\n", smpd_get_context_str(context));
	fflush(stdout);
	*/

	/* create a failure reply because this node does not have an initialized database */
	smpd_dbg_printf("sending a failure reply because this node does not have an initialized database.\n");
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - smpd does not have an initialized database.");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	    smpd_exit_fn("smpd_handle_dbs_command");
	    return SMPD_FAIL;
	}
	smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
	result = smpd_post_write_command(context, temp_cmd);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	    smpd_exit_fn("smpd_handle_dbs_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_SUCCESS;
    }

    /* do the dbs request */
    if (strcmp(cmd->cmd_str, "dbput") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, value) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_put(name, key, value) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbget") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_get(name, key, value) == SMPD_SUCCESS)
	{
	    result = smpd_add_command_arg(temp_cmd, "value", value);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the get value('%s') to the result command.\n", value);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbcreate") == 0)
    {
	if (MPIU_Str_get_string_arg(cmd->cmd, "name", name, SMPD_MAX_DBS_NAME_LEN) == MPIU_STR_SUCCESS)
	{
	    result = smpd_dbs_create_name_in(name);
	}
	else
	{
	    result = smpd_dbs_create(name);
	}
	if (result == SMPD_SUCCESS)
	{
	    result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbcreate name('%s') to the result command.\n", name);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbdestroy") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_destroy(name) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else if (strcmp(cmd->cmd_str, "dbfirst") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_first(name, key, value) == SMPD_SUCCESS)
	{
	    if (*key == '\0')
	    {
		/* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
		result = smpd_add_command_arg(temp_cmd, "key", DBS_END_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst key('%s') to the result command.\n", key);
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "key", key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst key('%s') to the result command.\n", key);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_arg(temp_cmd, "value", value);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbfirst value('%s') to the result command.\n", value);
		    return SMPD_FAIL;
		}
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbnext") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, NULL, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_next(name, key, value) == SMPD_SUCCESS)
	{
	    if (*key == '\0')
	    {
		/* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
		result = smpd_add_command_arg(temp_cmd, "key", DBS_END_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbndext key('%s') to the result command.\n", key);
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "key", key);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbnext key('%s') to the result command.\n", key);
		    return SMPD_FAIL;
		}
		result = smpd_add_command_arg(temp_cmd, "value", value);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the dbnext value('%s') to the result command.\n", value);
		    return SMPD_FAIL;
		}
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbfirstdb") == 0)
    {
	if (smpd_dbs_firstdb(name) == SMPD_SUCCESS)
	{
	    /* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
	    if (*name == '\0')
		result = smpd_add_command_arg(temp_cmd, "name", DBS_END_STR);
	    else
		result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbfirstdb name('%s') to the result command.\n", name);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbnextdb") == 0)
    {
	if (smpd_dbs_nextdb(name) == SMPD_SUCCESS)
	{
	    /* this can be changed to a special end_key if we don't want DBS_END_STR to be a reserved key */
	    if (*name == '\0')
		result = smpd_add_command_arg(temp_cmd, "name", DBS_END_STR);
	    else
		result = smpd_add_command_arg(temp_cmd, "name", name);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the dbnextdb name('%s') to the result command.\n", name);
		return SMPD_FAIL;
	    }
	    result_str = DBS_SUCCESS_STR;
	}
	else
	{
	    result_str = DBS_FAIL_STR;
	}
    }
    else if (strcmp(cmd->cmd_str, "dbdelete") == 0)
    {
	if (get_name_key_value(cmd->cmd, name, key, NULL) != SMPD_SUCCESS)
	    goto invalid_dbs_command;
	if (smpd_dbs_delete(name, key) == SMPD_SUCCESS)
	    result_str = DBS_SUCCESS_STR;
	else
	    result_str = DBS_FAIL_STR;
    }
    else
    {
	smpd_err_printf("unknown dbs command '%s', replying with failure.\n", cmd->cmd_str);
	goto invalid_dbs_command;
    }

    /* send the reply */
    smpd_dbg_printf("sending reply to dbs command '%s'.\n", cmd->cmd);
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_dbs_command");
    return SMPD_SUCCESS;

invalid_dbs_command:

    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR" - invalid dbs command.");
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for dbs command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("sending result command to %s context: \"%s\"\n", smpd_get_context_str(context), temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context: cmd '%s', dbs cmd '%s'", temp_cmd->cmd, cmd->cmd);
	smpd_exit_fn("smpd_handle_dbs_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_handle_dbs_command");
    return SMPD_SUCCESS;
}

int smpd_handle_launch_command(smpd_context_t *context)
{
    int result;
    int iproc;
    smpd_command_t *cmd, *temp_cmd;
    smpd_process_t *process;

    smpd_enter_fn("handle_launch_command");

    cmd = &context->read_cmd;

    /* parse the command */
    if (MPIU_Str_get_int_arg(cmd->cmd, "i", &iproc) != MPIU_STR_SUCCESS)
	iproc = 0;
    result = smpd_create_process_struct(iproc, &process);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a process structure.\n");
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }

    if (MPIU_Str_get_string_arg(cmd->cmd, "c", process->exe, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("launch command received with no executable: '%s'\n", cmd->cmd);
	smpd_exit_fn("handle_launch_command");
	return SMPD_FAIL;
    }
    MPIU_Str_get_string_arg(cmd->cmd, "e", process->env, SMPD_MAX_ENV_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "d", process->dir, SMPD_MAX_DIR_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "p", process->path, SMPD_MAX_PATH_LENGTH);
    MPIU_Str_get_string_arg(cmd->cmd, "k", process->kvs_name, SMPD_MAX_DBS_NAME_LEN);
    MPIU_Str_get_string_arg(cmd->cmd, "q", process->clique, SMPD_MAX_CLIQUE_LENGTH);
    MPIU_Str_get_int_arg(cmd->cmd, "n", &process->nproc);
    /* parse the -m drive mapping options */

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
	if (process->err_msg[0] != '\0')
	{
	    result = smpd_add_command_arg(temp_cmd, "error", process->err_msg);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the error field to the result command in response to launch command: '%s'\n", cmd->cmd);
		smpd_free_process_struct(process);
		smpd_exit_fn("handle_launch_command");
		return SMPD_FAIL;
	    }
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

int smpd_handle_close_command(smpd_context_t *context)
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

int smpd_handle_closed_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("handle_closed_command");

    cmd = &context->read_cmd;

    if (context == smpd_process.left_context)
    {
	smpd_dbg_printf("closed command received from left child, closing sock.\n");
	smpd_dbg_printf("MPIDU_Sock_post_close(%d)\n", MPIDU_Sock_get_sock_id(smpd_process.left_context->sock));
	smpd_process.left_context->state = SMPD_CLOSING;
	MPIDU_Sock_post_close(smpd_process.left_context->sock);
	if (smpd_process.right_context)
	{
	    smpd_exit_fn("handle_closed_command");
	    return SMPD_CLOSE;
	}
    }
    else if (context == smpd_process.right_context)
    {
	smpd_dbg_printf("closed command received from right child, closing sock.\n");
	smpd_dbg_printf("MPIDU_Sock_post_close(%d)\n", MPIDU_Sock_get_sock_id(smpd_process.right_context->sock));
	smpd_process.right_context->state = SMPD_CLOSING;
	MPIDU_Sock_post_close(smpd_process.right_context->sock);
	if (smpd_process.left_context)
	{
	    smpd_exit_fn("handle_closed_command");
	    return SMPD_CLOSE;
	}
    }
    else if (context == smpd_process.parent_context)
    {
	smpd_dbg_printf("closed command received from parent, closing sock.\n");
	smpd_dbg_printf("MPIDU_Sock_post_close(%d)\n", MPIDU_Sock_get_sock_id(smpd_process.parent_context->sock));
	smpd_process.parent_context->state = SMPD_CLOSING;
	MPIDU_Sock_post_close(smpd_process.parent_context->sock);
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

int smpd_handle_closed_request_command(smpd_context_t *context)
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

int smpd_handle_connect_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    smpd_context_t *dest;
    MPIDU_Sock_set_t dest_set;
    MPIDU_Sock_t dest_sock;
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
    if (MPIU_Str_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("connect command does not have a target host argument, discarding: \"%s\"\n", cmd->cmd);
	/* return failure result */
	smpd_exit_fn("handle_connect_command");
	return SMPD_SUCCESS;
    }
    if (MPIU_Str_get_int_arg(cmd->cmd, "id", &dest_id) != MPIU_STR_SUCCESS)
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
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, context->set, MPIDU_SOCK_INVALID_SOCK, dest_id, &dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a new context.\n");
	smpd_exit_fn("handle_connect_command");
	return SMPD_FAIL;
    }

    dest_set = context->set; /*smpd_process.set;*/

    /* start the connection logic here */
    result = MPIDU_Sock_post_connect(dest_set, dest, host, smpd_process.port, &dest_sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a connect to start the connect command,\nsock error: %s\n",
	    get_sock_error_string(result));
	result = smpd_post_abort_command("Unable to connect to '%s:%d',\nsock error: %s\n",
	    host, smpd_process.port, get_sock_error_string(result));
	smpd_exit_fn("handle_connect_command");
	/*return SMPD_FAIL;*/
	return result;
    }

    if (smpd_process.left_context == NULL)
    {
	smpd_dbg_printf("adding new left child context\n");
	smpd_init_context(dest, SMPD_CONTEXT_LEFT_CHILD, dest_set, dest_sock, dest_id);
	smpd_process.left_context = dest;
	MPIDU_Sock_set_user_ptr(dest_sock, dest);
    }
    else if (smpd_process.right_context == NULL)
    {
	smpd_dbg_printf("adding new right child context\n");
	smpd_init_context(dest, SMPD_CONTEXT_RIGHT_CHILD, dest_set, dest_sock, dest_id);
	smpd_process.right_context = dest;
	MPIDU_Sock_set_user_ptr(dest_sock, dest);
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

int smpd_handle_start_dbs_command(smpd_context_t *context)
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

int smpd_handle_print_command(smpd_context_t *context)
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

int smpd_handle_stat_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd, *cmd_iter;
    char param[100];
    char result_str[SMPD_MAX_CMD_LENGTH-100];
    smpd_context_t *iter;
    char *str;
    int len;

    smpd_enter_fn("smpd_handle_stat_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "param", param, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("stat command missing param parameter\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }

    if (strcmp(param, "context") == 0)
    {
	if (smpd_process.context_list == NULL)
	{
	    strcpy(result_str, "none");
	}
	else
	{
	    str = result_str;
	    len = SMPD_MAX_CMD_LENGTH-100;
	    iter = smpd_process.context_list;
	    while (iter)
	    {
		smpd_snprintf_update(&str, &len, "{\n");
		smpd_snprintf_update(&str, &len, " type               = %s\n", smpd_get_context_str(iter));
		smpd_snprintf_update(&str, &len, " id                 = %d\n", iter->id);
		smpd_snprintf_update(&str, &len, " state              = %s\n", smpd_get_state_string(iter->state));
		smpd_snprintf_update(&str, &len, " read_state         = %s\n", smpd_get_state_string(iter->read_state));
		smpd_snprintf_update(&str, &len, " read_cmd:\n");
		smpd_command_to_string(&str, &len, 2, &iter->read_cmd);
		smpd_snprintf_update(&str, &len, " write_state        = %s\n", smpd_get_state_string(iter->write_state));
		smpd_snprintf_update(&str, &len, " write_list         = %p\n", iter->write_list);
		cmd_iter = iter->write_list;
		while (cmd_iter)
		{
		    smpd_snprintf_update(&str, &len, " write_cmd:\n");
		    smpd_command_to_string(&str, &len, 2, cmd_iter);
		    cmd_iter = cmd_iter->next;
		}
		smpd_snprintf_update(&str, &len, " host               = %s\n", iter->host);
		smpd_snprintf_update(&str, &len, " rank               = %d\n", iter->rank);
		smpd_snprintf_update(&str, &len, " set                = %d\n", MPIDU_Sock_get_sock_set_id(iter->set));
		smpd_snprintf_update(&str, &len, " sock               = %d\n", MPIDU_Sock_get_sock_id(iter->sock));
		smpd_snprintf_update(&str, &len, " account            = %s\n", iter->account);
		smpd_snprintf_update(&str, &len, " password           = ***\n");
		smpd_snprintf_update(&str, &len, " connect_return_id  = %d\n", iter->connect_return_id);
		smpd_snprintf_update(&str, &len, " connect_return_tag = %d\n", iter->connect_return_tag);
		smpd_snprintf_update(&str, &len, " connect_to         = %p\n", iter->connect_to);
		smpd_snprintf_update(&str, &len, " cred_request       = %s\n", iter->cred_request);
		smpd_snprintf_update(&str, &len, " port_str           = %s\n", iter->port_str);
		smpd_snprintf_update(&str, &len, " pszChallengeResponse = %s\n", iter->pszChallengeResponse);
		smpd_snprintf_update(&str, &len, " pszCrypt           = %s\n", iter->pszCrypt);
		smpd_snprintf_update(&str, &len, " pwd_request        = %s\n", iter->pwd_request);
		smpd_snprintf_update(&str, &len, " session            = %s\n", iter->session);
		smpd_snprintf_update(&str, &len, " session_header     = '%s'\n", iter->session_header);
		smpd_snprintf_update(&str, &len, " smpd_pwd           = %s\n", iter->smpd_pwd);
		smpd_snprintf_update(&str, &len, " wait               = %d\n", (int)(iter->wait));
		smpd_snprintf_update(&str, &len, " wait_list          = %p\n", iter->wait_list);
		smpd_snprintf_update(&str, &len, " process            = %p\n", iter->process);
		if (iter->process)
		{
		    smpd_process_to_string(&str, &len, 2, iter->process);
		}
		smpd_snprintf_update(&str, &len, " next               = %p\n", iter->next);
		smpd_snprintf_update(&str, &len, "}\n");
		iter = iter->next;
	    }
	}
    }
    else if (strcmp(param, "process") == 0)
    {
	strcpy(result_str, "none");
    }
    else
    {
	strcpy(result_str, "unknown");
    }

    /* create a result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for the context.\n");
	smpd_exit_fn("smpd_handle_stat_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a stat command.\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }
    /* add the result string */
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a stat command.\n");
	smpd_exit_fn("smpd_handle_stat_command");
	return SMPD_FAIL;
    }

    /* post the result command */
    smpd_dbg_printf("replying to stat command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_stat_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_stat_command");
    return result;
}

int smpd_handle_abort_command(smpd_context_t *context)
{
    char error_str[2048];

    smpd_enter_fn("smpd_handle_abort_command");

    if (MPIU_Str_get_string_arg(context->read_cmd.cmd, "error", error_str, 2048) == MPIU_STR_SUCCESS)
    {
	smpd_err_printf("abort: %s\n", error_str);
    }

    smpd_exit_fn("smpd_handle_abort_command");
    return SMPD_EXIT;
}

int smpd_handle_validate_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char fullaccount[SMPD_MAX_ACCOUNT_LENGTH]="", domain[SMPD_MAX_ACCOUNT_LENGTH];
    char account[SMPD_MAX_ACCOUNT_LENGTH]="", password[SMPD_MAX_PASSWORD_LENGTH]="";
    char result_str[100];
#ifdef HAVE_WINDOWS_H
    HANDLE hUser;
#endif

    smpd_enter_fn("smpd_handle_validate_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "account", fullaccount, SMPD_MAX_ACCOUNT_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("validate command missing account parameter\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "password", password, SMPD_MAX_PASSWORD_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("validate command missing password parameter\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }

    /* validate user */
#ifdef HAVE_WINDOWS_H
    smpd_parse_account_domain(fullaccount, account, domain);
    result = smpd_get_user_handle(account, domain, password, &hUser);
    if (result != SMPD_SUCCESS)
    {
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }
    if (hUser != INVALID_HANDLE_VALUE)
	CloseHandle(hUser);
#else
    strcpy(result_str, SMPD_FAIL_STR);
#endif

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a validate command.\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a validate command.\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a validate command.\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to validate command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_validate_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_validate_command");
    return result;
}

int smpd_handle_status_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char dynamic_hosts[SMPD_MAX_CMD_LENGTH - 100];

    smpd_enter_fn("smpd_handle_status_command");

    cmd = &context->read_cmd;

    result = smpd_get_smpd_data("dynamic_hosts", dynamic_hosts, SMPD_MAX_CMD_LENGTH - 100);
    if (result != SMPD_SUCCESS)
	strcpy(dynamic_hosts, "none");

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a status command.\n");
	smpd_exit_fn("smpd_handle_status_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a status command.\n");
	smpd_exit_fn("smpd_handle_status_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", dynamic_hosts);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the dynamic hosts result string to the result command for a status command.\n");
	smpd_exit_fn("smpd_handle_status_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to status command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_status_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_status_command");
    return result;
}

int smpd_handle_get_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn("smpd_handle_get_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("get command missing key parameter\n");
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
    }

    /* get key */
    result = smpd_get_smpd_data(key, value, SMPD_MAX_VALUE_LENGTH);
    if (result != SMPD_SUCCESS)
    {
	/*
	smpd_err_printf("unable to get %s\n", key);
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
	*/
	strcpy(result_str, SMPD_FAIL_STR);
    }
    else
    {
	strcpy(result_str, SMPD_SUCCESS_STR);
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a get %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a get %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
    }
    if (strcmp(result_str, SMPD_SUCCESS_STR) == 0)
    {
	/* add the value */
	result = smpd_add_command_arg(temp_cmd, "value", value);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the value to the result command for a get %s=%s command.\n", key, value);
	    smpd_exit_fn("smpd_handle_get_command");
	    return SMPD_FAIL;
	}
    }
    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a get %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to get %s command: \"%s\"\n", key, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_get_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_get_command");
    return result;
}

int smpd_handle_set_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];
    char value[SMPD_MAX_VALUE_LENGTH];

    smpd_enter_fn("smpd_handle_set_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing key parameter\n");
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "value", value, SMPD_MAX_VALUE_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing value parameter\n");
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }

    /* set key=value */
    result = smpd_set_smpd_data(key, value);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to set %s=%s\n", key, value);
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }
    strcpy(result_str, SMPD_SUCCESS_STR);

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a set %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a set %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a set %s=%s command.\n", key, value);
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to set %s=%s command: \"%s\"\n", key, value, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_set_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_set_command");
    return result;
}

int smpd_handle_delete_command(smpd_context_t *context)
{
    int result = SMPD_SUCCESS;
    smpd_command_t *cmd, *temp_cmd;
    char result_str[100];
    char key[SMPD_MAX_NAME_LENGTH];

    smpd_enter_fn("smpd_handle_delete_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "key", key, SMPD_MAX_NAME_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("set command missing key parameter\n");
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }

    /* delete key */
    result = smpd_delete_smpd_data(key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to delete smpd data %s\n", key);
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }
    strcpy(result_str, SMPD_SUCCESS_STR);

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a delete %s command.\n", key);
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a delete %s command.\n", key);
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "result", result_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a delete %s command.\n", key);
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to delete %s command: \"%s\"\n", key, temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_delete_command");
    return result;
}

int smpd_handle_cred_request_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char host[SMPD_MAX_HOST_LENGTH];

    smpd_enter_fn("smpd_handle_cred_request_command");

    cmd = &context->read_cmd;

    if (MPIU_Str_get_string_arg(cmd->cmd, "host", host, SMPD_MAX_HOST_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no host parameter in the cred_request command: '%s'\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_cred_request_command");
	return SMPD_FAIL;
    }

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a cred_request command.\n");
	smpd_exit_fn("smpd_handle_cred_request_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a cred_request command.\n");
	smpd_exit_fn("smpd_handle_cred_request_command");
	return SMPD_FAIL;
    }

#ifdef HAVE_WINDOWS_H
    if (smpd_process.UserAccount[0] == '\0')
    {
	if (smpd_process.logon || 
	    (!smpd_get_cached_password(smpd_process.UserAccount, smpd_process.UserPassword) &&
	    !smpd_read_password_from_registry(smpd_process.UserAccount, smpd_process.UserPassword)))
	{
	    if (smpd_process.credentials_prompt)
	    {
		fprintf(stderr, "User credentials needed to launch processes on %s:\n", host);
		smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
		smpd_cache_password(smpd_process.UserAccount, smpd_process.UserPassword);
		result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the account parameter to the result command.\n");
		    smpd_exit_fn("smpd_handle_cred_request_command");
		    return result;
		}
		result = smpd_add_command_arg(temp_cmd, "password", smpd_process.UserPassword);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the password parameter to the result command.\n");
		    smpd_exit_fn("smpd_handle_cred_request_command");
		    return result;
		}
		result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the result parameter to the result command.\n");
		    smpd_exit_fn("smpd_handle_cred_request_command");
		    return result;
		}
	    }
	    else
	    {
		result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the result parameter to the result command.\n");
		    smpd_exit_fn("smpd_handle_cred_request_command");
		    return result;
		}
	    }
	}
	else
	{
	    result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "password", smpd_process.UserPassword);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	}
    }
    else
    {
	result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the account parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "password", smpd_process.UserPassword);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the password parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
    }
#else
    if (smpd_process.UserAccount[0] == '\0')
    {
	if (smpd_process.credentials_prompt)
	{
	    fprintf(stderr, "User credentials needed to launch processes on %s:\n", host);
	    smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
	    result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "password", smpd_process.UserPassword);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	}
	else
	{
	    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the result parameter to the result command.\n");
		smpd_exit_fn("smpd_handle_cred_request_command");
		return result;
	    }
	}
    }
    else
    {
	result = smpd_add_command_arg(temp_cmd, "account", smpd_process.UserAccount);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the account parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "password", smpd_process.UserPassword);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the password parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
	result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result parameter to the result command.\n");
	    smpd_exit_fn("smpd_handle_cred_request_command");
	    return result;
	}
    }
#endif
    /* send result back */
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(context));
	smpd_exit_fn("smpd_handle_delete_command");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_handle_cred_request_command");
    return result;
}

int smpd_handle_exit_on_done_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("smpd_handle_exit_on_done_command");

    cmd = &context->read_cmd;
    smpd_process.exit_on_done = SMPD_TRUE;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a exit_on_done command.\n");
	smpd_exit_fn("smpd_handle_exit_on_done_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a exit_on_done command.\n");
	smpd_exit_fn("smpd_handle_exit_on_done_command");
	return SMPD_FAIL;
    }

    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_SUCCESS_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a exit_on_done command.\n");
	smpd_exit_fn("smpd_handle_exit_on_done_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to exit_on_done command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_exit_on_done_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_exit_on_done_command");
    return result;
}

int smpd_handle_spawn_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    int ncmds, *maxprocs, *nkeyvals, npreput, i, j;
    smpd_launch_node_t node;
    char key[100], val[1024];
    char *iter1, *iter2;
    char maxprocs_str[1024], nkeyvals_str[1024], keyvals_str[1024];
    smpd_launch_node_t *launch_list, *launch_iter, *launch_temp;
    PMI_keyval_t *info;
    char key_temp[SMPD_MAX_NAME_LENGTH], val_temp[SMPD_MAX_VALUE_LENGTH];
    int cur_iproc;

    smpd_enter_fn("smpd_handle_spawn_command");

    cmd = &context->read_cmd;
    smpd_process.exit_on_done = SMPD_TRUE;

    /* prepare the result command */
    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a spawn command.\n");
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a spawn command.\n");
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the spawn command: '%s'\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }

    /* parse the spawn command */
    if (MPIU_Str_get_int_arg(cmd->cmd, "ncmds", &ncmds) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the ncmds parameter from the spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    printf("ncmds = %d\n", ncmds);fflush(stdout);
    if (MPIU_Str_get_string_arg(cmd->cmd, "maxprocs", maxprocs_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the maxrpocs parameter from the spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "nkeyvals", nkeyvals_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the nkeyvals parameter from the spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    maxprocs = (int*)malloc(ncmds * sizeof(int));
    nkeyvals = (int*)malloc(ncmds * sizeof(int));
    iter1 = maxprocs_str;
    iter2 = nkeyvals_str;
    for (i=0; i<ncmds; i++)
    {
	result = MPIU_Str_get_string(&iter1, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the maxprocs parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    smpd_exit_fn("smpd_handle_spawn_command");
	    return SMPD_FAIL;
	}
	maxprocs[i] = atoi(key);
	printf("maxprocs[%d] = %d\n", i, maxprocs[i]);fflush(stdout);
	result = MPIU_Str_get_string(&iter2, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the nkeyvals parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    smpd_exit_fn("smpd_handle_spawn_command");
	    return SMPD_FAIL;
	}
	nkeyvals[i] = atoi(key);
	printf("nkeyvals[%d] = %d\n", i, nkeyvals[i]);fflush(stdout);
    }
    info = NULL;
    launch_list = NULL;
    launch_iter = NULL;
    cur_iproc = 0;
    for (i=0; i<ncmds; i++)
    {
	if (info != NULL)
	{
	    /* free the last round of infos */
	    for (j=0; j<nkeyvals[i-1]; j++)
	    {
		free(info[j].key);
		free(info[j].val);
	    }
	    free(info);
	}
	/* allocate some new infos */
	if (nkeyvals[i] > 0)
	{
	    info = (PMI_keyval_t*)malloc(nkeyvals[i] * sizeof(PMI_keyval_t));
	    if (info == NULL)
	    {
		smpd_err_printf("unable to allocate memory for the info keyvals (cmd %d, num_infos %d).\n", i, nkeyvals[i]);
		smpd_exit_fn("smpd_handle_spawn_command");
		return SMPD_FAIL;
	    }
	}
	else
	{
	    info = NULL;
	}
	/* parse the keyvals into infos */
	sprintf(key, "keyvals%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, keyvals_str, 1024) == MPIU_STR_SUCCESS)
	{
	    printf("%s = '%s'\n", key, keyvals_str);fflush(stdout);
	    for (j=0; j<nkeyvals[i]; j++)
	    {
		sprintf(key, "%d", j);
		if (MPIU_Str_get_string_arg(keyvals_str, key, val, 1024) != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to get the %sth key from the keyval string '%s'.\n", key, keyvals_str);
		    smpd_exit_fn("smpd_handle_spawn_command");
		    return SMPD_FAIL;
		}
		printf("key %d = %s\n", j, val);fflush(stdout);
		key_temp[0] = '\0';
		val_temp[0] = '\0';
		iter1 = val;
		result = MPIU_Str_get_string(&iter1, key_temp, SMPD_MAX_NAME_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    smpd_exit_fn("smpd_handle_spawn_command");
		    return SMPD_FAIL;
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH); /* eat the '=' character */
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    smpd_exit_fn("smpd_handle_spawn_command");
		    return SMPD_FAIL;
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    smpd_exit_fn("smpd_handle_spawn_command");
		    return SMPD_FAIL;
		}
		info[j].key = strdup(key_temp);
		info[j].val = strdup(val_temp);
	    }
	}
	/* get the current command */
	sprintf(key, "cmd%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.exe, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    smpd_exit_fn("smpd_handle_spawn_command");
	    return SMPD_FAIL;
	}
	printf("%s = %s\n", key, node.exe);fflush(stdout);
	sprintf(key, "argv%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.args, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    smpd_exit_fn("smpd_handle_spawn_command");
	    return SMPD_FAIL;
	}
	printf("%s = %s\n", key, node.args);fflush(stdout);
	/* create launch nodes for the current command */
	for (j=0; j<maxprocs[i]; j++)
	{
	    if (launch_list == NULL)
	    {
		launch_list = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
		launch_iter = launch_list;
		if (launch_iter)
		{
		    launch_iter->prev = NULL;
		}
	    }
	    else
	    {
		launch_iter->next = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
		if (launch_iter->next)
		{
		    launch_iter->next->prev = launch_iter;
		    launch_iter = launch_iter->next;
		}
		else
		{
		    launch_iter = NULL;
		}
	    }
	    if (launch_iter == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure for the %dth command.\n", cur_iproc);
		smpd_exit_fn("smpd_handle_spawn_command");
		return SMPD_FAIL;
	    }
	    launch_iter->iproc = cur_iproc++;
	    launch_iter->args[0] = '\0';
	    launch_iter->clique[0] = '\0';
	    launch_iter->dir[0] = '\0';
	    launch_iter->env_data[0] = '\0';
	    launch_iter->env = launch_iter->env_data;
	    launch_iter->exe[0] = '\0';
	    launch_iter->host_id = 0;
	    launch_iter->map_list = NULL;
	    launch_iter->path[0] = '\0';
	    launch_iter->next = NULL;

	    strcpy(launch_iter->exe, node.exe);
	    strcpy(launch_iter->args, node.args);

	    /* interpret the infos for this command */
	}
    }

    /* Get the keyval pairs to be put in the process group keyval space before the processes are launched. */
    if (MPIU_Str_get_int_arg(cmd->cmd, "npreput", &npreput) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the npreput parameter from the spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }
    printf("npreput = %d\n", npreput);fflush(stdout);
    if (MPIU_Str_get_string_arg(cmd->cmd, "preput", keyvals_str, 1024) == MPIU_STR_SUCCESS)
    {
	printf("preput = '%s'\n", keyvals_str);fflush(stdout);
	for (j=0; j<npreput; j++)
	{
	    sprintf(key, "%d", j);
	    if (MPIU_Str_get_string_arg(keyvals_str, key, val, 1024) != MPIU_STR_SUCCESS)
	    {
		smpd_err_printf("unable to get the %sth key from the preput keyval string '%s'.\n", key, keyvals_str);
		smpd_exit_fn("smpd_handle_spawn_command");
		return SMPD_FAIL;
	    }
	    printf("key %d = %s\n", j, val);fflush(stdout);
	}
    }
    free(maxprocs);
    free(nkeyvals);

    /* do the spawn stuff */
    launch_iter = launch_list;
    while (launch_iter)
    {
	printf("launch_node:\n");
	printf(" rank: %d\n", launch_iter->iproc);
	printf(" exe: %s\n", launch_iter->exe);
	printf(" args: %s\n", launch_iter->args);
	launch_temp = launch_iter;
	launch_iter = launch_iter->next;
	free(launch_temp);
    }
    fflush(stdout);

    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a spawn command.\n");
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to spawn command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn("smpd_handle_spawn_command");
	return SMPD_FAIL;
    }

    smpd_exit_fn("smpd_handle_spawn_command");
    return result;
}

#if 0
/* use this template to add new command handler functions */
int smpd_handle__command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;

    smpd_enter_fn("smpd_handle__command");

    cmd = &context->read_cmd;

    result = handle command code;

    smpd_exit_fn("smpd_handle__command");
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
	smpd_err_printf("invalid command received, unable to determine the destination: '%s'\n", cmd->cmd);
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
	result = smpd_handle_close_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "closed") == 0)
    {
	result = smpd_handle_closed_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "closed_request") == 0)
    {
	result = smpd_handle_closed_request_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "result") == 0)
    {
	result = smpd_handle_result(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "exit") == 0)
    {
	int exitcode, iproc;

	if (MPIU_Str_get_int_arg(cmd->cmd, "code", &exitcode) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("no exit code in exit command: '%s'\n", cmd->cmd);
	}
	if (MPIU_Str_get_int_arg(cmd->cmd, "rank", &iproc) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("no iproc in exit command: '%s'\n", cmd->cmd);
	}
	if (smpd_process.output_exit_codes)
	{
	    printf("process %d exited with exit code %d\n", iproc, exitcode);
	    fflush(stdout);
	}
	smpd_process.nproc_exited++;
	if (smpd_process.nproc == smpd_process.nproc_exited)
	/*
	smpd_process.nproc--;
	if (smpd_process.nproc == 0)
	*/
	{
	    smpd_dbg_printf("last process exited, returning SMPD_EXIT.\n");
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_EXIT;
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd->cmd_str, "abort") == 0)
    {
	result = smpd_handle_abort_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "stdin") == 0)
    {
	result = smpd_handle_stdin_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "stdout") == 0)
    {
	result = smpd_handle_stdout_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "stderr") == 0)
    {
	result = smpd_handle_stderr_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "launch") == 0)
    {
	result = smpd_handle_launch_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "connect") == 0)
    {
	result = smpd_handle_connect_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "print") == 0)
    {
	result = smpd_handle_print_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "start_dbs") == 0)
    {
	result = smpd_handle_start_dbs_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    /*else if (strncmp(cmd->cmd_str, "db", 2) == 0)*/
    else if ((cmd->cmd_str[0] == 'd') && (cmd->cmd_str[1] == 'b'))
    {
	/* handle database command */
	result = smpd_handle_dbs_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "barrier") == 0)
    {
	result = smpd_handle_barrier_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "cred_request") == 0)
    {
	result = smpd_handle_cred_request_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "down") == 0)
    {
	context->state = SMPD_EXITING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on sock %d,\nsock error: %s\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_EXITING;
    }
    else if (strcmp(cmd->cmd_str, "exit_on_done") == 0)
    {
	result = smpd_handle_exit_on_done_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else if (strcmp(cmd->cmd_str, "done") == 0)
    {
	if (context->type != SMPD_CONTEXT_PMI)
	{
	    smpd_err_printf("done command read on %s context.\n", smpd_get_context_str(context));
	}
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on sock %d,\nsock error: %s\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_handle_command");
	    return SMPD_FAIL;
	}
	if (smpd_process.exit_on_done)
	{
	    smpd_process.nproc_exited++;
	    /*printf("%d exited\n", smpd_process.nproc_exited);*/
	    if (smpd_process.nproc == smpd_process.nproc_exited)
	    {
		context->state = SMPD_EXITING;
		smpd_dbg_printf("last process exited, returning SMPD_EXIT.\n");
		/*printf("last process exited, returning SMPD_EXIT.\n");fflush(stdout);*/
		smpd_exit_fn("smpd_handle_command");
		return /*SMPD_EXIT*/ SMPD_EXITING;
	    }
	}
	smpd_exit_fn("smpd_handle_command");
	return SMPD_CLOSE;
    }
    else if (strcmp(cmd->cmd_str, "spawn") == 0)
    {
	result = smpd_handle_spawn_command(context);
	smpd_exit_fn("smpd_handle_command");
	return result;
    }
    else
    {
	/* handle root commands */
	if (smpd_process.root_smpd)
	{
	    if ( (strcmp(cmd->cmd_str, "shutdown") == 0) || (strcmp(cmd->cmd_str, "restart") == 0) )
	    {
		if (strcmp(cmd->cmd_str, "restart") == 0)
		    smpd_process.restart = SMPD_TRUE;
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
	    else if (strcmp(cmd->cmd_str, "validate") == 0)
	    {
		result = smpd_handle_validate_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
	    }
	    else if (strcmp(cmd->cmd_str, "status") == 0)
	    {
		result = smpd_handle_status_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
	    }
	    else if (strcmp(cmd->cmd_str, "stat") == 0)
	    {
		result = smpd_handle_stat_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
	    }
	    else if (strcmp(cmd->cmd_str, "get") == 0)
	    {
		result = smpd_handle_get_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
	    }
	    else if (strcmp(cmd->cmd_str, "set") == 0)
	    {
		result = smpd_handle_set_command(context);
		smpd_exit_fn("smpd_handle_command");
		return result;
	    }
	    else if (strcmp(cmd->cmd_str, "delete") == 0)
	    {
		result = smpd_handle_delete_command(context);
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
