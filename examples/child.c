#include <stdio.h>
#include "mpi.h"
#include "mpiimpl.h"

int main( int argc, char *argv[] )
{
    MPI_Comm intercomm;
    char str[10];
    int err, rank;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_parent(&intercomm);

    if (rank == 0){
        err = MPI_Send("hi", 3, MPI_CHAR, 0, 0, intercomm);
        
        err = MPI_Recv(str, 4, MPI_CHAR, 0, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Child received from parent: %s\n", str);
        fflush(stdout);
    }

    printf("Child exited\n");
    fflush(stdout);

    printf("Sleeping for 5 sec\n");

    sleep(5);

    MPI_Finalize();
    return 0;
}
