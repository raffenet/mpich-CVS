/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
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

int smpd_get_hostname(char *host, int length)
{
#ifdef HAVE_WINDOWS_H
    DWORD len = length;
    if (!GetComputerName(host, &len))
	return SMPD_FAIL;
#else
    if (gethostname(host, length))
	return SMPD_FAIL;
#endif
    return SMPD_SUCCESS;
}

int smpd_get_pwd_from_file(char *file_name)
{
    char line[1024];
    FILE *fin;

    /* open the file */
    fin = fopen(file_name, "r");
    if (fin == NULL)
    {
	printf("Error, unable to open account file '%s'\n", file_name);
	return SMPD_FAIL;
    }

    /* read the account */
    if (!fgets(line, 1024, fin))
    {
	printf("Error, unable to read the account in '%s'\n", file_name);
	fclose(fin);
	return SMPD_FAIL;
    }

    /* strip off the newline characters */
    while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	line[strlen(line)-1] = '\0';
    if (strlen(line) == 0)
    {
	printf("Error, first line in password file must be the account name. (%s)\n", file_name);
	fclose(fin);
	return SMPD_FAIL;
    }

    /* save the account */
    strcpy(smpd_process.UserAccount, line);

    /* read the password */
    if (!fgets(line, 1024, fin))
    {
	printf("Error, unable to read the password in '%s'\n", file_name);
	fclose(fin);
	return SMPD_FAIL;
    }
    /* strip off the newline characters */
    while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	line[strlen(line)-1] = '\0';

    /* save the password */
    if (strlen(line))
	strcpy(smpd_process.UserPassword, line);
    else
	smpd_process.UserPassword[0] = '\0';

    fclose(fin);

    return SMPD_SUCCESS;
}

int smpd_get_next_hostname(char *host)
{
    if (smpd_process.s_host_list == NULL)
    {
	if (smpd_process.cur_default_host)
	{
	    if (smpd_process.cur_default_iproc >= smpd_process.cur_default_host->nproc)
	    {
		smpd_process.cur_default_host = smpd_process.cur_default_host->next;
		smpd_process.cur_default_iproc = 0;
		if (smpd_process.cur_default_host == NULL) /* This should never happen because the hosts are in a ring */
		    return SMPD_FAIL;
	    }
	    strcpy(host, smpd_process.cur_default_host->host);
	    smpd_process.cur_default_iproc++;
	}
	else
	{
	    if (smpd_get_hostname(host, SMPD_MAX_HOST_LENGTH) != 0)
		return SMPD_FAIL;
	}
	return SMPD_SUCCESS;
    }
    if (smpd_process.s_cur_host == NULL)
    {
	smpd_process.s_cur_host = smpd_process.s_host_list;
	smpd_process.s_cur_count = 0;
    }
    strcpy(host, smpd_process.s_cur_host->host);
    smpd_process.s_cur_count++;
    if (smpd_process.s_cur_count >= smpd_process.s_cur_host->nproc)
    {
	smpd_process.s_cur_host = smpd_process.s_cur_host->next;
	smpd_process.s_cur_count = 0;
    }
    return SMPD_SUCCESS;
}

SMPD_BOOL smpd_parse_machine_file(char *file_name)
{
    char line[1024];
    FILE *fin;
    smpd_host_node_t *node, *node_iter;
    char *hostname, *iter;
    int nproc;

    smpd_process.s_host_list = NULL;
    smpd_process.s_cur_host = NULL;
    smpd_process.s_cur_count = 0;

    /* open the file */
    fin = fopen(file_name, "r");
    if (fin == NULL)
    {
	printf("Error, unable to open machine file '%s'\n", file_name);
	return SMPD_FALSE;
    }

    while (fgets(line, 1024, fin))
    {
	/* strip off the newline characters */
	while (strlen(line) && (line[strlen(line)-1] == '\r' || line[strlen(line)-1] == '\n'))
	    line[strlen(line)-1] = '\0';
	hostname = line;
	/* move over any leading whitespace */
	while (isspace(*hostname))
	    hostname++;
	if (strlen(hostname) != 0 && hostname[0] != '#')
	{
	    iter = hostname;
	    /* move over the hostname and see if there is a number after it */
	    while ((*iter != '\0') && !isspace(*iter) && (*iter != ':'))
		iter++;
	    if (*iter != '\0')
	    {
		*iter = '\0';
		iter++;
		while (isspace(*iter))
		    iter++;
		nproc = atoi(iter);
		if (nproc < 1)
		    nproc = 1;
	    }
	    else
	    {
		nproc = 1;
	    }
	    node = (smpd_host_node_t*)malloc(sizeof(smpd_host_node_t));
	    strcpy(node->host, hostname);
	    node->connected = SMPD_FALSE;
	    node->id = -1;
	    node->parent = -1;
	    node->nproc = nproc;
	    node->next = NULL;
	    if (smpd_process.s_host_list == NULL)
		smpd_process.s_host_list = node;
	    else
	    {
		node_iter = smpd_process.s_host_list;
		while (node_iter->next != NULL)
		    node_iter = node_iter->next;
		node_iter->next = node;
	    }
	}
    }
    if (smpd_process.s_host_list != NULL)
    {
	node = smpd_process.s_host_list;
	while (node)
	{
	    smpd_dbg_printf("host = %s, nproc = %d\n", node->host, node->nproc);
	    node = node->next;
	}
	return SMPD_TRUE;
    }
    return SMPD_FALSE;
}

