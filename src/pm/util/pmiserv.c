/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* 
   Still to do:
   Register group membership so that it is separate from the
   process manager's list of processes

   FIXME: also get any new spawn commands and other changes.
 */
/*
 * This is a simple PMI server implementation.  This file implements
 * the PMI calls, including the PMI key value spaces.  This implements the
 * "server" end of the interface defined in mpich2/src/pmi/simple .
 */

#include "pmutilconf.h"
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* pmutil.h includes pmiserv.h */
#include "pmutil.h"
#include "simple_pmiutil.h"
#define DBG_PRINTF printf

#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
#endif

/*
 * The following structures and arrays are used to implement the PMI 
 * interface.  The global variables are declared static so that they'll
 * have the correct scope if we remove these routines from the mpiexec.c file.
 */

/* ----------------------------------------------------------------------- */
/* PMIState                                                                */
/* Many of the objects here are preallocated rather than being dynamically */
/* allocated as needed.  This ensures that no program fails because a      */
/* malloc fails.                                                           */
/* ----------------------------------------------------------------------- */
#include <sys/types.h>

#define MAXGROUPS   256		/* max number of groups */
#define MAXKEYLEN    64		/* max length of key in keyval space */
#define MAXVALLEN   128		/* max length of value in keyval space */
#define MAXPAIRS   1024		/* max number of pairs in a keyval space */
#define MAXKVSS      16		/* max number of keyval spaces */

#define MAXPMICMD   256         /* max length of a PMI command */

#define MAXGROUPSIZE 256        /* max size of a group */
/* Each group has a size, name, and supports a barrier operation that
   is implemented by counting the number in the barrier */
typedef struct {
    int  groupsize;
    int  fds[MAXGROUPSIZE];     /* The fds for sending messages to all
				   members of this group */
    pid_t pids[MAXGROUPSIZE];   /* The corresponding pids of the group 
				   members */
    int  num_in_barrier;
    char kvsname[MAXKVSNAME];
} PMIGroup;

/* key-value pairs are used to communicate information between processes. */
typedef struct {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
} PMIKeyVal;

/* There can be multiple key-value spaces, each with its own set of 
   PMIKeyVal pairs */
typedef struct {
    int active;
    char kvsname[MAXKVSNAME];
    PMIKeyVal pairs[MAXPAIRS];
} PMIKeyValSpace;

/* This structure contains all of the PMI state, including all
   of the groups and kvs tables */
typedef struct {
    int            nextnewgroup;
    int            kvsid;
    PMIGroup       grouptable[MAXGROUPS];
    PMIKeyValSpace kvstable[MAXKVSS];
    char           kvsname[MAXKVSNAME];
} PMIState;

static PMIState pmi = { 0, 0, };   /* Arrays are uninitialized */

static int pmidebug = 1;

/* Functions that handle PMI requests */
static int  fPMI_Allocate_kvs( int *, char [] );
static int  fPMI_Allocate_kvs_group( void );

static int fPMI_Handle_barrier( PMIProcess * );
static int fPMI_Handle_create_kvs( PMIProcess * );
static int fPMI_Handle_destroy_kvs( PMIProcess * );
static int fPMI_Handle_put( PMIProcess * );
static int fPMI_Handle_get( PMIProcess * );
static int fPMI_Handle_get_my_kvsname( PMIProcess * );
static int fPMI_Handle_init( PMIProcess * );
static int fPMI_Handle_get_maxes( PMIProcess * );
static int fPMI_Handle_getbyidx( PMIProcess * );
static int fPMI_Handle_init_port( PMIProcess * );
static int fPMI_Handle_spawn( PMIProcess * );

int PMIServHandleInput( int fd, void *extra );

typedef struct {
    ProcessState *pstate;
    /* ProcessState contains a PMIProcess member */
} PMIData;


/*
 * All PMI commands are handled by calling a routine that is associated with
 * the command.  New PMI commands can be added by updating this table.
 */
