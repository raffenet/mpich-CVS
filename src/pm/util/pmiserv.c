/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#include <string.h>
#include <ctype.h>
/* Use the memory defintions from mpich2/src/include */
#include "mpimem.h"
#include "pmutil.h"
#include "process.h"
#include "cmnargs.h"
#include "ioloop.h"
#include "pmiserv.h"

/* We need socket to create a socket pair */
#include <sys/socket.h>

/* ??? */
#include "simple_pmiutil.h"

#ifdef HAVE_SNPRINTF
#define MPIU_Snprintf snprintf
#ifdef NEEDS_SNPRINTF_DECL
int snprintf(char *, size_t, const char *, ...);
#endif
#endif

/* These need to be imported from the pmiclient */
#define MAXPMICMD   256         /* max length of a PMI command */

/* There is only a single PMI master, so we allocate it here */
static PMIMaster pmimaster = { 0, 0, 0 };
/* Allow the user to register a routine to be used for the PMI spawn 
   command */
static int (*userSpawner)( ProcessWorld *, void *) = 0;
static void *userSpawnerData = 0;
static int pmidebug = 1;

#if 0
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
#endif

/* Functions that handle PMI requests */
static int fPMI_Handle_finalize( PMIProcess * );
static int fPMI_Handle_abort( PMIProcess * );
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
static int fPMI_Handle_get_universe_size( PMIProcess * );
static int fPMI_Handle_get_appnum( PMIProcess * );

static PMIKVSpace *fPMIKVSAllocate( void );

int PMIServHandleInput( int, int, void * );

typedef struct {
    ProcessState *pstate;
    /* ProcessState contains a PMIProcess member */
} PMIData;


/*
 * All PMI commands are handled by calling a routine that is associated with
 * the command.  New PMI commands can be added by updating this table.
 */
typedef struct {
    char *cmdName;
    int (*handler)( PMIProcess * );
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
    { "get_universe_size", fPMI_Handle_get_universe_size },
    { "get_appnum",     fPMI_Handle_get_appnum },
    { "\0",             0 },                     /* Sentinal for end of list */
};

/* ------------------------------------------------------------------------- */

/*
 * Create a socket fd and setup the handler on that fd.
 * FIXME: need to close the fds in the child, parent (child closes 0, parent
 * closes 1.  Child process sends fd # in PMI_FD to exec child through 
 * environment.
 * Need a PMISetup typedef
 */
int PMISetupSockets( int kind, PMISetup *pmiinfo )
{
    if (kind == 0) {
	socketpair( AF_UNIX, SOCK_STREAM, 0, pmiinfo->fdpair );
    }
    else {
	return 1;
    }
    return 0;
}

/* 
 * This is the client side of the PMIserver setup.  It communicates to the
 * client the information needed to connect to the server (currently the
 * FD of a pre-existing socket).
 */
int PMISetupInClient( PMISetup *pmiinfo )
{
    char env_pmi_fd[100];

    close( pmiinfo->fdpair[0] );
    MPIU_Snprintf( env_pmi_fd, sizeof(env_pmi_fd), "PMI_FD=%d" , 
	      pmiinfo->fdpair[1] );
    if (putenv( env_pmi_fd )) {
	MPIU_Internal_error_printf( "Could not set environment PMI_FD" );
	perror( "Reason: " );
	return 1;
    }
    /* Indicate that this is a spawned process */
    /* putenv( "PMI_SPAWNED=1" ); */
    return 0;
}

/* Finish setting up the server end of the PMI interface */
int PMISetupFinishInServer( PMISetup *pmiinfo, ProcessState *pState )
{
    PMIProcess *pmiprocess;

    /* Close the other end of the socket pair */
    close( pmiinfo->fdpair[1] );

    /* We must initialize this process in the list of PMI processes. We
       pass the PMIProcess information to the handler */
    pmiprocess = PMISetupNewProcess( pmiinfo->fdpair[0], pState );
    MPIE_IORegister( pmiinfo->fdpair[0], IO_READ, PMIServHandleInput, 
		     pmiprocess );

    return 0;
}

static PMIGroup *curPMIGroup = 0;
static int       curNprocess = 0;

