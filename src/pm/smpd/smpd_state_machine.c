/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#include "crypt.h"

#ifdef HAVE_WINDOWS_H
void StdinThread(SOCKET hWrite)
{
    DWORD len;
    char str[SMPD_MAX_CMD_LENGTH];
    HANDLE h[2];
    int result;

    smpd_dbg_printf("StdinThread started.\n");
    h[0] = GetStdHandle(STD_INPUT_HANDLE);
    if (h[0] == NULL)
    {
	smpd_err_printf("Unable to get the stdin handle.\n");
	return;
    }
    h[1] = smpd_process.hCloseStdinThreadEvent;
    while (1)
    {
	/*smpd_dbg_printf("waiting for input from stdin\n");*/
	result = WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (result == WAIT_OBJECT_0)
	{
	    if (fgets(str, SMPD_MAX_CMD_LENGTH, stdin))
	    {
		len = (DWORD)strlen(str);
		smpd_dbg_printf("forwarding stdin: '%s'\n", str);
		if (send(hWrite, str, len, 0) == SOCKET_ERROR)
		{
		    smpd_err_printf("unable to forward stdin, WriteFile failed, error %d\n", GetLastError());
		    return;
		}
	    }
	    else
	    {
		/* fgets failed, what do I do? */
		shutdown(hWrite, SD_BOTH);
		closesocket(hWrite);
		smpd_dbg_printf("fgets failed, closing stdin reader thread.\n");
		return;
	    }
	}
	else if (result == WAIT_OBJECT_0 + 1)
	{
	    shutdown(hWrite, SD_BOTH);
	    closesocket(hWrite);
	    smpd_dbg_printf("hCloseStdinThreadEvent signalled, closing stdin reader thread.\n");
	    return;
	}
	else
	{
	    smpd_err_printf("stdin wait failed, error %d\n", GetLastError());
	    return;
	}
    }
}
#endif

char * smpd_get_state_string(smpd_state_t state)
{
    static char unknown_str[100];

    switch (state)
    {
    case SMPD_IDLE:
	return "SMPD_IDLE";
    case SMPD_MPIEXEC_CONNECTING_TREE:
	return "SMPD_MPIEXEC_CONNECTING_TREE";
    case SMPD_MPIEXEC_CONNECTING_SMPD:
	return "SMPD_MPIEXEC_CONNECTING_SMPD";
    case SMPD_CONNECTING:
	return "SMPD_CONNECTING";
    case SMPD_READING_CHALLENGE_STRING:
	return "SMPD_READING_CHALLENGE_STRING";
    case SMPD_WRITING_CHALLENGE_RESPONSE:
	return "SMPD_WRITING_CHALLENGE_RESPONSE";
    case SMPD_READING_CONNECT_RESULT:
	return "SMPD_READING_CONNECT_RESULT";
    case SMPD_WRITING_CHALLENGE_STRING:
	return "SMPD_WRITING_CHALLENGE_STRING";
    case SMPD_READING_CHALLENGE_RESPONSE:
	return "SMPD_READING_CHALLENGE_RESPONSE";
    case SMPD_WRITING_CONNECT_RESULT:
	return "SMPD_WRITING_CONNECT_RESULT";
    case SMPD_READING_STDIN:
	return "SMPD_READING_STDIN";
    case SMPD_WRITING_DATA_TO_STDIN:
	return "SMPD_WRITING_DATA_TO_STDIN";
    case SMPD_READING_STDOUT:
	return "SMPD_READING_STDOUT";
    case SMPD_READING_STDERR:
	return "SMPD_READING_STDERR";
    case SMPD_READING_CMD_HEADER:
	return "SMPD_READING_CMD_HEADER";
    case SMPD_READING_CMD:
	return "SMPD_READING_CMD";
    case SMPD_WRITING_CMD:
	return "SMPD_WRITING_CMD";
    case SMPD_SMPD_LISTENING:
	return "SMPD_SMPD_LISTENING";
    case SMPD_MGR_LISTENING:
	return "SMPD_MGR_LISTENING";
    case SMPD_READING_SESSION_REQUEST:
	return "SMPD_READING_SESSION_REQUEST";
    case SMPD_WRITING_SMPD_SESSION_REQUEST:
	return "SMPD_WRITING_SMPD_SESSION_REQUEST";
    case SMPD_WRITING_PROCESS_SESSION_REQUEST:
	return "SMPD_WRITING_PROCESS_SESSION_REQUEST";
    case SMPD_WRITING_PMI_SESSION_REQUEST:
	return "SMPD_WRITING_PMI_SESSION_REQUEST";
    case SMPD_WRITING_PWD_REQUEST:
	return "SMPD_WRITING_PWD_REQUEST";
    case SMPD_WRITING_NO_PWD_REQUEST:
	return "SMPD_WRITING_NO_PWD_REQUEST";
    case SMPD_READING_PWD_REQUEST:
	return "SMPD_READING_PWD_REQUEST";
    case SMPD_READING_SMPD_PASSWORD:
	return "SMPD_READING_SMPD_PASSWORD";
    case SMPD_WRITING_CRED_REQUEST:
	return "SMPD_WRITING_CRED_REQUEST";
    case SMPD_READING_CRED_ACK:
	return "SMPD_READING_CRED_ACK";
    case SMPD_WRITING_CRED_ACK_YES:
	return "SMPD_WRITING_CRED_ACK_YES";
    case SMPD_WRITING_CRED_ACK_NO:
	return "SMPD_WRITING_CRED_ACK_NO";
    case SMPD_READING_ACCOUNT:
	return "SMPD_READING_ACCOUNT";
    case SMPD_READING_PASSWORD:
	return "SMPD_READING_PASSWORD";
    case SMPD_WRITING_ACCOUNT:
	return "SMPD_WRITING_ACCOUNT";
    case SMPD_WRITING_PASSWORD:
	return "SMPD_WRITING_PASSWORD";
    case SMPD_WRITING_NO_CRED_REQUEST:
	return "SMPD_WRITING_NO_CRED_REQUEST";
    case SMPD_READING_CRED_REQUEST:
	return "SMPD_READING_CRED_REQUEST";
    case SMPD_WRITING_RECONNECT_REQUEST:
	return "SMPD_WRITING_RECONNECT_REQUEST";
    case SMPD_WRITING_NO_RECONNECT_REQUEST:
	return "SMPD_WRITING_NO_RECONNECT_REQUEST";
    case SMPD_READING_RECONNECT_REQUEST:
	return "SMPD_READING_RECONNECT_REQUEST";
    case SMPD_READING_SESSION_HEADER:
	return "SMPD_READING_SESSION_HEADER";
    case SMPD_WRITING_SESSION_HEADER:
	return "SMPD_WRITING_SESSION_HEADER";
    case SMPD_READING_SMPD_RESULT:
	return "SMPD_READING_SMPD_RESULT";
    case SMPD_READING_PROCESS_RESULT:
	return "SMPD_READING_PROCESS_RESULT";
    case SMPD_WRITING_SESSION_ACCEPT:
	return "SMPD_WRITING_SESSION_ACCEPT";
    case SMPD_WRITING_SESSION_REJECT:
	return "SMPD_WRITING_SESSION_REJECT";
    case SMPD_WRITING_PROCESS_SESSION_ACCEPT:
	return "SMPD_WRITING_PROCESS_SESSION_REJECT";
    case SMPD_WRITING_PROCESS_SESSION_REJECT:
	return "SMPD_WRITING_PROCESS_SESSION_REJECT";
    case SMPD_RECONNECTING:
	return "SMPD_RECONNECTING";
    case SMPD_EXITING:
	return "SMPD_EXITING";
    case SMPD_CLOSING:
	return "SMPD_CLOSING";
    case SMPD_WRITING_SMPD_PASSWORD:
	return "SMPD_WRITING_SMPD_PASSWORD";
    }
    sprintf(unknown_str, "unknown state %d", state);
    return unknown_str;
}

int smpd_state_reading_challenge_string(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];
    char *crypted;

    smpd_enter_fn("smpd_state_reading_challenge_string");

    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the challenge string, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_challenge_string");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read challenge string: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;
    strcpy(phrase, smpd_process.passphrase);
    /* crypt the passphrase + the challenge */
    if (strlen(phrase) + strlen(context->pszChallengeResponse) > SMPD_PASSPHRASE_MAX_LENGTH)
    {
	smpd_err_printf("smpd_client_authenticate: unable to process passphrase - too long.\n");
	smpd_exit_fn("smpd_state_reading_challenge_string");
	return SMPD_FAIL;
    }
    strcat(phrase, context->pszChallengeResponse);

    /*smpd_dbg_printf("crypting: %s\n", phrase);*/
    crypted = crypt(phrase, SMPD_SALT_VALUE);
    strcpy(context->pszChallengeResponse, crypted);

    /* write the response */
    /*smpd_dbg_printf("writing response: %s\n", pszStr);*/
    context->write_state = SMPD_WRITING_CHALLENGE_RESPONSE;
    result = MPIDU_Sock_post_write(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the encrypted response string,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_challenge_string");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_challenge_string");
    return SMPD_SUCCESS;
}

int smpd_state_writing_challenge_response(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_challenge_response");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the challenge response, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_challenge_response");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote challenge response: '%s'\n", context->pszChallengeResponse);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CONNECT_RESULT;
    result = MPIDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the connect response,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_challenge_response");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_challenge_response");
    return SMPD_SUCCESS;
}

int smpd_state_reading_connect_result(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_connect_result");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the connect result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_connect_result");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read connect result: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->pszChallengeResponse, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	/* FIXME */
	/* the close operation needs to know that the state machine needs to exit */
	/* How is this going to be done? Where does the state go since context->state is taken? */

	/* rejected connection, close */
	smpd_dbg_printf("connection rejected, server returned - %s\n", context->pszChallengeResponse);
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to close sock, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_connect_result");
	    return SMPD_FAIL;
	}

	/* connection failed, abort? */
	/* when does a forming context get assinged it's global place?  At creation?  At connection? */
	if (smpd_process.left_context == smpd_process.left_context)
	    smpd_process.left_context = NULL;
	if (smpd_process.do_console && smpd_process.console_host[0] != '\0')
	    result = smpd_post_abort_command("1 unable to connect to %s", smpd_process.console_host);
	else if (context->connect_to && context->connect_to->host[0] != '\0')
	    result = smpd_post_abort_command("2 unable to connect to %s", context->connect_to->host);
	else
	{
	    if (context->host[0] != '\0')
		result = smpd_post_abort_command("3 unable to connect to %s", context->host);
	    else
		result = smpd_post_abort_command("connection to smpd rejected");
	}
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_state_reading_connect_result");
	    return SMPD_FAIL;
	}
    }
    else
    {
	context->write_state = SMPD_WRITING_PROCESS_SESSION_REQUEST;
	switch (context->state)
	{
	case SMPD_MPIEXEC_CONNECTING_TREE:
	case SMPD_CONNECTING:
	    strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    break;
	case SMPD_MPIEXEC_CONNECTING_SMPD:
	    if (smpd_process.use_process_session)
		strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    else
	    {
		strcpy(context->session, SMPD_SMPD_SESSION_STR);
		context->write_state = SMPD_WRITING_SMPD_SESSION_REQUEST;
	    }
	    break;
	case SMPD_CONNECTING_PMI:
	    context->write_state = SMPD_WRITING_PMI_SESSION_REQUEST;
	    strcpy(context->session, SMPD_PMI_SESSION_STR);
	    break;
	default:
	    strcpy(context->session, SMPD_PROCESS_SESSION_STR);
	    break;
	}
	result = MPIDU_Sock_post_write(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session request '%s',\nsock error: %s\n",
		context->session, get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_connect_result");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_reading_connect_result");
    return SMPD_SUCCESS;
}