typedefs struct {
    const char cmdName[];
    int (*handler)( void * );
} PMICmdMap;

static PMICmdMap pmiCommands[] = { 
    { "barrier_in",     fPMI_Handle_barrier },
    { "finalize",       fPMI_Handle_finalize },
    { "abort",          fPMI_Handle_abort },
    { "create_kvs",     fPMI_Handle_create_kvs },
    { "destroy_kvs",    fPMI_Handle_destroy_kvs }, 
    { "put",            fPMI_Handle_put },
    { "get",            fPMI_Handle_get },
    { "get_my_kvsname", fPMI_Handle_get_my_kvsname },
    { "init",           fPMI_Handle_init },
    { "get_maxes",      fPMI_Handle_get_maxes },
    { "getbyidx",       fPMI_Handle_getbyidx },
    { "initack",        fPMI_Handle_init_port },
    { "spawn",          fPMI_Handle_spawn },
    { "\0",             0 },                     /* Sentinal for end of list */
};

/*
 * Initialize the information needed to communicate the the PMI client 
 * process (usually an MPI job).
 */
int IOSetupPMIHandler( IOSpec *ios, int fd, ProcessState *pstate,
		       int pmigroup, int np, int rank )
{
    PMIData *pmidata;

    pmidata = (PMIData *)MPIU_Malloc( sizeof(PMIData) );
    pmidata->pstate			 = pstate;
    pmidata->pstate->pmientry.fd	 = fd;
    pmidata->pstate->pmientry.group	 = pmigroup;
    pmidata->pstate->pmientry.kvs	 = pmi.kvsid;   /* Use current kvsid */
    /* These next two values are used to setup the initial MPI_COMM_WORLD
       on a created group of processes */
    pmidata->pstate->pmientry.nProcesses = np;
    pmidata->pstate->pmientry.rank	 = rank;
    
    ios->extra_state			 = (void *)pmidata;
    ios->isWrite			 = 0;
    ios->handler			 = PMIServHandleInput;
    ios->fd				 = fd;

    return 0;
}
/* 
 * Process input from the socket connecting the mpiexec process to the
 * child process.
 * The return status indicates the state of the process that this
 * handler interacted with.
 */
int PMIServHandleInput( int fd, void *extra )
{
    PMIData *pmidata = (PMIData *)extra;
    int     err;

    err = PMIServHandleInputFd( fd, -1, &pmidata->pstate->pmientry );

    if (err == PMI_DIED) return 0;
    
    return 1;
}

