/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"
#include "globus_usage.h"

/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)
#define MPIG_DEBUG_TMPSTR_SIZE ((size_t) 1024)

globus_debug_handle_t mpig_debug_handle;
time_t mpig_debug_start_tv_sec;


#define MPIG_UPPERCASE_STR(str_)	\
{					\
    char * tmp__;			\
					\
    tmp__ = (str_);			\
    while(*tmp__ != '\0')		\
    {					\
	if (islower(*tmp__))		\
	{				\
	    *tmp__ = toupper(*tmp__);	\
	}				\
	tmp__++;			\
    }					\
}


void mpig_debug_init(void)
{
    const char * levels;
    char * levels_uc;
    const char * timed_levels;
    char * timed_levels_uc = NULL;
    char settings[MPIG_DEBUG_TMPSTR_SIZE];
    const char * file_basename;
    struct timeval tv;

    levels = globus_libc_getenv("MPIG_DEBUG_LEVELS");
    if (levels == NULL || strlen(levels) == 0) goto fn_return;
    levels_uc = MPIU_Strdup(levels);
    MPIG_UPPERCASE_STR(levels_uc);
    
    file_basename = globus_libc_getenv("MPIG_DEBUG_FILE_BASENAME");
    if (file_basename != NULL)
    {
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "%s-%s-%d.log",
		      file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
	setenv("MPICH_DBG_FILENAME", settings, TRUE);
	
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,#%s-%s-%d.log,0",
		      levels_uc, file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
    }
    else
    {
	/* send debugging output to stderr */
	setenv("MPICH_DBG_FILENAME", "-stderr-", TRUE);
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,,0", levels_uc);
    }
    
    timed_levels = globus_libc_getenv("MPIG_DEBUG_TIMED_LEVELS");
    if (timed_levels != NULL)
    {
	const int len = strlen(settings);
	
	timed_levels_uc = MPIU_Strdup(timed_levels);
	MPIG_UPPERCASE_STR(timed_levels_uc);
	MPIU_Snprintf(settings + len, MPIG_DEBUG_TMPSTR_SIZE - len, ",%s", timed_levels_uc);
    }

    MPIU_DBG_Init(NULL, NULL, mpig_process.my_pg_rank);

    globus_module_setenv("MPIG_DEBUG_GLOBUS_DEBUG_SETTINGS", settings);
    globus_debug_init("MPIG_DEBUG_GLOBUS_DEBUG_SETTINGS", MPIG_DEBUG_LEVEL_NAMES, &mpig_debug_handle);

    MPIU_Free(levels_uc);
    if (timed_levels != NULL) MPIU_Free(timed_levels_uc);

    /* XXX: This is a hack to allow the MPIU_dbg_printf to send output to the current logging file */
    MPIUI_dbg_fp = mpig_debug_handle.file;
    MPIUI_dbg_state = MPIU_DBG_STATE_FILE;
	
    gettimeofday(&tv, NULL);
    mpig_debug_start_tv_sec = tv.tv_sec;
    
  fn_return:
    return;
}
/* mpig_debug_init */

