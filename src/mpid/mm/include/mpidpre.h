#ifndef MPIDPRE_H
#define MPIDPRE_H

#include "mpi.h"
#include "mpidconf.h"

#ifdef HAVE_NEW_METHOD
#include "new_method_pre.h"
#endif

#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#define MM_VECTOR         WSABUF
#define MM_VECTOR_LEN     len
#define MM_VECTOR_BUF     buf
#else
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#define MM_VECTOR         struct iovec
#define MM_VECTOR_LEN     iov_len
#define MM_VECTOR_BUF     iov_base
#endif
#define MM_VECTOR_LIMIT   16

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

/* name encoding:
 * MM {METHOD} WRITE from {METHOD} buffer CAR
 * MM {METHOD} READ into {METHOD} Buffer CAR
 * MM UNPACK from {METHOD} buffer CAR
 */
typedef enum MM_CAR_TYPE { 
    MM_NULL_CAR,
    MM_UNBOUND_READ_CAR,
    MM_UNBOUND_WRITE_CAR,
    MM_UNBOUND_UNPACK_CAR,
    MM_UNPACK_TMP_CAR,
#ifdef WITH_METHOD_SHM
    MM_UNPACK_SHM_CAR,
    MM_SHM_WRITE_MPI_CAR, 
    MM_SHM_WRITE_TMP_CAR,
    MM_SHM_WRITE_VIA_CAR,
    MM_SHM_READ_MPI_CAR,
    MM_SHM_READ_TMP_CAR,
    MM_SHM_READ_VIA_CAR,
#endif
#ifdef WITH_METHOD_TCP
    MM_TCP_WRITE_MPI_CAR, 
    MM_TCP_WRITE_TMP_CAR,
    MM_TCP_WRITE_SHM_CAR,
    MM_TCP_WRITE_VIA_CAR,
    MM_TCP_READ_MPI_CAR,
    MM_TCP_READ_TMP_CAR,
    MM_TCP_READ_SHM_CAR,
    MM_TCP_READ_VIA_CAR,
#endif
#ifdef WITH_METHOD_VIA
    MM_UNPACK_VIA_CAR,
    MM_VIA_WRITE_MPI_CAR, 
    MM_VIA_WRITE_TMP_CAR,
    MM_VIA_WRITE_SHM_CAR,
    MM_VIA_READ_MPI_CAR,
    MM_VIA_READ_TMP_CAR,
    MM_VIA_READ_SHM_CAR,
#endif
#ifdef WITH_METHOD_VIA_RDMA
    MM_UNPACK_VIA_RDMA_CAR,
    MM_VIA_RDMA_WRITE_MPI_CAR,
    MM_VIA_RDMA_WRITE_TMP_CAR,
    MM_VIA_RDMA_WRITE_SHM_CAR,
    MM_VIA_RDMA_READ_MPI_CAR,
    MM_VIA_RDMA_READ_TMP_CAR,
    MM_VIA_RDMA_READ_SHM_CAR,
#endif
#ifdef WITH_METHOD_NEW
    MM_UNPACK_NEW_METHOD_CAR,
    MM_NEW_METHOD_WRITE_MPI_CAR,
    MM_NEW_METHOD_WRITE_TMP_CAR,
    MM_NEW_METHOD_WRITE_SHM_CAR,
    MM_NEW_METHOD_WRITE_VIA_CAR,
    MM_NEW_METHOD_READ_MPI_CAR,
    MM_NEW_METHOD_READ_TMP_CAR,
    MM_NEW_METHOD_READ_SHM_CAR,
    MM_NEW_METHOD_READ_VIA_CAR,
#endif
    MM_END_MARKER_CAR
} MM_CAR_TYPE;

typedef struct MM_Car
{
    struct MPID_Request *request_ptr;
    struct MPIDI_VC *vc_ptr;
    int src, dest;
    MM_CAR_TYPE type;
    union data {
	int dummy;
#ifdef WITH_METHOD_SHM
#endif
#ifdef WITH_METHOD_TCP
#endif
#ifdef WITH_METHOD_VIA
#endif
#ifdef WITH_METHOD_VIA_RDMA
#endif
#ifdef HAVE_NEW_METHOD
#ifdef WITH_METHOD_NEW
	MPID_DEV_METHOD_NEW_DECL
#endif
#endif
    } data;
    struct MM_Car *next_ptr;
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
	    MM_VECTOR vec[MM_VECTOR_LIMIT];
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
