/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*********************** PMI implementation ********************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pmi.h"
#include "simple_pmiutil.h"


char PMIU_print_id[PMIU_IDSIZE];

int PMI_fd;
int PMI_size;
int PMI_rank;

/* Set PMI_initialized to 1 for regular initialized and 2 for 
   the singleton init case (no MPI_Init) */
#define SINGLETON_INIT 2
int PMI_initialized;

int PMI_kvsname_max;
int PMI_keylen_max;
int PMI_vallen_max;

int PMI_iter_next_idx;
int PMI_debug;

static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max );
static int PMII_iter( const char *kvsname, const int idx, int *nextidx, char *key, char *val );

/******************************** Group functions *************************/

int PMI_Init( int *spawned )
{
    char *p;

    if ( ( p = getenv( "PMI_FD" ) ) )
	PMI_fd = atoi( p );
    else if ( ( p = getenv( "PMI_PORT" ) ) ) {
	/* Not yet implemented.  Connect to the indicated port (in
	   format hostname:portnumber) and get the fd for the socket */
	PMP_fd = -1;
    }
    else {
	PMI_fd = -1;
    }

    if ( PMI_fd == -1 ) {
	/* Singleton init: Process not started with mpiexec, 
	   so set size to 1, rank to 0 */
	PMI_size = 1;
	PMI_rank = 0;
	
	/* MPIU_Error_printf( "warning: process not started with mpiexec\n" );
	PMI_initialized = 0;
	return( -1 ); */
	PMI_initialized = SINGLETON_INIT;
	PMI_kvsname_max = 256;
	PMI_keylen_max  = 256;
	PMI_vallen_max  = 256;
	
	return( 0 );
    }

    if ( ( p = getenv( "PMI_SIZE" ) ) )
	PMI_size = atoi( p );
    else 
	PMI_size = 1;

    if ( ( p = getenv( "PMI_RANK" ) ) ) {
	PMI_rank = atoi( p );
	snprintf( PMIU_print_id, PMIU_IDSIZE, "cli_%d", PMI_rank );
    }
    else 
	PMI_rank = 0;

    if ( ( p = getenv( "PMI_DEBUG" ) ) )
	PMI_debug = atoi( p );
    else 
	PMI_debug = 0;
	
    PMII_getmaxes( &PMI_kvsname_max, &PMI_keylen_max, &PMI_vallen_max );

    /* for now */
    *spawned = 0;

    PMI_initialized = 1;

    return( 0 );
}

int PMI_Initialized( void )
{
    /* Turn this into a logical value (1 or 0) .  This allows us
       to use PMI_initialized to distinguish between initialized with
       an PMI service (e.g., via mpiexec) and the singleton init, 
       which has no PMI service */
    return( PMI_initialized != 0 );
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

int PMI_Barrier( )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];

    if ( PMI_initialized == 1) {
	/* Ignore if SINGLETON_INIT */
	PMIU_writeline( PMI_fd, "cmd=barrier_in\n" );
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

/* Inform the process manager that we're in finalize */
int PMI_Finalize( )
{
    /* Ignore SINGLETON_INIT */
    if (PMI_initialized == 1)
	PMIU_writeline( PMI_fd, "cmd=finalize\n" );
    return( 0 );
}

/**************************************** Keymap functions *************************/

int PMI_KVS_Get_my_name( char *kvsname )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];

    if (PMI_initialized == SINGLETON_INIT) {
	/* Return a dummy name */
	MPIU_Strncpy( kvsname, "mykvs", PMIU_MAXLINE );
	return 0;
    }
    PMIU_writeline( PMI_fd, "cmd=get_my_kvsname\n" );
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

int PMI_KVS_Get_name_length_max( )
{
    return( PMI_kvsname_max );
}

int PMI_KVS_Get_key_length_max( )
{
    return( PMI_keylen_max );
}

int PMI_KVS_Get_value_length_max( )
{
    return( PMI_vallen_max );
}

int PMI_KVS_Create( char *kvsname )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    
    if (PMI_initialized == SINGLETON_INIT) {
	/* It is ok to pretend to *create* a kvs space */
	return 0;
    }

    PMIU_writeline( PMI_fd, "cmd=create_kvs\n" );
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

int PMI_KVS_Destroy( const char *kvsname )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int rc;

    if (PMI_initialized == SINGLETON_INIT) {
	return 0;
    }

    snprintf( buf, PMIU_MAXLINE, "cmd=destroy_kvs kvsname=%s\n", kvsname );
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

