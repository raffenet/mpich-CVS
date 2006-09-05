#include "mpidimpl.h"
#include "ib_module_priv.h"
#include "ib_utils.h"

/**
 * open_hca - Opens an IB HCA
 *
 * @dev: IB device to be opened (IN)
 * @ctxt: Context of opened device (OUT)
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_open_hca
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_open_hca(struct ibv_device *dev, 
        struct ibv_context **ctxt)
{
    int mpi_errno = MPI_SUCCESS;

    *ctxt = ibv_open_device(dev);

    MPIU_ERR_CHKANDJUMP1( (*ctxt) == NULL, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_open_device", 
            "**ibv_open_device %s", ibv_get_device_name(dev));

    NEM_IB_DBG("Opened Device %s", ibv_get_device_name(dev));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/**
 * alloc_pd - Allocates a Protection Domain
 *
 * @ctxt: Context of IB device for which protection
 *  domain has to be allocated (IN)
 * @pd: Allocated protection domain (OUT)
 * @dev: IB Device (IN)
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_alloc_pd
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_alloc_pd(struct ibv_context *ctxt, 
        struct ibv_pd **pd,
        struct ibv_device *dev)
{
    int mpi_errno = MPI_SUCCESS;

    *pd = ibv_alloc_pd(ctxt);

    MPIU_ERR_CHKANDJUMP1( (*pd) == NULL, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_alloc_pd", 
            "**ibv_alloc_pd %s", ibv_get_device_name(dev));

    NEM_IB_DBG("Allocated PD for %s", ibv_get_device_name(dev));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/**
 * MPID_nem_ib_module_get_dev_attr - Gets IB device attributes 
 *
 * @ctxt: Context of IB device for which protection
 *  domain has to be allocated (IN)
 * @pd: Allocated Device Attribute Struct (OUT)
 * @dev: IB Device (IN)
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_get_dev_attr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_get_dev_attr(struct ibv_context *ctxt, 
        struct ibv_device_attr *dev_attr,
        struct ibv_device *dev)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = ibv_query_device(ctxt, dev_attr);

    MPIU_ERR_CHKANDJUMP1( mpi_errno != 0, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_query_device", 
            "**ibv_query_device %s", ibv_get_device_name(dev));

    NEM_IB_DBG("Got device attr for %s", ibv_get_device_name(dev));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


/**
 * create_cq - Creates a Completion Queue
 *
 * @ctxt: Context of IB device for which CQ is required (IN)
 * @cq: Created completion queue (OUT)
 * @comp_channel: Created completion channel (OUT)
 * @cqe: Min number of CQ elements desired (IN)
 * @create_comp_channel: Boolean var to indicate
 *  whether a completion channel is desired for
 *  this device (IN)
 * @dev: IB Device (IN)
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_create_cq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_create_cq(struct ibv_context *ctxt,
        struct ibv_cq **cq,
        struct ibv_comp_channel **comp_channel,
        int cqe,
        int create_comp_channel,
        struct ibv_device *dev)
{
    int mpi_errno = MPI_SUCCESS;

    if(create_comp_channel) {
        *comp_channel = ibv_create_comp_channel(ctxt);

        MPIU_ERR_CHKANDJUMP1( (*comp_channel) == NULL, 
                mpi_errno, MPI_ERR_OTHER, "**ibv_create_comp_channel", 
                "**ibv_create_comp_channel %s", ibv_get_device_name(dev));
    } else {

        *comp_channel = NULL;
    }

    *cq = ibv_create_cq(ctxt, cqe, NULL, *comp_channel, 0);

    MPIU_ERR_CHKANDJUMP1( (*cq) == NULL, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_cq", 
            "**ibv_create_cq %s", ibv_get_device_name(dev));

    NEM_IB_DBG("Allocated CQ for %s", ibv_get_device_name(dev));

fn_exit:
    return mpi_errno;
fn_fail:
    if(*comp_channel) {
        ibv_destroy_comp_channel(*comp_channel);
    }
    if(*cq) {
        ibv_destroy_cq(*cq);
    }
    goto fn_exit;
}

/**
 * create_srq - Creates a Shared Receive Queue
 *
 * @ctxt: Context of IB device for which SRQ is required (IN)
 * @pd: Protection Domain (IN)
 * @srq: Created Shared Receive Queue (OUT)
 * @srq_wr: Min number of work queue requests (IN)
 * @srq_sge: Scatter/Gather entries for SRQ (IN)
 * @srq_limit: Limit for SRQ event (IN)
 * @dev: IB Device (IN)
 */

#undef FUNCNAME
#define FUNCNAME MPID_nem_ib_module_create_srq
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_ib_module_create_srq(struct ibv_context *ctxt,
        struct ibv_pd *pd,
        struct ibv_srq **srq,
        uint32_t srq_wr,
        uint32_t srq_sge,
        uint32_t srq_limit,
        struct ibv_device *dev)
{
    int mpi_errno = MPI_SUCCESS;
    struct ibv_srq_init_attr srq_init_attr;

