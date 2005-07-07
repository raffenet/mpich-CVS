/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_TIMING

#include <math.h>


/* global variables */
#if (USE_LOGGING == MPID_LOGGING_RLOG)
RLOG_Struct *g_pRLOG = NULL;
#endif

#if (USE_LOGGING == MPID_LOGGING_DLOG)
MPID_Timer_state g_timer_state[MPID_NUM_TIMER_STATES];
DLOG_Struct *g_pDLOG = NULL;
#endif


/* utility funcions */
#ifndef RGB
#define RGB(r,g,b)      ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
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



/* This section of code is for the DLOG logging library */
/* This is temporary - it will be moved into the dlog directory */
#if (USE_LOGGING == MPID_LOGGING_DLOG)

#define NUM_X_COLORS 744
static char * g_XColors[] = {
"snow","ghost white","GhostWhite","white smoke","WhiteSmoke","gainsboro","floral white","FloralWhite","old lace","OldLace",
"linen","antique white","AntiqueWhite","papaya whip","PapayaWhip","blanched almond","BlanchedAlmond","bisque","peach puff",
"PeachPuff","navajo white","NavajoWhite","moccasin","cornsilk","ivory","lemon chiffon","LemonChiffon","seashell","honeydew",
"mint cream","MintCream","azure","alice blue","AliceBlue","lavender","lavender blush","LavenderBlush","misty rose","MistyRose",
"white","black","dark slate gray","DarkSlateGray","dark slate grey","DarkSlateGrey","dim gray","DimGray","dim grey","DimGrey",
"slate gray","SlateGray","slate grey","SlateGrey","light slate gray","LightSlateGray","light slate grey","LightSlateGrey","gray",
"grey","light grey","LightGrey","light gray","LightGray","light_gray","midnight blue","MidnightBlue","navy","navy blue","NavyBlue",
"cornflower blue","CornflowerBlue","dark slate blue","DarkSlateBlue","slate blue","SlateBlue","medium slate blue","MediumSlateBlue",
"light slate blue","LightSlateBlue","medium blue","MediumBlue","royal blue","RoyalBlue","blue","Blue","dodger blue","DodgerBlue",
"deep sky blue","DeepSkyBlue","deepskyblue","sky blue","SkyBlue","skyblue","light sky blue","LightSkyBlue","steel blue","SteelBlue",
"light steel blue","LightSteelBlue","light blue","LightBlue","powder blue","PowderBlue","pale turquoise","PaleTurquoise","dark turquoise",
"DarkTurquoise","medium turquoise","MediumTurquoise","turquoise","cyan","light cyan","LightCyan","cadet blue","CadetBlue",
"medium aquamarine","MediumAquamarine","aquamarine","dark green","DarkGreen","dark olive green","DarkOliveGreen","dark sea green",
"DarkSeaGreen","seagreen","sea green","SeaGreen","medium sea green","MediumSeaGreen","light sea green","LightSeaGreen","pale green",
"PaleGreen","spring green","SpringGreen","springgreen","lawn green","LawnGreen","green","chartreuse","medium spring green",
"MediumSpringGreen","green yellow","GreenYellow","lime green","LimeGreen","yellow green","YellowGreen","forest green","ForestGreen",
"olive drab","OliveDrab","dark khaki","DarkKhaki","khaki","pale goldenrod","PaleGoldenrod","light goldenrod yellow",
"LightGoldenrodYellow","light yellow","LightYellow","yellow","gold","light goldenrod","LightGoldenrod","goldenrod","dark goldenrod",
"DarkGoldenrod","rosy brown","RosyBrown","indian red","IndianRed","saddle brown","SaddleBrown","sienna","peru","burlywood",
"beige","wheat","sandy brown","SandyBrown","tan","chocolate","firebrick","brown","dark salmon","DarkSalmon","salmon","light salmon",
"LightSalmon","orange","dark orange","DarkOrange","coral","light coral","LightCoral","tomato","orange red","OrangeRed","red",
"hot pink","HotPink","deep pink","DeepPink","pink","light pink","LightPink","pale violet red","PaleVioletRed","maroon",
"medium violet red","MediumVioletRed","violet red","VioletRed","magenta","violet","plum","orchid","medium orchid","MediumOrchid",
"dark orchid","DarkOrchid","dark violet","DarkViolet","blue violet","BlueViolet","purple","medium purple","MediumPurple",
"thistle","snow1","snow2","snow3","snow4","seashell1","seashell2","seashell3","seashell4","AntiqueWhite1","AntiqueWhite2",
"AntiqueWhite3","AntiqueWhite4","bisque1","bisque2","bisque3","bisque4","PeachPuff1","PeachPuff2","PeachPuff3","PeachPuff4",
"NavajoWhite1","NavajoWhite2","NavajoWhite3","NavajoWhite4","LemonChiffon1","LemonChiffon2","LemonChiffon3","LemonChiffon4",
"cornsilk1","cornsilk2","cornsilk3","cornsilk4","ivory1","ivory2","ivory3","ivory4","honeydew1","honeydew2","honeydew3","honeydew4",
"LavenderBlush1","LavenderBlush2","LavenderBlush3","LavenderBlush4","MistyRose1","MistyRose2","MistyRose3","MistyRose4","azure1",
"azure2","azure3","azure4","SlateBlue1","SlateBlue2","SlateBlue3","SlateBlue4","RoyalBlue1","RoyalBlue2","RoyalBlue3",
"RoyalBlue4","blue1","blue2","blue3","blue4","DodgerBlue1","DodgerBlue2","DodgerBlue3","DodgerBlue4","SteelBlue1","SteelBlue2",
"SteelBlue3","SteelBlue4","DeepSkyBlue1","DeepSkyBlue2","DeepSkyBlue3","DeepSkyBlue4","SkyBlue1","SkyBlue2","SkyBlue3",
"SkyBlue4","LightSkyBlue1","LightSkyBlue2","LightSkyBlue3","LightSkyBlue4","SlateGray1","SlateGray2","SlateGray3","SlateGray4",
"LightSteelBlue1","LightSteelBlue2","LightSteelBlue3","LightSteelBlue4","LightBlue1","LightBlue2","LightBlue3","LightBlue4",
"LightCyan1","LightCyan2","LightCyan3","LightCyan4","PaleTurquoise1","PaleTurquoise2","PaleTurquoise3","PaleTurquoise4",
"CadetBlue1","CadetBlue2","CadetBlue3","CadetBlue4","turquoise1","turquoise2","turquoise3","turquoise4","cyan1","cyan2",
"cyan3","cyan4","DarkSlateGray1","DarkSlateGray2","DarkSlateGray3","DarkSlateGray4","aquamarine1","aquamarine2","aquamarine3",
"aquamarine4","DarkSeaGreen1","DarkSeaGreen2","DarkSeaGreen3","DarkSeaGreen4","SeaGreen1","SeaGreen2","SeaGreen3","SeaGreen4",
"PaleGreen1","PaleGreen2","PaleGreen3","PaleGreen4","SpringGreen1","SpringGreen2","SpringGreen3","SpringGreen4","green1",
"green2","green3","green4","chartreuse1","chartreuse2","chartreuse3","chartreuse4","OliveDrab1","OliveDrab2","OliveDrab3",
"OliveDrab4","DarkOliveGreen1","DarkOliveGreen2","DarkOliveGreen3","DarkOliveGreen4","khaki1","khaki2","khaki3","khaki4",
"LightGoldenrod1","LightGoldenrod2","LightGoldenrod3","LightGoldenrod4","LightYellow1","LightYellow2","LightYellow3",
"LightYellow4","yellow1","yellow2","yellow3","yellow4","gold1","gold2","gold3","gold4","goldenrod1","goldenrod2","goldenrod3",
"goldenrod4","DarkGoldenrod1","DarkGoldenrod2","DarkGoldenrod3","DarkGoldenrod4","RosyBrown1","RosyBrown2","RosyBrown3",
"RosyBrown4","IndianRed1","IndianRed2","IndianRed3","IndianRed4","sienna1","sienna2","sienna3","sienna4","burlywood1",
"burlywood2","burlywood3","burlywood4","wheat1","wheat2","wheat3","wheat4","tan1","tan2","tan3","tan4","chocolate1","chocolate2",
"chocolate3","chocolate4","firebrick1","firebrick2","firebrick3","firebrick4","brown1","brown2","brown3","brown4","salmon1",
"salmon2","salmon3","salmon4","LightSalmon1","LightSalmon2","LightSalmon3","LightSalmon4","orange1","orange2","orange3",
"orange4","DarkOrange1","DarkOrange2","DarkOrange3","DarkOrange4","coral1","coral2","coral3","coral4","tomato1","tomato2",
"tomato3","tomato4","OrangeRed1","OrangeRed2","OrangeRed3","OrangeRed4","red1","red2","red3","red4","DeepPink1","DeepPink2",
"DeepPink3","DeepPink4","HotPink1","HotPink2","HotPink3","HotPink4","pink1","pink2","pink3","pink4","LightPink1","LightPink2",
"LightPink3","LightPink4","PaleVioletRed1","PaleVioletRed2","PaleVioletRed3","PaleVioletRed4","maroon1","maroon2","maroon3",
"maroon4","VioletRed1","VioletRed2","VioletRed3","VioletRed4","magenta1","magenta2","magenta3","magenta4","orchid1","orchid2",
"orchid3","orchid4","plum1","plum2","plum3","plum4","MediumOrchid1","MediumOrchid2","MediumOrchid3","MediumOrchid4","DarkOrchid1",
"DarkOrchid2","DarkOrchid3","DarkOrchid4","purple1","purple2","purple3","purple4","MediumPurple1","MediumPurple2","MediumPurple3",
"MediumPurple4","thistle1","thistle2","thistle3","thistle4","gray0","grey0","gray1","grey1","gray2","grey2","gray3","grey3",
"gray4","grey4","gray5","grey5","gray6","grey6","gray7","grey7","gray8","grey8","gray9","grey9","gray10","grey10","gray11",
"grey11","gray12","grey12","gray13","grey13","gray14","grey14","gray15","grey15","gray16","grey16","gray17","grey17","gray18",
"grey18","gray19","grey19","gray20","grey20","gray21","grey21","gray22","grey22","gray23","grey23","gray24","grey24","gray25",
"grey25","gray26","grey26","gray27","grey27","gray28","grey28","gray29","grey29","gray30","grey30","gray31","grey31","gray32",
"grey32","gray33","grey33","gray34","grey34","gray35","grey35","gray36","grey36","gray37","grey37","gray38","grey38","gray39",
"grey39","gray40","grey40","gray41","grey41","gray42","grey42","gray43","grey43","gray44","grey44","gray45","grey45","gray46",
"grey46","gray47","grey47","gray48","grey48","gray49","grey49","gray50","grey50","gray51","grey51","gray52","grey52","gray53",
"grey53","gray54","grey54","gray55","grey55","gray56","grey56","gray57","grey57","gray58","grey58","gray59","grey59","gray60",
"grey60","gray61","grey61","gray62","grey62","gray63","grey63","gray64","grey64","gray65","grey65","gray66","grey66","gray67",
"grey67","gray68","grey68","gray69","grey69","gray70","grey70","gray71","grey71","gray72","grey72","gray73","grey73","gray74",
"grey74","gray75","grey75","gray76","grey76","gray77","grey77","gray78","grey78","gray79","grey79","gray80","grey80","gray81",
"grey81","gray82","grey82","gray83","grey83","gray84","grey84","gray85","grey85","gray86","grey86","gray87","grey87","gray88",
"grey88","gray89","grey89","gray90","grey90","gray91","grey91","gray92","grey92","gray93","grey93","gray94","grey94","gray95",
"grey95","gray96","grey96","gray97","grey97","gray98","grey98","gray99","grey99","gray100","LightGreen"
};

