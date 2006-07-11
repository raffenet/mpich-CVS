
#if defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)                                   
#include "mpidimpl.h"
#include "globus_dc.h"
#include "globus_xio_tcp_driver.h"

#define MPIG_LAN_ENV "MPIG_LAN_NET"
#define MPIG_WAN_ENV "MPIG_WAN_NET"
#define MPIG_SYSTEM_ENV "MPIG_SYSTEM_NET"


static xio_l_conn_info_t                xio_l_lan_info;
static xio_l_conn_info_t                xio_l_wan_info;
static xio_l_conn_info_t                xio_l_system_info;

/*********************************************************************
 *    INIT FUNCTIONS
 *    --------------
 ********************************************************************/
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_xxx_init
MPIG_STATIC int
mpig_cm_xio_xxx_init(
    const char *                        env,
    xio_l_conn_info_t *                 xio_info)
{
    static const char                   fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t                     grc;
    int                                 mpi_errno = MPI_SUCCESS;
    char *                              env_str; 
    char *                              tmp_str; 
    char *                              driver_name = NULL;
    char *                              driver_args = NULL; 
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_xxx_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_xxx_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    memset(xio_info, '\0', sizeof(xio_l_conn_info_t));

    globus_module_activate(GLOBUS_XIO_MODULE);
    /* just up the ref count on the xio module */
    mpi_errno = mpig_cm_xio_premodule_init();
    if(mpi_errno != MPI_SUCCESS)
    {
        goto error_init;
    }

    env_str = globus_libc_getenv(env);
    if(env_str == NULL)
    {
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, "entering"));
        /* in this case there is no special driver for the 
            particular network so we just copy the tcp fallback
            information and carry on */
        xio_info->available = GLOBUS_FALSE;

        mpi_errno = 0;
    }
    else
    {
        env_str = strdup(env_str);
        if(env_str == NULL)
        {
            goto error_malloc;
        }
        tmp_str = strchr(env_str, ':');
        if(tmp_str != NULL)
        {
            tmp_str = '\0';
            driver_args = tmp_str + 1;
        }
        driver_name = env_str;

        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, 
            "Init DRIVER NAME=%s : args=%s", driver_name, driver_args));

        xio_info->driver_name = driver_name;
        grc = globus_xio_stack_init(&xio_info->stack, NULL);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_stack;
        }
        grc = globus_xio_driver_load(xio_info->driver_name, &xio_info->driver);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_load;
        }
        grc = globus_xio_stack_push_driver(xio_info->stack, xio_info->driver);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_push;
        }
        grc = globus_xio_attr_init(&xio_info->attr);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_push;
        }
        if(driver_args != NULL)
        {
#if defined(NEW_XIO)
            /* i dont think we care about an error here.  these are just
            supossed to be hints */
            globus_xio_attr_cntl(
                xio_info->attr,
                xio_info->driver,
                GLOBUS_XIO_SET_STRING_OPTIONS,
                driver_args);
#endif
        }

        /***** XXX optimization parameters to the attr ******/

        grc = globus_xio_server_create(
            &xio_info->server, xio_info->attr, xio_info->stack);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_attr;
        }
        grc = globus_xio_server_get_contact_string(
            xio_info->server, &xio_info->contact_string);
        if(grc != GLOBUS_SUCCESS)
        {
            goto error_cs;
        }

        mpi_errno = mpig_cm_xio_server_listen(xio_info->server);
        xio_info->available = GLOBUS_TRUE;
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, 
            "SERVER LISTENING %s 0x%x", xio_info->contact_string,
            xio_info->server));
    }

    return mpi_errno;

error_cs:
    free(xio_info->contact_string);
error_attr:
    globus_xio_attr_destroy(xio_info->attr);
error_push:
    globus_xio_driver_unload(xio_info->driver);
error_load:
    globus_xio_stack_destroy(xio_info->stack);
error_stack:
    free(xio_info->driver_name);
error_malloc:
    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER,
        "**globus|cm_xio|conn_driver_load_transport",
        "**globus|cm_xio|conn_driver_destroy_load_transport %s",
        globus_error_print_chain(globus_error_peek(grc)));
error_init:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_init
MPIG_STATIC int
mpig_cm_xio_lan_init(
    int *                               argc,
    char ***                            argv)
{
    int                                 rc;

    rc = mpig_cm_xio_xxx_init(MPIG_LAN_ENV, &xio_l_lan_info);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, "LAN AVAILABLE? %d", 
        xio_l_lan_info.available));

    return rc;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_init
