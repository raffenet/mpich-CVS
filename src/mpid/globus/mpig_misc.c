/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/*>>>>>>>>>>>>>>>>>>>>>>>>
  DEBUGGING OUTPUT SECTION
  >>>>>>>>>>>>>>>>>>>>>>>>*/
#define LOCAL_FMT_SZ 256

/*
 * Not all systems support va_copy().  These definitions were taken from mpich2/src/util/dbg/dbg_printf.c.  They rely on the
 * top-level configure script to test for va_copy() and __va_copy().
 */
#ifdef HAVE_VA_COPY
# define va_copy_end(a) va_end(a)
#else
# ifdef HAVE___VA_COPY
#  define va_copy(a,b) __va_copy(a,b)
#  define va_copy_end(a)
# else
#  define va_copy(a,b) ((a) = (b))
/* Some writers recommend define va_copy(a,b) memcpy(&a,&b,sizeof(va_list)) */
#  define va_copy_end(a)
# endif
#endif


/*
 * void mpig_dbg_printf([IN] level, [IN] func, [IN] fmt, [IN] ...)
 *
 * Paramters:
 *
 * level [IN] - debugging level (currently not used)
 * func [IN] - name of calling function
 * fmt [IN] - format string (see man page for printf)
 * ... [IN] - arguments matching substitutions in the format string
 *
 * Return: (nothing)
 */
#undef mpig_dbg_printf
#undef FUNCNAME
#define FUNCNAME mpig_dbg_printf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_dbg_printf(int level, const char * func, const char * fmt, ...)
{
    char lfmt[LOCAL_FMT_SZ];
    va_list list;

    va_start(list, fmt);
    MPIU_Snprintf(lfmt, LOCAL_FMT_SZ, "[%s:%d:%d] %s():%d: %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank,
		  0 /* MT: thread id */, func, level, fmt);
    MPIU_dbglog_vprintf(lfmt, list);
    fflush(stdout);
    va_end(list);
}
/* void mpig_dbg_printf([IN] level, [IN] func, [IN] fmt, [IN] ...) */

/*
 * void mpig_dbg_printf([IN] level, [IN] func, [IN], fmt, [IN] va)
 *
 * Paramters:
 *
 * level - [IN] debugging level (currently not used)
 * func - [IN] name of calling function
 * va - [IN] variable argument list containging parameter matching substitutions in the format string
 *
 * Return: (nothing)
 */
#undef mpig_dbg_vprintf
#undef FUNCNAME
#define FUNCNAME mpig_dbg_vprintf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
void mpig_dbg_vprintf(int level, const char * func, const char * fmt, va_list ap)
{
    char lfmt[LOCAL_FMT_SZ];
    va_list list;
    
    va_copy(list, ap);
    MPIU_Snprintf(lfmt, LOCAL_FMT_SZ, "[%s:%d:%d] %s(): %s\n", mpig_process.my_pg_id, mpig_process.my_pg_rank,
		  0 /* MT: thread id */, func, fmt);
    MPIU_dbglog_vprintf(lfmt, list);
    fflush(stdout);
    va_copy_end(list);
}
/* void mpig_dbg_printf([IN] level, [IN] func, [IN], fmt, [IN] va) */

/*<<<<<<<<<<<<<<<<<<<<<<<<
  DEBUGGING OUTPUT SECTION
  <<<<<<<<<<<<<<<<<<<<<<<<*/


#if defined(FOO)

