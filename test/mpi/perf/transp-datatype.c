#include "mpi.h"
#include <stdio.h>

#define SIZE 100
#define ITER 200

int main(int argc, char* argv[])
{
    int i, j;
    static double a[SIZE][SIZE],b[SIZE][SIZE];
    double t1,t2,t,ts1,ts2,ts;
    int myrank;
    MPI_Status status;
    MPI_Aint sizeofreal;

    MPI_Datatype row, xpose;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&myrank);
    MPI_Type_extent(MPI_DOUBLE, &sizeofreal);
 
    MPI_Type_vector(SIZE, 1, SIZE, MPI_DOUBLE, &row);
    MPI_Type_hvector(SIZE, 1, sizeofreal, row, &xpose);
    MPI_Type_commit(&xpose);

    for (i=0; i<SIZE; i++)
	for (j=0; j<SIZE; j++)
	    a[i][j]=0;
    a[SIZE-1][0] = 1;

    MPI_Barrier(MPI_COMM_WORLD);
    t1=MPI_Wtime();
    for(i=0;i< ITER; i++)
	{
	    if(myrank==0)
		MPI_Send(&a[0][0],1,xpose,1,0,MPI_COMM_WORLD);
	    else 
		MPI_Recv(&b[0][0],1,xpose,0,0,
		// MPI_Recv(&b[0][0],SIZE*SIZE,MPI_DOUBLE,0,0,
			 MPI_COMM_WORLD,&status);
	}
    t2=MPI_Wtime();
    t=(t2-t1)/ITER;
    if(myrank==1) {
//	printf("%f\n", b[0][SIZE-1]);
	printf("%f|",t);
	fflush(stdout);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    ts1=MPI_Wtime();
 
    for(i=0; i< ITER; i++){
	if(myrank==0)
	    {
		MPI_Send(&a[0][0],sizeof(a),MPI_BYTE,1,0,MPI_COMM_WORLD);
	    }
	else {
		MPI_Recv(&b[0][0],sizeof(b),MPI_BYTE,0,0,MPI_COMM_WORLD,&status);
		/*		for(i=0;i<SIZE;i++)
		    for(j=0;j<SIZE;j++)
		    b[i][j]=b[j][i]; */
	}
    }

    ts2=MPI_Wtime();
    ts=(ts2-ts1)/ITER;

    if(myrank==0)
	printf("%f\n",ts);
    else
	printf("%d: exit\n",myrank); 


    MPI_Finalize();
    return 0;
}
