/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*********************** PMI implementation ********************************/

#include "pmiconf.h" 

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef USE_PMI_PORT
#ifndef MAXHOSTNAME
#define MAXHOSTNAME 256
#endif
#endif
/* This should be moved to pmiu for shutdown */
#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif
/* mpimem includes the definitions for MPIU_Snprintfl, MPIU_Malloc, and 
   MPIU_Free */
#include "mpimem.h"

/* Temporary debug definitions */
#define DBG_PRINTF(args) printf args ; fflush(stdout)
#define DBG_FPRINTF(args) fprintf args 

#include "pmi.h"
#include "simple_pmiutil.h"

/* 
   These are global variable used *ONLY* in this file, and are hence
   declared static.
 */


static int PMI_fd = -1;
static int PMI_size = 1;
static int PMI_rank = 0;

/* Set PMI_initialized to 1 for regular initialized and 2 for 
   the singleton init case (no MPI_Init) */
#define SINGLETON_INIT 2
static int PMI_initialized = 0;

/* ALL GLOBAL VARIABLES MUST BE INITIALIZED TO AVOID POLLUTING THE 
   LIBRARY WITH COMMON SYMBOLS */
static int PMI_kvsname_max = 0;
static int PMI_keylen_max = 0;
static int PMI_vallen_max = 0;

static int PMI_iter_next_idx = 0;
static int PMI_debug = 0;
static int PMI_spawned = 0;

static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max );
static int PMII_iter( const char *kvsname, const int idx, int *nextidx, char *key, int key_len, char *val, int val_len );
static int PMII_Set_from_port( int, int );
static int PMII_Connect_to_pm( char *, int );

/******************************** Group functions *************************/

int PMI_Init( int *spawned )
{
    char *p;
    int notset = 1;

    /* setvbuf(stdout,0,_IONBF,0); */
    setbuf(stdout,NULL);
    /* PMIU_printf( 1, "PMI_INIT\n" ); */
    if ( ( p = getenv( "PMI_FD" ) ) )
	PMI_fd = atoi( p );
#ifdef USE_PMI_PORT
    else if ( ( p = getenv( "PMI_PORT" ) ) ) {
	int portnum;
	char hostname[MAXHOSTNAME];
	char *pn;
	int id = 0;
	/* Not yet implemented.  Connect to the indicated port (in
	   format hostname:portnumber) and get the fd for the socket */

	PMI_debug = 0;
	
	/* Split p into host and port */
	pn = strchr( p, ':' );

	if (PMI_debug) {
	    DBG_PRINTF( ("Connecting to %s\n", p) );
	}
	if (pn) {
	    MPIU_Strncpy( hostname, p, (pn - p) );
	    hostname[(pn-p)] = 0;
	    portnum = atoi( pn+1 );
	    /* FIXME: Check for valid integer after : */
	    PMI_fd = PMII_Connect_to_pm( hostname, portnum );
	}
	/* FIXME: If PMI_PORT specified but either no valid value of
	   fd is -1, give an error return */
	if (PMI_fd < 0) return -1;

	/* We should first handshake to get size, rank, debug. */
	p = getenv( "PMI_ID" );
	if (p) {
	    id = atoi( p );
	    /* PMII_Set_from_port sets up the values that are delivered
	       by enviroment variables when a separate port is not used */
	    PMII_Set_from_port( PMI_fd, id );
	    notset = 0;
	}
    }
#endif
    else {
	PMI_fd = -1;
    }

    if ( PMI_fd == -1 ) {
	/* Singleton init: Process not started with mpiexec, 
	   so set size to 1, rank to 0 */
	PMI_size = 1;
	PMI_rank = 0;
	*spawned = 0;
	
	PMI_initialized = SINGLETON_INIT;
	PMI_kvsname_max = 256;
	PMI_keylen_max  = 256;
	PMI_vallen_max  = 256;
	
	return( 0 );
    }

    /* If size, rank, and debug not set from a communication port,
       use the environment */
    if (notset) {
	if ( ( p = getenv( "PMI_SIZE" ) ) )
	    PMI_size = atoi( p );
	else 
	    PMI_size = 1;
	
	if ( ( p = getenv( "PMI_RANK" ) ) ) {
	    PMI_rank = atoi( p );
	    /* Let the util routine know the rank of this process for 
	       any messages (usually debugging or error) */
	    PMIU_Set_rank( PMI_rank );
	}
	else 
	    PMI_rank = 0;
	
	if ( ( p = getenv( "PMI_DEBUG" ) ) )
	    PMI_debug = atoi( p );
	else 
	    PMI_debug = 0;

	/* Leave unchanged otherwise, which indicates that no value
	   was set */
    }
	
    PMII_getmaxes( &PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max );

    if ( ( p = getenv( "PMI_SPAWNED" ) ) )
	PMI_spawned = atoi( p );
    else
	PMI_spawned = 0;
    if (PMI_spawned)
	*spawned = 1;
    else
	*spawned = 0;

    PMI_initialized = 1;

    /*****   RMB TEST BLOCK
    {
	int size, rc;
	rc = PMI_Get_universe_size( &size );
	PMIU_printf( 1, "PMI_INIT GOT RC=%d UNIVSIZE=%d\n", rc,size );
    }
    *****/

    return( 0 );
}

