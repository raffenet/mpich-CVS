#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int size;
    int rank;
    int msg = 0;

    if (MPI_Init(&argc, &argv) != MPI_SUCCESS)
    {
        printf("ERROR: problem with MPI_Init\n"); fflush(stdout);
    }
    
    if (MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)
    {
	printf("ERROR: problem with MPI_Comm_size\n"); fflush(stdout);
    }
    
    if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
    {
	printf("ERROR: problem with MPI_Comm_rank\n"); fflush(stdout);
    }

    printf("sr: size %d rank %d\n", size, rank); fflush(stdout);
    
    if (size < 2)
    {
	printf("ERROR: needs to be run with at least 2 procs\n");
	fflush(stdout);
    }
    else if (rank == 0)
    {
	/* msg = 2222;
	if (MPI_Send(&msg, 1, MPI_INT, 1, 5, MPI_COMM_WORLD) != MPI_SUCCESS) */
	if (MPI_Send(NULL, 0, MPI_INT, 1, 5, MPI_COMM_WORLD) != MPI_SUCCESS)
	{
	    printf("ERROR: problem with MPI_Send\n"); fflush(stdout);
	}
    }
    else if (rank == 1)
    {
	MPI_Status status;

	/* if (MPI_Recv(&msg, 1, MPI_INT, 0, 5, MPI_COMM_WORLD, &status) */
	if (MPI_Recv(NULL, 0, MPI_INT, 0, 5, MPI_COMM_WORLD, &status)
	    != MPI_SUCCESS)
	{
	    printf("ERROR: problem with MPI_Recv\n"); fflush(stdout);
	}
	
	printf("app: rcvd message %d\n", msg); fflush(stdout);
    }

    if (MPI_Finalize() != MPI_SUCCESS)
    {
        printf("ERROR: problem with MPI_Finalize\n"); fflush(stdout);
    }

    exit(0);
}
