/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MM_VIA_PRE_H
#define MM_VIA_PRE_H

#include "vipl.h"
#include "mpiimpl.h"
#include "mpiimplthread.h"

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

typedef struct MM_Car_data_via
{
    union 
    {
	struct car_via_tmp
	{
	    int num_read;
	} tmp;
	struct car_via_vec
	{
	    MPID_VECTOR vec[MPID_VECTOR_LIMIT];
	    int len;
	} vec;
#ifdef WITH_METHOD_SHM
	struct car_via_shm
	{
	    int num_read;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct car_via_via
	{
	    int num_read;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct car_via_via_rdma
	{
	    int num_read;
	} via_rdma;
#endif
    } buf;
} MM_Car_data_via;

#endif
