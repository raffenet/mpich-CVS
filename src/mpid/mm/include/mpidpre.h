#ifndef MPIDPRE_H
#define MPIDPRE_H

#include "mpi.h"
#include "mpidconf.h"
#include "mpid_datatype.h"

#ifdef HAVE_NEW_METHOD
#include "new_method_pre.h"
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MPID_VECTOR         WSABUF
#define MPID_VECTOR_LEN     len
#define MPID_VECTOR_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MPID_VECTOR         struct iovec
#define MPID_VECTOR_LEN     iov_len
#define MPID_VECTOR_BUF     iov_base
#endif
#define MPID_VECTOR_LIMIT   16

typedef enum MM_BUFFER_TYPE {
    MM_NULL_BUFFER,
    MM_TMP_BUFFER,
    MM_MPI_BUFFER,
#ifdef WITH_METHOD_SHM
    MM_SHM_BUFFER,
#endif
#ifdef WITH_METHOD_VIA
    MM_VIA_BUFFER,
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_VIA_RDMA_BUFFER,
#endif
#ifdef WITH_METHOD_NEW
    MM_NEW_METHOD_BUFFER,
#endif
    MM_END_MARKER_BUFFER
} MM_BUFFER_TYPE;

typedef enum MM_CAR_TYPE { 
    MM_NULL_CAR,
    MM_READ_CAR,
    MM_WRITE_CAR,
    MM_END_MARKER_TYPE_CAR
} MM_CAR_TYPE;

typedef struct MM_Car
{
    struct MPID_Request *request_ptr;
    struct MPIDI_VC *vc_ptr;
    int src, dest;
    MM_CAR_TYPE type;
    union data {
	struct packer
	{
	    int first;
	    int last;
	} packer;
	struct unpacker
	{
	    int first;
	    int last;
	} unpacker;
#ifdef WITH_METHOD_SHM
	struct shm 
	{
	} shm;
#endif
#ifdef WITH_METHOD_TCP
	struct tcp
	{
	} tcp;
#endif
#ifdef WITH_METHOD_VIA
	struct 
	{
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct
	{
	} viardma;
#endif
#ifdef HAVE_NEW_METHOD
#ifdef WITH_METHOD_NEW
	MPID_DEV_METHOD_NEW_DECL
#endif
#endif
    } data;
    struct MM_Car *next_ptr, *qnext_ptr;
} MM_Car;

typedef struct MM_Segment
{
    int tag;
    union buf {
	const void *send;
	void *recv;
    } buf;
    int count;
    MPI_Datatype dtype;
    int first;
    int last;
    MPID_Segment segment;
    MM_Car *write_list;
    MM_Car wcar;
    MM_Car rcar;
    int op_valid;
    MM_BUFFER_TYPE read_buf_type;
    union MM_Segment_buffer
    {
	struct tmp
	{
	    void *buf_ptr[2];
	    int cur_buf;
	    int num_read;
	    int min_num_written;
	} tmp;
	struct mpi
	{
	    MPID_VECTOR vec[MPID_VECTOR_LIMIT];
	    int size;
	    int num_read;
	    int min_num_written;
	} mpi;
#ifdef WITH_METHOD_SHM
	struct shm
	{
	    void *shm_ptr;
	    int num_read;
	} shm;
#endif
#ifdef WITH_METHOD_VIA
	struct via
	{
	    void *descriptor_ptr;
	    int num_descriptors;
	} via;
#endif
#ifdef WITH_METHOD_VIA_RDMA
	struct via_rdma
	{
	    void *descriptor_ptr;
	    int num_descriptors;
	} via_rdma;
#endif
#ifdef WITH_METHOD_NEW
#endif
    } read_buf;
    struct MPID_Request *next_ptr;
} MM_Segment;

#define MPID_DEV_REQUEST_DECL MM_Segment mm;

typedef struct MM_Comm_struct
{
    char *pmi_kvsname;
} MM_Comm_struct;

#define MPID_DEV_COMM_DECL MM_Comm_struct mm;

#endif
