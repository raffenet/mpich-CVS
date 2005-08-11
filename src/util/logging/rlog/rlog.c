/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "rlog.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mpimem.h"

#include "mpi.h"
/*#define RLOG_timestamp PMPI_Wtime*/
#include "mpichtimer.h"
double RLOG_timestamp()
{
    double d;
    MPID_Time_t t;
    MPID_Wtime(&t);
    MPID_Wtime_todouble(&t, &d);
    return d;
}

static int WriteFileData(const char *pBuffer, int length, FILE *fout)
{
    int num_written;

    while (length)
    {
	num_written = (int)fwrite(pBuffer, 1, length, fout);
	if (num_written == -1)
	{
	    MPIU_Error_printf("Error: fwrite failed - %s\n", strerror(errno));
	    return errno;
	}

	/*printf("fwrite(%d)", num_written);fflush(stdout);*/

	length -= num_written;
	pBuffer += num_written;
    }
    return 0;
}

static void MarkDiskStart(RLOG_Struct *pRLOG)
{
    pRLOG->DiskEvent.start_time = RLOG_timestamp() - pRLOG->dFirstTimestamp;
    pRLOG->DiskEvent.recursion = pRLOG->nRecursion;
}

static void WriteDiskEvent(RLOG_Struct *pRLOG)
{
    pRLOG->DiskEvent.end_time = RLOG_timestamp() - pRLOG->dFirstTimestamp;
    WriteFileData((const char *)&pRLOG->DiskHeader, sizeof(RLOG_HEADER), pRLOG->pOutput->f);
    WriteFileData((const char *)&pRLOG->DiskEvent, sizeof(RLOG_EVENT), pRLOG->pOutput->f);
}

RLOG_Struct* RLOG_InitLog(int rank, int size)
{
    RLOG_Struct* pRLOG;

    pRLOG = (RLOG_Struct*)MPIU_Malloc(sizeof(RLOG_Struct));
    if (pRLOG == NULL)
	return NULL;

    pRLOG->nRank = rank;
    pRLOG->nSize = size;
    pRLOG->nRecursion = 0;
    pRLOG->nCurEventId = RLOG_FIRST_EVENT_ID;
    pRLOG->dFirstTimestamp = 0.0;
    MPIU_Snprintf(pRLOG->pszFileName, 256, "log%d.irlog", rank);

    pRLOG->pOutput = NULL;
    pRLOG->pOutput = IRLOG_CreateOutputStruct(pRLOG->pszFileName);
    if (pRLOG->pOutput == NULL)
    {
	MPIU_Error_printf("RLOG Error: unable to allocate an output structure.\n");
	MPIU_Free(pRLOG);
	return NULL;
    }

    RLOG_EnableLogging(pRLOG);

    /* save the parts of the header and event that do not change */
    pRLOG->DiskEvent.event = RLOG_GetNextEventID(pRLOG);
    pRLOG->DiskEvent.rank = rank;
    pRLOG->DiskHeader.type = RLOG_EVENT_TYPE;
    pRLOG->DiskHeader.length = sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT);
    /* put the description of the state in the log file */
    RLOG_DescribeState(pRLOG, pRLOG->DiskEvent.event, "RLOG_DISK", "255   0   0");

    RLOG_DisableLogging(pRLOG);

    return pRLOG;
}

int RLOG_FinishLog(RLOG_Struct* pRLOG)
{
    RLOG_HEADER header;

    /* FixUpBuffer(); */
    MarkDiskStart(pRLOG);
    WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
    WriteDiskEvent(pRLOG);
    pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;

    header.type = RLOG_ENDLOG_TYPE;
    header.length = sizeof(RLOG_HEADER);
    WriteFileData((const char *)&header, sizeof(RLOG_HEADER), pRLOG->pOutput->f);

    IRLOG_CloseOutputStruct(&pRLOG->pOutput);

    return 0;
}

void RLOG_EnableLogging(RLOG_Struct* pRLOG)
{
    pRLOG->bLogging = RLOG_TRUE;
}

void RLOG_DisableLogging(RLOG_Struct* pRLOG)
{
    pRLOG->bLogging = RLOG_FALSE;
}

void RLOG_SaveFirstTimestamp(RLOG_Struct* pRLOG)
{
    pRLOG->dFirstTimestamp = RLOG_timestamp();
}

int RLOG_GetNextEventID(RLOG_Struct* pRLOG)
{
    return pRLOG->nCurEventId++;
}

void WriteCurrentDataAndLogEvent(RLOG_Struct *pRLOG, int event, double starttime, double endtime, int recursion)
{
    double disk_start, disk_end;
    /* save the disk event start */
    disk_start = RLOG_timestamp();

    /* write the data and reset the buffer */
    WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
    pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;

    /* save the disk event end */
    disk_end = RLOG_timestamp();

    /* save the event that was interrupted by the disk event */
    RLOG_LogEvent(pRLOG, event, starttime, endtime, recursion);

    /* save the disk event */
    RLOG_LogEvent(pRLOG, pRLOG->DiskEvent.event, disk_start, disk_end, recursion);
}

/* maybe these casts will be faster than using local pointer variables */
#define RLOG_HEADER_CAST() ((RLOG_HEADER*)pRLOG->pOutput->pCurHeader)
#define RLOG_EVENT_CAST()  ((RLOG_EVENT*)((char*)pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER)))
#define RLOG_IARROW_CAST()  ((RLOG_IARROW*)((char*)pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER)))