int smpd_state_writing_challenge_string(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_challenge_string");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the challenge string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	result = result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	smpd_exit_fn("smpd_state_writing_challenge_string");
	return result;
    }
    smpd_dbg_printf("wrote challenge string: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_READING_CHALLENGE_RESPONSE;
    context->write_state = SMPD_IDLE;
    result = MPIDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("posting a read of the challenge response string failed,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	result = result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	smpd_exit_fn("smpd_state_writing_challenge_string");
	return result;
    }
    smpd_exit_fn("smpd_state_writing_challenge_string");
    return SMPD_SUCCESS;
}

int smpd_state_reading_challenge_response(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_challenge_response");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the challenge response, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_challenge_response");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read challenge response: '%s'\n", context->pszChallengeResponse);
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_CONNECT_RESULT;
    if (strcmp(context->pszChallengeResponse, context->pszCrypt) == 0)
	strcpy(context->pszChallengeResponse, SMPD_AUTHENTICATION_ACCEPTED_STR);
    else
	strcpy(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_STR);
    result = MPIDU_Sock_post_write(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the connect result '%s',\nsock error: %s\n",
	    context->pszChallengeResponse, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_challenge_response");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_challenge_response");
    return SMPD_SUCCESS;
}

int smpd_state_writing_connect_result(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_connect_result");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the connect result, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_connect_result");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote connect result: '%s'\n", context->pszChallengeResponse);
    context->write_state = SMPD_IDLE;
    if (strcmp(context->pszChallengeResponse, SMPD_AUTHENTICATION_REJECTED_STR) == 0)
    {
	context->state = SMPD_CLOSING;
	smpd_dbg_printf("connection reject string written, closing sock.\n");
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_post_close failed, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_connect_result");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_connect_result");
	return SMPD_SUCCESS;
    }
    context->read_state = SMPD_READING_SESSION_REQUEST;
    result = MPIDU_Sock_post_read(context->sock, context->session, SMPD_SESSION_REQUEST_LEN, SMPD_SESSION_REQUEST_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n", get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_connect_result");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_connect_result");
    return SMPD_SUCCESS;
}

int smpd_state_reading_stdin(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    MPIDU_Sock_size_t num_read;
    char buffer[SMPD_MAX_CMD_LENGTH];
    int num_encoded;

    smpd_enter_fn("smpd_state_reading_stdin");
    if (event_ptr->error != MPI_SUCCESS)
    {
	/*smpd_err_printf("unable to read from stdin, %s.\n", get_sock_error_string(event_ptr->error));*/
	smpd_exit_fn("smpd_state_reading_stdin");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read from stdin\n");
    if (context->type == SMPD_CONTEXT_MPIEXEC_STDIN)
    {
	smpd_dbg_printf("read from %s\n", smpd_get_context_str(context));

	/* one byte read, attempt to read up to the buffer size */
	result = MPIDU_Sock_read(context->sock, &context->read_cmd.cmd[1], SMPD_STDIN_PACKET_SIZE-1, &num_read);
	if (result != MPI_SUCCESS)
	{
	    num_read = 0;
	    smpd_dbg_printf("MPIDU_Sock_read(%d) failed (%s), assuming %s is closed.\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
	}
	smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));
	smpd_encode_buffer(buffer, SMPD_MAX_CMD_LENGTH, context->read_cmd.cmd, num_read+1, &num_encoded);
	buffer[num_encoded*2] = '\0';
	/*smpd_dbg_printf("encoded %d characters: %d '%s'\n", num_encoded, strlen(buffer), buffer);*/

	/* create an stdin command */
	result = smpd_create_command("stdin", 0, 1 /* input always goes to node 1? */, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create an stdin command.\n");
	    smpd_exit_fn("smpd_state_reading_stdin");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "data", buffer);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the data to the stdin command.\n");
	    smpd_exit_fn("smpd_state_reading_stdin");
	    return SMPD_FAIL;
	}

	/* send the stdin command */
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the stdin command.\n");
	    smpd_exit_fn("smpd_state_reading_stdin");
	    return SMPD_FAIL;
	}
    }
    else
    {
	if (context->read_cmd.stdin_read_offset == SMPD_STDIN_PACKET_SIZE ||
	    context->read_cmd.cmd[context->read_cmd.stdin_read_offset] == '\n')
	{
	    if (context->read_cmd.cmd[context->read_cmd.stdin_read_offset] != '\n')
		smpd_err_printf("truncated command.\n");
	    context->read_cmd.cmd[context->read_cmd.stdin_read_offset] = '\0'; /* remove the \n character */
	    result = smpd_create_command("", -1, -1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a command structure for the stdin command.\n");
		smpd_exit_fn("smpd_state_reading_stdin");
		return SMPD_FAIL;
	    }
	    smpd_init_command(cmd_ptr);
	    strcpy(cmd_ptr->cmd, context->read_cmd.cmd);
	    if (MPIU_Str_get_int_arg(cmd_ptr->cmd, "src", &cmd_ptr->src) != MPIU_STR_SUCCESS)
	    {
		smpd_add_command_int_arg(cmd_ptr, "src", 0);
	    }
	    if (MPIU_Str_get_int_arg(cmd_ptr->cmd, "dest", &cmd_ptr->dest) != MPIU_STR_SUCCESS)
	    {
		smpd_add_command_int_arg(cmd_ptr, "dest", 1);
	    }
	    result = smpd_parse_command(cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("invalid command read from stdin, ignoring: \"%s\"\n", context->read_cmd.cmd);
	    }
	    else
	    {
		if (strcmp(cmd_ptr->cmd_str, "connect") == 0)
		{
		    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "tag", &cmd_ptr->tag) != MPIU_STR_SUCCESS)
		    {
			smpd_dbg_printf("adding tag %d to connect command.\n", smpd_process.cur_tag);
			smpd_add_command_int_arg(cmd_ptr, "tag", smpd_process.cur_tag);
			cmd_ptr->tag = smpd_process.cur_tag;
			smpd_process.cur_tag++;
		    }
		    cmd_ptr->wait = SMPD_TRUE;
		}
		if (strcmp(cmd_ptr->cmd_str, "set") == 0 || strcmp(cmd_ptr->cmd_str, "delete") == 0 ||
		    strcmp(cmd_ptr->cmd_str, "stat") == 0 || strcmp(cmd_ptr->cmd_str, "get") == 0)
		{
		    if (MPIU_Str_get_int_arg(context->read_cmd.cmd, "tag", &cmd_ptr->tag) != MPIU_STR_SUCCESS)
		    {
			smpd_dbg_printf("adding tag %d to %s command.\n", smpd_process.cur_tag, cmd_ptr->cmd_str);
			smpd_add_command_int_arg(cmd_ptr, "tag", smpd_process.cur_tag);
			cmd_ptr->tag = smpd_process.cur_tag;
			smpd_process.cur_tag++;
		    }
		    cmd_ptr->wait = SMPD_TRUE;
		}

		smpd_dbg_printf("command read from stdin, forwarding to left_child smpd\n");
		result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the command read from stdin: \"%s\"\n", cmd_ptr->cmd);
		    smpd_free_command(cmd_ptr);
		    smpd_exit_fn("smpd_state_reading_stdin");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("posted write of command: \"%s\"\n", cmd_ptr->cmd);
	    }
	    context->read_cmd.stdin_read_offset = 0;
	}
	else
	{
	    context->read_cmd.stdin_read_offset++;
	}
    }
    result = MPIDU_Sock_post_read(context->sock, &context->read_cmd.cmd[context->read_cmd.stdin_read_offset], 1, 1, NULL);
    if (result != MPI_SUCCESS)
    {
	/*
	if (result != SOCK_EOF)
	{
	    smpd_dbg_printf("MPIDU_Sock_post_read failed (%s), assuming %s is closed, calling sock_post_close(%d).\n",
		get_sock_error_string(result), smpd_get_context_str(context), sock_getid(context->sock));
	}
	*/
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_stdin");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_reading_stdin");
    return SMPD_SUCCESS;
}

int smpd_state_smpd_writing_data_to_stdin(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_stdin_write_node_t *node;

    smpd_enter_fn("smpd_state_smpd_writing_data_to_stdin");

    node = context->process->stdin_write_list;
    if (node == NULL)
    {
	smpd_err_printf("write completed to process stdin context with no write posted in the list.\n");
	smpd_exit_fn("smpd_state_smpd_writing_data_to_stdin");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("wrote %d bytes to stdin of rank %d\n", node->length, context->process->rank);
    free(node->buffer);
    free(node);

    context->process->stdin_write_list = context->process->stdin_write_list->next;
    if (context->process->stdin_write_list != NULL)
    {
	context->process->in->write_state = SMPD_WRITING_DATA_TO_STDIN;
	result = MPIDU_Sock_post_write(context->process->in->sock,
	    node->buffer, node->length, node->length, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of %d bytes to stdin for rank %d\n",
		node->length, context->process->rank);
	    smpd_exit_fn("smpd_state_smpd_writing_data_to_stdin");
	}
    }
    else
    {
	context->process->in->write_state = SMPD_IDLE;
    }

    smpd_exit_fn("smpd_state_smpd_writing_data_to_stdin");
    return SMPD_SUCCESS;
}