void random_X_color_string(char *str)
{
    int i = (int)(((double)rand() / (double)RAND_MAX) * (double)(NUM_X_COLORS-1));
    MPIU_Strncpy(str, g_XColors[i], 40);
}

static void init_state_strings()
{
    /* mpid functions */
    g_timer_state[MPID_STATE_MPID_ISEND].name = "MPID_Isend";
    g_timer_state[MPID_STATE_MPID_IRECV].name = "MPID_Irecv";
    g_timer_state[MPID_STATE_MPID_SEND].name = "MPID_Send";
    g_timer_state[MPID_STATE_MPID_RECV].name = "MPID_Recv";
    g_timer_state[MPID_STATE_MPID_PROGRESS_TEST].name = "MPID_Progress_test";
    g_timer_state[MPID_STATE_MPID_ABORT].name = "MPID_Abort";
    g_timer_state[MPID_STATE_MPID_CLOSE_PORT].name = "MPID_Close_port";
    g_timer_state[MPID_STATE_MPID_COMM_ACCEPT].name = "MPID_Comm_accept";
    g_timer_state[MPID_STATE_MPID_COMM_CONNECT].name = "MPID_Comm_connect";
    g_timer_state[MPID_STATE_MPID_COMM_DISCONNECT].name = "MPID_Comm_disconnect";
    g_timer_state[MPID_STATE_MPID_COMM_SPAWN_MULTIPLE].name = "MPID_Comm_spawn_multiple";
    g_timer_state[MPID_STATE_MPID_OPEN_PORT].name = "MPID_Open_port";
    g_timer_state[MPID_STATE_MPID_PROGRESS_WAIT].name = "MPID_Progress_wait";
    g_timer_state[MPID_STATE_MPID_REQUEST_RELEASE].name = "MPID_Request_release";

    /* util functions */
    g_timer_state[MPID_STATE_BREAD].name = "bread";
    g_timer_state[MPID_STATE_BREADV].name = "breadv";
    g_timer_state[MPID_STATE_BWRITE].name = "bwrite";
    g_timer_state[MPID_STATE_BWRITEV].name = "bwritev";
    g_timer_state[MPID_STATE_BSELECT].name = "bselect";

    /* mm functions */
    g_timer_state[MPID_STATE_MM_OPEN_PORT].name = "mm_open_port";
    g_timer_state[MPID_STATE_MM_CLOSE_PORT].name = "mm_close_port";
    g_timer_state[MPID_STATE_MM_ACCEPT].name = "mm_accept";
    g_timer_state[MPID_STATE_MM_CONNECT].name = "mm_connect";
    g_timer_state[MPID_STATE_MM_SEND].name = "mm_send";
    g_timer_state[MPID_STATE_MM_RECV].name = "mm_recv";
    g_timer_state[MPID_STATE_MM_CLOSE].name = "mm_close";
    g_timer_state[MPID_STATE_MM_REQUEST_ALLOC].name = "mm_request_alloc";
    g_timer_state[MPID_STATE_MM_REQUEST_FREE].name = "mm_request_free";
    g_timer_state[MPID_STATE_MM_CAR_INIT].name = "mm_car_init";
    g_timer_state[MPID_STATE_MM_CAR_FINALIZE].name = "mm_car_finalize";
    g_timer_state[MPID_STATE_MM_CAR_ALLOC].name = "mm_car_alloc";
    g_timer_state[MPID_STATE_MM_CAR_FREE].name = "mm_car_free";
    g_timer_state[MPID_STATE_MM_VC_INIT].name = "mm_vc_init";
    g_timer_state[MPID_STATE_MM_VC_FINALIZE].name = "mm_vc_finalize";
    g_timer_state[MPID_STATE_MM_VC_FROM_COMMUNICATOR].name = "mm_vc_from_communicator";
    g_timer_state[MPID_STATE_MM_VC_FROM_CONTEXT].name = "mm_vc_from_context";
    g_timer_state[MPID_STATE_MM_VC_ALLOC].name = "mm_vc_alloc";
    g_timer_state[MPID_STATE_MM_VC_CONNECT_ALLOC].name = "mm_vc_connect_alloc";
    g_timer_state[MPID_STATE_MM_VC_FREE].name = "mm_vc_free";
    g_timer_state[MPID_STATE_MM_CHOOSE_BUFFER].name = "mm_choose_buffer";
    g_timer_state[MPID_STATE_MM_RESET_CARS].name = "mm_reset_cars";
    g_timer_state[MPID_STATE_MM_GET_BUFFERS_TMP].name = "mm_get_buffers_tmp";
    g_timer_state[MPID_STATE_MM_RELEASE_BUFFERS_TMP].name = "mm_release_buffers_tmp";
    g_timer_state[MPID_STATE_MM_GET_BUFFERS_VEC].name = "mm_get_buffers_vec";
    g_timer_state[MPID_STATE_VEC_BUFFER_INIT].name = "vec_buffer_init";
    g_timer_state[MPID_STATE_TMP_BUFFER_INIT].name = "tmp_buffer_init";
    g_timer_state[MPID_STATE_SIMPLE_BUFFER_INIT].name = "simple_buffer_init";
    g_timer_state[MPID_STATE_MM_POST_RECV].name = "mm_post_recv";
    g_timer_state[MPID_STATE_MM_POST_SEND].name = "mm_post_send";
    g_timer_state[MPID_STATE_MM_POST_RNDV_DATA_SEND].name = "mm_post_rndv_data_send";
    g_timer_state[MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND].name = "mm_post_rndv_clear_to_send";
    g_timer_state[MPID_STATE_MM_CQ_TEST].name = "mm_cq_test";
    g_timer_state[MPID_STATE_MM_CQ_WAIT].name = "mm_cq_wait";
    g_timer_state[MPID_STATE_MM_CQ_ENQUEUE].name = "mm_cq_enqueue";
    g_timer_state[MPID_STATE_MM_CREATE_POST_UNEX].name = "mm_create_post_unex";
    g_timer_state[MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND].name = "mm_enqueue_request_to_send";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_HEAD_CAR].name = "cq_handle_read_head_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_DATA_CAR].name = "cq_handle_read_data_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_READ_CAR].name = "cq_handle_read_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_HEAD_CAR].name = "cq_handle_write_head_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_DATA_CAR].name = "cq_handle_write_data_car";
    g_timer_state[MPID_STATE_CQ_HANDLE_WRITE_CAR].name = "cq_handle_write_car";

    /* xfer functions */
    g_timer_state[MPID_STATE_XFER_INIT].name = "xfer_init";
    g_timer_state[MPID_STATE_XFER_RECV_OP].name = "xfer_recv_op";
    g_timer_state[MPID_STATE_XFER_RECV_MOP_OP].name = "xfer_recv_mop_op";
    g_timer_state[MPID_STATE_XFER_RECV_FORWARD_OP].name = "xfer_recv_forward_op";
    g_timer_state[MPID_STATE_XFER_RECV_MOP_FORWARD_OP].name = "xfer_mop_forward_op";
    g_timer_state[MPID_STATE_XFER_FORWARD_OP].name = "xfer_forward_op";
    g_timer_state[MPID_STATE_XFER_SEND_OP].name = "xfer_send_op";
    g_timer_state[MPID_STATE_XFER_REPLICATE_OP].name = "xfer_replicate_op";
    g_timer_state[MPID_STATE_XFER_START].name = "xfer_start";

    /* method functions */
    g_timer_state[MPID_STATE_TCP_INIT].name = "tcp_init";
    g_timer_state[MPID_STATE_TCP_FINALIZE].name = "tcp_finalize";
    g_timer_state[MPID_STATE_TCP_ACCEPT_CONNECTION].name = "tcp_accept_connection";
    g_timer_state[MPID_STATE_TCP_GET_BUSINESS_CARD].name = "tcp_get_business_card";
    g_timer_state[MPID_STATE_TCP_CAN_CONNECT].name = "tcp_can_connect";
    g_timer_state[MPID_STATE_TCP_POST_CONNECT].name = "tcp_post_connect";
    g_timer_state[MPID_STATE_TCP_POST_READ].name = "tcp_post_read";
    g_timer_state[MPID_STATE_TCP_MERGE_WITH_UNEXPECTED].name = "tcp_merge_with_unexpected";
    g_timer_state[MPID_STATE_TCP_POST_WRITE].name = "tcp_post_write";
    g_timer_state[MPID_STATE_TCP_MAKE_PROGRESS].name = "tcp_make_progress";
    g_timer_state[MPID_STATE_TCP_CAR_ENQUEUE].name = "tcp_car_enqueue";
    g_timer_state[MPID_STATE_TCP_CAR_DEQUEUE].name = "tcp_car_dequeue";
    g_timer_state[MPID_STATE_TCP_CAR_DEQUEUE_WRITE].name = "tcp_dequeue_write";
    g_timer_state[MPID_STATE_TCP_RESET_CAR].name = "tcp_reset_car";
    g_timer_state[MPID_STATE_TCP_POST_READ_PKT].name = "tcp_post_read_pkt";
    g_timer_state[MPID_STATE_TCP_READ].name = "tcp_read";
    g_timer_state[MPID_STATE_TCP_WRITE].name = "tcp_write";
    g_timer_state[MPID_STATE_TCP_READ_SHM].name = "tcp_read_shm";
    g_timer_state[MPID_STATE_TCP_READ_VIA].name = "tcp_read_via";
    g_timer_state[MPID_STATE_TCP_READ_VIA_RDMA].name = "tcp_read_via_rdma";
    g_timer_state[MPID_STATE_TCP_READ_VEC].name = "tcp_read_vec";
    g_timer_state[MPID_STATE_TCP_READ_TMP].name = "tcp_read_tmp";
    g_timer_state[MPID_STATE_TCP_READ_CONNECTING].name = "tcp_read_connecting";
    g_timer_state[MPID_STATE_TCP_WRITE_SHM].name = "tcp_write_shm";
    g_timer_state[MPID_STATE_TCP_WRITE_VIA].name = "tcp_write_via";
    g_timer_state[MPID_STATE_TCP_WRITE_VIA_RDMA].name = "tcp_write_via_rdma";
    g_timer_state[MPID_STATE_TCP_WRITE_VEC].name = "tcp_write_vec";
    g_timer_state[MPID_STATE_TCP_WRITE_TMP].name = "tcp_write_tmp";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_SHM].name = "tcp_stuff_vector_shm";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VIA].name = "tcp_stuff_vector_via";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VIA_RDMA].name = "tcp_stuff_vector_via_rdma";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_VEC].name = "tcp_stuff_vector_vec";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_TMP].name = "tcp_stuff_vector_tmp";
    g_timer_state[MPID_STATE_TCP_WRITE_AGGRESSIVE].name = "tcp_write_aggressive";
    g_timer_state[MPID_STATE_TCP_CAR_HEAD_ENQUEUE].name = "tcp_car_head_enqueue";
    g_timer_state[MPID_STATE_TCP_SETUP_PACKET_CAR].name = "tcp_setup_packet_car";
    g_timer_state[MPID_STATE_TCP_UPDATE_CAR_NUM_WRITTEN].name = "tcp_update_car_num_written";
    g_timer_state[MPID_STATE_TCP_MERGE_UNEXPECTED_DATA].name = "tcp_merge_unexpected_data";
    g_timer_state[MPID_STATE_TCP_MERGE_SHM].name = "tcp_merge_shm";
    g_timer_state[MPID_STATE_TCP_MERGE_VIA].name = "tcp_merge_via";
    g_timer_state[MPID_STATE_TCP_MERGE_VIA_RDMA].name = "tcp_merge_via_rdma";
    g_timer_state[MPID_STATE_TCP_MERGE_VEC].name = "tcp_merge_vec";
    g_timer_state[MPID_STATE_TCP_MERGE_TMP].name = "tcp_merge_tmp";
    g_timer_state[MPID_STATE_TCP_MERGE_SIMPLE].name = "tcp_merge_simple";
    g_timer_state[MPID_STATE_TCP_MERGE_WITH_POSTED].name = "tcp_merge_with_posted";
    g_timer_state[MPID_STATE_TCP_READ_HEADER].name = "tcp_read_header";
    g_timer_state[MPID_STATE_TCP_READ_DATA].name = "tcp_read_data";
    g_timer_state[MPID_STATE_TCP_READ_SIMPLE].name = "tcp_read_simple";
    g_timer_state[MPID_STATE_TCP_WRITE_SIMPLE].name = "tcp_write_simple";
    g_timer_state[MPID_STATE_TCP_STUFF_VECTOR_SIMPLE].name = "tcp_stuff_vector_simple";
    g_timer_state[MPID_STATE_FIND_IN_QUEUE].name = "find_in_queue";

    g_timer_state[MPID_STATE_SHM_CAN_CONNECT].name = "shm_can_connect";
    g_timer_state[MPID_STATE_SHM_GET_BUSINESS_CARD].name = "shm_get_business_card";
    g_timer_state[MPID_STATE_SHM_INIT].name = "shm_init";
    g_timer_state[MPID_STATE_SHM_FINALIZE].name = "shm_finalize";
    g_timer_state[MPID_STATE_SHM_MAKE_PROGRESS].name = "shm_make_progress";
    g_timer_state[MPID_STATE_SHM_ALLOC].name = "shm_alloc";
    g_timer_state[MPID_STATE_SHM_FREE].name = "shm_free";
    g_timer_state[MPID_STATE_SHM_GET_MEM_SYNC].name = "shm_get_mem_sync";
    g_timer_state[MPID_STATE_SHM_RELEASE_MEM].name = "shm_release_mem";
    g_timer_state[MPID_STATE_SHM_POST_CONNECT].name = "shm_post_connect";
    g_timer_state[MPID_STATE_SHM_POST_READ].name = "shm_post_read";
    g_timer_state[MPID_STATE_SHM_MERGE_WITH_UNEXPECTED].name = "shm_merge_with_unexpected";
    g_timer_state[MPID_STATE_SHM_POST_WRITE].name = "shm_post_write";

    g_timer_state[MPID_STATE_PACKER_CAR_ENQUEUE].name = "packer_car_enqueue";
    g_timer_state[MPID_STATE_PACKER_CAR_DEQUEUE].name = "packer_car_dequeue";
    g_timer_state[MPID_STATE_PACKER_INIT].name = "packer_init";
    g_timer_state[MPID_STATE_PACKER_FINALIZE].name = "packer_finalize";
    g_timer_state[MPID_STATE_PACKER_MAKE_PROGRESS].name = "packer_make_progress";
    g_timer_state[MPID_STATE_PACKER_POST_READ].name = "packer_post_read";
    g_timer_state[MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED].name = "packer_merge_with_unexpected";
    g_timer_state[MPID_STATE_PACKER_POST_WRITE].name = "packer_post_write";
    g_timer_state[MPID_STATE_PACKER_RESET_CAR].name = "packer_reset_car";

    g_timer_state[MPID_STATE_UNPACKER_CAR_ENQUEUE].name = "unpacker_car_enqueue";
    g_timer_state[MPID_STATE_UNPACKER_CAR_DEQUEUE].name = "unpacker_car_dequeue";
    g_timer_state[MPID_STATE_UNPACKER_INIT].name = "unpacker_init";
    g_timer_state[MPID_STATE_UNPACKER_FINALIZE].name = "unpacker_finalize";
    g_timer_state[MPID_STATE_UNPACKER_MAKE_PROGRESS].name = "unpacker_make_progress";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_SHM].name = "unpacker_write_shm";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VIA].name = "unpacker_write_via";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VIA_RDMA].name = "unpacker_write_via_rdma";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_VEC].name = "unpacker_write_vec";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_TMP].name = "unpacker_write_tmp";
    g_timer_state[MPID_STATE_UNPACKER_POST_READ].name = "unpacker_post_read";
    g_timer_state[MPID_STATE_UNPACKER_MERGE_WITH_UNEXPECTED].name = "unpacker_merge_with_unexpected";
    g_timer_state[MPID_STATE_UNPACKER_POST_WRITE].name = "unpacker_post_write";
    g_timer_state[MPID_STATE_UNPACKER_RESET_CAR].name = "unpacker_reset_car";
    g_timer_state[MPID_STATE_UNPACKER_WRITE_SIMPLE].name = "unpacker_write_simple";

    g_timer_state[MPID_STATE_VIA_CAN_CONNECT].name = "via_can_connect";
    g_timer_state[MPID_STATE_VIA_GET_BUSINESS_CARD].name = "via_get_business_card";
    g_timer_state[MPID_STATE_VIA_INIT].name = "via_init";
    g_timer_state[MPID_STATE_VIA_FINALIZE].name = "via_finalize";
    g_timer_state[MPID_STATE_VIA_MAKE_PROGRESS].name = "via_make_progress";
    g_timer_state[MPID_STATE_VIA_POST_CONNECT].name = "via_post_connect";
    g_timer_state[MPID_STATE_VIA_POST_READ].name = "via_post_read";
    g_timer_state[MPID_STATE_VIA_MERGE_WITH_UNEXPECTED].name = "via_merge_with_unexpected";
    g_timer_state[MPID_STATE_VIA_POST_WRITE].name = "via_post_write";

    g_timer_state[MPID_STATE_VIA_RDMA_CAN_CONNECT].name = "via_rdma_can_connect";
    g_timer_state[MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD].name = "via_rdma_get_business_card";
    g_timer_state[MPID_STATE_VIA_RDMA_INIT].name = "via_rdma_init";
    g_timer_state[MPID_STATE_VIA_RDMA_FINALIZE].name = "via_rdma_finalize";
    g_timer_state[MPID_STATE_VIA_RDMA_MAKE_PROGRESS].name = "via_rdma_make_progress";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_CONNECT].name = "via_rdma_post_connect";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_READ].name = "via_rdma_post_read";
    g_timer_state[MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED].name = "via_rdma_merge_with_unexpected";
    g_timer_state[MPID_STATE_VIA_RDMA_POST_WRITE].name = "via_rdma_post_write";

    /* mpi functions */
    g_timer_state[MPID_STATE_MPI_ABORT].name = "MPI_Abort";
    g_timer_state[MPID_STATE_MPI_ADDRESS].name = "MPI_Address";
    g_timer_state[MPID_STATE_MPI_ALLREDUCE].name = "MPI_Allreduce";
    g_timer_state[MPID_STATE_MPI_ATTR_GET].name = "MPI_Attr_get";
    g_timer_state[MPID_STATE_MPI_BARRIER].name = "MPI_Barrier";
    g_timer_state[MPID_STATE_MPI_BCAST].name = "MPI_Bcast";
    g_timer_state[MPID_STATE_MPI_CLOSE_PORT].name = "MPI_Close_port";
    g_timer_state[MPID_STATE_MPI_COMM_ACCEPT].name = "MPI_Comm_accept";
    g_timer_state[MPID_STATE_MPI_COMM_CALL_ERRHANDLER].name = "MPI_Comm_call_errhandler";
    g_timer_state[MPID_STATE_MPI_COMM_CONNECT].name = "MPI_Comm_connect";
    g_timer_state[MPID_STATE_MPI_COMM_CREATE].name = "MPI_Comm_create";
    g_timer_state[MPID_STATE_MPI_COMM_CREATE_ERRHANDLER].name = "MPI_Comm_create_errhandler";
    g_timer_state[MPID_STATE_MPI_COMM_DISCONNECT].name = "MPI_Comm_disconnect";
    g_timer_state[MPID_STATE_MPI_COMM_FREE].name = "MPI_Comm_free";
    g_timer_state[MPID_STATE_MPI_COMM_GET_ATTR].name = "MPI_Comm_get_attr";
    g_timer_state[MPID_STATE_MPI_COMM_GET_ERRHANDLER].name = "MPI_Comm_get_errhandler";
    g_timer_state[MPID_STATE_MPI_COMM_RANK].name = "MPI_Comm_rank";
    g_timer_state[MPID_STATE_MPI_COMM_SET_ERRHANDLER].name = "MPI_Comm_set_errhandler";
    g_timer_state[MPID_STATE_MPI_COMM_SIZE].name = "MPI_Comm_size";
    g_timer_state[MPID_STATE_MPI_COMM_SPAWN].name = "MPI_Comm_spawn";
    g_timer_state[MPID_STATE_MPI_COMM_SPAWN_MULTIPLE].name = "MPI_Comm_spawn_multiple";
    g_timer_state[MPID_STATE_MPI_ERRHANDLER_CREATE].name = "MPI_Errhandler_create";
    g_timer_state[MPID_STATE_MPI_ERRHANDLER_FREE].name = "MPI_Errhandler_free";
    g_timer_state[MPID_STATE_MPI_ERRHANDLER_GET].name = "MPI_Errhandler_get";
    g_timer_state[MPID_STATE_MPI_ERRHANDLER_SET].name = "MPI_Errhandler_set";
    g_timer_state[MPID_STATE_MPI_ERROR_CLASS].name = "MPI_Error_class";
    g_timer_state[MPID_STATE_MPI_ERROR_STRING].name = "MPI_Error_string";
    g_timer_state[MPID_STATE_MPI_FILE_CALL_ERRHANDLER].name = "MPI_File_call_errhandler";
    g_timer_state[MPID_STATE_MPI_FILE_CREATE_ERRHANDLER].name = "MPI_File_create_errhandler";
    g_timer_state[MPID_STATE_MPI_FILE_GET_ERRHANDLER].name = "MPI_File_get_errhandler";
    g_timer_state[MPID_STATE_MPI_FILE_SET_ERRHANDLER].name = "MPI_File_set_errhandler";
    g_timer_state[MPID_STATE_MPI_FINALIZE].name = "MPI_Finalize";
    g_timer_state[MPID_STATE_MPI_FINALIZED].name = "MPI_Finalized";
    g_timer_state[MPID_STATE_MPI_GET_ELEMENTS].name = "MPI_Get_elements";
    g_timer_state[MPID_STATE_MPI_INFO_CREATE].name = "MPI_Info_create";
    g_timer_state[MPID_STATE_MPI_INFO_DELETE].name = "MPI_Info_delete";
    g_timer_state[MPID_STATE_MPI_INFO_DUP].name = "MPI_Info_dup";
    g_timer_state[MPID_STATE_MPI_INFO_FREE].name = "MPI_Info_free";
    g_timer_state[MPID_STATE_MPI_INFO_GET].name = "MPI_Info_get";
    g_timer_state[MPID_STATE_MPI_INFO_GET_NKEYS].name = "MPI_Info_get_nkeys";
    g_timer_state[MPID_STATE_MPI_INFO_GET_NTHKEY].name = "MPI_Info_get_nthkey";
    g_timer_state[MPID_STATE_MPI_INFO_GET_VALUELEN].name = "MPI_Info_get_valuelen";
    g_timer_state[MPID_STATE_MPI_INFO_SET].name = "MPI_Info_set";
    g_timer_state[MPID_STATE_MPI_INIT].name = "MPI_Init";
    g_timer_state[MPID_STATE_MPI_INITIALIZED].name = "MPI_Initialized";
    g_timer_state[MPID_STATE_MPI_INIT_THREAD].name = "MPI_Init_thread";
    g_timer_state[MPID_STATE_MPI_IRECV].name = "MPI_Irecv";
    g_timer_state[MPID_STATE_MPI_ISEND].name = "MPI_Isend";
    g_timer_state[MPID_STATE_MPI_OPEN_PORT].name = "MPI_Open_port";
    g_timer_state[MPID_STATE_MPI_RECV].name = "MPI_Recv";
    g_timer_state[MPID_STATE_MPI_REDUCE].name = "MPI_Reduce";
    g_timer_state[MPID_STATE_MPI_SEND].name = "MPI_Send";
    g_timer_state[MPID_STATE_MPI_TEST].name = "MPI_Test";
    g_timer_state[MPID_STATE_MPI_WAIT].name = "MPI_Wait";
    g_timer_state[MPID_STATE_MPI_TYPE_VECTOR].name = "MPI_Type_vector";
    g_timer_state[MPID_STATE_MPI_WIN_CALL_ERRHANDLER].name = "MPI_Win_call_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_CREATE_ERRHANDLER].name = "MPI_Win_create_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_GET_ERRHANDLER].name = "MPI_Win_get_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_SET_ERRHANDLER].name = "MPI_Win_set_errhandler";
    g_timer_state[MPID_STATE_MPI_WTICK].name = "MPI_Wtick";
    g_timer_state[MPID_STATE_MPI_GET_ADDRESS].name = "MPI_Get_address";
    g_timer_state[MPID_STATE_MPI_GET_COUNT].name = "MPI_Get_count";
}

