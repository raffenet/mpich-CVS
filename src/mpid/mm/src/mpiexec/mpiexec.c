#include <stdio.h>
#include "mpi.h"
#include "pmi.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

void PrintOptions()
{
    fprintf( stderr, "Usage: mpiexec -n <numprocs> -soft <softness> -host <hostname> \\\n" );
    fprintf( stderr, "               -wdir <working directory> -path <search path> \\\n" );
    fprintf( stderr, "               -file <filename> execname <args>\n" );
}

static void StripArgs(int *argc, char ***argv, int n)
{
    int i;
    if (n+1 > *argc)
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, *argc-1);
    }
    for (i=n+1; i<=*argc; i++)
    {
	(*argv)[i-n] = (*argv)[i];
    }
    *argc -= n;
}

int CreateParameters(int *argcp, char **argvp[], int *pCount, char ***pCmds, char ****pArgvs, int **pMaxprocs, void **pInfos, int **pErrors)
{
    int i;
    int nArgsToStrip;
    int argc;
    char **argv;
    int nProc = 0;
    int bRunLocal = FALSE;
    int bUseMachineFile = FALSE;
    char pszMachineFileName[MAX_PATH];
    char pszDir[MAX_PATH] = ".";
    char pszEnv[MAX_PATH] = "";
    char pszExe[MAX_PATH] = "";
    char pszArgs[MAX_PATH] = "";
    char pszSoft[100];
    char pszHost[100];
    char pszArch[100];
    char pszPath[MAX_PATH];
    char pszFile[MAX_PATH];

    PMI_Args_to_info(argcp, argvp, pInfos[0]);

    argc = *argcp;
    argv = *argvp;
    while (argv[1] && (argv[1][0] == '-' || argv[1][0] == '/'))
    {
	nArgsToStrip = 1;
	if (strcmp(&argv[1][1], "n") == 0 || strcmp(&argv[1][1], "np") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no number specified after -n option.\n");
		return 0;
	    }
	    nProc = atoi(argv[2]);
	    if (nProc < 1)
	    {
		printf("Error: must specify a number greater than 0 after the -n option\n");
		return 0;
	    }
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "wdir") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no directory after -wdir option\n");
		return 0;
	    }
	    strcpy(pszDir, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "soft") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: nothing after -soft option\n");
		return 0;
	    }
	    strcpy(pszSoft, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "host") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no hostname after -host option\n");
		return 0;
	    }
	    strcpy(pszHost, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "arch") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no architecture after -arch option\n");
		return 0;
	    }
	    strcpy(pszArch, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "path") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no path after -path option\n");
		return 0;
	    }
	    strcpy(pszPath, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "file") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no filename after -file option\n");
		return 0;
	    }
	    strcpy(pszFile, argv[2]);
	    nArgsToStrip = 2;
	}
	else if (strcmp(&argv[1][1], "localonly") == 0)
	{
	    bRunLocal = TRUE;
	    if (argc > 2)
	    {
		if (isdigit(argv[2][0]))
		{
		    nProc = atoi(argv[2]);
		    if (nProc < 1)
		    {
			printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			return 0;
		    }
		    nArgsToStrip = 2;
		}
	    }
	}
	else if (strcmp(&argv[1][1], "machinefile") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no filename specified after -machinefile option.\n");
		return 0;
	    }
	    strcpy(pszMachineFileName, argv[2]);
	    bUseMachineFile = TRUE;
	    nArgsToStrip = 2;
	}
	/*
	else if (strcmp(&argv[1][1], "map") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no drive specified after -map option.\n");
		return 0;
	    }
	    if ((strlen(argv[2]) > 2) && argv[2][1] == ':')
	    {
		MapDriveNode *pNode = new MapDriveNode;
		pNode->cDrive = argv[2][0];
		strcpy(pNode->pszShare, &argv[2][2]);
		pNode->pNext = g_pDriveMapList;
		g_pDriveMapList = pNode;
	    }
	    nArgsToStrip = 2;
	}
	*/
	else if (strcmp(&argv[1][1], "env") == 0)
	{
	    if (argc < 3)
	    {
		printf("Error: no environment variables after -env option\n");
		return 0;
	    }
	    strcpy(pszEnv, argv[2]);
	    nArgsToStrip = 2;
	}
	/*
	else if (strcmp(&argv[1][1], "logon") == 0)
	{
	    bLogon = TRUE;
	}
	else if (strcmp(&argv[1][1], "tcp") == 0)
	{
	    bDoSMP = false;
	}
	else if (strcmp(&argv[1][1], "getphrase") == 0)
	{
	    GetMPDPassPhrase(phrase);
	    bPhraseNeeded = false;
	}
	else if (strcmp(&argv[1][1], "nocolor") == 0)
	{
	    g_bDoMultiColorOutput = false;
	}
	else if (strcmp(&argv[1][1], "nompi") == 0)
	{
	    g_bNoMPI = TRUE;
	}
	else if (strcmp(&argv[1][1], "nodots") == 0)
	{
	    bLogonDots = false;
	}
	else if (strcmp(&argv[1][1], "nomapping") == 0)
	{
	    g_bNoDriveMapping = TRUE;
	}
	*/
	else if (strcmp(&argv[1][1], "help") == 0 || argv[1][1] == '?')
	{
	    PrintOptions();
	    return 0;
	}
	else
	{
	    printf("Unknown option: %s\n", argv[1]);
	}
	StripArgs(argcp, argvp, nArgsToStrip);
	argc = *argcp;
	argv = *argvp;
    }

    if (argc < 2)
    {
	printf("Error: no executable or configuration file specified\n");
	PrintOptions();
	return 0;
    }

    /* The next argument is the executable or a configuration file */
    strcpy(pszExe, argv[1]);

    /* All the rest of the arguments are passed to the application */
    pszArgs[0] = '\0';
    for (i = 2; i<argc; i++)
    {
	strcat(pszArgs, argv[i]);
	if (i < argc-1)
	    strcat(pszArgs, " ");
    }
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

    CreateParameters(&argc, &argv, &count, &cmds, &argvs, &maxprocs, &infos, &errors);

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
