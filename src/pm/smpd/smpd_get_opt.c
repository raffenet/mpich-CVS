/* -*- Mode: C; c-basic-offset:4 ; -*-
 *
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "smpd.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

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
    if (*str == SMPD_QUOTE_CHAR)
    {
	/* move over string */
	str++; /* move over the first quote */
	if (*str == '\0')
	    return NULL;
	while (*str != SMPD_QUOTE_CHAR)
	{
	    /* move until the last quote, ignoring escaped quotes */
	    if (*str == SMPD_ESCAPE_CHAR)
	    {
		str++;
		if (*str == SMPD_QUOTE_CHAR)
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
	if (*str == SMPD_DELIM_CHAR)
	{
	    /* move over the DELIM token */
	    str++;
	}
	else
	{
	    /* move over literal */
	    while (!isspace(*str) && *str != SMPD_DELIM_CHAR && *str != '\0')
		str++;
	}
    }
    return first_token(str);
}

static int compare_token(const char *token, const char *str)
{
    if (token == NULL || str == NULL)
	return -1;

    if (*token == SMPD_QUOTE_CHAR)
    {
	/* compare quoted strings */
	token++; /* move over the first quote */
	/* compare characters until reaching the end of the string or the end quote character */
	do
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		{
		    /* move over the escape character if the next character is a quote character */
		    token++;
		}
		if (*token != *str)
		    break;
	    }
	    else
	    {
		if (*token != *str || *token == SMPD_QUOTE_CHAR)
		    break;
	    }
	    if (*str == '\0')
		break;
	    token++;
	    str++;
	} while (1);
	if (*str == '\0' && *token == SMPD_QUOTE_CHAR)
	    return 0;
	if (*token == SMPD_QUOTE_CHAR)
	    return 1;
	if (*str < *token)
	    return -1;
	return 1;
    }

    /* compare DELIM token */
    if (*token == SMPD_DELIM_CHAR)
    {
	if (*str == SMPD_DELIM_CHAR)
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
    while (*token == *str && *str != '\0' && *token != SMPD_DELIM_CHAR && !isspace(*token))
    {
	token++;
	str++;
    }
    if ( (*str == '\0') && (*token == SMPD_DELIM_CHAR || isspace(*token) || *token == '\0') )
	return 0;
    if (*token == SMPD_DELIM_CHAR || isspace(*token) || *token < *str)
	return -1;
    return 1;
}

static void token_copy(const char *token, char *str, int maxlen)
{
    /* check parameters */
    if (token == NULL || str == NULL)
	return;

    /* check special buffer lengths */
    if (maxlen < 1)
	return;
    if (maxlen == 1)
    {
	*str = '\0';
	return;
    }

    /* cosy up to the token */
    token = first_token(token);
    if (token == NULL)
	return;

    if (*token == SMPD_DELIM_CHAR)
    {
	/* copy the special deliminator token */
	str[0] = SMPD_DELIM_CHAR;
	str[1] = '\0';
	return;
    }

    if (*token == SMPD_QUOTE_CHAR)
    {
	/* quoted copy */
	token++; /* move over the first quote */
	do
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		    token++;
		*str = *token;
	    }
	    else
	    {
		if (*token == SMPD_QUOTE_CHAR)
		{
		    *str = '\0';
		    return;
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
	return;
    }

    /* literal copy */
    while (*token != SMPD_DELIM_CHAR && !isspace(*token) && *token != '\0' && maxlen)
    {
	*str = *token;
	str++;
	token++;
	maxlen--;
    }
    if (maxlen)
	*str = '\0';
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

    if (*token == SMPD_DELIM_CHAR)
    {
	*token = SMPD_HIDE_CHAR;
	return;
    }

    /* quoted */
    if (*token == SMPD_QUOTE_CHAR)
    {
	*token = SMPD_HIDE_CHAR;
	token++; /* move over the first quote */
	while (*token != '\0')
	{
	    if (*token == SMPD_ESCAPE_CHAR)
	    {
		if (*(token+1) == SMPD_QUOTE_CHAR)
		{
		    *token = SMPD_HIDE_CHAR;
		    token++;
		}
		*token = SMPD_HIDE_CHAR;
	    }
	    else
	    {
		if (*token == SMPD_QUOTE_CHAR)
		{
		    *token = SMPD_HIDE_CHAR;
		    return;
		}
		*token = SMPD_HIDE_CHAR;
	    }
	    token++;
	}
	return;
    }

    /* literal */
    while (*token != SMPD_DELIM_CHAR && !isspace(*token) && *token != '\0')
    {
	*token = SMPD_HIDE_CHAR;
	token++;
    }
}

int smpd_get_string_arg(const char *str, const char *flag, char *val, int maxlen)
{
    if (maxlen < 1)
	return SMPD_FALSE;

    /* line up with the first token */
    str = first_token(str);
    if (str == NULL)
	return SMPD_FALSE;

    /* This loop will match the first instance of "flag = value" in the string. */
    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = next_token(str);
	    if (compare_token(str, SMPD_DELIM_STR) == 0)
	    {
		str = next_token(str);
		if (str == NULL)
		    return SMPD_FALSE;
		token_copy(str, val, maxlen);
		return SMPD_TRUE;
	    }
	}
	else
	{
	    str = next_token(str);
	}
    } while (str);
    return SMPD_FALSE;
}

