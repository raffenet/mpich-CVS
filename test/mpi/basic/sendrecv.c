#include "mpi.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int size, rank;
    char buffer[100] = "garbage";
    /*MPI_Request request;*/
    MPI_Status status;
    int tag = 1;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2)
    {
	printf("Two processes needed.\n");
	MPI_Finalize();
	return 0;
    }

    if (rank == 0)
    {
      printf("Rank 0: sending message to process 1.\n");fflush(stdout);
	strcpy(buffer, "Hello process one.");
	/*MPI_Isend(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);*/
	MPI_Send(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
    }
    else if (rank == 1)
    {
      printf("Rank 1: receiving message from process 0.\n");fflush(stdout);
	/*MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);*/
	MPI_Recv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
	printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);
    }
    else
    {
      printf("Rank %d, I am not participating.\n", rank);fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}
