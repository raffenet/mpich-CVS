#include <stdio.h>
#include "mpi.h"

int CreateParameters(int argc, char *argv[], int *pCount, char ***pCmds, char ****pArgvs, int **pMaxprocs, void **pInfos, int **pErrors)
{
    return 0;
}

int main(int argc, char *argv[])
{
    int count;
    char **cmds;
    char ***argvs;
    int *maxprocs;
    void *infos;
    MPI_Comm intercomm;
    int *errors;

    CreateParameters(argc, argv, &count, &cmds, &argvs, &maxprocs, &infos, &errors);

    MPI_Init(NULL, NULL);

    MPI_Comm_spawn_multiple(
	count, 
	cmds, 
	argvs, 
	maxprocs, 
	infos, 
	0, 
	MPI_COMM_WORLD, 
	&intercomm, 
	errors);

    MPI_Finalize();
}
