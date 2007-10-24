/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2002 Argonne National Laboratory
 *
 * See COPYRIGHT.txt in the src/mpid/globus directory.
 */

#include "mpidimpl.h"

const char * const mpig_ctype_strings[] =
{
    "INVALID",
    "float",
    "double",
    "long_double",
    "char",
    "short",
    "int",
    "long",
    "long_long",
    "unsigned_char",
    "unsigned_short",
    "unsigned_int",
    "unsigned_long",
    "unsigned_long_long",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "LAST"
};

#define mpig_datatype_set_ctype_map(dt_, ctype_)                                                                                \
{                                                                                                                               \
    MPIU_Assert(HANDLE_GET_MPI_KIND(dt_) == MPID_DATATYPE);                                                                     \
    if ((dt_) != MPI_DATATYPE_NULL)                                                                                             \
    {                                                                                                                           \
        MPIU_Assert(HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN);                                                               \
        MPIU_Assert(MPID_Datatype_get_basic_id(dt_) >= 0 && MPID_Datatype_get_basic_id(dt_) < MPIG_DATATYPE_MAX_BASIC_TYPES);   \
        mpig_process.my_dfd.mpi_ctype_map[MPID_Datatype_get_basic_id(dt_)] = (char)(ctype_);                                    \
    }                                                                                                                           \
}

#define mpig_datatype_set_num_ctypes(dt_, num_)                                                                                 \
{                                                                                                                               \
    MPIU_Assert(HANDLE_GET_MPI_KIND(dt_) == MPID_DATATYPE);                                                                     \
    if ((dt_) != MPI_DATATYPE_NULL)                                                                                             \
    {                                                                                                                           \
        MPIU_Assert(HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN);                                                               \
        MPIU_Assert(MPID_Datatype_get_basic_id(dt_) >= 0 && MPID_Datatype_get_basic_id(dt_) < MPIG_DATATYPE_MAX_BASIC_TYPES);   \
        mpig_process.dt_num_ctypes[MPID_Datatype_get_basic_id(dt_)] = (char)(num_);                                             \
    }                                                                                                                           \
}

#define mpig_datatype_set_local_sizeof_ctype(ctype_, size_)     \
{                                                               \
    mpig_process.my_dfd.sizeof_ctypes[ctype_] = (char)(size_);  \
}

/*
 * <mpi_errno> mpig_datatype_init(void)
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_init
int mpig_datatype_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int i;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_init);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering"));

    MPIU_Assert(MPIG_MAX_CTYPES == MPIG_CTYPE_LAST);

    mpig_process.my_dfd.hetero = FALSE;
    mpig_process.my_dfd.endian = MPIG_MY_ENDIAN;
    mpig_process.my_dfd.gdc_format = GLOBUS_DC_FORMAT_LOCAL;
    
    /* create MPI datatype to C type mapping information for the local process */
    for (i = 0; i < MPIG_DATATYPE_MAX_BASIC_TYPES; i++)
    {
	mpig_process.my_dfd.mpi_ctype_map[i] = MPIG_CTYPE_INVALID;
    }

    mpig_datatype_set_ctype_map(MPI_CHAR, MPIG_CTYPE_CHAR);
    mpig_datatype_set_ctype_map(MPI_SIGNED_CHAR, MPIG_CTYPE_CHAR);
    mpig_datatype_set_ctype_map(MPI_UNSIGNED_CHAR, MPIG_CTYPE_UNSIGNED_CHAR);
    mpig_datatype_set_ctype_map(MPI_BYTE, MPIG_CTYPE_CHAR);
    mpig_datatype_set_ctype_map(MPI_WCHAR, MPIG_C_WCHAR_T_CTYPE);
    mpig_datatype_set_ctype_map(MPI_SHORT, MPIG_CTYPE_SHORT);
    mpig_datatype_set_ctype_map(MPI_UNSIGNED_SHORT, MPIG_CTYPE_UNSIGNED_SHORT);
    mpig_datatype_set_ctype_map(MPI_INT, MPIG_CTYPE_INT);
    mpig_datatype_set_ctype_map(MPI_UNSIGNED, MPIG_CTYPE_UNSIGNED_INT);
    mpig_datatype_set_ctype_map(MPI_LONG, MPIG_CTYPE_LONG);
    mpig_datatype_set_ctype_map(MPI_UNSIGNED_LONG, MPIG_CTYPE_UNSIGNED_LONG);
    mpig_datatype_set_ctype_map(MPI_FLOAT, MPIG_CTYPE_FLOAT);
    mpig_datatype_set_ctype_map(MPI_DOUBLE, MPIG_CTYPE_DOUBLE);
    mpig_datatype_set_ctype_map(MPI_LONG_DOUBLE, MPIG_CTYPE_LONG_DOUBLE);
    mpig_datatype_set_ctype_map(MPI_LONG_LONG, MPIG_CTYPE_LONG_LONG);
    /* mpig_datatype_set_ctype_map(MPI_LONG_LONG_INT, MPIG_CTYPE_LONG_LONG); -- MPI_LONG_LONG_INT = MPI_LONG_LONG in mpi.h */
    mpig_datatype_set_ctype_map(MPI_UNSIGNED_LONG_LONG, MPIG_CTYPE_UNSIGNED_LONG_LONG);
    mpig_datatype_set_ctype_map(MPI_PACKED, MPIG_CTYPE_CHAR);
    mpig_datatype_set_ctype_map(MPI_LB, MPIG_CTYPE_INVALID);
    mpig_datatype_set_ctype_map(MPI_UB, MPIG_CTYPE_INVALID);
    mpig_datatype_set_ctype_map(MPI_2INT, MPIG_CTYPE_INT);
