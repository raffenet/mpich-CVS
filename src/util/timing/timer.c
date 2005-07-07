/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Notes: This is a temporary file; the various dlog and rlog routines will 
 * eventually be moved into src/util/logging/{dlog,rlog}/, where they 
 * belong.  This file will then contain only the utility functions
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

static void random_X_color_string(char *str)
{
    int i = (int)(((double)rand() / (double)RAND_MAX) * (double)(NUM_X_COLORS-1));
    MPIU_Strncpy( str, g_XColors[i], MPID_MAX_COLOR_STR );
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
    
    MPIR_Describe_timer_states();

    DLOG_EnableLogging(g_pDLOG);
    DLOG_SaveFirstTimestamp(g_pDLOG);
    DLOG_LogCommID(g_pDLOG, (int)MPI_COMM_WORLD);

    return MPI_SUCCESS;
}

int MPIU_Timer_finalize(void)
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

#define MAX_RANDOM_COLOR_STR 40
static char random_color_str[MAX_RANDOM_COLOR_STR];
static char *get_random_color_str()
{
    unsigned char r,g,b;
    random_color(&r, &g, &b);
    MPIU_Snprintf(random_color_str, MAX_RANDOM_COLOR_STR, 
		  "%3d %3d %3d", (int)r, (int)g, (int)b);
    return random_color_str;
}

static int s_RLOG_Initialized = 0;
int MPIU_Timer_init(int rank, int size)
{
    if (s_RLOG_Initialized)
    {
	/* MPIU_Timer_init already called. */
	return -1;
    }
    g_pRLOG = RLOG_InitLog(rank, size);
    if (g_pRLOG == NULL)
	return -1;

    RLOG_EnableLogging(g_pRLOG);

    RLOG_SaveFirstTimestamp(g_pRLOG);

    RLOG_LogCommID(g_pRLOG, (int)MPI_COMM_WORLD);

    /* arrow state */
    RLOG_DescribeState(g_pRLOG, RLOG_ARROW_EVENT_ID, "Arrow", "255 255 255");

    MPIR_Describe_timer_states();

    s_RLOG_Initialized = 1;
    return MPI_SUCCESS;
}

int MPIU_Timer_finalize()
{
    if (g_pRLOG == NULL)
	return -1;
    if (!s_RLOG_Initialized)
	return 0;

    RLOG_DisableLogging(g_pRLOG);

    MPIU_Msg_printf( "Writing logfile.\n");fflush(stdout);
    RLOG_FinishLog(g_pRLOG);
    MPIU_Msg_printf("finished.\n");fflush(stdout);
    s_RLOG_Initialized = 0;

    return MPI_SUCCESS;
}
#endif /* MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
