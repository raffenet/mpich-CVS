#include <stdio.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    char str[10];
    int err=0, errcodes[256], rank;
    MPI_Comm intercomm;

    MPI_Init(&argc, &argv);

    err = MPI_Comm_spawn("child", MPI_ARGV_NULL, 2,
                         MPI_INFO_NULL, 0, MPI_COMM_WORLD,
                         &intercomm, errcodes);  
    if (err) printf("Error in MPI_Comm_spawn\n");

    printf("Parent out of MPI_Comm_spawn\n");
    fflush(stdout);
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        err = MPI_Recv(str, 3, MPI_CHAR, 0, 0, intercomm, MPI_STATUS_IGNORE);
        printf("Parent received from child: %s\n", str);
        fflush(stdout);
        
        err = MPI_Send("bye", 4, MPI_CHAR, rank, 0, intercomm); 
    } 

    /*sleep(5);*/
    MPI_Finalize();

    return 0;
}

