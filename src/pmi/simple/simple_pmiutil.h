/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* maximum sizes for arrays */
#define PMIU_MAXLINE 1024
#define PMIU_IDSIZE    32

/* This must be extern because two files include this header.  simple_pmi.c 
   declares this value */
extern char PMIU_print_id[PMIU_IDSIZE];

/* prototypes for PMIU routines */
void PMIU_printf( int print_flag, char *fmt, ... );
int  PMIU_readline( int fd, char *buf, int max );
int  PMIU_writeline( int fd, char *buf );
int  PMIU_parse_keyvals( char *st );
void PMIU_dump_keyvals( void );
char *PMIU_getval( const char *keystr, char *valstr, int vallen );
void PMIU_chgval( const char *keystr, char *valstr );