PMIProcess *PMISetupNewProcess( int fd, ProcessState *pState )
{
    PMIProcess *pmiprocess;

    pmiprocess = (PMIProcess *)MPIU_Malloc( sizeof(PMIProcess) );
    pmiprocess->fd     = fd;
    pmiprocess->group  = curPMIGroup;
    pmiprocess->pState = pState;

    /* Add this process to the curPMIGroup */
    curPMIGroup->pmiProcess[curNprocess++] = pmiprocess;

    return pmiprocess;
}

/* Initialize a new PMI group that will be the parent of all
   PMIProcesses until the next group is created 
   Each group also starts with a KV Space */
int PMISetupNewGroup( int nProcess, PMIKVSpace *kvs )
{
    PMIGroup *g;
    curPMIGroup = (PMIGroup *)MPIU_Malloc( sizeof(PMIGroup) );

    curPMIGroup->nProcess   = nProcess;
    curPMIGroup->groupID    = pmimaster.nGroups++;
    curPMIGroup->nInBarrier = 0;
    curPMIGroup->pmiProcess = (PMIProcess **)MPIU_Malloc( 
					 sizeof(PMIProcess*) * nProcess );
    curPMIGroup->nextGroup  = 0;
    curNprocess             = 0;

    /* Add to PMIMaster */
    g = pmimaster.groups;
    if (!g) {
	pmimaster.groups = curPMIGroup;
    }
    else {
	while (g) {
	    if (!g->nextGroup) {
		g->nextGroup = curPMIGroup;
		break;
	    }
	    g = g->nextGroup;
	}
    }
    if (kvs) {
	curPMIGroup->kvs = kvs;
    }
    else {
	curPMIGroup->kvs = fPMIKVSAllocate();
    }

    return 0;
}


#if 0
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
#endif

/* 
 * Process input from the socket connecting the mpiexec process to the
 * child process.
 *
 * The return status is interpreted by the IOLoop code in ioloop.c ;
 * a zero is a normal exit.
 */
int PMIServHandleInput( int fd, int rdwr, void *extra )
{
    PMIProcess *pentry = (PMIProcess *)extra;
    int        rc;
    int        returnCode = 0;
    char       inbuf[PMIU_MAXLINE], cmd[MAXPMICMD];
    PMICmdMap *p;
    int        cmdtype;

    DBG_PRINTFCOND(pmidebug,("Handling PMI input\n") );
    if ( ( rc = PMIU_readline( pentry->fd, inbuf, PMIU_MAXLINE ) ) > 0 ) {
	DBG_PRINTFCOND(pmidebug,
		       ("Entering PMIServHandleInputFd %s\n", inbuf) );

	PMIU_parse_keyvals( inbuf );
	cmdtype = PMIGetCommand( cmd, sizeof(cmd) );
	DBG_PRINTFCOND(pmidebug,( "cmd = %s\n", cmd ));
	p = pmiCommands;
	while (p->handler) {
	    if (strncmp( cmd, p->cmdName, MAXPMICMD ) == 0) {
		rc = (p->handler)( pentry );
		break;
	    }
	    p++;
	}
	if (!p->handler) {
	    PMIU_printf( 1, "unknown cmd %s\n", cmd );
	}
    }
    else {                        /* lost contact with client */
	DBG_PRINTFCOND(pmidebug,("EOF on PMI input\n"));
	/* returning a 1 causes the IO loop code to close the socket */
	returnCode = 1;
    }
    return returnCode;
}

/* ------------------------------------------------------------------------- */
/*
 * Perform any initialization.
 * Input
 * nprocs - Initial number of processes to create (size of initial group)
 * Output
 * kvsname is the initial kvs name (provide a string of size MAXKVSNAME.
 * Return value: groupid
 */
int PMIServInit( int (*spawner)(ProcessWorld *, void*), void * spawnerData )
{
    userSpawner     = spawner;
    userSpawnerData = spawnerData;

    return 0;
}
#if 0
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
#endif


/* ------------------------------------------------------------------------ */
/* Additional service routines                                              */
/* ------------------------------------------------------------------------ */
/*
 * Get either a cmd=val or mcmd=val.  return 0 if cmd, 1 if mcmd, and -1 
 * if neither (an error, since all PMI messages should contain one of
 * these).
 */
int PMIGetCommand( char *cmd, int cmdlen )
{
    char *valptr;
    int  cmdtype = 0;
    
    valptr = PMIU_getval( "cmd", cmd, cmdlen );
    if (!valptr) {
	valptr = PMIU_getval( "mcmd", cmd, cmdlen );
	if (valptr) cmdtype = 1;
	else        cmdtype = -1;
    }
    return cmdtype;
}

