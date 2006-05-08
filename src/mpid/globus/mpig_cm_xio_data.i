/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/**********************************************************************************************************************************
					    BEGIN HEADER PACKING AND UNPACKING MACROS
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

#define mpig_cm_xio_msg_hdr_put_init(msgbuf_)					\
{										\
    mpig_databuf_reset(msgbuf_);						\
    mpig_databuf_set_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_msg_size);	\
}

#define mpig_cm_xio_msg_hdr_sizeof_msg_size (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_msg_size(msgbuf_)							\
{													\
    *(unsigned char *)(mpig_databuf_get_ptr(msgbuf_)) = (unsigned char) mpig_databuf_get_eod(msgbuf_);	\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", msg_size=%u",	\
	(MPIG_PTR_CAST) (msgbuf_), (unsigned) mpig_databuf_get_eod(msgbuf_)));				\
}

#define mpig_cm_xio_msg_hdr_get_msg_size(msgbuf_, msg_size_p_)						\
{													\
    *(msg_size_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", msg_size=%u",	\
	(MPIG_PTR_CAST) (msgbuf_),  (unsigned) *(msg_size_p_)));					\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_msg_size);				\
}

#define mpig_cm_xio_msg_hdr_sizeof_msg_type (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_msg_type(msgbuf_, msg_type_)						\
{													\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) (msg_type_);		\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", msg_type=%s",	\
	(MPIG_PTR_CAST) (msgbuf_), mpig_cm_xio_msg_type_get_string(msg_type_)));			\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_msg_type);				\
}

#define mpig_cm_xio_msg_hdr_get_msg_type(msgbuf_, msg_type_p_)						\
{													\
    *(msg_type_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", msg_type=%s",	\
	(MPIG_PTR_CAST) (msgbuf_), mpig_cm_xio_msg_type_get_string(*(msg_type_p_))));			\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_msg_type);				\
}

#define mpig_cm_xio_msg_hdr_sizeof_endian (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_endian(msgbuf_, endian_)						\
{													\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) (endian_);			\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", endian=%s",		\
	(MPIG_PTR_CAST) (msgbuf_), ((endian_) == MPIG_ENDIAN_LITTLE) ? "little" : "big"));		\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_endian);					\
}

#define mpig_cm_xio_msg_hdr_get_endian(msgbuf_, endian_p_)							\
{														\
    *(endian_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", endian=%s",			\
	(MPIG_PTR_CAST) (msgbuf_), (*(endian_p_) == MPIG_ENDIAN_LITTLE) ? "little" : "big"));			\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_endian);						\
}

#define mpig_cm_xio_msg_hdr_sizeof_df (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_df(msgbuf_, df_)							\
{													\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) (df_);			\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", data_format=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), (df_)));								\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_df);					\
}

#define mpig_cm_xio_msg_hdr_get_df(msgbuf_, df_p_)							\
{													\
    *(df_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", data_format=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), *(df_p_)));								\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_df);					\
}

#define mpig_cm_xio_msg_hdr_sizeof_req_type (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_req_type(msgbuf_, req_type_)						\
{													\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) (req_type_);		\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", req_type=%s",	\
	(MPIG_PTR_CAST) (msgbuf_), mpig_request_type_get_string(req_type_)));				\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_req_type);				\
}

#define mpig_cm_xio_msg_hdr_get_req_type(endian_, msgbuf_, req_type_p_)					\
{													\
    *(req_type_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", req_type=%s",	\
	(MPIG_PTR_CAST) (msgbuf_), mpig_request_type_get_string(*(req_type_p_))));			\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_req_type);				\
}

#define mpig_cm_xio_msg_hdr_sizeof_envelope (3 * sizeof(int32_t))

#define mpig_cm_xio_msg_hdr_put_envelope(msgbuf_, rank_, tag_, ctx_)						\
{														\
    int32_t rank__ = (rank_);											\
    int32_t tag__ = (tag_);											\
    int32_t ctx__ = (ctx_);											\
    char * ptr__ = (char *) mpig_databuf_get_eod_ptr(msgbuf_);							\
    mpig_dc_put_int32(ptr__, (rank__));										\
    ptr__ += sizeof(int32_t);											\
    mpig_dc_put_int32(ptr__, (tag__));										\
    ptr__ += sizeof(int32_t);											\
    mpig_dc_put_int32(ptr__, (ctx__));										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", rank=%d, tag=%d ctx=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), (rank_), (tag_), (ctx_)));							\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_envelope);					\
}

#define mpig_cm_xio_msg_hdr_get_envelope(endian_, msgbuf_, rank_p_, tag_p_, ctx_p_)				\
{														\
    int32_t rank__;												\
    int32_t tag__;												\
    int32_t ctx__;												\
    char * ptr__ = (char *) mpig_databuf_get_pos_ptr(msgbuf_);							\
    mpig_dc_get_int32((endian_), ptr__, &rank__);								\
    ptr__ += sizeof(int32_t);											\
    mpig_dc_get_int32((endian_), ptr__, &tag__);								\
    ptr__ += sizeof(int32_t);											\
    mpig_dc_get_int32((endian_), ptr__, &ctx__);								\
    *(rank_p_) = rank__;											\
    *(tag_p_) = tag__;												\
    *(ctx_p_) = ctx__;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", rank=%d, tag=%d ctx=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), *(rank_p_), *(tag_p_), *(ctx_p_)));						\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_envelope);					\
}

#define mpig_cm_xio_msg_hdr_sizeof_rank (sizeof(int32_t))

#define mpig_cm_xio_msg_hdr_put_rank(msgbuf_, rank_)						\
{												\
    int32_t rank__ = (rank_);									\
    mpig_dc_put_int32(mpig_databuf_get_eod_ptr(msgbuf_), (rank__));				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", rank=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), (rank_)));							\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_rank);				\
}

#define mpig_cm_xio_msg_hdr_get_rank(endian_, msgbuf_, rank_p_)					\
{												\
    int32_t rank__;										\
    mpig_dc_get_int32((endian_), mpig_databuf_get_pos_ptr(msgbuf_), &rank__);			\
    *(rank_p_) = rank__;									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", rank=%d",	\
	(MPIG_PTR_CAST) (msgbuf_), *(rank_p_)));						\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_rank);				\
}

#define mpig_cm_xio_msg_hdr_sizeof_data_size (sizeof(u_int64_t))

#define mpig_cm_xio_msg_hdr_put_data_size(msgbuf_, data_size_);							\
{														\
    u_int64_t data_size__ = (u_int64_t) (data_size_);								\
    mpig_dc_put_uint64(mpig_databuf_get_eod_ptr(msgbuf_), data_size__);						\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", data_size=" MPIG_SIZE_FMT,	\
	(MPIG_PTR_CAST) (msgbuf_), (MPIU_Size_t) (data_size_)));						\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_data_size);					\
}

#define mpig_cm_xio_msg_hdr_get_data_size(endian_, msgbuf_, data_size_p_);					\
{														\
    u_int64_t data_size__;											\
    mpig_dc_get_uint64((endian_), mpig_databuf_get_pos_ptr(msgbuf_), &data_size__);				\
    *(data_size_p_) = data_size__;										\
    MPIU_Assertp(data_size__ == *(data_size_p_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", data_size=" MPIG_SIZE_FMT,	\
	(MPIG_PTR_CAST) (msgbuf_), (MPIU_Size_t) *(data_size_p_)));						\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_data_size);					\
}

#define mpig_cm_xio_msg_hdr_sizeof_req_id (sizeof(int32_t))

#define mpig_cm_xio_msg_hdr_put_req_id(msgbuf_, req_id_)							\
{														\
    int32_t req_id__ = (req_id_);										\
    mpig_dc_put_int32(mpig_databuf_get_eod_ptr(msgbuf_), (req_id__));						\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", req_id=" MPIG_HANDLE_FMT,	\
	(MPIG_PTR_CAST) (msgbuf_), (unsigned)(req_id_)));							\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_req_id);						\
}

#define mpig_cm_xio_msg_hdr_get_req_id(endian_, msgbuf_, req_id_p_)						\
{														\
    int32_t req_id__;												\
    mpig_dc_get_int32((endian_), mpig_databuf_get_pos_ptr(msgbuf_), &req_id__);					\
    *(req_id_p_) = req_id__;											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", req_id=" MPIG_HANDLE_FMT,	\
	(MPIG_PTR_CAST) (msgbuf_), (unsigned) *(req_id_p_)));							\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_req_id);						\
}

#define mpig_cm_xio_msg_hdr_sizeof_bool (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_bool(msgbuf_, bool_)							\
{													\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) ((bool_) ? 0xff : 0x0);	\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", bool=%s",		\
	(MPIG_PTR_CAST) (msgbuf_), MPIG_BOOL_STR(bool_)));						\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_bool);					\
}

#define mpig_cm_xio_msg_hdr_get_bool(endian_, msgbuf_, bool_p_)					\
{												\
    *(bool_p_) = (*(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_)) ? TRUE : FALSE;		\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", bool=%s",	\
	(MPIG_PTR_CAST) (msgbuf_), MPIG_BOOL_STR(*(bool_p_))));					\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_msg_type);			\
}

#define mpig_cm_xio_msg_hdr_sizeof_conn_open_resp (sizeof(unsigned char))

#define mpig_cm_xio_msg_hdr_put_conn_open_resp(msgbuf_, conn_open_resp_)					\
{														\
    *(unsigned char *)(mpig_databuf_get_eod_ptr(msgbuf_)) = (unsigned char) (conn_open_resp_);			\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr put: msgbuf=" MPIG_PTR_FMT ", conn_open_resp=%s",		\
	(MPIG_PTR_CAST) (msgbuf_), mpig_cm_xio_conn_open_resp_get_string(conn_open_resp_)));			\
    mpig_databuf_inc_eod((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_conn_open_resp);					\
}

#define mpig_cm_xio_msg_hdr_get_conn_open_resp(msgbuf_, conn_open_resp_p_)					\
{														\
    *(conn_open_resp_p_) = *(unsigned char *) mpig_databuf_get_pos_ptr(msgbuf_);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_MSGHDR, "hdr get: msgbuf=" MPIG_PTR_FMT ", conn_open_resp=%s",		\
	(MPIG_PTR_CAST) (msgbuf_), mpig_cm_xio_conn_open_resp_get_string(*(conn_open_resp_p_))));		\
    mpig_databuf_inc_pos((msgbuf_), mpig_cm_xio_msg_hdr_sizeof_conn_open_resp);					\
}

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					     END HEADER PACKING AND UNPACKING MACROS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					      BEGIN STREAM DATA CONVERSION ROUTINES
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

MPIG_STATIC void mpig_cm_xio_stream_sreq_init(
    MPID_Request * sreq, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_stream_sreq_pack(
    MPID_Request * sreq, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_stream_rreq_init(
    MPID_Request * rreq, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_stream_rreq_unpack(
    MPID_Request * rreq, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_stream_rreq_unpack_vc_msgbuf(
    mpig_vc_t * vc, MPID_Request * rreq, int * mpi_errno_p, bool_t * failed_p);

MPIG_STATIC void mpig_cm_xio_stream_rreq_handle_truncation(
    MPID_Request * rreq, MPIU_Size_t buffered_bytes, int * mpi_errno_p, bool_t * failed_p);


#define mpig_cm_xio_stream_set_size(req_, stream_size_)	\
{							\
    (req_)->cm.xio.stream_size = (stream_size_);	\
}

#define mpig_cm_xio_stream_get_size(req_) ((req_)->cm.xio.stream_size)

#define mpig_cm_xio_stream_set_cur_pos(req_, pos_)	\
{							\
    (req_)->cm.xio.stream_pos = (pos_);			\
}							\

#define mpig_cm_xio_stream_inc_cur_pos(req_, val_)	\
{							\
    (req_)->cm.xio.stream_pos = (val_);			\
}							\

#define mpig_cm_xio_stream_get_cur_pos(req_) ((req_)->cm.xio.stream_pos)

#define mpig_cm_xio_stream_set_max_pos(req_, max_pos_)										\
{																\
    (req_)->cm.xio.stream_max_pos = ((max_pos_) == 0 || (max_pos_) > (req_)->cm.xio.stream_size) ?				\
	(req_)->cm.xio.stream_size : (max_pos_);										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "set stream max_pos: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT		\
		       ", max_pos=" MPIG_SIZE_FMT, (req_)->handle, (MPIG_PTR_CAST) (req_), (req_)->cm.xio.stream_max_pos));	\
}

#define mpig_cm_xio_stream_get_max_pos(req_) ((req_)->cm.xio.stream_max_pos)

#define mpig_cm_xio_stream_is_complete(req_) ((req_)->cm.xio.stream_pos == (req_)->cm.xio.stream_size)


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * void mpig_cm_xio_stream_sreq_init([IN/MOD] sreq, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * initialize the send request's stream management fields in preparation for packing data via mpig_cm_xio_stream_sreq_pack().  if
 * the user buffer, as described by the (count, datatype) tuple, is too sparse to efficiently use an IOV, the stream will be
 * setup to use an intermediate buffer.
 *
 * sreq [IN/MOD] - send request
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * NOTE: this routine assumes that the request fields associate with the user buffer are set (use mpig_request_set_buffer()).
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_sreq_init
MPIG_STATIC void mpig_cm_xio_stream_sreq_init(
    MPID_Request * const sreq, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * sreq_cm = &sreq->cm.xio;
    const void * buf;
    int cnt;
    MPI_Datatype dt;
    bool_t dt_contig;
    MPIU_Size_t dt_size;
    MPIU_Size_t dt_nblks;
    MPI_Aint dt_true_lb;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_sreq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_sreq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, *mpi_errno_p));
    *failed_p = FALSE;
    
    /* get information about the user buffer.  store details in the request for use by other routines. */
    mpig_request_get_buffer(sreq, &buf, &cnt, &dt);
    mpig_datatype_get_info(dt, &dt_contig, &dt_size, &dt_nblks, &dt_true_lb);

    sreq_cm->buf_size = cnt * dt_size;
    sreq_cm->buf_true_lb = dt_true_lb;
    
    sreq_cm->stream_size = cnt * dt_size;
    sreq_cm->stream_pos = 0;
    sreq_cm->stream_max_pos = sreq_cm->stream_size;
    
    if (dt_contig == TRUE || cnt == 0)
    {
	/* if the user buffer is contiguous, then send data straight from the user buffer.  use of a segment is not required. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "contig user buffer; loading IOV: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT
			   ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq,
			   sreq_cm->buf_size, sreq_cm->stream_size));

	sreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_CONTIG;
    }
    else if (cnt * dt_nblks <= (MPIU_Size_t) mpig_iov_get_num_free_entries(sreq_cm->iov) ||
	     dt_size / dt_nblks >= MPIG_CM_XIO_DATA_DENSITY_THRESHOLD / (MPIU_Size_t) mpig_iov_get_num_entries(sreq_cm->iov))
    {
	/* if the user buffer is noncontiguous but the data is dense enough that an IOV can being used efficiently, then
	   populate the IOV with the first set of data to be sent */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "dense user buffer; initializing segment: sreq=" MPIG_HANDLE_FMT
			   ", sreqp=" MPIG_PTR_FMT ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			   sreq->handle, (MPIG_PTR_CAST) sreq, sreq_cm->buf_size, sreq_cm->stream_size));
	
	MPID_Segment_init(buf, cnt, dt, &sreq_cm->seg, 0); /* always packs using local data format */

	sreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_DENSE;
    }
    else
    {
	/* if the user buffer is noncontiguous and the data density is rather sparse, then an intermediate buffer is used
	   to first condense the data, allowing more data to be sent in a single write operation */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; initializing segment and allocating bufffer: sreq="
			   MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			   sreq->handle, (MPIG_PTR_CAST) sreq, sreq_cm->buf_size, sreq_cm->stream_size));
	
	MPID_Segment_init(buf, cnt, dt, &sreq_cm->seg, 0); /* always packs using local data format */
	
	mpig_databuf_create((MPIU_Size_t) MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE, &sreq_cm->databuf, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s",
			     "intermediate pack buffer");

	sreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_SPARSE;
    }
	
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x"
		       ", failed=%s", sreq->handle, (MPIG_PTR_CAST) sreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_sreq_init);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_stream_sreq_init() */


