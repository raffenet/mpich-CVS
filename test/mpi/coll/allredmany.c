#include <stdio.h>
#include "mpi.h"

/*
 * This example should be run with 2 processes and tests the ability of the
 * implementation to handle a flood of one-way messages.
 */

int main( int argc, char **argv )
{
  double wscale = 10.0, scale;
  int numprocs, myid,i,namelen;

  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);

  for ( i=0; i<10000; i++) {
    MPI_Allreduce(&wscale,&scale,1,MPI_DOUBLE,MPI_SUM,MPI_COMM_WORLD);
  }

  MPI_Finalize();
  printf("Done\n");
  
  return 0;
}
