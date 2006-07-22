/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/* This program is an updated version of the mclient program found on the MPICH-G2 home page. */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int passed_num;
    int my_id;
    MPI_Comm newcomm;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

    MPI_Comm_connect(argv[1], MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm); 

    if (my_id == 0)
    {
	MPI_Status status;
	MPI_Recv(&passed_num, 1, MPI_INT, 0, 0, newcomm, &status);
	printf("client 0: after receiving passed_num %d\n", passed_num); fflush(stdout);

	passed_num=123;
	MPI_Bcast(&passed_num, 1, MPI_INT, MPI_ROOT, newcomm);
	printf("client 0: after broadcasting passed_num %d\n", passed_num); fflush(stdout);
    }
    else
    {
	MPI_Bcast(NULL, 0, MPI_INT, MPI_PROC_NULL, newcomm);
    }

    MPI_Finalize();

    exit(0);
}
