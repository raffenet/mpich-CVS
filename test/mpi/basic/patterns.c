#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

int SendRecvTest(int rank, int n)
{
    int tag = 1;
    MPI_Status status;
    char buffer[100];
    int i;

    if (rank == 0)
    {
	strcpy(buffer, "Hello process one.");
	for (i=0; i<n; i++)
	    MPI_Send(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD);
    }
    else if (rank == 1)
    {
	for (i=0; i<n; i++)
	    MPI_Recv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &status);
	/*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);*/
    }

    return TRUE;
}

int IsendIrecvTest(int rank)
{
    int tag = 1;
    MPI_Status status;
    MPI_Request request;
    char buffer[100];

    if (rank == 0)
    {
	strcpy(buffer, "Hello process one.");
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);
    }
    else if (rank == 1)
    {
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag, MPI_COMM_WORLD, &request);
	MPI_Wait(&request, &status);
	/*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);*/
    }

    return TRUE;
}

int IsendIrecvTest2(int rank)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char buffer[100];

    if (rank == 0)
    {
	strcpy(buffer, "Hello process one.");
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request1, &status);
	MPI_Wait(&request2, &status);
    }
    else if (rank == 1)
    {
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request1);
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request1, &status);
	MPI_Wait(&request2, &status);
	/*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);*/
    }

    return TRUE;
}

int OutOfOrderTest(int rank)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char buffer[100];

    if (rank == 0)
    {
	strcpy(buffer, "Hello process one.");
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request1, &status);
	MPI_Wait(&request2, &status);
    }
    else if (rank == 1)
    {
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request1);
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request2, &status);
	MPI_Wait(&request1, &status);
	/*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);*/
    }

    return TRUE;
}

int ForceUnexpectedTest(int rank)
{
    int tag1 = 1;
    int tag2 = 2;
    MPI_Status status;
    MPI_Request request1, request2;
    char buffer[100];

    if (rank == 0)
    {
	strcpy(buffer, "Hello process one.");
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag1, MPI_COMM_WORLD, &request1);
	MPI_Isend(buffer, 100, MPI_BYTE, 1, tag2, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request1, &status);
	MPI_Wait(&request2, &status);
    }
    else if (rank == 1)
    {
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag2, MPI_COMM_WORLD, &request2);
	MPI_Wait(&request2, &status);
	MPI_Irecv(buffer, 100, MPI_BYTE, 0, tag1, MPI_COMM_WORLD, &request1);
	MPI_Wait(&request1, &status);
	/*printf("Rank 1: received message '%s'\n", buffer);fflush(stdout);*/
    }

    return TRUE;
}

int main(int argc, char *argv[])
{
    int result;
    int size, rank;
    int bDoAll = FALSE;
    int reps;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (size < 2)
    {
	printf("Two processes needed.\n");
	MPI_Finalize();
	return 0;
    }

    if (rank > 1)
    {
	printf("Rank %d, I am not participating.\n", rank);
	fflush(stdout);
    }
    else
    {
	if (argc < 2)
	    bDoAll = TRUE;

	if (bDoAll || (strcmp(argv[1], "sr") == 0))
	{
	    reps = 1;
	    if (argc > 2)
	    {
		reps = atoi(argv[2]);
		if (reps < 1)
		    reps = 1;
	    }
	    if (rank == 0)
	    {
		printf("Send/recv test: %d reps\n", reps);
		fflush(stdout);
	    }
	    result = SendRecvTest(rank, reps);
	    if (rank == 0)
	    {
		printf(result ? "SUCCESS\n" : "FAILURE\n");
		fflush(stdout);
	    }
	}

	if (bDoAll || (strcmp(argv[1], "isr") == 0))
	{
	    if (rank == 0)
	    {
		printf("Isend/irecv wait test\n");
		fflush(stdout);
	    }
	    result = IsendIrecvTest(rank);
	    if (rank == 0)
	    {
		printf(result ? "SUCCESS\n" : "FAILURE\n");
		fflush(stdout);
	    }
	}

	if (bDoAll || (strcmp(argv[1], "iisr") == 0))
	{
	    if (rank == 0)
	    {
		printf("Isend,isend/irecv,irecv wait wait test\n");
		fflush(stdout);
	    }
	    result = IsendIrecvTest2(rank);
	    if (rank == 0)
	    {
		printf(result ? "SUCCESS\n" : "FAILURE\n");
		fflush(stdout);
	    }
	}

	if (bDoAll || (strcmp(argv[1], "oo") == 0))
	{
	    if (rank == 0)
	    {
		printf("Out of order isend/irecv test\n");
		fflush(stdout);
	    }
	    result = OutOfOrderTest(rank);
	    if (rank == 0)
	    {
		printf(result ? "SUCCESS\n" : "FAILURE\n");
		fflush(stdout);
	    }
	}

	if (bDoAll || (strcmp(argv[1], "unex") == 0))
	{
	    if (rank == 0)
	    {
		printf("Force unexpected message test\n");
		fflush(stdout);
	    }
	    result = ForceUnexpectedTest(rank);
	    if (rank == 0)
	    {
		printf(result ? "SUCCESS\n" : "FAILURE\n");
		fflush(stdout);
	    }
	}
    }

    MPI_Finalize();
    return 0;
}
