#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/*
Mpiexec:main()
{
  /-->MPI_Init()
  |   parse standard mpiexec arguments - host, n, path, wdir, etc
  |/->MPID_Args_to_info()
  ||    parse device specific options and call PMI_args_to_info()
  ||    if necessary
  |\-loop
  \--loop
     MPI_Finalize()
        MPID_Finalize()
          PMI_Finalize() - block until output is redirected if singleton. }
*/

int parse_arguments(int *argcp, char **argvp[], int *count, char ***array_of_commands, char****array_of_argv, int **array_of_maxprocs, MPI_Info **array_of_info);

int main(int argc, char *argv[])
{
    int i, count;
    char **cmds;
    char ***argvs;
    int *maxprocs;
    MPI_Info *infos;
    MPI_Comm intercomm;
    int *errors;
    int result;
    char error_string[1024];
    int error_length;

    result = MPI_Init(&argc, &argv);
    if (result != MPI_SUCCESS)
    {
	printf("Unable to initialize MPI.\n");
	return result;
    }

    result = parse_arguments(&argc, &argv, &count, &cmds, &argvs, &maxprocs, &infos);
    if (result == 0)
    {
	errors = (int*)malloc(count * sizeof(int));
	result = MPI_Comm_spawn_multiple(count, cmds, argvs, maxprocs, infos, 0, MPI_COMM_WORLD, &intercomm, errors);
	if (result != MPI_SUCCESS)
	{
	    printf("Unable to create the processes.\n");
	    for (i=0; errors && i<count; i++)
	    {
		error_length = 1024;
		MPI_Error_string(errors[i], error_string, &error_length);
		if (count > 1)
		{
		    printf("[%d]command: %s\n", i, cmds[i]);
		    printf("[%d]error: %s\n", i, error_string);
		}
		else
		{
		    printf("command: %s\n", cmds[0]);
		    printf("error: %s\n", error_string);
		}
	    }
	}
    }

    result = MPI_Finalize();
    if (result != MPI_SUCCESS)
    {
	printf("Unable to finalize MPI.\n");
	return result;
    }
}
