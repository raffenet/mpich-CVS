/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * Notes: This is a temporary file; the various dlog and rlog routines will 
 * eventually be moved into src/util/logging/{dlog,rlog}/, where they 
 * belong.  This file will then contain only the utility functions
 */


#include "mpiimpl.h"

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI

#ifdef HAVE_TIMING

#if (USE_LOGGING == MPID_LOGGING_DLOG)

int MPIR_Describe_mpi_timer_states()
{
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
    g_timer_state[MPID_STATE_MPI_TYPE_CONTIGUOUS].name = "MPI_Type_contiguous";
    g_timer_state[MPID_STATE_MPI_WIN_CALL_ERRHANDLER].name = "MPI_Win_call_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_CREATE_ERRHANDLER].name = "MPI_Win_create_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_GET_ERRHANDLER].name = "MPI_Win_get_errhandler";
    g_timer_state[MPID_STATE_MPI_WIN_SET_ERRHANDLER].name = "MPI_Win_set_errhandler";
    g_timer_state[MPID_STATE_MPI_WTICK].name = "MPI_Wtick";
    g_timer_state[MPID_STATE_MPI_GET_ADDRESS].name = "MPI_Get_address";
    g_timer_state[MPID_STATE_MPI_GET_COUNT].name = "MPI_Get_count";

    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_DLOG */



/* This section of code is for the RLOG logging library */
#if (USE_LOGGING == MPID_LOGGING_RLOG)

#include <math.h>

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

