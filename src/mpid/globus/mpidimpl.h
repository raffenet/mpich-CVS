/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDIMPL_H_INCLUDED)
#define MPICH2_MPIDIMPL_H_INCLUDED


#include "mpiimpl.h"
#include "globus_module.h"
#include "globus_common.h"


/*
 * When memory tracing is enabled, mpimem.h redefines malloc(), calloc(), and free() to be invalid statements.  These
 * redefinitions have a negative interaction with the Globus heap routines which may be mapped (using C preprocessor defines)
 * directly to the heap routines supplied as part of the the operating system.  This is particularly a problem when a Globus
 * library routine returns an allocated object and expects to caller to free the object.  One cannot use MPIU_Free() to free the
 * object since MPIU_Free() is expecting a pointer to memory allocated by the memory tracing module.  Therefore, it is necessary
 * to remove the redefinitions of the heap routines, allowing the Globus heap routines to map directly to those provided by the
 * operating system.
 */
#if defined(USE_MEMORY_TRACING)
#undef malloc
#undef calloc
#undef free
#endif

/*
 * Function enter/exit macro, used primarily for logging, but can be used for other things.
 */
#define MPIG_STATE_DECL(a_) MPIDI_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIDI_FUNC_ENTER(a_)
#define MPIG_FUNC_EXIT(a_) MPIDI_FUNC_EXIT(a_)
#define MPIG_RMA_FUNC_ENTER(a_) MPIDI_RMA_FUNC_ENTER(a_)
#define MPIG_RMA_FUNC_EXIT(a_) MPIDI_RMA_FUNC_EXIT(a_)


#define MPIG_QUOTE(a_) MPIG_QUOTE2(a_)
#define MPIG_QUOTE2(a_) #a_


#if defined(NDEBUG)
#define MPIG_STATIC static
#else
#define MPIG_STATIC
#endif


#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif


/*>>>>>>>>>>>>>>>>>>>>
  PROCESS DATA SECTION
  >>>>>>>>>>>>>>>>>>>>*/
/*
 * The value 128 is returned by the echomaxprocname target in src/mpid/globus/Makefile.sm.  If the value is modified here, it
 * also needs to be modified in Makefile.sm.
 */
#if !defined(MPIG_PROCESSOR_NAME_SIZE)
#   define MPIG_PROCESSOR_NAME_SIZE 128
#endif

typedef struct mpig_process
{
    /* Pointer to the the process group to which this process belongs, the size of the process group, and the rank of the
       processs within the process group */
    struct mpig_pg * my_pg;
    const char * my_pg_id;
    int my_pg_size;
    int my_pg_rank;

    /* The sizeof of the subjob to which this process belongs, and the rank of the process within the subjob */
    int my_sj_size;
    int my_sj_rank;
    
    /* Local process ID counter for assigning a local ID to each virtual connection.  A local ID is necessary for the MPI_Group
       routines, implemented at the MPICH layer, to function properly.  MT: this requires a thread safe fetch-and-increment */
    int lpid_counter;

#if defined(MPIG_VMPI)
#define MPIG_VMPI_COMM_WORLD (&mpig_process.vmpi_cw)
    mpig_vmpi_comm_t vmpi_cw;
    int vmpi_cw_size;
    int vmpi_cw_rank;
#endif

    char hostname[MPIG_PROCESSOR_NAME_SIZE];
    pid_t pid;
}
mpig_process_t;

extern mpig_process_t mpig_process;

/* XXX: MT: needs to be made thread safe */
#define MPIDI_LPID_get_next(lpid_)		\
{						\
    *(lpid_) = mpig_process.lpid_counter++;	\
}
/*<<<<<<<<<<<<<<<<<<<<
  PROCESS DATA SECTION
  <<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>
  DATATYPE SECTION
  >>>>>>>>>>>>>>>>*/
#define mpig_datatype_get_info(count_, dt_, dt_contig_, data_sz_, dt_ptr_, dt_true_lb_)			\
{													\
    if (HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN)							\
    {													\
	*(dt_ptr_) = NULL;										\
	*(dt_contig_) = TRUE;										\
        *(dt_true_lb_) = 0;										\
	*(data_sz_) = (count_) * MPID_Datatype_get_basic_size(dt_);					\
	MPIG_DBG_PRINTF((15, FCNAME, "basic datatype: dt_contig=%d, dt_sz=%d, data_sz=" MPIG_AINT_FMT,	\
			  *(dt_contig_), MPID_Datatype_get_basic_size(dt_), *(data_sz_)));		\
    }													\
    else												\
    {													\
	MPID_Datatype_get_ptr((dt_), *(dt_ptr_));							\
	*(dt_contig_) = (*(dt_ptr_))->is_contig;							\
	*(data_sz_) = (count_) * (*(dt_ptr_))->size;							\
        *(dt_true_lb_) = (*(dt_ptr_))->true_lb;								\
	MPIG_DBG_PRINTF((15, FCNAME, "user defined dt: dt_contig=%d, dt_sz=%d, data_sz=" MPIG_AINT_FMT,	\
			  *(dt_contig_), (*(dt_ptr_))->size, *(data_sz_)));				\
    }													\
}
/*<<<<<<<<<<<<<<<<
  DATATYPE SECTION
  <<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>>>
  PROCESS MANAGEMENT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>>>*/
int mpig_pm_init(void);

int mpig_pm_finalize(void);

int mpig_pm_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs);

int mpig_pm_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_get_pg_size(int * pg_size);

int mpig_pm_get_pg_rank(int * pg_rank);

int mpig_pm_get_pg_id(const char ** pg_id);
/*<<<<<<<<<<<<<<<<<<<<<<<<<<
  PROCESS MANAGEMENT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>>
  DEBUGGING OUTPUT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>*/
void mpig_dbg_printf(int level, const char * fcname, const char * fmt, ...);
void mpig_dbg_vprintf(int level, const char * fcname, const char * fmt, va_list ap);

#if defined(MPICH_DBG_OUTPUT)
#define MPIG_DBG_PRINTF(e_)			\
{						\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)	\
    {						\
        mpig_dbg_printf e_;			\
    }						\
}
#define MPIG_DBG_VPRINTF(level_, fcname_, fmt_, ap_)		\
{								\
    if (MPIUI_dbg_state != MPIU_DBG_STATE_NONE)			\
    {								\
        mpig_dbg_vprintf((level_), (fcname_), (fmt_), (ap_));	\
    }								\
}
#else
#define MPIG_DBG_PRINTF(e_)
#define MPIG_DBG_VPRINTF(e_)
#endif

#if defined(HAVE_CPP_VARARGS)
#define mpig_dbg_printf(level_, func_, fmt_, args_...)										  \
{																  \
    MPIU_dbglog_printf("[%s:%d:%d] %s():%d: " fmt_ "\n", mpig_process.my_pg_id, mpig_process.my_pg_rank, 0 /* MT: thread id */,   \
		       (func_), (level_), ## args_);										  \
}
#endif
/*<<<<<<<<<<<<<<<<<<<<<<<<
  DEBUGGING OUTPUT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<*/

#endif /* MPICH2_MPIDIMPL_H_INCLUDED */
