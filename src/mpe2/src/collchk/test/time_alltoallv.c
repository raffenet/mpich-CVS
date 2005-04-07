/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

/*
  This program tests MPI_Alltoallv by having processor i send different
  amounts of data to each processor.

  Because there are separate send and receive types to alltoallv,
  there need to be tests to rearrange data on the fly.  Not done yet.
  
  The first test sends i items to processor i from all processors.

  Currently, the test uses only MPI_INT; this is adequate for testing systems
  that use point-to-point operations
 */

#define BLOCK_SIZE 100000

int main( int argc, char **argv )
{

    MPI_Comm      comm;
    MPI_Datatype  elemtype;
    double        time_init, time_final;
    int          *sbuff, *rbuff;
    int           rank, size;
    int          *sendcounts, *recvcounts, *rdispls, *sdispls;
    int           ii, jj, idx;

    MPI_Init( &argc, &argv );

      comm = MPI_COMM_WORLD;

      MPI_Comm_size( comm, &size );
      MPI_Comm_rank( comm, &rank );

      /* Create the buffer */
      MPI_Type_contiguous( BLOCK_SIZE, MPI_DOUBLE, &elemtype );
      MPI_Type_commit( &elemtype );
      sbuff = (int *)malloc( size * size * BLOCK_SIZE * sizeof(double) );
      rbuff = (int *)malloc( size * size * BLOCK_SIZE * sizeof(double) );
      if (!sbuff || !rbuff) {
        fprintf( stderr, "Could not allocated buffers!\n" );
        MPI_Abort( comm, 1 );
      }

      /* Load up the buffers */
      for ( ii = 0; ii < size*size; ii++ ) {
        for ( jj = 0; jj < BLOCK_SIZE; jj++ ) {
          idx        = ii * BLOCK_SIZE + jj;
          sbuff[idx] = ii + 100*rank;
          rbuff[idx] = -ii;
        }
      }

      /* Create and load the arguments to alltoallv */
      sendcounts = (int *)malloc( size * sizeof(int) );
      recvcounts = (int *)malloc( size * sizeof(int) );
      rdispls    = (int *)malloc( size * sizeof(int) );
      sdispls    = (int *)malloc( size * sizeof(int) );
      if (!sendcounts || !recvcounts || !rdispls || !sdispls) {
        fprintf( stderr, "Could not allocate arg items!\n" );
        MPI_Abort( comm, 1 );
      }
      for ( ii = 0; ii < size; ii++ ) {
        sendcounts[ii] = ii;
        recvcounts[ii] = rank;
        rdispls[ii]    = ii * rank;
        sdispls[ii]    = (ii * (ii+1))/2;
      }

      time_init   = MPI_Wtime();
      MPI_Alltoallv( sbuff, sendcounts, sdispls, elemtype,
                     rbuff, recvcounts, rdispls, elemtype, comm );
      time_final  = MPI_Wtime();
      
      fprintf( stdout, "time taken by MPI_AlltoAllv() at rank %d = %f\n",
                       rank, time_final - time_init );
      fflush( stdout );

      MPI_Type_free( &elemtype );
      
      free( sdispls );
      free( rdispls );
      free( recvcounts );
      free( sendcounts );
      free( rbuff );
      free( sbuff );

    MPI_Finalize( );
    return 0;
}
