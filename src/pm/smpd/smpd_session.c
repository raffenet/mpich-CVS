/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* getenv */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* strrchr */
#endif

#ifndef HAVE_WINDOWS_H
static int exists(char *filename)
{
    struct stat file_stat;

    if ((stat(filename, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode)))
    {
	return 0; /* no such file, or not a regular file */
    }
    return 1;
}

static SMPD_BOOL search_path(const char *smpd_path, const char *exe, int maxlen, char *str)
{
    char test[SMPD_MAX_EXE_LENGTH];
    char path[SMPD_MAX_PATH_LENGTH];
    char *token;
    int n;

    if (smpd_path == NULL || exe == NULL || maxlen < 1 || str == NULL)
	return SMPD_FALSE;

    strncpy(path, smpd_path, SMPD_MAX_PATH_LENGTH);
    path[SMPD_MAX_PATH_LENGTH - 1] = '\0';
    token = strtok(path, ";:");
    while (token)
    {
	/* this does not catch the case where SMPD_MAX_EXE_LENGTH is not long enough and the file exists */
	if (token[strlen(token)-1] != '/')
	    n = snprintf(test, SMPD_MAX_EXE_LENGTH, "%s/%s", token, exe);
	else
	    n = snprintf(test, SMPD_MAX_EXE_LENGTH, "%s%s", token, exe);
	test[SMPD_MAX_EXE_LENGTH-1] = '\0';
	if (exists(test))
	{
	    if (n < maxlen)
	    {
		strcpy(str, test);
		return SMPD_TRUE;
	    }
	    smpd_err_printf("buffer provided is too small: %d provided, %d needed\n", maxlen, n);
	    return SMPD_FALSE;
	}
	token = strtok(NULL, ";:");
    }
    return SMPD_FALSE;
}
#endif

#if 0
char *search_path(char **env, char *cmd, char *cwd, int uid, int gid, char *uname)
{
    int i, len;
    char *tmp, *path, succeeded = 0;
    static char filename[4096];

    for (i=0; env[i]; i++)
    {
	if (strncmp(env[i], "PATH=", 5) != 0)
	    continue;

	len = strlen(env[i])+1;
	if (!(path=(char *)malloc(len * sizeof(char))))
	{
	    return(NULL);
	}
	bcopy(env[i], path, len * sizeof(char));

	/* check for absolute or relative pathnames */
	if ((strncmp(cmd,"./",2)) && (strncmp(cmd,"../",3)) && (cmd[0]!='/'))
	{
	    /* ok, no pathname specified, search for a valid executable */
	    tmp = NULL;
	    for (strtok(path, "="); (tmp = strtok(NULL, ":")); )
	    {
		/* concatenate search path and command */
		/* use cwd if relative path is being used in environment */
		if (!strncmp(tmp,"../",3) || !strncmp(tmp,"./",2) ||
		    !strcmp(tmp,".") || !strcmp(tmp,".."))
		{
		    snprintf(filename, 4096, "%s/%s/%s", cwd, tmp, cmd);
		}
		else
		{
		    snprintf(filename, 4096, "%s/%s", tmp, cmd);
		}
		/* see if file is executable */
		if (is_executable(filename, uname, uid, gid))
		{
		    succeeded=1;
		    break;
		}
	    }
	}
	else
	{
	    /* ok, pathname is specified */
	    if (!(strncmp(cmd,"../",3)) || !(strncmp(cmd,"./",2)))
		snprintf(filename, 4096, "%s/%s", cwd, cmd);
	    else
		strncpy(filename,cmd,4096);
	    if (is_executable(filename, uname, uid, gid))
		succeeded=1;
	}
	return((succeeded) ? filename : NULL);
    }
    return(NULL);
}