MPIG_STATIC int
mpig_cm_xio_system_init(
    int *                               argc,
    char ***                            argv)
{
    int                                 rc;

    rc = mpig_cm_xio_xxx_init(MPIG_SYSTEM_ENV, &xio_l_system_info);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, "SYSTEM AVAILABLE? %d", 
        xio_l_lan_info.available));

    return rc;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_init
MPIG_STATIC int
mpig_cm_xio_wan_init(
    int *                               argc,
    char ***                            argv)
{
    int                                 rc;

    rc = mpig_cm_xio_xxx_init(MPIG_WAN_ENV, &xio_l_wan_info);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, "WAN AVAILABLE? %d", 
        xio_l_lan_info.available));
    return rc;
}


/*********************************************************************
 *    FINALIZE FUNCTIONS
 *    ------------------
 ********************************************************************/

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_xxx_finalize
MPIG_STATIC int
mpig_cm_xio_xxx_finalize(
    xio_l_conn_info_t *                 xio_info)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_xxx_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_xxx_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    /* shutdown the connection management subsystem */
    if(!xio_info->available)
    {
        return MPI_SUCCESS;
    }

    grc = globus_xio_server_close(xio_info->server);
    if(grc != GLOBUS_SUCCESS)
    {
    }

    /* free the rest of the struct */
    free(xio_info->driver_name);
    free(xio_info->contact_string);
    globus_xio_driver_unload(xio_info->driver);
    globus_xio_attr_destroy(xio_info->attr);
   
    /* decrement xio module reference count */
    globus_module_deactivate(GLOBUS_XIO_MODULE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_finalize
MPIG_STATIC int
mpig_cm_xio_lan_finalize(void)
{
    return mpig_cm_xio_xxx_finalize(&xio_l_lan_info);
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_finalize
MPIG_STATIC int
mpig_cm_xio_wan_finalize(void)
{
    return mpig_cm_xio_xxx_finalize(&xio_l_wan_info);
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_finalize
MPIG_STATIC int
mpig_cm_xio_system_finalize(void)
{
    return mpig_cm_xio_xxx_finalize(&xio_l_system_info);
}

/*********************************************************************
 *    ADD CONTACT FUNCTIONS
 *    ---------------------
 ********************************************************************/

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_add_contact_info
MPIG_STATIC int
mpig_cm_xio_lan_add_contact_info(
    mpig_bc_t * const                   bc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_lan_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_lan_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));
    
    if(xio_l_lan_info.available)
    {
        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_LAN_PROTO_NAME",
            xio_l_lan_info.driver_name);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_LAN_PROTO_NAME");

        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_LAN_CONTACT_STRING",
            xio_l_lan_info.contact_string);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_LAN_CONTACT_STRING");
    }

