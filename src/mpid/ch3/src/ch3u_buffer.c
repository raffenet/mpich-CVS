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

    if (HANDLE_GET_KIND(sdt) == HANDLE_KIND_BUILTIN)
    {
	sdt_contig = TRUE;
	sdata_sz = scount * MPID_Datatype_get_basic_size(sdt);
	MPIDI_DBG_PRINTF((15, FCNAME, "send basic dt: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, sdt_contig,
			  MPID_Datatype_get_basic_size(sdt), sdata_sz));
    }
    else
    {
	MPID_Datatype * sdtp;
	
	MPID_Datatype_get_ptr(sdt, sdtp);
	sdt_contig = sdtp->is_contig;
	sdata_sz = scount * sdtp->size;
	MPIDI_DBG_PRINTF((15, FCNAME, "send user dt: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, sdt_contig,
			  sdtp->size, sdata_sz));
    }

    if (HANDLE_GET_KIND(rdt) == HANDLE_KIND_BUILTIN)
    {
	rdt_contig = TRUE;
	rdata_sz = rcount * MPID_Datatype_get_basic_size(rdt);
	MPIDI_DBG_PRINTF((15, FCNAME, "recv basic dt: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, rdt_contig,
			  MPID_Datatype_get_basic_size(rdt), rdata_sz));
    }
    else
    {
	MPID_Datatype * rdtp;
	
	MPID_Datatype_get_ptr(rdt, rdtp);
	rdt_contig = rdtp->is_contig;
	rdata_sz = rcount * rdtp->size;
	MPIDI_DBG_PRINTF((15, FCNAME, "recv user dt: dt_contig=%d, "
			  "dt_sz=%d, data_sz=" MPIDI_MSG_SZ_FMT, rdt_contig,
			  rdtp->size, rdata_sz));
    }

    if (sdt_contig && rdt_contig)
    {
	MPIDI_msg_sz_t data_sz;
	
	if (sdata_sz <= rdata_sz)
	{
	    data_sz = sdata_sz;
	}
	else
	{
	    MPIDI_DBG_PRINTF((15, FCNAME, "message truncated, sdata_sz="
			      MPIDI_MSG_SZ_FMT " rdata_sz=", MPIDI_MSG_SZ_FMT,
			      sdata_sz, rdata_sz));
	    data_sz = rdata_sz;
	    *rmpi_errno = MPI_ERR_TRUNCATE;
	}

	memcpy(rbuf, sbuf, data_sz);
	*rsz = data_sz;
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
