#include "mpi.h"
#include "mpitest.h"

/* 
 * Initialize and Finalize MTest
 */
void MTest_Init( int *argc, char *(*argv)[] )
{
    int flag;

    MPI_Initialized( &flag );
    if (!flag) {
	MPI_Init( argc, argv );
    }
}

void MTest_Finalize( int errs )
{
    int size, rank, toterrs;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    
    MPI_Allreduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterrs) {
	    printf( " Found %d errors\n" );
	}
	else {
	    printf( " No errors\n" );
	}
    }
}

/*
 * Datatypes
 *
 * Eventually, this could read a description of a file.  For now, we hard 
 * code the choices
 *
 */
static int datatype_index = 0;
int MTest_Get_datatypes( MTest_Datatype *sendtype, MTest_Datatype *recvtype )
{
    /* Use datatype_index to choose a datatype to use.  If at the end of the
       list, return 0 */
    switch (datatype_index) {
    case 0:
	sendtype->InitBuf  = ;
	sendtype->FreeBuf  = ;
	sendtype->datatype = ;
	recvtype->InitBuf  = ;
	recvtype->FreeBuf  = ;
	recvtype->datatype = ;
	break;
    case 1:
	break;
    default:
	datatype_index = -1;
    }
    datatype_index++;
    return datatype_index;
}

void MTest_Reset_datatypes( void )
{
    datatype_index = 0;
}

int MTest_Check_recv( MPI_Status *status, MTest_Datatype *recvtype )
{
    int count;
    int errs = 0;

    MPI_Get_count( status, recvtype->datatype, &count );
    /* Check count against expected count */

    if (count != recvtype->count) {
	errs ++;
    }

    /* Check received data */
    if (!errs && recvtype->CheckBuf( recvtype )) {
	errs++;
    }
    return errs;
}