int PMIServHandleInputFd ( int fd, int pidx, void *extra )
{
    PMIProcess *pentry = (PMIProcess *)extra;
    int  rc;
    int  returnCode = 0;
    char inbuf[PMIU_MAXLINE], outbuf[PMIU_MAXLINE], cmd[MAXPMICMD];
    PMICmdMap *p;

    DBG_PRINTF( "Handling PMI input\n" ); fflush(stdout);
    if ( ( rc = PMIU_readline( pentry->fd, inbuf, PMIU_MAXLINE ) ) > 0 ) {
	if (pmidebug) {
	    DBG_PRINTF( "Entering PMIServHandleInputFd %s\n", inbuf );
	}

	PMIU_parse_keyvals( inbuf );
	PMIU_getval( "cmd", cmd, MAXPMICMD );
	DBG_PRINTF( "cmd = %s\n", cmd ); fflush(stdout);
	p = pmiCommands;
	while (p->handle) {
	    if (strncmp( cmd, p->name, MAXPMICMD ) == 0) {
		rc = (p->handle)( pentry );
		break;
	    }
	    p++;
	}
	if (!p->handle) {
	    PMIU_printf( 1, "unknown cmd %s\n", cmd );
	}

#if 0
	if ( strncmp( cmd, "barrier_in", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_barrier( pentry );
	}
	else if ( strncmp( cmd, "finalize", MAXPMICMD ) == 0 ) {
	    returnCode = PMI_FINALIZED;
	}
	else if ( strncmp( cmd, "abort", MAXPMICMD ) == 0 ) {
	    /* No PMI abort command has yet been implemented! */
	    returnCode = PMI_ALLEXIT;
	}
	else if ( strncmp( cmd, "get_my_kvsname", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_get_my_kvsname( pentry );
	}
	else if ( strncmp( cmd, "init", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_init( pentry );
	}
	else if ( strncmp( cmd, "get_maxes", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_get_maxes( pentry );
	}
	else if ( strncmp( cmd, "create_kvs", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_create_kvs( pentry );
	}
	else if ( strncmp( cmd, "destroy_kvs", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_destroy_kvs( pentry );
	}
	else if ( strncmp( cmd, "put", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_put( pentry );
	}
	else if ( strncmp( cmd, "get", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_get( pentry );
	}
	else if ( strncmp( cmd, "getbyidx", MAXPMICMD ) == 0 ) {
	    fPMI_Handle_getbyidx( pentry );
	}
	else if ( strncmp( cmd, "initack", MAXPMICMD ) == 0) {
	    fPMI_Handle_init_port( pentry );
	}
	else if (strncmp( cmd, "spawn", MAXPMICMD ) == 0) {
	    fPMI_Handle_spawn( pentry );
	}
	else {
	    PMIU_printf( 1, "unknown cmd %s\n", cmd );
	}
    }
    else {                        /* lost contact with client */
	/* close( pentry->fd );  */
	pentry->fd = -1;          /* forget the fd now that it is not valid */
	returnCode = PMI_DIED;
    }
    return returnCode;
}

/*
 * Perform any initialization.
 * Input
 * nprocs - Initial number of processes to create (size of initial group)
 * Output
 * kvsname is the initial kvs name (provide a string of size MAXKVSNAME.
 * Return value: groupid
 */
int PMIServInit( int nprocs )
{
    int i;
    for ( i = 0; i < MAXKVSS; i++ )
	pmi.kvstable[i].active = 0;

    /* set up group */
    pmi.grouptable[pmi.nextnewgroup].groupsize = nprocs;
    for (i=0; i<nprocs; i++) {
	pmi.grouptable[pmi.nextnewgroup].fds[i] = -1;
    }

    /* set up keyval space for this group */
    fPMI_Allocate_kvs( &pmi.kvsid, pmi.kvsname );

    return pmi.nextnewgroup++;   /* ++ missing in forker ? */
}

/* 
 * Initialize an entry as empty
 */
int PMIServInitEntry( PMIProcess *pmientry )
{
    pmientry->fd         = -1;
    pmientry->group      = -1;
    pmientry->kvs        = -1;
    pmientry->nProcesses = 1;
    pmientry->rank       = 0;

    return 0;
}

/*
 * Setup an entry for a created process
 */
int PMIServSetupEntry( int pmifd, int pmigroup, int np, int rank, 
		       PMIProcess *pmientry )
{
    pmientry->fd         = pmifd;
    pmientry->group      = pmigroup;
    pmientry->kvs        = pmi.kvsid;   /* Use current kvsid */
    pmientry->nProcesses = np;
    pmientry->rank       = rank;

    return 0;
}

/*
 * Add this process to a pmi group
 */
int PMIServAddtoGroup( int group, int rank, pid_t pid, int fd )
{
    pmi.grouptable[group].fds[rank]  = fd;
    pmi.grouptable[group].pids[rank] = pid;

    return 0;
}
/* ------------------------------------------------------------------------- */
/* The rest of these routines are internal                                   */
/* ------------------------------------------------------------------------ */
/* kvsname and kvsid is output */
static int fPMI_Allocate_kvs( int *kvsid, char kvsname[] )
{
    int i, j;
    
    for ( i = 0; i < MAXKVSS; i++ )
	if ( !pmi.kvstable[i].active )
	    break;

    if ( i >= MAXKVSS ) {
	MPIU_Internal_error_printf( stderr, "too many kvs's\n" );
	return( -1 );
    }
    else {
	pmi.kvstable[i].active = 1;
	for ( j = 0; j < MAXPAIRS; j++ ) {
	    pmi.kvstable[i].pairs[j].key[0] = '\0';
	    pmi.kvstable[i].pairs[j].val[0] = '\0';
	}
	MPIU_Snprintf( pmi.kvstable[i].kvsname, MAXKVSNAME, "kvs_%d", i );
	MPIU_Strncpy( kvsname, pmi.kvstable[i].kvsname, MAXKVSNAME ); 
	*kvsid = i;
	return( 0 );
    }
}

static int fPMI_Allocate_kvs_group( void )
{
    return pmi.nextnewgroup++;
}

/* 
 * Handle an incoming "barrier" command
 *
 * Need a structure that has the fds for all members of a pmi group
 */
static int fPMI_Handle_barrier( PMIProcess *pentry )
{
    int i;
    int group = pentry->group;

    if (pmidebug) {
	DBG_PRINTF( "Entering PMI_Handle_barrier for group %d\n", group );
    }

    pmi.grouptable[group].num_in_barrier++;
    if ( pmi.grouptable[group].num_in_barrier ==
	 pmi.grouptable[group].groupsize ) {
	for ( i = 0; i < pmi.grouptable[group].groupsize; i++ ) {
	    PMIU_writeline(pmi.grouptable[group].fds[i], 
			   "cmd=barrier_out\n" );
	}
	pmi.grouptable[group].num_in_barrier = 0;
    }
    return 0;
}

/* 
 * Handle an incoming "create_kvs" command
 */
static int fPMI_Handle_create_kvs( PMIProcess *pentry )
{
    int  kvsidx;
    char kvsname[MAXKVSNAME], outbuf[PMIU_MAXLINE];

    fPMI_Allocate_kvs( &kvsidx, kvsname );
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=newkvs kvsname=%s\n", kvsname );
    PMIU_writeline( pentry->fd, outbuf );
    if (pmidebug) {
	DBG_PRINTF( "Handle_create_kvs new %d name %s\n", kvsidx, kvsname );
    }
    return 0;
}

/* 
 * Handle an incoming "destroy_kvs" command 
 */
static int fPMI_Handle_destroy_kvs( PMIProcess *pentry )
{
    int  i, rc=0;
    char kvsname[MAXKVSNAME];
    char message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    for ( i = 0; i < MAXKVSS; i++ ) {
	if ( strncmp( pmi.kvstable[i].kvsname, kvsname, MAXKVSNAME ) == 0 ) {
	    if ( pmi.kvstable[i].active ) {
		pmi.kvstable[i].active = 0;
		MPIU_Snprintf( message, PMIU_MAXLINE,
			  "KVS_%s_successfully_destroyed", kvsname );
		rc = 0;
	    }
	    else {
		MPIU_Snprintf( message, PMIU_MAXLINE, "KVS_%s_previously_destroyed",
			  kvsname );
		rc = -1;
	    }
	    break;
	}
    }
    if ( i == MAXKVSS ) {
	rc = -1;
	MPIU_Snprintf( message, PMIU_MAXLINE, "KVS %s not found", kvsname );
    }
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=kvs_destroyed rc=%d msg=%s\n",
	      rc, message );
    PMIU_writeline( pentry->fd, outbuf );
    return 0;
}

/* 
 * Handle an incoming "put" command
 */
static int fPMI_Handle_put( PMIProcess *pentry )
{
    int  i, j, rc=0;
    char kvsname[MAXKVSNAME];
    char message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    char key[MAXKEYLEN];

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    if (pmidebug) {
	DBG_PRINTF( "Put: Finding kvs %s\n", kvsname );
    }

    for ( i = 0; i < MAXKVSS; i++ ) {
	if ( pmi.kvstable[i].active &&
	     strncmp( pmi.kvstable[i].kvsname, kvsname, MAXKVSNAME ) == 0 ) {
	    /* should check here for duplicate key and raise error */
	    PMIU_getval( "key", key, MAXKEYLEN );
	    for ( j = 0; j < MAXPAIRS; j++ ) {
		if ( strncmp( pmi.kvstable[i].pairs[j].key, key, MAXKEYLEN ) == 0 ) {
		    rc = -1;          /* no duplicate keys allowed */
		    MPIU_Snprintf( message, PMIU_MAXLINE, "duplicate_key %s", key );
		    break;
		}
		else if ( strncmp( pmi.kvstable[i].pairs[j].key, "", MAXKEYLEN ) == 0 ) {
		    PMIU_getval( "key", pmi.kvstable[i].pairs[j].key,
				 MAXKEYLEN );
		    PMIU_getval( "value", pmi.kvstable[i].pairs[j].val,
				 MAXVALLEN );
		    rc = 0;
		    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
		    break;
		}
	    }
	    if ( j == MAXPAIRS ) {
		rc = -1;
		MPIU_Snprintf( message, PMIU_MAXLINE, "no_room_in_kvs_%s",
			  kvsname );
	    }
	}
	break;
    }
    if ( i == MAXKVSS ) {
	rc = -1;
	MPIU_Snprintf( message, PMIU_MAXLINE, "kvs_%s_not_found", kvsname );
    }
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=put_result rc=%d msg=%s\n",
	      rc, message );
    PMIU_writeline( pentry->fd, outbuf );
    return 0;
}

/*
 * Handle incoming "get" command
 */
static int fPMI_Handle_get( PMIProcess *pentry )
{
    int  i, j, rc=0;
    char kvsname[MAXKVSNAME];
    char message[PMIU_MAXLINE], key[PMIU_MAXLINE], value[PMIU_MAXLINE];
    char outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    if (pmidebug) {
	DBG_PRINTF( "Get: Finding kvs %s\n", kvsname );
    }
    for ( i = 0; i < MAXKVSS; i++ ) {
	if ( pmi.kvstable[i].active &&
	     strncmp( pmi.kvstable[i].kvsname, kvsname, MAXKVSNAME ) == 0 ) {
	    PMIU_getval( "key", key, PMIU_MAXLINE );
	    for ( j = 0; j < MAXPAIRS; j++ ) {
		if ( strncmp( pmi.kvstable[i].pairs[j].key, key, MAXKEYLEN ) == 0 ) {
		    rc = 0;
		    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
		    MPIU_Strncpy( value, pmi.kvstable[i].pairs[j].val, 
				  PMIU_MAXLINE );
		    break;
		}
	    }
	    if ( j == MAXPAIRS ) {
		rc = -1;
		MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
		MPIU_Snprintf( message, PMIU_MAXLINE, "key_%s_not_found", kvsname );
	    }
	}
	break;
    }
    if ( i == MAXKVSS ) {
	rc = -1;
	MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
	MPIU_Snprintf( message, PMIU_MAXLINE, "kvs_%s_not_found", kvsname );
    }
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=get_result rc=%d msg=%s value=%s\n",
	      rc, message, value );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTF( "%s", outbuf );
    return rc;
}

/* Handle an incoming get_my_kvsname command */
static int fPMI_Handle_get_my_kvsname( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=my_kvsname kvsname=%s\n",
	      pmi.kvstable[pentry->kvs].kvsname );
    PMIU_writeline( pentry->fd, outbuf );
    return 0;
}

/* Handle an incoming "init" command */
static int fPMI_Handle_init( PMIProcess *pentry )
{
    /* nothing to do at present */
    return 0;
}

/* Handle an incoming "get_maxes" command */
static int fPMI_Handle_get_maxes( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    MPIU_Snprintf( outbuf, PMIU_MAXLINE,
	      "cmd=maxes kvsname_max=%d keylen_max=%d vallen_max=%d\n",
	      MAXKVSNAME, MAXKEYLEN, MAXVALLEN );
    PMIU_writeline( pentry->fd, outbuf );
    return 0;
}

/*
 * Handle incoming "getbyidx" command
 */
static int fPMI_Handle_getbyidx( PMIProcess *pentry )
{
    int i, j, rc=0;
    char kvsname[MAXKVSNAME], j_char[8], outbuf[PMIU_MAXLINE];

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    for ( i = 0; i < MAXKVSS; i++ ) {
	if ( pmi.kvstable[i].active &&
	     strncmp( pmi.kvstable[i].kvsname, kvsname, MAXKVSNAME ) == 0 ) {
	    PMIU_getval( "idx", j_char, 8 );
	    j = atoi( j_char );
	    if ( ( j > MAXPAIRS ) ||
		 strncmp( pmi.kvstable[i].pairs[j].key, "", MAXKEYLEN ) == 0 ) {
		MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
			  "reason=no_more_keyvals\n" );
	    }
	    else {
		MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results "
			  "rc=0 nextidx=%d key=%s val=%s\n",
			  j + 1,
			  pmi.kvstable[i].pairs[j].key,
			  pmi.kvstable[i].pairs[j].val );
	    }
	}
	break;
    }
    if ( i == MAXKVSS ) {
	rc = -1;
	MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
		  "reason=kvs_%s_not_found\n", kvsname );
    }
    PMIU_writeline( pentry->fd, outbuf );
    return rc;
}

static int fPMI_Handle_init_port( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];

    if (pmidebug) {
	DBG_PRINTF( "Entering fPMI_Handle_init to start connection\n" );
    }

    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set size=%d\n", pentry->nProcesses );
    PMIU_writeline( pentry->fd, outbuf );
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set rank=%d\n", pentry->rank );
    PMIU_writeline( pentry->fd, outbuf );
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set debug=%d\n", 1 );
    PMIU_writeline( pentry->fd, outbuf );
    return 0;
}

