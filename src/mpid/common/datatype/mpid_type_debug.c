/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

static char *MPIDI_combiner_to_string(int combiner);
static char *MPIDI_datatype_builtin_to_string(MPI_Datatype type);

void MPIDI_Datatype_printf(MPI_Datatype type,
			   int depth,
			   MPI_Aint displacement,
			   int blocklength,
			   int header)
{
    char *string;
    MPI_Aint size, extent, true_lb, true_ub, lb, ub, sticky_lb, sticky_ub;

    if (HANDLE_GET_KIND(type) == HANDLE_KIND_BUILTIN) {
	string = MPIDI_datatype_builtin_to_string(type);
	if (type == MPI_LB) sticky_lb = 1;
	else sticky_lb = 0;
	if (type == MPI_UB) sticky_ub = 1;
	else sticky_ub = 0;
    }
    else {
	MPID_Datatype *type_ptr;

	MPID_Datatype_get_ptr(type, type_ptr);
	string = MPIDI_combiner_to_string(type_ptr->contents->combiner);
	sticky_lb = type_ptr->has_sticky_lb;
	sticky_ub = type_ptr->has_sticky_ub;
    }

    MPI_Type_size(type, &size);
    MPI_Type_get_true_extent(type, &true_lb, &extent);
    true_ub = extent + true_lb;
    MPI_Type_get_extent(type, &lb, &extent);
    ub = extent + lb;

#if 0
    lb      += displacement;
    true_lb += displacement;
    ub      += displacement;
    true_ub += displacement;
#endif

    if (header == 1) {
	/*               012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 */
	MPIU_dbg_printf("------------------------------------------------------------------------------------------------------------------------------------------\n");
	MPIU_dbg_printf("depth                   type         size       extent      true_lb      true_ub           lb(s)           ub(s)         disp       blklen\n");
	MPIU_dbg_printf("------------------------------------------------------------------------------------------------------------------------------------------\n");
    }
    MPIU_dbg_printf("%5d  %21s  %11d  %11d  %11d  %11d  %11d(%1d)  %11d(%1d)  %11d  %11d\n",
		    depth,
		    string,
		    size,
		    extent,
		    true_lb,
		    true_ub,
		    lb,
		    sticky_lb,
		    ub,
		    sticky_ub,
		    displacement,
		    blocklength);
    return;
}

/* longest string is 21 characters */
static char *MPIDI_datatype_builtin_to_string(MPI_Datatype type)
{
    static char t_char[]             = "MPI_CHAR";
    static char t_uchar[]            ="MPI_UNSIGNED_CHAR";
    static char t_byte[]             ="MPI_BYTE";
    static char t_wchar_t[]          = "MPI_WCHAR_T";
    static char t_short[]            = "MPI_SHORT";
    static char t_ushort[]           = "MPI_UNSIGNED_SHORT";
    static char t_int[]              = "MPI_INT";
    static char t_uint[]             = "MPI_UNSIGNED";
    static char t_long[]             = "MPI_LONG";
    static char t_ulong[]            = "MPI_UNSIGNED_LONG";
    static char t_float[]            = "MPI_FLOAT";
    static char t_double[]           = "MPI_DOUBLE";
    static char t_longdouble[]       = "MPI_LONG_DOUBLE";
    static char t_longlongint[]      = "MPI_LONG_LONG_INT";
    static char t_longlong[]         = "MPI_LONG_LONG";

    static char t_packed[]           = "MPI_PACKED";
    static char t_lb[]               = "MPI_LB";
    static char t_ub[]               = "MPI_UB";

    static char t_floatint[]         = "MPI_FLOAT_INT";
    static char t_doubleint[]        = "MPI_DOUBLE_INT";
    static char t_longint[]          = "MPI_LONG_INT";
    static char t_shortint[]         = "MPI_SHORT_INT";
    static char t_2int[]             = "MPI_2INT";
    static char t_longdoubleint[]    = "MPI_LONG_DOUBLE_INT";

    static char t_complex[]          = "MPI_COMPLEX";
    static char t_doublecomplex[]    = "MPI_DOUBLE_COMPLEX";
    static char t_logical[]          = "MPI_LOGICAL";
    static char t_real[]             = "MPI_REAL";
    static char t_doubleprecision[]  = "MPI_DOUBLE_PRECISION";
    static char t_integer[]          = "MPI_INTEGER";
    static char t_2integer[]         = "MPI_2INTEGER";
    static char t_2complex[]         = "MPI_2COMPLEX";
    static char t_2doublecomplex[]   = "MPI_2DOUBLE_COMPLEX";
    static char t_2real[]            = "MPI_2REAL";
    static char t_2doubleprecision[] = "MPI_2DOUBLE_PRECISION";
    static char t_character[]        = "MPI_CHARACTER";

    if (type == MPI_CHAR)              return t_char;
    if (type == MPI_UNSIGNED_CHAR)     return t_uchar;
    if (type == MPI_BYTE)              return t_byte;
    if (type == MPI_WCHAR_T)           return t_wchar_t;
    if (type == MPI_SHORT)             return t_short;
    if (type == MPI_UNSIGNED_SHORT)    return t_ushort;
    if (type == MPI_INT)               return t_int;
    if (type == MPI_UNSIGNED)          return t_uint;
    if (type == MPI_LONG)              return t_long;
    if (type == MPI_UNSIGNED_LONG)     return t_ulong;
    if (type == MPI_FLOAT)             return t_float;
    if (type == MPI_DOUBLE)            return t_double;
    if (type == MPI_LONG_DOUBLE)       return t_longdouble;
    if (type == MPI_LONG_LONG_INT)     return t_longlongint;
    if (type == MPI_LONG_LONG)         return t_longlong;
	
    if (type == MPI_PACKED)            return t_packed;
    if (type == MPI_LB)                return t_lb;
    if (type == MPI_UB)                return t_ub;
	
    if (type == MPI_FLOAT_INT)         return t_floatint;
    if (type == MPI_DOUBLE_INT)        return t_doubleint;
    if (type == MPI_LONG_INT)          return t_longint;
    if (type == MPI_SHORT_INT)         return t_shortint;
    if (type == MPI_2INT)              return t_2int;
    if (type == MPI_LONG_DOUBLE_INT)   return t_longdoubleint;
	
    if (type == MPI_COMPLEX)           return t_complex;
    if (type == MPI_DOUBLE_COMPLEX)    return t_doublecomplex;
    if (type == MPI_LOGICAL)           return t_logical;
    if (type == MPI_REAL)              return t_real;
    if (type == MPI_DOUBLE_PRECISION)  return t_doubleprecision;
    if (type == MPI_INTEGER)           return t_integer;
    if (type == MPI_2INTEGER)          return t_2integer;
    if (type == MPI_2COMPLEX)          return t_2complex;
    if (type == MPI_2DOUBLE_COMPLEX)   return t_2doublecomplex;
    if (type == MPI_2REAL)             return t_2real;
    if (type == MPI_2DOUBLE_PRECISION) return t_2doubleprecision;
    if (type == MPI_CHARACTER)         return t_character;
    
    return NULL;
}

