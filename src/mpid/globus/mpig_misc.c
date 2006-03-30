/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)
#define MPIG_DEBUG_TMPSTR_SIZE 1024U

globus_debug_handle_t mpig_debug_handle;
time_t mpig_debug_start_tv_sec;

/*
 * Not all systems support va_copy().  These definitions were taken from mpich2/src/util/dbg/dbg_printf.c.  They rely on the
 * top-level configure script to test for va_copy() and __va_copy().
 */
#ifdef HAVE_VA_COPY
# define va_copy_end(a) va_end(a)
#else
# ifdef HAVE___VA_COPY
#  define va_copy(a,b) __va_copy(a,b)
#  define va_copy_end(a)
# else
#  define va_copy(a,b) ((a) = (b))
/* Some writers recommend define va_copy(a,b) memcpy(&a,&b,sizeof(va_list)) */
#  define va_copy_end(a)
# endif
#endif

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
    char * timed_levels_uc;
    char settings[MPIG_DEBUG_TMPSTR_SIZE];
    const char * file_basename;
    struct timeval tv;

    levels = getenv("MPIG_DEBUG_LEVELS");
    if (levels == NULL || strlen(levels) == 0) goto fn_return;
    levels_uc = MPIU_Strdup(levels);
    MPIG_UPPERCASE_STR(levels_uc);
    
    file_basename = getenv("MPIG_DEBUG_FILE_BASENAME");
    if (file_basename != NULL)
    {
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "%s-%s-%d.log",
		      file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
	setenv("MPICH_DBG_FILENAME", settings);
	
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,#%s-%s-%d.log,0",
		      levels_uc, file_basename, mpig_process.my_pg_id, mpig_process.my_pg_rank);
    }
    else
    {
	/* send debugging output to stderr */
	setenv("MPICH_DBG_FILENAME", "-stderr-");
	MPIU_Snprintf(settings, MPIG_DEBUG_TMPSTR_SIZE, "ERROR|%s,,0", levels_uc);
    }
    
    timed_levels = getenv("MPIG_DEBUG_TIMED_LEVELS");
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


#if !defined(HAVE_C99_VARIADIC_MACROS) && !defined(HAVE_GNU_VARIADIC_MACROS)
/*
 * void mpig_debug_printf([IN] handle, [IN] func, [IN] line, [IN] fmt, [IN] ...)
 *
 * Paramters:
 *
 * handle[IN] - debugging handle
 * func [IN] - name of calling function
 * line [IN] - line number of debug statement
 * fmt [IN] - format string (see man page for printf)
 * ... [IN] - arguments matching substitutions in the format string
 *
 * Returns: (nothing)
 */
