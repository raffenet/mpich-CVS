/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_VIA_PRE_H
#define MM_VIA_PRE_H

#include "vipl.h"
#include "mpiimpl.h"

typedef struct VI_Info
{
    long valid;
    MPID_Thread_lock_t lock;
    VIP_NIC_HANDLE      hNic;
    VIP_VI_HANDLE       hVi;
    VIP_VI_ATTRIBUTES   Vi_RemoteAttribs;
    VIP_DESCRIPTOR      *pRecvDesc, **pSendDesc, *pDesc;
    VIP_MEM_HANDLE      mhSend, mhReceive;
    void *pSendDescriptorBuffer, *pReceiveDescriptorBuffer;
    
    char remotebuf[40];
    VIP_NET_ADDRESS *pLocalAddress;
    VIP_NET_ADDRESS *pRemoteAddress;
    unsigned char *descriminator;
    int descriminator_len;

    VIP_DESCRIPTOR *pRecvList, *pRecvListTail;
    int nCurSendIndex;
    int nNumSendsAvailable;
    int nNumSendDescriptors;
    int nNumRecvDescriptors;
    int nReceivesPerAck;
    int nSendsPerAck;
    long nSendAcked;
    unsigned int nNumSent, nNumReceived, nSequenceNumberSend, nSequenceNumberReceive;
} VI_Info;

#endif