int PMI_Initialized( PMI_BOOL *initialized )
{
    /* Turn this into a logical value (1 or 0) .  This allows us
       to use PMI_initialized to distinguish between initialized with
       an PMI service (e.g., via mpiexec) and the singleton init, 
       which has no PMI service */
    *initialized = PMI_initialized != 0 ? PMI_TRUE : PMI_FALSE;
    return PMI_SUCCESS;
}

int PMI_Get_size( int *size )
{
    if ( PMI_initialized )
	*size = PMI_size;
    else
	*size = 1;
    return( 0 );
}

int PMI_Get_rank( int *rank )
{
    if ( PMI_initialized )
	*rank = PMI_rank;
    else
	*rank = 0;
    return( 0 );
}

int PMI_Get_universe_size( int *size)
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE], size_c[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    if ( PMI_initialized )
    {
#ifdef USE_HUMAN_READABLE_TOKENS
	iter = buf;
	maxlen = PMIU_MAXLINE;
	MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "get_universe_size");
	MPIU_Strncpy(iter, "\n", maxlen);
	PMIU_writeline( PMI_fd, buf );
#else
	PMIU_writeline( PMI_fd, "cmd=get_universe_size\n" );
#endif
	PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if ( strncmp( cmd, "universe_size", PMIU_MAXLINE ) != 0 ) {
	    PMIU_printf( 1, "expecting cmd=universe_size, got %s\n", buf );
	    return( PMI_FAIL );
	}
	else {
	    PMIU_getval( "size", size_c, PMIU_MAXLINE );
	    *size = atoi(size_c);
	    return( PMI_SUCCESS );
	}
    }
    else
	*size = 1;
    return( PMI_SUCCESS );
}

int PMI_Barrier( )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    if ( PMI_initialized == 1) {
	/* Ignore if SINGLETON_INIT */
#ifdef USE_HUMAN_READABLE_TOKENS
	iter = buf;
	maxlen = PMIU_MAXLINE;
	MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "barrier_in");
	MPIU_Strncpy(iter, "\n", maxlen);
	PMIU_writeline( PMI_fd, buf /*"cmd=barrier_in\n"*/ );
#else
	PMIU_writeline( PMI_fd, "cmd=barrier_in\n");
#endif
	PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if ( strncmp( cmd, "barrier_out", PMIU_MAXLINE ) != 0 ) {
	    PMIU_printf( 1, "expecting cmd=barrier_out, got %s\n", buf );
	    return( -1 );
	}
	else
	    return( 0 );
    }
    else
	return( 0 );
}

int PMI_Get_clique_size( int *size )
{
    *size = 1;
    return PMI_SUCCESS;
}

int PMI_Get_clique_ranks( int ranks[], int length )
{
    if ( length < 1 )
	return PMI_ERR_INVALID_ARG;
    else
	return PMI_Get_rank( &ranks[0] );
}

