/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef USE_MPE_PROFILING

#include <math.h>

MM_Timer_state g_timer_state[MM_NUM_TIMER_STATES];

int g_prof_rank, g_prof_size;
static char g_prof_filename[256];

/*
#ifndef RGB
#define WORD     unsigned short
#define DWORD    unsigned long
#define BYTE     unsigned char
#define COLORREF unsigned long
#define RGB      ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
*/
#ifndef RGB
#define RGB      ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#endif

static unsigned long getColorRGB(double fraction, double intensity, unsigned char *r, unsigned char *g, unsigned char *b)
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

static unsigned long random_color(unsigned char *r, unsigned char *g, unsigned char *b)
{
    double d1, d2;

    d1 = (double)rand() / (double)RAND_MAX;
    d2 = (double)rand() / (double)RAND_MAX;

    return getColorRGB(d1, d2 + 0.5, r, g, b);
}

#define MM_PROF_FUNC(a) g_timer_state[ a##_INDEX ].name = #a

static void init_state_strings()
{
    MM_PROF_FUNC(MM_OPEN_PORT);
    MM_PROF_FUNC(MM_CLOSE_PORT);
    MM_PROF_FUNC(MM_ACCEPT);
    MM_PROF_FUNC(MM_CONNECT);
    MM_PROF_FUNC(MM_SEND);
    MM_PROF_FUNC(MM_RECV);
    MM_PROF_FUNC(MM_CLOSE);
    MM_PROF_FUNC(MM_REQUEST_ALLOC);
    MM_PROF_FUNC(MM_REQUEST_FREE);
    MM_PROF_FUNC(MM_CAR_INIT);
    MM_PROF_FUNC(MM_CAR_FINALIZE);
    MM_PROF_FUNC(MM_CAR_ALLOC);
    MM_PROF_FUNC(MM_CAR_FREE);
    MM_PROF_FUNC(MM_VC_INIT);
    MM_PROF_FUNC(MM_VC_FINALIZE);
    MM_PROF_FUNC(MM_VC_FROM_COMMUNICATOR);
    MM_PROF_FUNC(MM_VC_FROM_CONTEXT);
    MM_PROF_FUNC(MM_VC_ALLOC);
    MM_PROF_FUNC(MM_VC_CONNECT_ALLOC);
    MM_PROF_FUNC(MM_VC_FREE);
    MM_PROF_FUNC(MM_CHOOSE_BUFFER);
    MM_PROF_FUNC(MM_RESET_CARS);
    MM_PROF_FUNC(MM_GET_BUFFERS_TMP);
    MM_PROF_FUNC(MM_RELEASE_BUFFERS_TMP);
    MM_PROF_FUNC(MM_GET_BUFFERS_VEC);
    MM_PROF_FUNC(VEC_BUFFER_INIT);
    MM_PROF_FUNC(TMP_BUFFER_INIT);
    MM_PROF_FUNC(MM_POST_RECV);
    MM_PROF_FUNC(MM_POST_SEND);
    MM_PROF_FUNC(MM_CQ_TEST);
    MM_PROF_FUNC(MM_CQ_WAIT);
    MM_PROF_FUNC(MM_CQ_ENQUEUE);
    MM_PROF_FUNC(MM_CREATE_POST_UNEX);
    MM_PROF_FUNC(MM_POST_UNEX_RNDV);
    MM_PROF_FUNC(XFER_INIT);
    MM_PROF_FUNC(XFER_RECV_OP);
    MM_PROF_FUNC(XFER_RECV_MOP_OP);
    MM_PROF_FUNC(XFER_RECV_FORWARD_OP);
    MM_PROF_FUNC(XFER_RECV_MOP_FORWARD_OP);
    MM_PROF_FUNC(XFER_FORWARD_OP);
    MM_PROF_FUNC(XFER_SEND_OP);
    MM_PROF_FUNC(XFER_REPLICATE_OP);
    MM_PROF_FUNC(XFER_START);
    MM_PROF_FUNC(TCP_INIT);
    MM_PROF_FUNC(TCP_FINALIZE);
    MM_PROF_FUNC(TCP_ACCEPT_CONNECTION);
    MM_PROF_FUNC(TCP_GET_BUSINESS_CARD);
    MM_PROF_FUNC(TCP_CAN_CONNECT);
    MM_PROF_FUNC(TCP_POST_CONNECT);
    MM_PROF_FUNC(TCP_POST_READ);
    MM_PROF_FUNC(TCP_MERGE_WITH_UNEXPECTED);
    MM_PROF_FUNC(TCP_POST_WRITE);
    MM_PROF_FUNC(TCP_MAKE_PROGRESS);
    MM_PROF_FUNC(TCP_CAR_ENQUEUE);
    MM_PROF_FUNC(TCP_CAR_DEQUEUE);
    MM_PROF_FUNC(TCP_RESET_CAR);
    MM_PROF_FUNC(TCP_POST_READ_PKT);
    MM_PROF_FUNC(TCP_READ);
    MM_PROF_FUNC(TCP_WRITE);
    MM_PROF_FUNC(TCP_READ_SHM);
    MM_PROF_FUNC(TCP_READ_VIA);
    MM_PROF_FUNC(TCP_READ_VIA_RDMA);
    MM_PROF_FUNC(TCP_READ_VEC);
    MM_PROF_FUNC(TCP_READ_TMP);
    MM_PROF_FUNC(TCP_READ_CONNECTING);
    MM_PROF_FUNC(TCP_WRITE_SHM);
    MM_PROF_FUNC(TCP_WRITE_VIA);
    MM_PROF_FUNC(TCP_WRITE_VIA_RDMA);
    MM_PROF_FUNC(TCP_WRITE_VEC);
    MM_PROF_FUNC(TCP_WRITE_TMP);
    MM_PROF_FUNC(TCP_STUFF_VECTOR_SHM);
    MM_PROF_FUNC(TCP_STUFF_VECTOR_VIA);
    MM_PROF_FUNC(TCP_STUFF_VECTOR_VIA_RDMA);
    MM_PROF_FUNC(TCP_STUFF_VECTOR_VEC);
    MM_PROF_FUNC(TCP_STUFF_VECTOR_TMP);
    MM_PROF_FUNC(TCP_WRITE_AGGRESSIVE);
}

