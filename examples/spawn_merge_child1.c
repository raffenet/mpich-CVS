#include <stdio.h>
#include "mpi.h"
#include "mpiimpl.h"

int main( int argc, char *argv[] )
{
    char str[10];
    MPI_Comm intercomm1, intracomm, intercomm2;
    int err, errcodes[256], rank;

    MPI_Init(&argc, &argv);

/*    printf("Child out of MPI_Init\n");
    fflush(stdout);
*/
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_parent(&intercomm1);

    MPI_Intercomm_merge(intercomm1, 1, &intracomm);

    err = MPI_Comm_spawn("spawn_merge_child2", MPI_ARGV_NULL, 2,
                         MPI_INFO_NULL, 2, intracomm,
                         &intercomm2, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    MPI_Comm_rank(intercomm2, &rank);

    if (rank == 2) {
        err = MPI_Recv(str, 3, MPI_CHAR, 1, 0, intercomm2, MPI_STATUS_IGNORE);
        printf("Parent (first child) received from child 2: %s\n", str);
        fflush(stdout);
        
        err = MPI_Send("bye", 4, MPI_CHAR, 1, 0, intercomm2); 
    }

    MPI_Barrier(intercomm2);
    sleep(5);

    MPI_Finalize();
    return 0;
}