#   if defined(HAVE_FORTRAN_BINDING)
    {
        mpig_datatype_set_ctype_map(MPI_CHARACTER, MPIG_F77_CHARACTER_CTYPE);
        mpig_datatype_set_ctype_map(MPI_LOGICAL, MPIG_F77_LOGICAL_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER, MPIG_F77_INTEGER_CTYPE);
        mpig_datatype_set_ctype_map(MPI_REAL, MPIG_F77_REAL_CTYPE);
        mpig_datatype_set_ctype_map(MPI_DOUBLE_PRECISION, MPIG_F77_DOUBLE_PRECISION_CTYPE);
        mpig_datatype_set_ctype_map(MPI_COMPLEX, MPIG_F77_COMPLEX_CTYPE);
        mpig_datatype_set_ctype_map(MPI_DOUBLE_COMPLEX, MPIG_F77_DOUBLE_COMPLEX_CTYPE);
        mpig_datatype_set_ctype_map(MPI_2INTEGER, MPIG_F77_2INTEGER_CTYPE);
        mpig_datatype_set_ctype_map(MPI_2REAL, MPIG_F77_2REAL_CTYPE);
        mpig_datatype_set_ctype_map(MPI_2DOUBLE_PRECISION, MPIG_F77_2DOUBLE_PRECISION_CTYPE);
        mpig_datatype_set_ctype_map(MPI_2COMPLEX, MPIG_F77_2COMPLEX_CTYPE);
        mpig_datatype_set_ctype_map(MPI_2DOUBLE_COMPLEX, MPIG_F77_2DOUBLE_COMPLEX_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER1, MPIG_F77_INTEGER1_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER2, MPIG_F77_INTEGER2_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER4, MPIG_F77_INTEGER4_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER8, MPIG_F77_INTEGER8_CTYPE);
        mpig_datatype_set_ctype_map(MPI_INTEGER16, MPIG_F77_INTEGER16_CTYPE);
        mpig_datatype_set_ctype_map(MPI_REAL4, MPIG_F77_REAL4_CTYPE);
        mpig_datatype_set_ctype_map(MPI_REAL8, MPIG_F77_REAL8_CTYPE);
        mpig_datatype_set_ctype_map(MPI_REAL16, MPIG_F77_REAL16_CTYPE);
        mpig_datatype_set_ctype_map(MPI_COMPLEX8, MPIG_F77_COMPLEX8_CTYPE);
        mpig_datatype_set_ctype_map(MPI_COMPLEX16, MPIG_F77_COMPLEX16_CTYPE);
        mpig_datatype_set_ctype_map(MPI_COMPLEX32, MPIG_F77_COMPLEX32_CTYPE);
    }
