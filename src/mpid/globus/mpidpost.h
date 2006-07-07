/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPOST_H_INCLUDED)
#define MPICH2_MPIDPOST_H_INCLUDED

#include "mpiallstates.h"

/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
int mpig_comm_construct(MPID_Comm * comm);

int mpig_comm_destruct(MPID_Comm * comm);

int mpig_comm_free_hook(MPID_Comm * comm);

#define MPID_Dev_comm_create_hook(comm_)		\
{							\
    int mrc__;						\
    mrc__ = mpig_comm_construct(comm_);			\
    if (mrc__) MPID_Abort(NULL, mrc__, 13, NULL);	\
}

#define MPID_Dev_comm_destroy_hook(comm_)		\
{							\
    int mrc__;						\
    mrc__ = mpig_comm_destruct(comm_);			\
    if (mrc__) MPID_Abort(NULL, mrc__, 13, NULL);	\
}

#define MPID_Dev_comm_free_hook(comm_)			\
{							\
    int mrc__;						\
    mrc__ = mpig_comm_free_hook(comm_);			\
    if (mrc__) MPID_Abort(NULL, mrc__, 13, NULL);	\
}

#if defined(MPIG_VMPI)

int mpig_comm_dup_hook(MPID_Comm * orig_comm, MPID_Comm * new_comm);
#define MPID_Dev_comm_dup_hook(orig_comm_, new_comm_, mpi_errno_p_)	\
    mpig_comm_free_hook((orig_comm_), (new_comm_), (mpi_errno_p_))

int mpig_intercomm_create_hook(MPID_Comm * local_comm, int local_leader, MPID_Comm * peer_comm, int remote_leader, int tag,
    MPID_Comm * new_intercomm);
#define MPID_Dev_intercomm_create_hook(orig_comm_, new_comm_, mpi_errno_p_)	\
    mpig_intercomm_create_hook((orig_comm_), (new_comm_), (mpi_errno_p_))

#endif /* defined(MPIG_VMPI) */

/*
 * MT-RC-NOTE: this routine does not perform insure the pointer to the function table in the VC and the the pointers within the
 * function table table are consistent across are threads.  as such, it is the responsibily of the calling routine to insure the
 * VC and the table to which it points is up-to-date before calling this routine.
 *
 * MT-RC-FIXME: give the use of this routine in the ADI3 macros below, it may be necessary for this routine to become a function
 * that performs an RC acquire on platforms that require it.  at present, this is not necessary since the function table pointer
 * is set in the main thread, the same thread in all of the MPI routines are called (hence the current MPI_THREAD_SINGLE
 * restriction).
 */
#define mpig_comm_get_vc_vtable(comm_, rank_) (((rank_) >= 0) ? ((comm_)->vcr[(rank_)]->vtable) : mpig_cm_other_vc->vtable)
/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
/*
 * NOTE: the setting, incrementing and decrementing of the request completion counter must be atomic since the progress engine
 * could be completing the request in one thread and the user could be cancelling the request in another thread.  if atomic
 * instructions are used, a write barrier or store-release may be needed to insure that updates to other fields in the request
 * structure are made visible before the completion counter is updated.  the exact operations required depending the on the
 * memory model of the processor architecture.
 */

MPID_Request * mpig_request_create(void);

void mpig_request_destroy(MPID_Request * req);

/*
 * request utility macros
 *
 * NOTE: these macros only reference publicly accessible functions and data, and therefore may be used in MPID macros.
 */
#define mpig_request_get_rank(req_) ((req_)->dev.rank)

#define mpig_request_i_set_ref_count(req_, ref_cnt_)	\
{							\
    (req_)->ref_count = (ref_cnt_);			\
}

#define mpig_request_set_ref_count(req_, ref_cnt_)								\
{														\
    mpig_request_i_set_ref_count((req_), (ref_cnt_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,						\
		     "setting request ref count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", ref_count=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->ref_count));				\
}

#define mpig_request_inc_ref_count(req_, was_inuse_p_)									\
{															\
    *(was_inuse_p_) = (((req_)->ref_count)++) ? TRUE : FALSE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,							\
		     "incrementing request ref count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", ref_count=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->ref_count));					\
}

#define mpig_request_dec_ref_count(req_, is_inuse_p_)									\
{															\
    *(is_inuse_p_) = (--((req_)->ref_count)) ? TRUE : FALSE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,							\
		     "deccrementing request ref count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", ref_count=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->ref_count));					\
}

