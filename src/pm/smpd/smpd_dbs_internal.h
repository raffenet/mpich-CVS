/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "smpd.h"

typedef struct DatabaseElementStruct
{
    char pszKey[SMPD_MAX_DBS_KEY_LEN];
    char pszValue[SMPD_MAX_DBS_VALUE_LEN];
    struct DatabaseElementStruct *pNext;
} DatabaseElement;

typedef struct DatabaseNodeStruct
{
    char pszName[SMPD_MAX_DBS_NAME_LEN];
    DatabaseElement *pData, *pIter;
    struct DatabaseNodeStruct *pNext;
} DatabaseNode;