/*
 * void mpig_cm_xio_stream_sreq_pack([IN/MOD] sreq, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * populate the send request's IOV with the location(s) of the next set of data to be written.  as noted in th previous routine,
 * if the user buffer is too sparse to efficiently use an IOV, then the data is packed into an intermediate buffer and the IOV is
 * pointed at that buffer.
 *
 * sreq [IN/MOD] - send request
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * NOTE: entries are appended to the IOV, so the request's IOV should be reset (possibly adding or reserving an entry for a
 * message header) prior to calling this routine in order to make room for the new entries.  if the IOV contains no free entries,
 * this routine will fail.
 *
 * NOTE: this routine assumes that all data described in the IOV as loaded by a previous stream routine has been sent prior to
 * calling this routine.  while the IOV routines can handle appending new entries, the logic that controls the stream position is
 * not designed to reload a partially sent IOV.  there is no clear evidence that such an operation would ever be needed for
 * packing data, and it would significant complicate the code.  (see the stream_unpack code if you don't believe me.)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_sreq_pack
MPIG_STATIC void mpig_cm_xio_stream_sreq_pack(
    MPID_Request * const sreq, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * sreq_cm = &sreq->cm.xio;
    MPIU_Size_t last;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_sreq_pack);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_sreq_pack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       sreq->handle, (MPIG_PTR_CAST) sreq, *mpi_errno_p));
    *failed_p = FALSE;

    MPIU_Assert(sreq_cm->stream_pos <= sreq_cm->stream_max_pos);

    if (sreq_cm->stream_pos == sreq_cm->stream_size)
    {
	/* if all of the data has been packed, then free any databuf associated with the request and return */
	if (sreq_cm->databuf != NULL)
	{
	    mpig_databuf_destroy(sreq_cm->databuf);
	    sreq_cm->databuf = NULL;
	}

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "pack data complete: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT
			   ", nbytes_sent=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq, sreq_cm->stream_pos));
    }
    else if (sreq_cm->stream_pos == sreq_cm->stream_max_pos)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "stream stalled; pos = max_pos; skipping IOV reload: sreq=" MPIG_HANDLE_FMT
			   ", sreqp=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			   sreq->handle, (MPIG_PTR_CAST) sreq, sreq_cm->stream_pos, sreq_cm->stream_max_pos,
			   sreq_cm->stream_size));
    }
    else if (sreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_CONTIG)
    {
	/* if the user buffer is contiguous, continue to send the data straight from the user buffer */
	last = sreq_cm->stream_max_pos;
	mpig_iov_add_entry(sreq_cm->iov, (char *) sreq->dev.buf + sreq_cm->buf_true_lb + sreq_cm->stream_pos,
			   sreq_cm->stream_max_pos - sreq_cm->stream_pos);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "contig user buffer; IOV loaded: sreq=" MPIG_HANDLE_FMT", sreqp=" MPIG_PTR_FMT
			   ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, sreq->handle,
			   (MPIG_PTR_CAST) sreq, sreq_cm->stream_pos, last, sreq_cm->stream_size));
	
	sreq_cm->stream_pos = last;
    }
    else if (sreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_DENSE)
    {
	/* if the user buffer is noncontiguous but the data is dense enough that an IOV is being used, then populate the IOV with
	   the next set of data to be sent */
	MPID_IOV * iov;
	int iov_count;
	
	iov = mpig_iov_get_next_free_entry_ptr(sreq_cm->iov);
	iov_count = mpig_iov_get_num_free_entries(sreq_cm->iov);
	MPIU_Assert(iov_count > 0);

	last = sreq_cm->stream_max_pos;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA,
	    "dense user buffer; reloading IOV: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT
	    ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT ", iov_count=%d", sreq->handle, (MPIG_PTR_CAST) sreq,
	    sreq_cm->stream_pos, last, sreq_cm->stream_size, iov_count));
	
	MPID_Segment_pack_vector(&sreq_cm->seg, (MPI_Aint) sreq_cm->stream_pos, (MPI_Aint *) &last, iov, &iov_count);
	if (last == sreq_cm->stream_pos)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
		"MPID_Segment_pack_vector() failed to advance the stream position: sreq=" MPIG_HANDLE_FMT ", sreq_p=" MPIG_PTR_FMT
		", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", iov_count=%d", sreq->handle,
		(MPIG_PTR_CAST) sreq, sreq_cm->stream_pos, sreq_cm->stream_max_pos, sreq_cm->stream_size, iov_count));
	    MPIU_ERR_SETFATALANDSTMT(*mpi_errno_p, MPI_ERR_INTERN, {goto fn_fail;}, "**globus|cm_xio|seg_pack_iov");
	}   /* --END ERROR HANDLING-- */
	
	mpig_iov_inc_num_inuse_entries(sreq_cm->iov, iov_count);
	mpig_iov_inc_num_bytes(sreq_cm->iov, last - sreq_cm->stream_pos);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA,
	    "dense user buffer; IOV loaded: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT
	    ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq, sreq_cm->stream_pos,
	    last, sreq_cm->stream_size));
	
	sreq_cm->stream_pos = last;
    }
    else /* if (sreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_SPARSE) */
    {
	/* otherwise, the data is sparse and must be packed into an intermediate buffer */
	MPIU_Assert(sreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_SPARSE);
	
	last = sreq_cm->stream_pos + mpig_databuf_get_size(sreq_cm->databuf);
	if (last > sreq_cm->stream_max_pos)
	{
	    last = sreq_cm->stream_max_pos;
	}
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA,
	    "sparse user buffer; packing intermediate buffer and loading IOV: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT
	    ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq,
	    sreq_cm->stream_pos, last, sreq_cm->stream_size));
	
	MPID_Segment_pack(&sreq_cm->seg, (MPI_Aint) sreq_cm->stream_pos, (MPI_Aint *) &last,
			  mpig_databuf_get_ptr(sreq_cm->databuf));
	if (last == sreq_cm->stream_pos)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR,
		"MPID_Segment_pack() failed to advance the stream position: sreq=" MPIG_HANDLE_FMT ", sreq_p=" MPIG_PTR_FMT
		", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq,
		sreq_cm->stream_pos, sreq_cm->stream_max_pos, sreq_cm->stream_size));
	    MPIU_ERR_SETFATALANDSTMT(*mpi_errno_p, MPI_ERR_INTERN, {goto fn_fail;}, "**globus|cm_xio|seg_pack_iov");
	}   /* --END ERROR HANDLING-- */

	mpig_iov_add_entry(sreq_cm->iov, mpig_databuf_get_ptr(sreq_cm->databuf), last - sreq_cm->stream_pos);
	mpig_databuf_set_eod(sreq_cm->databuf, last - sreq_cm->stream_pos);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA,
	    "sparse user buffer; intermediate buffer packed; IOV loaded: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT
	    ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, sreq->handle, (MPIG_PTR_CAST) sreq,
	    sreq_cm->stream_pos, last, sreq_cm->stream_size));
	
	sreq_cm->stream_pos = last;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x, failed=%s",
		       sreq->handle, (MPIG_PTR_CAST) sreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_sreq_pack);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_stream_sreq_pack() */


