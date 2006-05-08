/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidconf.h"

#if defined(MPIG_VMPI)

#define BUILDING_MPIG_VMPI_C


/* NOTE: TRUE and FALSE values must match the values set in mpidpre.h.  the defintion of mpig_aligned_t must also match the value
   defined in mpidpre.h. */
#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

typedef TYPEOF_MPIG_ALIGNED_T mpig_aligned_t;


#include "mpig_vmpi.h"
#include "mpi.h"


/*
 * symbols for exporting predefined vendor MPI handles and values
 */
mpig_vmpi_request_t mpig_vmpi_request_null;
int MPIG_VMPI_ANY_SOURCE;
int MPIG_VMPI_ANY_TAG;
void * MPIG_VMPI_IN_PLACE;
int MPIG_VMPI_MAX_ERROR_STRING;
int MPIG_VMPI_PROC_NULL;
mpig_vmpi_status_t * MPIG_VMPI_STATUS_IGNORE;
mpig_vmpi_status_t * MPIG_VMPI_STATUSES_IGNORE;
int MPIG_VMPI_UNDEFINED;

/* predefined communicators */
mpig_vmpi_comm_t mpig_vmpi_comm_null;
mpig_vmpi_comm_t mpig_vmpi_comm_world;
mpig_vmpi_comm_t mpig_vmpi_comm_self;

/* predefined datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_null;
/* c basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_byte;
mpig_vmpi_datatype_t mpig_vmpi_dt_char;
mpig_vmpi_datatype_t mpig_vmpi_dt_signed_char;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_char;
mpig_vmpi_datatype_t mpig_vmpi_dt_wchar;
mpig_vmpi_datatype_t mpig_vmpi_dt_short;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_short;
mpig_vmpi_datatype_t mpig_vmpi_dt_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned;
mpig_vmpi_datatype_t mpig_vmpi_dt_long;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long_long;
mpig_vmpi_datatype_t mpig_vmpi_dt_float;
mpig_vmpi_datatype_t mpig_vmpi_dt_double;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double;
/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_short_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_2int;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_float_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_int;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double_int;
/* fortran basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_logical;
mpig_vmpi_datatype_t mpig_vmpi_dt_character;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer;
mpig_vmpi_datatype_t mpig_vmpi_dt_real;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_precision;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_complex;
/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_2integer;
mpig_vmpi_datatype_t mpig_vmpi_dt_2complex;
mpig_vmpi_datatype_t mpig_vmpi_dt_2real;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_complex;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_precision;
/* fortran size specific datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_integer1;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer2;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer4;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer8;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer16;
mpig_vmpi_datatype_t mpig_vmpi_dt_real4;
mpig_vmpi_datatype_t mpig_vmpi_dt_real8;
mpig_vmpi_datatype_t mpig_vmpi_dt_real16;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex8;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex16;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex32;
/* type representing a packed user buffer */
mpig_vmpi_datatype_t mpig_vmpi_dt_packed;
/* pseudo datatypes used to manipulate the extent */
mpig_vmpi_datatype_t mpig_vmpi_dt_lb;
mpig_vmpi_datatype_t mpig_vmpi_dt_ub;

/* collective operations (for MPI_Reduce, etc.) */
mpig_vmpi_op_t mpig_vmpi_op_null;
mpig_vmpi_op_t mpig_vmpi_op_max;
mpig_vmpi_op_t mpig_vmpi_op_min;
mpig_vmpi_op_t mpig_vmpi_op_sum;
mpig_vmpi_op_t mpig_vmpi_op_prod;
mpig_vmpi_op_t mpig_vmpi_op_land;
mpig_vmpi_op_t mpig_vmpi_op_band;
mpig_vmpi_op_t mpig_vmpi_op_lor;
mpig_vmpi_op_t mpig_vmpi_op_bor;
mpig_vmpi_op_t mpig_vmpi_op_lxor;
mpig_vmpi_op_t mpig_vmpi_op_bxor;
mpig_vmpi_op_t mpig_vmpi_op_minloc;
mpig_vmpi_op_t mpig_vmpi_op_maxloc;
mpig_vmpi_op_t mpig_vmpi_op_replace;

