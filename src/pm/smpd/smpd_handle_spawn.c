/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

#undef FCNAME
#define FCNAME "smpd_handle_spawn_command"
int smpd_handle_spawn_command(smpd_context_t *context)
{
    int result;
    smpd_command_t *cmd, *temp_cmd;
    char ctx_key[100];
    int ncmds, *maxprocs, *nkeyvals, i, j;
    smpd_launch_node_t node;
    char key[100], val[1024];
    char *iter1, *iter2;
    char maxprocs_str[1024], nkeyvals_str[1024], keyvals_str[1024];
    smpd_launch_node_t *launch_list, *launch_iter/*, *launch_temp*/;
    PMI_keyval_t *info;
    char key_temp[SMPD_MAX_NAME_LENGTH], val_temp[SMPD_MAX_VALUE_LENGTH];
    int cur_iproc;
    smpd_host_node_t *host_iter, *host_list;
    int nproc;

    smpd_enter_fn(FCNAME);

    cmd = &context->read_cmd;
    smpd_process.exit_on_done = SMPD_TRUE;
    /* populate the host list */
    smpd_get_default_hosts();

    
    /* prepare the result command */


    result = smpd_create_command("result", smpd_process.id, cmd->src, SMPD_FALSE, &temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* add the command tag for result matching */
    result = smpd_add_command_int_arg(temp_cmd, "cmd_tag", cmd->tag);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the tag to the result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "cmd_orig", cmd->cmd_str);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add cmd_orig to the result command for a %s command\n", cmd->cmd_str);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    /* copy the ctx_key for pmi control channel lookup */
    if (MPIU_Str_get_string_arg(cmd->cmd, "ctx_key", ctx_key, 100) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("no ctx_key in the spawn command: '%s'\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    result = smpd_add_command_arg(temp_cmd, "ctx_key", ctx_key);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the ctx_key to the result command for spawn command '%s'.\n", cmd->cmd);
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* parse the spawn command */


    if (MPIU_Str_get_int_arg(cmd->cmd, "ncmds", &ncmds) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the ncmds parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
    }
    /*printf("ncmds = %d\n", ncmds);fflush(stdout);*/
    if (MPIU_Str_get_string_arg(cmd->cmd, "maxprocs", maxprocs_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the maxrpocs parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
    }
    if (MPIU_Str_get_string_arg(cmd->cmd, "nkeyvals", nkeyvals_str, 1024) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the nkeyvals parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
    }
    maxprocs = (int*)malloc(ncmds * sizeof(int));
    if (maxprocs == NULL)
    {
	smpd_err_printf("unable to allocate the maxprocs array.\n");
	goto spawn_failed;
    }
    nkeyvals = (int*)malloc(ncmds * sizeof(int));
    if (nkeyvals == NULL)
    {
	smpd_err_printf("unable to allocate the nkeyvals array.\n");
	goto spawn_failed;
    }
    iter1 = maxprocs_str;
    iter2 = nkeyvals_str;
    for (i=0; i<ncmds; i++)
    {
	result = MPIU_Str_get_string(&iter1, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the maxprocs parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    goto spawn_failed;
	    /*
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	    */
	}
	maxprocs[i] = atoi(key);
	/*printf("maxprocs[%d] = %d\n", i, maxprocs[i]);fflush(stdout);*/
	result = MPIU_Str_get_string(&iter2, key, 100);
	if (result != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %dth string from the nkeyvals parameter to the spawn command '%s'.\n", i, cmd->cmd);
	    goto spawn_failed;
	    /*
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	    */
	}
	nkeyvals[i] = atoi(key);
	/*printf("nkeyvals[%d] = %d\n", i, nkeyvals[i]);fflush(stdout);*/
    }
    info = NULL;
    launch_list = NULL;
    launch_iter = NULL;
    cur_iproc = 0;
    for (i=0; i<ncmds; i++)
    {
	/* reset the node fields */
	node.appnum = -1;
	node.args[0] = '\0';
	node.clique[0] = '\0';
	node.dir[0] = '\0';
	node.env = node.env_data;
	node.env_data[0] = '\0';
	node.exe[0] = '\0';
	node.host_id = -1;
	node.hostname[0] = '\0';
	node.iproc = -1;
	node.map_list = NULL;
	node.next = NULL;
	node.nproc = -1;
	node.path[0] = '\0';
	node.prev = NULL;
	node.priority_class = -1;
	node.priority_thread = -1;

	if (info != NULL)
	{
	    /* free the last round of infos */
	    for (j=0; j<nkeyvals[i-1]; j++)
	    {
		free(info[j].key);
		free(info[j].val);
	    }
	    free(info);
	}
	/* allocate some new infos */
	if (nkeyvals[i] > 0)
	{
	    info = (PMI_keyval_t*)malloc(nkeyvals[i] * sizeof(PMI_keyval_t));
	    if (info == NULL)
	    {
		smpd_err_printf("unable to allocate memory for the info keyvals (cmd %d, num_infos %d).\n", i, nkeyvals[i]);
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
		*/
	    }
	}
	else
	{
	    info = NULL;
	}
	/* parse the keyvals into infos */
	sprintf(key, "keyvals%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, keyvals_str, 1024) == MPIU_STR_SUCCESS)
	{
	    /*printf("%s = '%s'\n", key, keyvals_str);fflush(stdout);*/
	    for (j=0; j<nkeyvals[i]; j++)
	    {
		sprintf(key, "%d", j);
		if (MPIU_Str_get_string_arg(keyvals_str, key, val, 1024) != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to get the %sth key from the keyval string '%s'.\n", key, keyvals_str);
		    goto spawn_failed;
		    /*
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		    */
		}
		/*printf("key %d = %s\n", j, val);fflush(stdout);*/
		key_temp[0] = '\0';
		val_temp[0] = '\0';
		iter1 = val;
		result = MPIU_Str_get_string(&iter1, key_temp, SMPD_MAX_NAME_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		    /*
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		    */
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH); /* eat the '=' character */
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		    /*
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		    */
		}
		result = MPIU_Str_get_string(&iter1, val_temp, SMPD_MAX_VALUE_LENGTH);
		if (result != MPIU_STR_SUCCESS)
		{
		    smpd_err_printf("unable to parse the key from the %dth keyval pair in the %dth keyvals string.\n", j, i);
		    goto spawn_failed;
		    /*
		    smpd_exit_fn(FCNAME);
		    return SMPD_FAIL;
		    */
		}
		info[j].key = strdup(key_temp);
		info[j].val = strdup(val_temp);
	    }
	}
	/* get the current command */
	sprintf(key, "cmd%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.exe, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    goto spawn_failed;
	    /*
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	    */
	}
#ifdef HAVE_WINDOWS_H
	if (strlen(node.exe) > 2)
	{
	    /* the Windows version handles the common unix case where executables are specified like this: ./foo */
	    /* Instead of failing, simply fix the / character */
	    if (node.exe[0] == '.' && node.exe[1] == '/')
		node.exe[1] = '\\';
	}
#endif
	/*printf("%s = %s\n", key, node.exe);fflush(stdout);*/
	sprintf(key, "argv%d", i);
	if (MPIU_Str_get_string_arg(cmd->cmd, key, node.args, SMPD_MAX_EXE_LENGTH) != MPIU_STR_SUCCESS)
	{
	    node.args[0] = '\0';
	    /*
	    smpd_err_printf("unable to get the %s parameter from the spawn command '%s'.\n", key, cmd->cmd);
	    goto spawn_failed;
	    smpd_exit_fn(FCNAME);
	    return SMPD_FAIL;
	    */
	}
	/*printf("%s = %s\n", key, node.args);fflush(stdout);*/

	/* interpret the infos for this command */
	for (j=0; j<nkeyvals[i]; j++)
	{
	    /* path */
	    if (strcmp(info[j].key, "path") == 0)
	    {
		strcpy(node.path, info[j].val);
		smpd_dbg_printf("path = %s\n", info[j].val);
	    }
	    /* host */
	    if (strcmp(info[j].key, "host") == 0)
	    {
		smpd_dbg_printf("host key sent with spawn command: <%s>\n", info[j].val);
		if (smpd_get_host_id(info[j].val, &node.host_id) == SMPD_SUCCESS)
		{
		    strcpy(node.hostname, info[j].val);
		}
		else
		{
		    /* smpd_get_host_id should not modify host_id if there is a failure but just to be safe ... */
		    node.host_id = -1;
		}
	    }
	    /* env */
	    /* wdir */
	    /* map */
	    /* etc */
	}

	/* create launch nodes for the current command */
	for (j=0; j<maxprocs[i]; j++)
	{
	    if (launch_list == NULL)
	    {
		launch_list = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
		launch_iter = launch_list;
		if (launch_iter)
		{
		    launch_iter->prev = NULL;
		}
	    }
	    else
	    {
		launch_iter->next = (smpd_launch_node_t*)malloc(sizeof(smpd_launch_node_t));
		if (launch_iter->next)
		{
		    launch_iter->next->prev = launch_iter;
		    launch_iter = launch_iter->next;
		}
		else
		{
		    launch_iter = NULL;
		}
	    }
	    if (launch_iter == NULL)
	    {
		smpd_err_printf("unable to allocate a launch node structure for the %dth command.\n", cur_iproc);
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return SMPD_FAIL;
		*/
	    }
	    launch_iter->appnum = i;
	    launch_iter->iproc = cur_iproc++;
	    launch_iter->args[0] = '\0';
	    launch_iter->clique[0] = '\0';
	    launch_iter->dir[0] = '\0';
	    launch_iter->env_data[0] = '\0';
	    launch_iter->env = launch_iter->env_data;
	    launch_iter->exe[0] = '\0';
	    if (node.host_id != -1)
	    {
		launch_iter->host_id = node.host_id;
		strcpy(launch_iter->hostname, node.hostname);
	    }
	    else
	    {
		launch_iter->host_id = -1;
		launch_iter->hostname[0] = '\0';
	    }
	    launch_iter->map_list = NULL;
	    launch_iter->path[0] = '\0';
	    launch_iter->next = NULL;

	    strcpy(launch_iter->exe, node.exe);
	    /*strcpy(launch_iter->args, node.args);*/
	    if (strlen(node.args) > 0)
	    {
		strncat(launch_iter->exe, " ", SMPD_MAX_EXE_LENGTH);
		strncat(launch_iter->exe, node.args, SMPD_MAX_EXE_LENGTH);
	    }
	    strcpy(launch_iter->path, node.path);
	}
    }
    if (info != NULL)
    {
	/* free the last round of infos */
	for (j=0; j<nkeyvals[i-1]; j++)
	{
	    free(info[j].key);
	    free(info[j].val);
	}
	free(info);
    }
    info = NULL;

    /* create a spawn context to save parameters, state, etc. */
    context->spawn_context = (smpd_spawn_context_t*)malloc(sizeof(smpd_spawn_context_t));
    if (context->spawn_context == NULL)
    {
	smpd_err_printf("unable to create a spawn context.\n");
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return result;
	*/
    }
    context->spawn_context->context = context;
    context->spawn_context->kvs_name[0] = '\0';
    context->spawn_context->launch_list = NULL;
    context->spawn_context->npreput = -1;
    context->spawn_context->num_outstanding_launch_cmds = -1;
    context->spawn_context->preput[0] = '\0';
    context->spawn_context->result_cmd = NULL;

    /* Get the keyval pairs to be put in the process group keyval space before the processes are launched. */
    if (MPIU_Str_get_int_arg(cmd->cmd, "npreput", &context->spawn_context->npreput) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unable to get the npreput parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
    }
    /*printf("npreput = %d\n", context->spawn_context->npreput);fflush(stdout);*/
    if (context->spawn_context->npreput > 0 && MPIU_Str_get_string_arg(cmd->cmd, "preput", context->spawn_context->preput, SMPD_MAX_CMD_LENGTH) != MPIU_STR_SUCCESS)
    {
	smpd_err_printf("unablet to get the preput parameter from the spawn command '%s'.\n", cmd->cmd);
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
	*/
    }
    free(maxprocs);
    free(nkeyvals);


    /* do the spawn stuff */

    /* count the number of processes to spawn */
    nproc = 0;
    launch_iter = launch_list;
    while (launch_iter)
    {
	nproc++;
	launch_iter = launch_iter->next;
    }

    /* create the host list and add nproc to the launch list */
    host_list = NULL;
    launch_iter = launch_list;
    while (launch_iter)
    {
	if (launch_iter->host_id == -1)
	{
	    smpd_get_next_host(&host_list, launch_iter);
	}
	launch_iter->nproc = nproc;
	launch_iter = launch_iter->next;
    }
    smpd_create_cliques(launch_list);

    /* connect up the new smpd hosts */
    context = smpd_process.left_context;
    /* save the launch list to be used after the new hosts are connected */
    context->spawn_context->launch_list = launch_list;
    context->spawn_context->num_outstanding_launch_cmds = 0;/*nproc;*/ /* this assumes all launch commands will be successfully posted. */
    host_iter = smpd_process.host_list;
    while (host_iter)
    {
	if (!host_iter->connected)
	{
	    context->connect_to = host_iter;

	    /* create a connect command to be sent to the parent */
	    result = smpd_create_command("connect", 0, context->connect_to->parent, SMPD_TRUE, &cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to create a connect command.\n");
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return result;
		*/
	    }
	    result = smpd_add_command_arg(cmd, "host", context->connect_to->host);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the host parameter to the connect command for host %s\n", context->connect_to->host);
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return result;
		*/
	    }
	    result = smpd_add_command_int_arg(cmd, "id", context->connect_to->id);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to add the id parameter to the connect command for host %s\n", context->connect_to->host);
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return result;
		*/
	    }
	    if (smpd_process.plaintext)
	    {
		/* propagate the plaintext option to the manager doing the connect */
		result = smpd_add_command_arg(cmd, "plaintext", "yes");
		if (result != SMPD_SUCCESS)
		{
		    smpd_err_printf("unable to add the plaintext parameter to the connect command for host %s\n", context->connect_to->host);
		    goto spawn_failed;
		}
	    }

	    smpd_dbg_printf("sending first connect command to add new hosts for the spawn command.\n");
	    /*printf("sending first connect command to add new hosts for the spawn command.\n");fflush(stdout);*/
	    /* post a write of the command */
	    result = smpd_post_write_command(context, cmd);
	    if (result != SMPD_SUCCESS)
	    {
		smpd_err_printf("unable to post a write of the connect command.\n");
		goto spawn_failed;
		/*
		smpd_exit_fn(FCNAME);
		return result;
		*/
	    }

	    context->spawn_context->result_cmd = temp_cmd;
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	host_iter = host_iter->next;
    }

    /* create the new kvs space */
    smpd_dbg_printf("all hosts needed for the spawn command are available, sending start_dbs command.\n");
    /*printf("all hosts needed for the spawn command are available, sending start_dbs command.\n");fflush(stdout);*/
    /* create the start_dbs command to be sent to the first host */
    result = smpd_create_command("start_dbs", 0, 1, SMPD_TRUE, &cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to create a start_dbs command.\n");
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return result;
	*/
    }

    result = smpd_add_command_int_arg(cmd, "npreput", context->spawn_context->npreput);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the npreput value to the start_dbs command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    result = smpd_add_command_arg(cmd, "preput", context->spawn_context->preput);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the preput keyvals to the start_dbs command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* post a write of the command */
    result = smpd_post_write_command(context, cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the start_dbs command.\n");
	goto spawn_failed;
	/*
	smpd_exit_fn(FCNAME);
	return result;
	*/
    }

    context->spawn_context->result_cmd = temp_cmd;
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
    /* send the launch commands */

/*
    printf("host tree:\n");
    host_iter = smpd_process.host_list;
    if (!host_iter)
	printf("<none>\n");
    while (host_iter)
    {
	printf(" host: %s, parent: %d, id: %d, connected: %s\n",
	    host_iter->host,
	    host_iter->parent, host_iter->id,
	    host_iter->connected ? "yes" : "no");
	host_iter = host_iter->next;
    }

    printf("launch nodes:\n");
    launch_iter = launch_list;
    if (!launch_iter)
	printf("<none>\n");
    while (launch_iter)
    {
	printf(" launch_node:\n");
	printf("  id  : %d\n", launch_iter->host_id);
	printf("  rank: %d\n", launch_iter->iproc);
	printf("  size: %d\n", launch_iter->nproc);
	printf("  clique: %s\n", launch_iter->clique);
	printf("  exe : %s\n", launch_iter->exe);
	if (launch_iter->args[0] != '\0')
	    printf("  args: %s\n", launch_iter->args);
	if (launch_iter->path[0] != '\0')
	    printf("  path: %s\n", launch_iter->path);
	launch_temp = launch_iter;
	launch_iter = launch_iter->next;
	free(launch_temp);
    }
    fflush(stdout);
*/

spawn_failed:
    /* add the result */
    result = smpd_add_command_arg(temp_cmd, "result", SMPD_FAIL_STR);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to add the result string to the result command for a spawn command.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    /* send result back */
    smpd_dbg_printf("replying to spawn command: \"%s\"\n", temp_cmd->cmd);
    result = smpd_post_write_command(context, temp_cmd);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to post a write of the result command to the context.\n");
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }

    smpd_exit_fn(FCNAME);
    return result;
}