int MPIU_Timer_init(int rank, int size)
{
    int i;
    unsigned char r,g,b;

    g_pDLOG = DLOG_InitLog(rank, size);
    if (g_pDLOG == NULL)
	return -1;

    for (i=0; i<MPID_NUM_TIMER_STATES; i++)
    {
	g_timer_state[i].num_calls = 0;
	g_timer_state[i].in_id = DLOG_GetNextEvent(g_pDLOG);
	g_timer_state[i].out_id = DLOG_GetNextEvent(g_pDLOG);
	g_timer_state[i].color = random_color(&r, &g, &b);
	/*sprintf(g_timer_state[i].color_str, "%d %d %d", (int)r, (int)g, (int)b);*/
	random_X_color_string(g_timer_state[i].color_str);
    }
    init_state_strings();

    DLOG_EnableLogging(g_pDLOG);
    DLOG_SaveFirstTimestamp(g_pDLOG);
    DLOG_LogCommID(g_pDLOG, (int)MPI_COMM_WORLD);

    return MPI_SUCCESS;
}

int MPIU_Timer_finalize()
{
    int i;
    
    MPIU_Msg_printf( "Writing logfile.\n");fflush(stdout);
    for (i=0; i<MPID_NUM_TIMER_STATES; i++) 
    {
	/*DLOG_DescribeState(g_pDLOG, i, g_timer_state[i].name, g_timer_state[i].color_str);*/
	DLOG_DescribeOpenState(g_pDLOG,
	    g_timer_state[i].in_id, 
	    g_timer_state[i].out_id, 
	    g_timer_state[i].name,
	    g_timer_state[i].color_str );
    }

    DLOG_DisableLogging(g_pDLOG);

    DLOG_FinishLog(g_pDLOG, "mpid_prof");

    MPIU_Msg_printf("finished.\n");fflush(stdout);

    return MPI_SUCCESS;
}
#endif /* MPID_LOGGING_DLOG */






