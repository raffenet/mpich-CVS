/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPOST_H_INCLUDED)
#define MPICH2_MPIDPOST_H_INCLUDED


/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
void mpig_comm_list_add(MPID_Comm * comm);
#define MPID_Dev_comm_create_hook(comm_) mpig_comm_list_add(comm_)

void mpig_comm_list_remove(MPID_Comm * comm);
#define MPID_Dev_comm_destroy_hook(comm_) mpig_comm_list_remove(comm_)

void mpig_dev_comm_free_hook(MPID_Comm * comm, int * mpi_errno_p);
#define MPID_Dev_comm_free_hook(comm_, mpi_errno_p_) mpig_dev_comm_free_hook((comm_), (mpi_errno_p_))

#if defined(MPIG_VMPI)

void mpig_dev_comm_dup_hook(MPID_Comm * orig_comm, MPID_Comm * new_comm, int * mpi_errno_p);
#define MPID_Dev_comm_dup_hook(orig_comm_, new_comm_, mpi_errno_p_)	\
    mpig_dev_comm_free_hook((orig_comm_), (new_comm_), (mpi_errno_p_))

void mpig_dev_intercomm_create_hook(MPID_Comm * local_comm, int local_leader, MPID_Comm * peer_comm, int remote_leader, int tag,
    MPID_Comm * new_intercomm, int * mpi_errno_p);
#define MPID_Dev_intercomm_create_hook(orig_comm_, new_comm_, mpi_errno_p_)	\
    mpig_dev_intercomm_create_hook((orig_comm_), (new_comm_), (mpi_errno_p_))

#endif /* defined(MPIG_VMPI) */

/* MT-RC-NOTE: this routine may only be called (directly or indirectly) by MPI routines.  furthermore, the population of the
   cm_funcs table and the the pointer to the cm_funcs table in the VC may only be set routines called by MPI routines.  if the
   MPI routines may be called by multiple threads, then the MPI routines are required to perform the necessary release
   consistency operations to insure that the cm_funcs table is fresh. */
#define mpig_comm_get_vc_cmf(comm_, rank_)  (((rank_) >= 0) ? ((comm_)->vcr[(rank_)]->cm_funcs) : mpig_cm_other_vc->cm_funcs)
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
        mpig_progress_complete_op();				\
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
    mpig_progress_complete_op();			\
}
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN ADI3 MAPPING SECTION
**********************************************************************************************************************************/
#undef MPID_Send
#define MPID_Send(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_send((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Isend
#define MPID_Isend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_isend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Rsend
#define MPID_Rsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_rsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Irsend
#define MPID_Irsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_irsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Ssend
#define MPID_Ssend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_ssend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Issend
#define MPID_Issend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_issend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Recv
#define MPID_Recv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, status_ ,reqp_)							\
    (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_recv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (status_),	\
                          (reqp_)))

#undef MPID_Irecv
#define MPID_Irecv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
     (mpig_comm_get_vc_cmf((comm_), (rank_))->adi3_irecv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Cancel_send
#define MPID_Cancel_send(sreq_) (mpig_comm_get_vc_cmf((sreq_)->comm, mpig_request_get_rank(sreq_))->adi3_cancel_send(sreq_))

#undef MPID_Cancel_recv
#define MPID_Cancel_recv(rreq_) (mpig_comm_get_vc_cmf((rreq_)->comm, mpig_request_get_rank(rreq_))->adi3_cancel_recv(rreq_))

/**********************************************************************************************************************************
						    END ADI3 MAPPING SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
extern volatile mpig_progress_count_t mpig_progress_count;
extern int mpig_progress_num_cm_requiring_polling;
extern volatile int mpig_progress_ops_outstanding;

#define HAVE_MPID_PROGRESS_START_MACRO
#undef MPID_Progress_start
#define MPID_Progress_start(state_)		\
{						\
    (state_)->dev.count = mpig_progress_count;	\
    /* mpig_cm_vmpi_progress_start(state_); */	\
    mpig_cm_xio_progress_start(state_);		\
}

#define HAVE_MPID_PROGRESS_END_MACRO
#undef MPID_Progress_end
#define MPID_Progress_end(state_)		\
{						\
    /* mpig_cm_vmpi_progress_end(state_); */	\
    mpig_cm_xio_progress_end(state_);		\
}

#define HAVE_MPID_PROGRESS_POKE_MACRO
#undef MPID_Progress_poke
#define MPID_Progress_poke() MPID_Progress_test()

#define mpig_progress_start_op()												\
{																\
    /* MT-NOTE: this routine should only be called by the user thread, so it is safe to update the counter without locking a	\
       mutex. the communication modules are responsible for any locking they might require. */					\
    mpig_progress_ops_outstanding += 1;												\
}

#define mpig_progress_complete_op()												\
{																\
    /* NOTE_MT: this routine should only be called by the user thread, so it is safe to update the counter without locking a	\
       mutex.  the communication modules are responsible for any locking they might require. */					\
    mpig_progress_ops_outstanding -= 1;												\
    mpig_progress_wakeup();													\
    /* mpig_cm_xio_progress_signal_completion(); -- XXX: MT-APP-NOTE: what is really needed here??? probably nothing until we	\
       support multithreaded applications */											\
}

