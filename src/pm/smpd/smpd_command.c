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

int smpd_command_destination(smpd_context_t *cmd_context, smpd_context_t **dest_context)
{
    int src, dest, level_bit;
    char temp_str[100];

    /* get the source */
    src = smpd_process.id;

    /* get the destination */
    if (!smpd_get_string_arg(cmd_context->input_str, "dest", temp_str, 100))
    {
	smpd_err_printf("unable to determine command route, no dest flag in the command.\n");
	return SMPD_FAIL;
    }
    dest = atoi(temp_str);
    if (dest < 0)
    {
	smpd_err_printf("unable to determine command route, invalid dest: %d\n", dest);
	return SMPD_FAIL;
    }

    smpd_dbg_printf("determining destination context for %d -> %d\n", src, dest);
    /* determine the route and return the context */
    if (src == dest)
    {
	*dest_context = NULL;
	smpd_dbg_printf("%d -> %d : returning NULL context\n", src, dest);
	return SMPD_SUCCESS;
    }

    if (src == 0)
    {
	/* this assumes that the root uses the left context for it's only child. */
	if (smpd_process.left_context == NULL)
	    return SMPD_FAIL;
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("%d -> %d : returning left_context\n", src, dest);
	return SMPD_SUCCESS;
    }

    if (dest < src)
    {
	if (smpd_process.parent_context == NULL)
	    return SMPD_FAIL;
	*dest_context = smpd_process.parent_context;
	smpd_dbg_printf("%d -> %d : returning parent_context: %d < %d\n", src, dest, dest, src);
	return SMPD_SUCCESS;
    }

    level_bit = 0x1 << smpd_process.level;
    if (( src ^ level_bit ) == ( dest & (level_bit-1) ))
    {
	if (smpd_process.left_context == NULL)
	    return SMPD_FAIL;
	*dest_context = smpd_process.left_context;
	smpd_dbg_printf("returning left_context\n", src, dest);
	return SMPD_SUCCESS;
    }

    if ( src == ( dest & (level_bit-1) ) )
    {
	if (smpd_process.right_context == NULL)
	    return SMPD_FAIL;
	*dest_context = smpd_process.right_context;
	smpd_dbg_printf("%d -> %d : returning right_context\n", src, dest);
	return SMPD_SUCCESS;
    }

    if (smpd_process.parent_context == NULL)
	return SMPD_FAIL;
    *dest_context = smpd_process.parent_context;
    smpd_dbg_printf("%d -> %d : returning parent_context: fall through\n", src, dest);
    return SMPD_SUCCESS;
}

int smpd_forward_command(smpd_context_t *src, smpd_context_t *dest)
{
    int result;
    dest->output_str[0] = '\0';
    strcpy(dest->output_str, src->input_str);
    result = smpd_package_command(dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to package the command to be forwarded.\n");
	return SMPD_FAIL;
    }
    result = smpd_write_command(dest);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to forward the command, write_command failed.\n");
	return SMPD_FAIL;
    }
    return SMPD_SUCCESS;
}

int smpd_post_read_command(smpd_context_t *context)
{
    int result;

    /* post a read for the next command header */
    result = sock_post_read(context->sock, context->input_cmd_hdr_str, SMPD_CMD_HDR_LENGTH, NULL);
    if (result != SOCK_SUCCESS)
    {
	smpd_err_printf("unable to post a read for the next command header, sock error:\n%s\n", get_sock_error_string(result));
	return SMPD_FAIL;
    }

    return SMPD_SUCCESS;
}

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

int smpd_package_command(smpd_context_t *context)
{
    int length;

    /* create the command header - for now it is simply the length of the command string */
    length = (int)strlen(context->output_str) + 1;
    if (length > SMPD_MAX_CMD_LENGTH)
    {
	smpd_err_printf("unable to package invalid command of length %d\n", length);
	return SMPD_FAIL;
    }
    snprintf(context->output_cmd_hdr_str, SMPD_CMD_HDR_LENGTH, "%d", length);

    return SMPD_SUCCESS;
}

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

    smpd_dbg_printf("writing command from %d to %d on sock %d: <%s>\n", smpd_process.id, context->id, sock_getid(context->sock), context->output_str);

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
