/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include "mpiexec.h"
#include "smpd.h"

mp_process_t mp_process =
{
    SMPD_FALSE,
    "",
    NULL,
    SMPD_TRUE,
    SMPD_TRUE,
    SMPD_FALSE,
    SMPD_FALSE,
    SMPD_FALSE,
    SMPD_FALSE,
    SMPD_FALSE
};

void mp_print_options(void)
{
    printf("\n");
    printf("Usage:\n");
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
    printf("\n");
    printf("For a list of all mpiexec options, execute 'mpiexec -help2'\n");
    printf("\n");
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

int mp_parse_command_args(int *argcp, char **argvp[])
{
    /*mp_host_node_t *node;*/
    /*char host[MP_MAX_HOST_LENGTH];*/
    /*int found;*/
    int cur_rank;
    /*int n, parent, id;*/
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

    mp_enter_fn("mp_parse_command_args");

    /* check for console option */
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

    cur_rank = 1;
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

	/* parse the current block */
	while ((*argvp)[1] && (*argvp)[1][0] == '-')
	{
	    num_args_to_strip = 1;
	    if ((strcmp(&(*argvp)[1][1], "np") == 0) || (strcmp(&(*argvp)[1][1], "n") == 0))
	    {
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
	    else if (strcmp(&(*argvp)[1][1], "host") == 0)
	    {
	    }
	    else if (strcmp(&(*argvp)[1][1], "hosts") == 0)
	    {
		if (nproc != 0)
		{
		    printf("Error: only one option is allowed to determine the number of processes.\n");
		    printf("       -hosts cannot be used with -n, -np or -localonly\n");
		    return SMPD_FAIL;
		}
		if (argc > 2)
		{
		    if (isnumber((*argvp)[2]))
		    {
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
		return SMPD_FAIL;
	    }
	    else if (strcmp(&(*argvp)[1][1], "help2") == 0)
	    {
		mp_print_extra_options();
		return SMPD_FAIL;
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

	/*
	while (smpd_get_opt_string(argcp, argvp, "-host", host, MP_MAX_HOST_LENGTH))
	{
	    node = g_host_list;
	    found = 0;
	    while (node)
	    {
		if (strcmp(node->host, host) == 0)
		{
		    found = 1;
		    break;
		}
		if (node->next == NULL)
		    break;
		node = node->next;
	    }
	    if (!found)
	    {
		if (node != NULL)
		{
		    node->next = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
		    node = node->next;
		}
		else
		{
		    node = (mp_host_node_t *)malloc(sizeof(mp_host_node_t));
		    g_host_list = node;
		}
		if (node == NULL)
		{
		    mp_err_printf("malloc failed to allocate a host node structure of size %d\n", sizeof(mp_host_node_t));
		    mp_exit_fn("mp_parse_command_args");
		    return SMPD_FAIL;
		}
		strcpy(node->host, host);
		node->ip_str[0] = '\0';
		node->next = NULL;
	    }
	}
	*/

	/* move to the next block */
	*argvp = next_argv - 1;
	if (*next_argv)
	    **argvp = exe_ptr;
    } while (*next_argv);

    mp_exit_fn("mp_parse_command_args");
    return SMPD_SUCCESS;
}

int main(int argc, char* argv[])
{
    int result;
    int port = SMPD_LISTENER_PORT;
    smpd_command_t *cmd_ptr;
    sock_event_t event;

    mp_enter_fn("main");

    /* initialize */
    result = sock_init();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_init failed, sock error:\n%s\n", get_sock_error_string(result));
	mp_exit_fn("main");
	return result;
    }

    result = smpd_init_process();
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("smpd_init_process failed.\n");
	mp_exit_fn("main");
	return result;
    }

    /* parse the command line */
    result = mp_parse_command_args(&argc, &argv);
    if (result != SMPD_SUCCESS)
    {
	mp_err_printf("Unable to parse the command arguments.\n");
	mp_exit_fn("main");
	return result;
    }

    /* handle a console session or a job session */
    if (mp_process.do_console)
    {
	/* do a console session */

	result = mp_console(mp_process.console_host);
    }
    else
    {
	/* do mpi job */

	/* connect to all the hosts in the job */
	result = mp_connect_tree(mp_process.host_list);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to connect all the hosts in the job.\n");
	    goto quit_job;
	}

	/* initialize the pmi database engine on the root node */

	/* create the list of processes to launch */

	/* launch the processes */

	/* wait for them all to exit */

	/* close the tree */
	result = smpd_create_command("close", 0, 1, SMPD_FALSE, &cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to create the close command to tear down the job tree.\n");
	    goto quit_job;
	}
	result = smpd_post_write_command(smpd_process.left_context, cmd_ptr);
	if (result != SMPD_SUCCESS)
	{
	    mp_err_printf("unable to post a write of the close command to tear down the job tree.\n");
	    goto quit_job;
	}
	result = SMPD_SUCCESS;
	while (result == SMPD_SUCCESS)
	{
	    mp_dbg_printf("sock_waiting for next event.\n");
	    result = sock_wait(smpd_process.set, SOCK_INFINITE_TIME, &event);
	    if (result != SOCK_SUCCESS)
	    {
		mp_err_printf("sock_wait failed, error:\n%s\n", get_sock_error_string(result));
		smpd_close_connection(smpd_process.set, smpd_process.left_context->sock);
		goto quit_job;
	    }

	    switch (event.op_type)
	    {
	    case SOCK_OP_READ:
		result = handle_read(event.user_ptr, event.num_bytes, event.error, NULL);
		if (result == SMPD_CLOSE)
		{
		    mp_dbg_printf("handle_read returned SMPD_CLOSE\n");
		    break;
		}
		if (result != SMPD_SUCCESS)
		{
		    mp_err_printf("handle_read() failed.\n");
		}
		break;
	    case SOCK_OP_WRITE:
		result = handle_written(event.user_ptr, event.num_bytes, event.error);
		if (result != SMPD_SUCCESS)
		{
		    mp_err_printf("handle_written() failed.\n");
		}
		break;
	    case SOCK_OP_ACCEPT:
		mp_err_printf("unexpected accept event returned by sock_wait.\n");
		break;
	    case SOCK_OP_CONNECT:
		mp_err_printf("unexpected connect event returned by sock_wait.\n");
		break;
	    case SOCK_OP_CLOSE:
		mp_err_printf("unexpected close event returned by sock_wait.\n");
		free(smpd_process.left_context);
		mp_dbg_printf("closing the session.\n");
		result = sock_destroy_set(smpd_process.set);
		if (result != SOCK_SUCCESS)
		{
		    mp_err_printf("error destroying set: %s\n", get_sock_error_string(result));
		}
		mp_exit_fn("mp_connect_tree");
		return SMPD_FAIL;
		break;
	    default:
		mp_err_printf("unknown event returned by sock_wait: %d\n", event.op_type);
		break;
	    }
	}
    }

quit_job: /* use a goto label to avoid deep indenting in the above code */

    /* finalize */
    mp_dbg_printf("calling sock_finalize\n");
    result = sock_finalize();
    if (result != SOCK_SUCCESS)
    {
	mp_err_printf("sock_finalize failed, sock error:\n%s\n", get_sock_error_string(result));
    }

#ifdef HAVE_WINDOWS_H
    if (g_hCloseStdinThreadEvent)
	SetEvent(g_hCloseStdinThreadEvent);
    if (g_hStdinThread != NULL)
    {
	WaitForSingleObject(g_hStdinThread, 3000);
	CloseHandle(g_hStdinThread);
    }
    if (g_hCloseStdinThreadEvent)
	CloseHandle(g_hCloseStdinThreadEvent);
#endif
    mp_exit_fn("main");
    smpd_exit(0);
}

