/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LABELOUT_H_INCLUDED
#define LABELOUT_H_INCLUDED

typedef struct { int readOut[2], readErr[2]; } IOLabelSetup;

int IOLabelSetupFDs( IOLabelSetup * );
int IOLabelSetupInClient( IOLabelSetup * );
int IOLabelSetupFinishInServer( IOLabelSetup *, ProcessState * );
int IOLabelCheckEnv( void );

#endif
