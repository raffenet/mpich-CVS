/* -*- Mode: C; c-basic-offset:4 ; -*-
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#define MPIU_STR_QUOTE_CHAR     '\"'
#define MPIU_STR_DELIM_CHAR     '='
#define MPIU_STR_DELIM_STR      "="
#define MPIU_STR_ESCAPE_CHAR    '\\'
#define MPIU_STR_HIDE_CHAR      '*'
#define MPIU_STR_SEPAR_CHAR     ' '
#define MPIU_STR_SEPAR_STR      " "

#define MPIU_STR_SUCCESS    0
#define MPIU_STR_FAIL       1
#define MPIU_STR_TRUNCATED -1

#define MPIU_TRUE  1
#define MPIU_FALSE 0

static int encode_buffer(char *dest, int dest_length, const char *src, int src_length, int *num_encoded)
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
	    return MPIU_STR_TRUNCATED;
	}
	/*MPIU_DBG_PRINTF((" %c = %c%c\n", ch, dest[0], dest[1]));*/
	dest += num_used;
	dest_length -= num_used;
	src++;
	n++;
	src_length--;
    }
    *num_encoded = n;
    return src_length ? MPIU_STR_TRUNCATED : MPIU_STR_SUCCESS;
}

static int decode_buffer(const char *str, char *dest, int length, int *num_decoded)
{
    char hex[3];
    int value;
    int n = 0;

    if (str == NULL || dest == NULL || num_decoded == NULL)
	return MPIU_STR_FAIL;
    if (length < 1)
    {
	*num_decoded = 0;
	if (*str == '\0')
	    return MPIU_STR_SUCCESS;
	return MPIU_STR_TRUNCATED;
    }
    hex[2] = '\0';
    while (*str != '\0' && *str != ' ' && length)
    {
	hex[0] = *str;
	str++;
	hex[1] = *str;
	str++;
	sscanf(hex, "%X", &value);
	*dest = (char)value;
	/*MPIU_DBG_PRINTF((" %s = %c\n", hex, *dest));*/
	dest++;
	n++;
	length--;
    }
    *num_decoded = n;
    if (length == 0)
    {
	if (*str != '\0' && *str != ' ')
	    return MPIU_STR_TRUNCATED;
    }
    return MPIU_STR_SUCCESS;
}

static const char * first_token(const char *str)
{
    if (str == NULL)
	return NULL;
    while (isspace(*str))
	str++;
    if (*str == '\0')
	return NULL;
    return str;
}

static const char * next_token(const char *str)
{
    if (str == NULL)
	return NULL;
    str = first_token(str);
    if (str == NULL)
	return NULL;
    if (*str == MPIU_STR_QUOTE_CHAR)
    {
	/* move over string */
	str++; /* move over the first quote */
	if (*str == '\0')
	    return NULL;
	while (*str != MPIU_STR_QUOTE_CHAR)
	{
	    /* move until the last quote, ignoring escaped quotes */
	    if (*str == MPIU_STR_ESCAPE_CHAR)
	    {
		str++;
		if (*str == MPIU_STR_QUOTE_CHAR)
		    str++;
	    }
	    else
	    {
		str++;
	    }
	    if (*str == '\0')
		return NULL;
	}
	str++; /* move over the last quote */
    }
    else
    {
	if (*str == MPIU_STR_DELIM_CHAR)
	{
	    /* move over the DELIM token */
	    str++;
	}
	else
	{
	    /* move over literal */
	    while (!isspace(*str) && *str != MPIU_STR_DELIM_CHAR && *str != '\0')
		str++;
	}
    }
    return first_token(str);
}

static int compare_token(const char *token, const char *str)
{
    if (token == NULL || str == NULL)
	return -1;

    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	/* compare quoted strings */
	token++; /* move over the first quote */
	/* compare characters until reaching the end of the string or the end quote character */
	do
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		{
		    /* move over the escape character if the next character is a quote character */
		    token++;
		}
		if (*token != *str)
		    break;
	    }
	    else
	    {
		if (*token != *str || *token == MPIU_STR_QUOTE_CHAR)
		    break;
	    }
	    if (*str == '\0')
		break;
	    token++;
	    str++;
	} while (1);
	if (*str == '\0' && *token == MPIU_STR_QUOTE_CHAR)
	    return 0;
	if (*token == MPIU_STR_QUOTE_CHAR)
	    return 1;
	if (*str < *token)
	    return -1;
	return 1;
    }

    /* compare DELIM token */
    if (*token == MPIU_STR_DELIM_CHAR)
    {
	if (*str == MPIU_STR_DELIM_CHAR)
	{
	    str++;
	    if (*str == '\0')
		return 0;
	    return 1;
	}
	if (*token < *str)
	    return -1;
	return 1;
    }

    /* compare literals */
    while (*token == *str && *str != '\0' && *token != MPIU_STR_DELIM_CHAR && !isspace(*token))
    {
	token++;
	str++;
    }
    if ( (*str == '\0') && (*token == MPIU_STR_DELIM_CHAR || isspace(*token) || *token == '\0') )
	return 0;
    if (*token == MPIU_STR_DELIM_CHAR || isspace(*token) || *token < *str)
	return -1;
    return 1;
}