#undef mpig_debug_printf
#undef FUNCNAME
#define FUNCNAME mpig_debug_printf
void mpig_debug_printf(int levels, const char * fmt, ...)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char lfmt[MPIG_DEBUG_TMPSTR_SIZE];
    const char * func;
    int line;
    va_list ap;

    MPIG_UNUSED_VAR(fcname);

    /* XXX: get function name and line numbers from thread specific storage */
    func = "";
    line = -1;
    
    if (levels & mpig_debug_handle.levels)
    {
	if (mpig_debug_handle.file != NULL)
	{
	    if (levels & mpig_debug_handle.timestamp_levels == 0)
	    {
		va_start(ap, fmt);
		MPIU_Snprintf(lfmt, MPIG_DEBUG_TMPSTR_SIZE, "[%s:%d:%lu] %s(l=%d) %s\n", mpig_process.my_pg_id,
			      mpig_process.my_pg_rank, mpig_thread_get_id(), func, line, fmt);
		vfprintf(mpig_debug_handle.file, lfmt, ap);
		va_end(ap);
	    }
	    else
	    {
		struct timeval tv;
		
		gettimeofday(&tv, NULL);
		tv.tv_sec -= mpig_debug_start_tv_sec;
		va_start(ap, fmt);
		MPIU_Snprintf(lfmt, MPIG_DEBUG_TMPSTR_SIZE, "[%s:%d:%lu] %s(l=%d:t=%lu.%.6lu) %s\n", mpig_process.my_pg_id,
			      mpig_process.my_pg_rank, mpig_thread_get_id(), func, line, tv.tv_sec, tv.tv_usec, fmt);
		vfprintf(mpig_debug_handle.file, lfmt, ap);
		va_end(ap);
	    }
	}
    }
}
/* mpig_debug_printf() */
#endif /* compiler does not suppport variadic macros */
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
 * int mpig_datatype_set_my_bc([IN/MOD] bc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Paramters:
 *
 * bc - [IN/MOD] business card to augment with datatype information
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_set_my_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_datatype_set_my_bc(mpig_bc_t * const bc, int * const mpi_errno_p, bool_t * const failed_p)
{
    mpig_vc_t * vc;
    int loc;
    mpig_ctype_t cmap[MPIG_DATATYPE_MAX_BASIC_TYPES];
    char cmap_str[MPIG_DATATYPE_MAX_BASIC_TYPES + 1];
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_set_my_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_set_my_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x", (MPIG_PTR_CAST) bc, *mpi_errno_p));

    *failed_p = FALSE;
    
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
    mpig_bc_add_contact(bc, "DATATYPE_CMAP", cmap_str, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|datatype_cmap_to_bc");
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_set_my_bc);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_datatype_set_my_bc() */


/*
 * int mpig_datatype_process_bc([IN] bc, [IN/MOD] vc, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * Paramters:
 *
 * bc - [IN/MOD] business card from which to extract datatype information
 * vc - [IN/MOD] virtual connection to augment with extracted information
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_process_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_datatype_process_bc(const mpig_bc_t * const bc, mpig_vc_t * const vc, int * const mpi_errno_p, bool_t * const failed_p)
{
    char * cmap_str = NULL;
    int loc;
    char * strp;
    bool_t found;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_process_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_process_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: bc=" MPIG_PTR_FMT "vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       (MPIG_PTR_CAST) bc, (MPIG_PTR_CAST) vc, *mpi_errno_p));

    *failed_p = FALSE;
    
    mpig_bc_get_contact(bc, "DATATYPE_CMAP", &cmap_str, &found, mpi_errno_p, &failed);
    MPIU_ERR_CHKANDJUMP2((failed || found == FALSE), *mpi_errno_p, MPI_ERR_INTERN, "**globus|datatype_bc_to_cmap",
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
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: bc=" MPIG_PTR_FMT "vc=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) bc, (MPIG_PTR_CAST) vc, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_process_bc);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
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
 * void mpig_databuf_create([IN] size, [OUT] dbufp, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * size [IN] - size of intermediate buffer
 * dbufp [OUT] - pointer to new data buffer object
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_databuf_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_databuf_create(const MPIU_Size_t size, mpig_databuf_t ** const dbufp, int * const mpi_errno_p, bool_t * const failed_p)
{
    mpig_databuf_t * dbuf;
    MPIG_STATE_DECL(MPID_STATE_mpig_databuf_create);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_databuf_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "entering: dbufp=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT ", mpi_errno=0x%08x",
		       (MPIG_PTR_CAST) dbufp, size, *mpi_errno_p));
    
    *failed_p = FALSE;
    
    dbuf = (mpig_databuf_t *) MPIU_Malloc(sizeof(mpig_databuf_t) + size);
    MPIU_ERR_CHKANDJUMP1((dbuf == NULL), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "intermediate data buffer");

    mpig_databuf_construct(dbuf, size);

    *dbufp = dbuf;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATABUF,
		       "exiting: dbufp=" MPIG_PTR_FMT ", dbuf=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       (MPIG_PTR_CAST) dbufp, (MPIG_PTR_CAST) dbuf, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_databuf_create);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    *failed_p = TRUE;
    goto fn_return;
    /* --END ERROR HANDLING-- */
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
