/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2002 Argonne National Laboratory
 *
 * See COPYRIGHT.txt in the src/mpid/globus directory.
 */

/*
 * This file contains the integration of MPICH2 segment (i.e., DATA LOOP) functions and Globus data conversion functions.
 */

#include <stdio.h>
#include <stdlib.h>

#include "mpidimpl.h"

#if defined(MPID_HAS_HETERO)

#undef MPID_SP_VERBOSE
#undef MPID_SU_VERBOSE

/* 
 * -- These from MPI 1.1 standard, "Annex A: Lanugage Binding", pp 212-213
 * elementary datatypes (C) MPI_CHAR MPI_SHORT MPI_INT MPI_LONG MPI_UNSIGNED_CHAR 
 * MPI_UNSIGNED_SHORT MPI_UNSIGNED MPI_UNSIGNED_LONG 
 * MPI_FLOAT MPI_DOUBLE MPI_LONG_DOUBLE MPI_BYTE 
 * MPI_PACKED 
 * optional datatypes (C) MPI_LONG_LONG_INT 
 * elementary datatypes (Fortran) MPI_INTEGER MPI_REAL MPI_DOUBLE_PRECISION 
 * MPI_COMPLEX MPI_LOGICAL MPI_CHARACTER 
 * MPI_BYTE MPI_PACKED 
 * MPI_DOUBLE_COMPLEX <-- moved to optional 
 * Fortran in MPI2.0
 * optional datatypes (Fortran) MPI_INTEGER1 MPI_INTEGER2 MPI_INTEGER4 MPI_REAL2 
 * MPI_REAL4 MPI_REAL8 
 * MPI_DOUBLE_COMPLEX <-- moved to optional 
 * Fortran in MPI2.0
 * datatypes for reduction functions (C) MPI_FLOAT_INT MPI_DOUBLE_INT 
 * MPI_LONG_INT MPI_2INT MPI_SHORT_INT 
 * MPI_LONG_DOUBLE_INT 
 * datatypes for reduction functions (Fortran) MPI_2REAL MPI_2DOUBLE_PRECISION 
 * MPI_2INTEGER 
 *
 * -- These from MPI 2.0 standard, "4.15 New Predefined Datatypes", pp 76-77
 * elementary datatype (C) MPI_WCHAR (corresponds to wchart_t in <stddef.h>)
 * MPI_SIGNED_CHAR (corresponds to 'signed char')
 * MPI_UNSIGNED_LONG_LONG (ISO C9X cmte now includes long long and unsigned long long in C Standard)
 *
 * -- See also MPI 1.1, "Section 3.2 Blocking Send and Receive Operations", 
 * p 18, for the following tables:
 *
 * Possible values of this argument for Fortran and the corresponding 
 * Fortran types are listed below. 
 * MPI datatype         Fortran datatype 
 * ------------         ----------------
 * MPI_INTEGER          INTEGER 
 * MPI_REAL             REAL 
 * MPI_DOUBLE_PRECISION DOUBLE PRECISION 
 * MPI_COMPLEX          COMPLEX 
 * MPI_LOGICAL          LOGICAL 
 * MPI_CHARACTER        CHARACTER(1) 
 * MPI_BYTE 
 * MPI_PACKED 
 *
 * Possible values for this argument for C and the corresponding 
 * C types are listed below. 
 * MPI datatype       C datatype 
 * ------------       ----------
 * MPI_CHAR           signed char 
 * MPI_SHORT          signed short int 
 * MPI_INT            signed int 
 * MPI_LONG           signed long int 
 * MPI_UNSIGNED_CHAR  unsigned char 
 * MPI_UNSIGNED_SHORT unsigned short int 
 * MPI_UNSIGNED       unsigned int 
 * MPI_UNSIGNED_LONG  unsigned long int 
 * MPI_FLOAT          float 
 * MPI_DOUBLE         double 
 * MPI_LONG_DOUBLE    long double 
 * MPI_BYTE 
 * MPI_PACKED 
 *
 * The datatypes MPI_BYTE and MPI_PACKED do not correspond to a Fortran or C datatype.  A value of type MPI_BYTE consists of a
 * byte (8 binary digits).  A byte is uninterpreted and is different from a character.  Different machines may have different
 * representations for characters, or may use more than one byte to represent characters.  On the other hand, a byte has the same
 * binary value on all machines.  The use of the type MPI_PACKED is explained in Section 3.13.
 *
 * MPI requires support of the datatypes listed above, which match the basic datatypes of Fortran 77 and ANSI C.  Additional MPI
 * datatypes should be provided if the host language has additional data types: MPI_LONG_LONG_INT for (64 bit) C integers
 * declared to be of type longlong int; MPI_DOUBLE_COMPLEX for double precision complex in Fortran declared to be of type DOUBLE
 * COMPLEX; MPI_REAL2, MPI_REAL4 and MPI_REAL8 for Fortran reals, declared to be of type REAL*2, REAL*4 and REAL*8, respectively;
 * MPI_INTEGER1 MPI_INTEGER2 and MPI_INTEGER4 for Fortran integers, declared to be of type INTEGER*1, INTEGER*2 and INTEGER*4,
 * respectively; etc.
 *
 * -- from MPI 2.0 section 10.2.5 Additional Support for Fortran Numeric Intrinsic Types, pp 291-292
 *
 * Support for Size-specific MPI Datatypes MPI-1 provides named datatypes corresponding to optional Fortran 77 numeric types that
 * contain explicit byte lengths --- MPI_REAL4, MPI_INTEGER8, etc.  This section describes a mechanism that generalizes this
 * model to support all Fortran numeric intrinsic types.
 *
 * We assume that for each typeclass (integer, real, complex) and each word size there is a unique machine representation.  For
 * every pair (typeclass, n) supported by a compiler, MPI must provide a named size-specific datatype.  The name of this datatype
 * is of the form MPI <TYPE>n in C and Fortran and of the form MPI::<TYPE>n in C++ where <TYPE> is one of REAL, INTEGER and
 * COMPLEX, and n is the length in bytes of the machine representation.  This datatype locally matches all variables of type
 * (typeclass, n).  The list of names for such types includes:
 *
 * MPI_REAL4 
 * MPI_REAL8 
 * MPI_REAL16 
 * MPI_COMPLEX8 
 * MPI_COMPLEX16 
 * MPI_COMPLEX32 
 * MPI_INTEGER1 
 * MPI_INTEGER2 
 * MPI_INTEGER4 
 * MPI_INTEGER8 
 * MPI_INTEGER16 
 *
 * In MPI-1 these datatypes are all optional and correspond to the optional, nonstandard declarations supported by many Fortran
 * compilers.  In MPI-2, one datatype is required for each representation supported by the compiler.  To be backward compatible
 * with the interpretation of these types in MPI-1, we assume that the nonstandard declarations REAL*n, INTEGER*n, always create
 * a variable whose representation is of size n. All these datatypes are predefined.
 *
 * and finally, this from mpich2/src/include/mpi.h.in:
 * #define MPI_CHAR           ((MPI_Datatype)@MPI_CHAR@)
 * #define MPI_SIGNED_CHAR    ((MPI_Datatype)@MPI_SIGNED_CHAR@)
 * #define MPI_UNSIGNED_CHAR  ((MPI_Datatype)@MPI_UNSIGNED_CHAR@)
 * #define MPI_BYTE           ((MPI_Datatype)@MPI_BYTE@)
 * #define MPI_WCHAR          ((MPI_Datatype)@MPI_WCHAR@)
 * #define MPI_SHORT          ((MPI_Datatype)@MPI_SHORT@)
 * #define MPI_UNSIGNED_SHORT ((MPI_Datatype)@MPI_UNSIGNED_SHORT@)
 * #define MPI_INT            ((MPI_Datatype)@MPI_INT@)
 * #define MPI_UNSIGNED       ((MPI_Datatype)@MPI_UNSIGNED_INT@)
 * #define MPI_LONG           ((MPI_Datatype)@MPI_LONG@)
 * #define MPI_UNSIGNED_LONG  ((MPI_Datatype)@MPI_UNSIGNED_LONG@)
 * #define MPI_FLOAT          ((MPI_Datatype)@MPI_FLOAT@)
 * #define MPI_DOUBLE         ((MPI_Datatype)@MPI_DOUBLE@)
 * #define MPI_LONG_DOUBLE    ((MPI_Datatype)@MPI_LONG_DOUBLE@)
 * #define MPI_LONG_LONG_INT  ((MPI_Datatype)@MPI_LONG_LONG@)
 * #define MPI_UNSIGNED_LONG_LONG ((MPI_Datatype)@MPI_UNSIGNED_LONG_LONG@)
 * #define MPI_LONG_LONG      MPI_LONG_LONG_INT
 *
 * #define MPI_PACKED         ((MPI_Datatype)@MPI_PACKED@)
 * #define MPI_LB             ((MPI_Datatype)@MPI_LB@)
 * #define MPI_UB             ((MPI_Datatype)@MPI_UB@)
 *
 * The layouts for the types MPI_DOUBLE_INT etc are simply
 * struct { 
 *     double var;
 *     int    loc;
 * }
 *
 * This is documented in the man pages on the various datatypes.   
 * #define MPI_FLOAT_INT         ((MPI_Datatype)@MPI_FLOAT_INT@)
 * #define MPI_DOUBLE_INT        ((MPI_Datatype)@MPI_DOUBLE_INT@)
 * #define MPI_LONG_INT          ((MPI_Datatype)@MPI_LONG_INT@)
 * #define MPI_SHORT_INT         ((MPI_Datatype)@MPI_SHORT_INT@)
 * #define MPI_2INT              ((MPI_Datatype)@MPI_2INT@)
 * #define MPI_LONG_DOUBLE_INT   ((MPI_Datatype)@MPI_LONG_DOUBLE_INT@)
 *
 * --Fortran types--
 * #define MPI_COMPLEX           ((MPI_Datatype)@MPI_COMPLEX@)
 * #define MPI_DOUBLE_COMPLEX    ((MPI_Datatype)@MPI_DOUBLE_COMPLEX@)
 * #define MPI_LOGICAL           ((MPI_Datatype)@MPI_LOGICAL@)
 * #define MPI_REAL              ((MPI_Datatype)@MPI_REAL@)
 * #define MPI_DOUBLE_PRECISION  ((MPI_Datatype)@MPI_DOUBLE_PRECISION@)
 * #define MPI_INTEGER           ((MPI_Datatype)@MPI_INTEGER@)
 * #define MPI_2INTEGER          ((MPI_Datatype)@MPI_2INTEGER@)
 * #define MPI_2COMPLEX          ((MPI_Datatype)@MPI_2COMPLEX@)
 * #define MPI_2DOUBLE_COMPLEX   ((MPI_Datatype)@MPI_2DOUBLE_COMPLEX@)
 * #define MPI_2REAL             ((MPI_Datatype)@MPI_2REAL@)
 * #define MPI_2DOUBLE_PRECISION ((MPI_Datatype)@MPI_2DOUBLE_PRECISION@)
 * #define MPI_CHARACTER         ((MPI_Datatype)@MPI_CHARACTER@)
 *
 * --Size-specific types (see MPI-2, 10.2.5)--
 * #define MPI_REAL4             ((MPI_Datatype)@MPI_REAL4@)
 * #define MPI_REAL8             ((MPI_Datatype)@MPI_REAL8@)
 * #define MPI_REAL16            ((MPI_Datatype)@MPI_REAL16@)
 * #define MPI_COMPLEX8          ((MPI_Datatype)@MPI_COMPLEX8@)
 * #define MPI_COMPLEX16         ((MPI_Datatype)@MPI_COMPLEX16@)
 * #define MPI_COMPLEX32         ((MPI_Datatype)@MPI_COMPLEX32@)
 * #define MPI_INTEGER1          ((MPI_Datatype)@MPI_INTEGER1@)
 * #define MPI_INTEGER2          ((MPI_Datatype)@MPI_INTEGER2@)
 * #define MPI_INTEGER4          ((MPI_Datatype)@MPI_INTEGER4@)
 * #define MPI_INTEGER8          ((MPI_Datatype)@MPI_INTEGER8@)
 * #define MPI_INTEGER16         ((MPI_Datatype)@MPI_INTEGER16@)
 *  -- 
 */

