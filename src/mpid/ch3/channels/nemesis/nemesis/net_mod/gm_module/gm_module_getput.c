#include "gm_module_impl.h"
#include "gm.h"

static void
getput_callback (struct gm_port *p, void *completion_ctr, gm_status_t status)
{
    if (status != GM_SUCCESS)
    {
	gm_perror ("Send error", status);
    }

    ++num_send_tokens;

    MPID_NEM_ATOMIC_DEC ((int *)completion_ctr);
}


int
gm_module_get (void *target_p, void *source_p, int len, int source_node, int *completion_ctr)
{
    assert (source_node >= 0 && source_node < numnodes);
    assert (len >= 0);
    
    if (len == 0)
    {
	MPID_NEM_ATOMIC_DEC (completion_ctr);
	return 0;
    }
#if 0
    {
	gm_status_t status;
	
	status = gm_directcopy_get (port, source_p, target_p, len, 0, nodes[source_node].port_id);
	if (status != GM_SUCCESS)
	{
	    gm_perror ("directcopy", status);
	    exit (-1);
	}
	MPID_NEM_ATOMIC_DEC (completion_ctr);
    return 0;
    }
#endif
    if (!num_send_tokens)
    {
	gm_module_send_queue_t *e = gm_module_queue_alloc (send);

	e->type = SEND_TYPE_RDMA;
	e->u.rdma.type = RDMA_TYPE_GET;
	e->u.rdma.target_p = target_p;
	e->u.rdma.source_p = source_p;
	e->u.rdma.len = len;
	e->u.rdma.node = source_node;
	e->u.rdma.completion_ctr = completion_ctr;
	gm_module_queue_enqueue (send, e);
	return 0;
    }
	    
    gm_get (port, (long)source_p, target_p, len, GM_LOW_PRIORITY, nodes[source_node].node_id, nodes[source_node].port_id,
	    getput_callback, completion_ctr);

    --num_send_tokens;

    return 0;
}

int
gm_module_put (void *target_p, void *source_p, int len, int target_node, int *completion_ctr)
{
    assert (target_node >= 0 && target_node < numnodes);
    assert (len >= 0);
    
    if (len == 0)
    {
	MPID_NEM_ATOMIC_DEC (completion_ctr);
	return 0;
    }

    if (!num_send_tokens)
    {
	gm_module_send_queue_t *e = gm_module_queue_alloc (send);

	e->type = SEND_TYPE_RDMA;
	e->u.rdma.type = RDMA_TYPE_PUT;
	e->u.rdma.target_p = target_p;
	e->u.rdma.source_p = source_p;
	e->u.rdma.len = len;
	e->u.rdma.node = target_node;
	e->u.rdma.completion_ctr = completion_ctr;
	gm_module_queue_enqueue (send, e);
	return 0;
    }
	    
    gm_put (port, source_p, (long)target_p, len, GM_LOW_PRIORITY, nodes[target_node].node_id, nodes[target_node].port_id,
	    getput_callback, completion_ctr);

    --num_send_tokens;

    return 0;
}

