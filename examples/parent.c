#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    char *args[10], str[10];
    int err=0, errcodes[256], rank, size;
    MPI_Comm intercomm;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    err = MPI_Comm_spawn("child",
                         args, size, MPI_INFO_NULL, 0, MPI_COMM_WORLD,
                         &intercomm, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    err = MPI_Recv(str, 3, MPI_CHAR, rank, 0, intercomm, MPI_STATUS_IGNORE);
    printf("Received: %s\n", str);
    fflush(stdout);

    err = MPI_Send("bye", 4, MPI_CHAR, rank, 0, intercomm);

    MPI_Finalize();

    return 0;
}