#define mpig_request_i_set_cc(req_, cc_)	\
{						\
    (req_)->cc = (cc_);				\
}

#define mpig_request_set_cc(req_, cc_)										\
{														\
    mpig_request_i_set_cc((req_), (cc_));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,						\
		     "setting request completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cc));					\
}

#define mpig_request_inc_cc(req_, was_complete_p_)									\
{															\
    *(was_complete_p_) = ((*(req_)->cc_ptr)++) ? FALSE : TRUE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,							\
		     "incrementing request completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cc));						\
}

#define mpig_request_dec_cc(req_, is_complete_p_)									\
{															\
    *(is_complete_p_) = (--(*(req_)->cc_ptr)) ? FALSE : TRUE;								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ,							\
		     "decrementing request completion count: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d",	\
		     (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cc));						\
}

#define mpig_request_complete(req_)				\
{								\
    bool_t is_complete__;					\
								\
    mpig_request_dec_cc((req_), &is_complete__);		\
    if (is_complete__ == TRUE)					\
    {								\
	bool_t is_inuse__;					\
	mpig_request_dec_ref_count((req_), &is_inuse__);	\
	if (is_inuse__ == FALSE)				\
	{							\
	    mpig_request_destroy((req_));			\
	}							\
								\
        mpig_pe_complete_op();					\
    }								\
}

/*
 * Request routines implemented as macros
 */
#define MPID_Request_create() mpig_request_create()

#define MPID_Request_release(req_)			\
{							\
    bool_t is_inuse__;					\
							\
    mpig_request_dec_ref_count((req_), &is_inuse__);	\
    if (is_inuse__ == FALSE)				\
    {							\
	mpig_request_destroy(req_);			\
    }							\
}

#define MPID_Request_set_completed(req_)		\
{							\
    *(req_)->cc_ptr = 0;				\
    mpig_pe_complete_op();				\
}
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN ADI3 MAPPING SECTION
**********************************************************************************************************************************/
/* MT-RC-FIXME: at present, mpig_comm_get_vc_vtable does not perform a RC acquire.  see description above. */
#undef MPID_Send
#define MPID_Send(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_send((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Isend
#define MPID_Isend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_isend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Rsend
#define MPID_Rsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_rsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Irsend
#define MPID_Irsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_irsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Ssend
#define MPID_Ssend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_ssend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Issend
#define MPID_Issend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_issend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Recv
#define MPID_Recv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, status_ ,reqp_)						\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_recv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_),	\
	(status_), (reqp_)))

#undef MPID_Irecv
#define MPID_Irecv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
     (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_irecv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Cancel_send
#define MPID_Cancel_send(sreq_) (mpig_comm_get_vc_vtable((sreq_)->comm, mpig_request_get_rank(sreq_))->adi3_cancel_send(sreq_))

#undef MPID_Cancel_recv
#define MPID_Cancel_recv(rreq_) (mpig_comm_get_vc_vtable((rreq_)->comm, mpig_request_get_rank(rreq_))->adi3_cancel_recv(rreq_))
/**********************************************************************************************************************************
						    END ADI3 MAPPING SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
extern mpig_pe_count_t mpig_pe_count;
extern int mpig_pe_num_cm_must_be_polled;
extern int mpig_pe_active_ops_count;

#define HAVE_MPID_PROGRESS_START_MACRO
#undef MPID_Progress_start
#define MPID_Progress_start(state_)		\
{						\
    (state_)->dev.count = mpig_pe_count;	\
    mpig_cm_vmpi_pe_start(state_);		\
    mpig_cm_xio_pe_start(state_);		\
}

#define HAVE_MPID_PROGRESS_END_MACRO
#undef MPID_Progress_end
#define MPID_Progress_end(state_)	\
{					\
    mpig_cm_vmpi_pe_end(state_);	\
    mpig_cm_xio_pe_end(state_);		\
}

#define HAVE_MPID_PROGRESS_POKE_MACRO
#undef MPID_Progress_poke
#define MPID_Progress_poke() MPID_Progress_test()

#define mpig_pe_start_op()													\
{																\
    /* MT-NOTE: this routine should only be called by the user thread, so it is safe to update the counter without locking a	\
       mutex. the communication modules are responsible for any locking they might require. */					\
    mpig_pe_active_ops_count += 1;												\
}

