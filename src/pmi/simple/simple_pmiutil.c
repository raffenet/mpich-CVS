/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Utility functions associated with PMI implementation, but not part of
   the PMI interface itself.  Reading and writing on pipes, signals, and parsing
   key=value messages
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "simple_pmiutil.h"

#define MAXVALLEN 1024
#define MAXKEYLEN   32

/* These are not the keyvals in the keyval space that is part of the PMI specification.
   They are just part of this implementation's internal utilities.
*/
struct PMIU_keyval_pairs {
    char key[MAXKEYLEN];
    char value[MAXVALLEN];	
};
struct PMIU_keyval_pairs PMIU_keyval_tab[64];
int  PMIU_keyval_tab_idx;

void PMIU_printf( int print_flag, char *fmt, ... )
{
    va_list ap;

    if ( print_flag ) {
	fprintf( stderr, "[%s]: ", PMIU_print_id );
	va_start( ap, fmt );
	vfprintf( stderr, fmt, ap );
	va_end( ap );
	fflush( stderr );
    }
}

/* This function reads until it finds a newline character.  It returns the number of
   characters read, including the newline character.  The newline character is stored
   in buf, as in fgets.  It does not supply a string-terminating null character.
*/
int PMIU_readline( int fd, char *buf, int maxlen )
{
    int n, rc;
    char c, *ptr;

    ptr = buf;
    for ( n = 1; n < maxlen; n++ ) {
      again:
	rc = read( fd, &c, 1 );
	if ( rc == 1 ) {
	    *ptr++ = c;
	    if ( c == '\n' )	/* note \n is stored, like in fgets */
		break;
	}
	else if ( rc == 0 ) {
	    if ( n == 1 )
		return( 0 );	/* EOF, no data read */
	    else
		break;		/* EOF, some data read */
	}
	else {
	    if ( errno == EINTR )
		goto again;
	    return ( -1 );	/* error, errno set by read */
	}
    }
    *ptr = 0;			/* null terminate, like fgets */
    PMIU_printf( 0, " received :%s:\n", buf );
    return( n );
}

int PMIU_writeline( int fd, char *buf )	
{
    int size, n;

    size = strlen( buf );
    if ( size > PMIU_MAXLINE ) {
	buf[PMIU_MAXLINE-1] = '\0';
	PMIU_printf( 1, "write_line: message string too big: :%s:\n", buf );
    }
    else if ( buf[strlen( buf ) - 1] != '\n' )  /* error:  no newline at end */
	    PMIU_printf( 1, "write_line: message string doesn't end in newline: :%s:\n",
		       buf );
    else {
	n = write( fd, buf, size );
	if ( n < 0 ) {
	    PMIU_printf( 1, "write_line error; fd=%d buf=:%s:\n", fd, buf );
	    perror("system msg for write_line failure ");
	    return(-1);
	}
	if ( n < size)
	    PMIU_printf( 1, "write_line failed to write entire message\n" );
    }
    return 0;
}

int PMIU_parse_keyvals( char *st )
{
    char *p, *keystart, *valstart;

    if ( !st )
	return( -1 );

    PMIU_keyval_tab_idx = 0;          
    p = st;
    while ( 1 ) {
	while ( *p == ' ' )
	    p++;
	/* got non-blank */
	if ( *p == '=' ) {
	    PMIU_printf( 1, "PMIU_parse_keyvals:  unexpected = at character %d in %s\n",
		       p - st, st );
	    return( -1 );
	}
	if ( *p == '\n' || *p == '\0' )
	    return( 0 );	/* normal exit */
	/* got normal character */
	keystart = p;		/* remember where key started */
	while ( *p != ' ' && *p != '=' && *p != '\n' && *p != '\0' )
	    p++;
	if ( *p == ' ' || *p == '\n' || *p == '\0' ) {
	    PMIU_printf( 1,
	       "PMIU_parse_keyvals: unexpected key delimiter at character %d in %s\n",
		       p - st, st );
	    return( -1 );
	}
        strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].key, keystart, p - keystart );
	PMIU_keyval_tab[PMIU_keyval_tab_idx].key[p - keystart] = '\0'; /* store key */

	valstart = ++p;			/* start of value */
	while ( *p != ' ' && *p != '\n' && *p != '\0' )
	    p++;
        strncpy( PMIU_keyval_tab[PMIU_keyval_tab_idx].value, valstart, p - valstart );
	PMIU_keyval_tab[PMIU_keyval_tab_idx].value[p - valstart] = '\0'; /* store value */
	PMIU_keyval_tab_idx++;
	if ( *p == ' ' )
	    continue;
	if ( *p == '\n' || *p == '\0' )
	    return( 0 );	/* value has been set to empty */
    }
}
 
void PMIU_dump_keyvals( void )
{
    int i;
    for (i=0; i < PMIU_keyval_tab_idx; i++) 
	PMIU_printf(1, "  %s=%s\n",PMIU_keyval_tab[i].key, PMIU_keyval_tab[i].value);
}

char *PMIU_getval( char *keystr, char *valstr, int vallen )
{
    int i;
    
    for (i = 0; i < PMIU_keyval_tab_idx; i++) {
	if ( strcmp( keystr, PMIU_keyval_tab[i].key ) == 0 ) { 
	    strncpy( valstr, PMIU_keyval_tab[i].value, vallen - 1 );
	    valstr[vallen - 1] = '\0';
	    return valstr;
       } 
    }
    valstr[0] = '\0';
    return NULL;
}

void PMIU_chgval( char *keystr, char *valstr )
{
    int i;
    
    for ( i = 0; i < PMIU_keyval_tab_idx; i++ ) {
	if ( strcmp( keystr, PMIU_keyval_tab[i].key ) == 0 ) {
	    strncpy( PMIU_keyval_tab[i].value, valstr, MAXVALLEN - 1 );
	    PMIU_keyval_tab[i].value[MAXVALLEN - 1] = '\0';
	}
    }
}