/* ------------------------------------------------------------------------- */
/* The rest of these routines are internal                                   */
/* ------------------------------------------------------------------------ */
#if 0
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
#endif

/* ------------------------------------------------------------------------- */
/* 
 * Handle an incoming "barrier" command
 *
 * Need a structure that has the fds for all members of a pmi group
 */
static int fPMI_Handle_finalize( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    
    pentry->pState->status = PROCESS_FINALIZED;

    /* send back an acknowledgement to release the process */
    snprintf(outbuf, PMIU_MAXLINE, "cmd=finalize_ack\n");
    PMIU_writeline(pentry->fd, outbuf);
    
    return 0;
}
static int fPMI_Handle_abort( PMIProcess *pentry )
{
    return 1;
}
static int fPMI_Handle_barrier( PMIProcess *pentry )
{
    int i;
    PMIGroup *group = pentry->group;

    DBG_PRINTFCOND(pmidebug,("Entering PMI_Handle_barrier for group %d\n", 
		    group->groupID) );

    group->nInBarrier++;
    if (group->nInBarrier == group->nProcess) {
	for ( i=0; i<group->nProcess; i++) {
	    PMIU_writeline(group->pmiProcess[i]->fd, "cmd=barrier_out\n" );
	}
	group->nInBarrier = 0;
    }
    return 0;
}

/* Create a kvs and return a pointer to it */
static PMIKVSpace *fPMIKVSAllocate( void )
{
    PMIKVSpace *kvs, **kPrev, *k;
    int        rc;
    static int kvsnum = 0;    /* Used to generate names */

    /* Create the space */
    kvs = (PMIKVSpace *)MPIU_Malloc( sizeof(PMIKVSpace) );
    if (!kvs) {
	MPIU_Internal_error_printf( stderr, "too many kvs's\n" );
	return 0;
    }
    MPIU_Snprintf( (char *)(kvs->kvsname), MAXNAMELEN, "kvs_%d", kvsnum++ );
    kvs->pairs     = 0;
    kvs->lastByIdx = 0;
    kvs->lastIdx   = -1;

    /* Insert into the list of KV spaces */
    kPrev = &pmimaster.kvSpaces;
    k     = pmimaster.kvSpaces;
    while (k) {
	rc = strcmp( k->kvsname, kvs->kvsname );
	if (rc > 0) break;
	kPrev = &k->nextKVS;
	k     = k->nextKVS;
    }
    kvs->nextKVS = k;
    *kPrev = kvs;

    return kvs;
}
/* Create a kvs and generate its name; return that name as the argument */
static int fPMIKVSGetNewSpace( char kvsname[], int maxlen )
{
    PMIKVSpace *kvs;

    kvs = fPMIKVSAllocate( );
    MPIU_Strncpy( kvsname, kvs->kvsname, maxlen );
    return 0;
}
static int fPMIKVSFindKey( PMIKVSpace *kvs, 
			   const char key[], char val[], int maxval )
{
    PMIKVPair *p;
    int       rc;

    p = kvs->pairs;
    while (p) {
	rc = strcmp( p->key, key );
	if (rc == 0) {
	    /* Found it.  Get the value and return success */
	    MPIU_Strncpy( val, p->val, maxval );
	    return 0;
	}
	if (rc > 0) {
	    /* We're past the point in the sorted list where the
	       key could be found */
	    return 1;
	}
	p = p->nextPair;
    }
    return 1;
}

static int fPMIKVSAddPair( PMIKVSpace *kvs, 
			   const char key[], const char val[] )
{
    PMIKVPair *pair, *p, **pprev;
    int       rc;

    /* Find the location in which to insert the pair (if the
       same key already exists, that is an error) */
    p = kvs->pairs;
    pprev = &(kvs->pairs);
    while (p) {
	rc = strcmp( p->key, key );
	if (rc == 0) {
	    /* Duplicate.  Indicate an error */
	    return 1;
	}
	if (rc > 0) {
	    /* We've found the location (after pprev, before p) */
	    break;
	}
	pprev = &(p->nextPair);
	p = p->nextPair;
    }
    pair = (PMIKVPair *)MPIU_Malloc( sizeof(PMIKVPair) );
    if (!pair) {
	return -1;
    }
    MPIU_Strncpy( pair->key, key, sizeof(pair->key) );
    MPIU_Strncpy( pair->val, val, sizeof(pair->val) );

    /* Insert into the list */
    pair->nextPair = p;
    *pprev         = pair;

    /* Since the list has been modified, clear the index helpers */
    kvs->lastByIdx = 0;
    kvs->lastIdx   = -1;

    return 0;
}