int smpd_state_reading_stdouterr(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;
    MPIDU_Sock_size_t num_read;
    char buffer[SMPD_MAX_CMD_LENGTH];
    int num_encoded;

    smpd_enter_fn("smpd_state_reading_stdouterr");
    if (event_ptr->error != MPI_SUCCESS)
    {
	if (MPIR_ERR_GET_CLASS(event_ptr->error) != MPIDU_SOCK_ERR_CONN_CLOSED)
	{
	    smpd_dbg_printf("reading failed(%s), assuming %s is closed.\n",
		get_sock_error_string(event_ptr->error), smpd_get_context_str(context));
	}

	/* Return an error an then handle_sock_op_read will post a close
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_stdouterr");
	    return SMPD_FAIL;
	}
	*/

	smpd_exit_fn("smpd_state_reading_stdouterr");
	return SMPD_FAIL;
    }

    smpd_dbg_printf("read from %s\n", smpd_get_context_str(context));

    /* one byte read, attempt to read up to the buffer size */
    result = MPIDU_Sock_read(context->sock, &context->read_cmd.cmd[1], (SMPD_MAX_STDOUT_LENGTH/2)-2, &num_read);
    if (result != MPI_SUCCESS)
    {
	num_read = 0;
	smpd_dbg_printf("MPIDU_Sock_read(%d) failed (%s), assuming %s is closed.\n",
	    MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result), smpd_get_context_str(context));
    }
    smpd_dbg_printf("%d bytes read from %s\n", num_read+1, smpd_get_context_str(context));
    smpd_encode_buffer(buffer, SMPD_MAX_CMD_LENGTH, context->read_cmd.cmd, num_read+1, &num_encoded);
    buffer[num_encoded*2] = '\0';
    /*smpd_dbg_printf("encoded %d characters: %d '%s'\n", num_encoded, strlen(buffer), buffer);*/

    /* create an output command */
    result = smpd_create_command(
	smpd_get_context_str(context),
	smpd_process.id, 0 /* output always goes to node 0? */, SMPD_FALSE, &cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create an output command.\n");
	smpd_exit_fn("smpd_state_reading_stdouterr");
	return SMPD_FAIL;
    }
    result = smpd_add_command_int_arg(cmd_ptr, "rank", context->rank);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the rank to the %s command.\n", smpd_get_context_str(context));
	smpd_exit_fn("smpd_state_reading_stdouterr");
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(cmd_ptr, "data", buffer);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the data to the %s command.\n", smpd_get_context_str(context));
	smpd_exit_fn("smpd_state_reading_stdouterr");
	return SMPD_FAIL;
    }

    /* send the stdout command */
    result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the %s command.\n", smpd_get_context_str(context));
	smpd_exit_fn("smpd_state_reading_stdouterr");
	return SMPD_FAIL;
    }

    /* post a read for the next byte of data */
    result = MPIDU_Sock_post_read(context->sock, &context->read_cmd.cmd, 1, 1, NULL);
    if (result != MPI_SUCCESS)
    {
	/*
	if (result != SOCK_EOF)
	{
	    smpd_dbg_printf("MPIDU_Sock_post_read failed (%s), assuming %s is closed, calling sock_post_close(%d).\n",
		get_sock_error_string(result), smpd_get_context_str(context), MPIDU_Sock_get_sock_id(context->sock));
	}
	*/
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_stdouterr");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_reading_stdouterr");
    return SMPD_SUCCESS;
}

int smpd_state_reading_cmd_header(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_cmd_header");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the cmd header on the %s context, %s.\n",
	    smpd_get_context_str(context), get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd_header");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command header\n");
    context->read_cmd.length = atoi(context->read_cmd.cmd_hdr_str);
    if (context->read_cmd.length < 1)
    {
	smpd_err_printf("unable to read the command, invalid command length: %d\n", context->read_cmd.length);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd_header");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("command header read, posting read for data: %d bytes\n", context->read_cmd.length);
    context->read_cmd.state = SMPD_CMD_READING_CMD;
    context->read_state = SMPD_READING_CMD;
    result = MPIDU_Sock_post_read(context->sock, context->read_cmd.cmd, context->read_cmd.length, context->read_cmd.length, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the command string,\nsock error: %s\n", get_sock_error_string(result));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd_header");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_cmd_header");
    return SMPD_SUCCESS;
}

int smpd_state_reading_cmd(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_state_reading_cmd");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the command, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command\n");
    result = smpd_parse_command(&context->read_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to parse the read command: \"%s\"\n", context->read_cmd.cmd);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read command: \"%s\"\n", context->read_cmd.cmd);
    context->read_cmd.state = SMPD_CMD_READY;
    result = smpd_handle_command(context);
    if (result == SMPD_SUCCESS)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		smpd_exit_fn("smpd_state_reading_cmd");
		return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}
    }
    else if (result == SMPD_CLOSE || result == SMPD_EXITING)
    {
	smpd_dbg_printf("not posting read for another command because %s returned\n",
	    result == SMPD_CLOSE ? "SMPD_CLOSING" : "SMPD_EXITING");
	smpd_exit_fn("smpd_state_reading_cmd");
	return SMPD_SUCCESS;
    }
    else if (result == SMPD_EXIT)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}

	/* The last process has exited, create a close command to tear down the tree */
	smpd_process.closing = SMPD_TRUE;
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}
    }
    else if (result == SMPD_CONNECTED)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}

	/* mark the node as connected */
	context->connect_to->connected = SMPD_TRUE;

	/* send the next connect command or start_dbs command */
	/* create a command to connect to the next host in the tree */
	context->connect_to = context->connect_to->next;
	if (context->connect_to)
	{
	    /* create a connect command to be sent to the parent */
	    result = smpd_create_command("connect", 0, context->connect_to->parent, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a connect command.\n");
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", context->connect_to->host);
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "id", context->connect_to->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", context->connect_to->host);
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the connect command.\n");
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }
	}
	else
	{
	    /* create the start_dbs command to be sent to the first host */
	    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a start_dbs command.\n");
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }

	    if (context->spawn_context)
	    {
		smpd_dbg_printf("spawn_context found, adding preput values to the start_dbs command.\n");
		result = smpd_add_command_int_arg(cmd_ptr, "npreput", context->spawn_context->npreput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn("smpd_state_writing_session_header");
		    return result;
		}

		result = smpd_add_command_arg(cmd_ptr, "preput", context->spawn_context->preput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn("smpd_state_writing_session_header");
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the start_dbs command.\n");
		smpd_exit_fn("smpd_state_reading_cmd");
		return result;
	    }
	}
    }
    else if (result == SMPD_DBS_RETURN)
    {
	/*
	printf("SMPD_DBS_RETURN returned, not posting read for the next command.\n");
	fflush(stdout);
	*/
	smpd_exit_fn("smpd_state_reading_cmd");
	return SMPD_DBS_RETURN;
    }
    else if (result == SMPD_ABORT)
    {
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the next command on %s context.\n", smpd_get_context_str(context));
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}
	result = smpd_post_abort_command("");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post an abort command.\n");
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("unable to handle the command: \"%s\"\n", context->read_cmd.cmd);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_cmd");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_cmd");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_cmd");
    return SMPD_SUCCESS;
}