/* IS_EXECUTABLE() - checks to see if a given filename refers to a file
 * which a given user could execute
 *
 * Parameters:
 * fn  - pointer to string containing null terminated file name,
 *       including path
 * un  - pointer to string containing user name
 * uid - numeric user id for user
 * gid - numeric group id for user (from password entry)
 *
 * Returns 1 if the file exists and is executable, 0 otherwise.
 *
 * NOTE: This code in and of itself isn't a good enough check unless it
 * is called by a process with its uid/gid set to the values passed in.
 * Otherwise the directory path would not necessarily be traversable by
 * the user.
 */
int is_executable(char *fn, char *un, int uid, int gid)
{
    struct stat file_stat;

    if ((stat(fn, &file_stat) < 0) || !(S_ISREG(file_stat.st_mode)))
    {
	return(0); /* no such file, or not a regular file */
    }

    if (file_stat.st_mode & S_IXOTH)
    {
	return(1); /* other executable */
    }

    if ((file_stat.st_mode & S_IXUSR) && (file_stat.st_uid == uid))
    {
	return(1); /* user executable and user owns file */
    }

    if (file_stat.st_mode & S_IXGRP)
    {
	struct group *grp_info;
	int i;

	if (file_stat.st_gid == gid)
	{
	    return(1); /* group in passwd entry matches, executable */
	}

	/* check to see if user is in this group in /etc/group */
	grp_info = getgrgid(file_stat.st_gid);
	for(i=0; grp_info->gr_mem[i]; i++)
	{
	    if (!strcmp(un, grp_info->gr_mem[i]))
	    {
		return(1); /* group from groups matched, executable */
	    }
	}
    }
    return(0);
}
#endif

