/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef MPIDI_DEV_IMPLEMENTS_KVS
#include "pmi.h"
#endif

static MPIDI_PG_t * MPIDI_PG_list = NULL;
static MPIDI_PG_t * MPIDI_PG_iterator_next = NULL;
static MPIDI_PG_Compare_ids_fn_t MPIDI_PG_Compare_ids_fn;
static MPIDI_PG_Destroy_fn_t MPIDI_PG_Destroy_fn;


#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Init(MPIDI_PG_Compare_ids_fn_t compare_ids_fn, MPIDI_PG_Destroy_fn_t destroy_fn)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_PG_Compare_ids_fn = compare_ids_fn;
    MPIDI_PG_Destroy_fn = destroy_fn;

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

/* ifdefing out this check because the list will not be NULL in Ch3_finalize because
   one additional reference is retained in MPIDI_Process.my_pg. That reference is released
   only after ch3_finalize returns. If I release it before ch3_finalize, the ssm channel
   crashes.
*/
    
#ifdef FOO

    if (MPIDI_PG_list != NULL)
    { 
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN,
        "**dev|pg_finalize|list_not_empty", NULL); 
	/* --END ERROR HANDLING-- */
    }
#endif

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg = NULL;
    int p;
    int mpi_errno = MPI_SUCCESS;
    
    pg = MPIU_Malloc(sizeof(MPIDI_PG_t));
    if (pg == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    pg->vct = MPIU_Malloc(sizeof(MPIDI_VC_t) * vct_sz);
    if (pg->vct == NULL)
    {
	/* --BEGIN ERROR HANDLING-- */
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", NULL);
	goto fn_fail;
	/* --END ERROR HANDLING-- */
    }

    pg->handle = 0;
    MPIU_Object_set_ref(pg, vct_sz);
    pg->size = vct_sz;
    pg->id = pg_id;
    
    for (p = 0; p < vct_sz; p++)
    {
	/* Initialize device fields in the VC object */
	MPIDI_VC_Init(&pg->vct[p], pg, p);
    }

    pg->next = MPIDI_PG_list;
    if (MPIDI_PG_iterator_next == MPIDI_PG_list)
    {
	MPIDI_PG_iterator_next = pg;
    }
    MPIDI_PG_list = pg;
    
    *pg_ptr = pg;
    
  fn_exit:
    return mpi_errno;
    
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    if (pg->vct != NULL)
    {
	MPIU_Free(pg->vct);
    }

    if (pg != NULL)
    {
	MPIU_Free(pg);
    }

    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Destroy(MPIDI_PG_t * pg)
{
    /*int i;*/
    MPIDI_PG_t * pg_prev;
    MPIDI_PG_t * pg_cur;
    int mpi_errno = MPI_SUCCESS;

    pg_prev = NULL;
    pg_cur = MPIDI_PG_list;
    while(pg_cur != NULL)
    {
	if (pg_cur == pg)
	{
	    if (MPIDI_PG_iterator_next == pg)
	    { 
		MPIDI_PG_iterator_next = MPIDI_PG_iterator_next->next;
	    }

            if (pg_prev == NULL)
                MPIDI_PG_list = pg->next; 
            else
                pg_prev->next = pg->next;

	    /*
	    for (i=0; i<pg->size; i++)
	    {
		printf("[%s%d]freeing vc%d - %p (%s)\n", MPIU_DBG_parent_str, MPIR_Process.comm_world->rank, i, &pg->vct[i], pg->id);fflush(stdout);
	    }
	    */
	    MPIDI_PG_Destroy_fn(pg, pg->id);
	    MPIU_Free(pg->vct);
	    MPIU_Free(pg);

	    goto fn_exit;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

    /* --BEGIN ERROR HANDLING-- */
    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
				     "**dev|pg_not_found", "**dev|pg_not_found %p", pg);
    /* --END ERROR HANDLING-- */

  fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Find
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg;
    int mpi_errno = MPI_SUCCESS;
    
    pg = MPIDI_PG_list;
    while (pg != NULL)
    {
	if (MPIDI_PG_Compare_ids_fn(id, pg->id) != FALSE)
	{
	    *pg_ptr = pg;
	    goto fn_exit;
	}

	pg = pg->next;
    }

    *pg_ptr = NULL;

  fn_exit:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Id_compare
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Id_compare(void * id1, void *id2)
{
    return MPIDI_PG_Compare_ids_fn(id1, id2);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Get_next
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Get_next(MPIDI_PG_t ** pg_ptr)
{
    *pg_ptr = MPIDI_PG_iterator_next;
    if (MPIDI_PG_iterator_next != NULL)
    { 
	MPIDI_PG_iterator_next = MPIDI_PG_iterator_next->next;
    }

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Iterate_reset
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Iterate_reset()
{
    MPIDI_PG_iterator_next = MPIDI_PG_list;
    return MPI_SUCCESS;
}

#ifdef MPIDI_DEV_IMPLEMENTS_KVS

#undef FUNCNAME
#define FUNCNAME MPIDI_Allocate_more
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPIDI_Allocate_more(char **str, char **cur_pos, int *cur_len)
{
    char *longer;
    size_t orig_length, longer_length;

    orig_length = (*cur_pos - *str);
    longer_length = (*cur_pos - *str) + (1024*1024);

    longer = (char*)MPIU_Malloc(longer_length * sizeof(char));
    if (longer == NULL)
    {
	return MPI_ERR_NO_MEM;
    }
    memcpy(longer, *str, orig_length);
    longer[orig_length] = '\0';
    MPIU_Free(*str);
    
    *str = longer;
    *cur_pos = &longer[orig_length];
    *cur_len = (int)(longer_length - orig_length);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_To_string
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_To_string(MPIDI_PG_t *pg_ptr, char **str_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    char key[MPIDI_MAX_KVS_KEY_LEN];
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    char *str, *cur_pos;
    int cur_len;
    char len_str[20];

    cur_len = 1024*1024;
    str = (char*)MPIU_Malloc(cur_len*sizeof(char));
    if (str == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    cur_pos = str;
    /* Save the PG id */
    mpi_errno = MPIU_Str_add_string(&cur_pos, &cur_len, pg_ptr->id);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    /* Save the PG size */
    MPIU_Snprintf(len_str, 20, "%d", pg_ptr->size);
    mpi_errno = MPIU_Str_add_string(&cur_pos, &cur_len, len_str);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    /* Save all the KVS entries for this PG */
    mpi_errno = MPIDI_KVS_First(pg_ptr->ch.kvs_name, key, val);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    while (mpi_errno == MPI_SUCCESS && key[0] != '\0')
    {
	mpi_errno = MPIU_Str_add_string(&cur_pos, &cur_len, key);
	if (mpi_errno != MPIU_STR_SUCCESS)
	{
	    mpi_errno = MPIDI_Allocate_more(&str, &cur_pos, &cur_len);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		goto fn_exit;
	    }
	}
	mpi_errno = MPIU_Str_add_string(&cur_pos, &cur_len, val);
	if (mpi_errno != MPIU_STR_SUCCESS)
	{
	    mpi_errno = MPIDI_Allocate_more(&str, &cur_pos, &cur_len);
	    if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		goto fn_exit;
	    }
	}
	mpi_errno = MPIDI_KVS_Next(pg_ptr->ch.kvs_name, key, val);
    }
    *str_ptr = str;
fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Create_with_kvs
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Create_from_string(char * str, MPIDI_PG_t ** pg_pptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    char key[MPIDI_MAX_KVS_KEY_LEN];
    char val[MPIDI_MAX_KVS_VALUE_LEN];
    char *pg_id;
    int pgid_len;
    char sz_str[20];
    int vct_sz;
    MPIDI_PG_t *existing_pg, *pg_ptr;

    mpi_errno = PMI_Get_id_length_max(&pgid_len);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    pgid_len = MPIDU_MAX(pgid_len, MPIDI_MAX_KVS_NAME_LEN);

    pg_id = (char*)MPIU_Malloc(pgid_len * sizeof(char));
    /* --BEGIN ERROR HANDLING-- */
    if (pg_id == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    mpi_errno = MPIU_Str_get_string(&str, pg_id, pgid_len);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    mpi_errno = MPIU_Str_get_string(&str, sz_str, 20);
    if (mpi_errno != MPIU_STR_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    vct_sz = atoi(sz_str);

    mpi_errno = MPIDI_PG_Find(pg_id, &existing_pg);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
	goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    if (existing_pg != NULL)
    {
	/* return the existing PG */
	*pg_pptr = existing_pg;
	*flag = 0;
	goto fn_exit;
    }
    *flag = 1;

    mpi_errno = MPIDI_PG_Create(vct_sz, pg_id, pg_pptr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    pg_ptr = *pg_pptr;
    pg_ptr->ch.kvs_name = (char*)MPIU_Malloc(MPIDI_MAX_KVS_NAME_LEN * sizeof(char));
    if (pg_ptr->ch.kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }
    mpi_errno = MPIDI_KVS_Create(pg_ptr->ch.kvs_name);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	goto fn_exit;
    }

    key[0] = '\0';
    MPIU_Str_get_string(&str, key, MPIDI_MAX_KVS_KEY_LEN);
    MPIU_Str_get_string(&str, val, MPIDI_MAX_KVS_VALUE_LEN);
    while (key[0] != '\0')
    {
	mpi_errno = MPIDI_KVS_Put(pg_ptr->ch.kvs_name, key, val);
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	    goto fn_exit;
	}
	key[0] = '\0';
	MPIU_Str_get_string(&str, key, MPIDI_MAX_KVS_KEY_LEN);
	MPIU_Str_get_string(&str, val, MPIDI_MAX_KVS_VALUE_LEN);
    }

fn_exit:
    return mpi_errno;
}

void MPIDI_PG_IdToNum( MPIDI_PG_t *pg, int *id )
{
    const char *p = (const char *)pg->id;
    int pgid = 0;
    while (*p && !isdigit(*p)) p++;
    if (!*p) {
	p = (const char *)pg->id;
	while (*p) {
	    pgid += *p - ' ';
	}
	pgid = pgid ^ 0x100;
    }
    else {
	while (*p && isdigit(*p)) {
	    pgid = pgid * 10 + (*p++ - '0');
	}
    }
    *id = pgid;
}
#else
/* FIXME: This is a temporary hack for devices that do not define
   MPIDI_DEV_IMPLEMENTS_KVS
   FIXME: MPIDI_DEV_IMPLEMENTS_KVS should be removed
 */
void MPIDI_PG_IdToNum( MPIDI_PG_t *pg, int *id )
{
    *id = 0;
}
#endif