/*
 * void mpig_cm_xio_stream_rreq_init([IN/MOD] rreq, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * initialize the receive request's stream management fields in preparation for packing data via mpig_cm_xio_stream_rreq_unpack().
 * if the user buffer, as described by the (count, datatype) tuple, is too sparse to efficiently use an IOV, or the incoming data
 * requires conversion, the stream will be setup to use an intermediate buffer.
 *
 * rreq [IN/MOD] - receive request
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * NOTE: this routine assumes that the request fields associate with the user buffer are set (use mpig_request_set_buffer()).  it
 * also assumes that rreq_cm->stream_size is set to the number of bytes of data to be processed.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_rreq_init
MPIG_STATIC void mpig_cm_xio_stream_rreq_init(
    MPID_Request * const rreq, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * rreq_cm = &rreq->cm.xio;
    void * buf;
    int cnt;
    MPI_Datatype dt;
    bool_t dt_contig;
    MPIU_Size_t dt_size;
    MPIU_Size_t dt_nblks;
    MPI_Aint dt_true_lb;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_rreq_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_rreq_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", stream_size=" MPIG_SIZE_FMT
		       ", mpi_errno=0x%08x", rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_size, *mpi_errno_p));
    *failed_p = FALSE;

    /* get information about the user buffer.  store details in the request. */
    mpig_request_get_buffer(rreq, &buf, &cnt, &dt);
    mpig_datatype_get_info(dt, &dt_contig, &dt_size, &dt_nblks, &dt_true_lb);

    rreq_cm->buf_size = cnt * dt_size;
    rreq_cm->buf_true_lb = dt_true_lb;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "df=%d, local_df=%d, buf_size=" MPIG_SIZE_FMT, rreq_cm->df, GLOBUS_DC_FORMAT_LOCAL,
		       rreq_cm->buf_size));
    
    /* rreq_cm->stream_size -- must be set prior to calling this routine */
    rreq_cm->stream_pos = 0;
    rreq_cm->stream_max_pos = rreq_cm->stream_size;
    rreq->status.count = 0;

    if ((dt_contig && rreq_cm->df == GLOBUS_DC_FORMAT_LOCAL) || cnt == 0)
    {
	/* if the user buffer is contiguous, then receive data straight into the user buffer.  use of a segment is not required. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "contig user buffer; loading IOV: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
			   ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
			   rreq_cm->buf_size, rreq_cm->stream_size));
	
	rreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_CONTIG;
    }
    else if (cnt * dt_nblks <= (MPIU_Size_t) mpig_iov_get_num_free_entries(rreq_cm->iov) ||
	     dt_size / dt_nblks >= MPIG_CM_XIO_DATA_DENSITY_THRESHOLD / (MPIU_Size_t) mpig_iov_get_num_entries(rreq_cm->iov))
    {
	/* if the user buffer is noncontiguous but the data is dense enough that an IOV can being used efficiently, then the data
	   will be received straight into the user buffer */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "dense user buffer; initializing segment: rreq=" MPIG_HANDLE_FMT ", rreqp="
			   MPIG_PTR_FMT ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle,
			   (MPIG_PTR_CAST) rreq, rreq_cm->buf_size, rreq_cm->stream_size));
	
	MPID_Segment_init(buf, cnt, dt, &rreq_cm->seg, 0); /* XXX: USE GLOBUS DC VERSION ??? */

	rreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_DENSE;
    }
    else
    {
	/* if the user buffer is noncontiguous and the data density is rather sparse, then an intermediate buffer is used to
	   first condense the data, allowing more data to be acquired in a single write operation.  the intermediate buffer is
	   only allocated here if an unexpected buffer is not already attached to the request. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; initializing segment and allocating bufffer: rreq="
			   MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", buf_size=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->buf_size, rreq_cm->stream_size));
	
	MPID_Segment_init(buf, cnt, dt, &rreq_cm->seg, 0);  /* XXX: USE GLOBUS DC VERSION ??? */

	if (rreq_cm->databuf == NULL)
	{
	    mpig_databuf_create((MPIU_Size_t) MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE, &rreq_cm->databuf, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s",
				 "intermediate unpack buffer");
	}
	
	rreq_cm->buf_type = MPIG_CM_XIO_USERBUF_TYPE_SPARSE;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x"
		       ", failed=%s", rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_rreq_init);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_stream_rreq_init() */