fn_fail:
    MPIG_DEBUG_PRINTF(
        (MPIG_DEBUG_LEVEL_FUNC,
        "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_lan_add_contact_info);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_add_contact_info
MPIG_STATIC int
mpig_cm_xio_wan_add_contact_info(
    mpig_bc_t * const                   bc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_wan_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_wan_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    if(xio_l_wan_info.available)
    {
        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_WAN_PROTO_NAME",
            xio_l_wan_info.driver_name);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_WAN_PROTO_NAME");

        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_WAN_CONTACT_STRING",
            xio_l_wan_info.contact_string);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_WAN_CONTACT_STRING");
    }
fn_fail:
    MPIG_DEBUG_PRINTF(
        (MPIG_DEBUG_LEVEL_FUNC,
        "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_wan_add_contact_info);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_add_contact_info
MPIG_STATIC int
mpig_cm_xio_system_add_contact_info(
    mpig_bc_t * const                   bc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_system_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_system_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    if(xio_l_system_info.available)
    {
        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_SYSTEM_PROTO_NAME",
            xio_l_system_info.driver_name);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_SYSTEM_PROTO_NAME");

        mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_SYSTEM_CONTACT_STRING",
            xio_l_system_info.contact_string);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
            "**globus|bc_add_contact",
            "**globus|bc_add_contact %s", "CM_XIO_SYSTEM_CONTACT_STRING");
    }
fn_fail:
    MPIG_DEBUG_PRINTF(
        (MPIG_DEBUG_LEVEL_FUNC,
        "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_system_add_contact_info);
    return mpi_errno;
}


/*********************************************************************
 *    EXTRACT FUNCTIONS
 *    -----------------
 ********************************************************************/

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_xxx_extract_contact_info
MPIG_STATIC int
mpig_cm_xio_xxx_extract_contact_info(
    mpig_vc_t * const                   vc,
    int                                 level,
    xio_l_conn_info_t *                 xio_info)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_xxx_extract_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_xxx_extract_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
        "entering: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));

    vc->cm.xio.xio_info = NULL;
    if(xio_info->available)
    {
        vc->ci.topology_levels |= level;
    }
    /*  fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc="
        MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc,
        mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_xxx_extract_contact_info);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_extract_contact_info
MPIG_STATIC int
mpig_cm_xio_lan_extract_contact_info(
    mpig_vc_t * const                   vc)
{
    return mpig_cm_xio_xxx_extract_contact_info(
        vc,
        MPIG_TOPOLOGY_LEVEL_LAN_MASK,
        &xio_l_lan_info);
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_extract_contact_info
MPIG_STATIC int
mpig_cm_xio_wan_extract_contact_info(
    mpig_vc_t * const                   vc)
{
    return mpig_cm_xio_xxx_extract_contact_info(
        vc,
        MPIG_TOPOLOGY_LEVEL_WAN_MASK,
        &xio_l_wan_info);
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_extract_contact_info
MPIG_STATIC int
mpig_cm_xio_system_extract_contact_info(
    mpig_vc_t * const                   vc)
{
    return mpig_cm_xio_xxx_extract_contact_info(
        vc,
        MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK,
        &xio_l_system_info);
}


/*********************************************************************
 *    EXTRACT FUNCTIONS
 *    -----------------
 ********************************************************************/

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_xxx_select_module
MPIG_STATIC int
mpig_cm_xio_xxx_select_module(
    mpig_vc_t * const                   vc, 
    const char *                        proto_name,
    const char *                        cs_name,
    xio_l_conn_info_t *                 xio_info,
    bool_t * const                      selected)
{
    static const char                   fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t *                         bc;
    char *                              version_str = NULL;
    char *                              contact_str = NULL;
    char *                              format_str = NULL;
    char *                              endian_str = NULL;
    int                                 format;
    int                                 version;
    mpig_endian_t                       endian;
    bool_t                              found;
    int                                 rc;
    int                                 mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_select_module);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_select_module);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
        "entering: vc=" MPIG_PTR_FMT, (MPIG_PTR_CAST) vc));

    *selected = FALSE;

    if(!xio_info->available)
    {
        return mpi_errno;
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO, "select %s is avaialbe",
        proto_name)); 
    bc = mpig_vc_get_bc(vc);

    if (mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_UNDEFINED)
    {
        /* Get protocol version number and check that it is compatible 
            with this module */
        mpi_errno = mpig_bc_get_contact(
            bc, "CM_XIO_PROTO_VERSION", &version_str, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, 
            MPI_ERR_OTHER, "**globus|bc_get_contact",
        "**globus|bc_get_contact %s", "CM_XIO_PROTO_VERSION");
        if(!found)
        {
            goto fn_return;
        }

        rc = sscanf(version_str, "%d", &version);
        MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");
        if(version != MPIG_CM_XIO_PROTO_VERSION)
        {
            goto fn_return;
        }

        /* Get format of basic datatypes */
        mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_DC_FORMAT", 
            &format_str, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), 
            mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
            "**globus|bc_get_contact %s", "CM_XIO_DC_FORMAT");
        if(!found) 
        {
            goto fn_return;
        }
        rc = sscanf(format_str, "%d", &format);
        MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

        /* Get endianess of remote system */
        mpi_errno = mpig_bc_get_contact(bc, 
            "CM_XIO_DC_ENDIAN", &endian_str, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, 
            MPI_ERR_OTHER, "**globus|bc_get_contact",
            "**globus|bc_get_contact %s", "CM_XIO_DC_ENDIAN");
        if(!found) 
        {
            goto fn_return;
        }

        endian = (strcmp(endian_str, "little") == 0) ? 
            MPIG_ENDIAN_LITTLE : MPIG_ENDIAN_BIG;
    
        /* initialize CM XIO fields in the VC */
        mpig_cm_xio_vc_construct(vc);
        vc->cm.xio.endian = endian;
        vc->cm.xio.df = format;
    }

    if (mpig_vc_get_cm_type(vc) == MPIG_CM_TYPE_XIO)
    {
        char *                          cs;
        char *                          driver_name;
   
        /* first decide if we are gonna use it */
        mpi_errno = mpig_bc_get_contact(bc, proto_name, &driver_name, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno,
            MPI_ERR_OTHER, "**globus|bc_get_contact",
            "**globus|bc_get_contact %s", proto_name);
        if(!found)
        {
            goto fn_return;
        }

        /* verify both sides want to use the same protocol */
        if(strcmp(xio_info->driver_name, driver_name) != 0)
        {
            goto fn_return;
        }

        /* Get the contact string */
        mpi_errno = mpig_bc_get_contact(bc, cs_name, &contact_str, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, 
            MPI_ERR_OTHER, "**globus|bc_get_contact",
            "**globus|bc_get_contact %s", cs_name);
        if(!found) 
        {
            goto fn_return;
        }
        cs = MPIU_Strdup(contact_str);
        mpig_cm_xio_vc_set_contact_string(vc, cs);

        vc->cm.xio.xio_info = xio_info;
        *selected = TRUE;

        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO,
            "using %s, and cs %s for %s",
            xio_info->driver_name, cs, proto_name)); 
    }

  fn_return:
    if (version_str != NULL) mpig_bc_free_contact(version_str);
    if (contact_str != NULL) mpig_bc_free_contact(contact_str);
    if (format_str != NULL) mpig_bc_free_contact(format_str);
    if (endian_str != NULL) mpig_bc_free_contact(endian_str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", selected=%s, mpi_errno=" MPIG_ERRNO_FMT,
    (MPIG_PTR_CAST) vc, MPIG_BOOL_STR(*selected), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_select_module);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_select_module
