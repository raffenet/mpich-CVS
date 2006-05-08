/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_VMPI_H_INCLUDED)
#define MPICH2_MPIG_VMPI_H_INCLUDED 1


/* XXX: do we need still these, or is there enough information in the VC/PG to make these obsolete? */
#if XXX
extern int *				mpig_vmpi_vcwrank_to_pgrank;
extern int *				mpig_vmpi_pgrank_to_vcwrank;
#endif


/*
 * types large enough to hold vendor MPI object handles and structures
 */
typedef struct
{
    mpig_aligned_t vcomm[(SIZEOF_VMPI_COMM + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)];
}
mpig_vmpi_comm_t;

typedef struct
{
    mpig_aligned_t vdt[(SIZEOF_VMPI_DATATYPE + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)];
}
mpig_vmpi_datatype_t;

typedef struct
{
    mpig_aligned_t vreq[(SIZEOF_VMPI_REQUEST + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)];
}
mpig_vmpi_request_t;

typedef struct
{
    mpig_aligned_t vop[(SIZEOF_VMPI_OP + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)];
}
mpig_vmpi_op_t;

typedef struct
{
    mpig_aligned_t vstatus[(SIZEOF_VMPI_STATUS + sizeof(mpig_aligned_t) - 1) / sizeof(mpig_aligned_t)];
}
mpig_vmpi_status_t;

typedef TYPEOF_MPICH2_AINT mpig_vmpi_aint_t;


/*
 * externally accessible symbols containing vendor MPI values
 */
#if !defined(BUILDING_MPIG_VMPI_C)

/* miscellaneous values */
extern const mpig_vmpi_request_t mpig_vmpi_request_null;
#define MPIG_VMPI_REQUEST_NULL (&mpig_vmpi_request_null)
extern const int MPIG_VMPI_ANY_SOURCE;
extern const int MPIG_VMPI_ANY_TAG;
extern void * const MPIG_VMPI_IN_PLACE;
extern const int MPIG_VMPI_MAX_ERROR_STRING;
extern const int MPIG_VMPI_PROC_NULL;
extern mpig_vmpi_status_t * const MPIG_VMPI_STATUS_IGNORE;
extern mpig_vmpi_status_t * const MPIG_VMPI_STATUSES_IGNORE;
extern const int MPIG_VMPI_UNDEFINED;

/* predefined communicators */
#define MPIG_VMPI_COMM_NULL (&mpig_vmpi_comm_null)
extern const mpig_vmpi_comm_t mpig_vmpi_comm_null;
#define MPIG_VMPI_COMM_WORLD (&mpig_vmpi_comm_world)
extern const mpig_vmpi_comm_t mpig_vmpi_comm_world;
#define MPIG_VMPI_COMM_SELF (&mpig_vmpi_comm_self)
extern const mpig_vmpi_comm_t mpig_vmpi_comm_self;

