/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_write_string(sock_set_t set, sock_t sock, char *str)
{
    int result;
    sock_event_t event;
    sock_size_t len, num_written;

    smpd_dbg_printf("entering smpd_write_string.\n");

    smpd_dbg_printf("writing string on sock %d: '%s'\n", sock_getid(sock), str);

    len = (sock_size_t)strlen(str)+1;

    /* aggressively write string */
    result = sock_write(sock, str, len, &num_written);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("Unable to write string of length %d, sock error:\n%s\n", len, get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_write_string.\n");
	return SMPD_FAIL;
    }
    if (num_written == len)
    {
	smpd_dbg_printf("exiting smpd_write_string.\n");
	return SMPD_SUCCESS;
    }

    /* post a write for whatever is left of the string */
    str += num_written;
    len -= num_written;
    result = sock_post_write(sock, str, len, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("Unable to post a write for string of length %d, sock error:\n%s\n", len, get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_write_string.\n");
	return SMPD_FAIL;
    }

    /* wait for the write to finish */
    /*smpd_dbg_printf("smpd_write_string calling sock_wait.\n");*/
    result = sock_wait(set, SOCK_INFINITE_TIME, &event);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	smpd_dbg_printf("exiting smpd_write_string.\n");
	return SMPD_FAIL;
    }
    switch (event.op_type)
    {
    case SOCK_OP_READ:
	smpd_err_printf("sock_wait returned SOCK_OP_READ unexpectedly.\n");
	if (event.error != SOCK_SUCCESS)
	    smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	break;
    case SOCK_OP_WRITE:
	if (event.error != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    break;
	}
	/*smpd_dbg_printf("wrote: %s\n", str);*/
	smpd_dbg_printf("exiting smpd_write_string.\n");
	return SMPD_SUCCESS;
	break;
    case SOCK_OP_ACCEPT:
	smpd_err_printf("sock_wait returned SOCK_OP_ACCEPT unexpectedly.\n");
	if (event.error != SOCK_SUCCESS)
	    smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	break;
    case SOCK_OP_CONNECT:
	smpd_err_printf("sock_wait returned SOCK_OP_CONNECT unexpectedly.\n");
	if (event.error != SOCK_SUCCESS)
	    smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	break;
    case SOCK_OP_CLOSE:
	smpd_err_printf("sock_wait returned SOCK_OP_CLOSE unexpectedly.\n");
	if (event.error != SOCK_SUCCESS)
	    smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	break;
    default:
	smpd_err_printf("sock_wait returned unknown event type: %d\n", event.op_type);
	break;
    }
    smpd_dbg_printf("exiting smpd_write_string.\n");
    return SMPD_FAIL;
}

static int read_string(sock_t sock, char *str, int maxlen)
{
    char ch;
    int result;
    int num_bytes;
    int total = 0;

    if (maxlen < 1)
	return 0;
    result = sock_read(sock, &ch, 1, &num_bytes);
    while (result == SOCK_SUCCESS)
    {
	if (num_bytes == 0)
	    return total;
	total++;
	*str = ch;
	str++;
	if (ch == '\0' || total >= maxlen)
	    return total;
	result = sock_read(sock, &ch, 1, &num_bytes);
    }
    smpd_err_printf("Unable to read a string, sock error:\n%s\n", get_sock_error_string(result));
    return -1;
}

static int chew_up_string(sock_set_t set, sock_t sock)
{
    char ch;
    int result;

    result = smpd_read_string(set, sock, &ch, 1);
    while (result == SMPD_SUCCESS)
    {
	if (ch == '\0')
	    return SMPD_SUCCESS;
    }
    smpd_err_printf("Unable to read a string, sock error:\n%s\n", get_sock_error_string(result));
    return SMPD_FAIL;
}

int smpd_read_string(sock_set_t set, sock_t sock, char *str, int maxlen)
{
    int result;
    int num_bytes;
    sock_event_t event;

    smpd_dbg_printf("entering smpd_read_string.\n");

    if (maxlen == 0)
    {
	smpd_dbg_printf("zero length read string request on sock %d\n", sock_getid(sock));
	smpd_dbg_printf("exiting smpd_read_string.\n");
	return SMPD_SUCCESS;
    }

    while (1)
    {
	num_bytes = read_string(sock, str, maxlen);
	if (num_bytes == -1)
	{
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	}
	if (num_bytes > 0 && str[num_bytes-1] == '\0')
	{
	    smpd_dbg_printf("received string on sock %d: '%s'\n", sock_getid(sock), str);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_SUCCESS;
	}
	if (num_bytes == maxlen)
	{
	    /* received truncated string */
	    str[num_bytes-1] = '\0';
	    chew_up_string(set, sock);
	    smpd_dbg_printf("received string on sock %d: '%s'\n", sock_getid(sock), str);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_SUCCESS;
	}
	str += num_bytes;
	maxlen -= num_bytes;
	result = sock_post_read(sock, str, 1, NULL);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("Unable to read a string, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	}
	/*smpd_dbg_printf("smpd_read_string calling sock_wait.\n");*/
	result = sock_wait(set, SOCK_INFINITE_TIME, &event);
	if (result != SOCK_SUCCESS)
	{
	    smpd_err_printf("sock_wait failed, sock error:\n%s\n", get_sock_error_string(result));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	}
	switch (event.op_type)
	{
	case SOCK_OP_READ:
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    break;
	case SOCK_OP_WRITE:
	    smpd_err_printf("sock_wait returned SOCK_OP_WRITE unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	    break;
	case SOCK_OP_ACCEPT:
	    smpd_err_printf("sock_wait returned SOCK_OP_ACCEPT unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	    break;
	case SOCK_OP_CONNECT:
	    smpd_err_printf("sock_wait returned SOCK_OP_CONNECT unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	    break;
	case SOCK_OP_CLOSE:
	    smpd_err_printf("sock_wait returned SOCK_OP_CLOSE unexpectedly.\n");
	    if (event.error != SOCK_SUCCESS)
		smpd_err_printf("sock error: %s\n", get_sock_error_string(event.error));
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	    break;
	default:
	    smpd_err_printf("sock_wait returned unknown event type: %d\n", event.op_type);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_FAIL;
	    break;
	}
	if (*str == '\0')
	{
	    smpd_dbg_printf("received string on sock %d: '%s'\n", sock_getid(sock), str);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_SUCCESS;
	}
	str++;
	maxlen--;
    }
    smpd_dbg_printf("exiting smpd_read_string.\n");
    return SMPD_FAIL;
}
