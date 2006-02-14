#ifndef _MPID_NEM_H
#define _MPID_NEM_H

/* #define ENABLED_CHECKPOINTING 1 */

#include "mpid_nem_defs.h"
#include "mpid_nem_queue.h"
#include "mpid_nem_memdefs.h"
#include "mpid_nem_fbox.h"
#include <sys/uio.h>

#ifdef PAPI_MONITOR
#include "my_papi_defs.h"
#include "rdtsc.h"		
#endif /* PAPI_MONITOR */

int MPID_nem_init (int rank, MPIDI_PG_t *pg_p);
int _MPID_nem_init (int rank, MPIDI_PG_t *pg_p, int ckpt_restart);
int MPID_nem_finalize (void);
int MPID_nem_ckpt_shutdown (void);
void MPID_nem_barrier_init(MPID_nem_barrier_t *barrier_region);
void MPID_nem_barrier(int, int);
void MPID_nem_seg_create(MPID_nem_seg_ptr_t, int, int num_local, int local_rank, MPIDI_PG_t *pg_p);
void MPID_nem_seg_alloc( MPID_nem_seg_ptr_t, MPID_nem_seg_info_ptr_t, int);
void MPID_nem_check_alloc(int);
static inline void MPID_nem_waitforlock(MPID_nem_fbox_common_ptr_t pbox, int value, int count);
static inline int MPID_nem_islocked (MPID_nem_fbox_common_ptr_t pbox, int value, int count);
int MPID_nem_vc_init (MPIDI_VC_t *vc);
int MPID_nem_get_business_card (char *value, int length);
int MPID_nem_connect_to_root (const char *port_name, const int lpid);

#define MPID_NEM__MPICH2_HEADER_LEN sizeof(MPIDI_CH3_Pkt_t)
#define MPID_NEM__BYPASS_Q_MAX_VAL  ((MPID_NEM_MPICH2_DATA_LEN) - (MPID_NEM__MPICH2_HEADER_LEN))

#define MPID_NEM_MPICH2_SUCCESS 0
#define MPID_NEM_MPICH2_FAILURE 1
#define MPID_NEM_MPICH2_AGAIN   2  /* try again */

void MPID_nem_mpich2_init (int ckpt_restart);
/* int MPID_nem_mpich2_release_fbox (MPID_nem_cell_t *cell); */
#define MPID_nem_mpich2_release_fbox(cell) (MPID_nem_mem_region.mailboxes.in[(cell)->pkt.mpich2.source]->mpich2.flag.value = 0, \
					    MPID_NEM_MPICH2_SUCCESS)

int MPID_nem_ckpt_init (int ckpt_restart);
void MPID_nem_ckpt_finalize (void);
void MPID_nem_ckpt_maybe_take_checkpoint (void);
void MPID_nem_ckpt_got_marker (MPID_nem_cell_ptr_t *cell, int *in_fbox);
void MPID_nem_ckpt_log_message (MPID_nem_cell_ptr_t cell);
void MPID_nem_ckpt_send_markers (void);
int MPID_nem_ckpt_replay_message (MPID_nem_cell_ptr_t *cell);
void MPID_nem_ckpt_free_msg_log (void);


/* one-sided */

typedef struct MPID_nem_mpich2_win
{
    char filename[MPID_NEM_MAX_FNAME_LEN]; /* shared filename */
    int proc;                     /* rank of owner */
    void *home_address;           /* address of window at owner */
    int len;                      /* size of window */
    void *local_address;          /* address of window at this process */
}
MPID_nem_mpich2_win_t;

int MPID_nem_mpich2_alloc_win (void **buf, int len, MPID_nem_mpich2_win_t **win);
int MPID_nem_mpich2_free_win (MPID_nem_mpich2_win_t *win);
int MPID_nem_mpich2_attach_win (void **buf, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_detach_win (MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_serialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t *win, int *len);
int MPID_nem_mpich2_deserialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t **win);