#if defined(MPIG_DEBUG_REPORT_PGID)
#define mpig_debug_uvfprintf_macro(fp_, levels_, filename_, funcname_, line_, fmt_, ap_)				\
{															\
    char lfmt__[MPIG_DEBUG_TMPSTR_SIZE];										\
															\
    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)								\
    {															\
	MPIU_Snprintf(lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgid=%s:pgrank=%d:tid=%lu] %s(l=%d): %s\n",			\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_), (fmt_));	\
	vfprintf((fp_), lfmt__, (ap_));											\
    }															\
    else														\
    {															\
	struct timeval tv__;												\
															\
	gettimeofday(&tv__, NULL);											\
	tv__.tv_sec -= mpig_debug_start_tv_sec;										\
	MPIU_Snprintf(lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgid=%s:pgrank=%d:tid=%lu] %s(l=%d:t=%lu.%.6lu): %s\n",		\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_),			\
	    tv__.tv_sec, tv__.tv_usec, (fmt_));										\
	vfprintf((fp_), lfmt__, (ap_));											\
    }															\
}
#else
#define mpig_debug_uvfprintf_macro(fp_, levels_, filename_, funcname_, line_, fmt_, ap_)		\
{													\
    char lfmt__[MPIG_DEBUG_TMPSTR_SIZE];								\
													\
    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)						\
    {													\
	MPIU_Snprintf(lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgrank=%d:tid=%lu] %s(l=%d): %s\n",		\
	    mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_), (fmt_));		\
	vfprintf((fp_), lfmt__, (ap_));									\
    }													\
    else												\
    {													\
	struct timeval tv__;										\
													\
	gettimeofday(&tv__, NULL);									\
	tv__.tv_sec -= mpig_debug_start_tv_sec;								\
	MPIU_Snprintf(lfmt__, MPIG_DEBUG_TMPSTR_SIZE, "[pgrank=%d:tid=%lu] %s(l=%d:t=%lu.%.6lu): %s\n",	\
	    mpig_process.my_pg_rank, mpig_thread_get_id(), (funcname_), (line_),			\
	    tv__.tv_sec, tv__.tv_usec, (fmt_));								\
	vfprintf((fp_), lfmt__, (ap_));									\
    }													\
}
#endif

#if defined(HAVE_C99_VARIADIC_MACROS) || defined(HAVE_GNU_VARIADIC_MACROS)
#undef FUNCNAME
#define FUNCNAME mpig_debug_uprintf_fn
void mpig_debug_uprintf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    va_start(l_ap, fmt);
    mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
    va_end(l_ap);
}

#else /* the compiler does not support variadic macros */

globus_thread_once_t mpig_debug_create_state_key_once = GLOBUS_THREAD_ONCE_INIT;
globus_thread_key_t mpig_debug_state_key;

MPIG_STATIC void mpig_debug_destroy_state(void * state);

void mpig_debug_create_state_key(void)
{
    globus_thread_key_create(&mpig_debug_state_key, mpig_debug_destroy_state);
}

MPIG_STATIC void mpig_debug_destroy_state(void * state)
{
    MPIU_Free(state);
}

#undef FUNCNAME
#define FUNCNAME mpig_debug_printf_fn
void mpig_debug_printf_fn(unsigned levels, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    if (levels & mpig_debug_handle.levels)
    {
	mpig_debug_retrieve_state(&filename, &funcname, &line);
	va_start(l_ap, fmt);
	mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
	va_end(l_ap);
    }
}
/* mpig_debug_printf_fn() */


#undef FUNCNAME
#define FUNCNAME mpig_debug_uprintf_fn
void mpig_debug_uprintf_fn(unsigned levels, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    mpig_debug_retrieve_state(&filename, &funcname, &line);
    
    va_start(l_ap, fmt);
    mpig_debug_uvfprintf_macro(fp, levels, filename, funcname, line, fmt, l_ap);
    va_end(l_ap);
}
/* mpig_debug_uprintf_fn() */


#undef FUNCNAME
#define FUNCNAME mpig_debug_old_util_printf_fn
void mpig_debug_old_util_printf_fn(const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    FILE * fp = (mpig_debug_handle.file != NULL) ? mpig_debug_handle.file : stderr;
    const char * filename;
    const char * funcname;
    int line;
    va_list l_ap;

    MPIG_UNUSED_VAR(fcname);

    mpig_debug_retrieve_state(&filename, &funcname, &line);
    
    if (MPIG_DEBUG_LEVEL_MPI & mpig_debug_handle.levels)
    {
	va_start(l_ap, fmt);
	mpig_debug_uvfprintf_macro(fp, MPIG_DEBUG_LEVEL_MPI, filename, funcname, line, fmt, l_ap);
	va_end(l_ap);
    }
}
/* mpig_debug_old_util_printf_fn() */
#endif /* if variadic macros, else no variadic macros */

#endif /* defined(MPIG_DEBUG) */
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					    BEGIN MPI->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/
#define mpig_datatype_set_cmap(cmap_, dt_, ctype_)					\
{											\
    MPIU_Assert(MPID_Datatype_get_basic_id(dt_) < MPIG_DATATYPE_MAX_BASIC_TYPES);	\
    (cmap_)[MPID_Datatype_get_basic_id(dt_)] = (ctype_);				\
}


