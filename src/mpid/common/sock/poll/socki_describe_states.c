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

int MPIDU_Socki_describe_timer_states()
{
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_READ,
		       "socki_handle_read",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_WRITE,
		       "socki_handle_write",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_SOCK_ALLOC,
		       "socki_alloc",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_SOCK_FREE,
		       "socki_free",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_EVENT_ENQUEUE,
		       "socki_event_enqueue",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_EVENT_DEQUEUE,
		       "socki_event_dequeue",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_MPIDU_SOCKI_ADJUST_IOV,
		       "socki_adjust_iov",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_READ,
		       "read",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_READV,
		       "readv",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_WRITE,
		       "write",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_WRITEV,
		       "writev",
		       get_random_color_str());
    RLOG_DescribeState(g_pRLOG,
		       MPID_STATE_POLL,
		       "poll",
		       get_random_color_str());
    return MPI_SUCCESS;
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