/* Inform the process manager that we're in finalize */
int PMI_Finalize( )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    /* Ignore SINGLETON_INIT */
    if (PMI_initialized == 1)
    {
#ifdef USE_HUMAN_READABLE_TOKENS
	iter = buf;
	maxlen = PMIU_MAXLINE;
	MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "finalize");
	MPIU_Strncpy(iter, "\n", maxlen);
	PMIU_writeline( PMI_fd, buf );
#else
	PMIU_writeline( PMI_fd, "cmd=finalize\n" );
#endif
	PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
	PMIU_parse_keyvals( buf );
	PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
	if ( strncmp( cmd, "finalize_ack", PMIU_MAXLINE ) != 0 ) {
	    PMIU_printf( 1, "expecting cmd=finalize_ack, got %s\n", buf );
	    return( -1 );
	}
	shutdown( PMI_fd, SHUT_RDWR );
	close( PMI_fd );
    }
    return( 0 );
}

int PMI_Abort(int exit_code, const char error_msg[])
{
    fprintf(stderr, "aborting job:\n%s\n", error_msg);
    fflush(stderr);
    exit(exit_code);
    return -1;
}

/**************************************** Keymap functions *************************/

int PMI_KVS_Get_my_name( char kvsname[], int length )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    if (PMI_initialized == SINGLETON_INIT) {
	/* Return a dummy name */
	MPIU_Strncpy( kvsname, "mykvs", PMIU_MAXLINE );
	return 0;
    }
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "get_my_kvsname");
    MPIU_Strncpy(iter, "\n", maxlen);
    PMIU_writeline( PMI_fd, buf );
#else
    PMIU_writeline( PMI_fd, "cmd=get_my_kvsname\n" );
#endif
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "my_kvsname", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to get_my_kvsname :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "kvsname", kvsname, PMI_kvsname_max );
	return( 0 );
    }
}

int PMI_KVS_Get_name_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_kvsname_max;
    return PMI_SUCCESS;
}

int PMI_KVS_Get_key_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_keylen_max;
    return PMI_SUCCESS;
}

int PMI_KVS_Get_value_length_max( int *maxlen )
{
    if (maxlen == NULL)
	return PMI_ERR_INVALID_ARG;
    *maxlen = PMI_vallen_max;
    return PMI_SUCCESS;
}

/* We will use the default kvsname for both the kvs_domain_id and for the id */
/* Hence the implementation of the following three functions */

int PMI_Get_id_length_max( int *length )
{
    if (length == NULL)
	return PMI_ERR_INVALID_ARG;
    *length = PMI_kvsname_max;
    return PMI_SUCCESS;
}

int PMI_Get_id( char id_str[], int length )
{
    return PMI_KVS_Get_my_name( id_str, length );
}

int PMI_Get_kvs_domain_id( char id_str[], int length )
{
    return PMI_KVS_Get_my_name( id_str, length );
}

int PMI_KVS_Create( char kvsname[], int length )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif
    
    if (PMI_initialized == SINGLETON_INIT) {
	/* It is ok to pretend to *create* a kvs space */
	return 0;
    }

#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "create_kvs");
    MPIU_Strncpy(iter, "\n", maxlen);
    PMIU_writeline( PMI_fd, buf );
#else
    PMIU_writeline( PMI_fd, "cmd=create_kvs\n" );
#endif
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "newkvs", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to create_kvs :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "kvsname", kvsname, PMI_kvsname_max );
	return( 0 );
    }
}

int PMI_KVS_Destroy( const char kvsname[] )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int rc;
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    if (PMI_initialized == SINGLETON_INIT) {
	return 0;
    }

    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "destroy_kvs");
    MPIU_Str_add_string_arg(&iter, &maxlen, "kvsname", kvsname);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=destroy_kvs kvsname=%s\n", kvsname );
#endif
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "kvs_destroyed", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to destroy_kvs :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc != 0 ) {
	    PMIU_getval( "msg", buf, PMIU_MAXLINE );
	    PMIU_printf( 1, "KVS not destroyed, reason='%s'\n", buf );
	    return( -1 );
	}
	else {
	    return( 0 );
	}
    }
}

