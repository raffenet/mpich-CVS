#include <stdio.h>

int main(int argc, char *argv[])
{
    fprintf( stdout,"Process is alive\n" );
    sleep( 1 );
    fprintf( stderr, "Process %d exiting with exit code %d\n",
	     getpid( ), -getpid( ) );
    return( -getpid( ) );
}