int smpd_state_writing_cmd(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr, *cmd_iter;

    smpd_enter_fn("smpd_state_writing_cmd");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the command, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_cmd");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote command\n");
    if (context->write_list == NULL)
    {
	smpd_err_printf("data written on a context with no write command posted.\n");
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_cmd");
	return SMPD_FAIL;
    }
    context->write_state = SMPD_IDLE;
    cmd_ptr = context->write_list;
    context->write_list = context->write_list->next;
    smpd_dbg_printf("command written to %s: \"%s\"\n", smpd_get_context_str(context), cmd_ptr->cmd);
    if (strcmp(cmd_ptr->cmd_str, "closed") == 0)
    {
	smpd_dbg_printf("closed command written, posting close of the sock.\n");
	smpd_dbg_printf("MPIDU_Sock_post_close(%d)\n", MPIDU_Sock_get_sock_id(context->sock));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'closed' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return SMPD_FAIL;
	}
    }
    else if (strcmp(cmd_ptr->cmd_str, "down") == 0)
    {
	smpd_dbg_printf("down command written, posting a close of the %s context\n", smpd_get_context_str(context));
	if (smpd_process.restart)
	    context->state = SMPD_RESTARTING;
	else
	    context->state = SMPD_EXITING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'down' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn("smpd_state_writing_cmd");
	    smpd_exit(0);
	}
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_state_writing_cmd");
	return SMPD_SUCCESS;
    }
    else if (strcmp(cmd_ptr->cmd_str, "done") == 0)
    {
	smpd_dbg_printf("done command written, posting a close of the %s context\n", smpd_get_context_str(context));
	context->state = SMPD_DONE;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close of the sock after writing a 'done' command,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_free_command(cmd_ptr);
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return SMPD_FAIL;
	}
	smpd_free_command(cmd_ptr);
	smpd_exit_fn("smpd_state_writing_cmd");
	return SMPD_SUCCESS;
    }

    if (cmd_ptr->wait)
    {
	/* If this command expects a reply, move it to the wait list */
	smpd_dbg_printf("moving '%s' command to the wait_list.\n", cmd_ptr->cmd_str);
	if (context->wait_list)
	{
	    cmd_iter = context->wait_list;
	    while (cmd_iter->next)
		cmd_iter = cmd_iter->next;
	    cmd_iter->next = cmd_ptr;
	}
	else
	{
	    context->wait_list = cmd_ptr;
	}
	cmd_ptr->next = NULL;
    }
    else
    {
	/* otherwise free the command immediately. */
	result = smpd_free_command(cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to free the written command.\n");
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		smpd_exit_fn("smpd_state_writing_cmd");
		return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return SMPD_FAIL;
	}
    }

    cmd_ptr = context->write_list;
    if (cmd_ptr)
    {
	context->write_state = SMPD_WRITING_CMD;
	cmd_ptr->iov[0].MPID_IOV_BUF = cmd_ptr->cmd_hdr_str;
	cmd_ptr->iov[0].MPID_IOV_LEN = SMPD_CMD_HDR_LENGTH;
	cmd_ptr->iov[1].MPID_IOV_BUF = cmd_ptr->cmd;
	cmd_ptr->iov[1].MPID_IOV_LEN = cmd_ptr->length;
	smpd_dbg_printf("smpd_handle_written: posting write(%d bytes) for command: \"%s\"\n",
	    cmd_ptr->iov[0].MPID_IOV_LEN + cmd_ptr->iov[1].MPID_IOV_LEN, cmd_ptr->cmd);
	result = MPIDU_Sock_post_writev(context->sock, cmd_ptr->iov, 2, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write for the next command,\nsock error: %s\n", get_sock_error_string(result));
	    if (smpd_process.root_smpd)
	    {
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		smpd_exit_fn("smpd_state_writing_cmd");
		return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	    }
	    smpd_exit_fn("smpd_state_writing_cmd");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_writing_cmd");
    return SMPD_SUCCESS;
}

int smpd_state_smpd_listening(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr, MPIDU_Sock_set_t set)
{
    int result;
    MPIDU_Sock_t new_sock;
    smpd_context_t *new_context;
    char phrase[SMPD_PASSPHRASE_MAX_LENGTH];

    smpd_enter_fn("smpd_state_smpd_listening");
    result = MPIDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_state_smpd_listening");
	return SMPD_FAIL;
    }
    if (smpd_process.service_stop)
    {
	smpd_process.state = SMPD_EXITING;
	if (smpd_process.listener_context)
	{
	    smpd_process.listener_context->state = SMPD_EXITING;
	    smpd_dbg_printf("closing the listener (state = %s).\n", smpd_get_state_string(smpd_process.listener_context->state));
	    result = MPIDU_Sock_post_close(smpd_process.listener_context->sock);
	    smpd_process.listener_context = NULL;
	    if (result == MPI_SUCCESS)
	    {
		smpd_exit_fn("smpd_state_smpd_listening");
		return SMPD_SUCCESS;
	    }
	    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
		get_sock_error_string(result));
	}
	smpd_free_context(context);
#ifdef HAVE_WINDOWS_H
	SetEvent(smpd_process.hBombDiffuseEvent);
#endif
	smpd_exit_fn("smpd_state_smpd_listening");
	return SMPD_SUCCESS;
    }
    smpd_dbg_printf("authenticating new connection\n");
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, set, new_sock, -1, &new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the newly accepted sock.\n");
	smpd_exit_fn("smpd_state_smpd_listening");
	return SMPD_FAIL;
    }
    result = MPIDU_Sock_set_user_ptr(new_sock, new_context);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_smpd_listening");
	return SMPD_FAIL;
    }

    strcpy(phrase, smpd_process.passphrase);
    /* generate the challenge string and the encrypted result */
    if (smpd_gen_authentication_strings(phrase, new_context->pszChallengeResponse, new_context->pszCrypt) != SMPD_SUCCESS)
    {
	smpd_err_printf("failed to generate the authentication strings\n");
	smpd_exit_fn("smpd_state_smpd_listening");
	return SMPD_FAIL;
    }
    /*
    smpd_dbg_printf("gen_authentication_strings:\n passphrase - %s\n pszStr - %s\n pszCrypt - %s\n",
    phrase, pszStr, pszCrypt);
    */

    /* write the challenge string*/
    smpd_dbg_printf("posting a write of the challenge string: %s\n", new_context->pszChallengeResponse);
    new_context->write_state = SMPD_WRITING_CHALLENGE_STRING;
    result = MPIDU_Sock_post_write(new_sock, new_context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
    if (result != MPI_SUCCESS)
    {
	new_context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(new_sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("posting write of the challenge string failed\n");
	    smpd_exit_fn("smpd_state_smpd_listening");
	    return SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_smpd_listening");
    return SMPD_SUCCESS;
}

int smpd_state_mgr_listening(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr, MPIDU_Sock_set_t set)
{
    int result;
    MPIDU_Sock_t new_sock;
    smpd_context_t *new_context;

    smpd_enter_fn("smpd_state_mgr_listening");
    result = MPIDU_Sock_accept(context->sock, set, NULL, &new_sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("error accepting socket: %s\n", get_sock_error_string(result));
	smpd_exit_fn("smpd_state_mgr_listening");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("accepted re-connection\n");
    result = smpd_create_context(SMPD_CONTEXT_UNDETERMINED, set, new_sock, -1, &new_context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a context for the newly accepted sock.\n");
	smpd_exit_fn("smpd_state_mgr_listening");
	return SMPD_FAIL;
    }
    result = MPIDU_Sock_set_user_ptr(new_sock, new_context);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to set the user pointer on the newly accepted sock, error:\n%s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_mgr_listening");
	return SMPD_FAIL;
    }
    new_context->read_state = SMPD_READING_SESSION_HEADER;
    result = MPIDU_Sock_post_read(new_context->sock, new_context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_mgr_listening");
	return SMPD_FAIL;
    }
    /* close the listener */
    smpd_dbg_printf("closing the mgr listener.\n");
    result = MPIDU_Sock_post_close(context->sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a close on the listener sock after accepting the re-connection,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_mgr_listening");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_mgr_listening");
    return SMPD_SUCCESS;
}

int smpd_state_reading_session_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_session_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the session request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_session_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read session request: '%s'\n", context->session);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->session, SMPD_SMPD_SESSION_STR) == 0)
    {
	if (smpd_process.bPasswordProtect)
	{
	    context->write_state = SMPD_WRITING_PWD_REQUEST;
	    strcpy(context->pwd_request, SMPD_PWD_REQUEST);
	    context->write_state = SMPD_WRITING_PWD_REQUEST;
	}
	else
	{
	    context->write_state = SMPD_WRITING_NO_PWD_REQUEST;
	    strcpy(context->pwd_request, SMPD_NO_PWD_REQUEST);
	    context->write_state = SMPD_WRITING_NO_PWD_REQUEST;
	}
	result = MPIDU_Sock_post_write(context->sock, context->pwd_request, SMPD_MAX_PWD_REQUEST_LENGTH, SMPD_MAX_PWD_REQUEST_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the pwd request '%s',\nsock error: %s\n",
		context->pwd_request, get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_session_request");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else if (strcmp(context->session, SMPD_PROCESS_SESSION_STR) == 0)
    {
#ifdef HAVE_WINDOWS_H
	if (smpd_process.bService)
	{
	    context->write_state = SMPD_WRITING_CRED_REQUEST;
	    strcpy(context->cred_request, SMPD_CRED_REQUEST);
	}
	else
#endif
	{
	    context->write_state = SMPD_WRITING_NO_CRED_REQUEST;
	    strcpy(context->cred_request, SMPD_NO_CRED_REQUEST);
	}
	result = MPIDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the credential request string '%s',\nsock error: %s\n",
		context->cred_request, get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_session_request");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else if (strcmp(context->session, SMPD_PMI_SESSION_STR) == 0)
    {
	context->type = SMPD_CONTEXT_PMI;
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of a command after accepting a pmi connection.\n");
	    smpd_exit_fn("smpd_state_reading_session_request");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_err_printf("invalid session request: '%s'\n", context->session);
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_session_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_session_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_smpd_session_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_smpd_session_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_smpd_session_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote smpd session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_PWD_REQUEST;
    result = MPIDU_Sock_post_read(context->sock, context->pwd_request, SMPD_MAX_PWD_REQUEST_LENGTH, SMPD_MAX_PWD_REQUEST_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the pwd request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_smpd_session_request");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_smpd_session_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_process_session_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_process_session_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the process session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_process_session_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CRED_REQUEST;
    result = MPIDU_Sock_post_read(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the cred request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_process_session_request");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_process_session_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_pmi_session_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    smpd_enter_fn("smpd_state_writing_process_session_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the pmi session request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_process_session_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote pmi session request: '%s'\n", context->session);
    context->write_state = SMPD_IDLE;
    smpd_exit_fn("smpd_state_writing_process_session_request");
    return SMPD_DBS_RETURN;
}

int smpd_state_writing_pwd_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_pwd_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_pwd_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote pwd request: '%s'\n", context->pwd_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SMPD_PASSWORD;
    result = MPIDU_Sock_post_read(context->sock, context->password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the smpd password,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_pwd_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_pwd_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_no_pwd_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_no_pwd_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the no pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_pwd_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no pwd request: '%s'\n", context->pwd_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = MPIDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_pwd_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_no_pwd_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_pwd_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_pwd_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the pwd request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_pwd_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read pwd request: '%s'\n", context->pwd_request);
    if (strcmp(context->pwd_request, SMPD_PWD_REQUEST) == 0)
    {
	strcpy(context->smpd_pwd, SMPD_DEFAULT_PASSWORD);
	context->write_state = SMPD_WRITING_SMPD_PASSWORD;
	result = MPIDU_Sock_post_write(context->sock, context->smpd_pwd, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the smpd password,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_pwd_request");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_pwd_request");
	return SMPD_SUCCESS;
    }
    result = smpd_generate_session_header(context->session_header, 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn("smpd_state_reading_pwd_request");
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = MPIDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_pwd_request");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_pwd_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_smpd_password(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_smpd_password");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the smpd password, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_smpd_password");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read smpd password, %d bytes\n", strlen(context->password));
    context->read_state = SMPD_IDLE;
    if (strcmp(context->password, smpd_process.SMPDPassword) == 0)
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR);
	context->write_state = SMPD_WRITING_SESSION_ACCEPT;
	result = MPIDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session accepted message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_smpd_password");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_REJECTED_STR);
	context->write_state = SMPD_WRITING_SESSION_REJECT;
	result = MPIDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session rejected message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_smpd_password");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    smpd_exit_fn("smpd_state_reading_smpd_password");
    return SMPD_SUCCESS;
}

int smpd_state_writing_cred_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_cred_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_cred_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote cred request: '%s'\n", context->cred_request);
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_CRED_ACK;
    result = MPIDU_Sock_post_read(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the cred ack,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_cred_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_cred_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_cred_ack(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_cred_ack");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the cred ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_cred_ack");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read cred ack: '%s'\n", context->cred_request);
    context->write_state = SMPD_IDLE;
    if (strcmp(context->cred_request, "yes") == 0)
    {
	context->read_state = SMPD_READING_ACCOUNT;
	result = MPIDU_Sock_post_read(context->sock, context->account, SMPD_MAX_ACCOUNT_LENGTH, SMPD_MAX_ACCOUNT_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the account credential,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_writing_cred_request");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_cred_request");
	return SMPD_SUCCESS;
    }
    context->state = SMPD_CLOSING;
    result = MPIDU_Sock_post_close(context->sock);
    smpd_exit_fn("smpd_state_writing_cred_request");
    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
}

int smpd_state_writing_cred_ack_yes(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_cred_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request yes ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_cred_ack_yes");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request yes ack.\n");

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_WRITING_ACCOUNT;
    result = MPIDU_Sock_post_write(context->sock, context->account, SMPD_MAX_ACCOUNT_LENGTH, SMPD_MAX_ACCOUNT_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the account '%s',\nsock error: %s\n",
	    context->account, get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_cred_ack_yes");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_cred_ack_yes");
    return SMPD_SUCCESS;
}

int smpd_state_writing_cred_ack_no(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_cred_ack_no");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the cred request no ack, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_cred_ack_no");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }

    smpd_dbg_printf("wrote cred request yes ack.\n");

    /* insert code here to handle failed connection attempt */

    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    context->state = SMPD_CLOSING;
    result = MPIDU_Sock_post_close(context->sock);
    smpd_exit_fn("smpd_state_writing_cred_ack_no");
    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
}

int smpd_state_reading_account(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_account");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the account, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_account");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read account: '%s'\n", context->account);
    context->read_state = SMPD_READING_PASSWORD;
    result = MPIDU_Sock_post_read(context->sock, context->password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the password credential,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_account");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_account");
    return SMPD_SUCCESS;
}

int smpd_state_reading_password(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_password");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the password, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_password");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("read password, %d bytes\n", strlen(context->password));
#ifdef HAVE_WINDOWS_H
    /* launch the manager process */
    result = smpd_start_win_mgr(context);
    if (result == SMPD_SUCCESS)
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR);
	context->write_state = SMPD_WRITING_PROCESS_SESSION_ACCEPT;
	result = MPIDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the process session accepted message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_password");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
    else
    {
	strcpy(context->pwd_request, SMPD_AUTHENTICATION_REJECTED_STR);
	context->write_state = SMPD_WRITING_PROCESS_SESSION_REJECT;
	result = MPIDU_Sock_post_write(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the session rejected message,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_password");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
#else
    /* post a write of the noreconnect request */
    smpd_dbg_printf("smpd writing noreconnect request\n");
    context->write_state = SMPD_WRITING_NO_RECONNECT_REQUEST;
    strcpy(context->port_str, SMPD_NO_RECONNECT_PORT_STR);
    result = MPIDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_reading_password");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
#endif
    smpd_exit_fn("smpd_state_reading_password");
    return SMPD_SUCCESS;
}

int smpd_state_writing_account(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_account");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the account, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_account");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote account: '%s'\n", context->account);
    context->write_state = SMPD_WRITING_PASSWORD;
    result = MPIDU_Sock_post_write(context->sock, context->password, SMPD_MAX_PASSWORD_LENGTH, SMPD_MAX_PASSWORD_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the password,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_account");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_account");
    return SMPD_SUCCESS;
}

int smpd_state_writing_password(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_password");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the password, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_password");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote password\n");
    context->read_state = SMPD_READING_PROCESS_RESULT;
    result = MPIDU_Sock_post_read(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the process session result,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_password");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_password");
    return SMPD_SUCCESS;
}

int smpd_state_writing_no_cred_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_no_cred_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the no cred request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_cred_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no cred request: '%s'\n", context->cred_request);
#ifdef HAVE_WINDOWS_H
    /* launch the manager process */
    result = smpd_start_win_mgr(context);
    if (result != SMPD_SUCCESS)
    {
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a failed win_mgr, error:\n%s\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_no_cred_request");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_no_cred_request");
	return SMPD_SUCCESS;
    }
    /* post a write of the reconnect request */
    smpd_dbg_printf("smpd writing reconnect request: port %s\n", context->port_str);
    context->write_state = SMPD_WRITING_RECONNECT_REQUEST;
    result = MPIDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_cred_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