int prof_init(int rank, int size)
{
    int i;
    unsigned char r,g,b;

    g_prof_rank = rank;
    g_prof_size = size;

    MPE_Init_log();

    for (i=0; i<MM_NUM_TIMER_STATES; i++)
    {
	g_timer_state[i].num_calls = 0;
	g_timer_state[i].in_id = MPE_Log_get_event_number();
	g_timer_state[i].out_id = MPE_Log_get_event_number();
	g_timer_state[i].color = random_color(&r, &g, &b);
	sprintf(g_timer_state[i].color_str, "%d %d %d", (int)r, (int)g, (int)b);
    }
    init_state_strings();

    strcpy(g_prof_filename, "mpid_prof");
    
    MPE_Start_log();

    return MPI_SUCCESS;
}

int prof_finalize()
{
    int i;
    
    if (g_prof_rank == 0) 
    {
	printf( "Writing logfile %s.\n", g_prof_filename);fflush(stdout);
	for (i=0; i<MM_NUM_TIMER_STATES; i++) 
	{
	    MPE_Describe_state( 
		g_timer_state[i].in_id, 
		g_timer_state[i].out_id, 
		g_timer_state[i].name,
		g_timer_state[i].color_str );
	}
    }
    
    MPE_Finish_log( g_prof_filename );
    if (g_prof_rank == 0)
    {
	printf("finished.\n");fflush(stdout);
    }
    
    return MPI_SUCCESS;
}

#endif