/*
 * mpig_request_reload_sreq_iov()
 *
 * Fill the provided IOV with the next (or remaining) portion of data described by the segment contained in the request structure.
 * If the density of IOV is not sufficient, pack the data into a send/receive buffer and point the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME mpig_request_load_sreq_iov
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_request_load_sreq_iov(MPID_Request * const sreq, MPID_IOV * const iov, int * const iov_n)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_load_sreq_iov);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_load_sreq_iov);
    last = sreq->dev.segment_size;
    MPIG_DBG_PRINTF((40, FCNAME, "pre-pv: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    MPIU_Assert(sreq->dev.segment_first < last);
    MPIU_Assert(last > 0);
    MPIU_Assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    MPID_Segment_pack_vector(&sreq->dev.segment, sreq->dev.segment_first, &last, iov, iov_n);
    MPIG_DBG_PRINTF((40, FCNAME, "post-pv: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT ", iov_n=%d",
		      sreq->dev.segment_first, last, *iov_n));
    MPIU_Assert(*iov_n > 0 && *iov_n <= MPID_IOV_LIMIT);
    
    if (last == sreq->dev.segment_size)
    {
	MPIG_DBG_PRINTF((40, FCNAME, "remaining data loaded into sreq IOV"));
	sreq->dev.ca = MPIDI_CA_COMPLETE;
    }
    else if ((last - sreq->dev.segment_first) / *iov_n >= MPIDI_IOV_DENSITY_MIN)
    {
	MPIG_DBG_PRINTF((40, FCNAME, "more data loaded into sreq IOV"));
	sreq->dev.segment_first = last;
	sreq->dev.ca = MPIDI_CA_RELOAD_IOV;
    }
    else
    {
	MPI_Aint data_sz;
	
	MPIG_DBG_PRINTF((40, FCNAME, "sreq IOV density is low.  using datatbuf."));
	    
	data_sz = sreq->dev.segment_size - sreq->dev.segment_first;
	if (!mpig_request_get_srbuf_flag(sreq))
	{
	    mpig_request_databuf_alloc(sreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (sreq->dev.tmpbuf_sz == 0)
	    {
		MPIG_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		sreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}
		    
	last = (data_sz <= sreq->dev.tmpbuf_sz) ? sreq->dev.segment_size :
	    sreq->dev.segment_first + sreq->dev.tmpbuf_sz;
	MPIG_DBG_PRINTF((40, FCNAME, "pre-pack: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT,
			  sreq->dev.segment_first, last));
	MPID_Segment_pack(&sreq->dev.segment, sreq->dev.segment_first, &last, sreq->dev.tmpbuf);
	MPIG_DBG_PRINTF((40, FCNAME, "post-pack: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT,
			   sreq->dev.segment_first, last));
	iov[0].MPID_IOV_BUF = sreq->dev.tmpbuf;
	iov[0].MPID_IOV_LEN = last - sreq->dev.segment_first;
	*iov_n = 1;
	if (last == sreq->dev.segment_size)
	{
	    MPIG_DBG_PRINTF((40, FCNAME, "remaining data packed into SRBuf"));
	    sreq->dev.ca = MPIDI_CA_COMPLETE;
	}
	else 
	{
	    MPIG_DBG_PRINTF((40, FCNAME, "more data packed into SRBuf"));
	    sreq->dev.segment_first = last;
	    sreq->dev.ca = MPIDI_CA_RELOAD_IOV;
	}
    }
    
  fn_exit:
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_load_sreq_iov);
    return mpi_errno;
}
/* mpig_request_load_sreq_iov() */


/*
 * mpig_request_load_rreq_iov()
 *
 * Fill the request's IOV with the next (or remaining) portion of data described by the segment (also contained in the request
 * structure).  If the density of IOV is not sufficient, allocate a send/receive buffer and point the IOV at the buffer.
 */