/* predefined datatypes */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_null;
#define MPIG_VMPI_DATATYPE_NULL (&mpig_vmpi_dt_null)
/* c basic datatypes */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_byte;
#define MPIG_VMPI_BYTE (&mpig_vmpi_dt_byte)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_char;
#define MPIG_VMPI_CHAR (&mpig_vmpi_dt_char)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_signed_char;
#define MPIG_VMPI_SIGNED_CHAR (&mpig_vmpi_dt_signed_char)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_char;
#define MPIG_VMPI_UNSIGNED_CHAR (&mpig_vmpi_dt_unsigned_char)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_wchar;
#define MPIG_VMPI_WCHAR (&mpig_vmpi_dt_wchar)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_short;
#define MPIG_VMPI_SHORT (&mpig_vmpi_dt_short)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_short;
#define MPIG_VMPI_UNSIGNED_SHORT (&mpig_vmpi_dt_unsigned_short)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_int;
#define MPIG_VMPI_INT (&mpig_vmpi_dt_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned;
#define MPIG_VMPI_UNSIGNED (&mpig_vmpi_dt_unsigned)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long;
#define MPIG_VMPI_LONG (&mpig_vmpi_dt_long)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long;
#define MPIG_VMPI_UNSIGNED_LONG (&mpig_vmpi_dt_unsigned_long)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long_long;
#define MPIG_VMPI_LONG_LONG (&mpig_vmpi_dt_long_long)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long_long_int;
#define MPIG_VMPI_LONG_LONG_INT (&mpig_vmpi_dt_long_long_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long_long;
#define MPIG_VMPI_UNSIGNED_LONG_LONG (&mpig_vmpi_dt_unsigned_long_long)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_float;
#define MPIG_VMPI_FLOAT (&mpig_vmpi_dt_float)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_double;
#define MPIG_VMPI_DOUBLE (&mpig_vmpi_dt_double)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long_double;
#define MPIG_VMPI_LONG_DOUBLE (&mpig_vmpi_dt_long_double)
/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_short_int;
#define MPIG_VMPI_SHORT_INT (&mpig_vmpi_dt_short_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2int;
#define MPIG_VMPI_2INT (&mpig_vmpi_dt_2int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long_int;
#define MPIG_VMPI_LONG_INT (&mpig_vmpi_dt_long_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_float_int;
#define MPIG_VMPI_FLOAT_INT (&mpig_vmpi_dt_float_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_double_int;
#define MPIG_VMPI_DOUBLE_INT (&mpig_vmpi_dt_double_int)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_long_double_int;
#define MPIG_VMPI_LONG_DOUBLE_INT (&mpig_vmpi_dt_long_double_int)
/* fortran basic datatypes */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_logical;
#define MPIG_VMPI_LOGICAL (&mpig_vmpi_dt_logical)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_character;
#define MPIG_VMPI_CHARACTER (&mpig_vmpi_dt_character)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer;
#define MPIG_VMPI_INTEGER (&mpig_vmpi_dt_integer)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_real;
#define MPIG_VMPI_REAL (&mpig_vmpi_dt_real)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_double_precision;
#define MPIG_VMPI_DOUBLE_PRECISION (&mpig_vmpi_dt_double_precision)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_complex;
#define MPIG_VMPI_COMPLEX (&mpig_vmpi_dt_complex)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_double_complex;
#define MPIG_VMPI_DOUBLE_COMPLEX (&mpig_vmpi_dt_double_complex)
/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2integer;
#define MPIG_VMPI_2INTEGER (&mpig_vmpi_dt_2integer)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2complex;
#define MPIG_VMPI_2COMPLEX (&mpig_vmpi_dt_2complex)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2real;
#define MPIG_VMPI_2REAL (&mpig_vmpi_dt_2real)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2double_complex;
#define MPIG_VMPI_2DOUBLE_COMPLEX (&mpig_vmpi_dt_2double_complex)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_2double_precision;
#define MPIG_VMPI_2DOUBLE_PRECISION (&mpig_vmpi_dt_2double_precision)
/* fortran size specific datatypes */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer1;
#define MPIG_VMPI_INTEGER1 (&mpig_vmpi_dt_integer1)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer2;
#define MPIG_VMPI_INTEGER2 (&mpig_vmpi_dt_integer2)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer4;
#define MPIG_VMPI_INTEGER4 (&mpig_vmpi_dt_integer4)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer8;
#define MPIG_VMPI_INTEGER8 (&mpig_vmpi_dt_integer8)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_integer16;
#define MPIG_VMPI_INTEGER16 (&mpig_vmpi_dt_integer16)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_real4;
#define MPIG_VMPI_REAL4 (&mpig_vmpi_dt_real4)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_real8;
#define MPIG_VMPI_REAL8 (&mpig_vmpi_dt_real8)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_real16;
#define MPIG_VMPI_REAL16 (&mpig_vmpi_dt_real16)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_complex8;
#define MPIG_VMPI_COMPLEX8 (&mpig_vmpi_dt_complex8)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_complex16;
#define MPIG_VMPI_COMPLEX16 (&mpig_vmpi_dt_complex16)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_complex32;
#define MPIG_VMPI_COMPLEX32 (&mpig_vmpi_dt_complex32)
/* type representing a packed user buffer */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_packed;
#define MPIG_VMPI_PACKED (&mpig_vmpi_dt_packed)
/* pseudo datatypes used to manipulate the extent */
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_lb;
#define MPIG_VMPI_LB (&mpig_vmpi_dt_lb)
extern const mpig_vmpi_datatype_t mpig_vmpi_dt_ub;
#define MPIG_VMPI_UB (&mpig_vmpi_dt_ub)

