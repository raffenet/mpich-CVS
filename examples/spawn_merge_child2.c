#include <stdio.h>
#include "mpi.h"
#include "mpiimpl.h"

int main( int argc, char *argv[] )
{
    MPI_Comm intercomm;
    char str[10];
    int err, rank;

    MPI_Init(&argc, &argv);

/*    printf("Child out of MPI_Init\n");
    fflush(stdout);
*/
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_parent(&intercomm);

    if (rank == 1){
        err = MPI_Send("hi", 3, MPI_CHAR, 2, 0, intercomm);
        
        err = MPI_Recv(str, 4, MPI_CHAR, 2, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Child 2 received from parent (first child): %s\n", str);
        fflush(stdout);
    }

    MPI_Barrier(intercomm);

    sleep(5);

    MPI_Finalize();
    return 0;
}