MPIG_STATIC int
mpig_cm_xio_lan_select_module(
    mpig_vc_t * const                   vc,
    bool_t * const                      selected)
{
    globus_bool_t                       found;
    int                                 rc = MPI_SUCCESS;
    char *                              local_lan_id;
    char *                              vc_lan_id;
    mpig_bc_t *                         bc;
    int                                 mpi_errno;

    /* first check to see if we even need to bother */
    *selected = GLOBUS_FALSE;

    local_lan_id = globus_libc_getenv("GLOBUS_LAN_ID");
    vc_lan_id = vc->ci.lan_id;

    if(local_lan_id != NULL && vc_lan_id != NULL &&
        strcmp(local_lan_id, vc_lan_id) == 0)
    {
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO,
            "LAN_ID match: %s", local_lan_id));
        rc = mpig_cm_xio_xxx_select_module(
            vc,
            "CM_XIO_LAN_PROTO_NAME",
            "CM_XIO_LAN_CONTACT_STRING",
            &xio_l_lan_info,
            selected);
    }
    else
    {
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO,
            "LAN_ID did not match: %s != %s", local_lan_id, vc_lan_id));
        *selected = GLOBUS_FALSE;
    }

    return rc;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_select_module
MPIG_STATIC int
mpig_cm_xio_wan_select_module(
    mpig_vc_t * const                   vc,
    bool_t * const                      selected)
{
    return mpig_cm_xio_xxx_select_module(
        vc,
        "CM_XIO_WAN_PROTO_NAME",
        "CM_XIO_WAN_CONTACT_STRING",
        &xio_l_wan_info,
        selected);
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_select_module
MPIG_STATIC int
mpig_cm_xio_system_select_module(
    mpig_vc_t * const                   vc,
    bool_t * const                      selected)
{
    char *                              env_str;
    int                                 sc = 0;
    int                                 app_num;
    int                                 rc = MPI_SUCCESS;

    *selected = GLOBUS_FALSE;

    env_str = globus_libc_getenv("GLOBUS_DUROC_SUBJOB_INDEX");
    if(env_str != NULL)
    {
        sc = sscanf(env_str, "%d", &app_num);
    }

    if(sc == 1 && app_num == vc->ci.app_num)
    {
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO,
            "SYSTEM_ID_match: %d == %d", app_num, vc->ci.app_num));
        rc = mpig_cm_xio_xxx_select_module(
            vc,
            "CM_XIO_SYSTEM_PROTO_NAME",
            "CM_XIO_SYSTEM_CONTACT_STRING",
            &xio_l_system_info,
            selected);
    }
    else
    {
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_XIO,
            "SYSTEM_ID_did not match: %d != %d", app_num, vc->ci.app_num));
        *selected = GLOBUS_FALSE;
    }

    return rc;
}


