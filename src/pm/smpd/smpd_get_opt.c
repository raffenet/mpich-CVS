#include "smpd.h"
#include <stdio.h>

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
