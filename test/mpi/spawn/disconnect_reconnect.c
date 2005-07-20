/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define IF_VERBOSE(a) if (verbose) { printf a ; fflush(stdout); }

static char MTEST_Descrip[] = "A simple test of Comm_connect/accept/disconnect";

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, rsize, i, data, num_loops = 100;
    int np = 3;
    MPI_Comm      parentcomm, intercomm;
    MPI_Status    status;
    char port[MPI_MAX_PORT_NAME];
    int verbose = 0;
    char *env;

    env = getenv("MPITEST_VERBOSE");
    if (env)
    {
	if (*env != '0')
	    verbose = 1;
    }

    MTest_Init( &argc, &argv );

    MPI_Comm_get_parent( &parentcomm );

    if (parentcomm == MPI_COMM_NULL)
    {
	IF_VERBOSE(("spawning %d processes\n", np));
	/* Create 2 more processes */
	MPI_Comm_spawn("./disconnect_reconnect", MPI_ARGV_NULL, np,
			MPI_INFO_NULL, 0, MPI_COMM_WORLD,
			&intercomm, MPI_ERRCODES_IGNORE);
    }
    else
    {
	intercomm = parentcomm;
    }

    /* We now have a valid intercomm */

    MPI_Comm_remote_size(intercomm, &rsize);
    MPI_Comm_size(intercomm, &size);
    MPI_Comm_rank(intercomm, &rank);

    if (parentcomm == MPI_COMM_NULL)
    {
	IF_VERBOSE(("parent rank %d alive.\n", rank));
	/* Parent */
	if (rsize != np)
	{
	    errs++;
	    printf("Did not create %d processes (got %d)\n", np, rsize);
	    fflush(stdout);
	}
	if (rank == 0)
	{
	    MPI_Open_port(MPI_INFO_NULL, port);
	    IF_VERBOSE(("port = %s\n", port));
	    MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, intercomm);
	}
	IF_VERBOSE(("disconnecting child communicator\n"));
	MPI_Comm_disconnect(&intercomm);
	for (i=0; i<num_loops; i++)
	{
	    IF_VERBOSE(("accepting connection\n"));
	    MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);
	    if (rank == 0)
	    {
		data = i;
		MPI_Send(&data, 1, MPI_INT, 1, 100, intercomm);
		MPI_Recv(&data, 1, MPI_INT, 1, 100, intercomm, &status);
		if (data != i)
		{
		    errs++;
		}
	    }
	    IF_VERBOSE(("disconnecting communicator\n"));
	    MPI_Comm_disconnect(&intercomm);
	}

	/* Errors cannot be sent back to the parent because there is no communicator connected to the children
	for (i=0; i<rsize; i++)
	{
	    MPI_Recv( &err, 1, MPI_INT, i, 1, intercomm, MPI_STATUS_IGNORE );
	    errs += err;
	}
	*/
    }
    else
    {
	IF_VERBOSE(("child rank %d alive.\n", rank));
	/* Child */
	if (size != np)
	{
	    errs++;
	    printf("(Child) Did not create %d processes (got %d)\n", np, size);
	    fflush(stdout);
	}

	if (rank == 0)
	    MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, intercomm, &status);

	MPI_Comm_disconnect(&intercomm);
	for (i=0; i<num_loops; i++)
	{
	    MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &intercomm);
	    if (rank == 1)
	    {
		MPI_Recv(&data, 1, MPI_INT, 0, 100, intercomm, &status);
		if (data != i)
		{
		    printf("expected %d but received %d\n", i, data);
		    fflush(stdout);
		    MPI_Abort(MPI_COMM_WORLD, 1);
		}
		MPI_Send(&data, 1, MPI_INT, 0, 100, intercomm);
	    }
	    MPI_Comm_disconnect(&intercomm);
	}

	/* Send the errs back to the master process */
	/* Errors cannot be sent back to the parent because there is no communicator connected to the parent */
	/*MPI_Ssend( &errs, 1, MPI_INT, 0, 1, intercomm );*/
    }

    /* Note that the MTest_Finalize get errs only over COMM_WORLD */
    /* Note also that both the parent and child will generate "No Errors"
       if both call MTest_Finalize */
    if (parentcomm == MPI_COMM_NULL)
    {
	MTest_Finalize( errs );
    }

    MPI_Finalize();
    return 0;
}
