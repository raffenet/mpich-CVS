/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "pmi.h"

/* FIXME: These routines need a description.  What is their purpose?  Who
   calls them and why?  What does each one do?
*/
static MPIDI_PG_t * MPIDI_PG_list = NULL;
static MPIDI_PG_t * MPIDI_PG_iterator_next = NULL;
static MPIDI_PG_Compare_ids_fn_t MPIDI_PG_Compare_ids_fn;
static MPIDI_PG_Destroy_fn_t MPIDI_PG_Destroy_fn;

/* Key track of the process group corresponding to the MPI_COMM_WORLD 
   of this process */
static MPIDI_PG_t *pg_world = NULL;

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Init(MPIDI_PG_Compare_ids_fn_t compare_ids_fn, 
		  MPIDI_PG_Destroy_fn_t destroy_fn)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIDI_PG_Compare_ids_fn = compare_ids_fn;
    MPIDI_PG_Destroy_fn     = destroy_fn;

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* FIXME - straighten out the use of PG_Finalize - no use after 
       PG_Finalize */
    /* ifdefing out this check because the list will not be NULL in 
       Ch3_finalize because
       one additional reference is retained in MPIDI_Process.my_pg. 
       That reference is released
       only after ch3_finalize returns. If I release it before ch3_finalize, 
       the ssm channel crashes. */

    if (pg_world->connData) {
	int rc;
	rc = PMI_Finalize();
	if (rc) {
	    MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER, 
			  "**ch3|pmi_finalize", 
			  "**ch3|pmi_finalize %d", rc);
	}
    }
#if 0

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

/* FIXME: This routine needs to make it clear that the pg_id, for example
   is saved; thus, if the pg_id is a string, then that string is not 
   copied and must be freed by a PG_Destroy routine */

/* This routine creates a new process group description and appends it to 
   the list of the known process groups.  The pg_id is saved, not copied.
   The PG_Destroy routine that was set with MPIDI_PG_Init is responsible for
   freeing any storage associated with the pg_id. 

   The new process group is returned in pg_ptr 
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg = NULL, *pgnext;
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKPMEM_DECL(2);
    
    MPIU_CHKPMEM_MALLOC(pg,MPIDI_PG_t*,sizeof(MPIDI_PG_t),mpi_errno,"pg");
    MPIU_CHKPMEM_MALLOC(pg->vct,MPIDI_VC_t *,sizeof(MPIDI_VC_t)*vct_sz,
			mpi_errno,"pg->vct");

    pg->handle = 0;
    /* FIXME: This reference count may be too large, depending on 
       what communicator is associated with the process group. */
    MPIU_Object_set_ref(pg, vct_sz);
    pg->size = vct_sz;
    pg->id = pg_id;
    
    for (p = 0; p < vct_sz; p++)
    {
	/* Initialize device fields in the VC object */
	MPIDI_VC_Init(&pg->vct[p], pg, p);
    }
    
    /* Initialize the connection information to null.  Use
       the appropriate MPIDI_PG_InitConnXXX routine to set up these 
       fields */
    pg->connData           = 0;
    pg->getConnInfo        = 0;
    pg->connInfoToString   = 0;
    pg->connInfoFromString = 0;
    pg->freeConnInfo       = 0;

    /* The first process group is always the world group */
    if (!pg_world) { pg_world = pg; }
#if 0
    /* Add pg's to the head */
    pg->next = MPIDI_PG_list;
    if (MPIDI_PG_iterator_next == MPIDI_PG_list)
    {
	MPIDI_PG_iterator_next = pg;
    }
    MPIDI_PG_list = pg;
#else
    /* Add pg's at the tail so that comm world is always the first pg */
    pg->next = 0;
    if (!MPIDI_PG_list)
    {
	MPIDI_PG_list = pg;
    }
    else
    {
	pgnext = MPIDI_PG_list; 
	while (pgnext->next)
	{
	    pgnext = pgnext->next;
	}
	pgnext->next = pg;
    }
#endif    
    *pg_ptr = pg;
    
  fn_exit:
    return mpi_errno;
    
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
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
	    MPIDI_PG_Destroy_fn(pg);
	    MPIU_Free(pg->vct);
	    MPIU_Free(pg);

	    goto fn_exit;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

    /* PG not found if we got here */
    MPIU_ERR_SET1(mpi_errno,MPI_ERR_OTHER,
		  "**dev|pg_not_found", "**dev|pg_not_found %p", pg);

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

/* FIXME: What does DEV_IMPLEMENTS_KVS mean?  Why is it used?  Who uses 
   PG_To_string and why?  */

#ifdef MPIDI_DEV_IMPLEMENTS_KVS

/* Note: Allocated memory that is returned in str_ptr.  The user of
   this routine must free that data */