#   endif

#   if defined(HAVE_CXX_BINDING)
    {
        /*
         * FIXME: add C++ to C type mappings for MPI::BOOL, MPI::COMPLEX, MPI::DOUBLE_COMPLEX, and MPI::LONG_DOUBLE_COMPLEX
         */
    }
#   endif

    /* set the local size of each C type in the sizof_ctypes array */
    for (i = 0; i < MPIG_CTYPE_LAST; i++)
    {
        mpig_process.my_dfd.sizeof_ctypes[i] = (char) 0;
    }
    
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_FLOAT, sizeof(float));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_DOUBLE, sizeof(double));
#   if defined(HAVE_LONG_DOUBLE)
    {
        mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_LONG_DOUBLE, sizeof(long double));
    }
#   endif
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_CHAR, sizeof(char));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_SHORT, sizeof(short));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_INT, sizeof(int));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_LONG, sizeof(long));
#   if defined(HAVE_LONG_LONG)
    {
        mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_LONG_LONG, sizeof(long long));
    }
#   endif
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_UNSIGNED_CHAR, sizeof(unsigned char));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_UNSIGNED_SHORT, sizeof(unsigned short));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_UNSIGNED_INT, sizeof(unsigned int));
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_UNSIGNED_LONG, sizeof(unsigned long));
#   if defined(HAVE_LONG_LONG)
    {
        mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_UNSIGNED_LONG_LONG, sizeof(unsigned long long));
    }
#   endif
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_INT8, 1);
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_INT16, 2);
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_INT32, 4);
    mpig_datatype_set_local_sizeof_ctype(MPIG_CTYPE_INT64, 8);

    /* set the number of C types needed to represent each MPI datatype */
    for (i = 0; i < MPIG_DATATYPE_MAX_BASIC_TYPES; i++)
    {
        if (mpig_process.my_dfd.mpi_ctype_map[i] != MPIG_CTYPE_INVALID)
        {
            mpig_process.dt_num_ctypes[i] = 1;
        }
        else
        {
            mpig_process.dt_num_ctypes[i] = 0;
        }
    }

    mpig_datatype_set_num_ctypes(MPI_2INT, 2);
    mpig_datatype_set_num_ctypes(MPI_2INTEGER, 2);
    mpig_datatype_set_num_ctypes(MPI_2REAL, 2);
    mpig_datatype_set_num_ctypes(MPI_2DOUBLE_PRECISION, 2);
    mpig_datatype_set_num_ctypes(MPI_COMPLEX, 2);
    mpig_datatype_set_num_ctypes(MPI_DOUBLE_COMPLEX, 2);
    mpig_datatype_set_num_ctypes(MPI_COMPLEX8, 2);
    mpig_datatype_set_num_ctypes(MPI_COMPLEX16, 2);
    mpig_datatype_set_num_ctypes(MPI_COMPLEX32, 2);
    mpig_datatype_set_num_ctypes(MPI_2COMPLEX, 4);
    mpig_datatype_set_num_ctypes(MPI_2DOUBLE_COMPLEX, 4);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_init);
    return mpi_errno;
}
/* end mpig_datatype_init() */

/*
 * <mpi_errno> mpig_datatype_finalize(void)
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_finalize
int mpig_datatype_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_finalize);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering"));

    /* ... nothing to do ... */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_finalize);
    return mpi_errno;
}
/* end mpig_datatype_finalize() */