/*
 * <mpi_errno> mpig_datatype_set_my_bc([IN/MOD] bc)
 *
 * Paramters:
 *
 *   bc - [IN/MOD] business card to augment with datatype information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_set_my_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_set_my_bc(mpig_bc_t * const bc)
{
    mpig_vc_t * vc;
    int loc;
    mpig_ctype_t cmap[MPIG_DATATYPE_MAX_BASIC_TYPES];
    char cmap_str[MPIG_DATATYPE_MAX_BASIC_TYPES + 1];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_set_my_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_set_my_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA, "entering: bc=" MPIG_PTR_FMT,
	(MPIG_PTR_CAST) bc));

    mpig_pg_rc_acq(mpig_process.my_pg, TRUE);
    {
	mpig_pg_get_vc(mpig_process.my_pg, mpig_process.my_pg_rank, &vc);
    }
    mpig_pg_rc_rel(mpig_process.my_pg, FALSE);
    
    /* create mapping information and store in my VC */
    for (loc = 0; loc < MPIG_DATATYPE_MAX_BASIC_TYPES; loc++)
    {
	vc->dt_cmap[loc] = 0;
    }

    mpig_datatype_set_cmap(cmap, MPI_BYTE, MPIG_CTYPE_CHAR);
    mpig_datatype_set_cmap(cmap, MPI_CHAR, MPIG_CTYPE_CHAR);
    /* XXX: ... */

    /* prepare a text version of the mappings */
    MPIU_Assertp(MPIG_CTYPE_LAST <= 16);
    for (loc = 0; loc < MPIG_DATATYPE_MAX_BASIC_TYPES; loc++)
    {
	cmap_str[loc] = '0' + (char) vc->dt_cmap[loc];
    }
    cmap_str[MPIG_DATATYPE_MAX_BASIC_TYPES] = '\0';

    /* add mappings to the business card */
    mpi_errno = mpig_bc_add_contact(bc, "DATATYPE_CMAP", cmap_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|datatype_cmap_to_bc");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA, "exiting: bc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT,  (MPIG_PTR_CAST) bc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_set_my_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_datatype_set_my_bc() */


/*
 * <mpi_errno> mpig_datatype_process_bc([IN] bc, [IN/MOD] vc)
 *
 * Paramters:
 *
 *   bc - [IN/MOD] business card from which to extract datatype information
 *
 *   vc - [IN/MOD] virtual connection to augment with extracted information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_process_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_process_bc(const mpig_bc_t * const bc, mpig_vc_t * const vc)
{
    char * cmap_str = NULL;
    int loc;
    char * strp;
    bool_t found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_process_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_process_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA, "entering: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) bc, (MPIG_PTR_CAST) vc));

    mpi_errno = mpig_bc_get_contact(bc, "DATATYPE_CMAP", &cmap_str, &found);
    MPIU_ERR_CHKANDJUMP2((mpi_errno || found == FALSE), mpi_errno, MPI_ERR_INTERN, "**globus|datatype_bc_to_cmap",
	"**globus|datatype_cmap %s %d", mpig_vc_get_pg(vc), mpig_vc_get_pg_rank(vc));

    MPIU_Assert(strlen(cmap_str) <= MPIG_DATATYPE_MAX_BASIC_TYPES);
    
    loc = 0;
    strp = cmap_str;
    while (*strp != '\0')
    {
	unsigned u;
	
	sscanf(strp, "%1x", &u);
	vc->dt_cmap[loc] = (unsigned char) u;
	strp += 1;
    }
    
  fn_return:
    if (cmap_str != NULL) mpig_bc_free_contact(cmap_str);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA, "exiting: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) bc, (MPIG_PTR_CAST) vc, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_process_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_datatype_process_bc() */

/**********************************************************************************************************************************
					     END MPI->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR ROUTINES
**********************************************************************************************************************************/
/*
 * void mpig_iov_unpack_fn([IN] buf, [IN] size, [IN/MOD] iov)
 *
 * copy data from the buffer into the location(s) specified by the IOV
 *
 * buf [IN] - pointer to the data buffer
 * buf_size [IN] - amount of data in the buffer
 * iov [IN/MOD] - I/O vector describing the buffer(s) in which to unpack the data
 */
#undef FUNCNAME
#define FUNCNAME mpig_iov_unpack_fn
#undef FCNAME
#define FCNAME fcname
MPIU_Size_t mpig_iov_unpack_fn(const void * const buf, const MPIU_Size_t buf_size, mpig_iov_t * const iov)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char * ptr = (const char *) buf;
    MPIU_Size_t nbytes = buf_size;
    MPIG_STATE_DECL(MPID_STATE_mpig_iov_unpack_fn);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_iov_unpack_fn);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: buf=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT ", iov=" MPIG_PTR_FMT,
		       (MPIG_PTR_CAST) buf, buf_size, (MPIG_PTR_CAST) iov));
    
    while (nbytes > 0 && iov->cur_entry < iov->free_entry)
    {
	MPID_IOV * iov_entry = &iov->iov[iov->cur_entry];

	if (iov_entry->MPID_IOV_LEN <= nbytes)
	{
	    memcpy((void *) iov_entry->MPID_IOV_BUF, (void *) ptr, iov_entry->MPID_IOV_LEN);

	    iov->cur_entry += 1;
	    iov->num_bytes -= iov_entry->MPID_IOV_LEN;

	    ptr += iov_entry->MPID_IOV_LEN;
	    nbytes -= iov_entry->MPID_IOV_LEN;

	    iov_entry->MPID_IOV_BUF = NULL;
	    iov_entry->MPID_IOV_LEN = 0;
	}
	else
	{
	    memcpy((void *) iov_entry->MPID_IOV_BUF, (void *) ptr, nbytes);

	    iov->num_bytes -= nbytes;

	    iov_entry->MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *) (iov_entry->MPID_IOV_BUF) + nbytes);
	    iov_entry->MPID_IOV_LEN -= nbytes;
	    
	    ptr += nbytes;
	    nbytes = 0;
	}
    }

    if (iov->num_bytes == 0)
    {
	iov->cur_entry = 0;
	iov->free_entry = 0;
    }
    else if (iov->cur_entry > 0)
    {
	int n = 0;

	while(iov->cur_entry < iov->free_entry)
	{
	    iov->iov[n++] = iov->iov[iov->cur_entry++];
	}

	iov->cur_entry = 0;
	iov->free_entry = n;
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: buf=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT ", iov=" MPIG_PTR_FMT ", unpacked="
		       MPIG_SIZE_FMT, (MPIG_PTR_CAST) buf, buf_size, (MPIG_PTR_CAST) iov, buf_size - nbytes));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_iov_unpack_fn);
    return buf_size - nbytes;
}
/* mpig_iov_unpack_fn() */
/**********************************************************************************************************************************
						     END I/O VECTOR ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					      BEGIN DATA BUFFER MANAGEMENT ROUTINES
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_databuf_create([IN] size, [OUT] dbufp)
 *
 * size [IN] - size of intermediate buffer
 * dbufp [OUT] - pointer to new data buffer object
 */
