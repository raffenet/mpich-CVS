/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Buffer_copy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDI_CH3U_Buffer_copy(
    const void * const sbuf, int scount, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcount, MPI_Datatype rdt, MPIDI_msg_sz_t * rsz,
    int * rmpi_errno)
{
    int sdt_contig;
    int rdt_contig;
    MPIDI_msg_sz_t sdata_sz;
    MPIDI_msg_sz_t rdata_sz;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    MPIDI_CH3U_Datatype_get_info(scount, sdt, sdt_contig, sdata_sz);
    MPIDI_CH3U_Datatype_get_info(rcount, rdt, rdt_contig, rdata_sz);

    if (sdata_sz > rdata_sz)
    {
	MPIDI_DBG_PRINTF((15, FCNAME, "message truncated, sdata_sz=" MPIDI_MSG_SZ_FMT " rdata_sz=", MPIDI_MSG_SZ_FMT,
			  sdata_sz, rdata_sz));
	sdata_sz = rdata_sz;
	*rmpi_errno = MPI_ERR_TRUNCATE;
    }
    
    if (sdt_contig && rdt_contig)
    {
	memcpy(rbuf, sbuf, sdata_sz);
	*rsz = sdata_sz;
    }
    else if (sdt_contig)
    {
	abort();
    }
    else if (rdt_contig)
    {
	abort();
    }
    else
    {
	abort();
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_BUFFER_COPY);
}