/*
 * <mpi_errno> mpig_datatype_add_info_to_bc([IN/MOD] bc)
 *
 * Paramters:
 *
 *   bc - [IN/MOD] business card to augment with datatype information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_add_info_to_bc
int mpig_datatype_add_info_to_bc(mpig_bc_t * const bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int i;
    char uint_str[10];
    char ctype_map_str[MPIG_DATATYPE_MAX_BASIC_TYPES * 2 + 1];
    char ctype_sizes_str[MPIG_CTYPE_LAST * 2 + 1];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_add_info_to_bc);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_add_info_to_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "entering: bc=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(bc)));

    /* add the endianess of the local system to the business card */
    mpi_errno = mpig_bc_add_contact(bc, "DT_ENDIAN", MPIG_ENDIAN_STR(MPIG_MY_ENDIAN));
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|bc_add_contact",
        "**mpig|bc_add_contact %s", "DT_ENDIAN");

    /* add the globus data conversion format if we are using globus_dc module */
    MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) GLOBUS_DC_FORMAT_LOCAL);
    mpi_errno = mpig_bc_add_contact(bc, "DT_GDC_FORMAT", uint_str);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|bc_add_contact",
        "**mpig|bc_add_contact %s", "DT_GDC_FORMAT");
    
    /* prepare a text version of the MPI datatype to C type mappings */
    for (i = 0; i < MPIG_DATATYPE_MAX_BASIC_TYPES; i++)
    {
        char str[3];
	MPIU_Snprintf(str, 3, "%02x", (int) mpig_process.my_dfd.mpi_ctype_map[i]);
        ctype_map_str[i*2] = str[0];
        ctype_map_str[i*2+1] = str[1];
    }
    ctype_map_str[MPIG_DATATYPE_MAX_BASIC_TYPES * 2] = '\0';

    /* add mappings to the business card */
    mpi_errno = mpig_bc_add_contact(bc, "DT_CTYPE_MAP", ctype_map_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|datatype_add_ctype_map");

    /* prepare a text version of the local C type size array */
    for (i = 0; i < MPIG_CTYPE_LAST; i++)
    {
        char str[3];
	MPIU_Snprintf(str, 3, "%02x", (int) mpig_process.my_dfd.sizeof_ctypes[i]);
        ctype_sizes_str[i*2] = str[0];
        ctype_sizes_str[i*2+1] = str[1];
    }
    ctype_sizes_str[MPIG_CTYPE_LAST * 2] = '\0';

    /* add the local C type sizes to the business card */
    mpi_errno = mpig_bc_add_contact(bc, "DT_CTYPE_SIZES", ctype_sizes_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|datatype_add_ctype_sizes");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "exiting: bc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT,  MPIG_PTR_CAST(bc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_add_info_to_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_add_info_to_bc() */

/*
 * <mpi_errno> mpig_datatype_extract_info_from_bc([IN/MOD] vc)
 *
 * Paramters:
 *
 *   vc - [IN/MOD] virtual connection to augment with extracted information
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_extract_info_from_bc
int mpig_datatype_extract_info_from_bc(mpig_vc_t * const vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t * bc = mpig_vc_get_bc(vc);
    char * ctype_map_str = NULL;
    char * ctype_sizes_str = NULL;
    char * gdc_format_str = NULL;
    char * endian_str = NULL;
    int gdc_format;
    mpig_endian_t endian;
    int i;
    bool_t found;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_extract_info_from_bc);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_extract_info_from_bc);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "entering: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(bc), MPIG_PTR_CAST(vc)));

    /* get the endianess of source system */
    mpi_errno = mpig_bc_get_contact(bc, "DT_ENDIAN", &endian_str, &found);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|bc_get_contact",
        "**mpig|bc_get_contact %s", "DT_ENDIAN");
    if (!found) goto fn_return;

    endian = (strcmp(endian_str, "little") == 0) ? MPIG_ENDIAN_LITTLE : MPIG_ENDIAN_BIG;
	
    /* get the globus data conversion format if we are using globus_dc module */
    mpi_errno = mpig_bc_get_contact(bc, "DT_GDC_FORMAT", &gdc_format_str, &found);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**mpig|bc_get_contact",
        "**mpig|bc_get_contact %s", "DT_GDC_FORMAT");
    if (!found) goto fn_return;

    rc = sscanf(gdc_format_str, "%d", &gdc_format);
    MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

    /* get the MPI datatype to C type map */
    mpi_errno = mpig_bc_get_contact(bc, "DT_CTYPE_MAP", &ctype_map_str, &found);
    MPIU_ERR_CHKANDJUMP2((mpi_errno || found == FALSE), mpi_errno, MPI_ERR_INTERN, "**mpig|datatype_extract_ctype_map",
	"**mpig|datatype_extract_ctype_map %s %d", mpig_vc_get_pg(vc), mpig_vc_get_pg_rank(vc));

    /* get the the C type size information */
    mpi_errno = mpig_bc_get_contact(bc, "DT_CTYPE_SIZES", &ctype_sizes_str, &found);
    MPIU_ERR_CHKANDJUMP2((mpi_errno || found == FALSE), mpi_errno, MPI_ERR_INTERN, "**mpig|datatype_extract_ctype_sizes",
	"**mpig|datatype_extract_ctype_sizes %s %d", mpig_vc_get_pg(vc), mpig_vc_get_pg_rank(vc));

    MPIU_Assert(strlen(ctype_map_str) == MPIG_DATATYPE_MAX_BASIC_TYPES * 2);
    MPIU_Assert(strlen(ctype_sizes_str) == MPIG_CTYPE_LAST * 2);

    /* if all information was successfully extracted, then copy the information into the VC */
    mpig_dfd_set_endian(&vc->dfd, endian);
    mpig_dfd_set_gdc_format(&vc->dfd, gdc_format);
    
    /* extract the MPI datatype to C type mapping information and store in the VC */
    for (i = 0; i < MPIG_DATATYPE_MAX_BASIC_TYPES; i++)
    {
        char str[3];
        int ctype;
        str[0] = ctype_map_str[i*2];
        str[1] = ctype_map_str[i*2+1];
        str[2] = '\0';
        sscanf(str, "%02x", &ctype);
	vc->dfd.mpi_ctype_map[i] = (char) ctype;
    }
    
    /* extract the C type size information and store in the VC */
    for (i = 0; i < MPIG_CTYPE_LAST; i++)
    {
        char str[3];
        int size;
        str[0] = ctype_sizes_str[i*2];
        str[1] = ctype_sizes_str[i*2+1];
        str[2] = '\0';
        sscanf(str, "%02x", &size);
	vc->dfd.sizeof_ctypes[i] = (char) size;
    }

    vc->dfd.hetero = mpig_dfd_compare(&vc->dfd, &mpig_process.my_dfd) ? FALSE : TRUE;
    
  fn_return:
    if (gdc_format_str != NULL) mpig_bc_free_contact(gdc_format_str);
    if (endian_str != NULL) mpig_bc_free_contact(endian_str);
    if (ctype_map_str != NULL) mpig_bc_free_contact(ctype_map_str);
    if (ctype_sizes_str != NULL) mpig_bc_free_contact(ctype_sizes_str);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC | MPIG_DEBUG_LEVEL_DT, "exiting: bc=" MPIG_PTR_FMT
	"vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(bc), MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_extract_info_from_bc);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_extract_info_from_bc() */


#if defined(MPID_HAS_HETERO)

/*
 * <mpi_errno> mpig_datatype_pack(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_pack
int mpig_datatype_pack(void * inbuf, int incount, MPI_Datatype dt, void * outbuf, int outsize, int * position, MPID_Comm * comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPI_Aint dt_size;
    MPID_Segment seg;
    MPI_Aint last;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_pack);
    
    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_pack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "entering: inbuf=" MPIG_PTR_FMT
        ", incount=%d, dt=" MPIG_HANDLE_FMT ", outbuf=" MPIG_PTR_FMT "outsize=%d, position=%d", MPIG_PTR_CAST(inbuf), incount, dt,
        MPIG_PTR_CAST(outbuf), outsize, *position));

    MPID_Datatype_get_size_macro(dt, dt_size);
    
    if (*position == 0)
    {
        mpig_dfd_pack_header(&mpig_process.my_dfd, outbuf);
        *position = MPIG_PACK_HEADER_SIZE;
    }

    /* this error checking must occur after position has been adjusted for the header */
#   if defined(HAVE_ERROR_CHECKING)
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (dt_size * incount > outsize - *position)
            {
                MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_ARG, "**argpackbuf",
                    "**argpackbuf %d %d", dt_size * incount, outsize - *position );
            }
        }
    }