#undef FUNCNAME
#define FUNCNAME mpig_databuf_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_databuf_create(const MPIU_Size_t size, mpig_databuf_t ** const dbufp)
{
    mpig_databuf_t * dbuf;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_databuf_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_databuf_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "entering: dbufp=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT,
		       (MPIG_PTR_CAST) dbufp, size));
    
    dbuf = (mpig_databuf_t *) MPIU_Malloc(sizeof(mpig_databuf_t) + size);
    MPIU_ERR_CHKANDJUMP1((dbuf == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "intermediate data buffer");

    mpig_databuf_construct(dbuf, size);

    *dbufp = dbuf;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF, "exiting: dbufp=" MPIG_PTR_FMT ", dbuf=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) dbufp, (MPIG_PTR_CAST) dbuf, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_databuf_create);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}


/*
 * void mpig_databuf_destroy([IN/MOD] dbuf)
 *
 * dbuf [IN/MOD] - pointer to data buffer object to destroy
 */
#undef FUNCNAME
#define FUNCNAME mpig_databuf_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_databuf_destroy(mpig_databuf_t * const dbuf)
{
    MPIG_STATE_DECL(MPID_STATE_mpig_databuf_destroy);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_databuf_destroy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "entering: dbuf=" MPIG_PTR_FMT, (MPIG_PTR_CAST) dbuf));

    mpig_databuf_destruct(dbuf);
    MPIU_Free(dbuf);

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "exiting: dbuf=" MPIG_PTR_FMT, (MPIG_PTR_CAST) dbuf));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_databuf_destroy);
    return;
}
/**********************************************************************************************************************************
					       END DATA BUFFER MANAGEMENT ROUTINES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATION MODULE ROUTINES
**********************************************************************************************************************************/
/*
 * void mpig_cm_vtable_last_entry(none)
 *
 * this routine serves as the last function in the CM virtual table.  its purpose is to help detect when a communication
 * module's vtable has not be updated when a function be added or removed from the table.  it is not fool proof as it requires
 * the type signature not to match, but there should be few (if any) routines with such a type signature.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vtable_last_entry
char * mpig_cm_vtable_last_entry(int foo, float bar, const short * baz, char bif)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vtable_last_entry);

    MPIG_UNUSED_ARG(foo);
    MPIG_UNUSED_ARG(bar);
    MPIG_UNUSED_ARG(baz);
    MPIG_UNUSED_ARG(bif);
    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vtable_last_entry);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: mpig_cm_vtable_last_entry called.  aborting program"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vtable_last_entry);
    MPID_Abort(NULL, MPI_SUCCESS, 13, "FATAL ERROR: mpig_cm_vtable_last_entry called.  Aborting Program.");
    return NULL;
}
/**********************************************************************************************************************************
						END COMMUNICATION MODULE ROUTINES
**********************************************************************************************************************************/