/* 
 * From MPI 1.1 standard, section 3.3.1 Type matching rules:
 * ----------------------------------------------
 * One can think of message transfer as consisting of the following three phases.
 *
 * 1. Data is pulled out of the send buffer and a message is assembled. 
 * 2. A message is transferred from sender to receiver. 
 * 3. Data is pulled from the incoming message and disassembled into the receive buffer.
 *
 * Type matching has to be observed at each of these three phases: The type of each variable in the sender buffer has to match
 * the type specified for that entry by the send operation; the type specified by the send operation has to match the type
 * specified by the receive operation; and the type of each variable in the receive buffer has to match the type specified for
 * that entry by the receive operation.  A program that fails to observe these three rules is erroneous.
 *
 * To define type matching more precisely, we need to deal with two issues: matching of types of the host language with types
 * specified in communication operations; and matching of types at sender and receiver.
 *
 * The types of a send and receive match (phase two) if both operations use identical names.  That is, MPI_INTEGER matches
 * MPI_INTEGER, MPI_REAL matches MPI_REAL, and so on.  There is one exception to this rule, discussed in Sec. Pack and unpack,
 * the type MPI_PACKED can match any other type.
 * ----------------------------------------------
 * This means that (ignoring MPI_PACKED for the moment) the application must make sure that the MPI datatype specified in the
 * send matches exactly to the MPI datatype specified on the receive.
 */

