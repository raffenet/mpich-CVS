/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* brad : originally, this file was simply copied from channels/sock/src/ch3i_bizcard_cache.c
 *         to see if it compiled independently.  it did, and the functions haven't been changed
 *         from CH3I yet perhaps misleadingly.  this was so the calling functions didn't (yet)
 *         have to be changed but in the future they might need to be changed.
 *
 *        also, it may need to be more deeply investigated as to whether this implementation
 *         of a bizcard cache contained here is in some way sock-specific (i.e. would a (scalable)
 *         shared memory one look different?).
 *
 *        the error messages below have "..ch3|sock.." hard-coded into them.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"

typedef struct MPIDI_CH3I_Bizcard_cache_t {
    char *pg_id;
    int pg_size;
    char **bizcards;
    struct MPIDI_CH3I_Bizcard_cache_t *next;
} MPIDI_CH3I_Bizcard_cache_t;

static MPIDI_CH3I_Bizcard_cache_t *MPIDI_CH3I_Bizcard_cache_head = NULL;

/* Add the business card to the cache. Used in spawn/connect/accept functions */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Add_to_bizcard_cache
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Add_to_bizcard_cache(char *pg_id, int pg_size, int rank, char *bizcard)
{
    int mpi_errno=MPI_SUCCESS, i, max_bizcard_len, pgid_len;
    MPIDI_CH3I_Bizcard_cache_t* curr_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_ADD_TO_BIZCARD_CACHE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_ADD_TO_BIZCARD_CACHE);

    curr_ptr = MPIDI_CH3I_Bizcard_cache_head;

    /* --BEGIN ERROR HANDLING-- */
    if (rank >= pg_size) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|sock|pgrank", "**ch3|sock|pgrank %d %d", curr_ptr->pg_size, rank);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */

    while  ( curr_ptr && (strcmp(pg_id, curr_ptr->pg_id) != 0) )
        curr_ptr = curr_ptr->next;

    if (curr_ptr) {  
        /* pg_id found in cache. for sanity, check if the size is the same */
        /* --BEGIN ERROR HANDLING-- */
        if (curr_ptr->pg_size != pg_size) {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|sock|pgsize_cache", "**ch3|sock|pgsize_cache %d %d", curr_ptr->pg_size, pg_size);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        if (curr_ptr->bizcards[rank] != NULL) {
            /* bizcard found in cache. check if it is the same */
            /* --BEGIN ERROR HANDLING-- */
            if (strcmp(bizcard, curr_ptr->bizcards[rank]) != 0) {
                mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|sock|bizcard_cache", "**ch3|sock|bizcard_cache %s %s", curr_ptr->bizcards[rank], bizcard);
            }
            /* --END ERROR HANDLING-- */

            goto fn_exit;  /* exit if match is correct or incorrect */
        }
    }
    else {
        /* pg_id not found in cache. create an entry for it */

        curr_ptr = (MPIDI_CH3I_Bizcard_cache_t *) MPIU_Malloc(sizeof(MPIDI_CH3I_Bizcard_cache_t));
        /* --BEGIN ERROR HANDLING-- */
        if (curr_ptr == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        curr_ptr->pg_size = pg_size;

        mpi_errno = PMI_Get_id_length_max(&pgid_len);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno != PMI_SUCCESS)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_get_id_length_max", "**pmi_get_id_length_max %d", mpi_errno);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        curr_ptr->pg_id = (char *) MPIU_Malloc(pgid_len);

        MPIU_Strncpy(curr_ptr->pg_id, pg_id, pgid_len);

        curr_ptr->bizcards = (char **) MPIU_Malloc(pg_size * sizeof(char *));
        /* --BEGIN ERROR HANDLING-- */
        if (curr_ptr->bizcards == NULL)
        {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        for (i=0; i<pg_size; i++) 
            curr_ptr->bizcards[i] = NULL;

        curr_ptr->next = MPIDI_CH3I_Bizcard_cache_head;
        MPIDI_CH3I_Bizcard_cache_head = curr_ptr;
    }

    /* Add the bizcard to the cache (brad : this might as well still be in the else brackets) */

    curr_ptr->bizcards[rank] = (char *) MPIU_Malloc(strlen(bizcard)+1);
    /* --BEGIN ERROR HANDLING-- */
    if (curr_ptr->bizcards[rank] == NULL)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    mpi_errno = PMI_KVS_Get_value_length_max(&max_bizcard_len);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != PMI_SUCCESS)
    {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max", "**pmi_kvs_get_value_length_max %d", mpi_errno);
        goto fn_exit;
    }
    /* --END ERROR HANDLING-- */
    
    MPIU_Strncpy(curr_ptr->bizcards[rank], bizcard, max_bizcard_len);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_ADD_TO_BIZCARD_CACHE);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Lookup_bizcard_cache
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Lookup_bizcard_cache(char *pg_id, int rank, char *bizcard, int len, int *found)
{
    int mpi_errno=MPI_SUCCESS;
    MPIDI_CH3I_Bizcard_cache_t* curr_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_LOOKUP_BIZCARD_CACHE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_LOOKUP_BIZCARD_CACHE);

    *found = 0;
    curr_ptr = MPIDI_CH3I_Bizcard_cache_head;
   
    while  ( curr_ptr && ((strcmp(pg_id, curr_ptr->pg_id) != 0)) )
        curr_ptr = curr_ptr->next;

    if (curr_ptr) {
        /* rank should not be larger than pg_size */
        /* --BEGIN ERROR HANDLING-- */
        if (rank >= curr_ptr->pg_size) {
            mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**ch3|sock|pgrank_cache", "**ch3|sock|pgrank_cache %d %d", curr_ptr->pg_size, rank);
            goto fn_exit;
        }
        /* --END ERROR HANDLING-- */

        if (curr_ptr->bizcards[rank] != NULL) {
            *found = 1;
            MPIU_Strncpy(bizcard, curr_ptr->bizcards[rank], len);
        }
    }

 fn_exit:    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_LOOKUP_BIZCARD_CACHE);
    return mpi_errno;
}