#undef FUNCNAME
#define FUNCNAME mpig_request_load_rreq_iov
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_request_load_rreq_iov(MPID_Request * const rreq)
{
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_load_rreq_iov);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_load_rreq_iov);
    if (rreq->dev.segment_first < rreq->dev.segment_size)
    {
	/* still reading data that needs to go into the user buffer */
	
	if (mpig_request_get_srbuf_flag(rreq))
	{
	    MPI_Aint data_sz;
	    MPI_Aint tmpbuf_sz;

	    /* Once a SRBuf is in use, we continue to use it since a small amount of data may already be present at the beginning
	       of the buffer.  This data is left over from the previous unpack, most like a result of alignment issues.  NOTE: we
	       could force the use of the SRBuf only when (rreq->dev.tmpbuf_off > 0)... */
	    
	    data_sz = rreq->dev.segment_size - rreq->dev.segment_first - rreq->dev.tmpbuf_off;
	    MPIU_Assert(data_sz > 0);
	    tmpbuf_sz = rreq->dev.tmpbuf_sz - rreq->dev.tmpbuf_off;
	    if (data_sz > tmpbuf_sz)
	    {
		data_sz = tmpbuf_sz;
	    }
	    rreq->dev.iov[0].MPID_IOV_BUF = (char *) rreq->dev.tmpbuf + rreq->dev.tmpbuf_off;
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->dev.iov_count = 1;
	    MPIU_Assert(rreq->dev.segment_first + data_sz + rreq->dev.tmpbuf_off <= rreq->dev.recv_data_sz);
	    if (rreq->dev.segment_first + data_sz + rreq->dev.tmpbuf_off == rreq->dev.recv_data_sz)
	    {
		MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read the remaining data into the SRBuf"));
		rreq->dev.ca = MPIDI_CA_UNPACK_DATABUF_AND_COMPLETE;
	    }
	    else
	    {
		MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read more data into the SRBuf"));
		rreq->dev.ca = MPIDI_CA_UNPACK_DATABUF_AND_RELOAD_IOV;
	    }
	    goto fn_exit;
	}
	
	last = rreq->dev.segment_size;
	rreq->dev.iov_count = MPID_IOV_LIMIT;
	MPIG_DBG_PRINTF((40, FCNAME, "pre-upv: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT ", iov_n=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count));
	MPIU_Assert(rreq->dev.segment_first < last);
	MPIU_Assert(last > 0);
	MPID_Segment_unpack_vector(&rreq->dev.segment, rreq->dev.segment_first, &last, rreq->dev.iov, &rreq->dev.iov_count);
	MPIG_DBG_PRINTF((40, FCNAME, "post-upv: first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT ", iov_n=%d",
			  rreq->dev.segment_first, last, rreq->dev.iov_count));
	MPIU_Assert(rreq->dev.iov_count > 0 && rreq->dev.iov_count <= MPID_IOV_LIMIT);

	/* --BEGIN ERROR HANDLING-- */
	if (rreq->dev.iov_count == 0)
	{
	    /* If the data can't be unpacked, the we have a mis-match between the datatype and the amount of data received.  Adjust
	       the segment info so that the remaining data is received and thrown away. */
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							  "**dtypemismatch", 0);
	    rreq->status.count = (int)rreq->dev.segment_first;
	    rreq->dev.segment_size = rreq->dev.segment_first;
	    mpi_errno = mpig_request_load_rreq_iov(rreq);
	    goto fn_exit;
	}
	/* --END ERROR HANDLING-- */

	if (last == rreq->dev.recv_data_sz)
	{
	    MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read the remaining data directly into the user buffer"));
	    rreq->dev.ca = MPIDI_CA_COMPLETE;
	}
	else if (last == rreq->dev.segment_size || (last - rreq->dev.segment_first) / rreq->dev.iov_count >= MPIDI_IOV_DENSITY_MIN)
	{
	    MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read more data directly into the user buffer"));
	    rreq->dev.segment_first = last;
	    rreq->dev.ca = MPIDI_CA_RELOAD_IOV;
	}
	else
	{
	    /* Too little data would have been received using an IOV.  We will start receiving data into a SRBuf and unpacking it
	       later. */
	    MPIU_Assert(mpig_request_get_srbuf_flag(rreq) == FALSE);
	    
	    mpig_request_databuf_alloc(rreq, rreq->dev.segment_size - rreq->dev.segment_first);
	    rreq->dev.tmpbuf_off = 0;
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		/* FIXME - we should drain the data off the pipe here, but we don't have a buffer to drain it into.  should this be
		   a fatal error? */
		MPIG_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    /* fill in the IOV using a recursive call */
	    mpi_errno = mpig_request_load_rreq_iov(rreq);
	}
    }
    else
    {
	/* receive and toss any extra data that does not fit in the user's buffer */
	MPI_Aint data_sz;

	data_sz = rreq->dev.recv_data_sz - rreq->dev.segment_first;
	if (!mpig_request_get_srbuf_flag(rreq))
	{
	    MPIDI_Databuf_alloc(rreq, data_sz);
	    /* --BEGIN ERROR HANDLING-- */
	    if (rreq->dev.tmpbuf_sz == 0)
	    {
		MPIG_DBG_PRINTF((40, FCNAME, "SRBuf allocation failure"));
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		rreq->status.MPI_ERROR = mpi_errno;
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */
	}

	if (data_sz <= rreq->dev.tmpbuf_sz)
	{
	    MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow data into the SRBuf and complete"));
	    rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
	    rreq->dev.ca = MPIDI_CA_COMPLETE;
	}
	else
	{
	    MPIG_DBG_PRINTF((35, FCNAME, "updating rreq to read overflow data into the SRBuf and reload IOV"));
	    rreq->dev.iov[0].MPID_IOV_LEN = rreq->dev.tmpbuf_sz;
	    rreq->dev.segment_first += rreq->dev.tmpbuf_sz;
	    rreq->dev.ca = MPIDI_CA_RELOAD_IOV;
	}
	
	rreq->dev.iov[0].MPID_IOV_BUF = rreq->dev.tmpbuf;
	rreq->dev.iov_count = 1;
    }
    
  fn_exit:
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_load_rreq_iov);
    return mpi_errno;
}


