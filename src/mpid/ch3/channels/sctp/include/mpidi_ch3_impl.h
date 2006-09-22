/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3i_sctp_conf.h"
#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"

#include "all_hash.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

/* myct: MODULE DEF */
#define SCTP

typedef int (* MPIDU_Sock_progress_update_func_t)(MPIU_Size_t num_bytes, void * user_ptr);

typedef struct sctp_sndrcvinfo sctp_rcvinfo;

typedef struct MPIDU_Sock_ifaddr_t {
    int len, type;
    unsigned char ifaddr[16];
} MPIDU_Sock_ifaddr_t;

#define MPIDU_SOCK_ERR_NOMEM -1
#define MPIDU_SOCK_ERR_TIMEOUT -2

/* myct: hash table entry */
typedef struct hash_entry
{
    sctp_assoc_t assoc_id;
    MPIDI_VC_t * vc;
} MPIDI_CH3I_Hash_entry;

/* myct: global hash table */
HASH* MPIDI_CH3I_assocID_table;

/* one_to_many socket */
int MPIDI_CH3I_onetomany_fd;

/* number of items in the sendq's */
int sendq_total;

extern int MPIDI_CH3I_listener_port;

/* used for dynamic processes */
int MPIDI_CH3I_has_connect_ack_outstanding;
MPIDI_VC_t * MPIDI_CH3I_connecting_vc;
int MPIDI_CH3I_using_tmp_vc;

/* myct: determine the stream # of a req */
/* brad : want to avoid stream zero (since it's invalid in SCTP) as well as the highest
 *    stream number in case we ever want to have a control stream.
 *    also, we want to use context so that collectives don't cause head-of-line blocking
 *    for p2p...
 */
/*#define Req_Stream_from_match(match) (match.tag)%MPICH_SCTP_NUM_STREAMS*/
/* brad: the following keeps stream s.t. 1 <= stream <= MPICH_SCTP_NUM_REQS_ACTIVE */
#define Req_Stream_from_match(match) (abs((match.tag) + (match.context_id))% MPICH_SCTP_NUM_REQS_ACTIVE)+1
#define REQ_Stream(req) Req_Stream_from_match(req->dev.match)
int Req_Stream_from_pkt_and_req(MPIDI_CH3_Pkt_t * pkt, MPID_Request * sreq);

/* myct: may 9, initialize stream */
#define STREAM_INIT(x) \
{\
    x-> vc = NULL;\
    x-> have_sent_pg_id = MPIDI_CH3I_VC_STATE_UNCONNECTED;\
    x-> send_active = NULL;\
    x-> recv_active = NULL;\
    x-> sendQ_head = NULL;\
    x-> sendQ_tail = NULL;\
}

/* myct: use of STREAM macros to allow change of logic in future */
#define SEND_CONNECTED(vc, x) vc->ch.stream_table[x].have_sent_pg_id
#define SEND_ACTIVE(vc, x) vc->ch.stream_table[x].send_active
#define RECV_ACTIVE(vc, x) vc->ch.stream_table[x].recv_active
#define VC_SENDQ(vc, x) vc->ch.stream_table[x].sendQ_head
#define VC_SENDQ_TAIL(vc, x) vc->ch.stream_table[x].sendQ_tail
#define VC_IOV(vc, x) vc->ch.stream_table[x].iov


/* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_enqueue(vc, req)				\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x", vc, req->handle));  \
    sendq_total++; \
    req->dev.next = NULL;						\
    if (VC_SENDQ(vc,REQ_Stream(req)) != NULL)		      	\
    {									\
	VC_SENDQ_TAIL(vc, REQ_Stream(req))->dev.next = req;				\
    }									\
    else								\
    {									\
	VC_SENDQ(vc, REQ_Stream(req)) = req;					\
    }									\
    VC_SENDQ_TAIL(vc, REQ_Stream(req)) = req;						\
}

/* myct: enqueue to a specific stream */
#define MPIDI_CH3I_SendQ_enqueue_x(vc, req, x)                \
{                                                           \
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue vc=%p req=0x%08x", vc, req->handle));  \
    sendq_total++; \
    req->dev.next = NULL;						\
    if (VC_SENDQ_TAIL(vc, x) != NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x)->dev.next = req;				\
    }									\
    else								\
    {									\
	VC_SENDQ(vc, x) = req;					\
    }									\
    VC_SENDQ_TAIL(vc, x) = req;						\
}

/* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_enqueue_head(vc, req)				\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", vc, req->handle));\
    sendq_total++; \
    req->dev.next = VC_SENDQ(vc, REQ_Stream(req));					\
    if (VC_SENDQ_TAIL(vc, REQ_Stream(req)) == NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, REQ_Stream(req)) = req;					\
    }									\
    VC_SENDQ(vc, REQ_Stream(req)) = req;						\
}

/* myct: enqueue to a particular stream */
#define MPIDI_CH3I_SendQ_enqueue_head_x(vc, req, x)				\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_enqueue_head vc=%p req=0x%08x", vc, req->handle));\
    sendq_total++; \
    req->dev.next = VC_SENDQ(vc, x);					\
    if (VC_SENDQ_TAIL(vc, x) == NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x) = req;					\
    }									\
    VC_SENDQ(vc, x) = req;						\
}


    /* MT - not thread safe! */