#else
    /* post a write of the noreconnect request */
    smpd_dbg_printf("smpd writing noreconnect request\n");
    context->write_state = SMPD_WRITING_NO_RECONNECT_REQUEST;
    strcpy(context->port_str, SMPD_NO_RECONNECT_PORT_STR);
    result = MPIDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_cred_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
#endif
    smpd_exit_fn("smpd_state_writing_no_cred_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_cred_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_state_reading_cred_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the cred request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_cred_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read cred request: '%s'\n", context->cred_request);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->cred_request, SMPD_CRED_REQUEST) == 0)
    {
#ifdef HAVE_WINDOWS_H
	if (smpd_process.UserAccount[0] == '\0')
	{
	    if (smpd_process.logon || 
		(!smpd_get_cached_password(context->account, context->password) &&
		!smpd_read_password_from_registry(context->account, context->password)))
	    {
		if (smpd_process.id > 0 && smpd_process.parent_context && smpd_process.parent_context->sock != MPIDU_SOCK_INVALID_SOCK)
		{
		    result = smpd_create_command("cred_request", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to create a command structure for the cred_request command.\n");
			smpd_exit_fn("smpd_state_reading_cred_request");
			return result;
		    }
		    result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to add host=%s to the cred_request command.\n", context->connect_to->host);
			smpd_exit_fn("smpd_state_reading_cred_request");
			return result;
		    }
		    cmd_ptr->context = context;
		    result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		    if (result != SMPD_SUCCESS)
		    {
			smpd_err_printf("unable to post a write of the cred_request command.\n");
			smpd_exit_fn("smpd_state_reading_cred_request");
			return result;
		    }
		    smpd_exit_fn("smpd_state_reading_cred_request");
		    return SMPD_SUCCESS;
		}
		if (smpd_process.id == 0 && smpd_process.credentials_prompt)
		{
		    fprintf(stderr, "User credentials needed to launch processes:\n");
		    smpd_get_account_and_password(context->account, context->password);
		    smpd_cache_password(context->account, context->password);
		}
		else
		{
		    /*smpd_post_abort_command("User credentials needed to launch processes.\n");*/
		    strcpy(context->account, "invalid account");
		}
	    }
	    else if (!smpd_process.logon)
	    {
		/* This will re-cache cached passwords but I can't think of a way to determine the difference between
		a cached and non-cached password retrieval. */
		/*if (password_read_from_registry)*/
		smpd_cache_password(context->account, context->password);
	    }
	}
	else
	{
	    strcpy(context->account, smpd_process.UserAccount);
	    strcpy(context->password, smpd_process.UserPassword);
	}
#else
	if (smpd_process.UserAccount[0] == '\0')
	{
	    if (smpd_process.id > 0 && smpd_process.parent_context && smpd_process.parent_context->sock != MPIDU_SOCK_INVALID_SOCK)
	    {
		result = smpd_create_command("cred_request", smpd_process.id, 0, SMPD_TRUE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a command structure for the cred_request command.\n");
		    smpd_exit_fn("smpd_state_reading_cred_request");
		    return result;
		}
		result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add host=%s to the cred_request command.\n", context->connect_to->host);
		    smpd_exit_fn("smpd_state_reading_cred_request");
		    return result;
		}
		cmd_ptr->context = context;
		result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the cred_request command.\n");
		    smpd_exit_fn("smpd_state_reading_cred_request");
		    return result;
		}
		smpd_exit_fn("smpd_state_reading_cred_request");
		return SMPD_SUCCESS;
	    }
	    if (smpd_process.credentials_prompt)
	    {
		fprintf(stderr, "User credentials needed to launch processes:\n");
		smpd_get_account_and_password(context->account, context->password);
	    }
	    else
	    {
		/*smpd_post_abort_command("User credentials needed to launch processes.\n");*/
		strcpy(context->account, "invalid account");
	    }
	}
	else
	{
	    strcpy(context->account, smpd_process.UserAccount);
	    strcpy(context->password, smpd_process.UserPassword);
	}
#endif
	if (strcmp(context->account, "invalid account") == 0)
	{
	    smpd_err_printf("Attempting to create a session with an smpd that requires credentials without having obtained any credentials.\n");
	    strcpy(context->cred_request, "no");
	    context->write_state = SMPD_WRITING_CRED_ACK_NO;
	    context->read_state = SMPD_IDLE;
	    result = MPIDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	    smpd_exit_fn("smpd_state_reading_cred_request");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	strcpy(context->cred_request, "yes");
	context->write_state = SMPD_WRITING_CRED_ACK_YES;
	context->read_state = SMPD_IDLE;
	result = MPIDU_Sock_post_write(context->sock, context->cred_request, SMPD_MAX_CRED_REQUEST_LENGTH, SMPD_MAX_CRED_REQUEST_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the cred request yes ack.\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_cred_request");
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }
    context->read_state = SMPD_READING_RECONNECT_REQUEST;
    result = MPIDU_Sock_post_read(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the re-connect request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_cred_request");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_cred_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_reconnect_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_reconnect_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_reconnect_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote reconnect request: '%s'\n", context->port_str);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = MPIDU_Sock_post_close(context->sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a close on sock(%d) after reconnect request written, error:\n%s\n",
	    MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_reconnect_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_reconnect_request");
    return SMPD_SUCCESS;
}

int smpd_state_writing_no_reconnect_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_no_reconnect_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the no re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote no reconnect request: '%s'\n", context->port_str);
#ifdef HAVE_WINDOWS_H
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = MPIDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
#else
    /* fork the manager process */
    result = smpd_start_unx_mgr(context);
    if (result != SMPD_SUCCESS)
    {
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a failed unx_mgr, error:\n%s\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	return SMPD_SUCCESS;
    }
    if (smpd_process.root_smpd)
    {
	smpd_dbg_printf("root closing sock(%d) after fork.\n", MPIDU_Sock_get_sock_id(context->sock));
	context->state = SMPD_CLOSING;
	context->read_state = SMPD_IDLE;
	context->write_state = SMPD_IDLE;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close for the sock(%d) from a unx_mgr, error:\n%s\n",
		MPIDU_Sock_get_sock_id(context->sock), get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	    return SMPD_FAIL;
	}
    }
    else
    {
	smpd_dbg_printf("child closing listener sock(%d) after fork.\n", MPIDU_Sock_get_sock_id(smpd_process.listener_context->sock));
	/* close the listener in the child */
	smpd_process.listener_context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(smpd_process.listener_context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a close on the listener,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	    return SMPD_FAIL;
	}
	/* post a read of the session header */
	context->read_state = SMPD_READING_SESSION_HEADER;
	result = MPIDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for the session header,\nsock error: %s\n",
		get_sock_error_string(result));
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_writing_no_reconnect_request");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
    }
#endif
    smpd_exit_fn("smpd_state_writing_no_reconnect_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_reconnect_request(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_reconnect_request");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the re-connect request, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_reconnect_request");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read re-connect request: '%s'\n", context->port_str);
    if (strcmp(context->port_str, SMPD_NO_RECONNECT_PORT_STR))
    {
	int port;
	smpd_context_t *rc_context;

	smpd_dbg_printf("closing the old socket in the %s context.\n", smpd_get_context_str(context));
	/* close the old sock */
	smpd_dbg_printf("MPIDU_Sock_post_close(%d)\n", MPIDU_Sock_get_sock_id(context->sock));
	/*context->state = SMPD_CLOSING;*/ /* don't set it here, set it later after the state has been copied to the new context */
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("MPIDU_Sock_post_close failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_reconnect_request");
	    return SMPD_FAIL;
	}

	smpd_dbg_printf("connecting a new socket.\n");
	/* reconnect */
	port = atol(context->port_str);
	if (port < 1)
	{
	    smpd_err_printf("Invalid reconnect port read: %d\n", port);
	    smpd_exit_fn("smpd_state_reading_reconnect_request");
	    return SMPD_FAIL;
	}
	result = smpd_create_context(context->type, context->set, MPIDU_SOCK_INVALID_SOCK, context->id, &rc_context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a new context for the reconnection.\n");
	    smpd_exit_fn("smpd_state_reading_reconnect_request");
	    return SMPD_FAIL;
	}
	rc_context->state = context->state;
	rc_context->write_state = SMPD_RECONNECTING;
	context->state = SMPD_CLOSING;
	rc_context->connect_to = context->connect_to;
	rc_context->connect_return_id = context->connect_return_id;
	rc_context->connect_return_tag = context->connect_return_tag;
	strcpy(rc_context->host, context->host);
	/* replace the old context with the new */
	if (rc_context->type == SMPD_CONTEXT_LEFT_CHILD || rc_context->type == SMPD_CONTEXT_CHILD)
	    smpd_process.left_context = rc_context;
	if (rc_context->type == SMPD_CONTEXT_RIGHT_CHILD)
	    smpd_process.right_context = rc_context;
	smpd_dbg_printf("posting a re-connect to %s:%d in %s context.\n", rc_context->connect_to->host, port, smpd_get_context_str(rc_context));
	result = MPIDU_Sock_post_connect(rc_context->set, rc_context, rc_context->connect_to->host, port, &rc_context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("Unable to post a connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result));
	    if (smpd_post_abort_command("Unable to connect to '%s:%d',\nsock error: %s\n",
		rc_context->connect_to->host, port, get_sock_error_string(result)) != SMPD_SUCCESS)
	    {
		smpd_exit_fn("smpd_state_reading_reconnect_request");
		return SMPD_FAIL;
	    }
	}
	smpd_exit_fn("smpd_state_reading_reconnect_request");
	return SMPD_SUCCESS;
    }
    result = smpd_generate_session_header(context->session_header, context->connect_to->id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn("smpd_state_reading_reconnect_request");
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = MPIDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_reconnect_request");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_reconnect_request");
    return SMPD_SUCCESS;
}

int smpd_state_reading_session_header(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_session_header");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the session header, %s.\n", get_sock_error_string(event_ptr->error));
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_session_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_session_header");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read session header: '%s'\n", context->session_header);
    result = smpd_interpret_session_header(context->session_header);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to interpret the session header: '%s'\n", context->session_header);
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_session_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_session_header");
	return SMPD_FAIL;
    }
    context->type = SMPD_CONTEXT_PARENT;
    context->id = smpd_process.parent_id; /* set by smpd_interpret_session_header */
    if (smpd_process.parent_context && smpd_process.parent_context != context)
	smpd_err_printf("replacing parent context.\n");
    smpd_process.parent_context = context;
    result = smpd_post_read_command(context);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command\n");
	if (smpd_process.root_smpd)
	{
	    context->state = SMPD_CLOSING;
	    result = MPIDU_Sock_post_close(context->sock);
	    smpd_exit_fn("smpd_state_reading_session_header");
	    return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_session_header");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_session_header");
    return SMPD_SUCCESS;
}

