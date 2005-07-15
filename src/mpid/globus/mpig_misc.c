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
#define LOCAL_FMT_SZ 256

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


/*
 * void mpig_dbg_printf([IN] level, [IN] func, [IN] fmt, [IN] ...)
 *
 * Paramters:
 *
 * level [IN] - debugging level (currently not used)
 * func [IN] - name of calling function
 * fmt [IN] - format string (see man page for printf)
 * ... [IN] - arguments matching substitutions in the format string
 *
 * Returns: (nothing)
 */
#undef mpig_dbg_printf
#undef FUNCNAME
#define FUNCNAME mpig_dbg_printf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_dbg_printf(int level, const char * func, const char * fmt, ...)
{
    char lfmt[LOCAL_FMT_SZ];
    va_list list;

    va_start(list, fmt);
    MPIU_Snprintf(lfmt, LOCAL_FMT_SZ, "[%s:%d:%d] %s():%d: %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank,
		  0 /* MT: thread id */, func, level, fmt);
    MPIU_dbglog_vprintf(lfmt, list);
    fflush(stdout);
    va_end(list);
}
/* mpig_dbg_printf() */

/*
 * void mpig_dbg_printf([IN] level, [IN] func, [IN], fmt, [IN] va)
 *
 * Paramters:
 *
 * level - [IN] debugging level (currently not used)
 * func - [IN] name of calling function
 * va - [IN] variable argument list containging parameter matching substitutions in the format string
 *
 * Returns: (nothing)
 */
#undef mpig_dbg_vprintf
#undef FUNCNAME
#define FUNCNAME mpig_dbg_vprintf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_dbg_vprintf(int level, const char * func, const char * fmt, va_list ap)
{
    char lfmt[LOCAL_FMT_SZ];
    va_list list;
    
    va_copy(list, ap);
    MPIU_Snprintf(lfmt, LOCAL_FMT_SZ, "[%s:%d:%d] %s(): %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank,
		  0 /* MT: thread id */, func, fmt);
    MPIU_dbglog_vprintf(lfmt, list);
    fflush(stdout);
    va_copy_end(list);
}
/* mpig_dbg_printf() */

/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					    BEGIN FORTRAN->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/
/*
 * int mpig_datatype_set_my_bc([IN/OUT] bc)
 *
 * Paramters:
 *
 * bc - [IN/OUT] business card to augment with datatype information
 *
 * Returns: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_set_my_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_set_my_bc(mpig_bc_t * const bc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_set_my_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_set_my_bc);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_set_my_bc);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_datatype_set_my_bc() */

/*
 * int mpig_datatype_process_bc([IN] bc, [IN/OUT] vc)
 *
 * Paramters:
 *
 * bc - [IN/OUT] business card from which to extract datatype information
 * vc - [IN/OUT] virtual connection to augment with extracted information
 *
 * Returns: MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_process_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_datatype_process_bc(const mpig_bc_t * const bc, mpig_vc_t * const vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_process_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_process_bc);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_process_bc);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_datatype_process_bc() */

/*
 * void mpig_datatype_process_bc([IN] vc, [IN] ftype, [OUT] ctype)
 *
 * This routine returns the C datatype associated with the supplied Fortran datatype at the remote process.
 *
 * Paramters:
 *
 * vc - [IN/OUT] virtual connection
 * ftype - [IN] Fortran datatype
 * ctype - [OUT] C datatype 
 *
 * Returns: (nothing)
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_process_bc
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_datatype_get_src_ctype(const mpig_vc_t * const vc, const mpig_ftype_t ftype, mpig_ctype_t * const ctype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_process_bc);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_process_bc);
    MPIG_DBG_PRINTF((10, FCNAME, "entering"));

  fn_return:
    MPIG_DBG_PRINTF((10, FCNAME, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_process_bc);
    return;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/**********************************************************************************************************************************
					     END FORTRAN->C DATATYPE MAPPING SECTION
**********************************************************************************************************************************/