int MPIR_Describe_mpi_timer_states()
{
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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CONTIGUOUS, "MPI_Type_contiguous", get_random_color_str());
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
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_MATCH_SIZE, "MPI_Type_match_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_REGISTER_DATAREP, "MPI_Register_datarep", get_random_color_str());
    
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_SEND, "MPIC_Send", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_RECV, "MPIC_Recv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_SENDRECV, "MPIC_Sendrecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_IRECV, "MPIC_Irecv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_ISEND, "MPIC_Isend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPIC_WAIT, "MPIC_Wait", get_random_color_str());


    /* BEGINNING OF BLOCK OF AUTOMATICALLY GENERATED ENTRIES */
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ADD_ERROR_CLASS, "MPI_Add_error_class", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ADD_ERROR_CODE, "MPI_Add_error_code", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ADD_ERROR_STRING, "MPI_Add_error_string", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLGATHER, "MPI_Allgather", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLGATHERV, "MPI_Allgatherv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLOC_MEM, "MPI_Alloc_mem", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ALLTOALLW, "MPI_Alltoallw", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ATTR_DELETE, "MPI_Attr_delete", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_ATTR_PUT, "MPI_Attr_put", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BSEND, "MPI_Bsend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_BSEND_INIT, "MPI_Bsend_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_COMPARE, "MPI_Comm_compare", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_CREATE_KEYVAL, "MPI_Comm_create_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_DELETE_ATTR, "MPI_Comm_delete_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_DUP, "MPI_Comm_dup", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_FREE_KEYVAL, "MPI_Comm_free_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_GET_NAME, "MPI_Comm_get_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_GET_PARENT, "MPI_Comm_get_parent", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_GROUP, "MPI_Comm_group", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_JOIN, "MPI_Comm_join", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_REMOTE_GROUP, "MPI_Comm_remote_group", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_REMOTE_SIZE, "MPI_Comm_remote_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SET_ATTR, "MPI_Comm_set_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SET_NAME, "MPI_Comm_set_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_SPLIT, "MPI_Comm_split", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_COMM_TEST_INTER, "MPI_Comm_test_inter", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_EXSCAN, "MPI_Exscan", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_FREE_MEM, "MPI_Free_mem", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GATHER, "MPI_Gather", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_GATHERV, "MPI_Gatherv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_IBSEND, "MPI_Ibsend", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INTERCOMM_CREATE, "MPI_Intercomm_create", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_INTERCOMM_MERGE, "MPI_Intercomm_merge", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_LOOKUP_NAME, "MPI_Lookup_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PACK_EXTERNAL, "MPI_Pack_external", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PACK_EXTERNAL_SIZE, "MPI_Pack_external_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PACK_SIZE, "MPI_Pack_size", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_PUBLISH_NAME, "MPI_Publish_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_RECV_INIT, "MPI_Recv_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_REDUCE_SCATTER, "MPI_Reduce_scatter", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_REQUEST_GET_STATUS, "MPI_Request_get_status", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_RSEND_INIT, "MPI_Rsend_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SCAN, "MPI_Scan", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SCATTER, "MPI_Scatter", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SCATTERV, "MPI_Scatterv", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SENDRECV_REPLACE, "MPI_Sendrecv_replace", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SEND_INIT, "MPI_Send_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_SSEND_INIT, "MPI_Ssend_init", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_START, "MPI_Start", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_STARTALL, "MPI_Startall", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_STATUS_SET_ELEMENTS, "MPI_Status_set_elements", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TESTALL, "MPI_Testall", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TESTSOME, "MPI_Testsome", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_COMMIT, "MPI_Type_commit", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_DARRAY, "MPI_Type_create_darray", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_HINDEXED, "MPI_Type_create_hindexed", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_HVECTOR, "MPI_Type_create_hvector", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_INDEXED_BLOCK, "MPI_Type_create_indexed_block", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_KEYVAL, "MPI_Type_create_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_RESIZED, "MPI_Type_create_resized", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_STRUCT, "MPI_Type_create_struct", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_CREATE_SUBARRAY, "MPI_Type_create_subarray", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_DELETE_ATTR, "MPI_Type_delete_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_DUP, "MPI_Type_dup", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_EXTENT, "MPI_Type_extent", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_FREE, "MPI_Type_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_FREE_KEYVAL, "MPI_Type_free_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_ATTR, "MPI_Type_get_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_EXTENT, "MPI_Type_get_extent", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_GET_TRUE_EXTENT, "MPI_Type_get_true_extent", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_HINDEXED, "MPI_Type_hindexed", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_HVECTOR, "MPI_Type_hvector", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_INDEXED, "MPI_Type_indexed", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_LB, "MPI_Type_lb", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_SET_ATTR, "MPI_Type_set_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_STRUCT, "MPI_Type_struct", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_TYPE_UB, "MPI_Type_ub", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_UNPACK, "MPI_Unpack", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_UNPACK_EXTERNAL, "MPI_Unpack_external", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_UNPUBLISH_NAME, "MPI_Unpublish_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WAITANY, "MPI_Waitany", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_COMPLETE, "MPI_Win_complete", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_CREATE_KEYVAL, "MPI_Win_create_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_DELETE_ATTR, "MPI_Win_delete_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_FENCE, "MPI_Win_fence", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_FREE, "MPI_Win_free", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_FREE_KEYVAL, "MPI_Win_free_keyval", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_GET_ATTR, "MPI_Win_get_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_GET_GROUP, "MPI_Win_get_group", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_GET_NAME, "MPI_Win_get_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_LOCK, "MPI_Win_lock", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_POST, "MPI_Win_post", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_SET_ATTR, "MPI_Win_set_attr", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_SET_NAME, "MPI_Win_set_name", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_START, "MPI_Win_start", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_TEST, "MPI_Win_test", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_UNLOCK, "MPI_Win_unlock", get_random_color_str());
    RLOG_DescribeState(g_pRLOG, MPID_STATE_MPI_WIN_WAIT, "MPI_Win_wait", get_random_color_str());

    /* END OF BLOCK OF AUTOMATICALLY GENERATED ENTRIES */

    return 0;
}

#endif /* USE_LOGGING == MPID_LOGGING_RLOG */

#endif /* HAVE_TIMING */
#endif /* MPICH_MPI_FROM_PMPI */
