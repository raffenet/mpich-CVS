/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */


/*
 * This file contains the integration of MPICH2 segment (i.e., DATA LOOP) functions and Globus data conversion functions.
 */

#include <stdio.h>
#include <stdlib.h>

#include "mpidimpl.h"

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

/* mpig_segment_globus_dc_piece_params
 *
 * This structure is used to pass function-specific parameters into our segment processing function.  This allows us to get
 * additional parameters to the functions it calls without changing the prototype.
 */
struct mpig_segment_globus_dc_piece_params 
{
    globus_byte_t *src_buffer;
    int src_format;
    mpig_vc_t *vc;
}; /* end struct mpig_segment_globus_dc_piece_params */

/*******************/
/* LOCAL FUNCTIONS */
/*******************/
/* 
 * data conversion routines available from Globus ... see globus_dc.h globus_dc_get_{byte, char, short, int, long, long_long,
 * float, double} globus_dc_get_u_{char, short, int, long}
 */
#undef FUNCNAME
#define FUNCNAME mpig_segment_globus_dc_unpack_contig
static int mpig_segment_globus_dc_unpack_contig(
    DLOOP_Offset * blocks_p, /* count */
    DLOOP_Type el_type,
    DLOOP_Offset rel_off,  /* dest */
    void * bufp,            /* dest */
    void * v_paramp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_segment_globus_dc_piece_params * p = v_paramp;
    globus_byte_t ** s = &(p->src_buffer);    /* src        */
    char * d  = ((char *) bufp) + rel_off;    /* dest       */
    int f = p->src_format;                    /* src format */
    unsigned long c  = *blocks_p;             /* count      */
    mpig_vc_t * vc = p->vc;
    MPIG_STATE_DECL(MPID_STATE_mpig_segment_globus_dc_unpack_contig);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_segment_globus_dc_unpack_contig);

    MPIG_UNUSED_VAR(fcname);

    switch (el_type)
    {
        /* NICK: questionable types found in mpi.h.in: MPI_LB, MPI_UB */
        /*********************************************/
        /* Elementary C datatypes defined in MPI 1.1 */
        /*********************************************/
        case MPI_FLOAT:
        {
            globus_dc_get_float(s, (float *) d, c, f);
            break;
        }
        case MPI_DOUBLE:
        {
            globus_dc_get_double(s, (double *) d, c, f);
            break;
        }
#if defined(HAVE_MPI_LONG_DOUBLE)
        case MPI_LONG_DOUBLE:
        {
            /* NOTE: this datatype is not supported by the Globus data conversion library.  for that reason, this datatype is
               disabled by mpich2prereq-globus. */
            break;
        }
#endif
        case MPI_CHAR:
        {
            globus_dc_get_char(s, (char *) d, c, f);
            break;
        }
        case MPI_SHORT:
        {
            globus_dc_get_short(s, (short *) d, c, f);
            break;
        }
        case MPI_INT:
        {
            globus_dc_get_int(s, (int *) d, c, f);
            break;
        }
        case MPI_LONG:
        {
            globus_dc_get_long(s, (long *) d, c, f);
            break;
        }
#if defined(HAVE_MPI_LONG_LONG)
        case MPI_LONG_LONG:
        {
            globus_dc_get_long_long(s, (long long *) d, c, f);
            break;
        }
#endif
        case MPI_UNSIGNED_CHAR:
        {
            globus_dc_get_u_char(s, (unsigned char *) d, c, f);
            break;
        }
        case MPI_UNSIGNED_SHORT:
        {
            globus_dc_get_u_short(s, (unsigned short *) d, c, f);
            break;
        }
        case MPI_UNSIGNED:
        {
            globus_dc_get_u_int(s, (unsigned int *) d, c, f);
            break;
        }
        case MPI_UNSIGNED_LONG:
        {
            globus_dc_get_u_long(s, (unsigned long *) d, c, f);
            break;
        }
        /* BOTH OF THESE MUST BE A MEMCPY, (i.e., NOT CONVERTED) */
        case MPI_PACKED:
        case MPI_BYTE:
        {
               memcpy((void *) d, (void *) *s, c);
               (*s) += c;
               break;
        }

        /*******************************************/
        /* Optional C datatypes defined in MPI 1.1 */
        /*******************************************/
#if FALSE && defined(HAVE_MPI_LONG_LONG)
        /* MPICH2 defined MPI_LONG_LONG and MPI_LONG_LONG_INT as the same thing */
        case MPI_LONG_LONG_INT:
        {
            globus_dc_get_long_long  (s, (long long *) d, c, f);
            break;
        }            
#endif

        /**********************************************************/
        /* C datatypes for reduction functions defined in MPI 1.1 */
        /**********************************************************/

        /* 
         * NOTE: MPI_{FLOAT,DOUBLE,LONG,SHORT,LONG_DOUBLE}_INT are all constructed during intialization as derived datatypes and
         * are therefore decomposed by the segment code and eventually handled by this function as elementary datatypes ... in
         * other words, this function does NOT need to handle those cases explicitly.
         */
        case MPI_2INT:
        {
            globus_dc_get_int(s, (int *) d, 2*c, f);
            break;
        }

        /*************************************************/
        /* New Elementary C datatypes defined in MPI 2.0 */
        /*************************************************/
        case MPI_WCHAR:
        {
            /* TODO: add conversion for wide characters (is wchar_t always a fixed width integer?) */
            MPIU_Assertp(FALSE && "MPI_WCHAR conversion not implemented");
            break;
        }
        case MPI_SIGNED_CHAR:
        {
            globus_dc_get_char    (s, (char *) d, c, f);
            break;
        }
#if defined(HAVE_MPI_LONG_LONG)
        case MPI_UNSIGNED_LONG_LONG:
        {
            nexus_dc_get_u_long_long(s, (unsigned long long *) d, c, f);
            break;
        }
#endif

#ifdef HAVE_FORTRAN_BINDING
        /*
         * The Globus data conversion routines work only on C datatypes and so trying to use them on Fortran datatypes is a
         * little tricky.
         *
         * Step 1: identifying the globus_dc_get function (must match globus_dc_put function from sending side)
         *
         * First, we need to map the Fortran datatype to its equivalent C-type from the *sending* side (e.g., learn that an
         * MPI_REAL on the sending side is equivalent to a C-type float on the sending side.  This will determine which Globus
         * data conversion routine to call (e.g., if the C-type is a float on the sending side then we will call
         * globus_dc_get_float()).  This is required because we pass the format to the Globus data conversion routines and so if
         * we call globus_dc_get_float() and pass the format then globus_dc_get_float() will use the format to figure out how
         * many bytes a 'float' is on the sending side.
         *
         * Step 2: confirming application receive buffer is large enough
         *
         * Second, we need to take the size of the Fortran MPI datatype on the receiving side and compare that to the size on the
         * receiving side of the C-type discovered in the first step.  For example, if the MPI datatype is MPI_REAL and we learn
         * that an MPI_REAL on the sending side corresponds to a C-type float on the sending side then we must compare the size
         * of a C-type float on the receiving side to the size of an MPI_REAL on the receiving side.
         *
         * Sending side    |     Receiving side
         * ----------------+--------------------
         * |     MPI datatype
         * |       |    |   |
         * C-type <-------+-------+    |   +--------+
         * |            |            V            |
         * +------------+---> local  local     (if the two local
         * |     size   size       sizes are NOT equal)
         * |                         |
         * |                         v
         * |                       C-type
         *
         * If the two sizes match then it is safe to make ONE call to the Globus data conversion routine passing a pointer to the
         * source, a pointer to the destination, and the count (e.g., converting a source float to a source float) thus
         * converting the entire buffer (i.e., all 'count' elements) in a single function call.
         *
         * If, on the other hand, the two sizes do NOT match then we must create a temporary C variable of the same C-type as the
         * C-type representation of the Fortran datatype on the sending side (i.e., what we discovered in the first step) and
         * learn the C-type equivlaent of the MPI Fortran datatype on the receiving side (e.g., an MPI_REAL is a C-type float on
         * the receiving side).  We then convert the entire buffer by converting the elements one at a time into the temporary
         * variable and then assigning the temp variable to the destination buffer with the appropriate cast.  For example, if an
         * MPI_REAL is a C-type double (which forces us to call globus_dc_get_double()) that is 8 bytes on the sending side but a
         * C-type float having only 4 bytes on the receiving side then we would have to convert the entire buffer like this:
         * double *s = src_buff; double temp; float *d = dest_buff; for (i = 0; i < count; i ++) { globus_dc_get_double(s, &temp,
         * 1, format); *d = (float) temp; d ++; }
         *
         * This is all necessary because the Globus data conversion routines expect the destination buffer to be a C-type that
         * matches the function (e.g., globus_dc_get_float() requires a float destination buffer).
         *
         * NOTE: everything described above applies to both integer and floating point datatypes.
         */

        /***************************************************/
        /* Elementary FORTRAN datatypes defined in MPI 1.1 */
        /***************************************************/
        case MPI_LOGICAL:          case MPI_INTEGER: case MPI_REAL:
        case MPI_DOUBLE_PRECISION: case MPI_COMPLEX: case MPI_CHARACTER:
        /* MPI_{BYTE, PACKED} handled above in C cases */
        /*************************************************/
        /* Optional FORTRAN datatypes defined in MPI 1.1 */
        /*************************************************/
        case MPI_DOUBLE_COMPLEX:
#if defined(HAVE_MPI_INTEGER1)
        case MPI_INTEGER1:
#endif
#if defined(HAVE_MPI_INTEGER2)
        case MPI_INTEGER2:
#endif
#if defined(HAVE_MPI_INTEGER4)
        case MPI_INTEGER4:
#endif
#if defined(HAVE_MPI_INTEGER8)
        case MPI_INTEGER8:
#endif
#if defined(HAVE_MPI_INTEGER16)
        case MPI_INTEGER16
#endif
#if defined(HAVE_MPI_REAL2)
            case MPI_REAL2:
#endif
        /************************************************************/
        /* Additional optional FORTRAN datatypes defined in MPI 2.0 */
        /************************************************************/
#if defined(HAVE_MPI_REAL4)
        case MPI_REAL4:
#endif
#if defined(HAVE_MPI_REAL8)
        case MPI_REAL8:
#endif
#if defined(HAVE_MPI_REAL16)
        case MPI_REAL16:
#endif
#if defined(HAVE_MPI_REAL4)
        case MPI_COMPLEX8:
#endif
#if defined(HAVE_MPI_REAL8)
        case MPI_COMPLEX16:
#endif
#if defined(HAVE_MPI_REAL16)
        case MPI_COMPLEX32:
#endif
        /****************************************************************/
        /* FORTRAN datatypes for reduction functions defined in MPI 1.1 */
        /****************************************************************/
        case MPI_2REAL: case MPI_2DOUBLE_PRECISION: case MPI_2INTEGER:

        /*****************************************************************/
        /* FORTRAN datatypes for reduction functions defined in mpi.h.in */
        /*****************************************************************/
        case MPI_2COMPLEX: case MPI_2DOUBLE_COMPLEX: 
        { 
            /* *ALL* the Fortran datatype cases are handled by the code in this block */
            int m = mpig_datatype_get_ctype_size_multiplier(el_type);
            mpig_ctype_t remote_ctype = mpig_datatype_get_remote_ctype(vc, el_type);
            int local_csize_of_remote_ctype = mpig_datatype_get_local_sizeof_ctype(remote_ctype);

            if (FALSE && local_csize_of_remote_ctype <= MPID_Datatype_get_basic_size(el_type))
            {
                /* OK to receive directly into destination buffer */
                switch (remote_ctype)
                {
                    case MPIG_CTYPE_FLOAT:
                    {
                        globus_dc_get_float(s, (float *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_DOUBLE:
                    {
                        globus_dc_get_double(s, (double *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_CHAR:
                    {
                        globus_dc_get_char(s, (char *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_SHORT:
                    {
                        globus_dc_get_short(s, (short *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_INT:
                    {
                        globus_dc_get_int(s, (int *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_LONG:
                    {
                        globus_dc_get_long(s, (long *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_LONG_LONG:
                    {
                        globus_dc_get_long_long(s, (long long *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_UNSIGNED_CHAR:
                    {
                        globus_dc_get_u_char(s, (unsigned char *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_UNSIGNED_SHORT:
                    {
                        globus_dc_get_u_short(s, (unsigned short *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_UNSIGNED_INT:
                    {
                        globus_dc_get_u_int(s, (unsigned int *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_UNSIGNED_LONG:
                    {
                        globus_dc_get_u_long(s, (unsigned long *) d, m * c, f);
                        break;
                    }
                    case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                    {
                        nexus_dc_get_u_long_long(s, (unsigned long long *) d, m * c, f);
                        break;
                    }
                    default:
                    {
                        char err_str[256];

                        /* TODO: generate returnable error code rather than aborting */
                        MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, remote_ctype=%d", remote_ctype);
                        MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                        break;
                    }
                } /* end switch() */
            }
            else /* (local_csize_of_remote_ctype > MPID_Datatype_get_basic_size(el_type)) */
            {
                /* 
                   --NOT-- OK to receive directly into destination buffer  ...
                   ... need to convert one at a time
                */
                mpig_ctype_t local_ctype = mpig_datatype_get_local_ctype(el_type);
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

                for (i = 0; i < m * c; i ++)
                {
                    switch (remote_ctype)
                    {
                        case MPIG_CTYPE_FLOAT:
                        {
                            globus_dc_get_float(s, &tmp_buf.t_float, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_float;
                            break;
                        }
                        case MPIG_CTYPE_DOUBLE:
                        {
                            globus_dc_get_double(s, &tmp_buf.t_double, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_double;
                            tmp_buf_p = &tmp_buf.t_double;
                            break;
                        }
                        case MPIG_CTYPE_CHAR:
                        {
                            globus_dc_get_char(s, &tmp_buf.t_char, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_char;
                            break;
                        }
                        case MPIG_CTYPE_SHORT:
                        {
                            globus_dc_get_short(s, &tmp_buf.t_short, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_short;
                            break;
                        }
                        case MPIG_CTYPE_INT:
                        {
                            globus_dc_get_int(s, &tmp_buf.t_int, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_int;
                            break;
                        }
                        case MPIG_CTYPE_LONG:
                        {
                            globus_dc_get_long(s, &tmp_buf.t_long, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_long;
                            break;
                        }
                        case MPIG_CTYPE_LONG_LONG:
                        {
                            globus_dc_get_long_long(s, &tmp_buf.t_long_long, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_long_long;
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_CHAR:
                        {
                            globus_dc_get_u_char(s, &tmp_buf.t_unsigned_char, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_unsigned_char;
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_SHORT:
                        {
                            globus_dc_get_u_short(s, &tmp_buf.t_unsigned_short, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_unsigned_short;
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_INT:
                        {
                            globus_dc_get_u_int(s, &tmp_buf.t_unsigned_int, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_unsigned_int;
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_LONG:
                        {
                            globus_dc_get_u_long(s, &tmp_buf.t_unsigned_long, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_unsigned_long;
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                        {
                            nexus_dc_get_u_long_long(s, &tmp_buf.t_unsigned_long_long, 1, f);
                            tmp_buf_p = (void *) &tmp_buf.t_unsigned_long_long;
                            break;
                        }
                        default:
                        {
                            char err_str[256];
                            
                            /* TODO: generate returnable error code rather than aborting */
                            MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, remote_ctype=%d", remote_ctype);
                            MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                            break;
                        }
                    } /* end switch(remote_ctype) */

                    switch (local_ctype)
                    {
                        case MPIG_CTYPE_FLOAT:
                        {
                            *((float *) d) = *((float *) tmp_buf_p);
                            d += sizeof(float);
                            break;
                        }
                        case MPIG_CTYPE_DOUBLE:
                        {
                            *((double *) d) = *((double *) tmp_buf_p);
                            d += sizeof(double);
                            break;
                        }
                        case MPIG_CTYPE_CHAR:
                        {
                            *((char *) d) = *((char *) tmp_buf_p);
                            d += sizeof(char);
                            break;
                        }
                        case MPIG_CTYPE_SHORT:
                        {
                            *((short *) d) = *((short *) tmp_buf_p);
                            d += sizeof(short);
                            break;
                        }
                        case MPIG_CTYPE_INT:
                        {
                            *((int *) d) =  *((int *) tmp_buf_p);
                            d += sizeof(int);
                            break;
                        }
                        case MPIG_CTYPE_LONG:
                        {
                            *((long *) d) = *((long *) tmp_buf_p);
                            d += sizeof(long);
                            break;
                        }
                        case MPIG_CTYPE_LONG_LONG:
                        {
                            *((long long *) d) = *((long long *) tmp_buf_p);
                            d += sizeof(long long);
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_CHAR:
                        {
                            *((unsigned char *) d) = *((unsigned char *) tmp_buf_p);
                            d += sizeof(unsigned char);
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_SHORT:
                        {
                            *((unsigned short *) d) = *((unsigned short *) tmp_buf_p);
                            d += sizeof(unsigned short);
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_INT:
                        {
                            *((unsigned int *) d) = *((unsigned int *) tmp_buf_p);
                            d += sizeof(unsigned int);
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_LONG:
                        {
                            *((unsigned long *) d) = *((unsigned long *) tmp_buf_p);
                            d += sizeof(unsigned long);
                            break;
                        }
                        case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                        {
                            *((unsigned long *) d) = *((unsigned long long *) tmp_buf_p);
                            d += sizeof(unsigned long long);
                            break;
                        }
                        default:
                        {
                            char err_str[256];

                            /* TODO: generate returnable error code rather than aborting */
                            MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, local_ctype=%d", local_ctype);
                            MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                            break;
                        }
                    }
                    /* end switch(local_ctype) */
                                                
                } /* end for (i = 0; i < m * c; i ++) */
            } /* end if/else (local_csize_of_remote_ctype <= MPID_Datatype_get_basic_size(el_type)) */
        } /* end all Fortran dataype cases */
        break;
#endif
        default:
        {
            char err_str[256];
            
            /* TODO: generate returnable error code rather than aborting */
            MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPI datatype, el_type=" MPIG_HANDLE_FMT, el_type);
            MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
            break;
        }
    } /* end switch(el_type) */

    MPIG_FUNC_EXIT(MPID_STATE_mpig_segment_globus_dc_unpack_contig);
    return 0;

} /* end mpig_segment_unpack_contig_globus_dc() */

/* 
 * sizeof routines available from Globus ... see globus_dc.h 
 *  globus_dc_sizeof_remote_{byte, char, short, int, long, long_long, float, double}
 *  globus_dc_sizeof_remote_u_{char, short, int, long}
 *
 * FIXME: it would be simpler to get the sizes from the remote host and stick them in an array attached to the VC.  doing this
 * would likely improve performance as well.
 */

#undef FUNCNAME
#define FUNCNAME mpig_segment_globus_dc_sizeof_datatype
static MPI_Aint mpig_segment_globus_dc_sizeof_datatype(MPI_Datatype el_type, void *v_paramp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    struct mpig_segment_globus_dc_piece_params *p = (struct mpig_segment_globus_dc_piece_params *) v_paramp;
    int c = mpig_datatype_get_ctype_size_multiplier(el_type);
    int f = p->src_format;
    mpig_vc_t * vc = p->vc;
    MPI_Aint size = (MPI_Aint) -1; /* element size */
    MPIG_STATE_DECL(MPID_STATE_mpig_segment_globus_dc_sizeof_datatype);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_segment_globus_dc_sizeof_datatype);

    MPIG_UNUSED_VAR(fcname);
    /* 
     * sizeof routines available from Globus ... see globus_dc.h 
     *  globus_dc_sizeof_remote_{byte, char, short, int, long, long_long, float, double}
     *  globus_dc_sizeof_remote_u_{char, short, int, long}
     */
    switch (el_type)
    {
        /*********************************************/
        /* Elementary C datatypes defined in MPI 1.1 */
        /*********************************************/
        case MPI_CHAR:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_char(c,f);
            break;
        }
        case MPI_SHORT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_short(c,f);
            break;
        }
        case MPI_INT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_int(c,f);
            break;
        }
        case MPI_LONG:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_long(c,f);
            break;
        }
#if defined(HAVE_MPI_LONG_LONG)
        case MPI_LONG_LONG:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_long_long(c,f);
            break;
        }
#endif
        case MPI_UNSIGNED_CHAR:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_u_char(c,f);
            break;
        }
        case MPI_UNSIGNED_SHORT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_u_short(c,f);
            break;
        }
        case MPI_UNSIGNED:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_u_int(c,f);
            break;
        }
        case MPI_UNSIGNED_LONG:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_u_long(c,f);
            break;
        }
        case MPI_FLOAT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_float(c,f);
            break;
        }
        case MPI_DOUBLE:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_double(c,f);
            break;
        }
#if defined(HAVE_MPI_LONG_DOUBLE)
        case MPI_LONG_DOUBLE:
        {
            /* NOTE: this datatype is not supported by the Globus data conversion library.  for that reason, this datatype is
               disabled by mpich2prereq-globus. */
            break;
        }
#endif
        case MPI_PACKED:
        case MPI_BYTE:
        {
            /* BOTH OF THESE MUST BE A MEMCPY, (i.e., NOT CONVERTED) */
               size = (MPI_Aint) 1;
               break;
        }

        /*******************************************/
        /* Optional C datatypes defined in MPI 1.1 */
        /*******************************************/
#if FALSE && defined(HAVE_MPI_LONG_LONG)
        /* MPICH2 defined MPI_LONG_LONG and MPI_LONG_LONG_INT as the same thing */
        case MPI_LONG_LONG_INT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_long_long(c,f);
            break;
        }
#endif

        /**********************************************************/
        /* C datatypes for reduction functions defined in MPI 1.1 */
        /**********************************************************/

        /* 
         * NOTE: MPI_{FLOAT,DOUBLE,LONG,SHORT,LONG_DOUBLE}_INT are all constructed during intialization as derived datatypes
         * and are therefore decomposed by the segment code and eventually handled by this function as elementary datatypes
         * ... in other words, this function does NOT need to handle those cases explicitly.
         */
        case MPI_2INT:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_int(c,f);
            break;
        }

        /*************************************************/
        /* New Elementary C datatypes defined in MPI 2.0 */
        /*************************************************/
        case MPI_WCHAR:
        {
            /* TODO: add conversion for wide characters (is wchar_t always a fixed width integer?) */
            MPIU_Assertp(FALSE && "MPI_WCHAR conversion not implemented");
            break;
        }
        case MPI_SIGNED_CHAR:
        {
            size = (MPI_Aint) globus_dc_sizeof_remote_char(c,f);
            break;
        }
#if defined(HAVE_MPI_LONG_LONG)
        case MPI_UNSIGNED_LONG_LONG:
        {
            size = (MPI_Aint) nexus_dc_sizeof_remote_u_long_long(c,f);
            break;
        }
#endif

#ifdef HAVE_FORTRAN_BINDING
        /***************************************************/
        /* Elementary FORTRAN datatypes defined in MPI 1.1 */
        /***************************************************/
        case MPI_LOGICAL:          case MPI_INTEGER: case MPI_REAL:
        case MPI_DOUBLE_PRECISION: case MPI_COMPLEX: case MPI_CHARACTER:
        /* MPI_{BYTE, PACKED} handled above in C cases */

        /*************************************************/
        /* Optional FORTRAN datatypes defined in MPI 1.1 */
        /*************************************************/
        case MPI_DOUBLE_COMPLEX:

        /****************************************************************/
        /* FORTRAN datatypes for reduction functions defined in MPI 1.1 */
        /****************************************************************/
        case MPI_2REAL: case MPI_2DOUBLE_PRECISION: case MPI_2INTEGER:

        /*****************************************************************/
        /* FORTRAN datatypes for reduction functions defined in mpi.h.in */
        /*****************************************************************/
        case MPI_2COMPLEX: case MPI_2DOUBLE_COMPLEX: 
        { 
            /* *ALL* the Fortran datatype cases are handled by the code in this block */
            mpig_ctype_t remote_ctype = mpig_datatype_get_remote_ctype(vc, el_type);

            switch (remote_ctype)
            {
                case MPIG_CTYPE_FLOAT:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_float(c,f);
                    break;
                }
                case MPIG_CTYPE_DOUBLE:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_double(c,f);
                    break;
                }
                case MPIG_CTYPE_CHAR:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_char(c,f);
                    break;
                }
                case MPIG_CTYPE_SHORT:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_short(c,f);
                    break;
                }
                case MPIG_CTYPE_INT:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_int(c,f);
                    break;
                }
                case MPIG_CTYPE_LONG:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_long(c,f);
                    break;
                }
                case MPIG_CTYPE_LONG_LONG:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_long_long(c,f);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_CHAR:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_u_char(c,f);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_SHORT:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_u_short(c,f);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_INT:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_u_int(c,f);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG:
                {
                    size = (MPI_Aint) globus_dc_sizeof_remote_u_long(c,f);
                    break;
                }
                case MPIG_CTYPE_UNSIGNED_LONG_LONG:
                {
                    size = (MPI_Aint) nexus_dc_sizeof_remote_u_long_long(c,f);
                    break;
                }
                default:
                {
                    char err_str[256];

                    /* TODO: generate returnable error code rather than aborting */
                    MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPIG_CTYPE, remote_ctype=%d", remote_ctype);
                    MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
                    size = (MPI_Aint) 0;
                    break;
                }
            } /* end switch(remote_ctype) */
                    
            break;
        } /* end all variable sized Fortran dataype cases */

        /************************************************************/
        /* Optional fixed size FORTRAN datatypes defined in MPI 1.1 */
        /************************************************************/
#if defined(HAVE_MPI_INTEGER1)
        case MPI_INTEGER1:
        {
            size = (MPI_Aint) 1;
            break;
        }
#endif
#if defined(HAVE_MPI_INTEGER2)
        case MPI_INTEGER2:
        {
            size = (MPI_Aint) 2;
            break;
        }
#endif
#if defined(HAVE_MPI_INTEGER4)
        case MPI_INTEGER4:
        {
            size = (MPI_Aint) 4;
            break;
        }
#endif
#if defined(HAVE_MPI_INTEGER8)
        case MPI_INTEGER8:
        {
            size = (MPI_Aint) 8;
            break;
        }
#endif
#if defined(HAVE_MPI_INTEGER16)
        case MPI_INTEGER16
        {
            size = (MPI_Aint) 16;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL2)
            case MPI_REAL2:
        {
            size = (MPI_Aint) 2;
            break;
        }
#endif
        /************************************************************/
        /* Additional optional FORTRAN datatypes defined in MPI 2.0 */
        /************************************************************/
#if defined(HAVE_MPI_REAL4)
        case MPI_REAL4:
        {
            size = (MPI_Aint) 4;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL8)
        case MPI_REAL8:
        {
            size = (MPI_Aint) 8;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL16)
        case MPI_REAL16:
        {
            size = (MPI_Aint) 16;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL4)
        case MPI_COMPLEX8:
        {
            size = (MPI_Aint) 8;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL8)
        case MPI_COMPLEX16:
        {
            size = (MPI_Aint) 16;
            break;
        }
#endif
#if defined(HAVE_MPI_REAL16)
        case MPI_COMPLEX32:
        {
            size = (MPI_Aint) 32;
            break;
        }
#endif
#endif
        default:
        {
            char err_str[256];
            
            /* TODO: generate returnable error code rather than aborting */
            MPIU_Snprintf(err_str, 1024, "ERROR: unknown MPI datatype, el_type=" MPIG_HANDLE_FMT, el_type);
            MPID_Abort(NULL, MPI_SUCCESS, 13, err_str);
            break;
        }
    } /* end switch() */

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_CONTIG_UNPACK_EXTERNAL32_TO_BUF); */

    MPIG_FUNC_ENTER(MPID_STATE_mpig_segment_globus_dc_sizeof_datatype);
    return size;

} /* end mpig_segment_globus_dc_sizeof_datatype() */

/********************************/
/* EXTERNALLY VISIBLE FUNCTIONS */
/********************************/

/* this is the one and only function we're here for :-) */
void mpig_segment_globus_dc_unpack(
    struct DLOOP_Segment *segp, /* dest */
    DLOOP_Offset first,         /* src  */
    DLOOP_Offset *lastp,        /* src  */
    DLOOP_Buffer unpack_buffer, /* src  */
    int src_format, 
    mpig_vc_t *vc)
{
    struct mpig_segment_globus_dc_piece_params unpack_params;
    /* MPIDI_STATE_DECL(MPID_STATE_mpig_segment_globus_dc_unpack); */
    
    /* MPIDI_FUNC_ENTER(MPID_STATE_mpig_segment_globus_dc_unpack); */

    unpack_params.src_buffer = unpack_buffer;
    unpack_params.src_format = src_format;
    unpack_params.vc         = vc;

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
        mpig_segment_globus_dc_sizeof_datatype,
        (void *) &unpack_params);

    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_SEGMENT_UNPACK_EXTERNAL); */
    return;

}
/* end mpig_segment_globus_dc_unpack() */

#endif /* if defined(HAVE_GLOBUS_DC_MODULE) */