static PMIKVSpace *fPMIKVSFindSpace( const char kvsname[] )
{
    PMIKVSpace *kvs = pmimaster.kvSpaces;
    int rc;

    /* We require the kvs spaces to be stored in a sorted order */
    while (kvs) {
	rc = strcmp( kvs->kvsname, kvsname );
	if (rc == 0) {
	    /* Found */
	    return kvs;
	}
	if (rc > 0) {
	    /* Did not find (kvsname is sorted after kvs->kvsname) */
	    return 0;
	}
	kvs = kvs->nextKVS;
    }
    /* Did not find */
    return 0;
}
static int PMIKVSFree( PMIKVSpace *kvs )
{
    PMIKVPair *p, *pNext;
    PMIKVSpace **kPrev, *k;
    int        rc;
    
    /* Recover the pairs */
    p = kvs->pairs;
    while (p) {
	pNext = p->nextPair;
	MPIU_Free( p );
	p = pNext;
    }

    /* Recover the KVS space, and remove it from the master's list */
    kPrev = &pmimaster.kvSpaces;
    k     = pmimaster.kvSpaces;
    rc    = 1;
    while (k) {
	rc = strcmp( k->kvsname, kvs->kvsname );
	if (rc == 0) {
	    *kPrev = k->nextKVS;
	    MPIU_Free( k );
	    break;
	}
	kPrev = &(k->nextKVS);
	k = k->nextKVS;
    }
    
    /* Note that if we did not find the kvs, we have an internal 
       error, since all kv spaces are maintained within the pmimaster list */
    if (rc != 0) {
	MPIU_Internal_error_printf( "Could not find KV Space %s\n", 
				    kvs->kvsname );
	return 1;
    }
    return 0;
}
/* 
 * Handle an incoming "create_kvs" command
 */
static int fPMI_Handle_create_kvs( PMIProcess *pentry )
{
    char kvsname[MAXKVSNAME], outbuf[PMIU_MAXLINE];
    int rc;

    rc = fPMIKVSGetNewSpace( kvsname, sizeof(kvsname) );
    if (rc) {
	/* PANIC - allocation failed */
	return 1;
    }
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=newkvs kvsname=%s\n", kvsname );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug, ("Handle_create_kvs new name %s\n", kvsname ) );
    return 0;
}

/* 
 * Handle an incoming "destroy_kvs" command 
 */
static int fPMI_Handle_destroy_kvs( PMIProcess *pentry )
{
    int         rc=0;
    PMIKVSpace *kvs;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIKVSFree( kvs );
	MPIU_Snprintf( message, PMIU_MAXLINE,
		       "KVS_%s_successfully_destroyed", kvsname );
    }
    else {
	MPIU_Snprintf( message, PMIU_MAXLINE, "KVS %s not found", kvsname );
	rc = -1;
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
    int         rc=0;
    PMIKVSpace *kvs;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], outbuf[PMIU_MAXLINE];
    char        key[MAXKEYLEN], val[MAXVALLEN];

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    DBG_PRINTFCOND(pmidebug,( "Put: Finding kvs %s\n", kvsname ) );

    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	/* should check here for duplicate key and raise error */
	PMIU_getval( "key", key, MAXKEYLEN );
	PMIU_getval( "value", val, MAXVALLEN );
	rc = fPMIKVSAddPair( kvs, key, val );
	if (rc == 1) {
	    rc = -1;          /* no duplicate keys allowed */
	    MPIU_Snprintf( message, PMIU_MAXLINE, "duplicate_key %s", key );
	}
	else if (rc == -1) {
	    rc = -1;
	    MPIU_Snprintf( message, PMIU_MAXLINE, "no_room_in_kvs_%s",
			   kvsname );
	}
	else {
	    rc = 0;
	    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
	}
    }
    else {
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
    PMIKVSpace *kvs;
    int         rc=0;
    char        kvsname[MAXKVSNAME];
    char        message[PMIU_MAXLINE], key[PMIU_MAXLINE], value[PMIU_MAXLINE];
    char        outbuf[PMIU_MAXLINE];
    
    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    DBG_PRINTFCOND(pmidebug,( "Get: Finding kvs %s\n", kvsname ) );

    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIU_getval( "key", key, PMIU_MAXLINE );
	rc = fPMIKVSFindKey( kvs, key, value, sizeof(value) );
	if (rc == 0) {
	    rc = 0;
	    MPIU_Strncpy( message, "success", PMIU_MAXLINE );
	}
	else if (rc) {
	    rc = -1;
	    MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
	    MPIU_Snprintf( message, PMIU_MAXLINE, "key_%s_not_found", 
			   kvsname );
	}
    }
    else { 
	rc = -1;
	MPIU_Strncpy( value, "unknown", PMIU_MAXLINE );
	MPIU_Snprintf( message, PMIU_MAXLINE, "kvs_%s_not_found", kvsname );
    }
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, 
		   "cmd=get_result rc=%d msg=%s value=%s\n",
		   rc, message, value );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND( pmidebug, ("%s", outbuf ));
    return rc;
}

