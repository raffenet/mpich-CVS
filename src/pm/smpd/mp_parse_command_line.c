/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#include <string.h>

void mp_print_options(void)
{
    printf("\n");
    printf("Usage:\n");
    /*
    printf("   mpiexec -n #processes [options] executable [args ...]\n");
    printf("   mpiexec [options] configfile [args ...]\n");
    printf("\n");
    printf("mpiexec options:\n");
    printf("   -n num_processes\n");
    printf("   -host hostname\n");
    printf("   -env var=val\n");
    printf("   -wdir drive:\\my\\working\\directory\n");
    printf("   -wdir /my/working/directory\n");
    printf("   -file configfile\n");
    */
    printf("mpiexec -n <maxprocs> [options] executable [args ...]\n");
    printf("mpiexec [options] executable [args ...] : [options] exe [args] : ...\n");
    printf("mpiexec -file <configfile>\n");
    printf("\n");
    printf("options:\n");
    printf("\n");
    printf("standard:\n");
    printf("-n <maxprocs>\n");
    printf("-wdir <working directory>\n");
    printf("-file <filename> -\n");
    printf("       each line contains a complete set of mpiexec options\n");
    printf("       including the executable and arguments\n");
    printf("-host <hostname>\n");
    printf("-soft <Fortran90 triple> - acceptable number of processes up to maxprocs\n");
    printf("       a or a:b or a:b:c where\n");
    printf("       1) a = a\n");
    printf("       2) a:b = a, a+1, a+2, ..., b\n");
    printf("       3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc\n");
    printf("          where a+kc <= b if c>0\n");
    printf("                a+kc >= b if c<0\n");
    printf("-path <search path for executable, ; separated>\n");
    printf("-arch <architecture> - sun, linux, rs6000, ...\n");
    printf("\n");
    printf("extensions:\n");
    printf("-env <variable=value>\n");
    printf("-env <variable=value;variable2=value2;...>\n");
    printf("-hosts <n host1 host2 ... hostn>\n");
    printf("-hosts <n host1 m1 host2 m2 ... hostn mn>\n");
    printf("-machinefile <filename> - one host per line, #commented\n");
    printf("-localonly <numprocs>\n");
    printf("-nompi - processes never call any MPI functions\n");
    printf("-exitcodes - print the exit codes of processes as they exit\n");
    printf("\n");
    printf("examples:\n");
    printf("mpiexec -n 4 cpi\n");
    printf("mpiexec -n 1 -host foo master : -n 8 slave\n");
    printf("\n");
    printf("For a list of all mpiexec options, execute 'mpiexec -help2'\n");
}

void mp_print_extra_options(void)
{
    printf("\n");
    printf("All options to mpiexec:\n");
    printf("\n");
    printf("-n x\n");
    printf("-np x\n");
    printf("  launch x processes\n");
    printf("-localonly x\n");
    printf("-np x -localonly\n");
    printf("  launch x processes on the local machine\n");
    printf("-machinefile filename\n");
    printf("  use a file to list the names of machines to launch on\n");
    printf("-host hostname\n");
    printf("-hosts n host1 host2 ... hostn\n");
    printf("-hosts n host1 m1 host2 m2 ... hostn mn\n");
    printf("  launch on the specified hosts\n");
    printf("  In the second version the number of processes = m1 + m2 + ... + mn\n");
    printf("-map drive:\\\\host\\share\n");
    printf("  map a drive on all the nodes\n");
    printf("  this mapping will be removed when the processes exit\n");
    printf("-dir drive:\\my\\working\\directory\n");
    printf("-wdir /my/working/directory\n");
    printf("  launch processes in the specified directory\n");
    printf("-env var=val\n");
    printf("-env \"var1=val1;var2=val2;var3=val3...\"\n");
    printf("  set environment variables before launching the processes\n");
    printf("-logon\n");
    printf("  prompt for user account and password\n");
    printf("-pwdfile filename\n");
    printf("  read the account and password from the file specified\n");
    printf("  put the account on the first line and the password on the second\n");
    printf("-nocolor\n");
    printf("  don't use process specific output coloring\n");
    printf("-nompi\n");
    printf("  launch processes without the mpi startup mechanism\n");
    printf("-nomapping\n");
    printf("  don't try to map the current directory on the remote nodes\n");
    printf("-nopopup_debug\n");
    printf("  disable the system popup dialog if the process crashes\n");
    printf("-dbg\n");
    printf("  catch unhandled exceptions\n");
    printf("-exitcodes\n");
    printf("  print the process exit codes when each process exits.\n");
    printf("-noprompt\n");
    printf("  prevent mpirun from prompting for user credentials.\n");
    printf("-priority class[:level]\n");
    printf("  set the process startup priority class and optionally level.\n");
    printf("  class = 0,1,2,3,4   = idle, below, normal, above, high\n");
    printf("  level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest\n");
    printf("  the default is -priority 1:3\n");
    printf("-localroot\n");
    printf("  launch the root process without smpd if the host is local.\n");
    printf("  (This allows the root process to create windows and be debugged.)\n");
    printf("-iproot\n");
    printf("-noiproot\n");
    printf("  use or not the ip address of the root host instead of the host name.\n");
}

