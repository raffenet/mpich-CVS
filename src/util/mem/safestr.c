/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* 
 * This file contains "safe" versions of the various string and printf
 * operations.  So far, only strncpy is included.
 */

/*
 * MPIU_Strncpy - Copy at most n character.  Stop once a null is reached.
 *
 * This is different from strncpy, which null pads so that exactly
 * n characters are copied.  The strncpy behavior is correct for many 
 * applications because it guarantees that the string has no uninitialized
 * data.
 *
 * If n characters are copied without reaching a null, return an error.
 * Otherwise, return 0.
 */
int MPIU_Strncpy( char *dest, const char *src, size_t n )
{
    char * restrict d_ptr = dest;
    const char * restrict s_ptr = src;
    register int i;

    i = n;
    while (i-- > 0 && *s_ptr) {
	*d_ptr++ = *s_ptr++;
    }
    
    if (i > 0) { 
	*d_ptr = 0;
	return 0;
    }
    else
	/* We may want to force an error message here, at least in the
	   debugging version */
	return 1;
}

#ifndef HAVE_STRDUP
#ifdef MPIU_Strdup
#undef MPIU_Strdup
#endif
char *MPIU_Strdup( const char *str )
{
    char *p = MPIU_Malloc( strlen(str) + 1 );
    char *in_p = (char *)str;

    if (!p) {
	while (*in_p) {
	    *p++ = *in_p++;
	}
    }
    return p;
}
#endif
