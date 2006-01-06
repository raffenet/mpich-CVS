#include "gm_module_impl.h"
#include "gm.h"
#include "gm_module.h"
#include "my_papi_defs.h"

#define DO_PAPI3(x) /*x */

static void
send_callback (struct gm_port *p, void *context, gm_status_t status)
{
    MPID_nem_cell_t *cell = (MPID_nem_cell_t *)context;

    printf_d ("send_callback()\n");

    if (status != GM_SUCCESS)
    {
	gm_perror ("Send error", status);
    }

    ++num_send_tokens;

    MPID_nem_queue_enqueue (process_free_queue, cell);
}


/*
  requires that pkt is in registered memory, and there are sufficient tokens
 */
#if 0
static inline void
send_cell (int dest, MPID_nem_cell_t *cell, int datalen)
{
    MPID_nem_pkt_t *pkt = MPID_NEM_CELL_TO_PACKET (cell);

    assert (datalen <= MPID_NEM_MPICH2_DATA_LEN);

    DO_PAPI (PAPI_reset (PAPI_EventSet));
    gm_send_with_callback (port, pkt, PACKET_SIZE, datalen + MPID_NEM_MPICH2_HEAD_LEN, GM_LOW_PRIORITY, nodes[dest].node_id,
			   nodes[dest].port_id, send_callback, cell);
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues4));
    printf_d ("  Sent packet to node = %d, port = %d\n", nodes[dest].node_id, nodes[dest].port_id);
    printf_d ("    dest %d\n", dest);
    printf_d ("    datalen %d\n", datalen);
}
#else
#define send_cell(dest, cell, datalen) do {											\
    MPID_nem_pkt_t *pkt = MPID_NEM_CELL_TO_PACKET (cell);									\
																\
    assert ((datalen) <= MPID_NEM_MPICH2_DATA_LEN);										\
																\
    DO_PAPI (PAPI_reset (PAPI_EventSet));											\
    gm_send_with_callback (port, pkt, PACKET_SIZE, (datalen) + MPID_NEM_MPICH2_HEAD_LEN, GM_LOW_PRIORITY, nodes[dest].node_id,	\
			   nodes[dest].port_id, send_callback, cell);								\
    DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues4));									\
    printf_d ("  Sent packet to node = %d, port = %d\n", nodes[dest].node_id, nodes[dest].port_id);				\
    printf_d ("    dest %d\n", dest);												\
    printf_d ("    datalen + MPID_NEM_MPICH2_HEAD_LEN %d\n", datalen + MPID_NEM_MPICH2_HEAD_LEN);				\
} while (0)
#endif

/* #define BOUNCE_BUFFER */
inline void
send_from_queue()
{
    gm_module_send_queue_t *e;
#ifdef BOUNCE_BUFFER
    static MPID_nem_cell_t c;
    static int first = 1;

    if (first)
    {
	first = 0;
	gm_register_memory (port, &c, sizeof (c));
    }
#endif /* BOUNCE_BUFFER */
    
    while (!gm_module_queue_empty (send) && num_send_tokens)
    {
	gm_module_queue_dequeue (send, &e);

	switch (e->type)
	{
	case SEND_TYPE_CELL:
	    send_cell (e->u.cell->pkt.mpich2.dest, e->u.cell, e->u.cell->pkt.mpich2.datalen);
	    --num_send_tokens;
	    break;
	case SEND_TYPE_RDMA:
	    switch (e->u.rdma.type)
	    {
	    case RDMA_TYPE_GET:
		gm_module_get (e->u.rdma.target_p, e->u.rdma.source_p, e->u.rdma.len, e->u.rdma.node, e->u.rdma.completion_ctr);
		break;
	    case RDMA_TYPE_PUT:
		gm_module_put (e->u.rdma.target_p, e->u.rdma.source_p, e->u.rdma.len, e->u.rdma.node, e->u.rdma.completion_ctr);
		break;
	    default:
		FATAL_ERROR ("internal error");
		break;
	    }
	    break;
	default:
	    FATAL_ERROR ("internal error");
	    break;
	}
	gm_module_queue_free (send, e);
    }
}

void
gm_module_send (int dest, MPID_nem_cell_t *cell, int datalen)
{
    DO_PAPI3 (PAPI_reset (PAPI_EventSet));
    if (MPID_nem_queue_empty (module_gm_recv_queue) && num_send_tokens)
    {
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	send_cell (dest, cell, datalen);
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues16));
	--num_send_tokens;
    }
    else
    {
	gm_module_send_queue_t *e;

	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
	e = gm_module_queue_alloc (send);
	e->type = SEND_TYPE_CELL;
	e->u.cell = cell;
	
	cell->pkt.mpich2.source = rank;
	cell->pkt.mpich2.dest = dest;
	cell->pkt.mpich2.datalen = datalen;
	
	gm_module_queue_enqueue (send, e);
	send_from_queue();
	DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues17));
    }    
}