/* predefined error classes */
int MPIG_VMPI_ERR_BUFFER;
int MPIG_VMPI_ERR_COUNT;
int MPIG_VMPI_ERR_TYPE;
int MPIG_VMPI_ERR_TAG;
int MPIG_VMPI_ERR_COMM;
int MPIG_VMPI_ERR_RANK;
int MPIG_VMPI_ERR_ROOT;
int MPIG_VMPI_ERR_TRUNCATE;
int MPIG_VMPI_ERR_GROUP;
int MPIG_VMPI_ERR_OP;
int MPIG_VMPI_ERR_REQUEST;
int MPIG_VMPI_ERR_TOPOLOGY;
int MPIG_VMPI_ERR_DIMS;
int MPIG_VMPI_ERR_ARG;
int MPIG_VMPI_ERR_OTHER;
int MPIG_VMPI_ERR_UNKNOWN;
int MPIG_VMPI_ERR_INTERN;
int MPIG_VMPI_ERR_IN_STATUS;
int MPIG_VMPI_ERR_PENDING;
int MPIG_VMPI_ERR_FILE;
int MPIG_VMPI_ERR_ACCESS;
int MPIG_VMPI_ERR_AMODE;
int MPIG_VMPI_ERR_BAD_FILE;
int MPIG_VMPI_ERR_FILE_EXISTS;
int MPIG_VMPI_ERR_FILE_IN_USE;
int MPIG_VMPI_ERR_NO_SPACE;
int MPIG_VMPI_ERR_NO_SUCH_FILE;
int MPIG_VMPI_ERR_IO;
int MPIG_VMPI_ERR_READ_ONLY;
int MPIG_VMPI_ERR_CONVERSION;
int MPIG_VMPI_ERR_DUP_DATAREP;
int MPIG_VMPI_ERR_UNSUPPORTED_DATAREP;
int MPIG_VMPI_ERR_INFO;
int MPIG_VMPI_ERR_INFO_KEY;
int MPIG_VMPI_ERR_INFO_VALUE;
int MPIG_VMPI_ERR_INFO_NOKEY;
int MPIG_VMPI_ERR_NAME;
int MPIG_VMPI_ERR_NO_MEM;
int MPIG_VMPI_ERR_NOT_SAME;
int MPIG_VMPI_ERR_PORT;
int MPIG_VMPI_ERR_QUOTA;
int MPIG_VMPI_ERR_SERVICE;
int MPIG_VMPI_ERR_SPAWN;
int MPIG_VMPI_ERR_UNSUPPORTED_OPERATION;
int MPIG_VMPI_ERR_WIN;
int MPIG_VMPI_ERR_BASE;
int MPIG_VMPI_ERR_LOCKTYPE;
int MPIG_VMPI_ERR_KEYVAL;
int MPIG_VMPI_ERR_RMA_CONFLICT;
int MPIG_VMPI_ERR_RMA_SYNC;
int MPIG_VMPI_ERR_SIZE;
int MPIG_VMPI_ERR_DISP;
int MPIG_VMPI_ERR_ASSERT;


/*
 * miscellaneous internal variable and function declarations;
 */
int mpig_vmpi_call_mpi_finalize = FALSE;
int mpig_vmpi_module_ref_count = 0;


/*
 * mpig_vmpi_init(argc_p, argv_p)
 */