int PMI_KVS_Put( const char kvsname[], const char key[], const char value[] )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE], message[PMIU_MAXLINE];
    int  rc;
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    if (PMI_initialized == SINGLETON_INIT) {
	/* Ignore the put */
	return 0;
    }
    
    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "put");
    MPIU_Str_add_string_arg(&iter, &maxlen, "kvsname", kvsname);
    MPIU_Str_add_string_arg(&iter, &maxlen, "key", key);
    MPIU_Str_add_string_arg(&iter, &maxlen, "value", value);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=put kvsname=%s key=%s value=%s\n",
	      kvsname, key, value);
#endif
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "put_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to put :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc < 0 ) {
	    PMIU_getval( "msg", message, PMIU_MAXLINE );
	    PMIU_printf( 1, "put failed; reason = %s\n", message );
	    return( -1 );
	}
    }
    return( 0 );
}

int PMI_KVS_Commit( const char kvsname[] )
{
    /* no-op in this implementation */
    return( 0 );
}

int PMI_KVS_Get( const char kvsname[], const char key[], char value[], int length)
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int  rc;
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "get");
    MPIU_Str_add_string_arg(&iter, &maxlen, "kvsname", kvsname);
    MPIU_Str_add_string_arg(&iter, &maxlen, "key", key);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=get kvsname=%s key=%s\n", kvsname, key );
#endif
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "get_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to get :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc == 0 ) {
	    PMIU_getval( "value", value, PMI_vallen_max );
	    return( 0 );
	}
	else {
	    return( -1 );
	}
    }
}

int PMI_KVS_Iter_first(const char kvsname[], char key[], int key_len, char val[], int val_len)
{
    int rc;

    rc = PMII_iter( kvsname, 0, &PMI_iter_next_idx, key, key_len, val, val_len );
    return( rc );
}

int PMI_KVS_Iter_next(const char kvsname[], char key[], int key_len, char val[], int val_len)
{
    int rc;

    rc = PMII_iter( kvsname, PMI_iter_next_idx, &PMI_iter_next_idx, key, key_len, val, val_len );
    if ( rc == -2 )
	PMI_iter_next_idx = 0;
    return( rc );
}

/******************************** Process Creation functions *************************/

/* PMI_Spawn obsolete? (replaced by PMI_Spawn_multiple, of which it is a special case */

int PMI_Spawn(const char *command, const char *argv[], 
	      const int maxprocs, char *kvsname, int kvsnamelen )
{
    int  rc;
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "spawn");
    MPIU_Str_add_int_arg(&iter, &maxlen, "nprocs", maxprocs);
    MPIU_Str_add_string_arg(&iter, &maxlen, "execname", command);
    MPIU_Str_add_string_arg(&iter, &maxlen, "arg", argv[0]);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=spawn nprocs=%d execname=%s arg=%s\n",
	      maxprocs, command, argv[0] );
#endif
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    PMIU_getval( "remote_kvsname", kvsname, kvsnamelen );
    if ( strncmp( cmd, "spawn_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to spawn :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc == 0 ) {
	    return( 0 );
	}
	else {
	    return( -1 );
	}
    }
    return( -1 );
}

int PMI_Spawn_multiple(int count,
                       const char * cmds[],
                       const char ** argvs[],
                       const int maxprocs[],
                       const int info_keyval_sizes[],
                       const PMI_keyval_t * info_keyval_vectors[],
                       int preput_keyval_size,
                       const PMI_keyval_t preput_keyval_vector[],
                       int errors[])
{
    int  i,rc,argcnt,spawncnt;
    char buf[PMIU_MAXLINE], tempbuf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];

    /* PMIU_printf( 1, "PMI_Spawn_multiple not implemented yet\n" ); */
    /* printf("CMD0 = :%s:\n",cmds[0]);  fflush(stdout);  */
       /* printf("ARG00=:%s:\n",argvs[0][0]);  fflush(stdout);  */
    /* MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=spawn execname=/bin/hostname nprocs=1\n" ); */
    for (spawncnt=0; spawncnt < count; spawncnt++)
    {
	/* FIXME: Check for buf too short */
        MPIU_Snprintf(buf, PMIU_MAXLINE, "mcmd=spawn\nnprocs=%d\nexecname=%s\n",
	          maxprocs[spawncnt], cmds[spawncnt] );

	MPIU_Snprintf(tempbuf, PMIU_MAXLINE,"totspawns=%d\nspawnssofar=%d\n",
		      count, spawncnt+1);
	MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);

        argcnt = 0;
        if ((argvs != NULL) && (argvs[spawncnt] != NULL)) {
            for (i=0; argvs[spawncnt][i] != NULL; i++)
            {
		/* FIXME (protocol design flaw): command line arguments
		   may contain both = and <space> (and even tab!).
		   Also, command line args may be quite long, leading to 
		   errors when PMIU_MAXLINE is exceeded */
		/* FIXME: Check for tempbuf too short */
                MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"arg%d=%s\n",i+1,argvs[spawncnt][i]);
		/* FIXME: Check for error (buf too short for line) */
                MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
                argcnt++;
            }
        }
	/* FIXME: Check for tempbuf too short */
        MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"argcnt=%d\n",argcnt);
	/* FIXME: Check for error (buf too short for line) */
        MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
    