/* MPIDI_combiner_to_string(combiner)
 *
 * Converts a numeric combiner into a pointer to a string used for printing.
 *
 * longest string is 16 characters.
 */
static char *MPIDI_combiner_to_string(int combiner)
{
    static char c_named[]    = "named";
    static char c_contig[]   = "contig";
    static char c_vector[]   = "vector";
    static char c_hvector[]  = "hvector";
    static char c_indexed[]  = "indexed";
    static char c_hindexed[] = "hindexed";
    static char c_struct[]   = "struct";
#ifdef HAVE_MPI2_COMBINERS
    static char c_dup[]              = "dup";
    static char c_hvector_integer[]  = "hvector_integer";
    static char c_hindexed_integer[] = "hindexed_integer";
    static char c_indexed_block[]    = "indexed_block";
    static char c_struct_integer[]   = "struct_integer";
    static char c_subarray[]         = "subarray";
    static char c_darray[]           = "darray";
    static char c_f90_real[]         = "f90_real";
    static char c_f90_complex[]      = "f90_complex";
    static char c_f90_integer[]      = "f90_integer";
    static char c_resized[]          = "resized";
#endif

    if (combiner == MPI_COMBINER_NAMED)      return c_named;
    if (combiner == MPI_COMBINER_CONTIGUOUS) return c_contig;
    if (combiner == MPI_COMBINER_VECTOR)     return c_vector;
    if (combiner == MPI_COMBINER_HVECTOR)    return c_hvector;
    if (combiner == MPI_COMBINER_INDEXED)    return c_indexed;
    if (combiner == MPI_COMBINER_HINDEXED)   return c_hindexed;
    if (combiner == MPI_COMBINER_STRUCT)     return c_struct;
#ifdef HAVE_MPI2_COMBINERS
    if (combiner == MPI_COMBINER_DUP)              return c_dup;
    if (combiner == MPI_COMBINER_HVECTOR_INTEGER)  return c_hvector_integer;
    if (combiner == MPI_COMBINER_HINDEXED_INTEGER) return c_hindexed_integer;
    if (combiner == MPI_COMBINER_INDEXED_BLOCK)    return c_indexed_block;
    if (combiner == MPI_COMBINER_STRUCT_INTEGER)   return c_struct_integer;
    if (combiner == MPI_COMBINER_SUBARRAY)         return c_subarray;
    if (combiner == MPI_COMBINER_DARRAY)           return c_darray;
    if (combiner == MPI_COMBINER_F90_REAL)         return c_f90_real;
    if (combiner == MPI_COMBINER_F90_COMPLEX)      return c_f90_complex;
    if (combiner == MPI_COMBINER_F90_INTEGER)      return c_f90_integer;
    if (combiner == MPI_COMBINER_RESIZED)          return c_resized;
#endif
    
    return NULL;
}
