/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

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

int Sock_describe_timer_states()
{
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_INIT,
	"mpidu_sock_init",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_FINALIZE,
	"mpidu_sock_finalize",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_CREATE_SET,
	"mpidu_sock_create_set",
        get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_DESTROY_SET,
	"mpidu_sock_destroy_set",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_LISTEN,
	"mpidu_sock_listen",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_CONNECT,
	"mpidu_sock_post_connect",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_ACCEPT,
	"mpidu_sock_accept",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_CLOSE,
	"mpidu_sock_post_close",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_WAIT,
	"mpidu_sock_wait",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_SET_USER_PTR,
	"mpidu_sock_set_user_ptr",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_READ,
	"mpidu_sock_read",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_READV,
	"mpidu_sock_readv",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_WRITE,
	"mpidu_sock_write",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_WRITEV,
	"mpidu_sock_writev",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_READ,
	"mpidu_sock_post_read",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_READV,
	"mpidu_sock_post_readv",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_WRITE,
	"mpidu_sock_post_write",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_POST_WRITEV,
	"mpidu_sock_post_writev",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_GETID,
	"mpidu_sock_getid",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_GETSETID,
	"mpidu_sock_getsetid",
	get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
	MPID_STATE_MPIDU_SOCK_NATIVE_TO_SOCK,
	"mpidu_sock_native_to_sock",
	get_random_color_str());
    return Socki_describe_timer_states();
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