/*        snprintf(tempbuf,PMIU_MAXLINE,"preput_num=%d ",preput_num);
        strcat(buf,tempbuf);
        for (i=0; i < preput_num; i++)
        {
	    MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_%d=%s:%s ",i,preput_keys[i],preput_vals[i]);
*/ /* FIXME: Check for error (buf too short for line) *//*
	    MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
        }
*/

	/* FIXME: Check for tempbuf too short */
        MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_num=%d\n", preput_keyval_size);
	/* FIXME: Check for error (buf too short for line) */
        MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
        for (i=0; i < preput_keyval_size; i++)
        { 
	    /* FIXME: Check for tempbuf too short */
	    MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_key_%d=%s\n",i,preput_keyval_vector[i].key);
	    /* FIXME: Check for error (buf too short for line) */
	    MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    /* FIXME: Check for tempbuf too short */
	    MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"preput_val_%d=%s\n",i,preput_keyval_vector[i].val);
	    /* FIXME: Check for error (buf too short for line) */
	    MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
        } 
	/* FIXME: Check for tempbuf too short */
        MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_num=%d\n", info_keyval_sizes[spawncnt]);
	/* FIXME: Check for error (buf too short for line) */
        MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE);
	for (i=0; i < info_keyval_sizes[spawncnt]; i++)
	{
	    /****
	    printf("RMB: SPAWN_MULT: for cmd %d  keyval%d = (%s,%s)\n",
		   spawncnt,i,
		   info_keyval_vectors[spawncnt][i].key,
		   info_keyval_vectors[spawncnt][i].val);
	    ****/
	    /* FIXME: Check for tempbuf too short */
	    MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_key_%d=%s\n",i,info_keyval_vectors[spawncnt][i].key);
	    /* FIXME: Check for error (buf too short for line) */
	    MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	    /* FIXME: Check for tempbuf too short */
	    MPIU_Snprintf(tempbuf,PMIU_MAXLINE,"info_val_%d=%s\n",i,info_keyval_vectors[spawncnt][i].val);
	    /* FIXME: Check for error (buf too short for line) */
	    MPIU_Strnapp(buf,tempbuf,PMIU_MAXLINE); 
	}

	/* FIXME: Check for error (buf too short for line) */
        MPIU_Strnapp(buf, "endcmd\n", PMIU_MAXLINE);
        PMIU_writeline( PMI_fd, buf );
    }
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "spawn_result", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to spawn :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc != 0 ) {
	    return( -1 );
	}
    }
    return( 0 );
}



int PMI_Args_to_keyval(int *argcp, char *((*argvp)[]), PMI_keyval_t **keyvalp, int *size)
{
    return ( 0 );
}

int PMI_Parse_option(int num_args, char *args[], int *num_parsed, PMI_keyval_t **keyvalp, int *size)
{
    if (num_args < 1)
        return PMI_ERR_INVALID_NUM_ARGS;
    if (args == NULL)
        return PMI_ERR_INVALID_ARGS;
    if (num_parsed == NULL)
        return PMI_ERR_INVALID_NUM_PARSED;
    if (keyvalp == NULL)
        return PMI_ERR_INVALID_KEYVALP;
    if (size == NULL)
        return PMI_ERR_INVALID_SIZE;
    *num_parsed = 0;
    *keyvalp = NULL;
    *size = 0;
    return PMI_SUCCESS;
}

