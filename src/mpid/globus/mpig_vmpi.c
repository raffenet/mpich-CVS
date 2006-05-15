/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidconf.h"

#if defined(MPIG_VMPI)
#include <stdlib.h>


/* VERY-IMPORTANT-NOTE: TRUE and FALSE values must match the values set in mpidpre.h.  the defintion of mpig_aligned_t must also
   match the value defined in mpidpre.h. */
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
mpig_vmpi_request_t mpig_vmpi_request_null = MPIG_VMPI_REQUEST_INITIALIZER;
int mpig_vmpi_any_source = -1;
int mpig_vmpi_any_tag = -1;
void * mpig_vmpi_in_place = NULL;
int mpig_vmpi_max_error_string = -1;
int mpig_vmpi_proc_null = -1;
mpig_vmpi_status_t * mpig_vmpi_status_ignore = NULL;
mpig_vmpi_status_t * mpig_vmpi_statuses_ignore = NULL;
int mpig_vmpi_undefined = -1;

/* predefined communicators */
mpig_vmpi_comm_t mpig_vmpi_comm_null = MPIG_VMPI_COMM_INITIALIZER;
mpig_vmpi_comm_t mpig_vmpi_comm_world = MPIG_VMPI_COMM_INITIALIZER;
mpig_vmpi_comm_t mpig_vmpi_comm_self = MPIG_VMPI_COMM_INITIALIZER;

/* predefined datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_null = MPIG_VMPI_DATATYPE_INITIALIZER;
/* c basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_byte = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_signed_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_wchar = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_short = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_short = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_float = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double = MPIG_VMPI_DATATYPE_INITIALIZER;
/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_short_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_float_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double_int = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_logical = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_character = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_precision = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_2integer = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2real = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_precision = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran size specific datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_integer1 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer2 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer4 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real4 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex32 = MPIG_VMPI_DATATYPE_INITIALIZER;
/* type representing a packed user buffer */
mpig_vmpi_datatype_t mpig_vmpi_dt_packed = MPIG_VMPI_DATATYPE_INITIALIZER;
/* pseudo datatypes used to manipulate the extent */
mpig_vmpi_datatype_t mpig_vmpi_dt_lb = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_ub = MPIG_VMPI_DATATYPE_INITIALIZER;

/* collective operations (for MPI_Reduce, etc.) */
mpig_vmpi_op_t mpig_vmpi_op_null = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_max = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_min = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_sum = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_prod = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_land = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_band = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_lor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_bor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_lxor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_bxor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_minloc = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_maxloc = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_replace = MPIG_VMPI_OP_INITIALIZER;

/* predefined error classes */
int mpig_vmpi_err_buffer = -1;
int mpig_vmpi_err_count = -1;
int mpig_vmpi_err_type = -1;
int mpig_vmpi_err_tag = -1;
int mpig_vmpi_err_comm = -1;
int mpig_vmpi_err_rank = -1;
int mpig_vmpi_err_root = -1;
int mpig_vmpi_err_truncate = -1;
int mpig_vmpi_err_group = -1;
int mpig_vmpi_err_op = -1;
int mpig_vmpi_err_request = -1;
int mpig_vmpi_err_topology = -1;
int mpig_vmpi_err_dims = -1;
int mpig_vmpi_err_arg = -1;
int mpig_vmpi_err_other = -1;
int mpig_vmpi_err_unknown = -1;
int mpig_vmpi_err_intern = -1;
int mpig_vmpi_err_in_status = -1;
int mpig_vmpi_err_pending = -1;
int mpig_vmpi_err_file = -1;
int mpig_vmpi_err_access = -1;
int mpig_vmpi_err_amode = -1;
int mpig_vmpi_err_bad_file = -1;
int mpig_vmpi_err_file_exists = -1;
int mpig_vmpi_err_file_in_use = -1;
int mpig_vmpi_err_no_space = -1;
int mpig_vmpi_err_no_such_file = -1;
int mpig_vmpi_err_io = -1;
int mpig_vmpi_err_read_only = -1;
int mpig_vmpi_err_conversion = -1;
int mpig_vmpi_err_dup_datarep = -1;
int mpig_vmpi_err_unsupported_datarep = -1;
int mpig_vmpi_err_info = -1;
int mpig_vmpi_err_info_key = -1;
int mpig_vmpi_err_info_value = -1;
int mpig_vmpi_err_info_nokey = -1;
int mpig_vmpi_err_name = -1;
int mpig_vmpi_err_no_mem = -1;
int mpig_vmpi_err_not_same = -1;
int mpig_vmpi_err_port = -1;
int mpig_vmpi_err_quota = -1;
int mpig_vmpi_err_service = -1;
int mpig_vmpi_err_spawn = -1;
int mpig_vmpi_err_unsupported_operation = -1;
int mpig_vmpi_err_win = -1;
int mpig_vmpi_err_base = -1;
int mpig_vmpi_err_locktype = -1;
int mpig_vmpi_err_keyval = -1;
int mpig_vmpi_err_rma_conflict = -1;
int mpig_vmpi_err_rma_sync = -1;
int mpig_vmpi_err_size = -1;
int mpig_vmpi_err_disp = -1;
int mpig_vmpi_err_assert = -1;


/*
 * miscellaneous internal variable and function declarations;
 */
