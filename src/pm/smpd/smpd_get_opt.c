#include "smpd.h"
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

int smpd_get_string_arg(char *str, char *flag, char *val, int maxlen)
{
    char *pszFirst, *pszDelimLoc, *pszLast;
    char *pszDelim = "=";
    int bFirst = SMPD_TRUE;

    if (str == NULL || flag == NULL || val == NULL)
	return SMPD_FALSE;

    while (SMPD_TRUE)
    {
	// Find the name
	pszFirst = strstr(str, flag);
	if (pszFirst == NULL)
	    return SMPD_FALSE;

	// Check to see if we have matched a sub-string
	if (bFirst)
	{
	    bFirst = SMPD_FALSE;
	    if ((pszFirst != str) && (!isspace(*(pszFirst-1))))
	    {
		str = pszFirst + strlen(flag);
		continue;
	    }
	}
	else
	{
	    if (!isspace(*(pszFirst-1)))
	    {
		str = pszFirst + strlen(flag);
		continue;
	    }
	}

	// Skip over any white space after the name
	pszDelimLoc = &pszFirst[strlen(flag)];
	while (isspace(*pszDelimLoc))
	    pszDelimLoc++;

	// Find the deliminator
	if (strncmp(pszDelimLoc, pszDelim, strlen(pszDelim)) != 0)
	{
	    //str = &pszDelimLoc[strlen(pszDelim)];
	    str = pszDelimLoc;
	    continue;
	}
	
	// Skip over the deliminator and any white space
	pszFirst = &pszDelimLoc[strlen(pszDelim)];
	while (isspace(*pszFirst))
	    pszFirst++;

	if (*pszFirst == '\'')
	{
	    pszFirst++;
	    while (*pszFirst != '\'' && *pszFirst != '\0')
	    {
		*val++ = *pszFirst++;
	    }
	    *val = '\0';
	    break;
	}
	else
	{
	    // Find the next deliminator
	    pszLast = strstr(pszFirst, pszDelim);
	    if (pszLast == NULL)
	    {
		strcpy(val, pszFirst);
		break;
	    }
	    
	    // Back up over any white space and name preceding the second deliminator
	    pszLast--;
	    while (pszLast > pszFirst && isspace(*pszLast))
		pszLast--;
	    while (pszLast > pszFirst && !isspace(*pszLast))
		pszLast--;
	    while (pszLast > pszFirst && isspace(*pszLast))
		pszLast--;
	    
	    // Copy the data between first and last
	    pszLast++;
	    strncpy(val, pszFirst, pszLast-pszFirst);
	    val[pszLast-pszFirst] = '\0';
	}
	break;
    }
    return SMPD_TRUE;
}

int smpd_get_int_arg(char *str, char *flag, int *val_ptr)
{
    char int_str[12];

    if (smpd_get_string_arg(str, flag, int_str, 12))
    {
	*val_ptr = atoi(int_str);
	return SMPD_TRUE;
    }
    return SMPD_FALSE;
}

int smpd_add_string_arg(char **str_ptr, int *maxlen_ptr, char *flag, char *val)
{
    int num_chars;

    if (strstr(flag, " "))
    {
	smpd_err_printf("invalid flag, spaces not allowed: %s\n", flag);
	return SMPD_FAIL;
    }
    num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s='%s' ", flag, val);
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    return SMPD_SUCCESS;
}

int smpd_add_int_arg(char **str_ptr, int *maxlen_ptr, char *flag, int val)
{
    int num_chars;

    if (strstr(flag, " "))
    {
	smpd_err_printf("invalid flag, spaces not allowed: %s\n", flag);
	return SMPD_FAIL;
    }
    num_chars = snprintf(*str_ptr, *maxlen_ptr, "%s=%d ", flag, val);
    *str_ptr = *str_ptr + num_chars;
    *maxlen_ptr = *maxlen_ptr - num_chars;
    return SMPD_SUCCESS;
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
	    if (i+1 == *argc)
		return 0;
	    strncpy(str, (*argv)[i+1], len);
	    str[len-1] = '\0';
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