#undef FUNCNAME
#define FUNCNAME MPIDI_PG_To_string
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_To_string(MPIDI_PG_t *pg_ptr, char **str_ptr, int *lenStr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Replace this with the new string */
    if (pg_ptr->connInfoToString) {
	(*pg_ptr->connInfoToString)( str_ptr, lenStr, pg_ptr );
#if 0
	{ char *p; int i, len = *lenStr;
	p = *str_ptr;
	printf( "pg id is %s\n", p ); while (*p) p++; p++;
	printf( "size is %s\n", p ); while (*p) p++; p++;
	for (i=0; i<pg_ptr->size; i++) {
	    printf( "[%d] = %s\n", i, p );
	    while (*p) p++; p++;
	}
	printf( "string len is %d\n", len );fflush(stdout);
	}
#endif
    }
    else {
	printf( "Panic! no connInfoToString!\n" );fflush(stdout);
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This routine takes a string description of a process group (created with 
   MPIDI_PG_To_string, usually on a different process) and returns a pointer to
   the matching process group.  If the group already exists, flag is set to 
   false.  If the group does not exist, it is created with MPIDI_PG_Create (and
   hence is added to the list of active process groups) and flag is set to 
   true.  In addition, the connection information is set up using the 
   information in the input string.
*/
#undef FUNCNAME
#define FUNCNAME MPIDI_PG_Create_from_string
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_Create_from_string(char * str, MPIDI_PG_t ** pg_pptr, int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    char *p;
    int vct_sz;
    MPIDI_PG_t *existing_pg, *pg_ptr=0;

    /* The pg_id is at the beginning of the string, so we can just pass
       it to the find routine */
    /* printf( "Looking for pg with id %s\n", str );fflush(stdout); */
    mpi_errno = MPIDI_PG_Find(str, &existing_pg);
    if (mpi_errno != PMI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }

    if (existing_pg != NULL) {
	/* return the existing PG */
	*pg_pptr = existing_pg;
	*flag = 0;
	/* Note that the memory for the pg_id is freed in the exit */
	goto fn_exit;
    }
    *flag = 1;

    /* Get the size from the string */
    p = str;
    while (*p) p++; p++;
    vct_sz = atoi(p);

    mpi_errno = MPIDI_PG_Create(vct_sz, str, pg_pptr);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_POP(mpi_errno);
    }
    
    pg_ptr = *pg_pptr;
    pg_ptr->id = MPIU_Strdup( str );
    
    /* Set up the functions to use strings to manage connection information */
    MPIDI_PG_InitConnString( pg_ptr );
    (*pg_ptr->connInfoFromString)( str, pg_ptr );

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#ifdef HAVE_CTYPE_H
/* Needed for isdigit */
#include <ctype.h>
#endif

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
	/* mpd uses (pid_num) as part of the kvs name, so
	   we skip over - and _ */
	while (*p) {
	    if (isdigit(*p)) {
		pgid = pgid * 10 + (*p - '0');
	    }
	    else if (*p != '-' && *p != '_') {
		break;
	    }
	    p++;
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

/*
 * Managing connection information for process groups
 * 
 *
 */

/* Setting a process's connection information 
   
   This is a collective call (for scalability) over all of the processes in 
   the same MPI_COMM_WORLD.
*/
int MPIDI_PG_SetConnInfo( int rank, const char *connString )
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char key[128];

    MPIU_Assert(pg_world->connData);
    
    mpi_errno = MPIU_Snprintf(key, sizeof(key), "P%d-businesscard", rank);
    if (mpi_errno < 0 || mpi_errno > sizeof(key)) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**snprintf",
			     "**snprintf %d", mpi_errno);
    }
    pmi_errno = PMI_KVS_Put(pg_world->connData, key, connString );
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_put",
			     "**pmi_kvs_put %d", pmi_errno);
    }
    pmi_errno = PMI_KVS_Commit(pg_world->connData);
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_kvs_commit",
			     "**pmi_kvs_commit %d", pmi_errno);
    }
    
    pmi_errno = PMI_Barrier();
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, "**pmi_barrier",
			     "**pmi_barrier %d", pmi_errno);
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* For all of these routines, the format of the process group description
   that is created and used by the connTo/FromString routines is this:
   (All items are strings, terminated by null)

   process group id string
   sizeof process group (as string)
   conninfo for rank 0
   conninfo for rank 1
   ...

   The "conninfo for rank 0" etc. for the original (MPI_COMM_WORLD)
   process group are stored in the PMI_KVS space with the keys 
   p<rank>-businesscard .  

   Fixme: Add a routine to publish the connection info to this file so that
   the key for the businesscard is defined in just this one file.
*/


/* The "KVS" versions are for the process group to which the calling 
   process belongs.  These use the PMI_KVS routines to access the
   process information */