int PMI_KVS_Put( const char *kvsname, const char *key, const char *value )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE], message[PMIU_MAXLINE];
    int  rc;

    if (PMI_initialized == SINGLETON_INIT) {
	/* Ignore the put */
	return 0;
    }
    snprintf( buf, PMIU_MAXLINE, "cmd=put kvsname=%s key=%s value=%s\n",
	      kvsname, key, value);
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

int PMI_KVS_Commit( const char *kvsname )
{
    /* no-op in this implementation */
    return( 0 );
}

int PMI_KVS_Get( const char *kvsname, const char *key, char *value)
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int  rc;

    snprintf( buf, PMIU_MAXLINE, "cmd=get kvsname=%s key=%s\n", kvsname, key );
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

int PMI_KVS_iter_first(const char *kvsname, char *key, char *val)
{
    int rc;

    rc = PMII_iter( kvsname, 0, &PMI_iter_next_idx, key, val );
    return( rc );
}

int PMI_KVS_iter_next(const char *kvsname, char *key, char *val)
{
    int rc;

    rc = PMII_iter( kvsname, PMI_iter_next_idx, &PMI_iter_next_idx, key, val );
    if ( rc == -2 )
	PMI_iter_next_idx = 0;
    return( rc );
}

/******************************** Process Creation functions *************************/

int PMI_Spawn(const char *command, const char *argv[], 
	      const int maxprocs, char *kvsname, int kvsnamelen )
{
    int  rc;
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];

    snprintf( buf, PMIU_MAXLINE, "cmd=spawn nprocs=%d execname=%s arg=%s\n",
	      maxprocs, command, argv[0] );
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

int PMI_Spawn_multiple(int count, const char *cmds[], const char **argvs[], 
                       const int *maxprocs, const void *info, int *errors, 
                       int *same_domain, const void *preput_info)
{
    int  rc;
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    char *remote_kvsname = (char *) info;

    /* PMIU_printf( 1, "PMI_Spawn_multiple not implemented yet\n" ); */
    /* printf("CMD0=:%s:\n",cmds[0]); */
    /* printf("ARG00=:%s:\n",argvs[0][0]); */
    /* snprintf( buf, PMIU_MAXLINE, "cmd=spawn execname=/bin/hostname nprocs=1\n" ); */
    snprintf( buf, PMIU_MAXLINE, "cmd=spawn nprocs=%d execname=%s arg=%s\n",
	      count, cmds[0], argvs[0][0] );
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf ); 
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    PMIU_getval( "remote_kvsname", remote_kvsname, PMIU_MAXLINE );
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

int PMI_Args_to_info(int *argcp, char ***argvp, void *infop)
{
    return ( 0 );
}

/********************* Internal routines not part of PMI interface *****************/

/* get a keyval pair by specific index */

static int PMII_iter( const char *kvsname, const int idx, int *next_idx, char *key, char *val )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];
    int  rc;

    snprintf( buf, PMIU_MAXLINE, "cmd=getbyidx kvsname=%s idx=%d\n", kvsname, idx  );
    PMIU_writeline( PMI_fd, buf );
    PMIU_readline( PMI_fd, buf, PMIU_MAXLINE );
    PMIU_parse_keyvals( buf );
    PMIU_getval( "cmd", cmd, PMIU_MAXLINE );
    if ( strncmp( cmd, "getbyidx_results", PMIU_MAXLINE ) != 0 ) {
	PMIU_printf( 1, "got unexpected response to getbyidx :%s:\n", buf );
	return( -1 );
    }
    else {
	PMIU_getval( "rc", buf, PMIU_MAXLINE );
	rc = atoi( buf );
	if ( rc == 0 ) {
	    PMIU_getval( "nextidx", buf, PMIU_MAXLINE );
	    *next_idx = atoi( buf );
	    PMIU_getval( "key", key, PMI_keylen_max );
	    PMIU_getval( "val", val, PMI_vallen_max );
	    return( 0 );
	}
	else {
	    PMIU_getval( "reason", buf, PMIU_MAXLINE );
	    if ( strncmp( buf, "no_more_keyvals", PMIU_MAXLINE ) == 0 )
		return( -2 );
	    else {
		PMIU_printf( 1, "iter failed; reason = %s\n", buf );
		return( -1 );
	    }
	}
    }
}

/* to get all maxes in one message */
static int PMII_getmaxes( int *kvsname_max, int *keylen_max, int *vallen_max )
{
    char buf[PMIU_MAXLINE], cmd[PMIU_MAXLINE];

    PMIU_writeline( PMI_fd, "cmd=init\n" );
    PMIU_writeline( PMI_fd, "cmd=get_maxes\n" );
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
