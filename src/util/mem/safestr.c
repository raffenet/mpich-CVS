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
 *
 * Question: should we provide a way to request the length of the string,
 * since we know it?
 */
int MPIU_Strncpy( char *dest, const char *src, size_t n )
{
    char * restrict d_ptr = dest;
    const char * restrict s_ptr = src;
    register int i;

    i = (int)n;
    while (*s_ptr && i-- > 0) {
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

/* Append src to dest, but only allow dest to contain n characters (including
   any null, which is always added to the end of the line */
int MPIU_Strnapp( char *dest, const char *src, size_t n )
{
    char * restrict d_ptr = dest;
    const char * restrict s_ptr = src;
    register int i;

    /* Get to the end of dest */
    i = (int)n;
    while (i-- > 0 && *d_ptr++) ;
    /* The last ++ moved us past the null */
    d_ptr--;
    /* Append */
    while (i-- > 0 && *s_ptr) {
	*d_ptr++ = *s_ptr++;
    }

    if (i > 0) { 
	*d_ptr = 0;
	return 0;
    }
    else {
	/* Force the null at the end */
	d_ptr--;
	*d_ptr = 0;
    
	/* We may want to force an error message here, at least in the
	   debugging version */
	return 1;
    }
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

/* 
 * We need an snprintf replacement for systems without one
 */
#ifndef HAVE_SNPRINTF
/* 
 * This is an approximate form which is suitable for most uses within
 * the MPICH code
 */
int MPIU_Snprintf( char *str, size_t size, const char *format, ... )
{
    int n;
    const char *p;
    char *out_str = str;
    va_list list;

    va_start(list, format);

    p = format;
    while (*p && size > 0) {
	char *nf;

	nf = strchr(p, '%');
	if (!nf) {
	    /* No more format characters */
	    while (size-- > 0 && *p) {
		*out_str++ = *p++;
	    }
	}
	else {
	    int nc = nf[1];
	    
	    /* Copy until nf */
	    while (p < nf && size-- > 0) {
		*out_str++ = *p++;
	    }
	    /* Handle the format character */
	    switch (nc) {
	    case '%':
		*out_str++ = '%';
		size--;
		p += 2;
		break;

	    case 'd':
	    {
		int val;
		char tmp[20];
		char *t = tmp;
		p += 2;
		/* Get the argument, of integer type */
		val = va_arg( list, int );
		sprintf( tmp, "%d", val );
		while (size-- > 0 && *t) {
		    *out_str++ = *t++;
		}
	    }
	    break;

	    case 's':
	    {
		char *s_arg;
		p += 2;
		/* Get the argument, of pointer to char type */
		s_arg = va_arg( list, char * );
		while (size-- > 0 && s_arg && *s_arg) {
		    *out_str++ = *s_arg++;
		}
	    }
	    break;
	    default:
		/* Error, unknown case */
		return -1;
		break;
	    }
	}
    }

    va_end(list);

    if (size-- > 0) *out_str++ = '\0';

    n = (int)(out_str - str);
    return n;
}
#endif
