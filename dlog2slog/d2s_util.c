/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/******************** d2s_util.c ****************************/
/*
  This program converts a dlog file generated using DLOG Logging calls into an 
  slog file.
*/

/*
  a dlog file format:
  a record consists of a header which contains the timestamp, record type and
  process id.
  the headers are the same for all record types but the records themselves are
  different. this converter only pays attention to the following record types: 
  DLOG_STATEDEF,
  DLOG_OPEN_EVENT,
  DLOG_ARROW_EVENT,
  DLOG_COMMEVENT.
*/

#include <stdio.h>
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>
#if defined( HAVE_UNISTD_H )
#include <unistd.h> 
#endif
#include "dlog2slog.h"
#include "slog.h"
#include "mpi.h"

#if defined( D2S_BYTESWAP )
#undef D2S_BYTESWAP
#endif

/*
   the preprocessor variable STANDALONE should be defined when
   the file is compiled for standalone dlog2slog converter.
   Because byteswapping is needed to be done only in the converter.
           D2S_BYTESWAP = ! WORDS_BIGENDIAN
*/

#if defined( STANDALONE )
#    if ! defined( WORDS_BIGENDIAN )
#        define D2S_BYTESWAP
#    endif
#endif

static int proc_num    = 0;                
static long num_events  = 0;
static int state_id = 1024;             /* state id for dlog2slog independent of
					   the dlog state id's.*/

static struct state_info *first,        /* pointers to the beginning and end */
                         *last;         /* of the list of state defs.*/
static struct state_info *pEventStateList;

static struct list_elemnt *list_first,  /* pointers to the beginning and end */
                          *list_last;   /* of the list of start events.      */

static struct list_elemnt 
                       *msg_list_first, /* pointers to the beginning and end */
                       *msg_list_last;  /* of the list of start events.      */

static SLOG slog;                       /* a handle to the slog format.      */

/* function prototypes */
int logEvent(int, double, double, int);
int logOpenEvent(DLOG_HEADER *, int, int);
int logMsgEvent(DLOG_HEADER *, int, int);
int handle_extra_state_defs(DLOG_OPEN_STATE *);
int writeSLOGEvent(int state_id, double start_time, double end_time, int procid);
int writeSLOGInterval(double, int, struct list_elemnt);
int writeSLOGMsgInterval(double, int, int, struct list_elemnt);
int handleStartEvent(int, double, int, int);
int handleStartMsgEvent(int, double, int, int, int);
int addEventState(int, char *, char *);
int addState(int, int, int, char *, char *);
int replace_state_in_list(int, int, char *, char *);
int findState_strtEvnt(int);
int findState_endEvnt(int);
int addToList(int, int, int, double);
int addToMsgList(int, int, int, int, double);
int remove_element(int, int, int, struct list_elemnt *);
int remove_msg_elemnt(int, int, int, int, struct list_elemnt *);
void freeList(void);
void freeMsgList(void);
int get_new_state_id(void);

/****
     initialize dlog2slog data structures.
****/
int init_dlog2slog(char *dlog_file, char *slog_file)
{
    char *pExt;

    first                = NULL;
    last                 = NULL;
    list_first           = NULL;
    list_last            = NULL;
    msg_list_first       = NULL;
    msg_list_last        = NULL;
    pEventStateList      = NULL;

  /*
    slog_file has the same name as the dlog file - the .dlog extension is
    changed to .slog extension. the file is created in the same directory as 
    dlog file.
  */

    strcpy(slog_file, dlog_file);
    pExt = strrchr(slog_file,'d');
    if (pExt)
	*pExt = 's';
    else
	strcat(slog_file, ".slog");
    
    return D2S_SUCCESS;
}  


/****
     returns memory resources used by dlog2slog
****/
void DLOG_free_resources()
{
    DLOG_freeStateInfo();
    freeList();
    freeMsgList();
    SLOG_CloseOutputStream( slog );
}
 
  
/****
     the memory buffer read in from the dlog file is passed to this function
     the state definitions list initialized. 
     the return value of this function represents the end of a dlog block
     or the end of the log itself.
     this function is only interested in DLOG_STATEDEF and DLOG_COMMEVENT.
     the others are ignored.
     DLOG_COMMEVENT helps to initialize the proc_num global variable which is
     used in the thread initialization.
     WARNING: to be used when a first pass is made for initializing
     state definitions.
****/
int DLOG_init_state_defs(char *filename)
{
    DLOG_IOStruct *pInput;

    pInput = DLOG_CreateInputStruct(filename);
    if (pInput == NULL)
    {
	printf("DLOG_init_state_defs: Error, unable to create an input structure\n");
	return D2S_ERROR;
    }
    
    do
    {
	switch(pInput->header.type)
	{
	case DLOG_EVENT_TYPE:
	case DLOG_OPEN_EVENT_TYPE:
	    num_events++;
	    break;
	case DLOG_COMM_TYPE:
	    if (pInput->header.procid > proc_num)
		proc_num = pInput->header.procid;
	    break;
	case DLOG_STATE_TYPE:
	    if (addEventState(
		pInput->record.state.stateid,
		pInput->record.state.color,
		pInput->record.state.description) == D2S_ERROR)
	    {
		DLOG_CloseInputStruct(&pInput);
		return D2S_ERROR;
	    }
	    break;
	case DLOG_OPEN_STATE_TYPE:
	    if ( (findState_strtEvnt(pInput->record.ostate.endetype) == D2S_ERROR) ||
		(findState_endEvnt(pInput->record.ostate.startetype) == D2S_ERROR) )
	    {
		if (addState(
		    pInput->record.ostate.stateid,
		    pInput->record.ostate.startetype,
		    pInput->record.ostate.endetype,
		    pInput->record.ostate.color,
		    pInput->record.ostate.description) == D2S_ERROR)
		{
		    DLOG_CloseInputStruct(&pInput);
		    return D2S_ERROR;
		}
	    }
	    else
	    {
		fprintf(stderr,__FILE__":%d: event ids defined for state %s already "
		    "exist. Use MPE_Log_get_event_number() to define new event ids.\n",
		    __LINE__, pInput->record.ostate.description);
		DLOG_CloseInputStruct(&pInput);
		return D2S_ERROR;
	    }
	    break;
	}
    } while (!DLOG_GetNextRecord(pInput));

    if (addState(MSG_STATE, LOG_MESG_SEND, LOG_MESG_RECV, "White", "Message") == D2S_ERROR)
    {
	DLOG_CloseInputStruct(&pInput);
	return D2S_ERROR;
    }

    DLOG_CloseInputStruct(&pInput);
    return D2S_SUCCESS;
}