/*
 * mpig_request_unpack_srbuf
 *
 * Unpack data from a send/receive buffer into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME mpig_request_unpack_srbuf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_request_unpack_srbuf(MPID_Request * rreq)
{
    MPI_Aint last;
    int tmpbuf_last;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_unpack_srbuf);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_unpack_srbuf);

    tmpbuf_last = (int)(rreq->dev.segment_first + rreq->dev.tmpbuf_sz);
    if (rreq->dev.segment_size < tmpbuf_last)
    {
	tmpbuf_last = (int)rreq->dev.segment_size;
    }
    last = tmpbuf_last;
    MPID_Segment_unpack(&rreq->dev.segment, rreq->dev.segment_first, &last, rreq->dev.tmpbuf);
    if (last == 0 || last == rreq->dev.segment_first)
    {
	/* --BEGIN ERROR HANDLING-- */
	/* If no data can be unpacked, then we have a datatype processing problem.  Adjust the segment info so that the remaining
	   data is received and thrown away. */
	rreq->status.count = (int)rreq->dev.segment_first;
	rreq->dev.segment_size = rreq->dev.segment_first;
	rreq->dev.segment_first += tmpbuf_last;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
						      "**dtypemismatch", 0);
	/* --END ERROR HANDLING-- */
    }
    else if (tmpbuf_last == rreq->dev.segment_size)
    {
	/* --BEGIN ERROR HANDLING-- */
	if (last != tmpbuf_last)
	{
	    /* received data was not entirely consumed by unpack() because too few bytes remained to fill the next basic datatype.
	       Note: the segment_first field is set to segment_last so that if this is a truncated message, extra data will be read
	       off the pipe. */
	    rreq->status.count = (int)last;
	    rreq->dev.segment_size = last;
	    rreq->dev.segment_first = tmpbuf_last;
	    rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							  "**dtypemismatch", 0);
	}
	/* --END ERROR HANDLING-- */
    }
    else
    {
	rreq->dev.tmpbuf_off = (int)(tmpbuf_last - last);
	if (rreq->dev.tmpbuf_off > 0)
	{
	    /* move any remaining data to the beginning of the buffer.  Note: memmove() is used since the data regions could
               overlap. */
	    memmove(rreq->dev.tmpbuf, (char *) rreq->dev.tmpbuf + (last - rreq->dev.segment_first), rreq->dev.tmpbuf_off);
	}
	rreq->dev.segment_first = last;
    }

    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_unpack_srbuf);
    return mpi_errno;
}

/*
 * mpig_request_unpack_uebuf
 *
 * Copy/unpack data from an "unexpected eager buffer" into the user buffer.
 */
#undef FUNCNAME
#define FUNCNAME mpig_request_unpack_uebuf
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
int mpig_request_unpack_uebuf(MPID_Request * rreq)
{
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPI_Aint userbuf_sz;
    MPID_Datatype * dt_ptr;
    MPI_Aint unpack_sz;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_unpack_uebuf);
    MPIG_STATE_DECL(MPID_STATE_MEMCPY);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_unpack_uebuf);

    mpig_datatype_get_info(rreq->dev.user_count, rreq->dev.datatype, dt_contig, userbuf_sz, dt_ptr, dt_true_lb);
    
    if (rreq->dev.recv_data_sz <= userbuf_sz)
    {
	unpack_sz = rreq->dev.recv_data_sz;
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
	MPIG_DBG_PRINTF((40, FCNAME, "receive buffer overflow; message truncated, msg_sz=" MPIG_AINT_FMT ", buf_sz="
			  MPIG_AINT_FMT, rreq->dev.recv_data_sz, userbuf_sz));
	unpack_sz = userbuf_sz;
	rreq->status.count = (int)userbuf_sz;
	rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TRUNCATE,
						      "**truncate", "**truncate %d %d", rreq->dev.recv_data_sz, userbuf_sz);
	/* --END ERROR HANDLING-- */
    }

    if (unpack_sz > 0)
    {
	if (dt_contig)
	{
	    /* TODO - check that amount of data is consistent with datatype.  In other words, if we were to use Segment_unpack()
	       would last = unpack?  If not we should return an error (unless configured with --enable-fast) */
	    MPIG_FUNC_ENTER(MPID_STATE_MEMCPY);
	    memcpy((char *)rreq->dev.user_buf + dt_true_lb, rreq->dev.tmpbuf, unpack_sz);
	    MPIG_FUNC_EXIT(MPID_STATE_MEMCPY);
	}
	else
	{
	    MPID_Segment seg;
	    MPI_Aint last;

	    MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count, rreq->dev.datatype, &seg, 0);
	    last = unpack_sz;
	    MPID_Segment_unpack(&seg, 0, &last, rreq->dev.tmpbuf);
	    if (last != unpack_sz)
	    {
		/* --BEGIN ERROR HANDLING-- */
		/* received data was not entirely consumed by unpack() because too few bytes remained to fill the next basic
		   datatype */
		rreq->status.count = (int)last;
		rreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_TYPE,
							      "**dtypemismatch", 0);
		/* --END ERROR HANDLING-- */
	    }
	}
    }

    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_unpack_uebuf);
    return mpi_errno;
}
/* MPIDI_Rreq_unpack_uebuf() */

#endif /*defined(FOO) */
