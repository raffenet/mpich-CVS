#include <stdio.h>
#include "mpi.h"

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

    if (rank == 3){
        err = MPI_Send("hi", 3, MPI_CHAR, 3, 0, intercomm);
        
        err = MPI_Recv(str, 4, MPI_CHAR, 3, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Child received from parent: %s\n", str);
        fflush(stdout);

        printf("Child sleeping for 5 sec; ignore errors after this\n");
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}
