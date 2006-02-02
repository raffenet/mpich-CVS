/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef CH3USOCK_H_INCLUDED
#define CH3USOCK_H_INCLUDED

/* These implement the connection state machine for socket connections */
int MPIDI_CH3_Sockconn_handle_accept_event( void );
int MPIDI_CH3_Sockconn_handle_connect_event( MPIDI_CH3I_Connection_t *, int );
int MPIDI_CH3_Sockconn_handle_close_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_conn_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_connopen_event( MPIDI_CH3I_Connection_t * );
int MPIDI_CH3_Sockconn_handle_connwrite( MPIDI_CH3I_Connection_t * );

#endif
