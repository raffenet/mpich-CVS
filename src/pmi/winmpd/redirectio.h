/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef REDIRECT_IO_H
#define REDIRECT_IO_H

#include "mpichconf.h"
#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else
#endif

typedef struct RedirectIOArg_st
{
    int *m_pbfdStopIOSignalSocket;
    HANDLE hReadyEvent;
} RedirectIOArg;

void RedirectIOThread(RedirectIOArg *pArg);
void InitRedirection();
void FinalizeRedirection();

#endif