int smpd_get_host_id(char *host, int *id_ptr)
{
    smpd_host_node_t *node;
    int bit, mask, temp;

    /* look for the host in the list */
    node = smpd_process.host_list;
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
	node->next = (smpd_host_node_t *)malloc(sizeof(smpd_host_node_t));
	node = node->next;
    }
    else
    {
	node = (smpd_host_node_t *)malloc(sizeof(smpd_host_node_t));
	smpd_process.host_list = node;
    }
    if (node == NULL)
    {
	smpd_err_printf("malloc failed to allocate a host node structure\n");
	return SMPD_FAIL;
    }
    strcpy(node->host, host);
    node->parent = smpd_process.tree_parent;
    node->id = smpd_process.tree_id;
    node->connected = SMPD_FALSE;
    node->nproc = -1;
    node->next = NULL;

    /* move to the next id and parent */
    smpd_process.tree_id++;

    temp = smpd_process.tree_id >> 2;
    bit = 1;
    while (temp)
    {
	bit <<= 1;
	temp >>= 1;
    }
    mask = bit - 1;
    smpd_process.tree_parent = bit | (smpd_process.tree_id & mask);

    /* return the id */
    *id_ptr = node->id;

    return SMPD_SUCCESS;
}

int smpd_get_next_host(smpd_host_node_t **host_node_pptr, smpd_launch_node_t *launch_node)
{
    int result;
    char host[SMPD_MAX_HOST_LENGTH];
    smpd_host_node_t *host_node_ptr;

    if (host_node_pptr == NULL)
    {
	smpd_err_printf("invalid host_node_pptr argument.\n");
	return SMPD_FAIL;
    }

    if (*host_node_pptr == NULL)
    {
	result = smpd_get_next_hostname(host);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get the next available host name\n");
	    return SMPD_FAIL;
	}
	result = smpd_get_host_id(host, &launch_node->host_id);
	if (result != SMPD_SUCCESS)
	{
	    smpd_err_printf("unable to get a id for host %s\n", host);
	    return SMPD_FAIL;
	}
	MPIU_Strncpy(launch_node->hostname, host, SMPD_MAX_HOST_LENGTH);
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
	    smpd_err_printf("no more hosts in the list.\n");
	    return SMPD_FAIL;
	}
    }
    result = smpd_get_host_id(host_node_ptr->host, &launch_node->host_id);
    if (result != SMPD_SUCCESS)
    {
	smpd_err_printf("unable to get a id for host %s\n", host_node_ptr->host);
	return SMPD_FAIL;
    }
    MPIU_Strncpy(launch_node->hostname, host_node_ptr->host, SMPD_MAX_HOST_LENGTH);
    host_node_ptr->nproc--;
    if (host_node_ptr->nproc == 0)
    {
	(*host_node_pptr) = (*host_node_pptr)->next;
	free(host_node_ptr);
    }

    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_get_argcv_from_file"
