/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This file defines routines that can be called by the pre and postamble
 * functions in MPIE_ForkProcesses to add output line labeling to stdout 
 * and to stderr. 
 *
 * These functions are very simple and assume that either stdout/err won't
 * block, or that causing mpiexec to block until stdout/err unblock is
 * acceptable.  A more sophisticated approach would queue up the output
 * and take advantage of the IOLoop routine (in ioloop.c) to control
 * output.
 */

#include "pmutilconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include "ioloop.h"
#include "process.h"

typedef struct { int readOut[2], readErr[2]; } IOLabelSetup;

#define MAX_LABEL 32
typedef struct { char label[MAX_LABEL]; int lastNL; } IOLabel;

static int IOLabelWriteLine( int, int, void * );

/* Redirect stdout and stderr to a handler */
int IOLabelSetupFDs( IOLabelSetup *iofds )
{
    /* Each pipe call creates 2 fds, 0 for reading, 1 for writing */
    if (pipe(iofds->readOut)) return -1;
    if (pipe(iofds->readErr)) return -1;
    return 0;
}

/* Close one side of each pipe pair and replace stdout/err with the pipes */
int IOLabelSetupInClient( IOLabelSetup *iofds )
{
    close( iofds->readOut[0] );
    close(1);
    dup2( iofds->readOut[1], 1 );
    close( iofds->readErr[0] );
    close(2);
    dup2( iofds->readErr[1], 2 );

    return 0;
}

/* Close one side of the pipe pair and register a handler for the I/O */
int IOLabelSetupFinishInServer( IOLabelSetup *iofds, ProcessState *pState )
{
    IOLabel *leader, *leadererr;

    close( iofds->readOut[1] );
    close( iofds->readErr[1] );

    /* We need dedicated storage for the private data */
    leader = (IOLabel *)malloc( sizeof(IOLabel) );
    snprintf( leader->label, sizeof(leader->label), "%d>", pState->wRank );
    leader->lastNL = 1;
    leadererr = (IOLabel *)malloc( sizeof(IOLabel) );
    snprintf( leadererr->label, sizeof(leadererr->label), "err%d>", 
	      pState->wRank );
    leadererr->lastNL = 1;
    MPIE_IORegister( iofds->readOut[0], IO_READ, IOLabelWriteLine, leader );
    MPIE_IORegister( iofds->readErr[0], IO_READ, IOLabelWriteLine, leadererr );

    /* subsequent forks should not get these fds */
    fcntl( iofds->readOut[0], F_SETFD, FD_CLOEXEC );
    fcntl( iofds->readErr[0], F_SETFD, FD_CLOEXEC );
}

static int IOLabelWriteLine( int fd, int rdwr, void *data )
{
    IOLabel *label = (IOLabel *)data;
    char buf[1024], *p;
    int n;

    n = read( fd, buf, 1024 );
    if (n == 0) {
	printf( "Closing fd %d\n", fd );
	return 1;  /* ? EOF */
    }

    p = buf;
    while (n > 0) {
	int c;
	if (label->lastNL) {
	    printf( "%s", label->label );
	    label->lastNL = 0;
	}
	c = *p++; n--;
	putchar(c);
	label->lastNL = c == '\n';
    }
    return 0;
}

