#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void check_error(int error, char *fcname)
{
    char err_string[MPI_MAX_ERROR_STRING];
    int length;
    if (error != MPI_SUCCESS)
    {
	MPI_Error_string(error, err_string, &length);
	printf("%s failed: %s\n", fcname, err_string);
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, error);
    }
}

int main(int argc, char *argv[])
{
    int error;
    int rank, size;
    char *argv1[2] = { "connector", NULL };
    char *argv2[2] = { "acceptor", NULL };
    MPI_Comm comm_connector, comm_acceptor, comm_parent, comm;
    char port[MPI_MAX_PORT_NAME];
    MPI_Status status;
    int verbose = 0;

    if (getenv("MPITEST_VERBOSE"))
    {
	verbose = 1;
    }

    if (verbose) { printf("init.\n");fflush(stdout); }
    error = MPI_Init(&argc, &argv);
    check_error(error, "MPI_Init");

    if (verbose) { printf("size.\n");fflush(stdout); }
    error = MPI_Comm_size(MPI_COMM_WORLD, &size);
    check_error(error, "MPI_Comm_size");

    if (verbose) { printf("rank.\n");fflush(stdout); }
    error = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    check_error(error, "MPI_Comm_rank");

    if (argc == 1)
    {
	if (verbose) { printf("spawn connector.\n");fflush(stdout); }
	error = MPI_Comm_spawn("spaconacc", argv1, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm_connector, MPI_ERRCODES_IGNORE);
	check_error(error, "MPI_Comm_spawn");

	if (verbose) { printf("spawn acceptor.\n");fflush(stdout); }
	error = MPI_Comm_spawn("spaconacc", argv2, 1, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm_acceptor, MPI_ERRCODES_IGNORE);
	check_error(error, "MPI_Comm_spawn");

	if (verbose) { printf("recv port.\n");fflush(stdout); }
	error = MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_acceptor, &status);
	check_error(error, "MPI_Recv");

	if (verbose) { printf("send port.\n");fflush(stdout); }
	error = MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_connector);
	check_error(error, "MPI_Send");

	if (verbose) { printf("barrier acceptor.\n");fflush(stdout); }
	error = MPI_Barrier(comm_acceptor);
	check_error(error, "MPI_Barrier");

	if (verbose) { printf("barrier connector.\n");fflush(stdout); }
	error = MPI_Barrier(comm_connector);
	check_error(error, "MPI_Barrier");

	printf(" No Errors\n");
    }
    else if ((argc == 2) && (strcmp(argv[1], "acceptor") == 0))
    {
	if (verbose) { printf("get_parent.\n");fflush(stdout); }
	error = MPI_Comm_get_parent(&comm_parent);
	check_error(error, "MPI_Comm_get_parent");
	if (comm_parent == MPI_COMM_NULL)
	{
	    printf("acceptor's parent is NULL.\n");fflush(stdout);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
	if (verbose) { printf("open_port.\n");fflush(stdout); }
	error = MPI_Open_port(MPI_INFO_NULL, port);
	check_error(error, "MPI_Open_port");

	if (verbose) { printf("0: opened port: <%s>\n", port);fflush(stdout); }
	if (verbose) { printf("send.\n");fflush(stdout); }
	error = MPI_Send(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_parent);
	check_error(error, "MPI_Send");

	if (verbose) { printf("accept.\n");fflush(stdout); }
	error = MPI_Comm_accept(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
	check_error(error, "MPI_Comm_accept");

	if (verbose) { printf("close_port.\n");fflush(stdout); }
	error = MPI_Close_port(port);
	check_error(error, "MPI_Close_port");

	if (verbose) { printf("disconnect.\n");fflush(stdout); }
	error = MPI_Comm_disconnect(&comm);
	check_error(error, "MPI_Comm_disconnect");

	if (verbose) { printf("barrier.\n"); fflush(stdout); }
	error = MPI_Barrier(comm_parent);
	check_error(error, "MPI_Barrier");
    }
    else if ((argc == 2) && (strcmp(argv[1], "connector") == 0))
    {
	if (verbose) { printf("get_parent.\n");fflush(stdout); }
	error = MPI_Comm_get_parent(&comm_parent);
	check_error(error, "MPI_Comm_get_parent");
	if (comm_parent == MPI_COMM_NULL)
	{
	    printf("acceptor's parent is NULL.\n");fflush(stdout);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
	
	if (verbose) { printf("recv.\n");fflush(stdout); }
	error = MPI_Recv(port, MPI_MAX_PORT_NAME, MPI_CHAR, 0, 0, comm_parent, &status);
	check_error(error, "MPI_Recv");

	if (verbose) { printf("1: received port: <%s>\n", port);fflush(stdout); }
	if (verbose) { printf("connect.\n");fflush(stdout); }
	error = MPI_Comm_connect(port, MPI_INFO_NULL, 0, MPI_COMM_SELF, &comm);
	check_error(error, "MPI_Comm_connect");

	if (verbose) { printf("disconnect.\n");fflush(stdout); }
	error = MPI_Comm_disconnect(&comm);
	check_error(error, "MPI_Comm_disconnect");

	if (verbose) { printf("barrier.\n"); fflush(stdout); }
	error = MPI_Barrier(comm_parent);
	check_error(error, "MPI_Barrier");
    }
    else
    {
	printf("invalid command line.\n");fflush(stdout);
	{
	    int i;
	    for (i=0; i<argc; i++)
	    {
		printf("argv[%d] = <%s>\n", i, argv[i]);
	    }
	}
	fflush(stdout);
	MPI_Abort(MPI_COMM_WORLD, -2);
    }

    MPI_Finalize();
    return 0;
}
