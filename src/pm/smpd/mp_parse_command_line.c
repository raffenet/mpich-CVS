/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#if defined(HAVE_DIRECT_H) || defined(HAVE_WINDOWS_H)
#include <direct.h>
#endif

void mp_print_options(void)
{
    printf("\n");
    printf("Usage:\n");
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

    printf("-port port\n");
    printf("-p port\n");
    printf("  specify the port that smpd is listening on.\n");
    printf("-phrase passphrase\n");
    printf("  specify the passphrase to authenticate connections to smpd with.\n");
    printf("-smpdfile filename\n");
    printf("  specify the file where the smpd options are stored including the passphrase.\n");
    printf("-soft Fortran90_triple\n");
    printf("  acceptable number of processes to launch up to maxprocs\n");
    printf("-path search_path\n");
    printf("  search path for executable, ; separated\n");
    printf("-arch architecture\n");
    printf("  sun, linux, rs6000, ...\n");
    printf("-register\n");
    printf("  encrypt a user name and password to the Windows registry.\n");
    printf("-remove\n");
    printf("  delete the encrypted credentials from the Windows registry.\n");
    printf("-validate [-host hostname]\n");
    printf("  validate the encrypted credentials for the current or specified host.\n");
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

int mp_parse_command_args(int *argcp, char **argvp[])
{
    int cur_rank;
    int argc, next_argc;
    char **next_argv;
    char *exe_ptr;
    int num_args_to_strip;
    int nproc;
    int run_local = SMPD_FALSE;
    char machine_file_name[SMPD_MAX_FILENAME];
    int use_machine_file = SMPD_FALSE;
    smpd_map_drive_node_t *map_node, *drive_map_list;
    smpd_env_node_t *env_node, *env_list;
    char *env_str, env_data[SMPD_MAX_ENV_LENGTH];
    char *equal_sign_pos;
    char wdir[SMPD_MAX_DIR_LENGTH];
    int use_debug_flag;
    char pwd_file_name[SMPD_MAX_FILENAME];
    int use_pwd_file;
    smpd_host_node_t *host_node_ptr, *host_list, *host_node_iter;
    int no_drive_mapping;
    int n_priority_class, n_priority, use_priorities;
    int index, i;
    char configfilename[SMPD_MAX_FILENAME];
    int use_configfile;
    char exe[SMPD_MAX_EXE_LENGTH], *exe_iter;
    char exe_path[SMPD_MAX_EXE_LENGTH], *namepart;
    smpd_launch_node_t *launch_node, *launch_node_iter;
    int exe_len_remaining;
    char path[SMPD_MAX_PATH_LENGTH];
    char temp_password[SMPD_MAX_PASSWORD_LENGTH];
    FILE *fin_config;
    int result;

    smpd_enter_fn("mp_parse_command_args");

#ifdef HAVE_WINDOWS_H
    if (*argcp > 1)
    {
	if (strcmp((*argvp)[1], "-register") == 0)
	{
	    for (;;)
	    {
		smpd_get_account_and_password(smpd_process.UserAccount, smpd_process.UserPassword);
		printf("confirm password: ");fflush(stdout);
		smpd_get_password(temp_password);
		if (strcmp(smpd_process.UserPassword, temp_password) == 0)
		    break;
		printf("passwords don't match, please try again.\n");
	    }
	    if (smpd_setup_crypto_client())
	    {
		if (smpd_save_password_to_registry(smpd_process.UserAccount, smpd_process.UserPassword, SMPD_TRUE)) 
		{
		    printf("Password encrypted into the Registry.\n");
		    smpd_delete_cached_password();
		}
		else
		{
		    printf("Error: Unable to save encrypted password.\n");
		}
	    }
	    else
	    {
		printf("Error: Unable to setup the encryption service.\n");
	    }
	    smpd_exit(0);
	}
	if ( (strcmp((*argvp)[1], "-remove") == 0) || (strcmp((*argvp)[1], "-unregister") == 0) )
	{
	    if (smpd_delete_current_password_registry_entry())
	    {
		smpd_delete_cached_password();
		printf("Account and password removed from the Registry.\n");
	    }
	    else
	    {
		printf("ERROR: Unable to remove the encrypted password.\n");
	    }
	    smpd_exit(0);
	}
	if (strcmp((*argvp)[1], "-validate") == 0)
	{
	    if (smpd_setup_crypto_client())
	    {
		if (smpd_read_password_from_registry(smpd_process.UserAccount, smpd_process.UserPassword))
		{
		    if (!smpd_get_opt_string(argcp, argvp, "-host", smpd_process.console_host, SMPD_MAX_HOST_LENGTH))
		    {
			DWORD len = SMPD_MAX_HOST_LENGTH;
			GetComputerName(smpd_process.console_host, &len);
		    }
		    smpd_get_opt_int(argcp, argvp, "-port", &smpd_process.port);
		    smpd_get_opt_string(argcp, argvp, "-phrase", smpd_process.passphrase, SMPD_PASSPHRASE_MAX_LENGTH);
		    smpd_process.validate = SMPD_TRUE;
		    smpd_do_console();
		}
		else
		{
		    printf("FAIL: Unable to read the credentials from the registry.\n");fflush(stdout);
		}
	    }
	    else
	    {
		printf("FAIL: Unable to setup the encryption service.\n");fflush(stdout);
	    }
	    smpd_exit(0);
	}
    }
#endif

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
     * -verbose - same as setting environment variable to SMPD_DBG_OUTPUT=stdout
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

    /* Get a list of hosts from a file or the registry to be used with the -n,-np options */
    smpd_get_default_hosts();

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
	use_configfile = SMPD_FALSE;
configfile_loop:
	nproc = 0;
	drive_map_list = NULL;
	env_list = NULL;
	wdir[0] = '\0';
	use_debug_flag = SMPD_FALSE;
	use_pwd_file = SMPD_FALSE;
	host_list = NULL;
	no_drive_mapping = SMPD_FALSE;
	use_priorities = SMPD_FALSE;
	use_machine_file = SMPD_FALSE;
	path[0] = '\0';

	/* Check for the -file option.  It must be the first and only option in a group. */
	if ((*argvp)[1] && (*argvp)[1][0] == '-')
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
		(*argvp)[1][index-1] = '\0';
	    }
	    if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		if (use_configfile)
		{
		    printf("Error: -file option is not valid from within a configuration file.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], SMPD_MAX_FILENAME);
		use_configfile = SMPD_TRUE;
		fin_config = fopen(configfilename, "r");
		if (fin_config == NULL)
		{
		    printf("Error: unable to open config file '%s'\n", configfilename);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (!smpd_get_argcv_from_file(fin_config, argcp, argvp))
		{
		    fclose(fin_config);
		    printf("Error: unable to parse config file '%s'\n", configfilename);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
	    }
	}

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
		(*argvp)[1][index-1] = '\0';
	    }

	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no number specified after %s option.\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		nproc = atoi((*argvp)[2]);
		if (nproc < 1)
		{
		    printf("Error: must specify a number greater than 0 after the %s option\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
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
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			nproc = atoi((*argvp)[2]);
			if (nproc < 1)
			{
			    printf("Error: If you specify a number after -localonly option,\n        it must be greater than 0.\n");
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2;
		    }
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "machinefile") == 0)
	    {
		if (s_host_list != NULL)
		{
		    printf("Error: -machinefile can only be specified once per section.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (argc < 3)
		{
		    printf("Error: no filename specified after -machinefile option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(machine_file_name, (*argvp)[2], SMPD_MAX_FILENAME);
		use_machine_file = SMPD_TRUE;
		smpd_parse_machine_file(machine_file_name);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "map") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no drive specified after -map option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if ((strlen((*argvp)[2]) > 2) && (*argvp)[2][1] == ':')
		{
		    map_node = (smpd_map_drive_node_t*)malloc(sizeof(smpd_map_drive_node_t));
		    if (map_node == NULL)
		    {
			printf("Error: malloc failed to allocate map structure.\n");
			smpd_exit_fn("mp_parse_command_args");
			return SMPD_FAIL;
		    }
		    map_node->ref_count = 0;
		    map_node->drive = (*argvp)[2][0];
		    strncpy(map_node->share, &(*argvp)[2][2], SMPD_MAX_EXE_LENGTH);
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
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(wdir, (*argvp)[2], SMPD_MAX_DIR_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "env") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no environment variables after -env option\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		env_node = (smpd_env_node_t*)malloc(sizeof(smpd_env_node_t));
		if (env_node == NULL)
		{
		    printf("Error: malloc failed to allocate structure to hold an environment variable.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		equal_sign_pos = strstr((*argvp)[2], "=");
		if (equal_sign_pos == NULL)
		{
		    printf("Error: improper environment variable option. '%s %s' is not in the format '-env var=value'\n",
			(*argvp)[1], (*argvp)[2]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		*equal_sign_pos = '\0';
		strncpy(env_node->name, (*argvp)[2], SMPD_MAX_NAME_LENGTH);
		strncpy(env_node->value, equal_sign_pos+1, SMPD_MAX_VALUE_LENGTH);
		env_node->next = env_list;
		env_list = env_node;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "logon") == 0)
	    {
		smpd_process.logon = SMPD_TRUE;
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
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(pwd_file_name, (*argvp)[2], SMPD_MAX_FILENAME);
		smpd_get_pwd_from_file(pwd_file_name);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "file") == 0)
	    {
		printf("Error: The -file option must be the first and only option specified in a block.\n");
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
		/*
		if (argc < 3)
		{
		    printf("Error: no filename specifed after -file option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(configfilename, (*argvp)[2], SMPD_MAX_FILENAME);
		use_configfile = SMPD_TRUE;
		num_args_to_strip = 2;
		*/
	    }
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no host specified after -host option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -host option can only be called once and it cannot be combined with -hosts.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		/* create a host list of one and set nproc to -1 to be replaced by
		   nproc after parsing the block */
		host_list = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
		if (host_list == NULL)
		{
		    printf("failed to allocate memory for a host node.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		host_list->next = NULL;
		host_list->nproc = -1;
		strncpy(host_list->host, (*argvp)[2], SMPD_MAX_HOST_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "hosts") == 0)
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts, -n, -np and -localonly x are mutually exclusive\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (host_list != NULL)
		{
		    printf("Error: -hosts option can only be called once and it cannot be combined with -host.\n");
		    smpd_exit_fn("mp_parse_command_args");
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
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
			num_args_to_strip = 2 + nproc;
			index = 3;
			for (i=0; i<nproc; i++)
			{
			    if (index >= argc)
			    {
				printf("Error: missing host name after -hosts option.\n");
				smpd_exit_fn("mp_parse_command_args");
				return SMPD_FAIL;
			    }
			    host_node_ptr = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
			    if (host_node_ptr == NULL)
			    {
				printf("failed to allocate memory for a host node.\n");
				smpd_exit_fn("mp_parse_command_args");
				return SMPD_FAIL;
			    }
			    host_node_ptr->next = NULL;
			    host_node_ptr->nproc = 1;
			    strncpy(host_node_ptr->host, (*argvp)[index], SMPD_MAX_HOST_LENGTH);
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
			smpd_exit_fn("mp_parse_command_args");
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: not enough arguments specified for -hosts option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
	    }
	    else if (strcmp(&(*argvp)[1][1], "nocolor") == 0)
	    {
		smpd_process.do_multi_color_output = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nompi") == 0)
	    {
		smpd_process.no_mpi = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nomapping") == 0)
	    {
		no_drive_mapping = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "nopopup_debug") == 0)
	    {
#ifdef HAVE_WINDOWS_H
		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
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
		smpd_process.output_exit_codes = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "localroot") == 0)
	    {
		smpd_process.local_root = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "priority") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: you must specify a priority after the -priority option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		if (isnumber((*argvp)[2]))
		{
		    char *str;
		    n_priority_class = atoi((*argvp)[2]);
		    str = strchr((*argvp)[2], ':');
		    if (str)
		    {
			str++;
			n_priority = atoi(str);
		    }
		}
		else
		{
		    printf("Error: you must specify a priority after the -priority option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		smpd_dbg_printf("priorities = %d:%d\n", n_priority_class, n_priority);
		use_priorities = SMPD_TRUE;
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "iproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_TRUE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noiproot") == 0)
	    {
		smpd_process.use_iproot = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "verbose") == 0)
	    {
		smpd_process.verbose = SMPD_TRUE;
		smpd_process.dbg_state |= SMPD_DBG_STATE_ERROUT | SMPD_DBG_STATE_STDOUT | SMPD_DBG_STATE_TRACE;
	    }
	    else if ( (strcmp(&(*argvp)[1][1], "p") == 0) || (strcmp(&(*argvp)[1][1], "port") == 0))
	    {
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
			smpd_process.port = atoi((*argvp)[2]);
		    }
		    else
		    {
			printf("Error: you must specify the port smpd is listening on after the %s option.\n", (*argvp)[1]);
			smpd_exit_fn("mp_parse_command_args");
			return SMPD_FAIL;
		    }
		}
		else
		{
		    printf("Error: you must specify the port smpd is listening on after the %s option.\n", (*argvp)[1]);
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "path") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no path specifed after -path option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(path, (*argvp)[2], SMPD_MAX_PATH_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "noprompt") == 0)
	    {
		smpd_process.noprompt = SMPD_TRUE;
		smpd_process.credentials_prompt = SMPD_FALSE;
	    }
	    else if (strcmp(&(*argvp)[1][1], "phrase") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no passphrase specified afterh -phrase option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(smpd_process.passphrase, (*argvp)[2], SMPD_PASSPHRASE_MAX_LENGTH);
		num_args_to_strip = 2;
	    }
	    else if (strcmp(&(*argvp)[1][1], "smpdfile") == 0)
	    {
		if (argc < 3)
		{
		    printf("Error: no file name specified after -smpdfile option.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strncpy(smpd_process.smpd_filename, (*argvp)[2], SMPD_MAX_FILENAME);
		{
		    struct stat s;

		    if (stat(smpd_process.smpd_filename, &s) == 0)
		    {
			if (s.st_mode & 00077)
			{
			    printf("Error: .smpd file cannot be readable by anyone other than the current user.\n");
			    smpd_exit_fn("mp_parse_command_args");
			    return SMPD_FAIL;
			}
		    }
		}
		num_args_to_strip = 2;
	    }
	    else
	    {
		printf("Unknown option: %s\n", (*argvp)[1]);
	    }
	    strip_args(argcp, argvp, num_args_to_strip);
	}


	/* remaining args are the executable and it's args */
	if (argc < 2)
	{
	    printf("Error: no executable specified\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}

	exe_iter = exe;
	exe_len_remaining = SMPD_MAX_EXE_LENGTH;
	if (!((*argvp)[1][0] == '\\' && (*argvp)[1][1] == '\\') && (*argvp)[1][0] != '/' &&
	    !(strlen((*argvp)[1]) > 3 && (*argvp)[1][1] == ':' && (*argvp)[1][2] == '\\') )
	{
	    /* an absolute path was not specified so find the executable an save the path */
	    if (smpd_get_full_path_name((*argvp)[1], SMPD_MAX_EXE_LENGTH, exe_path, &namepart))
	    {
		if (path[0] != '\0')
		{
		    if (strlen(path) < SMPD_MAX_PATH_LENGTH)
		    {
			strcat(path, ";");
			strncat(path, exe_path, SMPD_MAX_PATH_LENGTH - strlen(path));
			path[SMPD_MAX_PATH_LENGTH-1] = '\0';
		    }
		}
		else
		{
		    strncpy(path, exe_path, SMPD_MAX_PATH_LENGTH);
		}
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, namepart);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
	    }
	    else
	    {
		result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
		if (result != MPIU_STR_SUCCESS)
		{
		    printf("Error: insufficient buffer space for the command line.\n");
		    smpd_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
	    }
	}
	else
	{
	    /* an absolute path was specified */
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[1]);
	}
	for (i=2; i<argc; i++)
	{
	    result = MPIU_Str_add_string(&exe_iter, &exe_len_remaining, (*argvp)[i]);
	}
	/* remove the trailing space */
	exe[strlen(exe)-1] = '\0';
	smpd_dbg_printf("handling executable:\n%s\n", exe);

	if (nproc == 0)
	{
	    smpd_err_printf("missing num_proc flag: -n, -np, -hosts, or -localonly.\n");
	    smpd_exit_fn("mp_parse_command_args");
	    return SMPD_FAIL;
	}
	if (host_list != NULL && host_list->nproc == -1)
	{
	    /* -host specified, replace nproc field */
	    host_list->nproc = nproc;
	}

	env_str = env_data;
	env_data[0] = '\0';
	while (env_list)
	{
	    env_str += snprintf(env_str,
		SMPD_MAX_ENV_LENGTH - (env_str - env_data),
		"%s=%s", env_list->name, env_list->value);
	    if (env_list->next)
	    {
		env_str += snprintf(env_str, SMPD_MAX_ENV_LENGTH - (env_str - env_data), ";");
	    }
	    env_node = env_list;
	    env_list = env_list->next;
	    free(env_node);
	}

	for (i=0; i<nproc; i++)
	{
	    /* create a launch_node */
	    launch_node = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
	    if (launch_node == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure.\n");
		smpd_exit_fn("mp_parse_command_args");
		return SMPD_FAIL;
	    }
	    launch_node->clique[0] = '\0';
	    smpd_get_next_host(&host_list, launch_node);
	    launch_node->iproc = cur_rank++;
	    launch_node->env = launch_node->env_data;
	    strcpy(launch_node->env_data, env_data);
	    if (wdir[0] != '\0')
		strcpy(launch_node->dir, wdir);
	    else
	    {
		launch_node->dir[0] = '\0';
		getcwd(launch_node->dir, SMPD_MAX_DIR_LENGTH);
	    }
	    if (path[0] != '\0')
		strcpy(launch_node->path, path);
	    else
		launch_node->path[0] = '\0';
	    launch_node->map_list = drive_map_list;
	    if (drive_map_list)
	    {
		/* ref count the list so when freeing the launch_node it can be know when to free the list */
		drive_map_list->ref_count++;
	    }
	    strcpy(launch_node->exe, exe);
#ifdef MPIEXEC_INORDER_LAUNCH
	    /* insert the node in order */
	    launch_node->next = NULL;
	    if (smpd_process.launch_list == NULL)
	    {
		smpd_process.launch_list = launch_node;
		launch_node->prev = NULL;
	    }
	    else
	    {
		launch_node_iter = smpd_process.launch_list;
		while (launch_node_iter->next)
		    launch_node_iter = launch_node_iter->next;
		launch_node_iter->next = launch_node;
		launch_node->prev = launch_node_iter;
	    }
#else
	    /* insert the node in reverse order */
	    launch_node->next = smpd_process.launch_list;
	    if (smpd_process.launch_list)
		smpd_process.launch_list->prev = launch_node;
	    smpd_process.launch_list = launch_node;
	    launch_node->prev = NULL;
#endif
	}

	if (s_host_list)
	{
	    /* free the current host list */
	    while (s_host_list)
	    {
		host_node_iter = s_host_list;
		s_host_list = s_host_list->next;
		free(host_node_iter);
	    }
	}

	if (use_configfile)
	{
	    if (smpd_get_argcv_from_file(fin_config, argcp, argvp))
		goto configfile_loop;
	    fclose(fin_config);
	}

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    /* add nproc to all the launch nodes */
    launch_node_iter = smpd_process.launch_list;
    while (launch_node_iter)
    {
	launch_node_iter->nproc = cur_rank;
	launch_node_iter = launch_node_iter->next;
    }

    /* create the cliques */
    smpd_create_cliques(smpd_process.launch_list);

    smpd_exit_fn("mp_parse_command_args");
    return SMPD_SUCCESS;
}
