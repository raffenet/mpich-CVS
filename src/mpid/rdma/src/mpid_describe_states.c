/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#ifdef HAVE_TIMING

/* This section of code is for the RLOG logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)

#include <math.h>

/* utility funcions */
#ifndef RGB
#define RGB(r,g,b)      ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#endif

static unsigned long getColorRGB(
				 double fraction, 
				 double intensity, 
				 unsigned char *r, 
				 unsigned char *g, 
				 unsigned char *b)
{
    double red, green, blue;
    double dtemp;

    fraction = fabs(modf(fraction, &dtemp));
    
    if (intensity > 2.0)
	intensity = 2.0;
    if (intensity < 0.0)
	intensity = 0.0;
    
    dtemp = 1.0/6.0;
    
    if (fraction < 1.0/6.0)
    {
	red = 1.0;
	green = fraction / dtemp;
	blue = 0.0;
    }
    else
    {
	if (fraction < 1.0/3.0)
	{
	    red = 1.0 - ((fraction - dtemp) / dtemp);
	    green = 1.0;
	    blue = 0.0;
	}
	else
	{
	    if (fraction < 0.5)
	    {
		red = 0.0;
		green = 1.0;
		blue = (fraction - (dtemp*2.0)) / dtemp;
	    }
	    else
	    {
		if (fraction < 2.0/3.0)
		{
		    red = 0.0;
		    green = 1.0 - ((fraction - (dtemp*3.0)) / dtemp);
		    blue = 1.0;
		}
		else
		{
		    if (fraction < 5.0/6.0)
		    {
			red = (fraction - (dtemp*4.0)) / dtemp;
			green = 0.0;
			blue = 1.0;
		    }
		    else
		    {
			red = 1.0;
			green = 0.0;
			blue = 1.0 - ((fraction - (dtemp*5.0)) / dtemp);
		    }
		}
	    }
	}
    }
    
    if (intensity > 1)
    {
	intensity = intensity - 1.0;
	red = red + ((1.0 - red) * intensity);
	green = green + ((1.0 - green) * intensity);
	blue = blue + ((1.0 - blue) * intensity);
    }
    else
    {
	red = red * intensity;
	green = green * intensity;
	blue = blue * intensity;
    }
    
    *r = (unsigned char)(red * 255.0);
    *g = (unsigned char)(green * 255.0);
    *b = (unsigned char)(blue * 255.0);

    return RGB(*r,*g,*b);
}

static unsigned long random_color(
				  unsigned char *r, 
				  unsigned char *g, 
				  unsigned char *b)
{
    double d1, d2;

    d1 = (double)rand() / (double)RAND_MAX;
    d2 = (double)rand() / (double)RAND_MAX;

    return getColorRGB(d1, d2 + 0.5, r, g, b);
}

static char random_color_str[40];
static char *get_random_color_str()
{
    unsigned char r,g,b;
    random_color(&r, &g, &b);
    sprintf(random_color_str, "%3d %3d %3d", (int)r, (int)g, (int)b);
    return random_color_str;
}

int MPIDU_Describe_timer_states()
{
    /* describe the states used in the ch3/src directory */

    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPI_ACCUMULATE,
		       "MPI_Accumulate",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPI_GET,
		       "MPI_Get",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPI_PUT,
		       "MPI_Put",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_IRSEND,
		       "MPID_Irsend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_ISEND,
		       "MPID_Isend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_ISSEND,
		       "MPID_Issend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_PROBE,
		       "MPID_Probe",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_PUT,
		       "MPID_PUT",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_RECV,
		       "MPID_Recv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_RSEND,
		       "MPID_Rsend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_SEND,
		       "MPID_Send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_SSEND,
		       "MPID_Ssend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_CREATE,
		       "MPID_Win_create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_COMPLETE,
		       "MPID_Win_complete",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_FENCE,
		       "MPID_Win_fence",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_FREE,
		       "MPID_Win_free",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_LOCK,
		       "MPID_Win_lock",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_UNLOCK,
		       "MPID_Win_unlock",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_POST,
		       "MPID_Win_post",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_WIN_WAIT,
		       "MPID_Win_wait",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_CREATE_REQUEST,
		       "Create_request",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_ABORT,
		       "MPID_Abort",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_CANCEL_RECV,
		       "MPID_Cancel_recv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_CANCEL_SEND,
		       "MPID_Cancel_send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_IPROBE,
		       "MPID_Iprobe",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_IRECV,
		       "MPID_Irecv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_SEND_INIT,
		       "MPID_Send_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_BSEND_INIT,
		       "MPID_Bsend_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_RSEND_INIT,
		       "MPID_Rsend_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_SSEND_INIT,
		       "MPID_Ssend_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_RECV_INIT,
		       "MPID_Recv_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_STARTALL,
		       "MPID_Startall",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_BARRIER,
		       "MPIDI_Barrier",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_HANDLE_UNORDERED_RECV_PKT,
		       "MPIDI_CH3U_Handle_unordered_recv_pkt",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_HANDLE_ORDERED_RECV_PKT,
		       "MPIDI_CH3U_Handle_ordered_recv_pkt",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ,
		       "MPIDI_CH3U_Handle_recv_req",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_HANDLE_SEND_REQ,
		       "MPIDI_CH3U_Handle_send_req",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_FU,
		       "MPIDI_CH3U_Recvq_FU",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_FDU,
		       "MPIDI_CH3U_Recvq_FDU",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_FDU_OR_AEP,
		       "MPIDI_CH3U_Recvq_FDU_or_AEP",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_DP,
		       "MPIDI_CH3U_Recvq_DP",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_FDP,
		       "MPIDI_CH3U_Recvq_FDP",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_RECVQ_FDP_OR_AEU,
		       "MPIDI_CH3U_Recvq_FDP_or_AEU",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_RECV_IOV,
		       "MPIDI_CH3U_Request_load_recv_iov",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_REQUEST_LOAD_SEND_IOV,
		       "MPIDI_CH3U_Request_load_send_iov",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_SRBUF,
		       "MPIDI_CH3U_Request_unpack_srbuf",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3U_REQUEST_UNPACK_UEBUF,
		       "MPIDI_CH3U_Request_unpack_uebuf",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPID_VCRT_CREATE,
		       "MPID_VCRT_Create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
			MPID_STATE_MPID_VCRT_ADD_REF,
		       "MPID_VCRT_Add_ref",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_VCRT_RELEASE,
		       "MPID_VCRT_Release",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_VCRT_GET_PTR,
		       "MPID_VCRT_Get_ptr",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_VCR_DUP,
		       "MPID_VCR_Dup",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_VCR_RELEASE,
		       "MPID_VCR_Release",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_VCR_GET_LPID,
		       "MPID_VCR_Get_lpid",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
			MPID_STATE_MPID_COMM_SPAWN_MULTIPLE,
		       "MPID_Comm_spawn_multiple",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3U_BUFFER_COPY,
		       "MPIDI_CH3U_buffer_copy",
		       get_random_color_str());

    /* call the channel function to describe the states found in the ch3/channels/xx/src directory */
    return CH3U_Describe_timer_states();
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