/****
     initialization of slog when all state definitions and the number of 
     processes and number of events are known.
****/
int init_SLOG (long num_frames, long frame_size, char *slog_file )
{
    long fixed_record_size, kilo_byte  = 1024, error = D2S_SUCCESS;

    slog = SLOG_OpenOutputStream( slog_file );

    if (slog == NULL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_OpenOutputStream returns null - "
	    "check SLOG documentation for more information.\n",__LINE__);
	DLOG_freeStateInfo();
	return D2S_ERROR;
    }

    fixed_record_size = SLOG_typesz[ min_IntvlRec ] + SLOG_typesz[ taskID_t ];

  /*
    calculating the number of frames that would be required to convert
    the dlog file. it is not possible to estimate this value for small
    frame byte sizes because the number of pseudo records in the slog file
    maybe much larger than the number of individual records.
  */
    if (num_frames == 0)
    {
	num_frames = (num_events * fixed_record_size) /
	    ((frame_size*kilo_byte)-SLOG_typesz[FrameHdr]);
	num_frames++;
    }

    SLOG_SetMaxNumOfFramesPerDir(slog, num_frames);
    SLOG_SetFrameByteSize(slog, frame_size*kilo_byte );
    SLOG_SetFrameReservedSpace(slog, 0);
    SLOG_SetIncreasingEndtimeOrder(slog);

  /*
    this is of no use and should be taken out whenever the
    dependency on the file "SLOG_Preview.txt"  gets removed
  */
#ifndef HAVE_WINDOWS_H
    SLOG_SetPreviewName(slog,SLOG_PREVIEW_NAME);
#endif

  /*
    initializing slog tread table, profiling and record definition table.
  */
    error = init_SLOG_TTAB();
    if (error == D2S_ERROR)
	return error;
    error = init_SLOG_PROF_RECDEF();
    
    return error;
}

/****
     initialize number of events and number of processes
****/
void DLOG_init_essential_values(long event_count, int process_count)
{
    num_events = event_count;
    proc_num   = process_count;
}

/****
     this is the function which does all the logging in the second pass.
     it looks for DLOG_RAWEVENT types and then passes it on to the logOpenEvent
     function where all the details are handled.
****/
int DLOG_makeSLOG(char *filename)
{
    DLOG_IOStruct *pInput;
    int result;

    pInput = DLOG_CreateInputStruct(filename);
    if (pInput == NULL)
    {
	printf("DLOG_init_state_defs: Error, unable to create an input structure\n");
	return D2S_ERROR;
    }

    do
    {
	switch(pInput->header.type)
	{
	case DLOG_EVENT_TYPE:
	    result = logEvent(
		pInput->record.event.event,
		pInput->record.event.start_time,
		pInput->record.event.end_time,
		pInput->header.procid);
	    break;
	case DLOG_OPEN_EVENT_TYPE:
	    result = logOpenEvent(&pInput->header, pInput->record.oevent.event, pInput->record.oevent.data);
	    break;
	case DLOG_ARROW_TYPE:
	    result = logMsgEvent(&pInput->header, pInput->record.arrow.event, pInput->record.arrow.data);
	    break;
	case DLOG_OPEN_STATE_TYPE:
	    result = handle_extra_state_defs(&pInput->record.ostate);
	    break;
	}
	if (result == D2S_ERROR)
	{
	    DLOG_CloseInputStruct(&pInput);
	    return D2S_ERROR;
	}
    } while (DLOG_GetNextRecord(pInput) == 0);

    DLOG_CloseInputStruct(&pInput);
    return D2S_SUCCESS;
}

int logEvent(int event, double start_time, double end_time, int procid)
{
    return writeSLOGEvent(event, start_time, end_time, procid);
}