SMPD_BOOL smpd_get_argcv_from_file(FILE *fin, int *argcp, char ***argvp)
{
    /* maximum of 8192 characters per line and 1023 args */
    static char line[8192];
    static char *argv[1024];
    char *token;
    int index;

    smpd_enter_fn(FCNAME);

    argv[0] = "bogus.exe";
    while (fgets(line, 8192, fin))
    {
	index = 1;
	token = strtok(line, " \r\n");
	while (token)
	{
	    argv[index] = token;
	    index++;
	    if (index == 1024)
	    {
		argv[1023] = NULL;
		break;
	    }
	    token = strtok(NULL, " \r\n");
	}
	if (index != 1)
	{
	    if (index < 1024)
		argv[index] = NULL;
	    *argcp = index;
	    *argvp = argv;
	    return SMPD_TRUE;
	}
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FALSE;
}

static smpd_launch_node_t *next_launch_node(smpd_launch_node_t *node, int id)
{
    while (node)
    {
	if (node->host_id == id)
	    return node;
	node = node->next;
    }
    return NULL;
}

static smpd_launch_node_t *prev_launch_node(smpd_launch_node_t *node, int id)
{
    while (node)
    {
	if (node->host_id == id)
	    return node;
	node = node->prev;
    }
    return NULL;
}

#undef FCNAME
#define FCNAME "smpd_create_cliques"
int smpd_create_cliques(smpd_launch_node_t *list)
{
    smpd_launch_node_t *iter, *cur_node;
    int cur_iproc, printed_iproc;
    char *cur_str;

    smpd_enter_fn(FCNAME);

    if (list == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_SUCCESS;
    }

    if (list->iproc == 0)
    {
	/* in order */
	cur_node = list;
	while (cur_node)
	{
	    /* point to the current structures */
	    printed_iproc = cur_iproc = cur_node->iproc;
	    cur_str = cur_node->clique;
	    cur_str += sprintf(cur_str, "%d", cur_iproc);
	    /* add the ranks of all other nodes with the same id */
	    iter = next_launch_node(cur_node->next, cur_node->host_id);
	    while (iter)
	    {
		if (iter->iproc == cur_iproc + 1)
		{
		    cur_iproc = iter->iproc;
		    iter = next_launch_node(iter->next, iter->host_id);
		    if (iter == NULL)
			cur_str += sprintf(cur_str, "..%d", cur_iproc);
		}
		else
		{
		    if (printed_iproc == cur_iproc)
		    {
			cur_str += sprintf(cur_str, ",%d", iter->iproc);
		    }
		    else
		    {
			cur_str += sprintf(cur_str, "..%d,%d", cur_iproc, iter->iproc);
		    }
		    printed_iproc = cur_iproc = iter->iproc;
		    iter = next_launch_node(iter->next, iter->host_id);
		}
	    }
	    /* copy the clique string to all the nodes with the same id */
	    iter = next_launch_node(cur_node->next, cur_node->host_id);
	    while (iter)
	    {
		strcpy(iter->clique, cur_node->clique);
		iter = next_launch_node(iter->next, iter->host_id);
	    }
	    /* move to the next node that doesn't have a clique string yet */
	    cur_node = cur_node->next;
	    while (cur_node && cur_node->clique[0] != '\0')
		cur_node = cur_node->next;
	}
    }
    else
    {
	/* reverse order */
	cur_node = list;
	/* go to the end of the list */
	while (cur_node->next)
	    cur_node = cur_node->next;
	while (cur_node)
	{
	    /* point to the current structures */
	    printed_iproc = cur_iproc = cur_node->iproc;
	    cur_str = cur_node->clique;
	    cur_str += sprintf(cur_str, "%d", cur_iproc);
	    /* add the ranks of all other nodes with the same id */
	    iter = prev_launch_node(cur_node->prev, cur_node->host_id);
	    while (iter)
	    {
		if (iter->iproc == cur_iproc + 1)
		{
		    cur_iproc = iter->iproc;
		    iter = prev_launch_node(iter->prev, iter->host_id);
		    if (iter == NULL)
			cur_str += sprintf(cur_str, "..%d", cur_iproc);
		}
		else
		{
		    if (printed_iproc == cur_iproc)
		    {
			cur_str += sprintf(cur_str, ",%d", iter->iproc);
		    }
		    else
		    {
			cur_str += sprintf(cur_str, "..%d,%d", cur_iproc, iter->iproc);
		    }
		    printed_iproc = cur_iproc = iter->iproc;
		    iter = prev_launch_node(iter->prev, iter->host_id);
		}
	    }
	    /* copy the clique string to all the nodes with the same id */
	    iter = prev_launch_node(cur_node->prev, cur_node->host_id);
	    while (iter)
	    {
		strcpy(iter->clique, cur_node->clique);
		iter = prev_launch_node(iter->prev, iter->host_id);
	    }
	    /* move to the next node that doesn't have a clique string yet */
	    cur_node = cur_node->prev;
	    while (cur_node && cur_node->clique[0] != '\0')
		cur_node = cur_node->prev;
	}
    }
    /*
    iter = list;
    while (iter)
    {
	printf("clique: <%s>\n", iter->clique);
	iter = iter->next;
    }
    */
    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