int smpd_state_writing_session_header(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr, MPIDU_Sock_set_t set)
{
    int result;
    smpd_command_t *cmd_ptr;
    smpd_context_t *result_context, *context_in;
    MPIDU_SOCK_NATIVE_FD stdin_fd;
    MPIDU_Sock_t insock;
#ifdef HAVE_WINDOWS_H
    DWORD dwThreadID;
    SOCKET hWrite;
#endif

    smpd_enter_fn("smpd_state_writing_session_header");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the session header, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_session_header");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session header: '%s'\n", context->session_header);
    switch (context->state)
    {
    case SMPD_MPIEXEC_CONNECTING_TREE:
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}

	/* mark the node as connected */
	context->connect_to->connected = SMPD_TRUE;

	/* create a command to connect to the next host in the tree */
	context->connect_to = context->connect_to->next;
	if (context->connect_to)
	{
	    smpd_dbg_printf("creating connect command to '%s'\n", context->connect_to->host);
	    /* create a connect command to be sent to the parent */
	    result = smpd_create_command("connect", 0, context->connect_to->parent, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a connect command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "host", context->connect_to->host);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", context->connect_to->host);
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }
	    result = smpd_add_command_int_arg(cmd_ptr, "id", context->connect_to->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", context->connect_to->host);
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the connect command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }
	}
	else
	{
	    /*
	    smpd_err_printf("this code seems to never get executed.\n");
	    return SMPD_FAIL;
	    */

	    smpd_dbg_printf("hosts connected, sending start_dbs command.\n");
	    /* create the start_dbs command to be sent to the first host */
	    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a start_dbs command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }

	    if (context->spawn_context)
	    {
		smpd_dbg_printf("spawn_context found, adding preput values to the start_dbs command.\n");
		result = smpd_add_command_int_arg(cmd_ptr, "npreput", context->spawn_context->npreput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn("smpd_state_writing_session_header");
		    return result;
		}

		result = smpd_add_command_arg(cmd_ptr, "preput", context->spawn_context->preput);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
		    smpd_exit_fn("smpd_state_writing_session_header");
		    return result;
		}
	    }

	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the start_dbs command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return result;
	    }
	}
	break;
    case SMPD_CONNECTING:
	/* post a read of the next command on the newly connected context */
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	/* send the successful result command back to the connector */
	result = smpd_create_command("result", smpd_process.id, context->connect_return_id, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a result command.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_int_arg(cmd_ptr, "cmd_tag", context->connect_return_tag);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the tag to the result command.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "cmd_orig", "connect");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add cmd_orig to the result command for a connect command\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	result = smpd_add_command_arg(cmd_ptr, "result", SMPD_SUCCESS_STR);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to add the result string to the result command.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	smpd_command_destination(context->connect_return_id, &result_context);
	smpd_dbg_printf("sending result command: \"%s\"\n", cmd_ptr->cmd);
	result = smpd_post_write_command(result_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a write of the result command to the %s context.\n", smpd_get_context_str(result_context));
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	break;
    case SMPD_MPIEXEC_CONNECTING_SMPD:
	/* post a read for a possible incoming command */
	result = smpd_post_read_command(context);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to post a read for an incoming command from the smpd on '%s', error:\n%s\n",
		context->host, get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}

	/* check to see if this is a shutdown session */
	if (smpd_process.shutdown)
	{
	    result = smpd_create_command("shutdown", 0, 1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a shutdown command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the shutdown command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a restart session */
	if (smpd_process.restart)
	{
	    result = smpd_create_command("restart", 0, 1, SMPD_FALSE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a restart command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the restart command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a validate session */
	if (smpd_process.validate)
	{
	    result = smpd_create_command("validate", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a validate command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "account", smpd_process.UserAccount);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the account(%s) to the validate command.\n", smpd_process.UserAccount);
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_add_command_arg(cmd_ptr, "password", smpd_process.UserPassword);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the password to the validate command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the validate command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    break;
	}

	/* check to see if this is a status session */
	if (smpd_process.do_status)
	{
	    result = smpd_create_command("status", 0, 1, SMPD_TRUE, &cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a status command.\n");
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    result = smpd_post_write_command(context, cmd_ptr);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the status command on the %s context.\n",
		    smpd_get_context_str(context));
		smpd_exit_fn("smpd_state_writing_session_header");
		return SMPD_FAIL;
	    }
	    break;
	}

	/* get a handle to stdin */
#ifdef HAVE_WINDOWS_H
	result = smpd_make_socket_loop((SOCKET*)&stdin_fd, &hWrite);
	if (result)
	{
	    smpd_err_printf("Unable to make a local socket loop to forward stdin.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
#else
	stdin_fd = fileno(stdin);
#endif

	/* convert the native handle to a sock */
	result = MPIDU_Sock_native_to_sock(context->set, stdin_fd, NULL, &insock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to create a sock from stdin,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	/* create a context for reading from stdin */
	result = smpd_create_context(SMPD_CONTEXT_STDIN, set, insock, -1, &context_in);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create a context for stdin.\n");
	    smpd_exit_fn("smpd_state_writing_session_header");
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
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	smpd_process.hStdinThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)StdinThread, (void*)hWrite, 0, &dwThreadID);
	if (smpd_process.hStdinThread == NULL)
	{
	    smpd_err_printf("Unable to create a thread to read stdin, error %d\n", GetLastError());
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
#endif

	/* post a read for a user command from stdin */
	context_in->read_state = SMPD_READING_STDIN;
	result = MPIDU_Sock_post_read(insock, context_in->read_cmd.cmd, 1, 1, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a read on stdin for an incoming user command, error:\n%s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_writing_session_header");
	    return SMPD_FAIL;
	}
	break;
    default:
	smpd_err_printf("wrote session header while in state %d\n", context->state);
	break;
    }
    smpd_exit_fn("smpd_state_writing_session_header");
    return SMPD_SUCCESS;
}

int smpd_state_reading_smpd_result(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_smpd_result");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the smpd result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_smpd_result");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read smpd result: '%s'\n", context->pwd_request);
    context->read_state = SMPD_IDLE;
    if (strcmp(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	smpd_dbg_printf("connection rejected, server returned - %s\n", context->pwd_request);
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to close sock, error:\n%s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_smpd_result");
	    return SMPD_FAIL;
	}
	/* abort here? */
	smpd_exit_fn("smpd_state_reading_smpd_result");
	return SMPD_SUCCESS;
    }
    result = smpd_generate_session_header(context->session_header, 1);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to generate a session header.\n");
	smpd_exit_fn("smpd_state_reading_smpd_result");
	return SMPD_FAIL;
    }
    context->write_state = SMPD_WRITING_SESSION_HEADER;
    result = MPIDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_smpd_result");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_smpd_result");
    return SMPD_SUCCESS;
}

int smpd_state_reading_process_result(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_reading_process_result");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to read the process session result, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_reading_process_result");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("read process session result: '%s'\n", context->pwd_request);
    if (strcmp(context->pwd_request, SMPD_AUTHENTICATION_ACCEPTED_STR))
    {
	char *host_ptr;
#ifdef HAVE_WINDOWS_H
	smpd_delete_cached_password();
#endif
	if (smpd_process.do_console && smpd_process.console_host[0] != '\0')
	    host_ptr = smpd_process.console_host;
	else if (context->connect_to && context->connect_to->host[0] != '\0')
	    host_ptr = context->connect_to->host;
	else if (context->host[0] != '\0')
	    host_ptr = context->host;
	else
	    host_ptr = NULL;
	if (host_ptr)
	    printf("Credentials for %s rejected connecting to %s\n", context->account, host_ptr);
	else
	    printf("Credentials for %s rejected.\n", context->account);
	fflush(stdout);
	smpd_dbg_printf("process session rejected\n");
	context->read_state = SMPD_IDLE;
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to close sock,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_state_reading_process_result");
	    return SMPD_FAIL;
	}
	/* when does a forming context get assinged it's global place?  At creation?  At connection? */
	if (smpd_process.left_context == smpd_process.left_context)
	    smpd_process.left_context = NULL;
	if (host_ptr)
	    result = smpd_post_abort_command("Unable to connect to %s", host_ptr);
	else
	    result = smpd_post_abort_command("connection failed");
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to create the close command to tear down the job tree.\n");
	    smpd_exit_fn("smpd_state_reading_process_result");
	    return SMPD_FAIL;
	}
	smpd_exit_fn("smpd_state_reading_process_result");
	return SMPD_SUCCESS;
    }
    context->read_state = SMPD_READING_RECONNECT_REQUEST;
    result = MPIDU_Sock_post_read(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the re-connect request,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_reading_process_result");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_reading_process_result");
    return SMPD_SUCCESS;
}

int smpd_state_writing_session_accept(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_session_accept");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the session accept string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_session_accept");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session accept: '%s'\n", context->pwd_request);
    context->read_state = SMPD_READING_SESSION_HEADER;
    result = MPIDU_Sock_post_read(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session header,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_session_accept");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_session_accept");
    return SMPD_SUCCESS;
}

int smpd_state_writing_session_reject(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_session_reject");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the session reject string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_session_reject");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote session reject: '%s'\n", context->pwd_request);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = MPIDU_Sock_post_close(context->sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a close of the sock after writing a reject message,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_session_reject");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_session_reject");
    return SMPD_SUCCESS;
}

int smpd_state_writing_process_session_accept(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_process_session_accept");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the process session accept string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_process_session_accept");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session accept: '%s'\n", context->pwd_request);
    /* post a write of the reconnect request */
    smpd_dbg_printf("smpd writing reconnect request: port %s\n", context->port_str);
    context->write_state = SMPD_WRITING_RECONNECT_REQUEST;
    result = MPIDU_Sock_post_write(context->sock, context->port_str, SMPD_MAX_PORT_STR_LENGTH, SMPD_MAX_PORT_STR_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("Unable to post a write of the re-connect port number(%s) back to mpiexec,\nsock error: %s\n",
	    context->port_str, get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_process_session_accept");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_process_session_accept");
    return SMPD_SUCCESS;
}

int smpd_state_writing_process_session_reject(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_process_session_reject");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the process session reject string, %s.\n", get_sock_error_string(event_ptr->error));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_process_session_reject");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_dbg_printf("wrote process session reject: '%s'\n", context->pwd_request);
    context->state = SMPD_CLOSING;
    context->read_state = SMPD_IDLE;
    context->write_state = SMPD_IDLE;
    result = MPIDU_Sock_post_close(context->sock);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a close of the sock after writing a process session reject message,\nsock error: %s\n",
	    get_sock_error_string(result));
	context->state = SMPD_CLOSING;
	result = MPIDU_Sock_post_close(context->sock);
	smpd_exit_fn("smpd_state_writing_process_session_reject");
	return result == MPI_SUCCESS ? SMPD_SUCCESS : SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_process_session_reject");
    return SMPD_SUCCESS;
}

int smpd_state_writing_smpd_password(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_state_writing_smpd_password");
    if (event_ptr->error != MPI_SUCCESS)
    {
	smpd_err_printf("unable to write the smpd password, %s.\n", get_sock_error_string(event_ptr->error));
	smpd_exit_fn("smpd_state_writing_smpd_password");
	return SMPD_FAIL;
    }
    smpd_dbg_printf("wrote smpd password.\n");
    context->write_state = SMPD_IDLE;
    context->read_state = SMPD_READING_SMPD_RESULT;
    result = MPIDU_Sock_post_read(context->sock, context->pwd_request, SMPD_AUTHENTICATION_REPLY_LENGTH, SMPD_AUTHENTICATION_REPLY_LENGTH, NULL);
    if (result != MPI_SUCCESS)
    {
	smpd_err_printf("unable to post a read of the session request result,\nsock error: %s\n",
	    get_sock_error_string(result));
	smpd_exit_fn("smpd_state_writing_smpd_password");
	return SMPD_FAIL;
    }
    smpd_exit_fn("smpd_state_writing_smpd_password");
    return SMPD_SUCCESS;
}