/****
     the most important function of all.
     this function takes the decision whether to add an event log into the
     start event list and reserve space in the slog file OR to match it with a 
     matching start event in the list and log the interval.
     the arguments to the function include a pointer to the header of the 
     event log as well as a pointer to the event log itself.
****/
int logOpenEvent(DLOG_HEADER *headr, int event, int data)
{
    int stat_id;
    struct list_elemnt one; /* a structure that will contain information
			     gathered from the start event list if there is
			     a start event corresponding to the incoming
			     event.
			  */

    if ( (stat_id = findState_strtEvnt(event)) != D2S_ERROR )
    {
      /* 
         if the above condition is true then we have run into a start event.
         now you know why that state definition list is so important. how
         else would one know if an event is a start event or an end event or 
         neither.
      */
	    return handleStartEvent( stat_id, headr->timestamp, headr->procid, data );
    }
    
    if ( (stat_id = findState_endEvnt(event)) != D2S_ERROR ) 
    {
      /*
        if the above condition is true we have run into an end event.
        so now we find the corresponding start event in the start event list
        and log an slog interval. a lot of memory allocation and deallocation
        happening around this point.
      */

	if ( remove_element(stat_id, data, headr->procid, &one) == D2S_ERROR ) 
	{
        /*
	  if the above condition is true then we haven't found a matching start 
	  event in the list of start events. hence there has been an error in
	  the logging. an end event without a start event - definitely something
	  wrong - either in this code or in the logging itself.
	  Of course, if it is the other way around then it is ignored. 
	  but that case arises only when the list of start events is not empty 
	  at the end of the second pass. i havent taken it into account.
        */
		fprintf( stderr, __FILE__":%d: couldnt find matching start event"
	                       ",state=%d,processid=%d,data=%d,timestamp=%f\n",
		 	       __LINE__, stat_id, headr->procid,
			       data, headr->timestamp );
		return D2S_ERROR;
	}

	return writeSLOGInterval( headr->timestamp, headr->procid, one );
    }

    return D2S_SUCCESS;
}

/****
     the most important function of all.
     this function takes the decision whether to add an event log into the
     start event list and reserve space in the slog file OR to match it with a 
     matching start event in the list and log the interval.
     the arguments to the function include a pointer to the header of the 
     event log as well as a pointer to the event log itself.
****/
int logMsgEvent(DLOG_HEADER *headr, int event, int data)
{
    int stat_id;
    struct list_elemnt one; /* a structure that will contain information
			     gathered from the start event list if there is
			     a start event corresponding to the incoming
			     event.
			  */
    if (data == MPI_PROC_NULL)
	return D2S_SUCCESS;

    if ( (stat_id = findState_strtEvnt(event)) != D2S_ERROR )
    {
	/* if the above condition is true then we have run into a start event. */
	if ( remove_msg_elemnt( stat_id, data, headr->procid, event, &one ) == D2S_ERROR )
	{
	    return handleStartMsgEvent( stat_id, headr->timestamp, headr->procid, data, event );
	}
	
	return writeSLOGMsgInterval( headr->timestamp, headr->procid, event, one );
    }

    if ( (stat_id = findState_endEvnt(event)) != D2S_ERROR ) 
    {
	/*
          if the above condition is true we have run into an end event.
	  so now we find the corresponding start event in the start event list
	  and log an slog interval.
	*/

	if ( remove_element(stat_id, data, headr->procid, &one) == D2S_ERROR ) 
	{
	    if ( remove_msg_elemnt( stat_id, data, headr->procid, event, &one ) == D2S_ERROR )
	    {
		return handleStartMsgEvent( stat_id, headr->timestamp, headr->procid, data, event );
	    }

	    return writeSLOGMsgInterval( headr->timestamp, headr->procid, event, one );
	}

	return writeSLOGMsgInterval( headr->timestamp, headr->procid, event, one );
    }

    return D2S_SUCCESS;
}