static int strip_args(int *argcp, char **argvp[], int n)
{
    int i;

    if (n+1 > (*argcp))
    {
	printf("Error: cannot strip %d args, only %d left.\n", n, (*argcp)-1);
	return SMPD_FAIL;
    }
    for (i=n+1; i<=(*argcp); i++)
    {
	(*argvp)[i-n] = (*argvp)[i];
    }
    (*argcp) -= n;
    return SMPD_SUCCESS;
}

static int isnumber(char *str)
{
    size_t i, n = strlen(str);
    for (i=0; i<n; i++)
    {
	if (!isdigit(str[i]))
	    return SMPD_FALSE;
    }
    return SMPD_TRUE;
}

int mp_get_next_hostname(char *host)
{
    if (gethostname(host, MP_MAX_HOST_LENGTH) == 0)
	return SMPD_SUCCESS;
    return SMPD_FAIL;
}

int mp_get_host_id(char *host, int *id_ptr)
{
    mp_host_node_t *node;
    static int parent = 0;
    static int id = 1;
    int bit, mask, temp;

    /* look for the host in the list */
    node = mp_process.host_list;
    while (node)
    {
	if (strcmp(node->host, host) == 0)
	{
	    /* return the id */
	    *id_ptr = node->id;
	    return SMPD_SUCCESS;
	}
	if (node->next == NULL)
	    break;
	node = node->next;
    }

    /* allocate a new node */
    if (node != NULL)
    {
	node->next = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
	node = node->next;
    }
    else
    {
	node = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
	mp_process.host_list = node;
    }
    if (node == NULL)
    {
	mp_err_printf("malloc failed to allocate a host node structure\n");
	return SMPD_FAIL;
    }
    strcpy(node->host, host);
    node->parent = parent;
    node->id = id;
    node->next = NULL;

    /* move to the next id and parent */
    id++;

    temp = id >> 2;
    bit = 1;
    while (temp)
    {
	bit <<= 1;
	temp >>= 1;
    }
    mask = bit - 1;
    parent = bit | (id & mask);

    /* return the id */
    *id_ptr = node->id;

    return SMPD_SUCCESS;
}