#define MPIDI_CH3I_SendQ_dequeue(vc)					\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x", vc, VC_SENDQ(vc,x)->handle));\
    sendq_total--; \
    VC_SENDQ(vc, REQ_Stream(req)) = VC_SENDQ(vc, REQ_Stream(req))->dev.next;   \
    if (VC_SENDQ(vc, REQ_Stream(req)) == NULL)					\
    {									\
        VC_SENDQ_TAIL(vc, REQ_Stream(req)) = NULL;					\
    }									\
}

#define MPIDI_CH3I_SendQ_dequeue_x(vc, x)					\
{									\
    MPIDI_DBG_PRINTF((50, FCNAME, "SendQ_dequeue vc=%p req=0x%08x", vc, VC_SENDQ(vc,x)->handle));\
    sendq_total--; \
    VC_SENDQ(vc, x) = VC_SENDQ(vc, x)->dev.next;\
    if (VC_SENDQ(vc, x) == NULL)					\
    {									\
	VC_SENDQ_TAIL(vc, x) = NULL;					\
    }									\
}


#define MPIDI_CH3I_SendQ_head(vc) (vc->ch.sendq_head)

/* myct: sendQ head for specific stream */
#define MPIDI_CH3I_SendQ_head_x(vc, x) (VC_SENDQ(vc, x))

#define MPIDI_CH3I_SendQ_empty(vc) (VC_SENDQ(vc, 0) == NULL)

#define MPIDI_CH3I_SendQ_empty_x(vc, x) (VC_SENDQ(vc, x) == NULL)

/* myct: posted IOV macros */
#define POST_IOV(x) (x->write.iov.ptr)
#define POST_IOV_CNT(x) (x->write.iov.iov_count)
#define POST_IOV_OFFSET(x) (x->write.iov.offset)
#define POST_IOV_FLAG(x) (x->write_iov_flag) 
#define POST_BUF(x) (x->write.buf.ptr)
#define POST_BUF_MIN(x) (x->write.buf.min)
#define POST_BUF_MAX(x) (x->write.buf.max)
#define POST_UPDATE_FN(x) (x)

/* myct: determine if SCTP_WAIT should keep spinning */
#define SPIN(x) (x == 0)? FALSE : TRUE

/* myct: we need a global event queue */

#define MPIDU_SCTP_EVENTQ_POOL_SIZE 20

typedef enum MPIDU_Sctp_op
{
    MPIDU_SCTP_OP_READ,
    MPIDU_SCTP_OP_WRITE,
    MPIDU_SCTP_OP_CLOSE,
    MPIDU_SCTP_OP_WAKEUP,
    MPIDU_SCTP_OP_ACCEPT
} MPIDU_Sctp_op_t;

typedef struct MPIDU_Sctp_event
{
    MPIDU_Sctp_op_t op_type;
    MPIU_Size_t num_bytes;
    int fd; /* in case we ever use more than one socket */
    sctp_rcvinfo sri;  /* this points to sctp_sndrcvinfo always for now on read */
    /* int stream_no; */  /* this is in the sri for read but no sri is provided for write */
    void * user_ptr;  /* this points to sctp_advbuf always for reads, and to vc for writes */
    void * user_ptr2;
    int user_value; /* can differ from flags in sri */
    int error;

} MPIDU_Sctp_event_t;

struct MPIDU_Sctp_eventq_elem {
    MPIDU_Sctp_event_t event[MPIDU_SCTP_EVENTQ_POOL_SIZE];
    int size;
    int head;
    int tail;
    struct MPIDU_Sctp_eventq_elem* next;
};

/* myct: we need global event queues, because we no longer have sock_set */
struct MPIDU_Sctp_eventq_elem* eventq_head;
struct MPIDU_Sctp_eventq_elem* eventq_tail;


int MPIDU_Sctp_event_enqueue(MPIDU_Sctp_op_t op, MPIU_Size_t num_bytes, 
				    sctp_rcvinfo* sri, int fd, 
				    void * user_ptr, void * user_ptr2, int msg_flags, int error);
int MPIDU_Sctp_event_dequeue(struct MPIDU_Sctp_event * eventp);

void MPIDU_Sctp_free_eventq_mem(void);