int smpd_hide_string_arg(char *str, const char *flag)
{
    /* line up with the first token */
    str = (char*)first_token(str);
    if (str == NULL)
	return SMPD_SUCCESS;

    do
    {
	if (compare_token(str, flag) == 0)
	{
	    str = (char*)next_token(str);
	    if (compare_token(str, SMPD_DELIM_STR) == 0)
	    {
		str = (char*)next_token(str);
		if (str == NULL)
		    return SMPD_SUCCESS;
		token_hide(str);
		return SMPD_SUCCESS;
	    }
	}
	else
	{
	    str = (char*)next_token(str);
	}
    } while (str);
    return SMPD_SUCCESS;
}

int smpd_get_int_arg(const char *str, const char *flag, int *val_ptr)
{
    char int_str[12];

    if (smpd_get_string_arg(str, flag, int_str, 12))
    {
	*val_ptr = atoi(int_str);
	return SMPD_TRUE;
    }
    return SMPD_FALSE;
}

static int quoted_printf(char *str, int maxlen, const char *val)
{
    int count = 0;
    if (maxlen < 1)
	return 0;
    *str = SMPD_QUOTE_CHAR;
    str++;
    maxlen--;
    count++;
    while (maxlen)
    {
	if (*val == '\0')
	    break;
	if (*val == SMPD_QUOTE_CHAR)
	{
	    *str = SMPD_ESCAPE_CHAR;
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
	*str = SMPD_QUOTE_CHAR;
	str++;
	maxlen--;
	count++;
	if (maxlen == 0)
	    return count;
	*str = '\0';
    }
    return count;
}

int smpd_add_string(char *str, int maxlen, const char *val)
{
    int num_chars;

    if (strstr(val, " ") || val[0] == SMPD_QUOTE_CHAR)
    {
	num_chars = quoted_printf(str, maxlen, val);
	if (num_chars < maxlen)
	{
	    str[num_chars] = ' ';
	    str[num_chars+1] = '\0';
	}
	num_chars++;
    }
    else
    {
	num_chars = snprintf(str, maxlen, "%s ", val);
    }
    return num_chars;
}

const char * smpd_get_string(const char *str, char *val, int maxlen, int *num_chars)
{
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
    token_copy(str, val, maxlen);
    *num_chars = (int)strlen(val);

    /* move to the next token */
    str = next_token(str);

    return str;
}

int smpd_add_string_arg(char **str_ptr, int *maxlen_ptr, const char *flag, const char *val)
{
    int num_chars;

    if (*maxlen_ptr < 1)
	return SMPD_FAIL;

    /* add the flag */
    if (strstr(flag, " ") || strstr(flag, SMPD_DELIM_STR) || flag[0] == SMPD_QUOTE_CHAR)
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
	smpd_dbg_printf("partial argument added to string: '%s'\n", *str_ptr);
	return SMPD_FAIL;
    }
    *str_ptr = *str_ptr + num_chars;

    /* add the deliminator character */
    **str_ptr = SMPD_DELIM_CHAR;
    *str_ptr = *str_ptr + 1;
    *maxlen_ptr = *maxlen_ptr - 1;

    /* add the value string */
    if (strstr(val, " ") || strstr(val, SMPD_DELIM_STR) || val[0] == SMPD_QUOTE_CHAR)
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
	smpd_dbg_printf("partial argument added to string: '%s'\n", *str_ptr);
	return SMPD_FAIL;
    }
    
    /* add the trailing space */
    **str_ptr = ' ';
    *str_ptr = *str_ptr + 1;
    **str_ptr = '\0';
    *maxlen_ptr = *maxlen_ptr - 1;

    return SMPD_SUCCESS;
}

int smpd_add_int_arg(char **str_ptr, int *maxlen_ptr, const char *flag, int val)
{
    char val_str[12];
    sprintf(val_str, "%d", val);
    return smpd_add_string_arg(str_ptr, maxlen_ptr, flag, val_str);
}

int smpd_get_opt(int *argc, char ***argv, char * flag)
{
    int i,j;
    if (flag == NULL)
	return 0;

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    for (j=i; j<*argc; j++)
	    {
		(*argv)[j] = (*argv)[j+1];
	    }
	    *argc -= 1;
	    return 1;
	}
    }
    return 0;
}

int smpd_get_opt_int(int *argc, char ***argv, char * flag, int *n)
{
    int i,j;
    if (flag == NULL)
	return 0;

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == *argc)
		return 0;
	    *n = atoi((*argv)[i+1]);
	    for (j=i; j<*argc-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    return 1;
	}
    }
    return 0;
}

int smpd_get_opt_long(int *argc, char ***argv, char * flag, long *n)
{
    int i;
    if (smpd_get_opt_int(argc, argv, flag, &i))
    {
	*n = (long)i;
	return 1;
    }
    return 0;
}

int smpd_get_opt_double(int *argc, char ***argv, char * flag, double *d)
{
    int i,j;

    if (flag == NULL)
	return 0;

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == *argc)
		return 0;
	    *d = atof((*argv)[i+1]);
	    for (j=i; j<*argc-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    return 1;
	}
    }
    return 0;
}

int smpd_get_opt_string(int *argc, char ***argv, char * flag, char *str, int len)
{
    int i,j;

    if (flag == NULL)
	return 0;

    for (i=0; i<*argc; i++)
    {
	if (strcmp((*argv)[i], flag) == 0)
	{
	    if (i+1 == (*argc))
		return 0;
	    if ((*argv)[i+1][0] == '-')
		return 0;
	    strncpy(str, (*argv)[i+1], len);
	    str[len-1] = '\0';
	    for (j=i; j<(*argc)-1; j++)
	    {
		(*argv)[j] = (*argv)[j+2];
	    }
	    *argc -= 2;
	    return 1;
	}
    }
    return 0;
}