static int token_copy(const char *token, char *str, int maxlen)
{
    /* check parameters */
    if (token == NULL || str == NULL)
	return MPIU_STR_FAIL;

    /* check special buffer lengths */
    if (maxlen < 1)
	return MPIU_STR_FAIL;
    if (maxlen == 1)
    {
	*str = '\0';
	return (str[0] == '\0') ? MPIU_STR_SUCCESS : MPIU_STR_TRUNCATED;
    }

    /* cosy up to the token */
    token = first_token(token);
    if (token == NULL)
    {
	*str = '\0';
	return MPIU_STR_SUCCESS;
    }

    if (*token == MPIU_STR_DELIM_CHAR)
    {
	/* copy the special deliminator token */
	str[0] = MPIU_STR_DELIM_CHAR;
	str[1] = '\0';
	return MPIU_STR_SUCCESS;
    }

    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	/* quoted copy */
	token++; /* move over the first quote */
	do
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		    token++;
		*str = *token;
	    }
	    else
	    {
		if (*token == MPIU_STR_QUOTE_CHAR)
		{
		    *str = '\0';
		    return MPIU_STR_SUCCESS;
		}
		*str = *token;
	    }
	    str++;
	    token++;
	    maxlen--;
	} while (maxlen);
	/* we've run out of destination characters so back up and null terminate the string */
	str--;
	*str = '\0';
	return MPIU_STR_TRUNCATED;
    }

    /* literal copy */
    while (*token != MPIU_STR_DELIM_CHAR && !isspace(*token) && *token != '\0' && maxlen)
    {
	*str = *token;
	str++;
	token++;
	maxlen--;
    }
    if (maxlen)
    {
	*str = '\0';
	return MPIU_STR_SUCCESS;
    }
    str--;
    *str = '\0';
    return MPIU_STR_TRUNCATED;
}

static void token_hide(char *token)
{
    /* check parameters */
    if (token == NULL)
	return;

    /* cosy up to the token */
    token = (char*)first_token(token);
    if (token == NULL)
	return;

    if (*token == MPIU_STR_DELIM_CHAR)
    {
	*token = MPIU_STR_HIDE_CHAR;
	return;
    }

    /* quoted */
    if (*token == MPIU_STR_QUOTE_CHAR)
    {
	*token = MPIU_STR_HIDE_CHAR;
	token++; /* move over the first quote */
	while (*token != '\0')
	{
	    if (*token == MPIU_STR_ESCAPE_CHAR)
	    {
		if (*(token+1) == MPIU_STR_QUOTE_CHAR)
		{
		    *token = MPIU_STR_HIDE_CHAR;
		    token++;
		}
		*token = MPIU_STR_HIDE_CHAR;
	    }
	    else
	    {
		if (*token == MPIU_STR_QUOTE_CHAR)
		{
		    *token = MPIU_STR_HIDE_CHAR;
		    return;
		}
		*token = MPIU_STR_HIDE_CHAR;
	    }
	    token++;
	}
	return;
    }

    /* literal */
    while (*token != MPIU_STR_DELIM_CHAR && !isspace(*token) && *token != '\0')
    {
	*token = MPIU_STR_HIDE_CHAR;
	token++;
    }
}

int MPIU_Str_get_string_arg(const char *str, const char *flag, char *val, int maxlen)
{
    if (maxlen < 1)
	return MPIU_STR_FAIL;

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
	return MPIU_STR_FAIL;

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		    return MPIU_STR_FAIL;
		return token_copy(str, val, maxlen);
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    return MPIU_STR_FAIL;
}

int MPIU_Str_get_binary_arg(const char *str, const char *flag, char *buffer, int maxlen)
{
    int num_decoded;
    if (maxlen < 1)
	return MPIU_STR_FAIL;

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
	return MPIU_STR_FAIL;

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		    return MPIU_STR_FAIL;
		return decode_buffer(str, buffer, maxlen, &num_decoded);
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    return MPIU_STR_FAIL;
}
int MPIU_Str_hide_string_arg(char *str, const char *flag)
{
    /* line up with the first token */
    str = (char*)first_token(str);
    if (str == NULL)
	return MPIU_TRUE;

    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = (char*)next_token(str);
	    if (compare_token(str, MPIU_STR_DELIM_STR) == 0)
	    {
		str = (char*)next_token(str);
		if (str == NULL)
		    return MPIU_TRUE;
		token_hide(str);
		return MPIU_TRUE;
	    }
	}
	else
	{
	    str = (char*)next_token(str);
	}
    } while (str);
    return MPIU_FALSE;
}

int MPIU_Str_get_int_arg(const char *str, const char *flag, int *val_ptr)
{
    int result;
    char int_str[12];

    result = MPIU_Str_get_string_arg(str, flag, int_str, 12);
    if (result == MPIU_STR_SUCCESS)
    {
	*val_ptr = atoi(int_str);
	return MPIU_STR_SUCCESS;
    }
    return result;
}