#ifndef RLOG_LogEvent
void RLOG_LogEvent(RLOG_Struct *pRLOG, int event, double starttime, double endtime, int recursion)
{
    if (pRLOG->bLogging == RLOG_FALSE)
	return;

    if (pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT) > pRLOG->pOutput->pEnd)
    {
	WriteCurrentDataAndLogEvent(pRLOG, event, starttime, endtime, recursion);
	return;
    }

    RLOG_HEADER_CAST()->type = RLOG_EVENT_TYPE;
    RLOG_HEADER_CAST()->length = sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT);
    RLOG_EVENT_CAST()->rank = pRLOG->nRank;
    RLOG_EVENT_CAST()->end_time = endtime - pRLOG->dFirstTimestamp;
    RLOG_EVENT_CAST()->start_time = starttime - pRLOG->dFirstTimestamp;
    RLOG_EVENT_CAST()->event = event;
    RLOG_EVENT_CAST()->recursion = recursion;

    /* advance the current position pointer */
    pRLOG->pOutput->pCurHeader += sizeof(RLOG_HEADER) + sizeof(RLOG_EVENT);
}
#endif

void RLOG_LogSend(RLOG_Struct* pRLOG, int dest, int tag, int size)
{
    if (pRLOG->bLogging == RLOG_FALSE)
	return;

    if (pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW) > pRLOG->pOutput->pEnd)
    {
	MarkDiskStart(pRLOG);
	WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
	WriteDiskEvent(pRLOG);
	pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;
    }

    RLOG_HEADER_CAST()->type = RLOG_IARROW_TYPE;
    RLOG_HEADER_CAST()->length = sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW);

    RLOG_IARROW_CAST()->sendrecv = RLOG_SENDER;
    RLOG_IARROW_CAST()->rank = pRLOG->nRank;
    RLOG_IARROW_CAST()->remote = dest;
    RLOG_IARROW_CAST()->tag = tag;
    RLOG_IARROW_CAST()->length = size;
    RLOG_IARROW_CAST()->timestamp = RLOG_timestamp() - pRLOG->dFirstTimestamp;

    /* advance the current position pointer */
    pRLOG->pOutput->pCurHeader += sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW);
}

void RLOG_LogRecv(RLOG_Struct* pRLOG, int src, int tag, int size)
{
    if (pRLOG->bLogging == RLOG_FALSE)
	return;

    if (pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW) > pRLOG->pOutput->pEnd)
    {
	MarkDiskStart(pRLOG);
	WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
	WriteDiskEvent(pRLOG);
	pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;
    }

    RLOG_HEADER_CAST()->type = RLOG_IARROW_TYPE;
    RLOG_HEADER_CAST()->length = sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW);

    RLOG_IARROW_CAST()->sendrecv = RLOG_RECEIVER;
    RLOG_IARROW_CAST()->rank = pRLOG->nRank;
    RLOG_IARROW_CAST()->remote = src;
    RLOG_IARROW_CAST()->tag = tag;
    RLOG_IARROW_CAST()->length = size;
    RLOG_IARROW_CAST()->timestamp = RLOG_timestamp() - pRLOG->dFirstTimestamp;

    /* advance the current position pointer */
    pRLOG->pOutput->pCurHeader += sizeof(RLOG_HEADER) + sizeof(RLOG_IARROW);
}

void RLOG_LogCommID(RLOG_Struct* pRLOG, int comm_id)
{
    RLOG_HEADER *pHeader;
    RLOG_COMM *pComm;

    if (pRLOG->bLogging == RLOG_FALSE)
	return;

    if (pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_COMM) > pRLOG->pOutput->pEnd)
    {
	MarkDiskStart(pRLOG);
	WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
	WriteDiskEvent(pRLOG);
	pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;
    }

    pHeader = (RLOG_HEADER*)pRLOG->pOutput->pCurHeader;
    pComm = (RLOG_COMM*)((char*)pHeader + sizeof(RLOG_HEADER));

    pHeader->type = RLOG_COMM_TYPE;
    pHeader->length = sizeof(RLOG_HEADER) + sizeof(RLOG_COMM);
    pComm->newcomm = comm_id;
    pComm->rank = pRLOG->nRank;

    /* advance the current position pointer */
    pRLOG->pOutput->pCurHeader += pHeader->length;
}

void RLOG_DescribeState(RLOG_Struct* pRLOG, int state, char *name, char *color)
{
    RLOG_HEADER *pHeader;
    RLOG_STATE *pState;

    if (pRLOG->bLogging == RLOG_FALSE)
	return;

    if (pRLOG->pOutput->pCurHeader + sizeof(RLOG_HEADER) + sizeof(RLOG_STATE) > pRLOG->pOutput->pEnd)
    {
	MarkDiskStart(pRLOG);
	WriteFileData(pRLOG->pOutput->buffer, pRLOG->pOutput->pCurHeader - pRLOG->pOutput->buffer, pRLOG->pOutput->f);
	WriteDiskEvent(pRLOG);
	pRLOG->pOutput->pCurHeader = pRLOG->pOutput->buffer;
    }

    pHeader = (RLOG_HEADER*)pRLOG->pOutput->pCurHeader;
    pState = (RLOG_STATE*)((char*)pHeader + sizeof(RLOG_HEADER));

    pHeader->type = RLOG_STATE_TYPE;
    pHeader->length = sizeof(RLOG_HEADER) + sizeof(RLOG_STATE);

    pState->event = state;
    if (color)
    {
	MPIU_Strncpy(pState->color, color, RLOG_COLOR_LENGTH);
	pState->color[RLOG_COLOR_LENGTH-1] = '\0';
    }
    else
    {
	pState->color[0] = '\0';
    }
    if (name)
    {
	MPIU_Strncpy(pState->description, name, RLOG_DESCRIPTION_LENGTH);
	pState->description[RLOG_DESCRIPTION_LENGTH-1] = '\0';
    }
    else
    {
	pState->description[0] = '\0';
    }

    /* advance the current position pointer */
    pRLOG->pOutput->pCurHeader += pHeader->length;
}