/* Handle an incoming get_my_kvsname command */
static int fPMI_Handle_get_my_kvsname( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    PMIKVSpace *kvs;

    kvs = pentry->group->kvs;
    if (kvs && kvs->kvsname) {
	MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=my_kvsname kvsname=%s\n",
		       kvs->kvsname );
    }
    else {
	MPIU_Internal_error_printf( "Group has no associated KVS" );
	return -1;
    }
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming get_universe_size command */
static int fPMI_Handle_get_universe_size( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];
    /* Import the universe size from the process structures */
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=universe_size size=%d\n",
	      pUniv.size );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming get_appnum command */
static int fPMI_Handle_get_appnum( PMIProcess *pentry )
{
    ProcessApp *app = pentry->pState->app;
    char outbuf[PMIU_MAXLINE];
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=appnum appnum=%d\n",
		   app->myAppNum );		
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle an incoming "init" command */
static int fPMI_Handle_init( PMIProcess *pentry )
{
    char version[PMIU_MAXLINE];
    char subversion[PMIU_MAXLINE];
    char outbuf[PMIU_MAXLINE];
    int rc;

    /* check version compatibility with PMI client library */
    PMIU_getval( "pmi_version", version, PMIU_MAXLINE );
    PMIU_getval( "pmi_subversion", subversion, PMIU_MAXLINE );
    if (PMI_VERSION == atoi(version) && PMI_SUBVERSION >= atoi(subversion))
	rc = 0;
    else
	rc = -1;

    snprintf( outbuf, PMIU_MAXLINE,
	      "cmd=response_to_init pmi_version=%d pmi_subversion=%d rc=%d\n",
	      PMI_VERSION, PMI_SUBVERSION, rc);
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
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
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/*
 * Handle incoming "getbyidx" command
 */
static int fPMI_Handle_getbyidx( PMIProcess *pentry )
{
    int j, jNext, rc=0;
    PMIKVSpace *kvs;
    char kvsname[MAXKVSNAME], j_char[8], outbuf[PMIU_MAXLINE];
    PMIKVPair *p;

    PMIU_getval( "kvsname", kvsname, MAXKVSNAME );
    kvs = fPMIKVSFindSpace( kvsname );
    if (kvs) {
	PMIU_getval( "idx", j_char, sizeof(j_char) );
	j = atoi( j_char );
	jNext = j+1;
	if (kvs->lastIdx >= 0 && j >= kvs->lastIdx) {
	    for (p = kvs->lastByIdx, j-= kvs->lastIdx; j-- > 0 && p; 
		 p = p->nextPair );
	}
	else {
	    for (p = kvs->pairs; j-- > 0 && p; p = p->nextPair) ;
	}
	if (p) {
	    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results "
			   "rc=0 nextidx=%d key=%s val=%s\n",
			   jNext, p->key, p->val );
	    kvs->lastIdx   = jNext-1;
	    kvs->lastByIdx = p;
	}
	else {
	    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
			   "reason=no_more_keyvals\n" );
	    kvs->lastIdx   = -1;
	    kvs->lastByIdx = 0;
	}
    }
    else {
	rc = -1;
	MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=getbyidx_results rc=-1 "
		  "reason=kvs_%s_not_found\n", kvsname );
    }

    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return rc;
}

/*
 * These routines are called when communication is established through 
 * a port instead of an fd, and no information is communicated
 * through environment variables.
 */
