/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef SMPD_SERVICE_H
#define SMPD_SERVICE_H

#include "smpd.h"

/* name of the service */
#define SMPD_SERVICE_NAME         "mpich2_smpd"
/* displayed name of the service */
#define SMPD_SERVICE_DISPLAY_NAME "MPICH2 Daemon (C) 2003 Argonne National Lab"

void smpd_install_service(SMPD_BOOL interact, SMPD_BOOL bSetupRestart);
SMPD_BOOL smpd_remove_service(SMPD_BOOL bErrorOnNotInstalled);
void smpd_stop_service();
void smpd_start_service();
void smpd_service_main(int argc, char *argv[]);
void smpd_service_stop();
void smpd_add_error_to_message_log(char *msg);
SMPD_BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void AddInfoToMessageLog(LPTSTR lpszMsg);

#endif