#   endif /* defined(HAVE_ERROR_CHECKING) */

    MPID_Segment_init(inbuf, incount, dt, &seg, MPID_DATALOOP_HOMOGENEOUS);
    
    last = SEGMENT_IGNORE_LAST;
    MPID_Segment_pack(&seg, 0, &last, (void *) ((char *) outbuf + *position));
    if (last != dt_size * incount)
    {
        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_ARG, "**mpig|dt_pack_failed",
            "**mpig|dt_pack_failed %d %d", dt_size * incount, outsize - *position );
    }
    
    *position += (int) last;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "exiting: inbuf=" MPIG_PTR_FMT
        ", incount=%d, dt=" MPIG_HANDLE_FMT ", outbuf=" MPIG_PTR_FMT ",outsize=%d, position=%d , mpi_errno=" MPIG_ERRNO_FMT,
        MPIG_PTR_CAST(inbuf), incount, dt, MPIG_PTR_CAST(outbuf), outsize, *position, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_pack);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_pack() */

/*
 * <mpi_errno> mpig_datatype_unpack(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_unpack
int mpig_datatype_unpack(void * inbuf, int insize, int * position, void * outbuf, int outcount, MPI_Datatype dt, MPID_Comm * comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_data_format_descriptor_t dfd;
    MPID_Segment seg;
    MPI_Aint last;
    MPI_Aint dt_size;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_unpack);
    
    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_unpack);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "entering: inbuf=" MPIG_PTR_FMT
        ", insize=%d, dt=" MPIG_HANDLE_FMT ", outbuf=" MPIG_PTR_FMT "outcoutn=%d, position=%d", MPIG_PTR_CAST(inbuf), insize, dt,
        MPIG_PTR_CAST(outbuf), outcount, *position));

    MPIU_ERR_CHKANDJUMP2((insize < MPIG_PACK_HEADER_SIZE), mpi_errno, MPI_ERR_OTHER, "**mpig|unpack_header_incomplete",
        "**mpig|unpack_header_incomplete %d %d", MPIG_PACK_HEADER_SIZE, insize);

    /* first, we must determine if the buffer data is in a format compatible with the local format.  if would be nice if we could
       store that information in the header of the packed buffer for future calls to unpack, but the C++ binding declares inbuf
       as const, so we must check the header each time. */
    mpig_dfd_unpack_header(&dfd, inbuf);
    if (*position == 0) *position = MPIG_PACK_HEADER_SIZE;

    dt_size = mpig_dfd_get_datatype_size(&dfd, dt);
    
    /* this error checking must occur after position has been adjusted for the header */