/**********************************************************************************************************************************
					       BEGIN USAGE STAT ROUTINES
**********************************************************************************************************************************/


/*
 *  base64 encode a string, string may not be null terminated
 *
 */
static void
mpig_usage_base64_encode(
    const unsigned char *               inbuf,
    int                                 in_len,
    unsigned char *                     outbuf,
    int *                               out_len)
{
    int                                 i;
    int                                 j;
    unsigned char                       c;
    char                                padding = '=';
    const char *                              base64_charset =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (i=0,j=0; i < in_len; i++)
    {
        switch (i%3)
        {
            case 0:
                outbuf[j++] = base64_charset[inbuf[i]>>2];
                c = (inbuf[i]&3)<<4;
                break;
            case 1:
                outbuf[j++] = base64_charset[c|inbuf[i]>>4];
                c = (inbuf[i]&15)<<2;
                break;
            case 2:
                outbuf[j++] = base64_charset[c|inbuf[i]>>6];
                outbuf[j++] = base64_charset[inbuf[i]&63];
                c = 0;
                break;
            default:
                globus_assert(0);
                break;
        }
    }
    if (i%3)
    {
        outbuf[j++] = base64_charset[c];
    }
    switch (i%3)
    {
        case 1:
            outbuf[j++] = padding;
        case 2:
            outbuf[j++] = padding;
    }

    outbuf[j] = '\0';
    *out_len = j;

    return;
}




/*
 * void mpig_usage_finalize(none)
 *
 */

#define MPIG_USAGE_ID 8
#define MPIG_USAGE_PACKET_VERSION 0

#define MPICH2VERSION "1.0.3" /* XXX get this from autoconf */