static int fPMI_Handle_init_port( PMIProcess *pentry )
{
    char outbuf[PMIU_MAXLINE];

    DBG_PRINTFCOND(pmidebug,
		   ( "Entering fPMI_Handle_init to start connection\n" ));

    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set size=%d\n", 
		   pentry->group->nProcess );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set rank=%d\n", 
		   pentry->pState->wRank );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=set debug=%d\n", 1 );
    PMIU_writeline( pentry->fd, outbuf );
    DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    return 0;
}

/* Handle a spawn command.  This makes use of the spawner routine that
   was set during PMIServInit
   
   Spawn command in the simple PMI is handled as multiple lines with the
   subcommands:
   nprocs=%d
   execname=%s
   totspawns=%d
   spawnssofar=%d
   argcnt=%d
   arg%d=%s
   preput_num=%d
   preput_key_%d=%s
   preput_val_%d=%s
   info_num=%d
   endcmd

   After all of the data is collected, a ProcessWorld structure is
   created and passed to the user's spawner command.  That command
   is responsible for adding the ProcessWorld to the pUniv (ProcessUniverse)
   structure (in case there is an error and the spawn fails, the user
   can decide whether to included the failed world in the pUniv structure).


   Note that this routine must create the new "current" PMI group so that
   there will be a place for the associated KV Space.
  
*/

