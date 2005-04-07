/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

typedef struct smpd_job_key_list_t
{
    char key[SMPD_MAX_NAME_LENGTH];
    char username[SMPD_MAX_NAME_LENGTH];
    HANDLE user_handle;
    struct smpd_job_key_list_t *next;
} smpd_job_key_list_t;

static smpd_job_key_list_t *list = NULL;

#undef FCNAME
#define FCNAME "smpd_add_job_key"
int smpd_add_job_key(const char *key, const char *username)
{
    smpd_job_key_list_t *node;
    smpd_enter_fn(FCNAME);

    node = (smpd_job_key_list_t*)malloc(sizeof(smpd_job_key_list_t));
    if (node == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(node->key, key);
    strcpy(node->username, username);
    node->user_handle = INVALID_HANDLE_VALUE;
    node->next = list;
    list = node;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_add_job_key"
int smpd_add_job_key_and_handle(const char *key, const char *username, HANDLE hUser)
{
    smpd_job_key_list_t *node;
    smpd_enter_fn(FCNAME);

    node = (smpd_job_key_list_t*)malloc(sizeof(smpd_job_key_list_t));
    if (node == NULL)
    {
	smpd_exit_fn(FCNAME);
	return SMPD_FAIL;
    }
    strcpy(node->key, key);
    strcpy(node->username, username);
    node->user_handle = hUser;
    node->next = list;
    list = node;

    smpd_exit_fn(FCNAME);
    return SMPD_SUCCESS;
}

#undef FCNAME
#define FCNAME "smpd_remove_job_key"
int smpd_remove_job_key(const char *key)
{
    smpd_job_key_list_t *iter, *trailer;
    smpd_enter_fn(FCNAME);

    iter = trailer = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    if (iter == list)
	    {
		list = list->next;
	    }
	    else
	    {
		trailer->next = iter->next;
	    }
	    if (iter->user_handle != INVALID_HANDLE_VALUE)
		CloseHandle(iter->user_handle);
	    free(iter);
	    smpd_exit_fn(FCNAME);
	    return SMPD_SUCCESS;
	}
	if (trailer != iter)
	    trailer = trailer->next;
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_associate_job_key"
int smpd_associate_job_key(const char *key, const char *username, HANDLE user_handle)
{
    smpd_job_key_list_t *iter;
    smpd_enter_fn(FCNAME);

    iter = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    /* key matches */
	    if (strcmp(iter->username, username) == 0)
	    {
		/* username matches */
		if (iter->user_handle == INVALID_HANDLE_VALUE)
		{
		    /* handle not already set */
		    smpd_dbg_printf("associated user token: %s,%p\n", username, user_handle);
		    iter->user_handle = user_handle;
		    smpd_exit_fn(FCNAME);
		    return SMPD_SUCCESS;
		}
	    }
	}
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}

#undef FCNAME
#define FCNAME "smpd_lookup_job_key"
int smpd_lookup_job_key(const char *key, const char *username, HANDLE *user_handle)
{
    smpd_job_key_list_t *iter;
    smpd_enter_fn(FCNAME);

    iter = list;
    while (iter)
    {
	if (strcmp(iter->key, key) == 0)
	{
	    if (strcmp(iter->username, username) == 0)
	    {
		if (iter->user_handle != INVALID_HANDLE_VALUE)
		{
		    *user_handle = iter->user_handle;
		    smpd_exit_fn(FCNAME);
		    return SMPD_SUCCESS;
		}
	    }
	}
	iter = iter->next;
    }

    smpd_exit_fn(FCNAME);
    return SMPD_FAIL;
}