int mpig_vmpi_init(int * argc_p, char *** argv_p)
{
    int flag;
    int vrc = MPI_SUCCESS;

    /* call the vendor implementation of MPI_Init(), but only if another library/module hasn't already called MPI_Init().  see
       the comments in mpig_vmpi_finalize() for a more detailed description of the problem. */
    vrc = MPI_Initialized(&flag);
    if (vrc) goto fn_fail;

    if (flag == FALSE)
    {
	vrc = MPI_Init(argc_p, argv_p);
	if (vrc) goto fn_fail;

	mpig_vmpi_call_mpi_finalize = TRUE;
    }

    if (mpig_vmpi_module_ref_count++ > 0) goto fn_return;
    
    /* set the error handlers for the predefined communicators so that all MPI functions return error codes */
    vrc = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    vrc = MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);

    /* copy miscellaneous vendor MPI symbols into externally visible constants */
    *(MPI_Request *) &mpig_vmpi_request_null = MPI_REQUEST_NULL;
    
    MPIG_VMPI_ANY_SOURCE = MPI_ANY_SOURCE;
    MPIG_VMPI_ANY_TAG = MPI_ANY_TAG;
    MPIG_VMPI_IN_PLACE = (void *) MPI_IN_PLACE;
    MPIG_VMPI_MAX_ERROR_STRING = MPI_MAX_ERROR_STRING;
    MPIG_VMPI_PROC_NULL = MPI_PROC_NULL;
    MPIG_VMPI_STATUS_IGNORE = (mpig_vmpi_status_t *) MPI_STATUS_IGNORE;
    MPIG_VMPI_STATUSES_IGNORE = (mpig_vmpi_status_t *) MPI_STATUSES_IGNORE;
    MPIG_VMPI_UNDEFINED = MPI_UNDEFINED;
    
    /* copy the vendor MPI predefined communicator handles into externally visible constants */
    *(MPI_Comm *) &mpig_vmpi_comm_null = MPI_COMM_NULL;
    *(MPI_Comm *) &mpig_vmpi_comm_world = MPI_COMM_WORLD;
    *(MPI_Comm *) &mpig_vmpi_comm_self = MPI_COMM_SELF;

    /* copy the vendor MPI predefined datatype handles into externally visible constants */
    *(MPI_Datatype *) &mpig_vmpi_dt_null = MPI_DATATYPE_NULL;
    /* c basic datatypes */
    *(MPI_Datatype *) &mpig_vmpi_dt_byte = MPI_BYTE;
    *(MPI_Datatype *) &mpig_vmpi_dt_char = MPI_CHAR;
    *(MPI_Datatype *) &mpig_vmpi_dt_signed_char = MPI_SIGNED_CHAR;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_char = MPI_UNSIGNED_CHAR;
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_WCHAR;
    *(MPI_Datatype *) &mpig_vmpi_dt_short = MPI_SHORT;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_short = MPI_UNSIGNED_SHORT;
    *(MPI_Datatype *) &mpig_vmpi_dt_int = MPI_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned = MPI_UNSIGNED;
    *(MPI_Datatype *) &mpig_vmpi_dt_long = MPI_LONG;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long = MPI_UNSIGNED_LONG;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long = MPI_LONG_LONG;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long_int = MPI_LONG_LONG_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long_long = MPI_UNSIGNED_LONG_LONG;
    *(MPI_Datatype *) &mpig_vmpi_dt_float = MPI_FLOAT;
    *(MPI_Datatype *) &mpig_vmpi_dt_double = MPI_DOUBLE;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double = MPI_LONG_DOUBLE;
    /* c paired datatypes used predominantly for minloc/maxloc reduce operations */
    *(MPI_Datatype *) &mpig_vmpi_dt_short_int = MPI_SHORT_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_2int = MPI_2INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_int = MPI_LONG_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_float_int = MPI_FLOAT_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_double_int = MPI_DOUBLE_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double_int = MPI_LONG_DOUBLE_INT;
    /* fortran basic datatypes */
    *(MPI_Datatype *) &mpig_vmpi_dt_logical = MPI_LOGICAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_character = MPI_CHARACTER;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer = MPI_INTEGER;
    *(MPI_Datatype *) &mpig_vmpi_dt_real = MPI_REAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_double_precision = MPI_DOUBLE_PRECISION;
    *(MPI_Datatype *) &mpig_vmpi_dt_complex = MPI_COMPLEX;
    *(MPI_Datatype *) &mpig_vmpi_dt_double_complex = MPI_DOUBLE_COMPLEX;
    /* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
    *(MPI_Datatype *) &mpig_vmpi_dt_2integer = MPI_2INTEGER;
    *(MPI_Datatype *) &mpig_vmpi_dt_2complex = MPI_2COMPLEX;
    *(MPI_Datatype *) &mpig_vmpi_dt_2real = MPI_2REAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_2double_complex = MPI_2DOUBLE_COMPLEX;
    *(MPI_Datatype *) &mpig_vmpi_dt_2double_precision = MPI_2DOUBLE_PRECISION;
    /* fortran size specific datatypes */
    *(MPI_Datatype *) &mpig_vmpi_dt_integer1 = MPI_INTEGER1;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer2 = MPI_INTEGER2;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer4 = MPI_INTEGER4;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer8 = MPI_INTEGER8;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer16 = MPI_INTEGER16;
    *(MPI_Datatype *) &mpig_vmpi_dt_real4 = MPI_REAL4;
    *(MPI_Datatype *) &mpig_vmpi_dt_real8 = MPI_REAL8;
    *(MPI_Datatype *) &mpig_vmpi_dt_real16 = MPI_REAL16;
    *(MPI_Datatype *) &mpig_vmpi_dt_complex8 = MPI_COMPLEX8;
    *(MPI_Datatype *) &mpig_vmpi_dt_complex16 = MPI_COMPLEX16;
    *(MPI_Datatype *) &mpig_vmpi_dt_complex32 = MPI_COMPLEX32;
    /* type representing a packed user buffer */
    *(MPI_Datatype *) &mpig_vmpi_dt_packed = MPI_PACKED;
    /* pseudo datatypes used to manipulate the extent */
    *(MPI_Datatype *) &mpig_vmpi_dt_lb = MPI_LB;
    *(MPI_Datatype *) &mpig_vmpi_dt_ub = MPI_UB;

    /* copy the vendor collective operations (for MPI_Reduce, etc.) */
    *(MPI_Op *) &mpig_vmpi_op_null = MPI_OP_NULL;
    *(MPI_Op *) &mpig_vmpi_op_max = MPI_MAX;
    *(MPI_Op *) &mpig_vmpi_op_min = MPI_MIN;
    *(MPI_Op *) &mpig_vmpi_op_sum = MPI_SUM;
    *(MPI_Op *) &mpig_vmpi_op_prod = MPI_PROD;
    *(MPI_Op *) &mpig_vmpi_op_land = MPI_LAND;
    *(MPI_Op *) &mpig_vmpi_op_band = MPI_BAND;
    *(MPI_Op *) &mpig_vmpi_op_lor = MPI_LOR;
    *(MPI_Op *) &mpig_vmpi_op_bor = MPI_BOR;
    *(MPI_Op *) &mpig_vmpi_op_lxor = MPI_LXOR;
    *(MPI_Op *) &mpig_vmpi_op_bxor =  MPI_BXOR;
    *(MPI_Op *) &mpig_vmpi_op_minloc = MPI_MINLOC;
    *(MPI_Op *) &mpig_vmpi_op_maxloc = MPI_MAXLOC;
    *(MPI_Op *) &mpig_vmpi_op_replace = MPI_REPLACE;
    
    /* copy the vendor MPI predefined error class values into the externally visible constants */
    MPIG_VMPI_ERR_BUFFER = MPI_ERR_BUFFER;
    MPIG_VMPI_ERR_COUNT = MPI_ERR_COUNT;
    MPIG_VMPI_ERR_TYPE = MPI_ERR_TYPE;
    MPIG_VMPI_ERR_TAG = MPI_ERR_TAG;
    MPIG_VMPI_ERR_COMM = MPI_ERR_COMM;
    MPIG_VMPI_ERR_RANK = MPI_ERR_RANK;
    MPIG_VMPI_ERR_ROOT = MPI_ERR_ROOT;
    MPIG_VMPI_ERR_TRUNCATE = MPI_ERR_TRUNCATE;
    MPIG_VMPI_ERR_GROUP = MPI_ERR_GROUP;
    MPIG_VMPI_ERR_OP = MPI_ERR_OP;
    MPIG_VMPI_ERR_REQUEST = MPI_ERR_REQUEST;
    MPIG_VMPI_ERR_TOPOLOGY = MPI_ERR_TOPOLOGY;
    MPIG_VMPI_ERR_DIMS = MPI_ERR_DIMS;
    MPIG_VMPI_ERR_ARG = MPI_ERR_ARG;
    MPIG_VMPI_ERR_OTHER = MPI_ERR_OTHER;
    MPIG_VMPI_ERR_UNKNOWN = MPI_ERR_UNKNOWN;
    MPIG_VMPI_ERR_INTERN = MPI_ERR_INTERN;
    MPIG_VMPI_ERR_IN_STATUS = MPI_ERR_IN_STATUS;
    MPIG_VMPI_ERR_PENDING = MPI_ERR_PENDING;
    MPIG_VMPI_ERR_FILE = MPI_ERR_FILE;
    MPIG_VMPI_ERR_ACCESS = MPI_ERR_ACCESS;
    MPIG_VMPI_ERR_AMODE = MPI_ERR_AMODE;
    MPIG_VMPI_ERR_BAD_FILE = MPI_ERR_BAD_FILE;
    MPIG_VMPI_ERR_FILE_EXISTS = MPI_ERR_FILE_EXISTS;
    MPIG_VMPI_ERR_FILE_IN_USE = MPI_ERR_FILE_IN_USE;
    MPIG_VMPI_ERR_NO_SPACE = MPI_ERR_NO_SPACE;
    MPIG_VMPI_ERR_NO_SUCH_FILE = MPI_ERR_NO_SUCH_FILE;
    MPIG_VMPI_ERR_IO = MPI_ERR_IO;
    MPIG_VMPI_ERR_READ_ONLY = MPI_ERR_READ_ONLY;
    MPIG_VMPI_ERR_CONVERSION = MPI_ERR_CONVERSION;
    MPIG_VMPI_ERR_DUP_DATAREP = MPI_ERR_DUP_DATAREP;
    MPIG_VMPI_ERR_UNSUPPORTED_DATAREP = MPI_ERR_UNSUPPORTED_DATAREP;
    MPIG_VMPI_ERR_INFO = MPI_ERR_INFO;
    MPIG_VMPI_ERR_INFO_KEY = MPI_ERR_INFO_KEY;
    MPIG_VMPI_ERR_INFO_VALUE = MPI_ERR_INFO_VALUE;
    MPIG_VMPI_ERR_INFO_NOKEY = MPI_ERR_INFO_NOKEY;
    MPIG_VMPI_ERR_NAME = MPI_ERR_NAME;
    MPIG_VMPI_ERR_NO_MEM = MPI_ERR_NO_MEM;
    MPIG_VMPI_ERR_NOT_SAME = MPI_ERR_NOT_SAME;
    MPIG_VMPI_ERR_PORT = MPI_ERR_PORT;
    MPIG_VMPI_ERR_QUOTA = MPI_ERR_QUOTA;
    MPIG_VMPI_ERR_SERVICE = MPI_ERR_SERVICE;
    MPIG_VMPI_ERR_SPAWN = MPI_ERR_SPAWN;
    MPIG_VMPI_ERR_UNSUPPORTED_OPERATION = MPI_ERR_UNSUPPORTED_OPERATION;
    MPIG_VMPI_ERR_WIN = MPI_ERR_WIN;
    MPIG_VMPI_ERR_BASE = MPI_ERR_BASE;
    MPIG_VMPI_ERR_LOCKTYPE = MPI_ERR_LOCKTYPE;
    MPIG_VMPI_ERR_KEYVAL = MPI_ERR_KEYVAL;
    MPIG_VMPI_ERR_RMA_CONFLICT = MPI_ERR_RMA_CONFLICT;
    MPIG_VMPI_ERR_RMA_SYNC = MPI_ERR_RMA_SYNC;
    MPIG_VMPI_ERR_SIZE = MPI_ERR_SIZE;
    MPIG_VMPI_ERR_DISP = MPI_ERR_DISP;
    MPIG_VMPI_ERR_ASSERT = MPI_ERR_ASSERT;

  fn_return:
    return vrc;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