int PMI_Free_keyvals(PMI_keyval_t keyvalp[], int size)
{
    int i;
    if (size < 0)
        return PMI_ERR_INVALID_ARG;
    if (keyvalp == NULL && size > 0)
        return PMI_ERR_INVALID_ARG;
    if (size == 0)
        return PMI_SUCCESS;
    /* free stuff */
    for (i=0; i<size; i++)
    {
	free(keyvalp[i].key);
	free(keyvalp[i].val);
    }
    free(keyvalp);
    return PMI_SUCCESS;
}

/********************* Internal routines not part of PMI interface *****************/

/* get a keyval pair by specific index */

static int PMII_iter( const char *kvsname, const int idx, int *next_idx, char *key, int key_len, char *val, int val_len)
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int  rc;
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "getbyidx");
    MPIU_Str_add_string_arg(&iter, &maxlen, "kvsname", kvsname);
    MPIU_Str_add_int_arg(&iter, &maxlen, "idx", idx);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=getbyidx kvsname=%s idx=%d\n", kvsname, idx  );
#endif
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "getbyidx_results", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to getbyidx :%s:\n", buf );
	return( PMI_FAIL );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc == 0 ) {
	    PMIU_getval( "nextidx", buf, PMIU_MAXLINE );
	    *next_idx = atoi( buf );
	    PMIU_getval( "key", key, PMI_keylen_max );
	    PMIU_getval( "val", val, PMI_vallen_max );
	    return( PMI_SUCCESS );
	}
	else {
	    PMIU_getval( "reason", buf, PMIU_MAXLINE );
	    if ( strncmp( buf, "no_more_keyvals", PMIU_MAXLINE ) == 0 ) {
		key[0] = '\0';
		return( PMI_SUCCESS );
	    }
	    else {
		PMIU_printf( 1, "iter failed; reason = %s\n", buf );
		return( PMI_FAIL );
	    }
	}
    }
}

/* to get all maxes in one message */
static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "init");
    MPIU_Strncpy(iter, "\n", maxlen);
    PMIU_writeline( PMI_fd, buf );
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "get_maxes");
    MPIU_Strncpy(iter, "\n", maxlen);
    PMIU_writeline( PMI_fd, buf );
#else
    PMIU_writeline( PMI_fd, "cmd=init\n" );
    PMIU_writeline( PMI_fd, "cmd=get_maxes\n" );
#endif
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "maxes", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to get_maxes :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "kvsname_max", buf, PMIU_MAXLINE );
	*kvsname_max = atoi( buf );
	PMIU_getval( "keylen_max", buf, PMIU_MAXLINE );
	*keylen_max = atoi( buf );
	PMIU_getval( "vallen_max", buf, PMIU_MAXLINE );
	*vallen_max = atoi( buf );
	return( 0 );
    }
}

#ifdef USE_PMI_PORT
/*
 * This code allows a program to contact a host/port for the PMI socket.
a */
#include <errno.h>
#include <sys/param.h>
#include <sys/socket.h>

/* sockaddr_in (Internet) */
#include <netinet/in.h>
/* TCP_NODELAY */
#include <netinet/tcp.h>

/* sockaddr_un (Unix) */
#include <sys/un.h>

/* defs of gethostbyname */
#include <netdb.h>

/* fcntl, F_GET/SETFL */
#include <fcntl.h>

/* This is really IP!? */
#ifndef TCP
#define TCP 0
#endif

/* stub for connecting to a specified host/port instead of using a 
   specified fd inherited from a parent process */