int mp_get_next_host(mp_host_node_t **host_node_pptr, mp_launch_node_t *launch_node)
{
    int result;
    char host[MP_MAX_HOST_LENGTH];
    mp_host_node_t *host_node_ptr;

    if (*host_node_pptr == NULL)
    {
	result = mp_get_next_hostname(host);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to get the next available host name\n");
	    return SMPD_FAIL;
	}
	result = mp_get_host_id(host, &launch_node->host_id);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to get a id for host %s\n", host);
	    return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }

    host_node_ptr = *host_node_pptr;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	free(host_node_ptr);
	host_node_ptr = *host_node_pptr;
	if (host_node_ptr == NULL)
	{
	    mp_err_printf("no more hosts in the list.\n");
	    return SMPD_FAIL;
	}
    }
    result = mp_get_host_id(host_node_ptr->host, &launch_node->host_id);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("unable to get a id for host %s\n", host_node_ptr->host);
	return SMPD_FAIL;
    }
    host_node_ptr->nproc--;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	free(host_node_ptr);
    }

    return SMPD_SUCCESS;
}

int mp_parse_command_args(int *argcp, char **argvp[])
{
    int cur_rank;
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;
    int num_args_to_strip;
    int nproc;
    int run_local = SMPD_FALSE;
    char machine_file_name[MP_MAX_EXE_LENGTH];
    int use_machine_file = SMPD_FALSE;
    mp_map_drive_node *map_node, *drive_map_list;
    mp_env_node *env_node, *env_list;
    char *equal_sign_pos;
    char wdir[MP_MAX_EXE_LENGTH];
    int logon;
    int use_debug_flag;
    char pwd_file_name[MP_MAX_EXE_LENGTH];
    int use_pwd_file;
    mp_host_node_t *host_node_ptr, *host_list, *host_node_iter;
    int no_drive_mapping;
    int n_priority_class, n_priority, use_priorities;
    int index, i;
    char configfilename[MP_MAX_FILENAME];
    int use_configfile;
    char exe[MP_MAX_EXE_LENGTH], args[MP_MAX_EXE_LENGTH];
#ifdef HAVE_WINDOWS_H
    char temp_exe[MP_MAX_EXE_LENGTH], *namepart;
#endif
    mp_launch_node_t *launch_node, *launch_node_iter;

    mp_enter_fn("mp_parse_command_args");

    /* check for console option: must be the first option */
    if (strcmp((*argvp)[1], "-console") == 0)
    {
	if (smpd_get_opt_string(argcp, argvp, "-console", mp_process.console_host, SMPD_MAX_HOST_LENGTH))
	{
	    mp_process.do_console = 1;
	}
	if (smpd_get_opt(argcp, argvp, "-console"))
	{
	    mp_process.do_console = 1;
	    gethostname(mp_process.console_host, SMPD_MAX_HOST_LENGTH);
	}
	if (smpd_get_opt(argcp, argvp, "-p"))
	{
	    mp_process.use_process_session = 1;
	}
	if (*argcp != 1)
	{
	    mp_err_printf("ignoring extra arguments passed with -console option.\n");
	}
	return SMPD_SUCCESS;
    }

    /* check for mpi options */
    /*
     * Required:
     * -n <maxprocs>
     * -host <hostname>
     * -soft <Fortran90 triple> - represents allowed number of processes up to maxprocs
     *        a or a:b or a:b:c where
     *        1) a = a
     *        2) a:b = a, a+1, a+2, ..., b
     *        3) a:b:c = a, a+c, a+2c, a+3c, ..., a+kc
     *           where a+kc <= b if c>0
     *                 a+kc >= b if c<0
     * -wdir <working directory>
     * -path <search path for executable>
     * -arch <architecture> - sun, linux, rs6000, ...
     * -file <filename> - each line contains a complete set of mpiexec options, #commented
     *
     * Extensions:
     * -env <variable=value>
     * -env <variable=value;variable2=value2;...>
     * -hosts <n host1 host2 ... hostn>
     * -hosts <n host1 m1 host2 m2 ... hostn mn>
     * -machinefile <filename> - one host per line, #commented
     * -localonly <numprocs>
     * -nompi - don't require processes to be MPI processes (don't have to call MPI_Init or PMI_Init)
     * -exitcodes - print the exit codes of processes as they exit
     * 
     * Windows extensions:
     * -map <drive:\\host\share>
     * -pwdfile <filename> - account on the first line and password on the second
     * -nomapping - don't copy the current directory mapping on the remote nodes
     * -dbg - debug
     * -noprompt - don't prompt for user credentials, fail with an error message
     * -logon - force the prompt for user credentials
     * -priority <class[:level]> - set the process startup priority class and optionally level.
     *            class = 0,1,2,3,4   = idle, below, normal, above, high
     *            level = 0,1,2,3,4,5 = idle, lowest, below, normal, above, highest
     * -localroot - launch the root process without smpd if the host is local.
     *              (This allows the root process to create windows and be debugged.)
     *
     * Backwards compatibility
     * -np <numprocs>
     * -dir <working directory>
     */

    cur_rank = 0;
    next_argc = *argcp;
    next_argv = *argvp + 1;
    exe_ptr = **argvp;
    do
    {
	/* calculate the current argc and find the next argv */
	argc = 1;
	while ( (*next_argv) != NULL && (**next_argv) != ':')
	{
	    argc++;
	    next_argc--;
	    next_argv++;
	}
	if ( (*next_argv) != NULL && (**next_argv) == ':')
	{
	    (*next_argv) = NULL;
	    next_argv++;
	}
	argcp = &argc;

	/* reset block global variables */
	nproc = 0;
	drive_map_list = NULL;
	env_list = NULL;
	wdir[0] = '\0';
	logon = SMPD_FALSE;
	use_debug_flag = SMPD_FALSE;
	use_pwd_file = SMPD_FALSE;
	host_list = NULL;
	no_drive_mapping = SMPD_FALSE;
	use_priorities = SMPD_FALSE;
	use_configfile = SMPD_FALSE;

	/* parse the current block */

	/* parse the mpiexec options */
	while ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    if ((*argvp)[1][1] == '-')
	    {
		/* double -- option provided, trim it to a single - */
		index = 2;
		while ((*argvp)[1][index] != '\0')
		{
		    (*argvp)[1][index-1] = (*argvp)[1][index];
		    index++;
		}
		(*argvp)[1][index] = '\0';
	    }

	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no number specified after %s option.\n", (*argvp)[1]);
		    return SMPD_FAIL;
		}
		nproc = atoi((*argvp)[2]);
		if (nproc < 1)
		{
		    printf("Error: must specify a number greater than 0 after the %s option\n", (*argvp)[1]);
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localonly") == 0)
	    {
		run_local = SMPD_TRUE;
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
			if (nproc != 0)
			{
			    printf("Error: only one option is allowed to determine the number of processes.\n");
			    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
			    return SMPD_FAIL;
			}
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2;
		    }
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "machinefile") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no filename specified after -machinefile option.\n");
		    return SMPD_FAIL;
		}
		strcpy(machine_file_name, (*argvp)[2]);
		use_machine_file = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "map") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no drive specified after -map option.\n");
		    return SMPD_FAIL;
		}
		if ((strlen((*argvp)[2]) > 2) && (*argvp)[2][1] == ':')
		{
		    map_node = (mp_map_drive_node*)malloc(sizeof(mp_map_drive_node));
		    if (map_node == NULL)
		    {
			printf("Error: malloc failed to allocate map structure.\n");
			return SMPD_FAIL;
		    }
		    map_node->drive = (*argvp)[2][0];
		    strcpy(map_node->share, &(*argvp)[2][2]);
		    map_node->next = drive_map_list;
		    drive_map_list = map_node;
		}
		num_args_to_strip = 2;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "dir") == 0) || (strcmp(&(*argvp)[1][1], "wdir") == 0) )
	    {
		if (argc < 3)
		{
		    printf("Error: no directory after %s option\n", (*argvp)[1]);
		    return SMPD_FAIL;
		}
		strcpy(wdir, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "env") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no environment variables after -env option\n");
		    return SMPD_FAIL;
		}
		env_node = (mp_env_node*)malloc(sizeof(mp_env_node));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    return SMPD_FAIL;
		}
		equal_sign_pos = strstr((*argvp)[2], "=");
		if (equal_sign_pos == NULL)
		{
		    printf("Error: improper environment variable option. '%s %s' is not in the format '-env var=value'\n",
			(*argvp)[1], (*argvp)[2]);
		    return SMPD_FAIL;
		}
		*equal_sign_pos = '\0';
		strcpy(env_node->name, (*argvp)[2]);
		strcpy(env_node->value, equal_sign_pos+1);
		env_node->next = env_list;
		env_list = env_node;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "logon") == 0)
	    {
		logon = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noprompt") == 0)
	    {
		mp_process.credentials_prompt = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "dbg") == 0)
	    {
		use_debug_flag = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "pwdfile") == 0)
	    {
		use_pwd_file = SMPD_TRUE;
		if (argc < 3)
		{
		    printf("Error: no filename specified after -pwdfile option\n");
		    return SMPD_FAIL;
		}
		strcpy(pwd_file_name, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    return SMPD_FAIL;
		}
		strcpy(configfilename, (*argvp)[2]);
		use_configfile = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -host option.\n");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -host option can only be called once and it cannot be combined with -hosts.\n");
		    return SMPD_FAIL;
		}
		/* create a host list of one and set nproc to -1 to be replaced by
		   nproc after parsing the block */
		host_list = (mp_host_node_t*)malloc(sizeof(mp_host_node_t));
		if (host_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    return SMPD_FAIL;
		}
		host_list->next = NULL;
		host_list->nproc = -1;
		strcpy(host_list->host, (*argvp)[2]);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "hosts") == 0)
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -hosts option can only be called once and it cannot be combined with -host.\n");
		    return SMPD_FAIL;
		}
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
			/* initially set nproc to be the number of hosts */
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: You must specify a number greater than 0 after -hosts.\n");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2 + nproc;
			index = 3;
			for (i=0; i<nproc; i++)
			{
			    if (index >= argc)
			    {
				printf("Error: missing host name after -hosts option.\n");
				return SMPD_FAIL;
			    }
			    host_node_ptr = (mp_host_node_t*)malloc(sizeof(mp_host_node_t));
			    if (host_node_ptr == NULL)
			    {
				printf("failed to allocate memory for a host node.\n");
				return SMPD_FAIL;
			    }
			    host_node_ptr->next = NULL;
			    host_node_ptr->nproc = 1;
			    strcpy(host_node_ptr->host, (*argvp)[index]);
			    index++;
			    if (argc > index)
			    {
				if (isnumber((*argvp)[index]))
				{
				    host_node_ptr->nproc = atoi((*argvp)[index]);
				    index++;
				    num_args_to_strip++;
				}
			    }
			    if (host_list == NULL)
			    {
				host_list = host_node_ptr;
			    }
			    else
			    {
				host_node_iter = host_list;
				while (host_node_iter->next)
				    host_node_iter = host_node_iter->next;
				host_node_iter->next = host_node_ptr;
			    }
			}

			/* adjust nproc to be the actual number of processes */
			host_node_iter = host_list;
			nproc = 0;
			while (host_node_iter)
			{
			    nproc += host_node_iter->nproc;
			    host_node_iter = host_node_iter->next;
			}
		    }
		    else
		    {
			printf("Error: You must specify the number of hosts after the -hosts option.\n");
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: not enough arguments specified for -hosts option.\n");
		    return SMPD_FAIL;
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "nocolor") == 0)
	    {
		mp_process.do_multi_color_output = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nompi") == 0)
	    {
		mp_process.no_mpi = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nomapping") == 0)
	    {
		no_drive_mapping = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nopopup_debug") == 0)
	    {
#ifdef HAVE_WINDOWS_H
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#endif
	    }
	    /* catch -help, -?, and --help */
	    else if (strcmp(&(*argvp)[1][1], "help") == 0 || (*argvp)[1][1] == '?' || strcmp(&(*argvp)[1][1], "-help") == 0)
	    {
		mp_print_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "help2") == 0)
	    {
		mp_print_extra_options();
		exit(0);
	    }
	    else if (strcmp(&(*argvp)[1][1], "exitcodes") == 0)
	    {
		mp_process.output_exit_codes = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localroot") == 0)
	    {
		mp_process.local_root = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "priority") == 0)
	    {
		char *str;
		n_priority_class = atoi((*argvp)[2]);
		str = strchr((*argvp)[2], ':');
		if (str)
		{
		    str++;
		    n_priority = atoi(str);
		}
		mp_dbg_printf("priorities = %d:%d\n", n_priority_class, n_priority);
		use_priorities = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "iproot") == 0)
	    {
		mp_process.use_iproot = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noiproot") == 0)
	    {
		mp_process.use_iproot = SMPD_FALSE;
	    }
	    else
	    {
		printf("Unknown option: %s\n", (*argvp)[1]);
	    }
	    strip_args(argcp, argvp, num_args_to_strip);
	}

	if (use_configfile)
	{
	    /* parse configuration file */
	    mp_err_printf("configuration file parsing not implemented yet.\n");
	    return SMPD_FAIL;
	}
	else
	{
	    /* remaining args are the executable and it's args */
	    if (argc < 2)
	    {
		printf("Error: no executable specified\n");
		return SMPD_FAIL;
	    }
	    
	    strncpy(exe, (*argvp)[1], MP_MAX_EXE_LENGTH);
	    exe[MP_MAX_EXE_LENGTH-1] = '\0';

	    args[0] = '\0';
	    for (i = 2; i<argc; i++)
	    {
		strncat(args, (*argvp)[i], MP_MAX_EXE_LENGTH - 1 - strlen(args));
		if (i < argc-1)
		{
		    strncat(args, " ", MP_MAX_EXE_LENGTH - 1 - strlen(args));
		}
	    }
#ifdef HAVE_WINDOWS_H
	    /* Fix up the executable name */
	    if (exe[0] == '\\' && exe[1] == '\\')
	    {
		strncpy(temp_exe, exe, MP_MAX_EXE_LENGTH);
		temp_exe[MP_MAX_EXE_LENGTH-1] = '\0';
	    }
	    else
	    {
		GetFullPathName(exe, MAX_PATH, temp_exe, &namepart);
	    }
	    /* Quote the executable in case there are spaces in the path */
	    sprintf(exe, "\"%s\"", temp_exe);
#endif

	    mp_dbg_printf("handling executable:\n%s %s\n", exe, args);
	}

	if (nproc == 0)
	{
	    mp_err_printf("missing num_proc flag: -n, -np, or -hosts.\n");
	    return SMPD_FAIL;
	}
	if (host_list != NULL && host_list->nproc == -1)
	{
	    /* -host specified, replace nproc field */
	    host_list->nproc = nproc;
	}

	for (i=0; i<nproc; i++)
	{
	    /* create a launch_node */
	    launch_node = (mp_launch_node_t*)malloc(sizeof(mp_launch_node_t));
	    if (launch_node == NULL)
	    {
		mp_err_printf("unable to allocate a launch node structure.\n");
		return SMPD_FAIL;
	    }
	    mp_get_next_host(&host_list, launch_node);
	    launch_node->iproc = cur_rank++;
	    launch_node->env = launch_node->env_data;
	    launch_node->env_data[0] = '\0';
	    if (args[0] != '\0')
		sprintf(launch_node->exe, "%s %s", exe, args);
	    else
		strcpy(launch_node->exe, exe);
	    launch_node->next = NULL;
	    if (mp_process.launch_list == NULL)
		mp_process.launch_list = launch_node;
	    else
	    {
		launch_node_iter = mp_process.launch_list;
		while (launch_node_iter->next)
		    launch_node_iter = launch_node_iter->next;
		launch_node_iter->next = launch_node;
	    }
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    mp_exit_fn("mp_parse_command_args");
    return SMPD_SUCCESS;
}