int MPID_nem_mpich2_win_put (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_get (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win);

int MPID_nem_mpich2_register_memory (void *buf, int len);
int MPID_nem_mpich2_deregister_memory (void *buf, int len);

int MPID_nem_mpich2_put (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr);
int MPID_nem_mpich2_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		       int *completion_ctr);
int MPID_nem_mpich2_get (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr);
int MPID_nem_mpich2_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		       int *completion_ctr);

     
#define MPID_nem_mpich2_win_put_val(val, d_buf, remote_win) do {			\
    char *_d_buf = d_buf;								\
											\
    _d_buf += (char *)(remote_win)->local_address - (char *)(remote_win)->home_address;	\
											\
    *(typeof (val) *)_d_buf = val;							\
} while (0)
     
#define MPID_nem_mpich2_win_get_val(s_buf, val, remote_win) do {			\
    char *_s_buf = s_buf;								\
											\
    _s_buf += (char *)(remote_win)->local_address - (char *)(remote_win)->home_address;	\
											\
    *(val) = *(typeof (*(val)) *)_s_buf;						\
} while (0)



#if 0
/* large message transfer functions */
int MPID_nem_mpich2_lmt_send_pre (struct iovec *iov, int n_iov, MPIDI_VC_t *dest_vc, struct iovec *cookie);
int MPID_nem_mpich2_lmt_recv_pre (struct iovec *iov, int n_iov, MPIDI_VC_t *src_vc, struct iovec *cookie);
int MPID_nem_mpich2_lmt_start_send (MPIDI_VC_t *dest_vc, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int MPID_nem_mpich2_lmt_start_recv (MPIDI_VC_t *src_vc, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int MPID_nem_mpich2_lmt_send_post (MPIDI_VC_t *dest_vc, struct iovec cookie);
int MPID_nem_mpich2_lmt_recv_post (MPIDI_VC_t *src_vc, struct iovec cookie);
#endif

/* #define DO_PAPI2(x) x */
static inline void
MPID_nem_waitforlock (MPID_nem_fbox_common_ptr_t pbox, int value, int count)
{
    DO_PAPI2 (PAPI_reset (PAPI_EventSet));
    while (pbox->flag.value != value)
    {
	if(--count == 0)
	{
	    sched_yield();
	}
	DO_PAPI2 (PAPI_reset (PAPI_EventSet));
    }  
    DO_PAPI2 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues8));
}

static inline int
MPID_nem_islocked (MPID_nem_fbox_common_ptr_t pbox, int value, int count)
{
    while (pbox->flag.value != value && --count == 0)
    {
    }
    return (pbox->flag.value != value);
}

#if MPID_NEM_INLINE
#include "mpid_nem_nets.h"
#include "mpid_nem_inline.h"
#else //MPID_NEM_INLINE
int MPID_nem_mpich2_send (void* buf, int size, MPIDI_VC_t *vc);
int MPID_nem_mpich2_send_ckpt_marker (unsigned short wave, MPIDI_VC_t *vc);
int MPID_nem_mpich2_send_header (void* buf, int size, MPIDI_VC_t *vc);
int MPID_nem_mpich2_sendv (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc);
int MPID_nem_mpich2_sendv_header (struct iovec **iov, int *n_iov, MPIDI_VC_t *vc);
int MPID_nem_mpich2_test_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);
int MPID_nem_mpich2_test_recv_wait (MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);
int recv_seqno_matches (MPID_nem_queue_ptr_t qhead) ;
int MPID_nem_mpich2_blocking_recv (MPID_nem_cell_ptr_t *cell, int *in_fbox);
int MPID_nem_mpich2_release_cell (MPID_nem_cell_ptr_t cell, MPIDI_VC_t *vc);
int MPID_nem_mpich2_enqueue_fastbox (int local_rank);
int MPID_nem_mpich2_dequeue_fastbox (int local_rank);
#endif //MPID_NEM_INLINE

#endif