#undef FUNCNAME
#define FUNCNAME mpig_usage_finalize
void mpig_usage_finalize(void)
{
    globus_usage_stats_handle_t mpig_usage_handle;
    int rc;
    globus_result_t result;
    struct timeval end_time;
    globus_off_t * total_nbytes;
    globus_off_t * total_nbytesv;
    int i;
    char ver_b[32];
    char start_b[32];
    char end_b[32];
    char nprocs_b[32];
    char test_b[32];
    char nbytesv_b[32];
    char nbytes_b[32];
    char fnmap_b[4096];
    int fnmap_b_len;
    unsigned char fnmap[MPIG_FUNC_CNT_NUMFUNCS * 2 * sizeof(int)];
    unsigned char * ptr;
    int total_function_count[MPIG_FUNC_CNT_NUMFUNCS] = { 0 };

 
    if(mpig_process.my_pg_rank == 0)
    {
        
        total_nbytes = (globus_off_t *) 
            globus_malloc(mpig_process.my_pg_size * sizeof(globus_off_t));
        total_nbytesv = (globus_off_t *) 
            globus_malloc(mpig_process.my_pg_size * sizeof(globus_off_t));
    }
/*
    MPIR_Nest_incr();
    MPI_Gather(
        &mpig_process.nbytes_sent, 8, MPI_BYTE, 
        &total_nbytes, 8, MPI_BYTE, 
        0, MPI_COMM_WORLD);
    MPIR_Nest_decr();

    MPIR_Nest_incr();
    MPI_Gather(
        &mpig_process.vendor_nbytes_sent, 8, MPI_BYTE, 
        &total_nbytesv, 8, MPI_BYTE, 
        0, MPI_COMM_WORLD);
    MPIR_Nest_decr();
*/
    MPIR_Nest_incr();
    MPI_Reduce(
        mpig_process.function_count, total_function_count, 
        MPIG_FUNC_CNT_NUMFUNCS, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPIR_Nest_decr();

    if(mpig_process.my_pg_rank == 0)
    {

        mpig_process.nbytes_sent = 0; 
        for(i = 0; i < mpig_process.my_pg_size; i++)
        {
            mpig_process.nbytes_sent += total_nbytes[i];
        }

        mpig_process.vendor_nbytes_sent = 0; 
        for(i = 0; i < mpig_process.my_pg_size; i++)
        {
            mpig_process.vendor_nbytes_sent += total_nbytesv[i];
        }

        globus_free(total_nbytes);
        globus_free(total_nbytesv);

        gettimeofday(&end_time, NULL);
        
        rc = globus_module_activate(GLOBUS_USAGE_MODULE);
        if(rc != 0)
        {
            goto err;
        }
        
        result = globus_usage_stats_handle_init(
            &mpig_usage_handle, 
            MPIG_USAGE_ID, 
            MPIG_USAGE_PACKET_VERSION, 
            "localhost:9999");      
        if(result != GLOBUS_SUCCESS)
        {
            globus_module_deactivate(GLOBUS_USAGE_MODULE);
            goto err;
        }

        /* will need to encode these into our own buffer in order to fit
        the function map */

        snprintf(ver_b, sizeof(ver_b), MPIG_MPICH2_VERSION);
        snprintf(start_b, sizeof(start_b), "%d.%d", 
            (int) mpig_process.start_time.tv_sec, (int) mpig_process.start_time.tv_usec);
        snprintf(end_b, sizeof(end_b), "%d.%d", 
            (int) end_time.tv_sec, (int) end_time.tv_usec);
        snprintf(nprocs_b, sizeof(nprocs_b), "%d", 
            mpig_process.my_pg_size);
        snprintf(test_b, sizeof(test_b), "%s", 
            getenv("MPIG_TEST") ? "1" : "0");
        snprintf(nbytesv_b, sizeof(nbytesv_b), "%"GLOBUS_OFF_T_FORMAT, 
            mpig_process.vendor_nbytes_sent);
        snprintf(nbytes_b, sizeof(nbytes_b), "%"GLOBUS_OFF_T_FORMAT, 
            mpig_process.nbytes_sent);
        
        
        /* write out the function counts, then base64 encode that buffer.
         * max size of the binary buffer (8 bytes for each function) is about
         * 1800 bytes, which gives ~2400 bytes in base64... We have about 1300
         * bytes in the usage packet to play with, so we can handle ~120
         * unique function calls.  If we can rely on the total count of 
         * functions staying under 255 (currently 241), we can shave it down 
         * to 6 bytes per function if needed, and then we'd be able to handle 
         * ~160 different calls in a given app.  If we care about more than 
         * that we'll need to get smarter with the encoding (compression)
         * or just add binary support to the c usage lib.
         */
         
        memset(fnmap, 0, sizeof(fnmap));
        ptr = fnmap;
        for(i = 0; i < MPIG_FUNC_CNT_NUMFUNCS; i++)
        {
            if(total_function_count[i] > 0)
            {
                memcpy(ptr, &i, sizeof(int));
                ptr += sizeof(int);
                memcpy(ptr, &total_function_count[i], sizeof(int));
                ptr += sizeof(int);
            }
        }

        mpig_usage_base64_encode(fnmap, ptr - fnmap, fnmap_b, &fnmap_b_len);
        
        
        result = globus_usage_stats_send(
            mpig_usage_handle,
            8,
            "MPICHVER", ver_b,
            "START", start_b,
            "END", end_b,
            "NPROCS", nprocs_b,
            "NBYTES", nbytes_b,
            "NBYTESV", nbytesv_b,
            "TEST", test_b,
            "FNMAP", fnmap_b);
        if(result != GLOBUS_SUCCESS)
        {
            /* debug output */
        }
        
        globus_usage_stats_handle_destroy(mpig_usage_handle);
    
    }
    return;
        
err:
    return;
}

/**********************************************************************************************************************************
						END USAGE STAT ROUTINES
**********************************************************************************************************************************/
