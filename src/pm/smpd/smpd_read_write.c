/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_encode_buffer(char *dest, int dest_length, const char *src, int src_length, int *num_encoded)
{
    int num_used;
    int n = 0;
    char ch;
    while (src_length && dest_length)
    {
	ch = *src;
	num_used = snprintf(dest, dest_length, "%02X", (int)*src);
	if (num_used < 0)
	{
	    *num_encoded = n;
	    return SMPD_SUCCESS;
	}
	/*smpd_dbg_printf(" %c = %c%c\n", ch, dest[0], dest[1]);*/
	dest += num_used;
	dest_length -= num_used;
	src++;
	n++;
	src_length--;
    }
    *num_encoded = n;
    return SMPD_SUCCESS;
}

int smpd_decode_buffer(const char *str, char *dest, int length, int *num_decoded)
{
    char hex[3];
    int value;
    int n = 0;

    if (length < 1)
    {
	return SMPD_FAIL;
    }

    hex[2] = '\0';
    while (*str != '\0' && length)
    {
	hex[0] = *str;
	str++;
	hex[1] = *str;
	str++;
	sscanf(hex, "%X", &value);
	*dest = (char)value;
	/*smpd_dbg_printf(" %s = %c\n", hex, *dest);*/
	dest++;
	length--;
	n++;
    }
    *num_decoded = n;
    return SMPD_SUCCESS;
}

int smpd_read(MPIDU_Sock_t sock, void *buf, MPIDU_Sock_size_t len)
{
    int result;
    MPIU_Size_t num_read;

    smpd_enter_fn("smpd_read");

    smpd_dbg_printf("reading %d bytes from sock %d\n", len, MPIDU_Sock_get_sock_id(sock));

    while (len)
    {
	/* aggressively write */
	result = MPIDU_Sock_read(sock, buf, len, &num_read);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("Unable to read %d bytes,\nsock error: %s\n", len, get_sock_error_string(result));
	    smpd_exit_fn("smpd_read");
	    return SMPD_FAIL;
	}
	if (num_read == len)
	{
	    smpd_exit_fn("smpd_write");
	    return SMPD_SUCCESS;
	}

	if (num_read == 0)
	{
#ifdef HAVE_WINDOWS_H
	    Sleep(1);
#elif defined(HAVE_USLEEP)
	    usleep(1000);
#endif
	}
	else
	{
	    buf = (char*)buf + num_read;
	    len -= num_read;
	}
    }

    smpd_exit_fn("smpd_read");
    return SMPD_SUCCESS;
}

int smpd_write(MPIDU_Sock_t sock, void *buf, MPIDU_Sock_size_t len)
{
    int result;
    MPIU_Size_t num_written;

    smpd_enter_fn("smpd_write");

    smpd_dbg_printf("writing %d bytes to sock %d\n", len, MPIDU_Sock_get_sock_id(sock));

    while (len)
    {
	/* aggressively write */
	result = MPIDU_Sock_write(sock, buf, len, &num_written);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("Unable to write %d bytes,\nsock error: %s\n",
			    len, get_sock_error_string(result));
	    smpd_exit_fn("smpd_write");
	    return SMPD_FAIL;
	}
	if (num_written == len)
	{
	    smpd_exit_fn("smpd_write");
	    return SMPD_SUCCESS;
	}

	if (num_written == 0)
	{
	    smpd_dbg_printf("0 bytes written.\n");
#ifdef HAVE_WINDOWS_H
	    Sleep(1);
#elif defined(HAVE_USLEEP)
	    usleep(1000);
#endif
	}
	else
	{
	    smpd_dbg_printf("wrote %d bytes\n", num_written);
	    buf = (char*)buf + num_written;
	    len -= num_written;
	}
    }

    smpd_exit_fn("smpd_write");
    return SMPD_SUCCESS;
}

int smpd_write_string(MPIDU_Sock_t sock, char *str)
{
    int result;
    MPIU_Size_t len, num_written;

    smpd_enter_fn("smpd_write_string");

    smpd_dbg_printf("writing string on sock %d: \"%s\"\n", MPIDU_Sock_get_sock_id(sock), str);

    len = (MPIU_Size_t)strlen(str)+1;

    while (len)
    {
	/* aggressively write string */
	result = MPIDU_Sock_write(sock, str, len, &num_written);
	if (result != MPI_SUCCESS)
	{
	    smpd_err_printf("Unable to write string of length %d,\nsock error: %s\n", len, get_sock_error_string(result));
	    smpd_exit_fn("smpd_write_string");
	    return SMPD_FAIL;
	}
	if (num_written == len)
	{
	    smpd_exit_fn("smpd_write_string");
	    return SMPD_SUCCESS;
	}

	/* post a write for whatever is left of the string */
	str += num_written;
	len -= num_written;
    }

    smpd_exit_fn("smpd_write_string");
    return SMPD_SUCCESS;
}

static int read_string(MPIDU_Sock_t sock, char *str, int maxlen)
{
    char ch;
    int result;
    MPIU_Size_t num_bytes;
    int total = 0;

    if (maxlen < 1)
	return 0;
    result = MPIDU_Sock_read(sock, &ch, 1, &num_bytes);
    while (result == MPI_SUCCESS)
    {
	if (num_bytes == 0)
	    return total;
	total++;
	*str = ch;
	str++;
	if (ch == '\0' || total >= maxlen)
	    return total;
	result = MPIDU_Sock_read(sock, &ch, 1, &num_bytes);
    }
    smpd_err_printf("Unable to read a string,\nsock error: %s\n", get_sock_error_string(result));
    return -1;
}

static int chew_up_string(MPIDU_Sock_t sock)
{
    char ch;
    int result;

    result = smpd_read_string(sock, &ch, 1);
    while (result == SMPD_SUCCESS)
    {
	if (ch == '\0')
	    return SMPD_SUCCESS;
	result = smpd_read_string(sock, &ch, 1);
    }
    smpd_err_printf("Unable to read a string,\nsock error: %s\n", get_sock_error_string(result));
    return SMPD_FAIL;
}

int smpd_read_string(MPIDU_Sock_t sock, char *str, int maxlen)
{
    int num_bytes;
    char *str_orig;

    smpd_enter_fn("smpd_read_string");

    str_orig = str;

    if (maxlen == 0)
    {
	smpd_dbg_printf("zero length read string request on sock %d\n", MPIDU_Sock_get_sock_id(sock));
	smpd_exit_fn("smpd_read_string");
	return SMPD_SUCCESS;
    }

    for (;;)
    {
	num_bytes = read_string(sock, str, maxlen);
	if (num_bytes == -1)
	{
	    smpd_exit_fn("smpd_read_string");
	    return SMPD_FAIL;
	}
	if (num_bytes > 0 && str[num_bytes-1] == '\0')
	{
	    smpd_dbg_printf("received string on sock %d: \"%s\"\n", MPIDU_Sock_get_sock_id(sock), str_orig);
	    smpd_exit_fn("smpd_read_string");
	    return SMPD_SUCCESS;
	}
	if (num_bytes == maxlen)
	{
	    /* received truncated string */
	    str[num_bytes-1] = '\0';
	    chew_up_string(sock);
	    smpd_dbg_printf("received truncated string on sock %d: \"%s\"\n", MPIDU_Sock_get_sock_id(sock), str_orig);
	    smpd_exit_fn("smpd_read_string");
	    return SMPD_SUCCESS;
	}
	str += num_bytes;
	maxlen -= num_bytes;
    }
    /*
    smpd_exit_fn("smpd_read_string");
    return SMPD_FAIL;
    */
}