#define mpig_pe_complete_op()													\
{																\
    /* NOTE_MT: this routine should only be called by the user thread, so it is safe to update the counter without locking a	\
       mutex.  the communication modules are responsible for any locking they might require. */					\
    mpig_pe_active_ops_count -= 1;												\
    mpig_pe_wakeup();														\
    /* mpig_cm_xio_pe_signal_completion(); -- XXX: MT-APP-NOTE: what is really needed here??? probably nothing until we		\
       support multithreaded applications */											\
}

#define mpig_pe_wakeup()													\
{																\
    /* MT: this routine should only be called by the user thread, so it is safe to update the counter without locking a mutex.	\
       the communication modules are responsible for any locking they might require. */						\
    mpig_pe_count += 1;														\
}

/* a communication module should call this routine if a new unpexpected message has arrived.  this routine will wake up the
   progress engine if it is blocking inside of a MPID_Probe(). */
#define mpig_pe_notify_unexp_recv()												\
{																\
    /* MT: this routine should only be called by the user thread, so it is safe to update the counter without locking a mutex.	\
       the communication modules are responsible for any locking they might require. */						\
    mpig_pe_count += 1;														\
}

#define mpig_pe_can_block(cm_active_op_count_, can_block_)							\
{														\
    (mpig_pe_num_cm_must_be_polled <= 1 && (cm_active_op_count_) == mpig_pe_active_ops_count) ? TRUE : FALSE;	\
}

/* a communication module should call this routine during intialization if it requires polling to make progress.  FIXME:
   eventually, we want to pass the polling function to this routine so that it can create a list of polling functions to call.
   for now, these routines are hard coded into the MPID_Progress routines. */
#define mpig_pe_cm_must_be_polled()	\
{					\
    mpig_pe_num_cm_must_be_polled += 1;	\
}
/**********************************************************************************************************************************
						   END PROGRESS ENGINE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN PROCESS ID SECTION
**********************************************************************************************************************************/
/*
 * FIXME: XXX: gpids need to be bigger than two integers.  ideally, we would like to use a uuid supplied by globus to identify
 * the process group and an integer to represent the process rank within the process group; however, this would require reduce
 * the uuid through some hashing algorithm and thus increase the probability of two unique process groups having the same process
 * id.
 */
   