static int getConnInfoKVS( int rank, char *buf, int bufsize, MPIDI_PG_t *pg )
{
    char key[MPIDI_MAX_KVS_KEY_LEN];
    int  mpi_errno = MPI_SUCCESS, rc, pmi_errno;;

    rc = MPIU_Snprintf(key, MPIDI_MAX_KVS_KEY_LEN, "P%d-businesscard", rank );
    if (rc < 0 || rc > MPIDI_MAX_KVS_KEY_LEN) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
    }
    pmi_errno = PMI_KVS_Get(pg->connData, key, buf, bufsize );
    if (pmi_errno) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_get");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static int connToStringKVS( char **buf_p, int *slen, MPIDI_PG_t *pg )
{
    char *string = 0;
    char *pg_idStr = (char *)pg->id;      /* In the PMI/KVS space,
					     the pg id is a string */
    char buf[MPIDI_MAX_KVS_VALUE_LEN];
    int   i, j, vallen, rc, mpi_errno = MPI_SUCCESS, len;
    int   curSlen;

    /* Make an initial allocation of a string with an estimate of the
       needed space */
    len = 0;
    curSlen = 10 + pg->size * 128;
    string = (char *)MPIU_Malloc( curSlen );

    /* Start with the id of the pg */
    while (*pg_idStr && len < curSlen) 
	string[len++] = *pg_idStr++;
    string[len++] = 0;
    
    /* Add the size of the pg */
    MPIU_Snprintf( &string[len], curSlen, "%d", pg->size );
    while (string[len]) len++;
    len++;

    for (i=0; i<pg->size; i++) {
	rc = getConnInfoKVS( i, buf, MPIDI_MAX_KVS_VALUE_LEN, pg );
#ifndef USE_PERSISTENT_SHARED_MEMORY
	/* FIXME: This is a hack to avoid including shared-memory 
	   queue names in the buisness card that may be used
	   by processes that were not part of the same COMM_WORLD. 
	   To fix this, the shared memory channels should look at the
	   returned connection info and decide whether to use 
	   sockets or shared memory by determining whether the
	   process is in the same MPI_COMM_WORLD. */
/*	printf( "Adding key %s value %s\n", key, val ); */
	{
	char *p = strstr( buf, "$shm_host" );
	if (p) p[1] = 0;
	/*	    printf( "(fixed) Adding key %s value %s\n", key, val ); */
	}
#endif
	/* Add the information to the output buffer */
	vallen = strlen(buf);
	/* Check that this will fix in the remaining space */
	if (len + vallen + 1 >= curSlen) {
	    string = MPIU_Realloc( string, 
				   curSlen + (pg->size - i) * (vallen + 1 ));
	}
	/* Append to string */
	for (j=0; j<vallen+1; j++) {
	    string[len++] = buf[j];
	}
    }

    *buf_p = string;
    *slen  = len;
 fn_exit:
    return mpi_errno;
 fn_fail:
    if (string) MPIU_Free(string);
    goto fn_exit;
}
static int connFromStringKVS( const char *buf, MPIDI_PG_t *pg )
{
    /* Fixme: this should be a failure to call this routine */
    return MPI_SUCCESS;
}
static int connFreeKVS( MPIDI_PG_t *pg )
{
    /* In this implementation, there is no local data */
    return MPI_SUCCESS;
}

int MPIDI_PG_InitConnKVS( MPIDI_PG_t *pg )
{
    int pmi_errno, kvs_name_sz;
    int mpi_errno = MPI_SUCCESS;

    pmi_errno = PMI_KVS_Get_name_length_max( &kvs_name_sz );
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_kvs_get_name_length_max", 
			     "**pmi_kvs_get_name_length_max %d", pmi_errno);
    }
    
    pg->connData = (char *)MPIU_Malloc(kvs_name_sz + 1);
    if (pg->connData == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }
    
    pmi_errno = PMI_KVS_Get_my_name(pg->connData, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER, 
			     "**pmi_kvs_get_my_name", 
			     "**pmi_kvs_get_my_name %d", pmi_errno);
    }
    
    pg->getConnInfo        = getConnInfoKVS;
    pg->connInfoToString   = connToStringKVS;
    pg->connInfoFromString = connFromStringKVS;
    pg->freeConnInfo       = connFreeKVS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    if (pg->connData) { MPIU_Free(pg->connData); }
    goto fn_exit;
}

/* Return the kvsname associated with the MPI_COMM_WORLD of this process. */
int MPIDI_PG_GetConnKVSname( char ** kvsname )
{
    *kvsname = pg_world->connData;
    return MPI_SUCCESS;
}

/* For process groups that are not our MPI_COMM_WORLD, store the connection
   information in an array of strings.  These routines and structure
   implement the access to this information. */