/*
 * void mpig_cm_xio_stream_rreq_unpack([IN/MOD] rreq, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * populate the receive request's IOV with the location(s) in which to read the next set of incoming data associated with the
 * current message.  if the user buffer is too sparse to efficiently use an IOV, or the incoming data requires conversion, then
 * would have previously been read into an intermediate buffer.  in that case, as much of the data as possible is unpacked, and
 * then IOV is pointed at the first free location in that buffer after the buffer is compacted.
 *
 * rreq [IN/MOD] - receive request
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * NOTE: this routine may be called even all of the data described by the IOV has not been read.  the only restriction is that at
 * least one entry must be free in the IOV, so it is recommend that the IOV be compacted before this routine is called.
 * (mpig_iov_unpack() automatically compacts the IOV.)  this capability was considered necessary to reduce the number of read
 * operations, specifically when the rendezvous protocol is used with noncontiguous buffers or data requiring conversion.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_rreq_unpack
MPIG_STATIC void mpig_cm_xio_stream_rreq_unpack(
    MPID_Request * const rreq, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * rreq_cm = &rreq->cm.xio;
    MPIU_Size_t last;
    MPIU_Size_t nbytes;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_rreq_unpack);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_rreq_unpack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ". mpi_errno=0x%08x",
		       rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p));
    *failed_p = FALSE;

    if (rreq_cm->stream_pos == rreq_cm->stream_size)
    {
	/* if all of the data has been acquired, then free an databuf associated with the request and return */
	if (rreq_cm->databuf != NULL)
	{
	    mpig_databuf_destroy(rreq_cm->databuf);
	    rreq_cm->databuf = NULL;
	}

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "data acquisition complete: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
			   ", nbytes_received=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos));
    }
    else if (rreq_cm->stream_pos == rreq_cm->stream_max_pos)
    {
	/* if all of the maximum amount of data allowed has been acquired, then exit */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "stream stalled; pos = max_pos; skipping IOV reload: rreq=" MPIG_HANDLE_FMT
			   ", rreqp=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			   rreq_cm->stream_size));
    }
    else if (rreq_cm->stream_pos != (MPIU_Size_t) rreq->status.count)
    {
	/* if truncation has occurred, then set the IOV to drain the next chunck bytes from the network. */
	mpig_cm_xio_stream_rreq_handle_truncation(rreq, (MPIU_Size_t) 0, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_trunc");
    }
    else if (rreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_CONTIG)
    {
	/* the user buffer is contigious, and no data conversion is required, so the IOV is set to read data directly into the
	   user buffer */
	
	/* if an unexpected bufffer exists, then unpack as much of the data as possible */
	if (rreq_cm->databuf != NULL)
	{
	    last = MPIG_MIN(MPIG_MIN(rreq_cm->buf_size, rreq_cm->stream_max_pos),
			    (rreq_cm->stream_pos + mpig_databuf_get_remaining_bytes(rreq_cm->databuf)));
	    nbytes = last - rreq_cm->stream_pos;

	    if (nbytes > 0)
	    {
		memcpy((char*) rreq->dev.buf + rreq_cm->buf_true_lb + rreq_cm->stream_pos,
		       mpig_databuf_get_pos_ptr(rreq_cm->databuf), nbytes);

		mpig_databuf_inc_pos(rreq_cm->databuf, nbytes);
		if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) == 0)
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "contig user buffer; fully unpacked unexpected buffer: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			 rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));
		    
		    mpig_databuf_destroy(rreq_cm->databuf);
		    rreq_cm->databuf = NULL;
		}
		else
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "contig user buffer; partiallly unpacked unexpected buffer: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			 rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));
		}
		
		rreq_cm->stream_pos = last;
		rreq->status.count = last;
	    }
	}

	/* if more data needs to be acquired, then load the IOV with a pointer to the user buffer */
	last = MPIG_MIN(rreq_cm->buf_size, rreq_cm->stream_max_pos);
	nbytes = last - rreq_cm->stream_pos;

	if (nbytes > 0)
	{
	    MPIU_Assert(rreq_cm->databuf == NULL);
	    
	    /* load the IOV with the location to drop the rest of the data up to the maximum size of the user buffer */
	    mpig_iov_add_entry(rreq_cm->iov, (char *) rreq->dev.buf + rreq_cm->buf_true_lb + rreq_cm->stream_pos, nbytes);
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "contig user buffer; IOV loaded: rreq=" MPIG_HANDLE_FMT ", rreqp="
			       MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			       rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->buf_size, rreq_cm->stream_size));
	    
	    rreq_cm->stream_pos = last;
	    rreq->status.count = last;
	}

	/* if more data was available than user buffer space, then call the routine to handle truncation */
	if (rreq_cm->stream_pos < rreq_cm->stream_max_pos)
	{
	    nbytes = (rreq_cm->databuf != NULL) ? mpig_databuf_get_remaining_bytes(rreq_cm->databuf) : 0;
	    mpig_cm_xio_stream_rreq_handle_truncation(rreq, nbytes, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_trunc");
	}
    }
    else if (rreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_DENSE)
    {
	/* the user buffer is noncontiguous, but dense enough that an IOV is being used, so the IOV is loaded with the locations
	   in user buffer in which to place the next set of data. */
	MPID_IOV * iov;
	int iov_count;

	/* if an unexpected bufffer exists, then unpack as much of the data as possible */
	if (rreq_cm->databuf != NULL)
	{
	    last = MPIG_MIN(rreq_cm->stream_pos + mpig_databuf_get_remaining_bytes(rreq_cm->databuf), rreq_cm->stream_max_pos);
	    MPID_Segment_unpack(&rreq_cm->seg, (MPI_Aint) rreq_cm->stream_pos, (MPI_Aint *) &last,
				mpig_databuf_get_pos_ptr(rreq_cm->databuf));
	    if (last != rreq_cm->stream_pos)
	    {
		mpig_databuf_inc_pos(rreq_cm->databuf, last - rreq_cm->stream_pos);
		rreq_cm->stream_pos = last;
		rreq->status.count = last;
		
		if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) == 0)
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; fully unpacked unexpected buffer: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			 rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));
		    
		    mpig_databuf_destroy(rreq_cm->databuf);
		    rreq_cm->databuf = NULL;
		}
		else
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; partially unpacked unexpected buffer: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			 rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));
		}
	    }
	}

	/* if more data is required, then load the IOV with positions in the user buffer in which to place the data */
	if (rreq_cm->stream_pos < rreq_cm->stream_max_pos)
	{
	    iov = mpig_iov_get_next_free_entry_ptr(rreq_cm->iov);
	    iov_count = mpig_iov_get_num_free_entries(rreq_cm->iov);
	    MPIU_Assert(iov_count > 0);

	    last = rreq_cm->stream_max_pos;
	    MPID_Segment_unpack_vector(&rreq_cm->seg, (MPI_Aint) rreq_cm->stream_pos, (MPI_Aint *) &last, iov, &iov_count);
	    if (last != rreq_cm->stream_pos)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "dense user buffer; IOV loaded: rreq=" MPIG_HANDLE_FMT", rreqp="
				   MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
				   rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));

		mpig_iov_inc_num_inuse_entries(rreq_cm->iov, iov_count);
		mpig_iov_inc_num_bytes(rreq_cm->iov, last - rreq_cm->stream_pos);
		
		rreq_cm->stream_pos = last;
		rreq->status.count = last;

		/* if data still remains in an unexpected buffer, then extract data from it.  this could happen if the amount of
		   data in the buffer was less than the next basic type to be processed.  as such, we would expect a very small
		   amount of data to be in the buffer. */
		if (rreq_cm->databuf != NULL)
		{
		    nbytes = mpig_iov_unpack(mpig_databuf_get_pos_ptr(rreq_cm->databuf),
					     mpig_databuf_get_remaining_bytes(rreq_cm->databuf),
					     rreq_cm->iov);
		    mpig_databuf_inc_pos(rreq_cm->databuf, nbytes);

		    rreq_cm->stream_pos = last;
		    rreq->status.count = last;
		    
		    if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) == 0)
		    {
			if (mpig_iov_get_num_bytes(rreq_cm->iov) > 0)
			{
			    MPIG_DEBUG_PRINTF(
				(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; fully unpacked unexpected buffer; IOV adjusted: rreq="
				 MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT
				 ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
				 rreq_cm->stream_pos - nbytes, last, rreq_cm->stream_size));
			}
			else 
			{
			    MPIG_DEBUG_PRINTF(
				(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; fully unpacked unexpected buffer; IOV empty: rreq="
				 MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT
				 ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
				 rreq_cm->stream_pos - nbytes, last, rreq_cm->stream_size));
			}
			
			mpig_databuf_destroy(rreq_cm->databuf);
			rreq_cm->databuf = NULL;
		    }
		    else
		    {
			if (rreq_cm->stream_max_pos < rreq_cm->stream_size)
			{
			    MPIG_DEBUG_PRINTF(
				(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; partially unpacked unexpected buffer; IOV empty: rreq="
				 MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT
				 ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
				 rreq_cm->stream_pos - nbytes, last, rreq_cm->stream_size));

			    rreq_cm->stream_max_pos = rreq_cm->stream_pos;
			}
			else
			{
			    MPIG_DEBUG_PRINTF(
				(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; partially unpacked unexpected buffer; end of stream: "
				 "rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT
				 ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
				 rreq_cm->stream_pos - nbytes, last, rreq_cm->stream_size));
			    
			    nbytes = (rreq_cm->databuf != NULL) ? mpig_databuf_get_remaining_bytes(rreq_cm->databuf) : 0;
			    mpig_cm_xio_stream_rreq_handle_truncation(rreq, nbytes, mpi_errno_p, &failed);
			    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_trunc");
			}
		    }
		} /* endif (rreq_cm->databuf != NULL) */
	    }
	    else /* if (last == rreq_cm->stream_pos) */
	    {
		if (mpig_iov_get_num_bytes(rreq_cm->iov) > 0)
		{
		    /* a call was made to reload the IOV when the IOV wasn't completely empty.  in this case, the lack of
		       advancement of the position counter is not a problem since data still exists to be acquired.  that is not
		       to say that a problem doesn't exist, but it will be detected during the next pass. */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; position did not advance; IOV not empty; ignoring: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));
		}
		else if (rreq_cm->stream_max_pos < rreq_cm->stream_size)
		{
		    /* if the maximum stream position is less than the whole stream of data, then the unpack routine stopped
		       short because it was at the last basic type boundary before the maximum position.  for now reset the
		       maximum position to the current position to prevent the system from trying to unpack again until the
		       maximum position is advanced. */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; position did not advance; resetting max_pos: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));
		    
		    rreq_cm->stream_max_pos = rreq_cm->stream_pos;
		}
		else
		{
		    /* if truncation has occurred, then call the truncation handler to populate the IOV to drain the next chunk
		       of data from the network. */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "dense user buffer; position did not advance; handling truncation: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));

		    nbytes = (rreq_cm->databuf != NULL) ? mpig_databuf_get_remaining_bytes(rreq_cm->databuf) : 0;
		    mpig_cm_xio_stream_rreq_handle_truncation(rreq, nbytes, mpi_errno_p, &failed);
		    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_trunc");
		}
	    }
	}
    }
    else /* if (rreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_SPARSE) */
    {
	/* the user buffer is noncontiguous and the data density is rather sparse, or if data conversion is necessaary, so an
	   intermediate buffer is used to receive the data in condensed form, allowing more data to be received by a single read
	   operation */
	bool_t reload_iov = TRUE;
	
	MPIU_Assert(rreq_cm->buf_type == MPIG_CM_XIO_USERBUF_TYPE_SPARSE);
	
	/* adjust the end of the data in the intermediate buffer if the IOV still describes data to be acquired */
	nbytes = mpig_iov_get_num_bytes(rreq_cm->iov);
	if (nbytes > 0)
	{
	    MPIG_DEBUG_PRINTF(
		(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; adjusting intermediate buffer EOD and IOV before unpack: rreq="
		 MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", old_eod=" MPIG_SIZE_FMT ", new_eod=" MPIG_SIZE_FMT, rreq->handle,
		 (MPIG_PTR_CAST) rreq, mpig_databuf_get_eod(rreq_cm->databuf), mpig_databuf_get_eod(rreq_cm->databuf) + nbytes));
	    
	    mpig_databuf_dec_eod(rreq_cm->databuf, nbytes);
	    mpig_iov_reset(rreq_cm->iov, 0);
	}

	/* attempt to unpack any data in the intermediate buffer */
	if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) > 0)
	{
	    last = MPIG_MIN(rreq_cm->stream_pos + mpig_databuf_get_remaining_bytes(rreq_cm->databuf), rreq_cm->stream_max_pos);
	    MPID_Segment_unpack(&rreq_cm->seg, (MPI_Aint) rreq_cm->stream_pos, (MPI_Aint *) &last,
				mpig_databuf_get_ptr(rreq_cm->databuf));
	
	    if (last != rreq_cm->stream_pos)
	    {
		MPIG_DEBUG_PRINTF(
		    (MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; unpack advanced position: rreq=" MPIG_HANDLE_FMT ", rreqp="
		     MPIG_PTR_FMT ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
		     rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, last, rreq_cm->stream_size));
	    
		/* update the intermediate buffer, status count, and stream position to reflect bytes consumed by the unpack */
		mpig_databuf_inc_pos(rreq_cm->databuf, last - rreq_cm->stream_pos);
		rreq_cm->stream_pos = last;
		rreq->status.count = last;

		/* if any bytes remain in the intermediate buffer, move them to the beginning of the buffer; otherwise, reset the
		   intermediate buffer to its zeroed state */
		if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) > 0 && mpig_databuf_get_pos(rreq_cm->databuf) > 0)
		{
		    nbytes = mpig_databuf_get_remaining_bytes(rreq_cm->databuf);
		    memmove(mpig_databuf_get_ptr(rreq_cm->databuf), mpig_databuf_get_pos_ptr(rreq_cm->databuf), nbytes);
		    mpig_databuf_set_pos(rreq_cm->databuf, 0);
		    mpig_databuf_set_eod(rreq_cm->databuf, nbytes);
		}
		else
		{
		    mpig_databuf_reset(rreq_cm->databuf);
		}

		if (rreq_cm->stream_pos == rreq_cm->stream_size)
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; data acquisition complete: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", nbytes_received=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
			 rreq_cm->stream_pos));
		    
		    reload_iov = FALSE;
		}
		else if (rreq_cm->stream_pos == rreq_cm->stream_max_pos)
		{
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; pos = maxpos; skipping IOV reload: rreq=" MPIG_HANDLE_FMT
			 ", rreqp=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT,
			 rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos, rreq_cm->stream_size));
		    
		    reload_iov = FALSE;
		}
	    }
	    else
	    {
		if (mpig_databuf_get_remaining_bytes(rreq_cm->databuf) < mpig_databuf_get_size(rreq_cm->databuf) &&
		    mpig_databuf_get_remaining_bytes(rreq_cm->databuf) < rreq_cm->stream_max_pos - rreq_cm->stream_pos)
		{
		    /* if the intermediate buffer is not full and it does not the remainder of the stream in it, then reload the
		       IOV to acquire more data */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; position did not advance; need more data: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT,rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));
		    
		}
		else if (rreq_cm->stream_max_pos < rreq_cm->stream_size)
		{
		    /* if the maximum stream position is less than the whole stream of data, then the unpack routine stopped
		       short because it was at the last basic type boundary before the maximum position.  for now reset the
		       maximum position to the current position to prevent the system from trying to unpack again until the
		       maximum position is advanced. */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; position did not advance; resetting max_pos: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));
		    
		    rreq_cm->stream_max_pos = rreq_cm->stream_pos;
		    reload_iov = FALSE;
		}
		else
		{
		    /* if truncation has occurred, then call the truncation handler to populate the IOV to drain the next chunk
		       of data from the network. */
		    MPIG_DEBUG_PRINTF(
			(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; position did not advance; handling truncation: rreq="
			 MPIG_HANDLE_FMT ", rreq_p=" MPIG_PTR_FMT ", pos=" MPIG_SIZE_FMT ", max_pos=" MPIG_SIZE_FMT ", size="
			 MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_max_pos,
			 rreq_cm->stream_size));
		    
		    mpig_cm_xio_stream_rreq_handle_truncation(rreq, mpig_databuf_get_remaining_bytes(rreq_cm->databuf),
							      mpi_errno_p, &failed);
		    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_trunc");
		    
		    reload_iov = FALSE;
		}
	    }
	}
	
	if (reload_iov)
	{
	    /* if the current intermediate buffer is too small (because it started as an unexpected data buffer), then allocate a
	       new buffer and transfer any data remaining in the old buffer to the new buffer. */
	    nbytes = MPIG_MIN((rreq_cm->stream_size - rreq_cm->stream_pos), MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE);
	    if (mpig_databuf_get_size(rreq_cm->databuf) < nbytes)
	    {
		mpig_databuf_t * old_databuf;

		old_databuf = rreq_cm->databuf;
		
		mpig_databuf_create(nbytes, &rreq_cm->databuf, mpi_errno_p, &failed);
		MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s",
				     "intermediate unpack buffer");

		nbytes = mpig_databuf_get_remaining_bytes(old_databuf);
		if (nbytes > 0)
		{
		    memcpy(mpig_databuf_get_ptr(rreq_cm->databuf), mpig_databuf_get_pos_ptr(old_databuf), nbytes);
		    mpig_databuf_set_eod(rreq_cm->databuf, nbytes);
		}

		mpig_databuf_destroy(old_databuf);
	    }
	    
	    /* compute the number of bytes left to read, capping it at the number of bytes available in the intermediate buffer */
	    nbytes = rreq_cm->stream_max_pos - rreq_cm->stream_pos - mpig_databuf_get_remaining_bytes(rreq_cm->databuf);
	    if (nbytes > mpig_databuf_get_free_bytes(rreq_cm->databuf))
	    {
		nbytes = mpig_databuf_get_free_bytes(rreq_cm->databuf);
	    }

	    MPIU_Assert(nbytes > 0);

	    /* populate the IOV with a pointer to the intermediate buffer */
	    MPIU_Assert(mpig_iov_get_num_inuse_entries(rreq_cm->iov) == 0);
	    mpig_iov_add_entry(rreq_cm->iov, mpig_databuf_get_eod_ptr(rreq_cm->databuf), nbytes);
	    mpig_databuf_inc_eod(rreq_cm->databuf, nbytes);

	    MPIG_DEBUG_PRINTF(
		(MPIG_DEBUG_LEVEL_DATA, "sparse user buffer; IOV loaded: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
		 ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle,
		 (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos, rreq_cm->stream_pos + nbytes, rreq_cm->stream_size));
	}
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x"
		       ", failed=%s", rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_rreq_unpack);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_rreg_stream_rreq_unpack() */