SMPD_BOOL smpd_get_full_path_name(const char *exe, int maxlen, char *exe_path, char **namepart)
{
#ifdef HAVE_WINDOWS_H
    DWORD dwResult;
    DWORD dwLength;
    int len;
    char buffer[SMPD_MAX_EXE_LENGTH];
    char info_buffer[sizeof(REMOTE_NAME_INFO) + SMPD_MAX_EXE_LENGTH];
    REMOTE_NAME_INFO *info = (REMOTE_NAME_INFO*)info_buffer;
    char *filename;
    char temp_name[SMPD_MAX_EXE_LENGTH];

    /* make a full path out of the name provided */
    len = GetFullPathName(exe, maxlen, exe_path, namepart);
    if (len == 0 || len > maxlen)
    {
	smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
	return SMPD_FALSE;
    }
    *(*namepart - 1) = '\0'; /* separate the path from the executable */
    
    /* Verify file exists.  If it doesn't search the path for exe */
    if ((len = SearchPath(exe_path, *namepart, NULL, SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
    {
	if ((len = SearchPath(exe_path, *namepart, ".exe", SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
	{
	    /* search the default path for an exact match */
	    if ((len = SearchPath(NULL, *namepart, NULL, SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
	    {
		/* search the default path for a match + .exe */
		if ((len = SearchPath(NULL, *namepart, ".exe", SMPD_MAX_EXE_LENGTH, buffer, &filename)) == 0)
		{
		    smpd_dbg_printf("path not found. leaving as is in case the path exists on the remote machine.\n");
		    return SMPD_TRUE;
		}
	    }
	    if (len > SMPD_MAX_EXE_LENGTH || len > maxlen)
	    {
		smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
		return SMPD_FALSE;
	    }
	    *(filename - 1) = '\0'; /* separate the file name */
	    /* copy the path */
	    strcpy(exe_path, buffer);
	    *namepart = &exe_path[strlen(exe_path)+1];
	    /* copy the filename */
	    strcpy(*namepart, filename);
	}
    }
    if (len > maxlen)
    {
	smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n", maxlen, len);
	return SMPD_FALSE;
    }

    /* save the filename */
    strcpy(temp_name, *namepart);

    /* convert the path to its UNC equivalent to avoid need to map a drive */
    dwLength = sizeof(REMOTE_NAME_INFO)+SMPD_MAX_EXE_LENGTH;
    info->lpConnectionName = NULL;
    info->lpRemainingPath = NULL;
    info->lpUniversalName = NULL;
    dwResult = WNetGetUniversalName(exe_path, REMOTE_NAME_INFO_LEVEL, info, &dwLength);
    if (dwResult == NO_ERROR)
    {
	if ((int)(strlen(info->lpUniversalName) + strlen(temp_name) + 2) > maxlen)
	{
	    smpd_err_printf("buffer provided too short for path: %d provided, %d needed\n",
		maxlen, strlen(info->lpUniversalName) + strlen(temp_name) + 2);
	    return SMPD_FALSE;
	}
	strcpy(exe_path, info->lpUniversalName);
	*namepart = &exe_path[strlen(exe_path)+1];
	strcpy(*namepart, temp_name);
    }

    return SMPD_TRUE;
#else
    char *path = NULL;
    char temp_str[SMPD_MAX_EXE_LENGTH] = "./";

    getcwd(temp_str, SMPD_MAX_EXE_LENGTH);

    if (temp_str[strlen(temp_str)-1] != '/')
	strcat(temp_str, "/");

    /* start with whatever they give you tacked on to the cwd */
    snprintf(exe_path, maxlen, "%s%s", temp_str, exe);
    if (exists(exe_path))
    {
	*namepart = strrchr(exe_path, '/');
	*(*namepart - 1) = '\0'; /* separate the path from the executable */
	return SMPD_TRUE;
    }
    *namepart = strrchr(exe_path, '/');
    *(*namepart - 1) = '\0'; /* separate the path from the executable */

    /* add searching of the path and verifying file exists */
    path = getenv("PATH");
    strcpy(temp_str, *namepart);
    if (search_path(path, temp_str, maxlen, exe_path))
    {
	*namepart = strrchr(exe_path, '/');
	*(*namepart - 1) = '\0';
	return SMPD_TRUE;
    }
    return SMPD_FALSE;
#endif
}

SMPD_BOOL smpd_search_path(const char *path, const char *exe, int maxlen, char *str)
{
#ifdef HAVE_WINDOWS_H
    char *filepart;

    /* search for exactly what's specified */
    if (SearchPath(path, exe, NULL, maxlen, str, &filepart) == 0)
    {
	/* search for file + .exe */
	if (SearchPath(path, exe, ".exe", maxlen, str, &filepart) == 0)
	{
	    /* search the default path */
	    if (SearchPath(NULL, exe, NULL, maxlen, str, &filepart) == 0)
	    {
		/* search the default path + .exe */
		if (SearchPath(NULL, exe, ".exe", maxlen, str, &filepart) == 0)
		    return SMPD_FALSE;
	    }
	}
    }
    return SMPD_TRUE;
#else
    return search_path(path, exe, maxlen, str);
#endif
}

#ifndef HAVE_WINDOWS_H
smpd_sig_fn_t *smpd_signal( int signo, smpd_sig_fn_t func )
{
    struct sigaction act, oact;

    act.sa_handler = func;
    sigemptyset( &act.sa_mask );
    act.sa_flags = 0;
    if ( signo == SIGALRM )
    {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;   /* SunOS 4.x */
#endif
    }
    else
    {
#ifdef SA_RESTART
        act.sa_flags |= SA_RESTART;     /* SVR4, 4.4BSD */
#endif
    }
    if ( sigaction( signo,&act, &oact ) < 0 )
        return ( SIG_ERR );
    return( oact.sa_handler );
}
#endif

int smpd_create_process_struct(int rank, smpd_process_t **process_ptr)
{
    int result;
    smpd_process_t *p;
    static int cur_id = 0;

    smpd_enter_fn("smpd_create_process_struct");

    p = (smpd_process_t*)malloc(sizeof(smpd_process_t));
    if (p == NULL)
    {
	*process_ptr = NULL;
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    p->id = cur_id++; /* MT - If smpd is to be thread safe, this will have to be changed */
    p->rank = rank;
    p->nproc = 1;
    p->kvs_name[0] = '\0';
    p->exe[0] = '\0';
    p->env[0] = '\0';
    p->dir[0] = '\0';
    p->path[0] = '\0';
    p->err_msg[0] = '\0';
    p->next = NULL;
    result = smpd_create_context(SMPD_CONTEXT_STDIN, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->in);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdin context.\n");
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDOUT, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->out);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stdout context.\n");
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_STDERR, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->err);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create stderr context.\n");
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    result = smpd_create_context(SMPD_CONTEXT_PMI, smpd_process.set, SOCK_INVALID_SOCK, -1, &p->pmi);
    if (result != SMPD_SUCCESS)
    {
	free(p);
	*process_ptr = NULL;
	smpd_err_printf("unable to create pmi context.\n");
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    p->in->rank = rank;
    p->out->rank = rank;
    p->err->rank = rank;
    p->num_valid_contexts = 3;
    p->context_refcount = 0;
    p->exitcode = 0;
    p->in->process = p;
    p->out->process = p;
    p->err->process = p;
    p->pmi->process = p;
    p->next = NULL;

    *process_ptr = p;

    smpd_exit_fn("smpd_create_process_struct");
    return SMPD_SUCCESS;
}

int smpd_free_process_struct(smpd_process_t *process)
{
    smpd_enter_fn("smpd_free_process_struct");
    if (process == NULL)
    {
	smpd_dbg_printf("smpd_free_process_struct passed NULL process pointer.\n");
	smpd_exit_fn("smpd_free_process_struct");
	return SMPD_SUCCESS;
    }
    if (process->in)
	smpd_free_context(process->in);
    process->in = NULL;
    if (process->out)
	smpd_free_context(process->out);
    process->out = NULL;
    if (process->err)
	smpd_free_context(process->err);
    process->err = NULL;
    if (process->pmi)
	smpd_free_context(process->pmi);
    process->pmi = NULL;
    process->dir[0] = '\0';
    process->env[0] = '\0';
    process->exe[0] = '\0';
    process->path[0] = '\0';
    process->pid = -1;
    process->rank = -1;
    process->next = NULL;
    free(process);
    smpd_exit_fn("smpd_free_process_struct");
    return SMPD_SUCCESS;
}

int smpd_interpret_session_header(char *str)
{
    char temp_str[100];

    smpd_enter_fn("smpd_interpret_session_header");

    smpd_dbg_printf("interpreting session header: \"%s\"\n", str);

    /* get my id */
    if (smpd_get_string_arg(str, "id", temp_str, 100))
    {
	smpd_dbg_printf(" id = %s\n", temp_str);
	smpd_process.id = atoi(temp_str);
	if (smpd_process.id < 0)
	{
	    smpd_err_printf("invalid id passed in session header: %d\n", smpd_process.id);
	    smpd_process.id = 0;
	}
    }

    /* get my parent's id */
    if (smpd_get_string_arg(str, "parent", temp_str, 100))
    {
	smpd_dbg_printf(" parent = %s\n", temp_str);
	smpd_process.parent_id = atoi(temp_str);
	if (smpd_process.parent_id < 0)
	{
	    smpd_err_printf("invalid parent id passed in session header: %d\n", smpd_process.parent_id);
	    smpd_process.parent_id = -1;
	}
    }

    /* get my level */
    if (smpd_get_string_arg(str, "level", temp_str, 100))
    {
	smpd_dbg_printf(" level = %s\n", temp_str);
	smpd_process.level = atoi(temp_str);
	if (smpd_process.level < 0)
	{
	    smpd_err_printf("invalid session level passed in session header: %d\n", smpd_process.level);
	    smpd_process.level = 0;
	}
    }

    smpd_exit_fn("smpd_interpret_session_header");
    return SMPD_SUCCESS;
}
