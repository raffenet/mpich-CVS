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

/* this state should be here instead of in the common describe states function? 
MPID_STATE_CREATE_REQUEST
*/
int CH3U_Describe_timer_states()
{
    /* locks */
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_COMPARE_SWAP,
	"MPIDU_Compare_swap",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDU_PROCESS_LOCK_INIT,
	"MPIDU_Process_lock_init",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDU_PROCESS_LOCK_FREE,
	"MPIDU_Process_lock_free",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDU_PROCESS_LOCK,
	"MPIDU_Process_lock",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDU_PROCESS_UNLOCK,
	"MPIDU_Process_lock",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT,
	"MPIDU_Process_lock_busy_wait",
	get_random_color_str());
    /* end locks */
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_ALLOC,
	"MPIDI_CH3I_SHM_Alloc",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_FREE,
	"MPIDI_CH3I_SHM_Free",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_SYNC,
	"MPIDI_CH3I_SHM_Get_mem_sync",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM,
	"MPIDI_CH3I_SHM_Release_mem",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDI_CH3I_SHM_POST_READ,
	"MPIDI_CH3I_SHM_post_read",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDI_CH3I_SHM_POST_READV,
	"MPIDI_CH3I_SHM_post_readv",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_WRITE,
	"MPIDI_CH3I_SHM_write",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_WRITEV,
	"MPIDI_CH3I_SHM_writev",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_WAIT,
	"shm_wait",
	get_random_color_str());
    /*
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_INIT,
	"MPIDI_CH3I_SHM_init",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_FINALIZE,
	"MPIDI_CH3I_SHM_finalize",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3I_SHM_SET_USER_PTR,
	"MPIDI_CH3I_SHM_set_user_ptr",
	get_random_color_str());
	*/
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_SHMI_BUFFER_UNEX_READ,
	"shmi_buffer_unex_read",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_SHMI_READ_UNEX,
	"shmi_read_unex",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_SHMI_READV_UNEX,
	"shmi_readv_unex",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_COMM_SPAWN,
	"mpidi_comm_spawn",
	get_random_color_str());
    /*
    RLOG_DescribeState(g_pRLOG, 
	MPID_STATE_MPIDI_CH3I_SETUP_CONNECTIONS,
	"MPIDI_CH3I_Setup_connections",
	get_random_color_str());
	*/
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDI_CH3_CANCEL_SEND,
	"MPIDI_CH3_Cancel_send",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
        MPID_STATE_MPIDU_YIELD,
	"MPIDU_Yield",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SLEEP_YIELD,
	"MPIDU_Sleep_yield",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_HANDLE_READ,
		       "handle_read",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_HANDLE_WRITTEN,
		       "handle_written",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_IREAD,
		       "MPIDI_CH3_Iread",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_ISEND,
		       "MPIDI_CH3_Isend",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_ISENDV,
		       "MPIDI_CH3_Isendv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_ISTARTMSG,
		       "MPIDI_CH3_iStartMsg",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_ISTARTMSGV,
		       "MPIDI_CH3_iStartMsgv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_IWRITE,
		       "MPIDI_CH3_iWrite",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS,
		       "MPIDI_CH3_Progress",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE,
		       "MPIDI_CH3_Progress_finalize",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_INIT,
		       "MPIDI_CH3_Progress_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_POKE,
		       "MPIDI_CH3_Progress_poke",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF,
		       "MPIDI_CH3_Request_add_ref",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_CREATE,
		       "MPIDI_CH3_Request_create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_DESTROY,
		       "MPIDI_CH3_Request_destroy",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF,
		       "MPIDI_CH3_Request_release_ref",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV,
		       "MPIDI_CH3I_Request_adjust_iov",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_POST_PKT_RECV,
		       "post_pkt_recv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_POST_PKT_SEND,
		       "post_pkt_send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_POST_QUEUED_SEND,
		       "post_queued_send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_UPDATE_REQUEST,
		       "update_request",
		       get_random_color_str());
    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
