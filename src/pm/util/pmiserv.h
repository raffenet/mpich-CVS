/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMISERV_H_INCLUDED
#define PMISERV_H_INCLUDED

/*
 * This file contains the definitions and prototypes exported by the 
 * pmi server package
 */

/* 
   The basic item is the PMIProcess, which contains the fd used to 
   communicate with the PMI client code, a pointer to the PMI Group 
   to which this process belongs, and the process state (defined in 
   process.h)
*/
typedef struct PMIProcess {
    int                  fd;             /* fd to client */
    struct PMIGroup     *group;          /* PMI group to which this process 
					    belongs */
    struct ProcessState *pState;         /* ProcessState */
    /* The following are used to hold data used to handle
       spawn and spawn multiple commands */
    struct ProcessApp   *spawnApp, 
                        *spawnAppTail;  /* In progress spawn multiple 
					   data */
    struct ProcessWorld *spawnWorld;    /* In progress spawn multiple 
					   new world */
    struct PMIKVSpace   *spawnKVS;      /* In progres KV space for new world */
} PMIProcess;

typedef struct PMIGroup {
    int              nProcess;    /* Number of PMI processes in this group */
    int              groupID;     /* Numeric id of group */
    int              nInBarrier;  /* Number of group members currently
				     waiting in the PMIBarrier */
    struct PMIKVSpace   *kvs;     /* KVSpace for this group of processes */
    PMIProcess      **pmiProcess; /* Array of pointers to PMIProcess'es
				     in this group */
    struct PMIGroup *nextGroup;   /* Pointer to next group structure */
} PMIGroup;

/* The key-value space is described by a pairs of char pointers and a 
   header that provides the name of the space and links all of the spaces 
   together */
/* The PMI specification requires that the server be able to inform
   the client of the maximum key and value size that is supported.  For 
   simplicity, we define each KV pair with fixed-length entries of
   the maximum size */
#define MAXKEYLEN    64		/* max length of key in keyval space */
#define MAXVALLEN   128		/* max length of value in keyval space */
#define MAXNAMELEN  256         /* max length of various names */
#define MAXKVSNAME  MAXNAMELEN  /* max length of a kvsname */
typedef struct PMIKVPair {
    char key[MAXKEYLEN];
    char val[MAXVALLEN];
    struct PMIKVPair *nextPair;
} PMIKVPair;

typedef struct PMIKVSpace {
    const char        kvsname[MAXNAMELEN];   /* Name of this kvs space */
    PMIKVPair         *pairs;     /* Pointer to the pairs in this space */
    PMIKVPair         *lastByIdx; /* Pointer to the element corresponding
				     to the index lastIdx (improves
				     support for getbyidx method) */
    int               lastIdx;
    struct PMIKVSpace *nextKVS;   /* Pointer to the next KVS */
} PMIKVSpace;

/* 
   The master PMI structure contains the "shared" objects:
   groups and key-value spaces (kvs)
*/
typedef struct PMIMaster {
    int        nGroups;      /* Number of groups allocated (non-decreasing) */
    PMIGroup   *groups;      /* Pointer to allocated groups */
    PMIKVSpace *kvSpaces;    /* Pointer to allocated KV spaces */
} PMIMaster;

typedef struct {
    int        fdpair[2];    /* fd's used to communicate between the
				client and the server */
} PMISetup;

/* version to check with client PMI library */
#define PMI_VERSION    1
#define PMI_SUBVERSION 1

int PMIServInit( int (*)(ProcessWorld *, void*), void * );
PMIProcess *PMISetupNewProcess( int, ProcessState * );
int PMISetupSockets( int, PMISetup * );
int PMISetupInClient( PMISetup * );
int PMISetupFinishInServer( PMISetup *, ProcessState * );
int PMISetupNewGroup( int, PMIKVSpace * );

#endif