typedef struct {
    int     toStringLen;   /* Length needed to encode this connection info */
    char ** connStrings;   /* pointer to an array, indexed by rank, containing
			      connection information */
} MPIDI_ConnInfo;

static int getConnInfo( int rank, char *buf, int bufsize, MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *connInfo = (MPIDI_ConnInfo *)pg->connData;

    /* printf( "Entering getConnInfo\n" ); fflush(stdout); */
    if (!connInfo || !connInfo->connStrings || !connInfo->connStrings[rank]) {
	/* FIXME: Turn this into a valid error code create/return */
	printf( "Fatal error in getConnInfo (rank = %d)\n", rank );
	printf( "connInfo = %p\n", connInfo );fflush(stdout);
	if (connInfo) {
	    printf( "connInfo->connStrings = %p\n", connInfo->connStrings );
	}
	/* Fatal error.  Connection information missing */
	fflush(stdout);   
    }

    /* printf( "Copying %s to buf\n", connInfo->connStrings[rank] ); fflush(stdout); */
    
    MPIU_Strncpy( buf, connInfo->connStrings[rank], bufsize );
    return MPI_SUCCESS;
}
static int connToString( char **buf_p, int *slen, MPIDI_PG_t *pg )
{
    char *string = 0, *str, *pg_id;
    int  i, len=0;
    
    MPIDI_ConnInfo *connInfo = (MPIDI_ConnInfo *)pg->connData;

    /* Create this from the string array */
    string = (char *)MPIU_Malloc( connInfo->toStringLen );
    str = string;

    pg_id = pg->id;
    while (*pg_id) str[len++] = *pg_id++;
    str[len++] = 0;
    
    MPIU_Snprintf( &str[len], 20, "%d", pg->size);
    /* Skip over the length */
    while (str[len++]);

    /* Copy each connection string */
    for (i=0; i<pg->size; i++) {
	char *p = connInfo->connStrings[i];
	while (*p) { str[len++] = *p++; }
	str[len++] = 0;
    }

    if (len > connInfo->toStringLen) {
	*buf_p = 0;
	*slen  = 0;
	return MPIR_Err_create_code(MPI_SUCCESS,MPIR_ERR_FATAL,"connToString",
			    __LINE__, MPI_ERR_INTERN, "**intern", NULL);
    }

    *buf_p = string;
    *slen = len;

    return MPI_SUCCESS;
}
static int connFromString( const char *buf, MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *conninfo = 0;
    int i, mpi_errno = MPI_SUCCESS;
    int totlen = 0;
    const char *buf0 = buf;   /* save the start of buf */

    /* printf( "Starting with buf = %s\n", buf );fflush(stdout); */

    /* Skip the pg id */
    while (*buf) buf++; buf++;

    /* Determine the size of the pg */
    pg->size = atoi( buf );
    while (*buf) buf++; buf++;

    conninfo = (MPIDI_ConnInfo *)MPIU_Malloc( sizeof(MPIDI_ConnInfo) );
    conninfo->connStrings = (char **)MPIU_Malloc( pg->size * sizeof(char *));

    /* For now, make a copy of each item */
    for (i=0; i<pg->size; i++) {
	/* printf( "Adding conn[%d] = %s\n", i, buf );fflush(stdout); */
	conninfo->connStrings[i] = MPIU_Strdup( buf );
	while (*buf) buf++;
	buf++;
    }
    pg->connData = conninfo;
	
    /* Save the length of the string needed to encode the connection
       information */
    conninfo->toStringLen = (int)(buf - buf0) + 1;

    return mpi_errno;
}
static int connFree( MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *conninfo = (MPIDI_ConnInfo *)pg->connData;
    int i;

    for (i=0; i<pg->size; i++) {
	MPIU_Free( conninfo->connStrings[i] );
    }
    MPIU_Free( conninfo->connStrings );
    MPIU_Free( conninfo );

    return MPI_SUCCESS;
}

int MPIDI_PG_InitConnString( MPIDI_PG_t *pg )
{
    int mpi_errno = MPI_SUCCESS;

    pg->connData           = 0;
    pg->getConnInfo        = getConnInfo;
    pg->connInfoToString   = connToString;
    pg->connInfoFromString = connFromString;
    pg->freeConnInfo       = connFree;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Temp to get connection value for rank r */
#undef FUNCNAME
#define FUNCNAME MPIDI_PG_GetConnString
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_PG_GetConnString( MPIDI_PG_t *pg, int rank, char *val, int vallen )
{
    int mpi_errno = MPI_SUCCESS;

    if (pg->getConnInfo) {
	mpi_errno = (*pg->getConnInfo)( rank, val, vallen, pg );
    }
    else {
	printf( "Panic: no getConnInfo defined!\n" );
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
