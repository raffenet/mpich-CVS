/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef DLOG_H
#define DLOG_H

#include <stdio.h>

#include "mpi.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* definitions */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define DLOG_BUFFSIZE 8*1024*1024
#define DLOG_MAX_RECORD_SIZE 1024
#define DLOG_COLOR_LENGTH       3 * sizeof(double)
#define DLOG_DESCRIPTION_LENGTH 4 * sizeof(double)

#define LOG_MESG_SEND -101
#define LOG_MESG_RECV -102

/* structures */

typedef enum DLOG_RECORD_TYPE
{
    DLOG_INVALID_TYPE = 0,
    DLOG_ENDLOG_TYPE,
    DLOG_EVENT_TYPE,
    DLOG_OPEN_EVENT_TYPE,
    DLOG_ARROW_TYPE,
    DLOG_STATE_TYPE,
    DLOG_OPEN_STATE_TYPE,
    DLOG_COMM_TYPE
} DLOG_RECORD_TYPE;

typedef struct DLOG_HEADER 
{
    DLOG_RECORD_TYPE type;
    int length;
    double timestamp;
    int procid;			/* currently rank in COMM_WORLD */
    int pad;			/* keep length a multiple of sizeof(dbl) */
} DLOG_HEADER;

typedef struct DLOG_STATE
{
    int stateid;		/* integer identifier for state */
    int pad;
    char color[DLOG_COLOR_LENGTH];		/* string for color */
    char description[DLOG_DESCRIPTION_LENGTH];	/* string describing state */
} DLOG_STATE;

typedef struct DLOG_OPEN_STATE
{
    int stateid;		/* integer identifier for state */
    int startetype;		/* starting event for state */
    int endetype;		/* ending event for state */
    int pad;
    char color[DLOG_COLOR_LENGTH];		/* string for color */
    char description[DLOG_DESCRIPTION_LENGTH];	/* string describing state */
} DLOG_OPEN_STATE;

typedef struct DLOG_OPEN_EVENT
{
    int event;
    int data;
} DLOG_OPEN_EVENT;

typedef struct DLOG_EVENT
{
    int event;
    double start_time;
    double end_time;
} DLOG_EVENT;

typedef struct DLOG_ARROW
{
    int event;
    int data;
    int tag;
    int length;
} DLOG_ARROW;

typedef struct DLOG_COMM
{
    int etype;
    int newcomm;
} DLOG_COMM;

typedef struct DLOG_IOStruct
{
    FILE *f;
    DLOG_HEADER header;
    union DLOG_DATA
    {
	DLOG_STATE state;
	DLOG_OPEN_STATE ostate;
	DLOG_EVENT event;
	DLOG_OPEN_EVENT oevent;
	DLOG_ARROW arrow;
	DLOG_COMM comm;
    } record;
    char *pCurHeader;
    char *pNextHeader;
    char *pEnd;
    char buffer[DLOG_BUFFSIZE];
} DLOG_IOStruct;

typedef struct DLOG_Struct
{
    int bLogging;
    int nCurEventId;
    int nCurStateId;
    char pszFileName[256];
    int rank;
    int size;
    double dFirstTimestamp;
    int nDiskEventIDIn, nDiskEventIDOut;
    DLOG_HEADER DiskHeader;
    DLOG_OPEN_EVENT DiskEvent;

    DLOG_IOStruct *pOutput;
} DLOG_Struct;

/* function prototypes */

#define DLOG_timestamp PMPI_Wtime

DLOG_Struct* DLOG_InitLog(int rank, int size);
int DLOG_FinishLog(DLOG_Struct* pDLOG, char *filename);
void DLOG_LogEvent(DLOG_Struct *pDLOG, int event, double starttime);
void DLOG_LogOpenEvent(DLOG_Struct* pDLOG, int event, int data);
void DLOG_LogSend(DLOG_Struct* pDLOG, int dest, int tag, int size);
void DLOG_LogRecv(DLOG_Struct* pDLOG, int src, int tag, int size);
void DLOG_LogCommID(DLOG_Struct* pDLOG, int id);
void DLOG_DescribeState(DLOG_Struct* pDLOG, int state, char *name, char *color);
void DLOG_DescribeOpenState(DLOG_Struct* pDLOG, int start, int end, char *name, char *color);
void DLOG_EnableLogging(DLOG_Struct* pDLOG);
void DLOG_DisableLogging(DLOG_Struct* pDLOG);
int DLOG_GetNextEvent(DLOG_Struct* pDLOG);
void DLOG_SaveFirstTimestamp(DLOG_Struct* pDLOG);

DLOG_IOStruct *DLOG_CreateInputStruct(char *filename);
DLOG_IOStruct *DLOG_CreateOutputStruct(char *filename);
int DLOG_GetNextRecord(DLOG_IOStruct *pInput);
int DLOG_WriteRecord(DLOG_HEADER *pRecord, DLOG_IOStruct *pOutput);
int DLOG_CloseInputStruct(DLOG_IOStruct **ppInput);
int DLOG_CloseOutputStruct(DLOG_IOStruct **ppOutput);

#if defined(__cplusplus)
}
#endif

#endif
