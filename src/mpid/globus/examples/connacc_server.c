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
    int my_id;
    char port_name[MPI_MAX_PORT_NAME];
    MPI_Comm newcomm;
    int passed_num;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_id);

    passed_num = 111;

    if (my_id == 0)
    {
	MPI_Open_port(MPI_INFO_NULL, port_name);
	printf("%s\n\n", port_name); fflush(stdout);
    }

    MPI_Comm_accept(port_name, MPI_INFO_NULL, 0, MPI_COMM_WORLD, &newcomm); 

    if (my_id == 0)
    {
	MPI_Send(&passed_num, 1, MPI_INT, 0, 0, newcomm);
	printf("server 0: after sending passed_num %d\n", passed_num); fflush(stdout);
    }

    MPI_Bcast(&passed_num, 1, MPI_INT, 0, newcomm);
    printf("server %d: after receiving broadcast of passed_num %d\n", my_id, passed_num); fflush(stdout);
    
    if (my_id == 0)
    {
	MPI_Close_port(port_name);
    }
    
    MPI_Finalize();

    exit(0);
}
