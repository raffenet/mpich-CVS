/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_MPID_H_INCLUDED)
#define MPICH_MPIDI_CH3_MPID_H_INCLUDED
#define HAVE_CH3_PRELOAD

typedef struct MPIDI_CH3_Funcs {
    int (*Init)( int, MPIDI_PG_t *, int );
    int (*Finalize)(void);
    int (*iSend)( MPIDI_VC_t *, MPID_Request *, MPIDI_CH3_Pkt_t *, int );
    int (*iSendv)( MPIDI_VC_t *, MPID_Request *, MPID_IOV *, int );
    int (*iStartMsg)( MPIDI_VC_t *, MPIDI_CH3_Pkt_t *, int, MPID_Request ** );
    int (*iStartMsgv)( MPIDI_VC_t *, MPID_IOV *, int, MPID_Request ** );
    int (*Progress_test)( void );
    int (*Progress_wait)( MPID_Progress_state * );

    int (*VC_Init)( struct MPIDI_VC * );
    int (*PG_Init)( struct MPIDI_PG * );
    int (*Connection_terminate)( MPIDI_VC_t * );

    /* These are used in support of dynamic process features */
    int (*PortFnsInit)( MPIDI_PortFns * );
    int (*Connect_to_root)( const char *, MPIDI_VC_t ** );
    int (*Get_business_card)( int, char *, int );

    /* VC_GetStateString is only used for debugging, and may be a no-op */
    const char *(*VC_GetStateString)( struct MPIDI_VC * );
} MPIDI_CH3_Funcs;

/* This is a shared structure that defines the CH3 functions.  It is 
   defined to work with the MPIU_CALL macro */
extern struct MPIDI_CH3_Funcs MPIU_CALL_MPIDI_CH3;

#endif /* MPICH_MPIDI_CH3_MPID_H_INCLUDED */