/* 
 * From MPI 1.1 standard, section 3.3.2 Data Conversion:
 * ----------------------------------------------
 * One of the goals of MPI is to support parallel computations across heterogeneous environments.  Communication in a
 * heterogeneous environment may require data conversions.  We use the following terminology.
 *
 * type conversion -- changes the datatype of a value, e.g., by rounding a REAL to an INTEGER.
 *
 * representation conversion -- changes the binary representation of a value, e.g., from Hex floating point to IEEE floating
 * point.
 *
 * The type matching rules imply that MPI communication never entails type conversion.  On the other hand, MPI requires that a
 * representation conversion be performed when a typed value is transferred across environments that use different
 * representations for the datatype of this value.  MPI does not specify rules for representation conversion.  Such conversion is
 * expected to preserve integer, logical or character values, and to convert a floating point value to the nearest value that can
 * be represented on the target system.
 *
 * Overflow and underflow exceptions may occur during floating point conversions.  Conversion of integers or characters may also
 * lead to exceptions when a value that can be represented in one system cannot be represented in the other system.  An exception
 * occurring during representation conversion results in a failure of the communication.  An error occurs either in the send
 * operation, or the receive operation, or both.
 *
 * If a value sent in a message is untyped (i.e., of type MPI_BYTE), then the binary representation of the byte stored at the
 * receiver is identical to the binary representation of the byte loaded at the sender.  This holds true, whether sender and
 * receiver run in the same or in distinct environments.  No representation conversion is required. (Note that representation
 * conversion may occur when values of type MPI_CHARACTER or MPI_CHAR are transferred, for example, from an EBCDIC encoding to an
 * ASCII encoding.)
 * ----------------------------------------------
 * To reiterate, this means 
 * (1) that the app must match types specified in send and recv,
 * (2) if underflow or overflow occurs when converting floats or ints then we are supposed to treat this as an error on the send,
 *     receive, or both, and
 * (3) we never convert MPI_BYTE.
 */