/****
     handles state definitions during parsing of log file when slog conversion
     begins.
****/
int handle_extra_state_defs(DLOG_OPEN_STATE *state)
{
    SLOG_intvltype_t intvltype;
    SLOG_bebit_t     bebit_0 = 1;
    SLOG_bebit_t     bebit_1 = 1;
    SLOG_N_assocs_t  Nassocs = 0;
    SLOG_N_args_t    Nargs   = 0;

    int ierr;
  
  /*
    checking to see if state has not already been defined 
    and also to see if the start or end event id or both
    are not already defined. DLOG2SLOG requires unique event ids'
    to distinguish between DLOG records.
  */
    if ((findState_strtEvnt(state->endetype) != D2S_ERROR) ||
	(findState_endEvnt(state->startetype) != D2S_ERROR))
    {
	fprintf(stderr, __FILE__":%d: event ids defined for state %s already "
	    "exist. Use MPE_Log_get_event_number() to define new event ids.\n",
	    __LINE__,state->description); 
	return D2S_ERROR;
    }

    if ((findState_strtEvnt(state->startetype) != D2S_ERROR) &&
	  (findState_endEvnt(state->endetype) != D2S_ERROR))
    {
	return D2S_SUCCESS;
    }

    if ( addState(state->stateid, state->startetype, state->endetype,
	       state->color, state->description) == D2S_ERROR )
    {
	return D2S_ERROR;
    }

    intvltype = (unsigned int)(state_id - 1);

    ierr = SLOG_RDEF_AddExtraRecDef( slog, intvltype, bebit_0, bebit_1,
                                   Nassocs, Nargs );
    if ( ierr != SLOG_SUCCESS )
    {
	fprintf( stderr, __FILE__":%d: SLOG Record Definition initialization"
	     " failed. \n", __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }

    ierr = SLOG_PROF_AddExtraIntvlInfo( slog, intvltype, bebit_0, bebit_1,
                                      CLASS_TYPE,
				      state->description, state->color );
    if ( ierr != SLOG_SUCCESS )
    {
	fprintf( stderr, __FILE__":%d: SLOG_PROF_AddExtraIntvlInfo failed - "
	     "check SLOG documentation for more information. \n",
	     __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }
    
    return D2S_SUCCESS;
}
       	     
/****
     writes an SLOG event interval
****/
int writeSLOGEvent(int state_id, double start_time, double end_time, int procid)
{
  /*
    SLOG STUFF:
    we are not interested in bebits, cpu_id, thread_id, irec_iaddr
    and the rec_type is always a constant, NON_MSG_RECORD, a fixed
    interval record as opposed to a variable interval record.
  */
    SLOG_Irec irec;

  /*
    finally an slog fixed interval record is created and logged.
  */
    irec = SLOG_Irec_Create();

    if (irec == NULL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_Create returned null - "
	    "system might be low on memory.\n",__LINE__);
	return D2S_ERROR;
    }

    if (SLOG_Irec_SetMinRec(
	irec,
	(SLOG_rectype_t)NON_MSG_RECORD,
	(SLOG_intvltype_t)state_id,
	(SLOG_bebit_t)1,
	(SLOG_bebit_t)1,
	(SLOG_starttime_t)start_time,
	(SLOG_duration_t)(end_time - start_time),
	(SLOG_nodeID_t)procid,
	(SLOG_cpuID_t)0,
	(SLOG_threadID_t)0,
	(SLOG_iaddr_t)0 ) == SLOG_FAIL)
    {
	SLOG_Irec_Free(irec);
	fprintf(stderr, __FILE__":%d: SLOG_Irec_SetMinRec returns failure - "
	    "check SLOG documentation for more information.\n",__LINE__);
	return D2S_ERROR;
    }

    if (SLOG_Irec_ToOutputStream( slog, irec ) == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_ToOutputStream returns failure - "
	    "check SLOG documentation for more information.\n", __LINE__);
	SLOG_Irec_Free(irec);
	return D2S_ERROR;
    }

    SLOG_Irec_Free(irec);
    return D2S_SUCCESS;
}

/****
     writes an SLOG interval
****/
int writeSLOGInterval(double timestamp, int procid, struct list_elemnt one)
{
  /*
    SLOG STUFF:
    we are not interested in bebits, cpu_id, thread_id, irec_iaddr
    and the rec_type is always a constant, NON_MSG_RECORD, a fixed
    interval record as opposed to a variable interval record.
  */
    SLOG_Irec irec;

  /*
    finally an slog fixed interval record is created and logged.
  */
    irec = SLOG_Irec_Create();

    if (irec == NULL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_Create returned null - "
	    "system might be low on memory.\n",__LINE__);
	return D2S_ERROR;
    }

    if (SLOG_Irec_SetMinRec( 
	irec, 
	(SLOG_rectype_t)NON_MSG_RECORD, 
	(SLOG_intvltype_t)one.state_id,
	(SLOG_bebit_t)1, 
	(SLOG_bebit_t)1, 
	(SLOG_starttime_t)one.start_time, 
	(SLOG_duration_t)(timestamp - one.start_time),
	(SLOG_nodeID_t)procid, 
	(SLOG_cpuID_t)0, 
	(SLOG_threadID_t)0,
	(SLOG_iaddr_t)0 ) == SLOG_FAIL)
    {
	SLOG_Irec_Free(irec);
	fprintf(stderr, __FILE__":%d: SLOG_Irec_SetMinRec returns failure - "
	    "check SLOG documentation for more information.\n",__LINE__);
	return D2S_ERROR;
    }

    if (SLOG_Irec_ToOutputStream( slog, irec ) == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_ToOutputStream returns failure - "
	    "check SLOG documentation for more information.\n", __LINE__);
	SLOG_Irec_Free(irec);
	return D2S_ERROR;
    }

    SLOG_Irec_Free(irec);
    return D2S_SUCCESS;
}

/****
     writes an SLOG interval
****/
int writeSLOGMsgInterval(double timestamp, int procid, int event, struct list_elemnt one)
{
  /*
    SLOG STUFF:
    we are not interested in bebits, cpu_id, thread_id, irec_iaddr
    and the rec_type is always a constant, NON_MSG_RECORD, a fixed
    interval record as opposed to a variable interval record.
  */
    SLOG_Irec irec;
    SLOG_intvltype_t irec_intvltype;

  /*
    finally an slog fixed interval record is created and logged.
  */
    irec = SLOG_Irec_Create();

    if (irec == NULL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_Create returned null - "
	    "system might be low on memory.\n", __LINE__);
	return D2S_ERROR;
    }

    if (event == LOG_MESG_RECV)
	irec_intvltype = FORWARD_MSG;
    else
	irec_intvltype = BACKWARD_MSG;
    
    if (SLOG_Irec_SetMinRec(
	irec,
	(SLOG_rectype_t)MSG_RECORD,
	irec_intvltype,
	(SLOG_bebit_t)1,
	(SLOG_bebit_t)1,
	(SLOG_starttime_t)one.start_time,
	(SLOG_duration_t)timestamp - one.start_time,
	(SLOG_nodeID_t)one.process_id,
	(SLOG_cpuID_t)0,
	(SLOG_threadID_t)0,
	(SLOG_iaddr_t)0,
	(SLOG_nodeID_t)one.data,
	(SLOG_cpuID_t)0,
	(SLOG_threadID_t)0 ) == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_SetMinRec returns failure - "
	    "check SLOG documentation for more information.\n",__LINE__);
	SLOG_Irec_Free(irec);
	return D2S_ERROR;
    }

    if (SLOG_Irec_ToOutputStream( slog, irec ) == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_ToOutputStream returns failure - "
	    "check SLOG documentation for more information.\n",
	     __LINE__);
	SLOG_Irec_Free(irec);
	return D2S_ERROR;
    }

    SLOG_Irec_Free(irec);
    return D2S_SUCCESS;
}

