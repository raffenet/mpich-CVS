/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "pmi.h"
#include "sock.h"

#define USE_GCC_X86_CYCLE_ASM 1
#define USE_WIN_X86_CYCLE_ASM 2

#if MPICH_CPU_TICK_TYPE == USE_GCC_X86_CYCLE_ASM
/* This cycle counter is the read time stamp (rdtsc) instruction with gcc asm */
#define MPID_CPU_TICK(var_ptr) \
{ \
    __asm__ __volatile__  ( "cpuid ; rdtsc ; mov %%edx,%1 ; mov %%eax,%0" \
                            : "=m" (*((char *) (var_ptr))), \
                              "=m" (*(((char *) (var_ptr))+4)) \
                            :: "eax", "ebx", "ecx", "edx" ); \
}
typedef long long MPID_CPU_Tick_t;

#elif MPICH_CPU_TICK_TYPE == USE_WIN_X86_CYCLE_ASM
/* This cycle counter is the read time stamp (rdtsc) instruction with Microsoft asm */
#define MPID_CPU_TICK(var_ptr) \
{ \
    register int *f1 = (int*)var_ptr; \
    __asm cpuid \
    __asm rdtsc \
    __asm mov ecx, f1 \
    __asm mov [ecx], eax \
    __asm mov [ecx + TYPE int], edx \
}
typedef unsigned __int64 MPID_CPU_Tick_t;

#else
#error CPU tick instruction needed to count progress time
#endif

extern volatile unsigned int MPIDI_CH3I_progress_completions;

typedef enum conn_state
{
    CONN_STATE_UNCONNECTED,
    CONN_STATE_LISTENING,
    CONN_STATE_CONNECTING,
    CONN_STATE_OPEN_CSEND,
    CONN_STATE_OPEN_CRECV,
    CONN_STATE_OPEN_LRECV,
    CONN_STATE_OPEN_LSEND,
    CONN_STATE_CONNECTED,
    CONN_STATE_CLOSING,
    CONN_STATE_CLOSED,
    CONN_STATE_FAILED
} conn_state;

typedef struct MPIDI_CH3I_Connection
{
    MPIDI_VC * vc;
    sock_t sock;
    enum conn_state state;
    MPID_Request * send_active;
    MPID_Request * recv_active;
    MPIDI_CH3_Pkt_t pkt;
} MPIDI_CH3I_Connection_t;

extern sock_set_t sock_set;
extern int listener_port;
extern MPIDI_CH3I_Connection_t * listener_conn;

extern int shutting_down;

MPIDI_CH3I_Connection_t * connection_alloc(void);
void connection_free(MPIDI_CH3I_Connection_t * conn);
void connection_post_sendq_req(MPIDI_CH3I_Connection_t * conn);
void connection_post_send_pkt(MPIDI_CH3I_Connection_t * conn);
void connection_post_recv_pkt(MPIDI_CH3I_Connection_t * conn);
void connection_send_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);
void connection_recv_fail(MPIDI_CH3I_Connection_t * conn, int sock_errno);

int handle_sock_op(sock_event_t *event_ptr);

int handle_shm_read(MPIDI_VC *vc, int nb);
int MPIDI_CH3I_SHM_write_progress(MPIDI_VC * vc);