static int PMII_Connect_to_pm( char *hostname, int portnum )
{
    struct hostent     *hp;
    struct sockaddr_in sa;
    int                fd;
    int                optval = 1;
    int                flags;
    int                q_wait = 1;
    
    hp = gethostbyname( hostname );
    if (!hp) {
	return -1;
    }
    
    bzero( (void *)&sa, sizeof(sa) );
    bcopy( (void *)hp->h_addr, (void *)&sa.sin_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port   = htons( (unsigned short) portnum );
    
    fd = socket( AF_INET, SOCK_STREAM, TCP );
    if (fd < 0) {
	return -1;
    }
    
    if (setsockopt( fd, IPPROTO_TCP, TCP_NODELAY, 
		    (char *)&optval, sizeof(optval) )) {
	perror( "Error calling setsockopt:" );
    }

    /* If a non-blocking socket, then can use select on write to test for
       connect would succeed.  Thus, we mark the socket as non-blocking now */
#ifdef FOO
    /* Mark this fd as non-blocking */
    /* Do we want to do this? */
    if (!q_wait) {
	flags = fcntl( fd, F_GETFL, 0 );
	if (flags >= 0) {
	    flags |= O_NDELAY;
	    fcntl( fd, F_SETFL, flags );
	}
    }
#endif
    if (connect( fd, (struct sockaddr *)&sa, sizeof(sa) ) < 0) {
	switch (errno) {
	case ECONNREFUSED:
	    /* (close socket, get new socket, try again) */
	    if (q_wait)
		close(fd);
	    return -1;
	    
	case EINPROGRESS: /*  (nonblocking) - select for writing. */
	    break;
	    
	case EISCONN: /*  (already connected) */
	    break;
	    
	case ETIMEDOUT: /* timed out */
	    return -1;

	default:
	    return -1;
	}
    }
#ifdef FOO
    /* Do we want to make the fd nonblocking ? */
    if (fd >= 0 && q_wait) {
	flags = fcntl( fd, F_GETFL, 0 );
	if (flags >= 0) {
	    flags |= O_NDELAY;
	    fcntl( fd, F_SETFL, flags );
	}
    }
#endif

    return fd;
}

static int PMII_Set_from_port( int fd, int id )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int err;
#ifdef USE_HUMAN_READABLE_TOKENS
    char *iter;
    int maxlen;
#endif

    /* We start by sending a startup message to the server */

    if (PMI_debug) {
	PMIU_printf( 1, "Writing initack to destination fd %d\n", fd );
    }
    /* Handshake and initialize from a port */

    /* FIXME: Check for tempbuf too short */
#ifdef USE_HUMAN_READABLE_TOKENS
    iter = buf;
    maxlen = PMIU_MAXLINE;
    MPIU_Str_add_string_arg(&iter, &maxlen, "cmd", "initack");
    MPIU_Str_add_int_arg(&iter, &maxlen, "pmiid", id);
    MPIU_Strncpy(iter, "\n", maxlen);
#else
    MPIU_Snprintf( buf, PMIU_MAXLINE, "cmd=initack pmiid=%d\n", id );
#endif
    err = PMIU_writeline( fd, buf );
    if (err) {
	PMIU_printf( 1, "Error in writeline initack\n" );
	return -1;
    }

    /* cmd=initack */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading initack on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp( cmd, "initack" ) ) {
	PMIU_printf( 1, "got unexpected input %s\n", buf );
	return -1;
    }
    
    /* Read, in order, size, rank, and debug.  Eventually, we'll want 
       the handshake to include a version number */

    /* size */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading size on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set size=n */
    PMIU_getval( "size", cmd, PMIU_MAXLINE );
    PMI_size = atoi(cmd);

    /* rank */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading rank on %d\n", fd );
	perror( "Error on readline:" );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set rank=n */
    PMIU_getval( "rank", cmd, PMIU_MAXLINE );
    PMI_rank = atoi(cmd);

    /* debug flag */
    err = PMIU_readline( fd, buf, PMIU_MAXLINE );
    if (err < 0) {
	PMIU_printf( 1, "Error reading debug on %d\n", fd );
	return -1;
    }
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strcmp(cmd,"set")) {
	PMIU_printf( 1, "got unexpected command %s in %s\n", cmd, buf );
	return -1;
    }
    /* cmd=set debug=n */
    PMIU_getval( "debug", cmd, PMIU_MAXLINE );
    PMI_debug = atoi(cmd);

    if (PMI_debug) {
	DBG_PRINTF( ("end of handshake, rank = %d, size = %d\n", 
		    PMI_rank, PMI_size )); 
    }

    return 0;
}
#endif