int mpig_vmpi_call_mpi_finalize = FALSE;
int mpig_vmpi_module_ref_count = 0;


/*
 * initialization and termination functions
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
    
    mpig_vmpi_any_source = MPI_ANY_SOURCE;
    mpig_vmpi_any_tag = MPI_ANY_TAG;
    mpig_vmpi_in_place = (void *) MPI_IN_PLACE;
    mpig_vmpi_max_error_string = MPI_MAX_ERROR_STRING;
    mpig_vmpi_proc_null = MPI_PROC_NULL;
    mpig_vmpi_status_ignore = (mpig_vmpi_status_t *) MPI_STATUS_IGNORE;
    mpig_vmpi_statuses_ignore = (mpig_vmpi_status_t *) MPI_STATUSES_IGNORE;
    mpig_vmpi_undefined = MPI_UNDEFINED;
    
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
    mpig_vmpi_err_buffer = MPI_ERR_BUFFER;
    mpig_vmpi_err_count = MPI_ERR_COUNT;
    mpig_vmpi_err_type = MPI_ERR_TYPE;
    mpig_vmpi_err_tag = MPI_ERR_TAG;
    mpig_vmpi_err_comm = MPI_ERR_COMM;
    mpig_vmpi_err_rank = MPI_ERR_RANK;
    mpig_vmpi_err_root = MPI_ERR_ROOT;
    mpig_vmpi_err_truncate = MPI_ERR_TRUNCATE;
    mpig_vmpi_err_group = MPI_ERR_GROUP;
    mpig_vmpi_err_op = MPI_ERR_OP;
    mpig_vmpi_err_request = MPI_ERR_REQUEST;
    mpig_vmpi_err_topology = MPI_ERR_TOPOLOGY;
    mpig_vmpi_err_dims = MPI_ERR_DIMS;
    mpig_vmpi_err_arg = MPI_ERR_ARG;
    mpig_vmpi_err_other = MPI_ERR_OTHER;
    mpig_vmpi_err_unknown = MPI_ERR_UNKNOWN;
    mpig_vmpi_err_intern = MPI_ERR_INTERN;
    mpig_vmpi_err_in_status = MPI_ERR_IN_STATUS;
    mpig_vmpi_err_pending = MPI_ERR_PENDING;
    mpig_vmpi_err_file = MPI_ERR_FILE;
    mpig_vmpi_err_access = MPI_ERR_ACCESS;
    mpig_vmpi_err_amode = MPI_ERR_AMODE;
    mpig_vmpi_err_bad_file = MPI_ERR_BAD_FILE;
    mpig_vmpi_err_file_exists = MPI_ERR_FILE_EXISTS;
    mpig_vmpi_err_file_in_use = MPI_ERR_FILE_IN_USE;
    mpig_vmpi_err_no_space = MPI_ERR_NO_SPACE;
    mpig_vmpi_err_no_such_file = MPI_ERR_NO_SUCH_FILE;
    mpig_vmpi_err_io = MPI_ERR_IO;
    mpig_vmpi_err_read_only = MPI_ERR_READ_ONLY;
    mpig_vmpi_err_conversion = MPI_ERR_CONVERSION;
    mpig_vmpi_err_dup_datarep = MPI_ERR_DUP_DATAREP;
    mpig_vmpi_err_unsupported_datarep = MPI_ERR_UNSUPPORTED_DATAREP;
    mpig_vmpi_err_info = MPI_ERR_INFO;
    mpig_vmpi_err_info_key = MPI_ERR_INFO_KEY;
    mpig_vmpi_err_info_value = MPI_ERR_INFO_VALUE;
    mpig_vmpi_err_info_nokey = MPI_ERR_INFO_NOKEY;
    mpig_vmpi_err_name = MPI_ERR_NAME;
    mpig_vmpi_err_no_mem = MPI_ERR_NO_MEM;
    mpig_vmpi_err_not_same = MPI_ERR_NOT_SAME;
    mpig_vmpi_err_port = MPI_ERR_PORT;
    mpig_vmpi_err_quota = MPI_ERR_QUOTA;
    mpig_vmpi_err_service = MPI_ERR_SERVICE;
    mpig_vmpi_err_spawn = MPI_ERR_SPAWN;
    mpig_vmpi_err_unsupported_operation = MPI_ERR_UNSUPPORTED_OPERATION;
    mpig_vmpi_err_win = MPI_ERR_WIN;
    mpig_vmpi_err_base = MPI_ERR_BASE;
    mpig_vmpi_err_locktype = MPI_ERR_LOCKTYPE;
    mpig_vmpi_err_keyval = MPI_ERR_KEYVAL;
    mpig_vmpi_err_rma_conflict = MPI_ERR_RMA_CONFLICT;
    mpig_vmpi_err_rma_sync = MPI_ERR_RMA_SYNC;
    mpig_vmpi_err_size = MPI_ERR_SIZE;
    mpig_vmpi_err_disp = MPI_ERR_DISP;
    mpig_vmpi_err_assert = MPI_ERR_ASSERT;

  fn_return:
    return vrc;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_vmpi_init() */

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
/* mpig_vmpi_finalize() */

int mpig_vmpi_abort(mpig_vmpi_comm_t * comm, int exit_code)
{
    return MPI_Abort(*(MPI_Comm *) comm, exit_code);
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
