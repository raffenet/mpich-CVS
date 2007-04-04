/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_MPID_H_INCLUDED)
#define MPICH_MPIDI_CH3_MPID_H_INCLUDED
#define HAVE_CH3_PRELOAD

/* Define the ABI version for the channel interface.  This will
   be checked with the version in the dynamically loaded library */
#define MPICH_CH3ABIVERSION "1.1"

/* The void * argument in iSend and iStartMsg is really a pointer to 
   a message packet. However, to simplify the definition of "private" 
   packets, there is no universal packet type (there is the 
   MPIDI_CH3_Pkt_t, but the "extension" packets are not of this type).
   Using a void * pointer avoids unnecessary complaints from the
   compilers */
typedef struct MPIDI_CH3_Funcs {
    int (*Init)( int, MPIDI_PG_t *, int );
    int (*InitCompleted)(void);
    int (*Finalize)(void);
    int (*iSend)( MPIDI_VC_t *, MPID_Request *, void *, int );
    int (*iSendv)( MPIDI_VC_t *, MPID_Request *, MPID_IOV *, int );
    int (*iStartMsg)( MPIDI_VC_t *, void *, int, MPID_Request ** );
    int (*iStartMsgv)( MPIDI_VC_t *, MPID_IOV *, int, MPID_Request ** );
    int (*Progress)( int, MPID_Progress_state * );
    /*    int (*Progress_test)( void );
	  int (*Progress_wait)( MPID_Progress_state * ); */

    int (*VC_Init)( struct MPIDI_VC * );
    int (*VC_Destroy)( struct MPIDI_VC * );
    int (*PG_Init)( struct MPIDI_PG * );
    int (*PG_Destroy)( struct MPIDI_PG * );
    int (*Connection_terminate)( MPIDI_VC_t * );
    int (*RMAFnsInit)( struct MPIDI_RMA_Ops * );

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