/*********************************************************************
 *    COMPATABILITY FUNCTIONS
 *    -----------------------
 ********************************************************************/

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_lan_get_vc_compatability
MPIG_STATIC int
mpig_cm_xio_lan_get_vc_compatability(
    const mpig_vc_t * const             vc1, 
    const mpig_vc_t * const             vc2,
    const unsigned                      levels_in, 
    unsigned * const                    levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_lan_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_lan_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
        "entering: vc1=" MPIG_PTR_FMT ", vc2=" 
        MPIG_PTR_FMT ", levels_in=0x%08x",
        (MPIG_PTR_CAST) vc1, (MPIG_PTR_CAST) vc2, levels_in));

    if (levels_in & MPIG_TOPOLOGY_LEVEL_LAN_MASK
        && xio_l_lan_info.available)
    {
        *levels_out |= MPIG_TOPOLOGY_LEVEL_LAN_MASK;
    }


    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT 
        ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
        "mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc1,
        (MPIG_PTR_CAST) vc2, *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_lan_get_vc_compatability);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_wan_get_vc_compatability
MPIG_STATIC int
mpig_cm_xio_wan_get_vc_compatability(
    const mpig_vc_t * const             vc1,
    const mpig_vc_t * const             vc2,
    const unsigned                      levels_in,
    unsigned * const                    levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_wan_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_wan_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
        "entering: vc1=" MPIG_PTR_FMT ", vc2="
        MPIG_PTR_FMT ", levels_in=0x%08x",
        (MPIG_PTR_CAST) vc1, (MPIG_PTR_CAST) vc2, levels_in));

    if (levels_in & MPIG_TOPOLOGY_LEVEL_WAN_MASK
        && xio_l_wan_info.available)
    {
        *levels_out |= MPIG_TOPOLOGY_LEVEL_WAN_MASK;
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT
        ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
        "mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc1,
        (MPIG_PTR_CAST) vc2, *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_wan_get_vc_compatability);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_system_get_vc_compatability
MPIG_STATIC int
mpig_cm_xio_system_get_vc_compatability(
    const mpig_vc_t * const             vc1,
    const mpig_vc_t * const             vc2,
    const unsigned                      levels_in,
    unsigned * const                    levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_system_wan_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_system_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC,
        "entering: vc1=" MPIG_PTR_FMT ", vc2="
        MPIG_PTR_FMT ", levels_in=0x%08x",
        (MPIG_PTR_CAST) vc1, (MPIG_PTR_CAST) vc2, levels_in));

    if (levels_in & MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK
         && xio_l_system_info.available)
    {
        *levels_out |= MPIG_TOPOLOGY_LEVEL_SUBJOB_MASK;
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT
        ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
        "mpi_errno=" MPIG_ERRNO_FMT, (MPIG_PTR_CAST) vc1,
        (MPIG_PTR_CAST) vc2, *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_system_get_vc_compatability);
    return mpi_errno;
}



/*
 * communication module virtual table
 */
const mpig_cm_vtable_t                  mpig_cm_xio_wan_vtable =
{
    MPIG_CM_TYPE_XIO,
    "XIO_WAN",
    mpig_cm_xio_wan_init,
    mpig_cm_xio_wan_finalize,
    mpig_cm_xio_wan_add_contact_info,
    mpig_cm_xio_wan_extract_contact_info,
    mpig_cm_xio_wan_select_module,
    mpig_cm_xio_wan_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

/*
 * communication module virtual table
 */
const mpig_cm_vtable_t                  mpig_cm_xio_lan_vtable =
{
    MPIG_CM_TYPE_XIO,
    "XIO_LAN",
    mpig_cm_xio_lan_init,
    mpig_cm_xio_lan_finalize,
    mpig_cm_xio_lan_add_contact_info,
    mpig_cm_xio_lan_extract_contact_info,
    mpig_cm_xio_lan_select_module,
    mpig_cm_xio_lan_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

/*
 * communication module virtual table
 */
const mpig_cm_vtable_t                  mpig_cm_xio_system_vtable =
{
    MPIG_CM_TYPE_XIO,
    "XIO_SYSTEM",
    mpig_cm_xio_system_init,
    mpig_cm_xio_system_finalize,
    mpig_cm_xio_system_add_contact_info,
    mpig_cm_xio_system_extract_contact_info,
    mpig_cm_xio_system_select_module,
    mpig_cm_xio_system_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

#endif  /*MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS*/