/* collective operations (for MPI_Reduce, etc.) */
extern const mpig_vmpi_op_t mpig_vmpi_op_null;
#define MPIG_VMPI_OP_NULL (&mpig_vmpi_op_null)
extern const mpig_vmpi_op_t mpig_vmpi_op_max;
#define MPIG_VMPI_MAX (&mpig_vmpi_op_max)
extern const mpig_vmpi_op_t mpig_vmpi_op_min;
#define MPIG_VMPI_MIN (&mpig_vmpi_op_min)
extern const mpig_vmpi_op_t mpig_vmpi_op_sum;
#define MPIG_VMPI_SUM (&mpig_vmpi_op_sum)
extern const mpig_vmpi_op_t mpig_vmpi_op_prod;
#define MPIG_VMPI_PROD (&mpig_vmpi_op_prod)
extern const mpig_vmpi_op_t mpig_vmpi_op_land;
#define MPIG_VMPI_LAND (&mpig_vmpi_op_land)
extern const mpig_vmpi_op_t mpig_vmpi_op_band;
#define MPIG_VMPI_BAND (&mpig_vmpi_op_band)
extern const mpig_vmpi_op_t mpig_vmpi_op_lor;
#define MPIG_VMPI_LOR (&mpig_vmpi_op_lor)
extern const mpig_vmpi_op_t mpig_vmpi_op_bor;
#define MPIG_VMPI_BOR (&mpig_vmpi_op_bor)
extern const mpig_vmpi_op_t mpig_vmpi_op_lxor;
#define MPIG_VMPI_LXOR (&mpig_vmpi_op_lxor)
extern const mpig_vmpi_op_t mpig_vmpi_op_bxor;
#define MPIG_VMPI_BXOR (&mpig_vmpi_op_bxor)
extern const mpig_vmpi_op_t mpig_vmpi_op_minloc;
#define MPIG_VMPI_MINLOC (&mpig_vmpi_op_minloc)
extern const mpig_vmpi_op_t mpig_vmpi_op_maxloc;
#define MPIG_VMPI_MAXLOC (&mpig_vmpi_op_maxloc)
extern const mpig_vmpi_op_t mpig_vmpi_op_replace;
#define MPIG_VMPI_REPLACE (&mpig_vmpi_op_replace)