    memset(&srq_init_attr, 0, sizeof(srq_init_attr));

    srq_init_attr.srq_context = ctxt;
    srq_init_attr.attr.max_wr = srq_wr;
    srq_init_attr.attr.max_sge = srq_sge;
    /* The limit value should be ignored during SRQ create,
     * setting it anyways */
    srq_init_attr.attr.srq_limit = srq_limit;

    *srq = ibv_create_srq(pd, &srq_init_attr);

    MPIU_ERR_CHKANDJUMP1( (*srq) == NULL, 
            mpi_errno, MPI_ERR_OTHER, "**ibv_create_srq", 
            "**ibv_create_srq %s", ibv_get_device_name(dev));

    NEM_IB_DBG("Allocated SRQ for %s", ibv_get_device_name(dev));

fn_exit:
    return mpi_errno;
fn_fail:
    if(*srq) {
        ibv_destroy_srq(*srq);
    }
    goto fn_exit;
}

/**
 * MPID_nem_ib_module_is_port_active - Finds out if a particular
 *  port is active or not. 
 *
 * @ctxt: Context of IB device (IN)
 * @port_num: Port number to be checked (IN)
 *
 * Notes: this function is called after opening a device, so
 * technically getting port attributes shouldn't fail.
 */

int MPID_nem_ib_module_is_port_active(struct ibv_context *ctxt,
        uint8_t port_num)
{
    int ret;
    struct ibv_port_attr port_attr;

    ret = ibv_query_port(ctxt, port_num, &port_attr);

    if(!ret) {

        if(port_attr.state == IBV_PORT_ACTIVE && port_attr.lid) {
            return 1;
        } else {
            return 0;
        }
    }

    /* Silently ignore the error case, though
     * it shouldn't happen ever */

    return 0;
}

/**
 * MPID_nem_ib_module_is_port_active - Finds out if a particular
 *  port is active or not. 
 *
 * @ctxt: Context of IB device (IN)
 * @port_num: Port number to be checked (IN)
 *
 * Notes: this function is called after opening a device, so
 * technically getting port attributes shouldn't fail.
 */

int MPID_nem_ib_module_get_port_prop(struct ibv_context *ctxt,
        uint8_t port_num, struct ibv_port_attr *attr)
{
    int ret;

    ret = ibv_query_port(ctxt, port_num, attr);

    return ret;
}

int MPID_nem_ib_module_reg_mem(struct ibv_pd *pd,
        void *addr,
        size_t length,
        int acl,
        struct ibv_mr **mr)
{
    enum ibv_access_flags acc_flag;

    if(acl == MPID_NEM_IB_BUF_READ_ONLY) {
        acc_flag = IBV_ACCESS_REMOTE_READ;
    } else if(acl == MPID_NEM_IB_BUF_READ_WRITE) {
        acc_flag = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE |
            IBV_ACCESS_REMOTE_READ;
    }

    *mr = ibv_reg_mr(pd, addr, length, acc_flag);

    if(NULL == mr) {
        return 1;
    } else {
        return 0;
    }
}

int MPID_nem_ib_module_post_srq(struct ibv_srq *srq,
        struct ibv_recv_wr *r_wr)
{
    int ret;
    struct ibv_recv_wr *bad_wr;

    MPIU_Assert(NULL != srq);

    ret = ibv_post_srq_recv(srq, r_wr, &bad_wr);

    return ret;
}

int MPID_nem_ib_module_post_send(struct ibv_qp *qp,
        struct ibv_send_wr *s_wr)
{
    int ret;
    struct ibv_send_wr *bad_wr;

    MPIU_Assert(NULL != qp);

    ret = ibv_post_send(qp, s_wr, &bad_wr);

    return ret;
}

int MPID_nem_ib_module_poll_cq(struct ibv_cq *cq, 
        struct ibv_wc *wc)
{
    int ret;

    ret = ibv_poll_cq(cq, 1, wc);

    if(ret < 0) {

        NEM_IB_ERR("Error polling CQ");

        ret = -1;

    } else if (ret > 0) {

        if(IBV_WC_SUCCESS != wc->status) {

            NEM_IB_ERR("Got completion with error, code %d", wc->status);
            ret = -1;

        } else {

            ret = 1;
        }

    } else {

        ret = 0;
    }

    return ret;
}

int MPID_nem_ib_module_modify_srq(
            struct ibv_srq *srq,
            uint32_t max_wr,
            uint32_t srq_limit)
{
    int ret;
    struct ibv_srq_attr attr;

    memset(&attr, 0, sizeof(struct ibv_srq_attr));

    attr.max_wr = max_wr;
    attr.max_sge = 1;
    attr.srq_limit = srq_limit;

    ret = ibv_modify_srq(srq, &attr, IBV_SRQ_LIMIT);

    return ret;
}