int mpig_vmpi_finalize(void)
{
    int vrc = MPIG_VMPI_SUCCESS;

    mpig_vmpi_module_ref_count -= 1;
    if (mpig_vmpi_module_ref_count == 0)
    {
	/*
	 * call the vendor implementation of MPI_Finalize(), but only if we also called MPI_Init().  if some other library/module
	 * called MPI_Init(), then we should let them decide when to call MPI_Finalize().
	 *
	 * this is particularily important for globus/nexus which delays calling MPI_Finalize() until exit() is called.  nexus does
	 * this so that it can be activated and deactivated multiple times, something MPI can't handle.  also, nexus keeps an
	 * outstanding receive posted until exit() is called, and calling MPI_Finalize() before that receive is cancelled causes
	 * some implementations (ex: SGI) to hang.
	 */
	if (mpig_vmpi_call_mpi_finalize)
	{
	    vrc = MPI_Finalize();
	    if (vrc) goto fn_fail;
	}
    }

  fn_return:
    return vrc;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}


/*
 * point-to-point functions
 */
int mpig_vmpi_send(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm)
{
    return MPI_Send((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm);
}

int mpig_vmpi_isend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p)
{
    return MPI_Isend((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm, (MPI_Request *) request_p);
}

int mpig_vmpi_rsend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm)
{
    return MPI_Rsend((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm);
}

int mpig_vmpi_irsend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p)
{
    return MPI_Irsend((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm, (MPI_Request *) request_p);
}

int mpig_vmpi_ssend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm)
{
    return MPI_Ssend((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm);
}

int mpig_vmpi_issend(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int dest, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p)
{
    return MPI_Issend((void *) buf, cnt, *(MPI_Datatype *) dt, dest, tag, *(MPI_Comm *) comm, (MPI_Request *) request_p);
}

int mpig_vmpi_recv(void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int src, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_status_t * status_p)
{
    return MPI_Recv(buf, cnt, *(MPI_Datatype *) dt, src, tag, *(MPI_Comm *) comm, (MPI_Status *) status_p);
}

int mpig_vmpi_irecv(void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int src, int tag, const mpig_vmpi_comm_t * comm,
    mpig_vmpi_request_t * request_p)
{
    return MPI_Irecv(buf, cnt, *(MPI_Datatype *) dt, src, tag, *(MPI_Comm *) comm, (MPI_Request *) request_p);
}

int mpig_vmpi_cancel(mpig_vmpi_request_t * request_p)
{
    return MPI_Cancel((MPI_Request *) request_p);
}

int mpig_vmpi_request_free(mpig_vmpi_request_t * request_p)
{
    return MPI_Request_free((MPI_Request *) request_p);
}

int mpig_vmpi_wait(mpig_vmpi_request_t * request_p, mpig_vmpi_status_t * status_p)
{
    return MPI_Wait((MPI_Request *) request_p, (MPI_Status *) status_p);
}

int mpig_vmpi_test(mpig_vmpi_request_t * request_p, int * flag_p, mpig_vmpi_status_t * status_p)
{
    return MPI_Test((MPI_Request *) request_p, flag_p, (MPI_Status *) status_p);
}

int mpig_vmpi_test_cancelled(mpig_vmpi_status_t * status_p, int * flag_p)
{
    return MPI_Test_cancelled((MPI_Status *) status_p, flag_p);
}

int mpig_vmpi_probe(int src, int tag, const mpig_vmpi_comm_t * comm, mpig_vmpi_status_t * status_p)
{
    return MPI_Probe(src, tag, *(MPI_Comm *) comm, (MPI_Status *) status_p);
}

int mpig_vmpi_iprobe(int src, int tag, const mpig_vmpi_comm_t * comm, int * flag_p, mpig_vmpi_status_t * status_p)
{
    return MPI_Iprobe(src, tag, *(MPI_Comm *) comm, flag_p, (MPI_Status *) status_p);
}


/*
 * broadcast functions
 */
int mpig_vmpi_bcast(const void * buf, int cnt, const mpig_vmpi_datatype_t * dt, int root, const mpig_vmpi_comm_t * comm)
{
    return MPI_Bcast((void *) buf, cnt, *(MPI_Datatype *) dt, root, *(MPI_Comm *) comm);
}

int mpig_vmpi_gather(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t * send_dt, void * recv_buf, int recv_cnt,
    const mpig_vmpi_datatype_t * recv_dt, int root, const mpig_vmpi_comm_t * comm)
{
    return MPI_Gather((void *) send_buf, send_cnt, *(MPI_Datatype *) send_dt, recv_buf, recv_cnt, *(MPI_Datatype *) recv_dt, root,
	*(MPI_Comm *) comm);
}

int mpig_vmpi_gatherv(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t * send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, const mpig_vmpi_datatype_t * recv_dt, int root, const mpig_vmpi_comm_t * comm)
{
    return MPI_Gatherv((void *) send_buf, send_cnt, *(MPI_Datatype *) send_dt, recv_buf, (int *) recv_cnts, (int *) recv_displs,
	*(MPI_Datatype *) recv_dt, root, *(MPI_Comm *) comm);
}

int mpig_vmpi_allreduce(const void * send_buf, void * recv_buf, int cnt, const mpig_vmpi_datatype_t * dt,
    const mpig_vmpi_op_t * op, const mpig_vmpi_comm_t * comm)
{
    return MPI_Allreduce((void *) send_buf, recv_buf, cnt, *(MPI_Datatype *) dt, *(MPI_Op *) op, *(MPI_Comm *) comm);
}


/*
 * communicator functions
 */
int mpig_vmpi_comm_size(const mpig_vmpi_comm_t * comm, int * size_p)
{
    return MPI_Comm_size(*(MPI_Comm *) comm, size_p);
}

int mpig_vmpi_comm_rank(const mpig_vmpi_comm_t * comm, int * rank_p)
{
    return MPI_Comm_rank(*(MPI_Comm *) comm, rank_p);
}

int mpig_vmpi_comm_dup(const mpig_vmpi_comm_t * old_comm, mpig_vmpi_comm_t * new_comm_p)
{
    return MPI_Comm_dup(*(MPI_Comm *) old_comm, (MPI_Comm *) new_comm_p);
}

int mpig_vmpi_comm_split(const mpig_vmpi_comm_t * old_comm, int color, int key, mpig_vmpi_comm_t * new_comm_p)
{
    return MPI_Comm_split(*(MPI_Comm *) old_comm, color, key, (MPI_Comm *) new_comm_p);
}

int mpig_vmpi_intercomm_create(const mpig_vmpi_comm_t * local_comm, int local_leader, const mpig_vmpi_comm_t * peer_comm,
    int remote_leader, int tag, mpig_vmpi_comm_t * new_intercomm_p)
{
    return MPI_Intercomm_create(*(MPI_Comm *) local_comm, local_leader, *(MPI_Comm *) peer_comm, remote_leader, tag,
	(MPI_Comm *) new_intercomm_p);
}

int mpig_vmpi_intercomm_merge(const mpig_vmpi_comm_t * intercomm, int high, mpig_vmpi_comm_t * new_intracomm_p)
{
    return MPI_Intercomm_merge(*(MPI_Comm *) intercomm, high, (MPI_Comm *) new_intracomm_p);
}

int mpig_vmpi_comm_free(mpig_vmpi_comm_t * comm_p)
{
    return MPI_Comm_free((MPI_Comm *) comm_p);
}


/*
 * datatype functions
 */
int mpig_vmpi_get_count(mpig_vmpi_status_t * const status, const mpig_vmpi_datatype_t * const dt, int * const count)
{
    return MPI_Get_count((MPI_Status *) status, *(MPI_Datatype *) dt, count);
}


/*
 * error extraction and conversion functions
 */
int mpig_vmpi_error_class(int vendor_errno, int * vendor_class_p)
{
    return MPI_Error_class(vendor_errno, vendor_class_p);
}

int mpig_vmpi_error_string(int vendor_errno, char * string, int * result_length)
{
    return MPI_Error_string(vendor_errno, string, result_length);
}


/*
 * utility functions not defined in the MPI standard but needed to transition between MPICH2 and the vendor MPI
 */
int mpig_vmpi_status_get_source(const mpig_vmpi_status_t * status_p)
{
    return ((MPI_Status *) status_p)->MPI_SOURCE;
}

int mpig_vmpi_status_get_tag(const mpig_vmpi_status_t * status_p)
{
    return ((MPI_Status *) status_p)->MPI_TAG;
}

int mpig_vmpi_status_get_error(const mpig_vmpi_status_t * status_p)
{
    return ((MPI_Status *) status_p)->MPI_ERROR;
}

int mpig_vmpi_comm_is_null(const mpig_vmpi_comm_t * comm)
{
    return ((*(MPI_Comm *) comm) == MPI_COMM_NULL) ? TRUE : FALSE;
}

int mpig_vmpi_datatype_is_null(const mpig_vmpi_datatype_t * dt)
{
    return ((*(MPI_Datatype *) dt) == MPI_DATATYPE_NULL) ? TRUE : FALSE;
}

int mpig_vmpi_request_is_null(const mpig_vmpi_request_t * req)
{
    return ((*(MPI_Request *) req) == MPI_REQUEST_NULL) ? TRUE : FALSE;
}


#endif /* defined(MPIG_VMPI) */