/* error classes */
extern const int MPIG_VMPI_ERR_BUFFER;
extern const int MPIG_VMPI_ERR_COUNT;
extern const int MPIG_VMPI_ERR_TYPE;
extern const int MPIG_VMPI_ERR_TAG;
extern const int MPIG_VMPI_ERR_COMM;
extern const int MPIG_VMPI_ERR_RANK;
extern const int MPIG_VMPI_ERR_ROOT;
extern const int MPIG_VMPI_ERR_TRUNCATE;
extern const int MPIG_VMPI_ERR_GROUP;
extern const int MPIG_VMPI_ERR_OP;
extern const int MPIG_VMPI_ERR_REQUEST;
extern const int MPIG_VMPI_ERR_TOPOLOGY;
extern const int MPIG_VMPI_ERR_DIMS;
extern const int MPIG_VMPI_ERR_ARG;
extern const int MPIG_VMPI_ERR_OTHER;
extern const int MPIG_VMPI_ERR_UNKNOWN;
extern const int MPIG_VMPI_ERR_INTERN;
extern const int MPIG_VMPI_ERR_IN_STATUS;
extern const int MPIG_VMPI_ERR_PENDING;
extern const int MPIG_VMPI_ERR_FILE;
extern const int MPIG_VMPI_ERR_ACCESS;
extern const int MPIG_VMPI_ERR_AMODE;
extern const int MPIG_VMPI_ERR_BAD_FILE;
extern const int MPIG_VMPI_ERR_FILE_EXISTS;
extern const int MPIG_VMPI_ERR_FILE_IN_USE;
extern const int MPIG_VMPI_ERR_NO_SPACE;
extern const int MPIG_VMPI_ERR_NO_SUCH_FILE;
extern const int MPIG_VMPI_ERR_IO;
extern const int MPIG_VMPI_ERR_READ_ONLY;
extern const int MPIG_VMPI_ERR_CONVERSION;
extern const int MPIG_VMPI_ERR_DUP_DATAREP;
extern const int MPIG_VMPI_ERR_UNSUPPORTED_DATAREP;
extern const int MPIG_VMPI_ERR_INFO;
extern const int MPIG_VMPI_ERR_INFO_KEY;
extern const int MPIG_VMPI_ERR_INFO_VALUE;
extern const int MPIG_VMPI_ERR_INFO_NOKEY;
extern const int MPIG_VMPI_ERR_NAME;
extern const int MPIG_VMPI_ERR_NO_MEM;
extern const int MPIG_VMPI_ERR_NOT_SAME;
extern const int MPIG_VMPI_ERR_PORT;
extern const int MPIG_VMPI_ERR_QUOTA;
extern const int MPIG_VMPI_ERR_SERVICE;
extern const int MPIG_VMPI_ERR_SPAWN;
extern const int MPIG_VMPI_ERR_UNSUPPORTED_OPERATION;
extern const int MPIG_VMPI_ERR_WIN;
extern const int MPIG_VMPI_ERR_BASE;
extern const int MPIG_VMPI_ERR_LOCKTYPE;
extern const int MPIG_VMPI_ERR_KEYVAL;
extern const int MPIG_VMPI_ERR_RMA_CONFLICT;
extern const int MPIG_VMPI_ERR_RMA_SYNC;
extern const int MPIG_VMPI_ERR_SIZE;
extern const int MPIG_VMPI_ERR_DISP;
extern const int MPIG_VMPI_ERR_ASSERT;

#endif /* !defined(BUILDING_MPIG_VMPI_C) */

#define MPIG_VMPI_SUCCESS (0)  /* the MPI standard defines MPI_SUCCESS to always be zero */


/*
 * translation functions used to access vendor MPI routines and data structures
 */
int mpig_vmpi_init(int * argc, char *** argv);

int mpig_vmpi_finalize(void);

/* point-to-point functions */
int mpig_vmpi_send(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_isend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p);

int mpig_vmpi_rsend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_irsend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p);

int mpig_vmpi_ssend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_issend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p);

int mpig_vmpi_recv(void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int src, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_status_t * status_p);

int mpig_vmpi_irecv(void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int src, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p);

int mpig_vmpi_cancel(mpig_vmpi_request_t * request_p);

int mpig_vmpi_request_free(mpig_vmpi_request_t * request_p);

int mpig_vmpi_wait(mpig_vmpi_request_t * request_p, mpig_vmpi_status_t * status_p);

int mpig_vmpi_test(mpig_vmpi_request_t * request_p, int * flag_p, mpig_vmpi_status_t * status_p);

int mpig_vmpi_test_cancelled(mpig_vmpi_status_t * status_p, int * flag_p);

int mpig_vmpi_probe(int src, int tag, const mpig_vmpi_comm_t * comm, mpig_vmpi_status_t * status_p);