#   if defined(HAVE_ERROR_CHECKING)
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (dt_size * outcount > insize - *position)
            {
                MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_ARG, "**mpig|argunpackbuf",
                    "**mpig|argunpackbuf %d %d", dt_size * outcount, insize - *position );
            }
        }
    }
#   endif /* defined(HAVE_ERROR_CHECKING) */

    last = SEGMENT_IGNORE_LAST;

    mpig_segment_init(outbuf, outcount, dt, &dfd, &seg);
    mpig_segment_unpack(&seg, &dfd, 0, &last, (void *) ((char *) inbuf + *position));
    
    if (last != dt_size * outcount)
    {
        MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_ARG, "**mpig|dt_unpack_failed",
            "**mpig|dt_unpack_failed %d %d", dt_size * outcount, insize - *position );
    }
    
    *position += (int) last;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "exiting: inbuf=" MPIG_PTR_FMT
        ", insize=%d, dt=" MPIG_HANDLE_FMT ", outbuf=" MPIG_PTR_FMT ",outcount=%d, position=%d ,mpi_errno=" MPIG_ERRNO_FMT,
        MPIG_PTR_CAST(inbuf), insize, dt, MPIG_PTR_CAST(outbuf), outcount, *position, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_unpack);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_datatype_unpack() */

