#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    MPI_Comm intercomm;
    char str[10];
    int err, rank;

    MPI_Init(&argc, &argv);

    MPI_Comm_get_parent(&intercomm);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    err = MPI_Send("hi", 3, MPI_CHAR, rank, 0, intercomm);

    err = MPI_Recv(str, 4, MPI_CHAR, rank, 0, intercomm, MPI_STATUS_IGNORE);
    printf("Received: %s\n", str);
    fflush(stdout);

    MPI_Finalize();
    return 0;
}