/****
     add incoming event to list of start events. aka "list_first/list_last"
     list. 
****/
int handleStartEvent(int stat_id, double start_time, int proc_id, int data)
{
  /*
    SLOG STUFF:
    we are not interested in bebits, cpu_id, thread_id, irec_iaddr
    and the rec_type is always a constant, NON_MSG_RECORD, a fixed
    interval record as opposed to a variable interval record.
  */

    if (addToList( stat_id, data, proc_id, start_time) == D2S_ERROR)
	return D2S_ERROR;

    if (SLOG_Irec_ReserveSpace(
		slog,
		(SLOG_rectype_t)NON_MSG_RECORD,
		(SLOG_intvltype_t)stat_id,
		(SLOG_bebit_t)1, 
		(SLOG_bebit_t)1,
		(SLOG_starttime_t)start_time ) == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_ReserveSpace returns FAILURE"
	    " - system might have run out of memory. Check SLOG "
	    "documentation for more information.\n", __LINE__);
	return D2S_ERROR;
    }

    return D2S_SUCCESS;
}

int handleStartMsgEvent(int stat_id, double start_time, int proc_id, int data, int etype)
{
  /*
    SLOG STUFF:
    we are not interested in bebits, cpu_id, thread_id, irec_iaddr
    and the rec_type is always a constant, NON_MSG_RECORD, a fixed
    interval record as opposed to a variable interval record.
  */
    int error = SLOG_SUCCESS;

    if (addToMsgList( stat_id, data, proc_id, etype, start_time) == D2S_ERROR)
	return D2S_ERROR;
    
    if (etype == LOG_MESG_SEND)
    {
	error = SLOG_Irec_ReserveSpace(
	    slog,
	    (SLOG_rectype_t)MSG_RECORD,
	    (SLOG_intvltype_t)FORWARD_MSG,
	    (SLOG_bebit_t)1,
	    (SLOG_bebit_t)1,
	    (SLOG_starttime_t)start_time );
    }
    else
    {
	error = SLOG_Irec_ReserveSpace(
	    slog,
	    (SLOG_rectype_t)MSG_RECORD,
	    (SLOG_intvltype_t)BACKWARD_MSG,
	    (SLOG_bebit_t)1,
	    (SLOG_bebit_t)1,
	    (SLOG_starttime_t)start_time );
    }

    if (error == SLOG_FAIL)
    {
	fprintf(stderr, __FILE__":%d: SLOG_Irec_ReserveSpace returns FAILURE"
	    " - system might have run out of memory. Check SLOG "
	    "documentation for more information.\n", __LINE__);
	return D2S_ERROR;
    }

    return D2S_SUCCESS;
}

/****
     initialzes the thread table.
****/
int init_SLOG_TTAB()
{
    int ierr, ii;

    SLOG_nodeID_t      node_id;
    SLOG_threadID_t    thread_id    = 0;
    SLOG_OSprocessID_t OSprocess_id = 0;
    SLOG_OSthreadID_t  OSthread_id  = 0;
    SLOG_appID_t       app_id;

    if ( SLOG_TTAB_Open( slog ) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_TTAB_Open() fails! \n",__LINE__ );
	DLOG_free_resources();
	return D2S_ERROR;
    }

    for( ii=0 ; ii <= proc_num ; ii++)
    {
	node_id = app_id = ii;
	ierr = SLOG_TTAB_AddThreadInfo( slog, node_id, thread_id,
				    OSprocess_id, OSthread_id, app_id);

	if ( ierr != SLOG_SUCCESS )
	{
	    fprintf( stderr,__FILE__":%d: SLOG Thread Table initialization "
	       "failed.\n",__LINE__);
	    DLOG_free_resources();
	    return D2S_ERROR;
	}
    }

    if ((ierr = SLOG_TTAB_Close( slog )) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_TTAB_Close() fails! \n", __LINE__ );
	DLOG_free_resources();
	return D2S_ERROR;
    }
    
    return D2S_SUCCESS;
}

