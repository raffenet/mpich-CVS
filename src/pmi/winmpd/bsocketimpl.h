/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef BSOCKETIMPL_H
#define BSOCKETIMPL_H

#include "bsocket.h"

#ifdef __cplusplus
extern "C" {
#endif

//#include "mpidimpl.h"
#ifdef USE_WINCONF_H
#include "winmpichconf.h"
#else
#include "mpichconf.h"
#endif
#include "blockallocator.h"

#ifdef __cplusplus
}
#endif

#endif
