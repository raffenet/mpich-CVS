/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>

int main( int argc, char *argv[] )
{
    int i;

    for (i=0; ; i++)
    {
	if (i % 10000000 == 0)
	{
            printf("i=%d\n",i);
	    fflush(stdout);
	}
    }
    return 0;
}
