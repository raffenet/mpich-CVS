/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id$
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPD_H
#define MPD_H

#define COPYRIGHT "Copyright 2001 Argonne National Lab"
#define VERSION_RELEASE 1
#define VERSION_MAJOR   2
#define VERSION_MINOR   2

#define MAX_CMD_LENGTH	    1024
#define MAX_HOST_LENGTH	    64
#define MPD_PASSPHRASE_MAX_LENGTH    256
#define MPD_SALT_VALUE               "14"

#define MPD_DEFAULT_PORT             8675
#define MPD_DEFAULT_PASSPHRASE       "MPICHIsGreat"
#define MPD_REGISTRY_KEY             "SOFTWARE\\MPICH\\MPD"

#define DBS_SUCCESS_STR	    "DBS_SUCCESS"
#define DBS_FAIL_STR	    "DBS_FAIL"
#define DBS_END_STR	    "DBS_END"

#endif