/* This section of code is for the RLOG logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)

static char random_color_str[40];
static char *get_random_color_str()
{
    unsigned char r,g,b;
    random_color(&r, &g, &b);
    sprintf(random_color_str, "%3d %3d %3d", (int)r, (int)g, (int)b);
    return random_color_str;
}

int MPIU_Timer_init(int rank, int size)
{
    g_pRLOG = RLOG_InitLog(rank, size);
    if (g_pRLOG == NULL)
	return -1;

    RLOG_EnableLogging(g_pRLOG);

    RLOG_SaveFirstTimestamp(g_pRLOG);

    RLOG_LogCommID(g_pRLOG, (int)MPI_COMM_WORLD);

    /* arrow state */
    RLOG_DescribeState(g_pRLOG, RLOG_ARROW_EVENT_ID, "Arrow", "255 255 255");

    /* mpid functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_ISEND, "MPID_Isend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_IRECV, "MPID_Irecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_SEND, "MPID_Send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_RECV, "MPID_Recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_PROGRESS_TEST, "MPID_Progress_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_ABORT, "MPID_Abort", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_CLOSE_PORT, "MPID_Close_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_ACCEPT, "MPID_Comm_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_CONNECT, "MPID_Comm_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_DISCONNECT, "MPID_Comm_disconnect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_COMM_SPAWN_MULTIPLE, "MPID_Comm_spawn_multiple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_OPEN_PORT, "MPID_Open_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_PROGRESS_WAIT, "MPID_Progress_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPID_REQUEST_RELEASE, "MPID_Request_release", get_random_color_str());

    /* util functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREAD, "bread", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BREADV, "breadv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITE, "bwrite", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BWRITEV, "bwritev", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_BSELECT, "bselect", get_random_color_str());

    /* mm functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_OPEN_PORT, "mm_open_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CLOSE_PORT, "mm_close_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_ACCEPT, "mm_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CONNECT, "mm_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_SEND, "mm_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RECV, "mm_recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CLOSE, "mm_close", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_REQUEST_ALLOC, "mm_request_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_REQUEST_FREE, "mm_request_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_INIT, "mm_car_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_FINALIZE, "mm_car_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_ALLOC, "mm_car_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CAR_FREE, "mm_car_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_INIT, "mm_vc_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FINALIZE, "mm_vc_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FROM_COMMUNICATOR, "mm_vc_from_communicator", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FROM_CONTEXT, "mm_vc_from_context", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_ALLOC, "mm_vc_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_CONNECT_ALLOC, "mm_vc_connect_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_VC_FREE, "mm_vc_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CHOOSE_BUFFER, "mm_choose_buffer", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RESET_CARS, "mm_reset_cars", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_GET_BUFFERS_TMP, "mm_get_buffers_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_RELEASE_BUFFERS_TMP, "mm_release_buffers_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_GET_BUFFERS_VEC, "mm_get_buffers_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VEC_BUFFER_INIT, "vec_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TMP_BUFFER_INIT, "tmp_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SIMPLE_BUFFER_INIT, "simple_buffer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RECV, "mm_post_recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_SEND, "mm_post_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RNDV_DATA_SEND, "mm_post_rndv_data_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_POST_RNDV_CLEAR_TO_SEND, "mm_post_rndv_clear_to_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_TEST, "mm_cq_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_WAIT, "mm_cq_wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CQ_ENQUEUE, "mm_cq_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_CREATE_POST_UNEX, "mm_create_post_unex", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MM_ENQUEUE_REQUEST_TO_SEND, "mm_enqueue_request_to_send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_HEAD_CAR, "cq_handle_read_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_DATA_CAR, "cq_handle_read_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_READ_CAR, "cq_handle_read_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_HEAD_CAR, "cq_handle_write_head_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_DATA_CAR, "cq_handle_write_data_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_CQ_HANDLE_WRITE_CAR, "cq_handle_write_car", get_random_color_str());

    /* xfer functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_INIT, "xfer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_OP, "xfer_recv_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_MOP_OP, "xfer_recv_mop_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_FORWARD_OP, "xfer_recv_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_RECV_MOP_FORWARD_OP, "xfer_mop_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_FORWARD_OP, "xfer_forward_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_SEND_OP, "xfer_send_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_REPLICATE_OP, "xfer_replicate_op", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_XFER_START, "xfer_start", get_random_color_str());

    /* method functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_INIT, "tcp_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_FINALIZE, "tcp_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_ACCEPT_CONNECTION, "tcp_accept_connection", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_GET_BUSINESS_CARD, "tcp_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAN_CONNECT, "tcp_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_CONNECT, "tcp_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_READ, "tcp_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_WITH_UNEXPECTED, "tcp_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_WRITE, "tcp_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MAKE_PROGRESS, "tcp_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_ENQUEUE, "tcp_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_DEQUEUE, "tcp_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_DEQUEUE_WRITE, "tcp_dequeue_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_RESET_CAR, "tcp_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_POST_READ_PKT, "tcp_post_read_pkt", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ, "tcp_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE, "tcp_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_SHM, "tcp_read_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VIA, "tcp_read_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VIA_RDMA, "tcp_read_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_VEC, "tcp_read_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_TMP, "tcp_read_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_CONNECTING, "tcp_read_connecting", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_SHM, "tcp_write_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VIA, "tcp_write_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VIA_RDMA, "tcp_write_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_VEC, "tcp_write_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_TMP, "tcp_write_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_SHM, "tcp_stuff_vector_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VIA, "tcp_stuff_vector_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VIA_RDMA, "tcp_stuff_vector_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_VEC, "tcp_stuff_vector_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_TMP, "tcp_stuff_vector_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_AGGRESSIVE, "tcp_write_aggressive", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_CAR_HEAD_ENQUEUE, "tcp_car_head_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_SETUP_PACKET_CAR, "tcp_setup_packet_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_UPDATE_CAR_NUM_WRITTEN, "tcp_update_car_num_written", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_UNEXPECTED_DATA, "tcp_merge_unexpected_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_SHM, "tcp_merge_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VIA, "tcp_merge_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VIA_RDMA, "tcp_merge_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_VEC, "tcp_merge_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_TMP, "tcp_merge_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_SIMPLE, "tcp_merge_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_MERGE_WITH_POSTED, "tcp_merge_with_posted", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_HEADER, "tcp_read_header", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_DATA, "tcp_read_data", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_READ_SIMPLE, "tcp_read_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_WRITE_SIMPLE, "tcp_write_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_TCP_STUFF_VECTOR_SIMPLE, "tcp_stuff_vector_simple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_FIND_IN_QUEUE, "find_in_queue", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_CAN_CONNECT, "shm_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_GET_BUSINESS_CARD, "shm_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_INIT, "shm_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_FINALIZE, "shm_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_MAKE_PROGRESS, "shm_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_ALLOC, "shm_alloc", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_FREE, "shm_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_GET_MEM_SYNC, "shm_get_mem_sync", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_RELEASE_MEM, "shm_release_mem", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_CONNECT, "shm_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_READ, "shm_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_MERGE_WITH_UNEXPECTED, "shm_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_SHM_POST_WRITE, "shm_post_write", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_CAR_ENQUEUE, "packer_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_CAR_DEQUEUE, "packer_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_INIT, "packer_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_FINALIZE, "packer_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_MAKE_PROGRESS, "packer_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_POST_READ, "packer_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_MERGE_WITH_UNEXPECTED, "packer_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_POST_WRITE, "packer_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_PACKER_RESET_CAR, "packer_reset_car", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_CAR_ENQUEUE, "unpacker_car_enqueue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_CAR_DEQUEUE, "unpacker_car_dequeue", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_INIT, "unpacker_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_FINALIZE, "unpacker_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_MAKE_PROGRESS, "unpacker_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_SHM, "unpacker_write_shm", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VIA, "unpacker_write_via", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VIA_RDMA, "unpacker_write_via_rdma", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_VEC, "unpacker_write_vec", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_TMP, "unpacker_write_tmp", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_POST_READ, "unpacker_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_MERGE_WITH_UNEXPECTED, "unpacker_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_POST_WRITE, "unpacker_post_write", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_RESET_CAR, "unpacker_reset_car", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_UNPACKER_WRITE_SIMPLE, "unpacker_write_simple", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_CAN_CONNECT, "via_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_GET_BUSINESS_CARD, "via_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_INIT, "via_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_FINALIZE, "via_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_MAKE_PROGRESS, "via_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_CONNECT, "via_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_READ, "via_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_MERGE_WITH_UNEXPECTED, "via_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_POST_WRITE, "via_post_write", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_CAN_CONNECT, "via_rdma_can_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_GET_BUSINESS_CARD, "via_rdma_get_business_card", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_INIT, "via_rdma_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_FINALIZE, "via_rdma_finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_MAKE_PROGRESS, "via_rdma_make_progress", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_CONNECT, "via_rdma_post_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_READ, "via_rdma_post_read", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_MERGE_WITH_UNEXPECTED, "via_rdma_merge_with_unexpected", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_VIA_RDMA_POST_WRITE, "via_rdma_post_write", get_random_color_str());

    /* mpi functions */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ABORT, "MPI_Abort", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ADDRESS, "MPI_Address", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLREDUCE, "MPI_Allreduce", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ATTR_GET, "MPI_Attr_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BARRIER, "MPI_Barrier", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BCAST, "MPI_Bcast", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CLOSE_PORT, "MPI_Close_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_ACCEPT, "MPI_Comm_accept", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_CALL_ERRHANDLER, "MPI_Comm_call_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_CONNECT, "MPI_Comm_connect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_CREATE, "MPI_Comm_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_CREATE_ERRHANDLER, "MPI_Comm_create_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_DISCONNECT, "MPI_Comm_disconnect", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_FREE, "MPI_Comm_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_GET_ATTR, "MPI_Comm_get_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_GET_ERRHANDLER, "MPI_Comm_get_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_RANK, "MPI_Comm_rank", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SET_ERRHANDLER, "MPI_Comm_set_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SIZE, "MPI_Comm_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SPAWN, "MPI_Comm_spawn", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SPAWN_MULTIPLE, "MPI_Comm_spawn_multiple", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERRHANDLER_CREATE, "MPI_Errhandler_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERRHANDLER_FREE, "MPI_Errhandler_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERRHANDLER_GET, "MPI_Errhandler_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERRHANDLER_SET, "MPI_Errhandler_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERROR_CLASS, "MPI_Error_class", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ERROR_STRING, "MPI_Error_string", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FILE_CALL_ERRHANDLER, "MPI_File_call_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FILE_CREATE_ERRHANDLER, "MPI_File_create_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FILE_GET_ERRHANDLER, "MPI_File_get_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FILE_SET_ERRHANDLER, "MPI_File_set_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FINALIZE, "MPI_Finalize", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FINALIZED, "MPI_Finalized", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GET_ELEMENTS, "MPI_Get_elements", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_CREATE, "MPI_Info_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_DELETE, "MPI_Info_delete", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_DUP, "MPI_Info_dup", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_FREE, "MPI_Info_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_GET, "MPI_Info_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_GET_NKEYS, "MPI_Info_get_nkeys", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_GET_NTHKEY, "MPI_Info_get_nthkey", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_GET_VALUELEN, "MPI_Info_get_valuelen", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INFO_SET, "MPI_Info_set", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INIT, "MPI_Init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INITIALIZED, "MPI_Initialized", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INIT_THREAD, "MPI_Init_thread", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_IRECV, "MPI_Irecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ISEND, "MPI_Isend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_OPEN_PORT, "MPI_Open_port", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_RECV, "MPI_Recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_REDUCE, "MPI_Reduce", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SEND, "MPI_Send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TEST, "MPI_Test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WAIT, "MPI_Wait", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_VECTOR, "MPI_Type_vector", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_CALL_ERRHANDLER, "MPI_Win_call_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_CREATE_ERRHANDLER, "MPI_Win_create_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_GET_ERRHANDLER, "MPI_Win_get_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_SET_ERRHANDLER, "MPI_Win_set_errhandler", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WTICK, "MPI_Wtick", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GET_ADDRESS, "MPI_Get_address", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GET_COUNT, "MPI_Get_count", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_KEYVAL_CREATE, "MPI_Keyval_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_KEYVAL_FREE, "MPI_Keyval_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_OP_CREATE, "MPI_Op_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_OP_FREE, "MPI_Op_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PACK, "MPI_Pack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_NAME, "MPI_Type_get_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_SET_NAME, "MPI_Type_set_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_SIZE, "MPI_Type_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_COMPARE, "MPI_Group_compare", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_DIFFERENCE, "MPI_Group_difference", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_EXCL, "MPI_Group_excl", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_FREE, "MPI_Group_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_INCL, "MPI_Group_incl", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_INTERSECTION, "MPI_Group_intersection", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_RANGE_EXCL, "MPI_Group_range_excl", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_RANGE_INCL, "MPI_Group_range_incl", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_RANK, "MPI_Group_rank", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_SIZE, "MPI_Group_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_TRANSLATE_RANKS, "MPI_Group_translate_ranks", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GROUP_UNION, "MPI_Group_union", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_IS_THREAD_MAIN, "MPI_Is_thread_main", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_QUERY_THREAD, "MPI_Query_thread", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GET_PROCESSOR_NAME, "MPI_Get_processor_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PCONTROL, "MPI_Pcontrol", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GET_VERSION, "MPI_Get_version", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BUFFER_ATTACH, "MPI_Buffer_attach", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BUFFER_DETACH, "MPI_Buffer_detach", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CANCEL, "MPI_Cancel", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_REQUEST_FREE, "MPI_Request_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GREQUEST_START, "MPI_Grequest_start", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GREQUEST_COMPLETE, "MPI_Grequest_complete", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_IPROBE, "MPI_Iprobe", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PROBE, "MPI_Probe", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SENDRECV, "MPI_Sendrecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_STATUS_SET_CANCELLED, "MPI_Status_set_cancelled", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TEST_CANCELLED, "MPI_Test_cancelled", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TESTANY, "MPI_Testany", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_COORDS, "MPI_Cart_coords", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_CREATE, "MPI_Cart_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_GET, "MPI_Cart_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_MAP, "MPI_Cart_map", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_RANK, "MPI_Cart_rank", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_SHIFT, "MPI_Cart_shift", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CART_SUB, "MPI_Cart_sub", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_DIMS_CREATE, "MPI_Dims_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPH_GET, "MPI_Graph_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPH_MAP, "MPI_Graph_map", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPH_NEIGHBORS, "MPI_Graph_neighbors", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPH_CREATE, "MPI_Graph_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPHDIMS_GET, "MPI_Graphdims_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GRAPH_NEIGHBORS_COUNT, "MPI_Graph_neighbors_count", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_CARTDIM_GET, "MPI_Cartdim_get", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TOPO_TEST, "MPI_Topo_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WAITALL, "MPI_Waitall", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WAITSOME, "MPI_Waitsome", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_ENVELOPE, "MPI_Type_get_envelope", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_CONTENTS, "MPI_Get_contents", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_IRSEND, "MPI_Irsend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ISSEND, "MPI_Issend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_RSEND, "MPI_Rsend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SSEND, "MPI_Ssend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLTOALL, "MPI_Alltoall", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLTOALLV, "MPI_Alltoallv", get_random_color_str());

    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_SEND, "MPIC_Send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_RECV, "MPIC_Recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_SENDRECV, "MPIC_Sendrecv", get_random_color_str());

    return MPI_SUCCESS;
}

int MPIU_Timer_finalize()
{
    RLOG_DisableLogging(g_pRLOG);

    MPIU_Msg_printf( "Writing logfile.\n");fflush(stdout);
    RLOG_FinishLog(g_pRLOG);
    MPIU_Msg_printf("finished.\n");fflush(stdout);

    return MPI_SUCCESS;
}
#endif /* MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
