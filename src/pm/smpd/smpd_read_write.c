/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_write_string(sock_t sock, char *str)
{
    int result;
    sock_size_t len, num_written;

    smpd_dbg_printf("entering smpd_write_string.\n");

    smpd_dbg_printf("writing string on sock %d: \"%s\"\n", sock_getid(sock), str);

    len = (sock_size_t)strlen(str)+1;

    while (len)
    {
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

static int chew_up_string(sock_t sock)
{
    char ch;
    int result;

    result = smpd_read_string(sock, &ch, 1);
    while (result == SMPD_SUCCESS)
    {
	if (ch == '\0')
	    return SMPD_SUCCESS;
	smpd_read_string(sock, &ch, 1);
    }
    smpd_err_printf("Unable to read a string, sock error:\n%s\n", get_sock_error_string(result));
    return SMPD_FAIL;
}

int smpd_read_string(sock_t sock, char *str, int maxlen)
{
    int num_bytes;
    char *str_orig;

    smpd_dbg_printf("entering smpd_read_string.\n");

    str_orig = str;

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
	    smpd_dbg_printf("received string on sock %d: \"%s\"\n", sock_getid(sock), str_orig);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_SUCCESS;
	}
	if (num_bytes == maxlen)
	{
	    /* received truncated string */
	    str[num_bytes-1] = '\0';
	    chew_up_string(sock);
	    smpd_dbg_printf("received truncated string on sock %d: \"%s\"\n", sock_getid(sock), str_orig);
	    smpd_dbg_printf("exiting smpd_read_string.\n");
	    return SMPD_SUCCESS;
	}
	str += num_bytes;
	maxlen -= num_bytes;
    }
    smpd_dbg_printf("exiting smpd_read_string.\n");
    return SMPD_FAIL;
}