/* quoted_printf does not NULL terminate the string if maxlen is reached */
static int quoted_printf(char *str, int maxlen, const char *val)
{
    int count = 0;
    if (maxlen < 1)
	return 0;
    *str = MPIU_STR_QUOTE_CHAR;
    str++;
    maxlen--;
    count++;
    while (maxlen)
    {
	if (*val == '\0')
	    break;
	if (*val == MPIU_STR_QUOTE_CHAR)
	{
	    *str = MPIU_STR_ESCAPE_CHAR;
	    str++;
	    maxlen--;
	    count++;
	    if (maxlen == 0)
		return count;
	}
	*str = *val;
	str++;
	maxlen--;
	count++;
	val++;
    }
    if (maxlen)
    {
	*str = MPIU_STR_QUOTE_CHAR;
	str++;
	maxlen--;
	count++;
	if (maxlen == 0)
	    return count;
	*str = '\0';
    }
    return count;
}

int MPIU_Str_add_string(char *str, int maxlen, const char *val)
{
    int num_chars;

    if (strchr(val, ' ') || strchr(val, MPIU_STR_QUOTE_CHAR) || strchr(val, MPIU_STR_DELIM_CHAR))
    {
	num_chars = quoted_printf(str, maxlen, val);
	if (num_chars == maxlen)
	{
	    str[maxlen - 1] = '\0';
	    return -1;
	}
	if (num_chars < maxlen - 1)
	{
	    str[num_chars] = ' ';
	    str[num_chars+1] = '\0';
	    num_chars++;
	}
	else
	{
	    str[num_chars] = '\0';
	}
    }
    else
    {
	num_chars = snprintf(str, maxlen, "%s ", val);
    }
    return num_chars;
}

const char * MPIU_Str_get_string(const char *str, char *val, int maxlen, int *num_chars)
{
    int result;

    if (maxlen < 1)
    {
	*num_chars = 0;
	return NULL;
    }

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
    {
	*num_chars = 0;
	return NULL;
    }

    /* copy the token */
    result = token_copy(str, val, maxlen);
    if (result == MPIU_STR_SUCCESS)
	*num_chars = (int)strlen(val);
    else if (result == MPIU_STR_TRUNCATED)
	*num_chars = -1;
    else
    {
	*num_chars = -2;
	return NULL;
    }

    /* move to the next token */
    str = next_token(str);

    return str;
}

int MPIU_Str_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *flag, const char *val)
{
    int num_chars;

    if (*maxlen_ptr < 1)
	return MPIU_STR_FAIL;

    /* add the flag */
    if (strstr(flag, " ") || strstr(flag, MPIU_STR_DELIM_STR) || flag[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, flag);
    }
    else
    {
	num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s", flag);
    }
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 1)
    {
	(*str_ptr)[num_chars-1] = '\0';
	MPIU_DBG_PRINTF(("partial argument added to string: '%s'\n", *str_ptr));
	return MPIU_STR_FAIL;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = MPIU_STR_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
    if (strstr(val, " ") || strstr(val, MPIU_STR_DELIM_STR) || val[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, val);
    }
    else
    {
	num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s", val);
    }
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 2)
    {
	*str_ptr = *str_ptr - 1;
	**str_ptr = '\0';
	MPIU_DBG_PRINTF(("partial argument added to string: '%s'\n", *str_ptr));
	return MPIU_STR_FAIL;
    }
    
    /* add the trailing space */
    **str_ptr = ' ';
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    return MPIU_STR_SUCCESS;
}

int MPIU_Str_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *flag, int val)
{
    char val_str[12];
    sprintf(val_str, "%d", val);
    return MPIU_Str_add_string_arg(str_ptr, maxlen_ptr, flag, val_str);
}

int MPIU_Str_add_binary_arg(char **str_ptr, int *maxlen_ptr, const char *flag, const char *buffer, int length)
{
    int result;
    int num_chars;

    if (*maxlen_ptr < 1)
	return MPIU_STR_FAIL;

    /* add the flag */
    if (strstr(flag, " ") || strstr(flag, MPIU_STR_DELIM_STR) || flag[0] == MPIU_STR_QUOTE_CHAR)
    {
	num_chars = quoted_printf(*str_ptr, *maxlen_ptr, flag);
    }
    else
    {
	num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s", flag);
    }
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 1)
    {
	(*str_ptr)[num_chars-1] = '\0';
	MPIU_DBG_PRINTF(("partial argument added to string: '%s'\n", *str_ptr));
	return MPIU_STR_FAIL;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = MPIU_STR_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
    result = encode_buffer(*str_ptr, *maxlen_ptr, buffer, length, &num_chars);
    if (result != MPIU_STR_SUCCESS)
    {
	return result;
    }
    num_chars = num_chars * 2; /* the encoding function turns one source character into two destination characters */
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    if (*maxlen_ptr < 2)
    {
	*str_ptr = *str_ptr - 1;
	**str_ptr = '\0';
	MPIU_DBG_PRINTF(("partial argument added to string: '%s'\n", *str_ptr));
	return MPIU_STR_FAIL;
    }
    
    /* add the trailing space */
    **str_ptr = ' ';
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    return MPIU_STR_SUCCESS;
}