/*
 * void mpig_cm_xio_stream_rreq_unpack_vc_msgbuf([IN/MOD] vc, [IN/MOD] rreq, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * unpack any data that might be in the message buffer (following the message header)
 * 
 * rreq [IN/MOD] - receive request
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_rreq_unpack_vc_msgbuf
MPIG_STATIC void mpig_cm_xio_stream_rreq_unpack_vc_msgbuf(
    mpig_vc_t * const vc, MPID_Request * const rreq, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_vc * vc_cm = &vc->cm.xio;
    struct mpig_cm_xio_request * rreq_cm = &rreq->cm.xio;
    MPIU_Size_t nbytes;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_memcpy);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_rreq_unpack_vc_msgbuf);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_rreq_unpack_vc_msgbuf);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x",
		       rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p));
    *failed_p = FALSE;

    /* if any data remains in the VC message buffer, then move it into the user or intermediate buffer */
    while (mpig_databuf_get_remaining_bytes(vc_cm->msgbuf) > 0 && mpig_iov_get_num_bytes(rreq_cm->iov) > 0)
    {
	nbytes = mpig_iov_unpack(mpig_databuf_get_pos_ptr(vc_cm->msgbuf),
				 mpig_databuf_get_remaining_bytes(vc_cm->msgbuf),
				 rreq_cm->iov);
	mpig_databuf_inc_pos(vc_cm->msgbuf, nbytes);
	if (mpig_iov_get_num_bytes(rreq_cm->iov) == 0)
	{
	    /* the IOV was satisfied by the message buffer, so reload the IOV */
	    mpig_cm_xio_stream_rreq_unpack(rreq, mpi_errno_p, &failed);
	    MPIU_ERR_CHKANDJUMP((failed), *mpi_errno_p, MPI_ERR_OTHER, "**globus|cm_xio|stream_rreq_unpack");
	}
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x"
		       ", failed=%s", rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_rreq_unpack_vc_msgbuf);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_stream_rreq_unpack_vc_msgbuf() */


