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

int CH3U_Describe_timer_states()
{
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3_CANCEL_SEND,
		       "MPIDI_CH3_Cancel_send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3_COMM_SPAWN,
		       "MPIDI_CH3_Comm_spawn",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_FINALIZE,
		       "MPIDI_CH3_Finalize",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_INIT,
		       "MPIDI_CH3_Init",
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
		       MPID_STATE_MPIDI_CH3_PROGRESS_TEST,
		       "MPIDI_CH3_Progress_test",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_START,
		       "MPIDI_CH3_Progress_start",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_END,
		       "MPIDI_CH3_Progress_end",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS,
		       "MPIDI_CH3_Progress",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_POKE,
		       "MPIDI_CH3_Progress_poke",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_CREATE,
		       "MPIDI_CH3_Request_create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_ADD_REF,
		       "MPIDI_CH3_Request_add_ref",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_RELEASE_REF,
		       "MPIDI_CH3_Request_release_ref",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_REQUEST_DESTROY,
		       "MPIDI_CH3_Request_destroy",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3I_LISTENER_GET_PORT,
		       "MPIDI_CH3I_Listener_get_port",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_FINALIZE,
		       "MPIDI_CH3I_Progress_finalize",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3_PROGRESS_INIT,
		       "MPIDI_CH3I_Progress_init",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3I_VC_POST_CONNECT,
		       "MPIDI_CH3I_VC_post_connect",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3I_VC_POST_READ,
		       "MPIDI_CH3I_SSM_VC_post_read",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG, 
		       MPID_STATE_MPIDI_CH3I_VC_POST_WRITE,
		       "MPIDI_CH3I_SSM_VC_post_write",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3U_BUFFER_COPY,
		       "MPIDI_CH3U_buffer_copy",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_UPDATE_REQUEST,
		       "update_request",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_ALLOC,
		       "connection_alloc",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_FREE,
		       "connection_free",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_POST_SENDQ_REQ,
		       "connection_post_sendq_req",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_POST_SEND_PKT,
		       "connection_post_send_pkt",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_POST_RECV_PKT,
		       "connection_post_recv_pkt",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_SEND_FAIL,
		       "connection_send_fail",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_CONNECTION_RECV_FAIL,
		       "connection_recv_fail",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_GET_BUSINESS_CARD,
		       "MPIDI_CH3I_Get_business_card",
		       get_random_color_str());
    /*
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_BOOTSTRAPQWNDPROC,
		       "BOOTSTRAPQWNDPROC",
		       get_random_color_str());
    */
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_GET_NEXT_BOOTSTRAP_MSG,
		       "Get_next_bootstrap_msg",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE,
		       "MPIDI_CH3I_BootstrapQ_create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSRAPQ_TOSTRING,
		       "MPIDI_CH3I_BootstrapQ_tostring",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DESTROY,
		       "MPIDI_CH3I_BootstrapQ_destroy",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_ATTACH,
		       "MPIDI_CH3I_BootstrapQ_attach",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_DETACH,
		       "MPIDI_CH3I_BootstrapQ_detach",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_SEND_MSG,
		       "MPIDI_CH3I_BootstrapQ_send_msg",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_RECV_MSG,
		       "MPIDI_CH3I_BootstrapQ_recv_msg",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_NAMED,
		       "MPIDI_CH3I_BootstrapQ_create_named",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_CREATE_UNIQUE_NAME,
		       "MPIDI_CH3I_BootstrapQ_create_unique_name",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_BOOTSTRAPQ_UNLINK,
		       "MPIDI_CH3I_BOOTSTRAPQ_UNLINK",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_HANDLE_SOCK_OP,
		       "handle_sock_op",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_HANDLE_SHM_READ,
		       "handle_shm_read",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_HANDLE_SHM_WRITTEN,
		       "handle_shm_written",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_REQUEST_ADJUST_IOV,
		       "MPIDI_CH3I_request_adjust_iov",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MEMCPY,
		       "memcpy",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_SHM_GET_MEM,
		       "MPIDI_CH3I_SHM_Get_mem",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_SHM_GET_MEM_NAMED,
		       "MPIDI_CH3I_SHM_Get_mem_named",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_SHM_UNLINK_MEM,
		       "MPIDI_CH3I_SHM_Unlink_mem",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_SHM_ATTACH_TO_MEM,
		       "MPIDI_CH3I_SHM_Attach_to_mem",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_SHM_RELEASE_MEM,
		       "MPIDI_CH3I_SHM_Release_mem",
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
		       "MPIDI_CH3I_SHM_wait",
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
		       MPID_STATE_MPIDU_YIELD,
		       "MPIDU_Yield",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_MQSHM_CREATE,
		       "MPIDI_CH3I_mqshm_create",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_MQSHM_CLOSE,
		       "MPIDI_CH3I_mqshm_close",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_MQSHM_UNLINK,
		       "MPIDI_CH3I_mqshm_unlink",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_MPIDI_CH3I_MQSHM_SEND,
		       "MPIDI_MPIDI_CH3I_mqshm_send",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDI_CH3I_MQSHM_RECEIVE,
		       "MPIDI_CH3I_mqshm_receive",
		       get_random_color_str());
return Sock_describe_timer_states();
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
