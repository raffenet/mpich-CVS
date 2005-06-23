/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPOST_H_INCLUDED)
#define MPICH2_MPIDPOST_H_INCLUDED


/*>>>>>>>>>>>>>>>>>>>>
  COMMUNICATOR SECTION
  >>>>>>>>>>>>>>>>>>>>*/
#define mpig_comm_get_vc(comm_, rank_)  (((rank_) >= 0) ? ((comm_)->vcr[(rank_)]) : (mpig_cm_other_vc))
/*<<<<<<<<<<<<<<<<<<<<
  COMMUNICATOR SECTION
  <<<<<<<<<<<<<<<<<<<<*/


/*>>>>>>>>>>>>>>>
  REQUEST SECTION
  >>>>>>>>>>>>>>>*/
/*
 * NOTE: The setting, incrementing and decrementing of the request completion counter must be atomic since the progress engine
 * could be completing the request in one thread and the application could be cancelling the request in another thread.  If atomic
 * instructions are used, a write barrier or store-release may be needed to insure that updates to other fields in the request
 * structure are made visible before the completion counter is updated.  The exact operations required depending the on the memory
 * model of the processor architecture.
 */


MPID_Request * mpig_request_create(void);
void mpig_request_destroy(MPID_Request * req);

/*
 * Request utility macros. NOTE: These macros only reference publicly accessible functions and data, and therefore may be used in
 * MPID macros.
 *
 * NOTE: Some of these macros may also be implemented in mpid_request.c.  Any changes made here should also be made there.
 */
#define mpig_request_set_ref(req_, ref_cnt_)	\
{						\
    (req_)->ref_count = (ref_cnt_);		\
}

#define mpig_request_inc_ref(req_, ref_flag_)	\
{						\
    *(ref_flag_) = ((req_)->ref_count)++;	\
}

#define mpig_request_dec_ref(req_, ref_flag_)	\
{						\
    *(ref_flag_) = --((req_)->ref_count);	\
}

#define mpig_request_set_cc(req_, cc_)		\
{						\
    (req_)->cc = (cc_);				\
}

#define mpig_request_inc_cc(req_, cc_flag_)	\
{						\
    *(cc_flag_) = (*(req_)->cc_ptr)++;		\
}

#define mpig_request_dec_cc(req_, cc_flag_)	\
{						\
    *(cc_flag_) = --(*(req_)->cc_ptr);		\
}

#define mpig_request_complete(reqp_)			\
{							\
    int flag__;						\
							\
    mpig_request_dec_cc(*(reqp_), &flag__);		\
    if (flag__ == FALSE)				\
    {							\
	mpig_request_dec_ref(*(reqp_), &flag__);	\
	if (flag__ == FALSE)				\
	{						\
	    mpig_request_destroy(*(reqp_));		\
	    *(reqp_) = NULL;				\
	}						\
    }							\
}

/*
 * Request routines implemented as macros
 */
#define MPID_Request_create() mpig_request_create()

#define MPID_Request_release(req_)				\
{								\
    int ref_flag__;						\
								\
    mpig_request_dec_ref((req_), &ref_flag__);			\
    if (ref_flag__ == FALSE)					\
    {								\
	mpig_request_destroy(req_);				\
    }								\
}

#define MPID_Request_set_completed(req_)		\
{							\
    *(req_)->cc_ptr = 0;				\
    /* XXX: mpig_progress_signal_completion(); */	\
}
/*<<<<<<<<<<<<<<<
  REQUEST SECTION
  <<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>>>>>>>>>>>
  PROGRESS ENGINE SECTION
  >>>>>>>>>>>>>>>>>>>>>>>*/
#if defined(XXX)
#define MPID_Progress_start(state_)		\
{						\
}

#define MPID_Progress_end(state_)		\
{						\
}
#endif
/*<<<<<<<<<<<<<<<<<<<<<<<
  PROGRESS ENGINE SECTION
  <<<<<<<<<<<<<<<<<<<<<<<*/

/*>>>>>>>>>>>>>
  PT2PT SECTION
  >>>>>>>>>>>>>*/
#define MPID_Send(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)			\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_send((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Isend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)			\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_isend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Rsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)			\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_rsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Irsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)		\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_irsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Ssend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)			\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_ssend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Issend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)		\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
     adi3_issend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#define MPID_Recv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, status_ ,reqp_)			\
    (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.						\
     adi3_recv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (status_), (reqp_)))

#define MPID_Irecv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)			\
     (mpig_comm_get_vc((comm_), (rank_))->cm_funcs.					\
      adi3_irecv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))
/*<<<<<<<<<<<<<
  PT2PT SECTION
  <<<<<<<<<<<<<*/

#endif /* MPICH2_MPIDPOST_H_INCLUDED */