/*
 * void mpig_cm_xio_stream_rreq_handle_truncation(
 *          [IN/MOD] rreq, [IN] buffered_bytes, [OUT] complete, [IN/OUT] mpi_errno, [OUT] failed)
 *
 * set the IOV to read any extraneous bytes from the network.  if all of the bytes have already been read (possibly buffered),
 * then the IOV remains unaltered.
 * 
 * rreq [IN/MOD] - receive request
 * buffered_bytes [IN] - number of bytes already read in and buffered (and thus should be ignored)
 * mpi_errno [IN/OUT] - MPI error code
 * failed [OUT] - TRUE if the routine failed; FALSE otherwise
 *
 * NOTE: this routine may need to be called more than once if the amount of extraneous data being received is larger than
 * MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_stream_rreq_handle_truncation
MPIG_STATIC void mpig_cm_xio_stream_rreq_handle_truncation(
    MPID_Request * const rreq, const MPIU_Size_t buffered_bytes, int * const mpi_errno_p, bool_t * const failed_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_cm_xio_request * rreq_cm = &rreq->cm.xio;
    MPIU_Size_t nbytes;
    bool_t failed;
    MPIG_STATE_DECL(MPID_STATE_memcpy);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_stream_rreq_handle_truncation);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_stream_rreq_handle_truncation);
    MPIG_DEBUG_PRINTF(
	(MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
	 "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", bufferred_bytes=" MPIG_SIZE_FMT
	 ", mpi_errno=0x%08x", rreq->handle, (MPIG_PTR_CAST) rreq, buffered_bytes, *mpi_errno_p));
    *failed_p = FALSE;

    /* If this is the first call, then generate an error and attach it to the request */
    if (rreq_cm->stream_pos == (MPIU_Size_t) rreq->status.count)
    {
        MPIU_ERR_SET2(rreq->status.MPI_ERROR, MPI_ERR_TRUNCATE, "**truncate", "**truncate %d %d",
		      rreq_cm->stream_size, rreq_cm->stream_pos);

	/* XXX: we need to detect if this is really a truncation error or a datatype mismatch error.  to do this, we must write a
	   contig function that gets passed to MPID_Segment_manipulate().  the routine needs to save the size of the next basic
	   type in the user arg.  if the size of the basic type is greater than the amount remaining data (size - pos), then the
	   problem is a datatype mismatch, not a truncation error. */
    }

    /* adjust for any bytes already sitting a buffer */
    if (buffered_bytes)
    {
	rreq_cm->stream_pos += buffered_bytes;
	MPIU_Assert(rreq_cm->stream_pos <= rreq_cm->stream_max_pos);
	
	MPIG_DEBUG_PRINTF(
	    (MPIG_DEBUG_LEVEL_DATA, "truncation; consumed buffered bytes: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
	     ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle,
	     (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos - buffered_bytes, rreq_cm->stream_pos, rreq_cm->stream_size));
    }

    /* if all of the data has been read, then notify the calling routine and return */
    if (rreq_cm->stream_pos == rreq_cm->stream_size)
    {
	goto fn_return;
    }

    /* if the request does not have a sufficiently sized buffer, the allocate a temporary buffer in which to receive the data */
    if (rreq_cm->databuf == NULL || mpig_databuf_get_size(rreq_cm->databuf) < MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE)
    {
	if (rreq_cm->databuf != NULL) mpig_databuf_destroy(rreq_cm->databuf);
	mpig_databuf_create((MPIU_Size_t) MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE, &rreq_cm->databuf, mpi_errno_p, &failed);
	MPIU_ERR_CHKANDJUMP1((failed), *mpi_errno_p, MPI_ERR_OTHER, "**nomem", "**nomem %s", "truncation receive buffer");
    }

    /* set the IOV to receive the extra data (as much of it as possible) into the temporary buffer */
    nbytes = MPIG_MIN(mpig_databuf_get_size(rreq_cm->databuf), rreq_cm->stream_max_pos - rreq_cm->stream_pos);
    
    if (nbytes > 0)
    {
	mpig_databuf_set_pos(rreq_cm->databuf, 0);
	mpig_databuf_set_eod(rreq_cm->databuf, nbytes);
	mpig_iov_add_entry(rreq_cm->iov, mpig_databuf_get_ptr(rreq_cm->databuf), nbytes);
	rreq_cm->stream_pos += nbytes;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "truncation; IOV loaded: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT
			   ", start=" MPIG_SIZE_FMT ", end=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle,
			   (MPIG_PTR_CAST) rreq, rreq_cm->stream_pos - nbytes, rreq_cm->stream_pos, rreq_cm->stream_size));
    }
    else
    {
	MPIG_DEBUG_PRINTF(
	    (MPIG_DEBUG_LEVEL_DATA, "truncation; stream position limit reached; IOV not set: rreq=" MPIG_HANDLE_FMT ", rreqp="
	     MPIG_PTR_FMT ", max_pos=" MPIG_SIZE_FMT ", stream_size=" MPIG_SIZE_FMT, rreq->handle, (MPIG_PTR_CAST) rreq,
	     rreq_cm->stream_max_pos, rreq_cm->stream_size));
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", mpi_errno=0x%08x"
		       ", failed=%s", rreq->handle, (MPIG_PTR_CAST) rreq, *mpi_errno_p, MPIG_BOOL_STR(*failed_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_stream_rreq_handle_truncation);
    return;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	*failed_p = TRUE;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_stream_rreq_handle_truncation() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					  END RECEIVE REQUEST DATA MARSHALLING ROUTINES
**********************************************************************************************************************************/
