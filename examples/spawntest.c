/* 
   If you simply run:
       mpdrun.py -np 1 spawntest 1 /bin/hostname
   it will HANG because hostname does not do the necessary pmi calls to 
   check into the pmi barrier.
   You really need to spawn a program that will make the necessary calls
   to check in to the pmi barrier. So, a quick test is to have this pgm
   spawn itself:
       mpdrun.py -np 1 spawntest 1 spawntest spawned
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

int main( int argc, char *argv[] )
{
    int rank;
    int size;
    int nprocs,maxprocs, errs, same_domain;
    char *cmds[32];          /* 32 cmd names */
    char *args_1_cmd[64];    /* up to 64 args for each of the 32 cmds */
    char *args[32];          /* arg sets for all of the 32 cmds */
    char spawned_kvsname[4096];  /* 4096 is hardcoded current maxlen */
    char my_kvsname[4096];  /* 4096 is hardcoded current maxlen */
    char value[4096];
    
    MPI_Init( 0, 0 );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf( "Spawn test from process %d of %d\n", rank, size );
    if (argc > 1)
        nprocs = atoi(argv[1]);
    else
	nprocs = 1;
    if (argc > 2)
	cmds[0] = argv[2];
    else
        cmds[0] = "/bin/hostname";
    if (argc > 3)
	args_1_cmd[0] = argv[3];
    else
	args_1_cmd[0] = "";
    if (argc > 1  &&  strcmp(argv[1],"spawned") == 0)
    {
	printf("I got spawned\n");
	exit(0);
    }
    args[0] = (char *)&(args_1_cmd[0]);
    /*****
    if (PMI_KVS_Create(spawned_kvsname) < 0)
    {
	printf("failed to get new kvs name; exiting\n");
	exit(-1);
    }
    *****/
    PMI_Spawn_multiple(nprocs,cmds,args,&maxprocs,spawned_kvsname,&errs,&same_domain,NULL);
    printf("spawned kvs name=:%s:\n",spawned_kvsname);
    PMI_KVS_Get( spawned_kvsname, "rmb_test", value);
    printf("spawned kvs has dummy_for_test=:%s:\n",value);
    MPI_Finalize();
    return 0;
}
