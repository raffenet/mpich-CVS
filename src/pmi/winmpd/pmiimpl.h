/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PMIIMPL_H
#define PMIIMPL_H

#include "pmi.h"
#include "mpd.h"
#include "mpidi_ch3_conf.h"
#include "mpichconf.h"
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else


typedef void *HANDLE;
#define BOOL int
#define TRUE 1
#define FALSE 0
#define INFINITE 0xFFFFFFFF
#define WAIT_TIMEOUT 258
#define WAIT_OBJECT_0 0
#define MAX_PATH 260
#define DWORD unsigned long
#define WINAPI
#define LPVOID void*
typedef DWORD (WINAPI *PTHREAD_START_ROUTINE)( LPVOID lpThreadParameter);
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;
#define STD_INPUT_HANDLE    ((DWORD)-10)
#define STD_OUTPUT_HANDLE   ((DWORD)-11)
#define STD_ERROR_HANDLE    ((DWORD)-12)
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

#define LPSECURITY_ATTRIBUTES void *
#define LPCSTR const char *
HANDLE CreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);

#define SIZE_T unsigned long
HANDLE CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
    DWORD dwCreationFlags, DWORD* lpThreadId);

HANDLE GetStdHandle(DWORD nStdHandle);

#endif

#define PMI_MAX_KEY_LEN          256
#define PMI_MAX_VALUE_LEN        1024
#define PMI_MAX_KVS_NAME_LENGTH  100

#define PMI_SUCCESS     0
#define PMI_FAIL       -1

#define PMI_INITIALIZED 0
#define PMI_FINALIZED   1

extern char g_pszKVSName[PMI_MAX_KVS_NAME_LENGTH];
extern char g_pszMPDHost[100];
extern int g_nMPDPort;
extern char g_pszPMIAccount[100];
extern char g_pszPMIPassword[100];
extern char g_pszMPDPhrase[MPD_PASSPHRASE_MAX_LENGTH];
extern int g_bfdMPD;
extern int g_nIproc;
extern int g_nNproc;
extern int g_bInitFinalized;
extern HANDLE g_hSpawnMutex;
extern char g_pszIOHost[100];
extern int g_nIOPort;
extern HANDLE g_hJobThreads[100];
extern int g_nNumJobThreads;
extern BOOL g_bPMIFinalizeWaiting;

#endif