static int fPMI_Handle_spawn( PMIProcess *pentry )
{
    /* Input:
       nprocs=%d execname=%s arg=%s

       Output:
       cmd=spawn_result remote_kvsname=name rc=integer
    */
    return 0;
}

/*  
 * FIXME:
 * Question: What does this need to do?
 * 1.  Is nproces in range?
 * 2.  Is the program executable?
 * 3.  Create the KVS group
 * 4.  Invoke startup -> the process startup procedure must have
 *     been registered by the calling program (e.g., usually by
 *     mpiexec, but possibly a separate pmiserver process)
 * 5. return kvsname; return code
 *    How do we handle soft (no specific return size required).
 *    Also, is part fo the group associated with these processes or
 *    another group (the spawner?) of processes?
 */
void PMI_Init_remote_proc( int fd, PMIProcess *pentry,
			   int rank, int np, int debug )
{
    pentry->fd	       = fd;
    pentry->nProcesses = np;
    pentry->rank       = rank;
    pentry->group      = 0;
    PMIU_writeline( fd, "cmd=initack\n" );
    fPMI_Handle_init_port( pentry );
}
/* 
 * This is a special routine.  It accepts the first input from the
 * remote process, and returns the PMI_ID value.  -1 is returned on error 
 */
int PMI_Init_port_connection( int fd )
{
    char message[PMIU_MAXLINE], cmd[MAXPMICMD];
    int pmiid = -1;

    if (pmidebug) {
	DBG_PRINTF( "Beginning initial handshake read\n" ); fflush(stdout);
    }
    PMIU_readline( fd, message, PMIU_MAXLINE );
    if (pmidebug) {
	DBG_PRINTF( "received message %s\n", message );fflush(stdout);
    }
    PMIU_parse_keyvals( message );
    PMIU_getval( "cmd", cmd, MAXPMICMD );
    if (strcmp(cmd,"initack")) {
	PMIU_printf( 1, "Unexpected cmd %s\n", cmd );
	return -1;
    }
    PMIU_getval( "pmiid", cmd, MAXPMICMD );
    pmiid = atoi(cmd);

    return pmiid;
}