int MPID_GPID_Get(MPID_Comm * comm, int rank, int gpid[]);
/**********************************************************************************************************************************
						     END PROCESS ID SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/
#define MPID_Thread_mutex_create(mutex_, mpi_errno_p_)	\
{							\
    if (globus_mutex_init((mutex_), NULL) != 0)		\
    {							\
	*(mpi_errno_p_) = MPI_ERR_OTHER;		\
    }							\
}

#define MPID_Thread_mutex_destroy(mutex_, mpi_errno_p_)	\
{							\
    if (globus_mutex_destroy((mutex_) != 0)		\
    {							\
	*(mpi_errno_p_) = MPI_ERR_OTHER;		\
    }							\
}

#define MPID_Thread_mutex_lock(mutex_)	\
{					\
    globus_mutex_lock(mutex_);		\
}

#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    globus_mutex_unlock(mutex_);		\
}

unsigned long mpig_thread_get_id(void);

#if !defined(MPIG_THREADED)
#define mpig_thread_get_id() ((unsigned long) (0))
#else
#define mpig_thread_get_id() ((unsigned long) globus_thread_self())
#endif
/**********************************************************************************************************************************
						END DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)

void mpig_debug_init(void);

extern globus_debug_handle_t mpig_debug_handle;
extern time_t mpig_debug_start_tv_sec;

#define MPIG_DEBUG_LEVEL_NAMES \
    "ERROR FUNC ADI3 PT2PT COLL DYNAMIC WIN THREADS PROGRESS DATA COUNT REQ COMM TOPO VC PG BC RECVQ VCCM PM DATABUF MSGHDR MPI" 

typedef enum mpig_debug_levels
{
    MPIG_DEBUG_LEVEL_ERROR =		1 << 0,
    MPIG_DEBUG_LEVEL_FUNC =		1 << 1,
    MPIG_DEBUG_LEVEL_ADI3 =		1 << 2,
    MPIG_DEBUG_LEVEL_PT2PT =		1 << 3,
    MPIG_DEBUG_LEVEL_COLL =		1 << 4,
    MPIG_DEBUG_LEVEL_DYNAMIC =		1 << 5,
    MPIG_DEBUG_LEVEL_WIN =		1 << 6,
    MPIG_DEBUG_LEVEL_THREADS =		1 << 7,
    MPIG_DEBUG_LEVEL_PROGRESS =		1 << 8,
    MPIG_DEBUG_LEVEL_DATA =		1 << 9,
    MPIG_DEBUG_LEVEL_COUNT =		1 << 10,
    MPIG_DEBUG_LEVEL_REQ =		1 << 11,
    MPIG_DEBUG_LEVEL_COMM =		1 << 12,
    MPIG_DEBUG_LEVEL_TOPO =		1 << 13,
    MPIG_DEBUG_LEVEL_VC =		1 << 14,
    MPIG_DEBUG_LEVEL_PG =		1 << 15,
    MPIG_DEBUG_LEVEL_BC =		1 << 16,
    MPIG_DEBUG_LEVEL_RECVQ =		1 << 17,
    MPIG_DEBUG_LEVEL_VCCM =		1 << 18,
    MPIG_DEBUG_LEVEL_PM =		1 << 19,
    MPIG_DEBUG_LEVEL_DATABUF =		1 << 20,
    MPIG_DEBUG_LEVEL_MSGHDR =		1 << 21,
    MPIG_DEBUG_LEVEL_MPI =		1 << 22
}
mpig_debug_levels_t;

#define MPIG_DEBUG_STMT(levels_, a_)  if ((levels_) & mpig_debug_handle.levels) {a_}

#if defined(HAVE_C99_VARIADIC_MACROS) || defined(HAVE_GNU_VARIADIC_MACROS)
void mpig_debug_printf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...);
void mpig_debug_uprintf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...);
#define MPIG_DEBUG_PRINTF(a_) mpig_debug_printf a_
#define MPIG_DEBUG_UPRINTF(a_) mpig_debug_uprintf a_
#endif

#if defined(HAVE_C99_VARIADIC_MACROS)

#define mpig_debug_printf(levels_, fmt_, ...)								\
{													\
    if ((levels_) & mpig_debug_handle.levels)								\
    {													\
	mpig_debug_uprintf_fn(levels_, __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## __VA_ARGS__);	\
    }													\
}

#define mpig_debug_uprintf(levels_, fmt_, ...)								\
{													\
    mpig_debug_uprintf_fn(levels_, __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## __VA_ARGS__);	\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_UTIL_PRINTF a_
#define MPIG_DEBUG_OLD_UTIL_PRINTF(fmt_, ...)				\
{									\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPI, fmt_, ## __VA_ARGS__);	\
}

#elif defined(HAVE_GNU_VARIADIC_MACROS)

#define mpig_debug_printf(levels_, fmt_, args_...)							\
{													\
    if ((levels_) & mpig_debug_handle.levels)								\
    {													\
	mpig_debug_printf_fn(levels_, __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## args_);	\
    }													\
}

#define mpig_debug_uprintf(levels_, fmt_, args_...)						\
{												\
    mpig_debug_uprintf_fn(levels_, __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## args_);	\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_UTIL_PRINTF a_
#define MPIG_DEBUG_OLD_UTIL_PRINTF(fmt_, args,...)		\
{								\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPI, fmt_, ## args_);	\
}

#else /* the compiler does not support variadic macros */

#define MPIG_DEBUG_PRINTF(a_) mpig_debug_printf(a_)
#define MPIG_DEBUG_UPRINTF(a_) mpig_debug_uprintf(a_)

typedef struct mpig_debug_thread_state
{
    const char * filename;
    const char * funcname;
    int line;
}
mpig_debug_thread_state_t;

extern globus_thread_once_t mpig_debug_create_state_key_once;
extern globus_thread_key_t mpig_debug_state_key;

void mpig_debug_printf_fn(unsigned levels, const char * fmt, ...);
void mpig_debug_uprintf_fn(unsigned levels, const char * fmt, ...);
void mpig_debug_old_util_printf_fn(const char * fmt, ...);
void mpig_debug_create_state_key(void);

#define	mpig_debug_save_state(filename_, funcname_, line_)								\
{															\
    mpig_debug_thread_state_t * state__;										\
    globus_thread_once(&mpig_debug_create_state_key_once, mpig_debug_create_state_key);					\
    state__ = globus_thread_getspecific(mpig_debug_state_key);								\
    if (state__ == NULL)												\
    {															\
	state__ = MPIU_Malloc(sizeof(mpig_debug_thread_state_t));							\
	MPIU_Assertp((state__ != NULL) && "ERROR: unable to allocate memory for thread specific debugging state");	\
	globus_thread_setspecific(mpig_debug_state_key, state__);							\
    }															\
    state__->filename = (filename_);											\
    state__->funcname = (funcname_);											\
    state__->line = (line_);												\
}

