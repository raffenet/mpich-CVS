/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "process.h"
#include "ioloop.h"
#include <signal.h>

struct iofds { int readOut[2], readErr[2]; };

int mypreamble( void * );
int myprefork( void *, void * );
int mypostamble( void *, void * );
int labeler( int, int, void * );

#include <sys/wait.h>
int handlesig( void ) 
{
    int p;
    signal( SIGCHLD, handlesig );
    printf( "In signal handler\n" );
    wait( &p );
    printf( "status was %d\n", p );
}
/* 
   Test the argument parsing and processing routines.  See the Makefile
   for typical test choices 
*/
int main( int argc, char *argv[], char *envp[] )
{
    ProcessUniverse pUniv;
    int             rc;
    struct iofds    newfds;

    signal( SIGCHLD, handlesig );
    MPIE_Args( argc, argv, &pUniv, 0, 0 );
    MPIE_PrintProcessUniverse( stdout, &pUniv );
    MPIE_ForkProcesses( &pUniv.worlds[0], envp, mypreamble, &newfds, 
			myprefork, 0, mypostamble, 0 );
    MPIE_IOLoop( 10 );
    MPIE_WaitProcesses( &pUniv );
    
    /* rc = MPIE_GetExitStatus( &pUniv ); */
    
    return rc;
}

/* Redirect stdout and stderr to a handler */
int mypreamble( void *data )
{
    struct iofds *newfds = (struct iofds *)data;
    /* Each pipe call creates 2 fds, 0 for reading, 1 for writing */
    if (pipe(newfds->readOut)) return -1;
    if (pipe(newfds->readErr)) return -1;
}
/* Close one side of each pipe pair and replace stdout/err with the pipes */
int myprefork( void *predata, void *data )
{
    struct iofds *newfds = (struct iofds *)predata;

    close( newfds->readOut[0] );
    close(1);
    dup2( newfds->readOut[1], 1 );
    close( newfds->readErr[0] );
    close(2);
    dup2( newfds->readErr[1], 2 );
}
#include <fcntl.h>
/* Close one side of the pipe pair and register a handler for the I/O */
int mypostamble( void *predata, void *data )
{
    struct iofds *newfds = (struct iofds *)predata;
    char *leader, *leadererr;
    static int wRank = 0;  /* TEMP */

#if 0
    close( newfds->readOut[1] );
    close( newfds->readErr[1] );
#endif

    /* We need dedicated storage for the private data */
    leader = (char *)malloc( 32 );
    snprintf( leader, 32, "%d>", wRank++ );
    leadererr = (char *)malloc( 32 );
    snprintf( leadererr, 32, "err%d>", wRank++ );
    MPIE_IORegister( newfds->readOut[0], IO_READ, labeler, leader );
    MPIE_IORegister( newfds->readErr[0], IO_READ, labeler, leadererr );

    /* subsequent forks should not get these fds */
#if 0
    fcntl( newfds->readOut[0], F_SETFD, FD_CLOEXEC );
    fcntl( newfds->readErr[0], F_SETFD, FD_CLOEXEC );
#endif
}
int labeler( int fd, int rdwr, void *data )
{
    char *label = (char *)data;
    char buf[1024], *p;
    int n;
    static int lastNL = 1;

    n = read( fd, buf, 1024 );
    if (n == 0) {
	printf( "Closing fd %d\n", fd );
	return 1;  /* ? EOF */
    }

    p = buf;
    while (n > 0) {
	int c;
	if (lastNL) {
	    printf( "%s", label );
	    lastNL = 0;
	}
	c = *p++; n--;
	putchar(c);
	lastNL = c == '\n';
    }
    return 0;
}
int mpiexec_usage( const char *str )
{
    fprintf( stderr, "Usage: %s\n", str );
    return 0;
}