/*
 * <mpi_errno> mpig_datatype_pack_size(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_datatype_pack_size
int mpig_datatype_pack_size(int incount, MPI_Datatype dt, int rank, MPID_Comm * comm, int * size_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int dt_size;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_datatype_pack_size);
    
    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_datatype_pack_size);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "entering: incount=%d, dt="
        MPIG_HANDLE_FMT, incount, dt));

    if (rank == MPI_PROC_NULL || rank == comm->rank)
    {
        MPID_Datatype_get_size_macro(dt, dt_size);
        *size_p = MPIG_PACK_HEADER_SIZE + incount * dt_size;
    }
    else
    {
        mpig_vc_t * vc = mpig_comm_get_remote_vc(comm, rank);
        *size_p = MPIG_PACK_HEADER_SIZE + incount * mpig_dfd_get_datatype_size(&vc->dfd, dt);
    }
        
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DT, "exiting: incount=%d, dt="
        MPIG_HANDLE_FMT ", size=%d, mpi_errno=" MPIG_ERRNO_FMT, incount, dt, *size_p, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_datatype_pack_size);
    return mpi_errno;
}
/* end mpig_datatype_pack_size() */

#endif /* defined(MPID_HAS_HETERO) */

/*
 * <mpi_errno> mpig_dfd_get_datatype_size
 */
#undef FUNCNAME
#define FUNCNAME mpig_dfd_get_datatype_size
MPIU_Size_t mpig_dfd_get_datatype_size(const mpig_data_format_descriptor_t * const dfd, const MPI_Datatype dt)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_Size_t size;
    
    MPIG_UNUSED_VAR(fcname);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "entering: dt=" MPIG_HANDLE_FMT ", hetero=%s", dt,
        MPIG_BOOL_STR(mpig_dfd_is_hetero(dfd))));
    
    if (mpig_dfd_is_hetero(dfd) == FALSE)
    {
        MPID_Datatype_get_size_macro(dt, size);
    }
    else
    {
#       if defined(MPID_HAS_HETERO)
        {
            if (HANDLE_GET_KIND(dt) == HANDLE_KIND_BUILTIN)
            {
                mpig_ctype_t ctype = mpig_dfd_get_mpi_ctype_mapping(dfd, dt);
                int sizeof_ctype = mpig_dfd_get_sizeof_ctype(dfd, ctype);
                int mult = mpig_datatype_get_num_ctypes(dt);

                size = (MPI_Aint) mult * sizeof_ctype;

                /* 
                 * MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "dt=" MPIG_HANDLE_FMT ", ctype=%s, sizeof_ctype=%d, mult=%d, size="
                 *     MPIG_AINT_FMT, dt, mpig_ctype_get_string(ctype), sizeof_ctype, mult, size));
                 */
            }
            else
            {
                MPID_Dataloop * dlp = NULL;
                mpig_segment_piece_params_t params;
            
                params.src_buffer = NULL;
                params.src_dfd = dfd;
                MPID_Datatype_get_loopptr_macro(dt, dlp, MPID_DATALOOP_HETEROGENEOUS);
                size = MPID_Dataloop_stream_size(dlp, mpig_segment_sizeof_source_basic_datatype, &params);
            }
        }
#       else /* !defined(MPID_HAS_HETERO) */
        {
            MPIU_Assert(FALSE && "MPID_HAS_HETERO is not defined");
        }
#       endif /* if/else defined(MPID_HAS_HETERO) */
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "exiting: dt=" MPIG_HANDLE_FMT ", hetero=%s, size=" MPIG_SIZE_FMT, dt,
        MPIG_BOOL_STR(mpig_dfd_is_hetero(dfd)), size));
    return size;
}
/* end mpig_dfd_get_datatype_size() */