#define mpig_progress_wakeup()													\
{																\
    /* MT: this routine should only be called by the user thread, so it is safe to update the counter without locking a mutex.	\
       the communication modules are responsible for any locking they might require. */						\
    mpig_progress_count += 1;													\
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

#define MPIG_DEBUG_STMT(a_) a_

void mpig_debug_init(void);

extern globus_debug_handle_t mpig_debug_handle;
extern time_t mpig_debug_start_tv_sec;

#define MPIG_DEBUG_LEVEL_NAMES \
    "ERROR FUNC ADI3 PT2PT COLL DYNAMIC WIN THREADS PROGRESS DATA COUNT REQ COMM VC PG BC RECVQ VCCM PM DATABUF MSGHDR MPI" 

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
    MPIG_DEBUG_LEVEL_VC =		1 << 13,
    MPIG_DEBUG_LEVEL_PG =		1 << 14,
    MPIG_DEBUG_LEVEL_BC =		1 << 15,
    MPIG_DEBUG_LEVEL_RECVQ =		1 << 16,
    MPIG_DEBUG_LEVEL_VCCM =		1 << 17,
    MPIG_DEBUG_LEVEL_PM =		1 << 18,
    MPIG_DEBUG_LEVEL_DATABUF =		1 << 19,
    MPIG_DEBUG_LEVEL_MSGHDR =		1 << 20,
    MPIG_DEBUG_LEVEL_MPI =		1 << 21
}
mpig_debug_levels_t;

#if defined(HAVE_C99_VARIADIC_MACROS)

#define MPIG_DEBUG_PRINTF(a_)	\
{				\
    mpig_debug_printf a_;	\
}

#define mpig_debug_printf(levels_, fmt_, ...)				\
{									\
    if ((levels_) & mpig_debug_handle.levels)				\
    {									\
	if (mpig_debug_handle.file != NULL)				\
	{								\
	    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)	\
	    {								\
		mpig_debug_printf_untimed(fmt_, ## __VA_ARGS__);	\
	    }								\
	    else							\
	    {								\
		mpig_debug_printf_timed(fmt_, ## __VA_ARGS__);		\
	    }								\
	}								\
    }									\
}

#define mpig_debug_printf_untimed(fmt_, ...)						\
{											\
    fprintf(mpig_debug_handle.file, "[%s:%d:%lu] %s(l=%d) " fmt_ "\n",			\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(),	\
	    MPIU_QUOTE(FUNCNAME), __LINE__, ## __VA_ARGS__);				\
}

#define mpig_debug_printf_timed(fmt_, ...)								\
{													\
    struct timeval tv;											\
    gettimeofday(&tv, NULL);										\
    tv.tv_sec -= mpig_debug_start_tv_sec;								\
    fprintf(mpig_debug_handle.file, "[%s:%d:%lu] %s(l=%d:t=%lu.%.6lu) " fmt_ "\n",			\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), MPIU_QUOTE(FUNCNAME),	\
	    __LINE__, (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec, ## __VA_ARGS__);		\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_STYLE_PRINTF a_
#define MPIG_DEBUG_OLD_STYLE_PRINTF(fmt_, ...)				\
{									\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPI, fmt_, ## __VA_ARGS__);	\
}

#elif defined(HAVE_GNU_VARIADIC_MACROS)

#define MPIG_DEBUG_PRINTF(a_)	\
{				\
    mpig_debug_printf a_;	\
}

#define mpig_debug_printf(levels_, fmt_, args_...)			\
{									\
    if ((levels_) & mpig_debug_handle.levels)				\
    {									\
	if (mpig_debug_handle.file != NULL)				\
	{								\
	    if (((levels_) & mpig_debug_handle.timestamp_levels) == 0)	\
	    {								\
		mpig_debug_printf_untimed((fmt_), ## args_);		\
	    }								\
	    else							\
	    {								\
		mpig_debug_printf_timed((fmt_), ## args_);		\
	    }								\
	}								\
    }									\
}

#define mpig_debug_printf_untimed(fmt_, args_...)					\
{											\
    fprintf(mpig_debug_handle.file, "[%s:%d:%lu] %s(l=%d) " fmt_ "\n",			\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(),	\
	    MPIU_QUOTE(FUNCNAME), __LINE__, ## args_);					\
}

#define mpig_debug_printf_timed(fmt_, args_...)								\
{													\
    struct timeval tv;											\
    gettimeofday(&tv, NULL);										\
    tv.tv_sec -= mpig_debug_start_tv_sec;								\
    fprintf(mpig_debug_handle.file, "[%s:%d:%lu] %s(l=%d:t=%lu.%.6lu) " fmt_ "\n",			\
	    mpig_process.my_pg_id, mpig_process.my_pg_rank, mpig_thread_get_id(), MPIU_QUOTE(FUNCNAME),	\
	    __LINE__, (unsigned long) tv.tv_sec, (unsigned long) tv.tv_usec, ## args_);			\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_STYLE_PRINTF a_
#define MPIG_DEBUG_OLD_STYLE_PRINTF(fmt_, args_...)		\
{								\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPI, fmt_, ## args__);	\
}

#else /* compiler does not support variadic macros */

typedef struct mpig_debug_thread_state
{
    const char * file;
    const int line;
    const char * funcname;
}
mpig_debug_thread_state_t;

extern mpig_debug_thread_state_t mpig_debug_thread_state;

void mpig_debug_printf(int levels, const char * fmt, ...);

#define	mpig_debug_save_state()								\
{											\
    /* XXX: put function name, file name, and line number into thread specific data */	\
}

#define	mpig_debug_retrieve_state(funcname_p_, file_p_, line_p_)			\
{											\
    /* XXX: get function name, file name, and line number from thread specific data */	\
}

#define	MPIG_DEBUG_PRINTF(a_)	\
{				\
    mpig_debug_save_state();	\
    mpig_debug_printf a_;	\
}

#endif

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
    MPIG_FUNCNAME_CHECK();					\
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

#define MPIG_DEBUG_STMT(a_)
#define	MPIG_DEBUG_PRINTF(a_)

#endif
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDPOST_H_INCLUDED */
