#include "mpi.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    int size, rank;
    char buffer[100] = "garbage";
    MPI_Request request;
    MPI_Status status;
    int tag = 1;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0)
    {
	printf("Rank 0: sending message to process 1.\n");
	strcpy(buffer, "Hello process one.");
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);
    }
    else if (rank == 1)
    {
	printf("Rank 1: receiving message from process 0.\n");
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);
	printf("Rank 1: received message '%s'\n", buffer);
    }
    else
    {
	printf("Rank %d, I am not participating.\n", rank);
    }

    MPI_Finalize();
    return 0;
}