#define	mpig_debug_retrieve_state(filename_p_, funcname_p_, line_p_)	\
{									\
    mpig_debug_thread_state_t * state__;				\
    state__ = globus_thread_getspecific(mpig_debug_state_key);		\
    *(filename_p_) = state__->filename;					\
    *(funcname_p_) = state__->funcname;					\
    *(line_p_) = state__->line;						\
}

#define	mpig_debug_printf(a_)						\
{									\
    mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
    mpig_debug_printf_fn a_;						\
}

#define	mpig_debug_uprintf(a_)						\
{									\
    mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
    mpig_debug_uprintf_fn a_;						\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) mpig_debug_old_util_printf a_
#define mpig_debug_old_util_printf(a_)					\
{									\
    mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
    mpig_debug_old_util_printf_fn a_;					\
}
#endif /* variadic macros */


/* override the stardard MPICH2 enter/exit debug logging macros */
#undef MPIR_FUNC_ENTER
#define MPIR_FUNC_ENTER(state_)					\
{								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));	\
    MPIG_FUNCNAME_CHECK();					\
}

#undef MPIR_FUNC_EXIT
#define MPIR_FUNC_EXIT(state_)					\
{								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting"));	\
}

/* macro to verify that the function name in FUNCNAME is really the name of the function */
#if defined(HAVE__FUNC__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __func__) == 0)
#elif defined (HAVE__FUNCTION__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __FUNCTION__) == 0)
#elif defined (HAVE_CAP__FUNC__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __FUNC__) == 0)
#else
#define MPIG_FUNCNAME_CHECK()
#endif

#else

#define MPIG_DEBUG_TEST(levels_) (FALSE)
#define MPIG_DEBUG_STMT(levels_, a_)
#define	MPIG_DEBUG_PRINTF(a_)

#endif
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/

#define MPIR_FUNC_COUNT_ENTER(state_)                                  \
{                                                                      \
    MPIR_FUNC_ENTER(state_);                                           \
    if(state_ >= MPIG_FUNC_CNT_FIRST && state_ <= MPIG_FUNC_CNT_LAST)  \
    {                                                                  \
        mpig_process.function_count[state_ - MPIG_FUNC_CNT_FIRST]++;   \
    }                                                                  \
}

#define MPIR_FUNC_COUNT_EXIT(state_)                            \
{                                                               \
    MPIR_FUNC_EXIT(state_);                                     \
}

/* override the enter/exit debug logging macros to count function calls */
#undef MPID_MPI_FUNC_ENTER
#undef MPID_MPI_FUNC_EXIT
#undef MPID_MPI_PT2PT_FUNC_ENTER
#undef MPID_MPI_PT2PT_FUNC_EXIT
#undef MPID_MPI_PT2PT_FUNC_ENTER_FRONT
#undef MPID_MPI_PT2PT_FUNC_EXIT_FRONT
#undef MPID_MPI_PT2PT_FUNC_ENTER_BACK
#undef MPID_MPI_PT2PT_FUNC_ENTER_BOTH
#undef MPID_MPI_PT2PT_FUNC_EXIT_BACK
#undef MPID_MPI_PT2PT_FUNC_EXIT_BOTH
#undef MPID_MPI_COLL_FUNC_ENTER
#undef MPID_MPI_COLL_FUNC_EXIT
#undef MPID_MPI_RMA_FUNC_ENTER
#undef MPID_MPI_RMA_FUNC_EXIT
#undef MPID_MPI_INIT_FUNC_ENTER
#undef MPID_MPI_INIT_FUNC_EXIT
#undef MPID_MPI_FINALIZE_FUNC_ENTER
#undef MPID_MPI_FINALIZE_FUNC_EXIT

#define MPID_MPI_FUNC_ENTER(a)                  MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)                   MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)            MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)             MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)      MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)       MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)       MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)       MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)        MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)        MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)             MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)              MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)              MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)               MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)             MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)              MPIR_FUNC_COUNT_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)         MPIR_FUNC_COUNT_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)          MPIR_FUNC_COUNT_EXIT(a)


#endif /* MPICH2_MPIDPOST_H_INCLUDED */
