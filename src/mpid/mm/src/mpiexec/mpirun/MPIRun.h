/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIRUN_H
#define MPIRUN_H

#ifdef WSOCK2_BEFORE_WINDOWS
#include <winsock2.h>
#endif
#include <windows.h>
#include <tchar.h>

#include "global.h"

extern long g_nHosts;
extern char g_pszExe[MAX_CMD_LENGTH];
extern char g_pszArgs[MAX_CMD_LENGTH];
extern char g_pszEnv[MAX_CMD_LENGTH];
extern char g_pszDir[MAX_PATH];
extern bool g_bNoMPI;

#endif