void print_SCTP_event(struct MPIDU_Sctp_event * eventp);

/* myct: for send queue */
void MPIDU_Sctp_stream_init(MPIDI_VC_t* vc, MPID_Request* req, int stream);

int MPIDU_Sctp_writev_fd(int fd, struct sockaddr_in * to, struct iovec* ldata,
                         int iovcnt, int stream, int ppid, MPIU_Size_t* nb);
int MPIDU_Sctp_writev(MPIDI_VC_t* vc, struct iovec* data,int iovcnt, 
		      int stream, int ppid, MPIU_Size_t* nb);
int MPIDU_Sctp_write(MPIDI_VC_t* vc, void* buf, MPIU_Size_t len, 
		     int stream_no, int ppid, MPIU_Size_t* num_written);

int readv_from_advbuf(MPID_IOV* iovp, int iov_cnt, char* from, int bytes_read);
int read_from_advbuf(char* from, char* to, int nbytes, int offset);

inline int MPIDU_Sctp_post_writev(MPIDI_VC_t* vc, MPID_Request* sreq, int offset,
				  MPIDU_Sock_progress_update_func_t fn, int stream_no);

inline int MPIDU_Sctp_post_write(MPIDI_VC_t* vc, MPID_Request* sreq, MPIU_Size_t minlen, 
			  MPIU_Size_t maxlen, MPIDU_Sock_progress_update_func_t fn, int stream_no);

/* duplicate static existed so made this non-static */
int adjust_iov(MPID_IOV ** iovp, int * countp, MPIU_Size_t nb);
/* used in ch3_init.c and ch3_progress.c . sock equivalents in util/sock */
int MPIDU_Sctp_wait(int fd, int timeout, MPIDU_Sctp_event_t * event);
int MPIDI_CH3I_Progress_handle_sctp_event(MPIDU_Sctp_event_t * event);


/* BUFFER management routines/structs */

#define READ_AMOUNT 262144

typedef struct my_buffer {
  int free_space; /* free space */
  char buffer[READ_AMOUNT];
  char* buf_ptr;
  struct my_buffer* next; /* if we ever chain bufferNode */
  char dynamic;
} BufferNode_t;

inline void BufferList_init(BufferNode_t* node);
int inline buf_init(BufferNode_t* node);
int inline buf_clean(BufferNode_t* node);
inline char* request_buffer(int size, BufferNode_t** node);
int inline update_size(BufferNode_t* node, int size);
#define remain_size(x) x-> free_space


/* END BUFFER management */

/* FIXME: Any of these used in the ch3->channel interface should be
   defined in a header file in ch3/include that defines the 
   channel interface */
int MPIDI_CH3I_Progress_init(int pg_size);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_VC_post_connect(MPIDI_VC_t *);


/* myct: Apr3 global sendQ stuff */
typedef struct gb_sendQ_head {
    struct MPID_Request* head;
    struct MPID_Request* tail;
    int count;
} GLB_SendQ_Head;

extern GLB_SendQ_Head Global_SendQ;

/* myct: global sendQ enqueue */
/* link list may not be a good choice */

/* myct: malloc is not needed, because ch portion of the REQ has all the info 
 * we need: vc, stream */

#define Global_SendQ_enqueue(vc, req, x) {               \
        req->ch.vc = vc;                        \
        req->ch.stream = x;                \
        SEND_ACTIVE(vc, x) = req;\
        req->dev.next = NULL;\
	if(Global_SendQ.head == NULL) {	\
		Global_SendQ.head = req;	\
                Global_SendQ.tail = req;  \
	} else {				\
		Global_SendQ.tail->dev.next = req;	\
		Global_SendQ.tail = req;		\
	}					\
	Global_SendQ.count++;			\
}

/* myct: init global sendQ */
#define Global_SendQ_init() {                   \
	Global_SendQ.count = 0;			\
	Global_SendQ.head = NULL;		\
	Global_SendQ.tail = Global_SendQ.head;	\
}

#define Global_SendQ_head() (Global_SendQ.head)
#define Global_SendQ_count() (Global_SendQ.count)

/* myct: this is dequeue + pop */
#define Global_SendQ_dequeue(x) {                                       \
	if(Global_SendQ.count) {					\
	        x = Global_SendQ.head;                               \
		Global_SendQ.head = Global_SendQ.head->dev.next;	\
		Global_SendQ.count--;					\
	}                                                               \
	if(!Global_SendQ.count) {					\
		Global_SendQ.tail = Global_SendQ.head;	         	\
	}								\
}

/* PERFORMANCE stuff */

#define PERF_MEASURE

typedef struct perf_struct {
    int wait_count;
    int event_q_size;
} Performance_t;

/* END PERFORMANCE stuff */


/* Connection setup OPT */
#define CONNECT_THRESHOLD 2


#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