/*
 * GLOBUS_DC - segment processing routines that use the globus data conversion module
 */
#if defined(HAVE_GLOBUS_DC_MODULE)

/*******************/
/* LOCAL FUNCTIONS */
/*******************/
/* 
 * mpig_segment_globus_dc_unpack_contig() - unpack data from a stream into a contiguous buffer using the globus data conversion
 * routines
 *
 * NOTE: Globus data conversion routines available are declared and documented in globus_dc.h.  The following routines are used.
 *
 *     globus_dc_get_{byte, char, short, int, long, long_long, float, double}
 *     globus_dc_get_u_{char, short, int, long}
 *     nexus_dc_get_u_long_long
 */
#undef FUNCNAME
#define FUNCNAME mpig_segment_globus_dc_unpack_contig
static int mpig_segment_globus_dc_unpack_contig(
    DLOOP_Offset * blocks_p,      /* count */
    DLOOP_Type el_type,           /* MPI basic datatype */
    DLOOP_Offset dest_rel_off,    /* dest buffer relative offset */
    void * dest_bufp,             /* dest buffer */
    void * v_paramp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_segment_piece_params_t * p = v_paramp;
    globus_byte_t ** src_buf = (globus_byte_t **) &(p->src_buffer);
    const mpig_data_format_descriptor_t * const src_dfd = p->src_dfd;
    char * dest_buf = ((char *) dest_bufp) + dest_rel_off;
    MPIU_Size_t count = *blocks_p;
    mpig_ctype_t src_ctype = mpig_dfd_get_mpi_ctype_mapping(src_dfd, el_type);;
    int local_sizeof_src_ctype = mpig_dfd_get_sizeof_ctype(&mpig_process.my_dfd, src_ctype);
    int mult = mpig_datatype_get_num_ctypes(el_type);
    mpig_endian_t src_endian = mpig_dfd_get_endian(src_dfd);
    int src_gdc_format = mpig_dfd_get_gdc_format(src_dfd);;
    int mpi_errno = MPI_SUCCESS;

    MPIG_UNUSED_VAR(fcname);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: dest_bufp=" MPIG_PTR_FMT ", dest_offset="
        MPIG_AINT_FMT ", cnt=" MPIG_SIZE_FMT ", dt=" MPIG_HANDLE_FMT ", src_buf=" MPIG_PTR_FMT ", src_dfd=" MPIG_PTR_FMT,
        MPIG_PTR_CAST(dest_bufp), dest_rel_off, count, el_type, MPIG_PTR_CAST(*src_buf), MPIG_PTR_CAST(src_dfd)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "info: src_dfd=" MPIG_PTR_FMT ", src_endian=%s, src_gdc_format=%d, src_ctype=%s, "
        "local_sizeof_ctype=%d, mult=%d", MPIG_PTR_CAST(src_dfd), MPIG_ENDIAN_STR(src_endian), src_gdc_format,
        mpig_ctype_get_string(src_ctype), local_sizeof_src_ctype, mult));
    
    if (local_sizeof_src_ctype <= MPID_Datatype_get_basic_size(el_type))
    {
        /* OK to receive directly into destination buffer */
        switch (src_ctype)
        {
            case MPIG_CTYPE_FLOAT:
            {
                globus_dc_get_float(src_buf, (float *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_DOUBLE:
            {
                globus_dc_get_double(src_buf, (double *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_CHAR:
            {
                globus_dc_get_char(src_buf, (char *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_SHORT:
            {
                globus_dc_get_short(src_buf, (short *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_INT:
            {
                globus_dc_get_int(src_buf, (int *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_LONG:
            {
                globus_dc_get_long(src_buf, (long *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_LONG_LONG:
            {
                globus_dc_get_long_long(src_buf, (long long *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_UNSIGNED_CHAR:
            {
                globus_dc_get_u_char(src_buf, (unsigned char *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_UNSIGNED_SHORT:
            {
                globus_dc_get_u_short(src_buf, (unsigned short *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_UNSIGNED_INT:
            {
                globus_dc_get_u_int(src_buf, (unsigned int *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_UNSIGNED_LONG:
            {
                globus_dc_get_u_long(src_buf, (unsigned long *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_UNSIGNED_LONG_LONG:
            {
                nexus_dc_get_u_long_long(src_buf, (unsigned long long *) dest_buf, mult * count, src_gdc_format);
                break;
            }
            case MPIG_CTYPE_INT8:
            {
                mpig_dc_getn_int8(src_endian, *src_buf, (int8_t *) dest_buf, mult * count);
                src_buf += 1;
                break;
            }
            case MPIG_CTYPE_INT16:
            {
                mpig_dc_getn_int16(src_endian, *src_buf, (int16_t *) dest_buf, mult * count);
                src_buf += 2;
                break;
            }
            case MPIG_CTYPE_INT32:
            {
                mpig_dc_getn_int32(src_endian, *src_buf, (int32_t *) dest_buf, mult * count);
                src_buf += 4;
                break;
            }
            case MPIG_CTYPE_INT64:
            {
                mpig_dc_getn_int64(src_endian, *src_buf, (int64_t *) dest_buf, mult * count);
                src_buf += 8;
                break;
            }
            default:
            {
                char err_str[256];

                /* TODO: generate returnable error code rather than aborting */
                MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, dt=" MPIG_HANDLE_FMT ", src_ctype=%d",
                    el_type, src_ctype);
                MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                break;
            }
        }
        /* end switch(src_ctype) */
    }
    else /* (local_csize_of_src_ctype > MPID_Datatype_get_basic_size(el_type)) */
    {
        /* 
           --NOT-- OK to receive directly into destination buffer  ...
           ... need to convert one at a time
        */
        mpig_ctype_t local_ctype = mpig_dfd_get_mpi_ctype_mapping(&mpig_process.my_dfd, el_type);
        unsigned i;
        /* single-instance temp buffs */
        union
        {
            float t_float;
            double t_double;
            char t_char;
            short t_short;
            int t_int;
            long t_long;
            long long t_long_long;
            unsigned char t_unsigned_char;
            unsigned short t_unsigned_short;
            unsigned int t_unsigned_int;
            unsigned long t_unsigned_long;
            unsigned long long t_unsigned_long_long;
        }
        tmp_buf;
        void * tmp_buf_p = NULL;

        for (i = 0; i < mult * count; i ++)
        {
            switch (src_ctype)
            {
                case MPIG_CTYPE_FLOAT:
                {
                    globus_dc_get_float(src_buf, &tmp_buf.t_float, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_float;
                    break;
                }
                case MPIG_CTYPE_DOUBLE:
                {
                    globus_dc_get_double(src_buf, &tmp_buf.t_double, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_double;
                    tmp_buf_p = &tmp_buf.t_double;
                    break;
                }
                case MPIG_CTYPE_CHAR:
                {
                    globus_dc_get_char(src_buf, &tmp_buf.t_char, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_char;
                    break;
                }
                case MPIG_CTYPE_SHORT:
                {
                    globus_dc_get_short(src_buf, &tmp_buf.t_short, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_short;
                    break;
                }
                case MPIG_CTYPE_INT:
                {
                    globus_dc_get_int(src_buf, &tmp_buf.t_int, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_int;
                    break;
                }
                case MPIG_CTYPE_LONG:
                {
                    globus_dc_get_long(src_buf, &tmp_buf.t_long, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_long;
                    break;
                }
                case MPIG_CTYPE_LONG_LONG:
                {
                    globus_dc_get_long_long(src_buf, &tmp_buf.t_long_long, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_long_long;
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_CHAR:
                {
                    globus_dc_get_u_char(src_buf, &tmp_buf.t_unsigned_char, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_unsigned_char;
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_SHORT:
                {
                    globus_dc_get_u_short(src_buf, &tmp_buf.t_unsigned_short, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_unsigned_short;
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_INT:
                {
                    globus_dc_get_u_int(src_buf, &tmp_buf.t_unsigned_int, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_unsigned_int;
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG:
                {
                    globus_dc_get_u_long(src_buf, &tmp_buf.t_unsigned_long, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_unsigned_long;
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                {
                    nexus_dc_get_u_long_long(src_buf, &tmp_buf.t_unsigned_long_long, 1, src_gdc_format);
                    tmp_buf_p = (void *) &tmp_buf.t_unsigned_long_long;
                    break;
                }
                default:
                {
                    char err_str[256];
                            
                    /* TODO: generate returnable error code rather than aborting */
                    MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, dt=" MPIG_HANDLE_FMT ", src_ctype=%d",
                        el_type, src_ctype);
                    MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                    break;
                }
            } /* end switch(src_ctype) */

            switch (local_ctype)
            {
                case MPIG_CTYPE_FLOAT:
                {
                    *((float *) dest_buf) = *((float *) tmp_buf_p);
                    dest_buf += sizeof(float);
                    break;
                }
                case MPIG_CTYPE_DOUBLE:
                {
                    *((double *) dest_buf) = *((double *) tmp_buf_p);
                    dest_buf += sizeof(double);
                    break;
                }
                case MPIG_CTYPE_CHAR:
                {
                    *((char *) dest_buf) = *((char *) tmp_buf_p);
                    dest_buf += sizeof(char);
                    break;
                }
                case MPIG_CTYPE_SHORT:
                {
                    *((short *) dest_buf) = *((short *) tmp_buf_p);
                    dest_buf += sizeof(short);
                    break;
                }
                case MPIG_CTYPE_INT:
                {
                    *((int *) dest_buf) =  *((int *) tmp_buf_p);
                    dest_buf += sizeof(int);
                    break;
                }
                case MPIG_CTYPE_LONG:
                {
                    *((long *) dest_buf) = *((long *) tmp_buf_p);
                    dest_buf += sizeof(long);
                    break;
                }
                case MPIG_CTYPE_LONG_LONG:
                {
                    *((long long *) dest_buf) = *((long long *) tmp_buf_p);
                    dest_buf += sizeof(long long);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_CHAR:
                {
                    *((unsigned char *) dest_buf) = *((unsigned char *) tmp_buf_p);
                    dest_buf += sizeof(unsigned char);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_SHORT:
                {
                    *((unsigned short *) dest_buf) = *((unsigned short *) tmp_buf_p);
                    dest_buf += sizeof(unsigned short);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_INT:
                {
                    *((unsigned int *) dest_buf) = *((unsigned int *) tmp_buf_p);
                    dest_buf += sizeof(unsigned int);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG:
                {
                    *((unsigned long *) dest_buf) = *((unsigned long *) tmp_buf_p);
                    dest_buf += sizeof(unsigned long);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                {
                    *((unsigned long *) dest_buf) = *((unsigned long long *) tmp_buf_p);
                    dest_buf += sizeof(unsigned long long);
                    break;
                }
                default:
                {
                    char err_str[256];

                    /* TODO: generate returnable error code rather than aborting */
                    MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, dt=" MPIG_HANDLE_FMT ", local_ctype=%d",
                        el_type, local_ctype);
                    MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                    break;
                }
            }
            /* end switch(local_ctype) */
        } /* end for (i = 0; i < mult * count; i ++) */
    } /* end if/else (local_csize_of_src_ctype <= MPID_Datatype_get_basic_size(el_type)) */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting: dest_bufp=" MPIG_PTR_FMT ", src_buf=" MPIG_PTR_FMT
        ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(dest_bufp), MPIG_PTR_CAST(*src_buf), mpi_errno));

    return mpi_errno;
}
/* end mpig_segment_globus_dc_unpack_contig() */

/********************************/
/* EXTERNALLY VISIBLE FUNCTIONS */
/********************************/

#undef FUNCNAME 
#define FUNCNAME mpig_segment_globus_dc_unpack
void mpig_segment_globus_dc_unpack(struct DLOOP_Segment * const segp, const mpig_data_format_descriptor_t * const src_dfd,
    const DLOOP_Offset first, DLOOP_Offset * const lastp, DLOOP_Buffer src_buffer)
{
    struct mpig_segment_piece_params unpack_params;
    /* MPIDI_STATE_DECL(MPID_STATE_mpig_segment_globus_dc_unpack); */
    
    /* MPIDI_FUNC_ENTER(MPID_STATE_mpig_segment_globus_dc_unpack); */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: segp=" MPIG_PTR_FMT ", src_dfd=" MPIG_PTR_FMT
        ", first=" MPIG_AINT_FMT ", last=" MPIG_AINT_FMT ", src_buffer=" MPIG_PTR_FMT,MPIG_PTR_CAST(segp), MPIG_PTR_CAST(src_dfd),
        first, *lastp, MPIG_PTR_CAST(src_buffer)));

    unpack_params.src_buffer = src_buffer;
    unpack_params.src_dfd = src_dfd;

    MPID_Segment_manipulate(
        segp,
        first,
        lastp,
        /* function pointer we supply to convert contig data */
        mpig_segment_globus_dc_unpack_contig,
        /* these next three are function pointers for vector,
           block indexed, and index, respectively.  for each
           one we have the option to supply our own routine
           OR simply pass NULL in which case the segment
           code will 'unwrap" each of the three down to 
           their 'contig' elementary types and call our 
           contig function above.  
           we would choose to write our own functions for 
           optimization (eliminate calling up and down the function 
           stack multiple times) but that's probably not worth
           the time to write ... the savings is miniscule compared
           to the WAN/LAN latency and data conversion on these types.
        */
        NULL,  /* vector      */
        NULL,  /* block index */
        NULL,  /* index       */
        /* 
           func ptr we provide to return basic sizes of in the src buffer
           (i.e., what the segment does NOT point to).  in our case, data sizes 
           of MPI datatypes on the sending side.
        */
        mpig_segment_sizeof_source_basic_datatype,
        (void *) &unpack_params);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting: segp=" MPIG_PTR_FMT ", last=" MPIG_AINT_FMT,
        MPIG_PTR_CAST(segp), *lastp));
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL); */
    return;

}
/* end mpig_segment_globus_dc_unpack() */

#endif /* if defined(HAVE_GLOBUS_DC_MODULE) */

/* 
 * mpig_segment_sizeof_source_basic_datatype() - compute the size of a basic MPI datatype at the source process
 */
#undef FUNCNAME 
#define FUNCNAME mpig_segment_sizeof_source_basic_datatype
MPI_Aint mpig_segment_sizeof_source_basic_datatype(MPI_Datatype dt, void * v_paramp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_segment_piece_params * p = (struct mpig_segment_piece_params *) v_paramp;
    const mpig_data_format_descriptor_t * const src_dfd = p->src_dfd;
    mpig_ctype_t src_ctype = mpig_dfd_get_mpi_ctype_mapping(src_dfd, dt);
    int src_sizeof_ctype = mpig_dfd_get_sizeof_ctype(src_dfd, src_ctype);
    int mult = mpig_datatype_get_num_ctypes(dt);
    MPI_Aint size = (MPI_Aint) mult * src_sizeof_ctype;

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "dt=" MPIG_HANDLE_FMT ", src_ctype=%s, src_sizeof_ctype=%d, mult=%d, size="
        MPIG_AINT_FMT, dt, mpig_ctype_get_string(src_ctype), src_sizeof_ctype, mult, size));
    
    MPIG_DEBUG_ASSERT(src_ctype > MPIG_CTYPE_INVALID && src_ctype < MPIG_CTYPE_LAST);
    MPIG_DEBUG_ASSERT(size >= 0);

    return size;
}
/* end mpig_segment_sizeof_source_basic_datatype() */

#endif /* defined(MPID_HAS_HETERO) */