int mpig_vmpi_iprobe(int src, int tag, const mpig_vmpi_comm_t * comm, int * flag_p, mpig_vmpi_status_t * status_p);

/* collective communication functions */
int mpig_vmpi_bcast(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int root, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_gather(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t * send_dt, void * recv_buf, int recv_cnt,
    const mpig_vmpi_datatype_t * recv_dt, int root, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_gatherv(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t * send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, const mpig_vmpi_datatype_t * recv_dt, int root, const mpig_vmpi_comm_t * comm);

int mpig_vmpi_allreduce(const void * send_buf, void * recv_buf, int cnt, const mpig_vmpi_datatype_t * dt,
    const mpig_vmpi_op_t * op, const mpig_vmpi_comm_t * comm);

/* communicator functions */
int mpig_vmpi_comm_size(const mpig_vmpi_comm_t * comm, int * size_p);

int mpig_vmpi_comm_rank(const mpig_vmpi_comm_t * comm, int * rank_p);

int mpig_vmpi_comm_dup(const mpig_vmpi_comm_t * old_comm, mpig_vmpi_comm_t * new_comm_p);

int mpig_vmpi_comm_split(const mpig_vmpi_comm_t * old_comm, int color, int key, mpig_vmpi_comm_t * new_comm_p);

int mpig_vmpi_intercomm_create(const mpig_vmpi_comm_t * local_comm, int local_leader, const mpig_vmpi_comm_t * peer_comm,
    int remote_leader, int tag, mpig_vmpi_comm_t * new_intercomm_p);

int mpig_vmpi_intercomm_merge(const mpig_vmpi_comm_t * intercomm, int high, mpig_vmpi_comm_t * new_intracomm_p);

int mpig_vmpi_comm_free(mpig_vmpi_comm_t * comm_p);

/* datatype functions */
int mpig_vmpi_type_commit(mpig_vmpi_datatype_t dt_p);

int mpig_vmpi_type_free(mpig_vmpi_datatype_t * dt_p);

int mpig_vmpi_type_contiguous(int cnt, const mpig_vmpi_datatype_t * old_dt, mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_hvector(int cnt, int blocklength, mpig_vmpi_aint_t stride, const mpig_vmpi_datatype_t * old_dt,
    mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_hindexed(int cnt, int * blocklengths, mpig_vmpi_aint_t * displacements, const mpig_vmpi_datatype_t * old_dt,
    mpig_vmpi_datatype_t * new_dt);

int mpig_vmpi_type_struct(int cnt, int * blocklengths, mpig_vmpi_aint_t * displacements, const mpig_vmpi_datatype_t ** old_dts,
    mpig_vmpi_datatype_t * new_dt);

int mpig_vmpi_get_count(mpig_vmpi_status_t * status_p, const mpig_vmpi_datatype_t * dt, int * cnt_p);

int mpig_vmpi_get_elements(mpig_vmpi_status_t * status_p, const mpig_vmpi_datatype_t * dt, int * elements_p);

/* error extraction and conversion functions */
int mpig_vmpi_error_class(int vendor_errno, int * vendor_class_p);

int mpig_vmpi_error_string(int vendor_errno, char * string, int * result_length);


/*
 * utility functions that are not defined in the MPI standard but are needed to transition between MPICH2 and the vendor MPI
 */
int mpig_vmpi_status_get_source(const mpig_vmpi_status_t * status_p);

int mpig_vmpi_status_get_tag(const mpig_vmpi_status_t * status_p);

int mpig_vmpi_status_get_error(const mpig_vmpi_status_t * status_p);

int mpig_vmpi_comm_is_null(const mpig_vmpi_comm_t * comm);

int mpig_vmpi_datatype_is_null(const mpig_vmpi_datatype_t * dt);

int mpig_vmpi_request_is_null(const mpig_vmpi_request_t * req);


#endif /* !defined(MPICH2_MPIG_VMPI_H_INCLUDED) */