/* Maximum number of arguments */
#define PMI_MAX_ARGS 256
static int fPMI_Handle_spawn( PMIProcess *pentry )
{
    char          inbuf[PMIU_MAXLINE];
    char          *(args[PMI_MAX_ARGS]);
    char          key[MAXKEYLEN];
    char          outbuf[PMIU_MAXLINE];
    ProcessWorld *pWorld;
    ProcessApp   *app = 0;
    int           preputNum = 0, rc;
    int           i;
    int           totspawns, spawnnum;
    PMIKVSpace    *kvs = 0;

    DBG_PRINTFCOND(pmidebug,( "Entering fPMI_Handle_spawn\n" ));

    pWorld = (ProcessWorld *)MPIU_Malloc( sizeof(ProcessWorld) );
    /* FIXME check pWorld */
    
    pWorld->apps      = 0;
    pWorld->nProcess  = 0;
    pWorld->nextWorld = 0;
    pWorld->nApps     = 0;
    pWorld->worldNum  = pUniv.nWorlds++;


    /* FIXME: each mcmd=spawn should create an app.  When all apps
       are present, then then can be linked to a world */

    /* Create an initial app */
    app = (ProcessApp *)MPIU_Malloc( sizeof(ProcessApp) );
    app->myAppNum  = 0;
    app->exename   = 0;
    app->arch      = 0;
    app->path      = 0;
    app->wdir      = 0;
    app->hostname  = 0;
    app->args      = 0;
    app->nArgs     = 0;
    app->soft.nelm = 0;
    app->nProcess  = 0;
    app->pState    = 0;
    app->nextApp   = 0;
    app->pWorld    = pWorld;

    kvs = fPMIKVSAllocate();

    for (i=0; i<PMI_MAX_ARGS; i++) args[i] = 0;

    /* Input:
       (see above)

       Output:
       cmd=spawn_result remote_kvsname=name rc=integer
    */

    /* Get lines until we find either cmd or mcmd (an error) or endcmd 
       (expected end) */
    while ((rc = PMIU_readline( pentry->fd, inbuf, sizeof(inbuf) )) > 0) {
	char *cmdPtr, *valPtr, *p;

	printf( "Spawn: read line %s", inbuf );
	/* Find the command = format */
	p = inbuf;
	/* Find first nonblank */
	while (*p && isascii(*p) && isspace(*p)) p++;
	if (!*p) {
	    /* Empty string.  Ignore */
	    continue;
	}
	cmdPtr = p++;
	/* Find '=' */
	while (*p && *p != '=') p++;
	if (!*p) {
	    /* No =.  Check for endcmd */
	    p--;
	    /* Trim spaces */
	    while (isascii(*p) && isspace(*p)) p--;
	    /* Add null to end */
	    *++p = 0;
	    if (strcmp( "endcmd", cmdPtr ) == 0) { break; }
	    /* FIXME: Otherwise, we have a problem */
	}
	else {
	    *p = 0;
	}
	
	printf( "Cmd = %s\n", cmdPtr );
	/* Found an = .  value is the rest of the line */
	valPtr = ++p; 
	while (*p && *p != '\n') p++;
	if (*p) *p = 0;     /* Remove the newline */

	/* Now, process the cmd and value */
	if (strcmp( "nprocs", cmdPtr ) == 0) {
	    app->nProcess     = atoi(valPtr);
	    pWorld->nProcess += app->nProcess;
	}
	else if (strcmp( "execname", cmdPtr ) == 0) {
	    app->exename = MPIU_Strdup( valPtr );
	}
	else if (strcmp( "totspawns", cmdPtr ) == 0) {
	    /* This tells us how many separate spawn commands
	       we expect to see (e.g., for spawn multiple).
	       Each spawn command is a separate "app" */
	    totspawns = atoi(valPtr);
	}
	else if (strcmp( "spawnssofar", cmdPtr ) == 0) {
	    /* This tells us which app we are (starting from 1) */
	    spawnnum      = atoi(valPtr);
	    app->myAppNum = spawnnum - 1;
	}
	else if (strcmp( "argcnt", cmdPtr ) == 0) {
	    /* argcnt may not be set before the args */
	    app->nArgs = atoi(valPtr);
	}
	else if (strncmp( "arg", cmdPtr, 3 ) == 0) {
	    int argnum;
	    /* argcnt may not be set before the args */
	    /* Handle arg%d.  Values are 1 - origin */
	    argnum = atoi( cmdPtr + 3 ) - 1;
	    printf( "Argnum = %d, arg = %s\n", argnum, valPtr );
	    if (argnum < 0 || argnum >= PMI_MAX_ARGS) {
		/* FIXME: malformed command */
	    }
	    args[argnum] = (char *)MPIU_Strdup( valPtr );
	}
	else if (strcmp( "preput_num", cmdPtr ) == 0) {
	    preputNum = atoi(valPtr);
	}
	else if (strncmp( "preput_key_", cmdPtr, 11 ) == 0) {
	    /* Save the key */
	    MPIU_Strncpy( key, valPtr, sizeof(key) );
	}
	else if (strncmp( "preput_val_", cmdPtr, 11 ) == 0) {
	    /* Place the key,val into the space associate with the current 
	       PMI group */
	    fPMIKVSAddPair( kvs, key, valPtr );
	}
	else if (strcmp( "info_num", cmdPtr ) == 0) {
	}
	else {
	    /* FIXME: Unrecognized subcommand */
	}
    }	

    if (app->nArgs > 0) {
	app->args  = (const char **)MPIU_Malloc( app->nArgs * sizeof(char *) );
	for (i=0; i<app->nArgs; i++) {
	    printf( "Setting args[%d] to %s\n", i, args[i] );
	    app->args[i] = args[i];
	    args[i]      = 0;
	}
    }

    /* FIXME: end of read mcmd=spawn for an app */

    /* Now that we've read the commands, invoke the user's spawn command */
    pWorld->nApps ++;
    /* FIXME: Add to end */
    pWorld->apps  = app;

    if (totspawns == spawnnum) {
	PMISetupNewGroup( pWorld->nProcess, kvs );
	
	if (userSpawner) {
	    rc = (*userSpawner)( pWorld, userSpawnerData );
	}
	else {
	    printf( "Unable to spawn %s\n", app->exename );
	    rc = 1;
	    MPIE_PrintProcessWorld( stdout, pWorld );
	}
	
	MPIU_Snprintf( outbuf, PMIU_MAXLINE, "cmd=spawn_result rc=%d\n", rc );
	PMIU_writeline( pentry->fd, outbuf );
	DBG_PRINTFCOND(pmidebug,( "%s", outbuf ));
    }
    
    /* If totspawnnum != spawnnum, then we are expecting a 
       spawnmult with additional items */
    return 0;
}

/* ------------------------------------------------------------------------- */
#if 0
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
#endif
/* 
 * This is a special routine.  It accepts the first input from the
 * remote process, and returns the PMI_ID value.  -1 is returned on error 
 */
int PMI_Init_port_connection( int fd )
{
    char message[PMIU_MAXLINE], cmd[MAXPMICMD];
    int pmiid = -1;

    DBG_PRINTFCOND(pmidebug,( "Beginning initial handshake read\n" ));
    PMIU_readline( fd, message, PMIU_MAXLINE );
    DBG_PRINTFCOND(pmidebug,( "received message %s\n", message ));

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
