/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPDUTIL_H
#define MPDUTIL_H

#ifndef BOOL
#define BOOL int
#endif

BOOL ReadStringMax(int bfd, char *str, int max);
BOOL ReadString(int bfd, char *str);
BOOL ReadStringTimeout(int bfd, char *str, int timeout);
int WriteString(int bfd, char *str);
int ConnectToMPD(char *host, int port, char *phrase, int *pbfd);
void MakeLoop(int *pbfdRead, int *pbfdWrite);

void err_printf(char *str, ...);

#endif