/****
     initializes the profiling as well as the record definition tables.
     for details look at the slog api documentation.
     dlog counterparts:
     state definitions  =  record definition + profiling
****/
int init_SLOG_PROF_RECDEF()
{
    struct state_info *pState;
    int ierr;
    struct state_info *one  = NULL,
                      *two  = NULL;

    if ( SLOG_PROF_Open( slog ) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_PROF_Open() fails! \n", __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }

    /* initialization of the interval table */

    for( one = first ; one != NULL ; one = two )
    {
	two = one->next;
	if ((one->start_event_num != LOG_MESG_SEND) || (one->end_event_num != LOG_MESG_RECV))
	{
	    ierr = SLOG_PROF_AddIntvlInfo(
			slog,
			(SLOG_intvltype_t)one->state_id,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			CLASS_TYPE,
			one->description,
			one->color,
			(SLOG_N_args_t)0 );
	}
	else
	{
	    ierr = SLOG_PROF_AddIntvlInfo(
			slog,
			FORWARD_MSG,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			FORWARD_MSG_CLASSTYPE,
			FORWARD_MSG_LABEL,
			FORWARD_MSG_COLOR,
			(SLOG_N_args_t)0 );
	    if ( ierr != SLOG_SUCCESS )
	    {
		fprintf( stderr,__FILE__":%d: SLOG Profile initialization failed.\n",
		    __LINE__);
		DLOG_free_resources();
		return D2S_ERROR;
	    }
	    ierr = SLOG_PROF_AddIntvlInfo(
			slog,
			BACKWARD_MSG,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			BACKWARD_MSG_CLASSTYPE,
			BACKWARD_MSG_LABEL,
			BACKWARD_MSG_COLOR,
			(SLOG_N_args_t)0 );
	}
	if ( ierr != SLOG_SUCCESS )
	{
	    fprintf( stderr,__FILE__":%d: SLOG Profile initialization failed. \n",
		__LINE__);
	    DLOG_free_resources();
	    return D2S_ERROR;
	}
    }

    pState = pEventStateList;
    while (pState)
    {
	if (SLOG_PROF_AddIntvlInfo(
	    slog,
	    (SLOG_intvltype_t)pState->state_id,
	    (SLOG_bebit_t)1,
	    (SLOG_bebit_t)1,
	    CLASS_TYPE,
	    pState->description,
	    pState->color,
	    (SLOG_N_args_t)0 ) != SLOG_SUCCESS)
	{
	    fprintf( stderr,__FILE__":%d: SLOG Profile initialization failed. \n", __LINE__);
	    DLOG_free_resources();
	    return D2S_ERROR;
	}
	pState = pState->next;
    }

    if ( SLOG_PROF_SetExtraNumOfIntvlInfos( slog, EXTRA_STATES ) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_PROF_SetExtraNumOfIntvlInfos() fails! \n", __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }
    if ( SLOG_RDEF_Open( slog ) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_RDEF_Open() fails! \n", __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }

    for( one = first ; one != NULL ; one = two )
    {
	two = one->next;

	if ((one->start_event_num != LOG_MESG_SEND) || (one->end_event_num != LOG_MESG_RECV))
	{
	    ierr = SLOG_RDEF_AddRecDef(
			slog,
			(SLOG_intvltype_t)one->state_id,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			(SLOG_N_assocs_t)0,
			(SLOG_N_args_t)0 );
	}
	else
	{
	    ierr = SLOG_RDEF_AddRecDef(
			slog,
			FORWARD_MSG,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			(SLOG_N_assocs_t)0,
			(SLOG_N_args_t)0 );
	    if ( ierr != SLOG_SUCCESS )
	    {
		fprintf( stderr,__FILE__":%d: SLOG Record Definition initialization failed. \n",__LINE__);
		DLOG_free_resources();
		return D2S_ERROR;
	    }
	    ierr = SLOG_RDEF_AddRecDef(
			slog,
			BACKWARD_MSG,
			(SLOG_bebit_t)1,
			(SLOG_bebit_t)1,
			(SLOG_N_assocs_t)0,
			(SLOG_N_args_t)0 );
	}
	if ( ierr != SLOG_SUCCESS )
	{
	    fprintf( stderr,__FILE__":%d: SLOG Record Definition initialization failed. \n", __LINE__);
	    DLOG_free_resources();
	    return D2S_ERROR;
	}
    }

    pState = pEventStateList;
    while (pState)
    {
	if (SLOG_RDEF_AddRecDef(
	    slog,
	    (SLOG_intvltype_t)pState->state_id,
	    (SLOG_bebit_t)1,
	    (SLOG_bebit_t)1,
	    (SLOG_N_assocs_t)0,
	    (SLOG_N_args_t)0 ) != SLOG_SUCCESS)
	{
	    fprintf( stderr,__FILE__":%d: SLOG Record Definition initialization failed. \n", __LINE__);
	    DLOG_free_resources();
	    return D2S_ERROR;
	}
	pState = pState->next;
    }

    if ( SLOG_RDEF_SetExtraNumOfRecDefs( slog,EXTRA_STATES ) != SLOG_SUCCESS )
    {
	fprintf(stderr, __FILE__":%d: SLOG_RDEF_SetExtraNumOfRecDefs() fails! \n", __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }
    
    return D2S_SUCCESS;
}

int addEventState(int state, char *color, char *description)
{
    struct state_info *pState;

    pState = pEventStateList;
    while (pState)
    {
	if (pState->state_id == state)
	    return D2S_SUCCESS;
	pState = pState->next;
    }

    pState = ( struct state_info *) MALLOC( sizeof(struct state_info) );
    pState->state_id = state;
    if (color)
	strcpy(pState->color, color);
    else
	pState->color[0] = '\0';
    if (description)
	strcpy(pState->description, description);
    else
	pState->description[0] = '\0';
    pState->end_event_num = -1;
    pState->start_event_num = -1;

    pState->next = pEventStateList;
    pEventStateList = pState;

    return D2S_SUCCESS;
}

/****
     add a new state definition to the end of the state definition list.
****/
int addState(int stat_id, int strt_id, int end_id, char *colr, char *desc)
{
    struct state_info *temp_ptr;
    int s_id, e_id;

    s_id = findState_strtEvnt(strt_id);
    e_id = findState_endEvnt(end_id);
    if ((s_id != D2S_ERROR) && (e_id != D2S_ERROR))
    {
	if (s_id == e_id)
	{
	    replace_state_in_list(strt_id, end_id, colr, desc);
	    return D2S_SUCCESS;
	}
	else
	{
	    fprintf(stderr,__FILE__":%d: event ids defined for state %s already "
	      "exist. Use DLOG_GetNextEvent() to define new event ids.",
	      __LINE__,desc);
	    return D2S_ERROR;
	}
    }
    else if ((s_id != D2S_ERROR) || (e_id != D2S_ERROR))
    {
	fprintf(stderr,__FILE__":%d: event ids defined for state %s already "
	    "exist. Use DLOG_GetNextEvent() to define new event ids.",
	    __LINE__,desc);
	return D2S_ERROR;
    }

    temp_ptr  =  ( struct state_info *) MALLOC( sizeof(struct state_info) );

    if (temp_ptr == NULL)
    {
	fprintf(stderr,__FILE__":%d: not enough memory for start event list!\n",
	    __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }
    if (stat_id != MSG_STATE)
	temp_ptr->state_id           = get_new_state_id();
    else
	temp_ptr->state_id           = MSG_STATE;
    temp_ptr->start_event_num    = strt_id;
    temp_ptr->end_event_num      = end_id;
    /*temp_ptr->color = (char *)(MALLOC(strlen(colr)+1));      */
    /*temp_ptr->description = (char *)(MALLOC(strlen(desc)+1));*/
    strncpy(temp_ptr->color, colr, DLOG_COLOR_LENGTH);
    temp_ptr->color[DLOG_COLOR_LENGTH-1] = '\0';
    strncpy(temp_ptr->description, desc, DLOG_DESCRIPTION_LENGTH-1);
    temp_ptr->description[DLOG_DESCRIPTION_LENGTH-1] = '\0';
    temp_ptr->next               = NULL;

    if (first == NULL)
    {
	first = temp_ptr;
	last  = temp_ptr;
    }
    else
    {
	last->next = temp_ptr;
	last       = temp_ptr;
    }

    return D2S_SUCCESS;
}

int replace_state_in_list(int start_id, int end_id, 
			  char *colr, char *desc)
{
    struct state_info *one = NULL,
	              *two = NULL;
    for (one = first; one != NULL; one = two)
    {
	two = one->next;
	if (one->start_event_num == start_id)
	{
	    one->start_event_num    = start_id;
	    one->end_event_num      = end_id;
	    strncpy(one->color, colr, DLOG_COLOR_LENGTH-1);
	    one->color[DLOG_COLOR_LENGTH-1] = '\0';
	    strncpy(one->description, desc, DLOG_DESCRIPTION_LENGTH-1);
	    one->description[DLOG_DESCRIPTION_LENGTH-1] = '\0';
	    return D2S_SUCCESS;
	}
    }

    return D2S_ERROR;
} 

/****
     finds a state id for the given start event id from the state def list.
****/
int findState_strtEvnt(int strt_id)
{
    struct state_info *p;

    for (p = first; p != NULL; p = p->next)
    {
	if (p->start_event_num == strt_id)
	    return p->state_id;
    }

    return D2S_ERROR;
}

/****
     finds a state id for the given end event id from the state def list.
****/
int findState_endEvnt(int end_id)
{
    struct state_info *p;

    for (p = first; p != NULL; p = p->next)
    {
	if (p->end_event_num == end_id)
	    return p->state_id;
    }
    return D2S_ERROR;
}

/****
     frees up memory malloced for state definitions in the list of state defs.
****/
void DLOG_freeStateInfo(void)
{
    struct state_info *one = NULL, *two = NULL;

    for (one = first; one != NULL; one = two)
    {
	two = one->next;
	free(one);
    }
}

/****
     adds a start event to the start event list. the only relevant info 
     needed for slogging are state id, the data from DLOG_RAWEVENT and the 
     start time. the start time is needed to calculate the interval duration
     when the end event is found. the new element is added to the front of the
     list - why??? - well, just in case there are nested events(threads) or 
     non-blocking MPI calls. hasnt been tested on those two conditions yet.
****/
int addToList(int stat_id, int data, int proc_id, double strt_time)
{
    struct list_elemnt *temp_ptr =
	(struct list_elemnt *)MALLOC(sizeof(struct list_elemnt));

    if (temp_ptr == NULL)
    {
	fprintf(stderr,__FILE__":%d: not enough memory for start event list!\n",
	    __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }

    temp_ptr->state_id        = stat_id;
    temp_ptr->data            = data;
    temp_ptr->process_id      = proc_id;
    temp_ptr->start_time      = strt_time;
    temp_ptr->next            = NULL;

    if (list_first == NULL)
    {
	list_first = temp_ptr;
	list_last  = temp_ptr;
    }
    else
    {
	temp_ptr->next = list_first;
	list_first     = temp_ptr;
    }
    return D2S_SUCCESS;
}

/****
     adds a start event to the start event list. the only relevant info 
     needed for slogging are state id, the data from DLOG_RAWEVENT and the 
     start time. the start time is needed to calculate the interval duration
     when the end event is found. the new element is added to the front of the
     list - why??? - well, just in case there are nested events(threads) or 
     non-blocking MPI calls. hasnt been tested on those two conditions yet.
****/
int addToMsgList(int stat_id, int data, int proc_id,
                 int rec_type, double strt_time)
{
    struct list_elemnt *temp_ptr =
	(struct list_elemnt *)MALLOC(sizeof(struct list_elemnt));

    if (temp_ptr == NULL)
    {
	fprintf(stderr,__FILE__":%d: not enough memory for message list!\n",
	    __LINE__);
	DLOG_free_resources();
	return D2S_ERROR;
    }

    temp_ptr->state_id        = stat_id;
    temp_ptr->data            = data;
    temp_ptr->process_id      = proc_id;
    temp_ptr->rectype         = rec_type;
    temp_ptr->start_time      = strt_time;
    temp_ptr->next            = NULL;

    if (msg_list_first == NULL)
    {
	msg_list_first = temp_ptr;
	msg_list_last  = temp_ptr;
    }
    else
    {
	msg_list_last->next = temp_ptr;
	msg_list_last       = temp_ptr;
    }
    return D2S_SUCCESS;
}

/****
     find a start element for the corresponding end event info passed in the 
     arguments from the start event list. 
     that start event is then removed from the list and copied to pResult.
****/
int remove_element(int stat_id, int data,int procid, struct list_elemnt *pResult)
{
    struct list_elemnt *pElement   = NULL,
                       *pNext   = NULL,
                       *pTrailer = NULL;

    for (pElement = list_first, pTrailer = list_first; pElement != NULL; pElement = pNext)
    {
	pNext = pElement->next;
	if ((pElement->state_id != MSG_STATE) && 
	    (pElement->state_id == stat_id) && 
	    (pElement->process_id == procid))
	{
	    pResult->state_id   = pElement->state_id;
	    pResult->data       = pElement->data;
	    pResult->process_id = pElement->process_id;
	    pResult->start_time = pElement->start_time;
	    pResult->next = NULL;
	    if (pElement == list_first)
	    {
		if (pElement->next == NULL)
		{
		    list_first = NULL;
		    list_last  = NULL;
		}
		else
		{
		    list_first = pElement->next;
		}
	    }
	    else
	    {
		pTrailer->next = pElement->next;
		if (pElement == list_last)
		{
		    list_last = pTrailer;
		}
	    }
	    free(pElement);
	    return D2S_SUCCESS;
	}
	if (pElement != list_first)
	{
	    pTrailer = pTrailer->next;
	}
    }
    return D2S_ERROR;
}

int remove_msg_elemnt(int stat_id, int data, int procid, int record_type,
                    struct list_elemnt *pResult)
{
    struct list_elemnt *pElement   = NULL,
                       *pNext   = NULL,
                       *pTrailer = NULL;

    for(pElement = msg_list_first, pTrailer = msg_list_first; pElement != NULL; pElement = pNext)
    {
	pNext = pElement->next;
	if ((pElement->state_id == MSG_STATE) && (pElement->state_id == stat_id) && 
	    (pElement->process_id == data) && (pElement->data == procid) &&
	    (pElement->rectype != record_type))
	{
	    pResult->state_id   = pElement->state_id;
	    pResult->data       = pElement->data;
	    pResult->process_id = pElement->process_id;
	    pResult->rectype    = pElement->rectype;
	    pResult->start_time = pElement->start_time;
	    pResult->next = NULL;
	    if (pElement == msg_list_first)
	    {
		if (pElement->next == NULL)
		{
		    msg_list_first = NULL;
		    msg_list_last  = NULL;
		}
		else
		    msg_list_first = pElement->next;
	    }
	    else
	    {
		pTrailer->next = pElement->next;
		if (pElement == msg_list_last)
		    msg_list_last = pTrailer;
	    }
	    free(pElement);
	    return D2S_SUCCESS;
	}
	if (pElement != msg_list_first)
	{
	    pTrailer = pTrailer->next;
	}
    }
    return D2S_ERROR;
}

/****
     frees memory malloced to the start event list.
     in theory there shouldnt be any elements left in this list at the end
     of the second pass but wierd things happen when you dont know how the
     dlog file got generated.
****/
void freeList(void)
{
    struct list_elemnt *one = NULL,
                       *two = NULL;

    for(one = list_first; one != NULL; one = two)
    {
	two = one->next;
	free(one);
    }
    list_first = NULL;
    list_last = NULL;
}
/****
     frees memory malloced to the message event list.
****/
void freeMsgList(void)
{
    struct list_elemnt *one = NULL,
                       *two = NULL;

    for(one = msg_list_first; one != NULL; one = two)
    {
	two = one->next;
	free(one);
    }
    msg_list_first = NULL;
    msg_list_last = NULL;
}

int get_new_state_id(void) 
{
    return state_id++;
}






