/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "smpd.h"

int smpd_create_process_struct(int rank, smpd_process_t **process_ptr)
{
    int result;
    smpd_process_t *p;

    smpd_enter_fn("smpd_create_process_struct");

    p = (smpd_process_t*)malloc(sizeof(smpd_process_t));
    if (p == NULL)
    {
	*process_ptr = NULL;
	smpd_exit_fn("smpd_create_process_struct");
	return SMPD_FAIL;
    }
    p->rank = rank;
    p->exe[0] = '\0';
    p->env[0] = '\0';
    p->dir[0] = '\0';
    p->path[0] = '\0';
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
    p->in->rank = rank;
    p->out->rank = rank;
    p->err->rank = rank;
    p->num_valid_contexts = 3;
    p->context_refcount = 0;
    p->exitcode = 0;
    p->in->process = p;
    p->out->process = p;
    p->err->process = p;
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