int MPIDI_CH3I_Bizcard_cache_free(void)
{
    MPIDI_CH3I_Bizcard_cache_t *curr_ptr, *prev_ptr;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_BIZCARD_CACHE_FREE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_BIZCARD_CACHE_FREE);

    curr_ptr = MPIDI_CH3I_Bizcard_cache_head;

    while (curr_ptr != NULL) {
        MPIU_Free(curr_ptr->pg_id);
        for (i=0; i<curr_ptr->pg_size; i++) {
            if (curr_ptr->bizcards[i])
                MPIU_Free(curr_ptr->bizcards[i]);
        }
        MPIU_Free(curr_ptr->bizcards);
        prev_ptr = curr_ptr;
        curr_ptr = curr_ptr->next;
        MPIU_Free(prev_ptr);
    }
    
    MPIDI_CH3I_Bizcard_cache_head = NULL;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_BIZCARD_CACHE_FREE);
    return MPI_SUCCESS;
}

/* brad : returns rank if found or -1 if not. only used for intercommunicator in shared mem since
*          the rank-vc mappings (usually) differ on each side of an intercommunicator
*/
int MPIDI_CH3I_Bizcard_Search(char *pg_id, int pid)
{
#ifdef MPIDI_CH3_USES_SSHM
    int mpi_errno, i;
    char val[1000],  val2[1000];
    char my_hostname[256];
    MPIDI_CH3I_Bizcard_cache_t *curr_ptr = MPIDI_CH3I_Bizcard_cache_head;

    while  ( curr_ptr && ((strcmp(pg_id, curr_ptr->pg_id) != 0)) )
        curr_ptr = curr_ptr->next;

    if(curr_ptr)
    {
        gethostname(my_hostname, 256);
        for(i=0; i < curr_ptr->pg_size; i++)
        {
            /* get rid of 1000, fix error conditions */
            mpi_errno = MPIU_Str_get_string_arg(curr_ptr->bizcards[i],MPIDI_CH3I_SHM_QUEUE_KEY, val, 1000);
            if (mpi_errno != MPIU_STR_SUCCESS)
                return -1;
            mpi_errno = MPIU_Str_get_string_arg(curr_ptr->bizcards[i],MPIDI_CH3I_SHM_HOST_KEY, val2, 1000);
            if (mpi_errno != MPIU_STR_SUCCESS)
                return -1;
            /* brad : could have same pid on different host */
            if(pid == atoi(val) && strcmp(val2, my_hostname) == 0)
                return i;
        }
    }
#else
    MPIU_UNREFERENCED_ARG(pg_id);
    MPIU_UNREFERENCED_ARG(pid);
#endif
    /* not found */
    return -1;
}