int smpd_handle_op_read(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;

    smpd_enter_fn("smpd_handle_op_read");

    switch (context->read_state)
    {
    case SMPD_READING_CHALLENGE_STRING:
	result = smpd_state_reading_challenge_string(context, event_ptr);
	break;
    case SMPD_READING_CONNECT_RESULT:
	result = smpd_state_reading_connect_result(context, event_ptr);
	break;
    case SMPD_READING_SMPD_RESULT:
	result = smpd_state_reading_smpd_result(context, event_ptr);
	break;
    case SMPD_READING_PROCESS_RESULT:
	result = smpd_state_reading_process_result(context, event_ptr);
	break;
    case SMPD_READING_CHALLENGE_RESPONSE:
	result = smpd_state_reading_challenge_response(context, event_ptr);
	break;
    case SMPD_READING_STDIN:
	result = smpd_state_reading_stdin(context, event_ptr);
	break;
    case SMPD_READING_STDOUT:
    case SMPD_READING_STDERR:
	result = smpd_state_reading_stdouterr(context, event_ptr);
	break;
    case SMPD_READING_CMD_HEADER:
	result = smpd_state_reading_cmd_header(context, event_ptr);
	break;
    case SMPD_READING_CMD:
	result = smpd_state_reading_cmd(context, event_ptr);
	break;
    case SMPD_READING_SESSION_REQUEST:
	result = smpd_state_reading_session_request(context, event_ptr);
	break;
    case SMPD_READING_PWD_REQUEST:
	result = smpd_state_reading_pwd_request(context, event_ptr);
	break;
    case SMPD_READING_SMPD_PASSWORD:
	result = smpd_state_reading_smpd_password(context, event_ptr);
	break;
    case SMPD_READING_CRED_ACK:
	result = smpd_state_reading_cred_ack(context, event_ptr);
	break;
    case SMPD_READING_ACCOUNT:
	result = smpd_state_reading_account(context, event_ptr);
	break;
    case SMPD_READING_PASSWORD:
	result = smpd_state_reading_password(context, event_ptr);
	break;
    case SMPD_READING_CRED_REQUEST:
	result = smpd_state_reading_cred_request(context, event_ptr);
	break;
    case SMPD_READING_RECONNECT_REQUEST:
	result = smpd_state_reading_reconnect_request(context, event_ptr);
	break;
    case SMPD_READING_SESSION_HEADER:
	result = smpd_state_reading_session_header(context, event_ptr);
	break;
    default:
	smpd_err_printf("sock_op_read returned while context is in state: %s\n",
	    smpd_get_state_string(context->read_state));
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn("smpd_handle_op_read");
    return result;
}

int smpd_handle_op_write(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr, MPIDU_Sock_set_t set)
{
    int result;

    smpd_enter_fn("smpd_handle_op_write");
    switch (context->write_state)
    {
    case SMPD_WRITING_CHALLENGE_RESPONSE:
	result = smpd_state_writing_challenge_response(context, event_ptr);
	break;
    case SMPD_WRITING_CHALLENGE_STRING:
	result = smpd_state_writing_challenge_string(context, event_ptr);
	break;
    case SMPD_WRITING_CONNECT_RESULT:
	result = smpd_state_writing_connect_result(context, event_ptr);
	break;
    case SMPD_WRITING_CMD:
	result = smpd_state_writing_cmd(context, event_ptr);
	break;
    case SMPD_WRITING_SMPD_SESSION_REQUEST:
	result = smpd_state_writing_smpd_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_REQUEST:
	result = smpd_state_writing_process_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PMI_SESSION_REQUEST:
	result = smpd_state_writing_pmi_session_request(context, event_ptr);
	break;
    case SMPD_WRITING_PWD_REQUEST:
	result = smpd_state_writing_pwd_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_PWD_REQUEST:
	result = smpd_state_writing_no_pwd_request(context, event_ptr);
	break;
    case SMPD_WRITING_SMPD_PASSWORD:
	result = smpd_state_writing_smpd_password(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_REQUEST:
	result = smpd_state_writing_cred_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_CRED_REQUEST:
	result = smpd_state_writing_no_cred_request(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_YES:
	result = smpd_state_writing_cred_ack_yes(context, event_ptr);
	break;
    case SMPD_WRITING_CRED_ACK_NO:
	result = smpd_state_writing_cred_ack_no(context, event_ptr);
	break;
    case SMPD_WRITING_RECONNECT_REQUEST:
	result = smpd_state_writing_reconnect_request(context, event_ptr);
	break;
    case SMPD_WRITING_NO_RECONNECT_REQUEST:
	result = smpd_state_writing_no_reconnect_request(context, event_ptr);
	break;
    case SMPD_WRITING_ACCOUNT:
	result = smpd_state_writing_account(context, event_ptr);
	break;
    case SMPD_WRITING_PASSWORD:
	result = smpd_state_writing_password(context, event_ptr);
	break;
    case SMPD_WRITING_SESSION_HEADER:
	result = smpd_state_writing_session_header(context, event_ptr, set);
	break;
    case SMPD_WRITING_SESSION_ACCEPT:
	result = smpd_state_writing_session_accept(context, event_ptr);
	break;
    case SMPD_WRITING_SESSION_REJECT:
	result = smpd_state_writing_session_reject(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_ACCEPT:
	result = smpd_state_writing_process_session_accept(context, event_ptr);
	break;
    case SMPD_WRITING_PROCESS_SESSION_REJECT:
	result = smpd_state_writing_process_session_reject(context, event_ptr);
	break;
    case SMPD_WRITING_DATA_TO_STDIN:
	result = smpd_state_smpd_writing_data_to_stdin(context, event_ptr);
	break;
    default:
	if (event_ptr->error != MPI_SUCCESS)
	{
	    smpd_err_printf("sock_op_write failed while context is in state %s, %s\n",
		smpd_get_state_string(context->write_state), get_sock_error_string(event_ptr->error));
	}
	else
	{
	    smpd_err_printf("sock_op_write returned while context is in state %s, %s\n",
		smpd_get_state_string(context->write_state), get_sock_error_string(event_ptr->error));
	}
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn("smpd_handle_op_write");
    return result;
}

int smpd_handle_op_accept(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr, MPIDU_Sock_set_t set)
{
    int result;

    smpd_enter_fn("smpd_handle_op_accept");
    switch (context->state)
    {
    case SMPD_SMPD_LISTENING:
	result = smpd_state_smpd_listening(context, event_ptr, set);
	break;
    case SMPD_MGR_LISTENING:
	result = smpd_state_mgr_listening(context, event_ptr, set);
	break;
    default:
	smpd_err_printf("sock_op_accept returned while context is in state: %s\n",
	    smpd_get_state_string(context->state));
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn("smpd_handle_op_accept");
    return result;
}

int smpd_handle_op_connect(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result = SMPD_SUCCESS;

    smpd_enter_fn("smpd_handle_op_connect");
    switch (context->state)
    {
    case SMPD_MPIEXEC_CONNECTING_TREE:
	if (event_ptr->error != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to connect mpiexec tree, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
    case SMPD_CONNECTING:
	if (event_ptr->error != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to connect an internal tree node, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
	if (context->write_state == SMPD_RECONNECTING)
	{
	    result = smpd_generate_session_header(context->session_header, context->connect_to->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to generate a session header.\n");
		smpd_exit_fn("smpd_handle_op_connect");
		return SMPD_FAIL;
	    }
	    context->write_state = SMPD_WRITING_SESSION_HEADER;
	    result = MPIDU_Sock_post_write(context->sock, context->session_header, SMPD_MAX_SESSION_HEADER_LENGTH, SMPD_MAX_SESSION_HEADER_LENGTH, NULL);
	    if (result != MPI_SUCCESS)
	    {
		smpd_err_printf("unable to post a send of the session header,\nsock error: %s\n",
		    get_sock_error_string(result));
		smpd_exit_fn("smpd_handle_op_connect");
		return SMPD_FAIL;
	    }
	    result = SMPD_SUCCESS;
	    break;
	}
	/* no break here, not-reconnecting contexts fall through */
    case SMPD_MPIEXEC_CONNECTING_SMPD:
    case SMPD_CONNECTING_PMI:
	if (event_ptr->error != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to connect to the smpd, %s.\n", get_sock_error_string(event_ptr->error));
	    result = SMPD_FAIL;
	    break;
	}
	smpd_dbg_printf("connect succeeded, posting read of the challenge string\n");
	context->read_state = SMPD_READING_CHALLENGE_STRING;
	result = MPIDU_Sock_post_read(context->sock, context->pszChallengeResponse, SMPD_AUTHENTICATION_STR_LEN, SMPD_AUTHENTICATION_STR_LEN, NULL);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("unable to post a read of the challenge string,\nsock error: %s\n",
		get_sock_error_string(result));
	    smpd_exit_fn("smpd_handle_op_connect");
	    return SMPD_FAIL;
	}
	result = SMPD_SUCCESS;
	break;
    default:
	if (event_ptr->error != MPI_SUCCESS)
	{
	    smpd_err_printf("sock_op_connect failed while %s context is in state %s, %s\n",
		smpd_get_context_str(context), smpd_get_state_string(context->state),
		get_sock_error_string(event_ptr->error));
	}
	else
	{
	    smpd_err_printf("sock_op_connect returned while %s context is in state %s, %s\n",
		smpd_get_context_str(context), smpd_get_state_string(context->state),
		get_sock_error_string(event_ptr->error));
	}
	result = SMPD_FAIL;
	break;
    }
    smpd_exit_fn("smpd_handle_op_connect");
    return result;
}

int smpd_handle_op_close(smpd_context_t *context, MPIDU_Sock_event_t *event_ptr)
{
    int result;
    smpd_command_t *cmd_ptr;

    smpd_enter_fn("smpd_handle_op_close");
    smpd_dbg_printf("op_close received - %s state.\n", smpd_get_state_string(context->state));
    switch (context->state)
    {
    case SMPD_SMPD_LISTENING:
    case SMPD_MGR_LISTENING:
	smpd_process.listener_context = NULL;
	break;
    case SMPD_DONE:
	smpd_free_context(context);
	smpd_exit_fn("smpd_handle_op_close");
	return SMPD_EXIT;
    case SMPD_RESTARTING:
	smpd_restart();
	break;
    case SMPD_EXITING:
	if (smpd_process.listener_context)
	{
	    smpd_process.listener_context->state = SMPD_EXITING;
	    smpd_dbg_printf("closing the listener (state = %s).\n", smpd_get_state_string(smpd_process.listener_context->state));
	    result = MPIDU_Sock_post_close(smpd_process.listener_context->sock);
	    smpd_process.listener_context = NULL;
	    if (result == MPI_SUCCESS)
	    {
		break;
	    }
	    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
		get_sock_error_string(result));
	}
	smpd_free_context(context);
	/*smpd_exit(0);*/
#ifdef HAVE_WINDOWS_H
	if (smpd_process.bService)
	    SetEvent(smpd_process.hBombDiffuseEvent);
#endif
	smpd_exit_fn("smpd_handle_op_close");
	return SMPD_EXIT;
    case SMPD_CLOSING:
	if (context->process && (context->type == SMPD_CONTEXT_STDOUT || context->type == SMPD_CONTEXT_STDERR || context->type == SMPD_CONTEXT_PMI))
	{
	    context->process->context_refcount--;
	    if (context->process->context_refcount < 1)
	    {
		smpd_process_t *trailer, *iter;

#ifdef HAVE_WINDOWS_H
		smpd_process_from_registry(context->process);
#endif
		result = smpd_wait_process(context->process->wait, &context->process->exitcode);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to wait for the process to exit: '%s'\n", context->process->exe);
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
		/* create the process exited command */
		result = smpd_create_command("exit", smpd_process.id, 0, SMPD_FALSE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create an exit command for rank %d\n", context->process->rank);
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
		result = smpd_add_command_int_arg(cmd_ptr, "rank", context->process->rank);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the rank %d to the exit command.\n", context->process->rank);
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
		result = smpd_add_command_int_arg(cmd_ptr, "code", context->process->exitcode);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the exit code to the exit command for rank %d\n", context->process->rank);
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("creating an exit command for %d:%d, exit code %d.\n",
		    context->process->rank, context->process->pid, context->process->exitcode);

		/* send the exit command */
		result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the exit command for rank %d\n", context->process->rank);
		    smpd_exit_fn("handle_launch_command");
		    return SMPD_FAIL;
		}

		/* remove the process structure from the global list */
		trailer = iter = smpd_process.process_list;
		while (iter)
		{
		    if (context->process == iter)
		    {
			if (iter == smpd_process.process_list)
			{
			    smpd_process.process_list = smpd_process.process_list->next;
			}
			else
			{
			    trailer->next = iter->next;
			}
			/* NULL the context pointer just to be sure no one uses it after it is freed. */
			switch (context->type)
			{
			case SMPD_CONTEXT_STDIN:
			    context->process->in = NULL;
			    break;
			case SMPD_CONTEXT_STDOUT:
			    context->process->out = NULL;
			    break;
			case SMPD_CONTEXT_STDERR:
			    context->process->err = NULL;
			    break;
			case SMPD_CONTEXT_PMI:
			    context->process->pmi = NULL;
			    break;
			}
			/* free the process structure */
			smpd_free_process_struct(iter);
			context->process = NULL;
			iter = NULL;
		    }
		    else
		    {
			if (trailer != iter)
			    trailer = trailer->next;
			iter = iter->next;
		    }
		}
	    }
	    else
	    {
		/* NULL the context pointer just to be sure no one uses it after it is freed. */
		switch (context->type)
		{
		case SMPD_CONTEXT_STDIN:
		    context->process->in = NULL;
		    break;
		case SMPD_CONTEXT_STDOUT:
		    context->process->out = NULL;
		    break;
		case SMPD_CONTEXT_STDERR:
		    context->process->err = NULL;
		    break;
		case SMPD_CONTEXT_PMI:
		    context->process->pmi = NULL;
		    break;
		}
	    }
	}
	if (context == smpd_process.left_context)
	    smpd_process.left_context = NULL;
	if (context == smpd_process.right_context)
	    smpd_process.right_context = NULL;
	if (context == smpd_process.parent_context)
	    smpd_process.parent_context = NULL;
	if (context == smpd_process.listener_context)
	    smpd_process.listener_context = NULL;
	if (smpd_process.closing && smpd_process.left_context == NULL && smpd_process.right_context == NULL)
	{
	    if (smpd_process.parent_context == NULL)
	    {
		if (smpd_process.listener_context)
		{
		    smpd_dbg_printf("all contexts closed, closing the listener.\n");
		    smpd_process.listener_context->state = SMPD_EXITING;
		    result = MPIDU_Sock_post_close(smpd_process.listener_context->sock);
		    if (result == MPI_SUCCESS)
		    {
			break;
		    }
		    smpd_err_printf("unable to post a close of the listener sock, error:\n%s\n",
			get_sock_error_string(result));
		}
		smpd_free_context(context);
		smpd_dbg_printf("all contexts closed, exiting state machine.\n");
		/*smpd_exit(0);*/
		smpd_exit_fn("smpd_handle_op_close");
		return SMPD_EXIT;
	    }
	    else
	    {
		/* both children are closed, send closed_request to parent */
		result = smpd_create_command("closed_request", smpd_process.id, smpd_process.parent_context->id, SMPD_FALSE, &cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to create a closed_request command for the parent context.\n");
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
		/*smpd_dbg_printf("posting write of closed_request command to parent: \"%s\"\n", cmd_ptr->cmd);*/
		result = smpd_post_write_command(smpd_process.parent_context, cmd_ptr);
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to post a write of the closed_request command to the parent context.\n");
		    smpd_exit_fn("smpd_handle_op_close");
		    return SMPD_FAIL;
		}
	    }
	}
	break;
    default:
	smpd_err_printf("sock_op_close returned while context is in state: %s\n",
	    smpd_get_state_string(context->state));
	break;
    }
    /* free the context */
    result = smpd_free_context(context);
    smpd_exit_fn("smpd_handle_op_close");
    return SMPD_SUCCESS;
}

int smpd_enter_at_state(MPIDU_Sock_set_t set, smpd_state_t state)
{
    int result;
    MPIDU_Sock_event_t event;
    smpd_context_t *context;

    smpd_enter_fn("smpd_enter_at_state");

    while (1)
    {
	event.error = MPI_SUCCESS;
	smpd_dbg_printf("sock_waiting for the next event.\n");
	result = MPIDU_Sock_wait(set, MPIDU_SOCK_INFINITE_TIME, &event);
	if (result != MPI_SUCCESS)
	{
	    /*
	    if (result == SOCK_ERR_TIMEOUT)
	    {
		smpd_err_printf("Warning: MPIDU_Sock_wait returned SOCK_ERR_TIMEOUT when infinite time was passed in.\n");
		continue;
	    }
	    */
	    if (event.error != MPI_SUCCESS)
		result = event.error;
	    smpd_err_printf("MPIDU_Sock_wait failed,\nsock error: %s\n", get_sock_error_string(result));
	    smpd_exit_fn("smpd_enter_at_state");
	    return result;
	}
	context = event.user_ptr;
	if (context == NULL)
	{
	    smpd_err_printf("MPIDU_Sock_wait returned an event with a NULL user pointer.\n");
	    if (event.error != MPI_SUCCESS)
	    {
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_FAIL;
	    }
	    continue;
	}
	switch (event.op_type)
	{
	case MPIDU_SOCK_OP_READ:
	    smpd_dbg_printf("SOCK_OP_READ\n");
	    if (event.error != MPI_SUCCESS)
	    {
		/* don't print EOF errors because they usually aren't errors */
		/*if (event.error != SOCK_EOF)*/
		{
		    /* don't print errors from the pmi context because processes that don't 
		       call PMI_Finalize will get read errors that don't need to be printed.
		       */
		    if (context->type != SMPD_CONTEXT_PMI)
		    {
			smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
		    }
		}
	    }
	    result = smpd_handle_op_read(context, &event);
	    if (result == SMPD_DBS_RETURN)
	    {
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS || event.error != MPI_SUCCESS)
	    {
		if (context->type == SMPD_CONTEXT_PARENT)
		{
		    smpd_err_printf("connection to my parent broken, aborting.\n");
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("SOCK_OP_READ failed - result = %d, closing %s context.\n", result, smpd_get_context_str(context));
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		if (result != MPI_SUCCESS)
		{
		    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_FAIL;
		}
	    }
	    break;
	case MPIDU_SOCK_OP_WRITE:
	    smpd_dbg_printf("SOCK_OP_WRITE\n");
	    if (event.error != MPI_SUCCESS)
	    {
		smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
	    }
	    result = smpd_handle_op_write(context, &event, set);
	    if (result == SMPD_DBS_RETURN)
	    {
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_SUCCESS;
	    }
	    if (result != SMPD_SUCCESS || event.error != MPI_SUCCESS)
	    {
		smpd_dbg_printf("SOCK_OP_WRITE failed, closing %s context.\n", smpd_get_context_str(context));
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		if (result != MPI_SUCCESS)
		{
		    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_FAIL;
		}
	    }
	    break;
	case MPIDU_SOCK_OP_ACCEPT:
	    smpd_dbg_printf("SOCK_OP_ACCEPT\n");
	    if (event.error != MPI_SUCCESS)
	    {
		smpd_err_printf("error listening and accepting socket: %s\n", get_sock_error_string(event.error));
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_FAIL;
	    }
	    result = smpd_handle_op_accept(context, &event, set);
	    if (result != SMPD_SUCCESS || event.error != MPI_SUCCESS)
	    {
		smpd_dbg_printf("SOCK_OP_ACCEPT failed, closing %s context.\n", smpd_get_context_str(context));
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		if (result != MPI_SUCCESS)
		{
		    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_FAIL;
		}
	    }
	    break;
	case MPIDU_SOCK_OP_CONNECT:
	    smpd_dbg_printf("SOCK_OP_CONNECT\n");
	    if (event.error != MPI_SUCCESS)
		smpd_err_printf("error: %s\n", get_sock_error_string(event.error));
	    result = smpd_handle_op_connect(context, &event);
	    if (result != SMPD_SUCCESS || event.error != MPI_SUCCESS)
	    {
		smpd_process.state_machine_ret_val = (result != SMPD_SUCCESS) ? result : event.error;
		smpd_dbg_printf("SOCK_OP_CONNECT failed, closing %s context.\n", smpd_get_context_str(context));
		context->state = SMPD_CLOSING;
		result = MPIDU_Sock_post_close(context->sock);
		if (context->connect_to)
		    smpd_post_abort_command("unable to connect to %s", context->connect_to->host);
		else
		    smpd_post_abort_command("connect failure");
		if (result != MPI_SUCCESS)
		{
		    smpd_err_printf("unable to post a close on a broken %s context.\n", smpd_get_context_str(context));
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_FAIL;
		}
	    }
	    break;
	case MPIDU_SOCK_OP_CLOSE:
	    smpd_dbg_printf("SOCK_OP_CLOSE\n");
	    if (event.error != MPI_SUCCESS)
		smpd_err_printf("error closing socket: %s\n", get_sock_error_string(event.error));
	    result = smpd_handle_op_close(context, &event);
	    if (event.error != MPI_SUCCESS)
	    {
		smpd_err_printf("unable to close the %s context.\n", smpd_get_context_str(context));
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_FAIL;
	    }
	    if (result != SMPD_SUCCESS)
	    {
		if (result == SMPD_EXIT)
		{
		    smpd_exit_fn("smpd_enter_at_state");
		    return SMPD_SUCCESS;
		}
		smpd_err_printf("unable to close the %s context.\n", smpd_get_context_str(context));
		smpd_exit_fn("smpd_enter_at_state");
		return SMPD_FAIL;
	    }
	    break;
	default:
	    smpd_dbg_printf("SOCK_OP_UNKNOWN\n");
	    smpd_err_printf("MPIDU_Sock_wait returned unknown sock event type: %d\n", event.op_type);
	    break;
	}
    }
    smpd_exit_fn("smpd_enter_at_state");
    return SMPD_SUCCESS;
}
