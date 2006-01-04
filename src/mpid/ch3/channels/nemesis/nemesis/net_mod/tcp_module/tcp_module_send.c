#include "tcp_module_impl.h"
#include "tcp_module.h"
#include "my_papi_defs.h"

#define DO_PAPI3(x) /*x */

/*#define TRACE */

static inline void
send_cell (int dest, MPID_nem_cell_t *cell, int datalen)
{
  MPID_nem_pkt_t *pkt    = MPID_NEM_CELL_TO_PACKET (cell);
  int    len    = MPID_NEM_PACKET_OPT_LEN(pkt);
  int    offset = 0;
  assert (datalen <= MPID_NEM_MPICH2_DATA_LEN + MPID_NEM_MPICH2_HEAD_LEN);

  DO_PAPI (PAPI_reset (PAPI_EventSet));
  offset = write(nodes[dest].desc, pkt,len);
  DO_PAPI (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues4));

  if( offset == len )
    {
      nodes[dest].left2write = 0;
#ifdef TRACE
      {
	int index;
	fprintf(stderr,"[%i] -- TCP DIRECT SEND : sent ALL MSG (%i len, payload is %i , datalen %i)\
\n",rank,offset, pkt->mpich2.datalen,datalen);
	/*
	for(index = 0 ; index < ((pkt->mpich2.datalen)/sizeof(int)); index ++)
	  {
	    fprintf(stderr,"[%i] --- cell[%i] : %i\n",rank,index,((int *)&(cell->pkt.mpich2))[index] );
	  }
	*/
      }
#endif

      MPID_nem_queue_enqueue ( process_free_queue, MPID_NEM_ABS_TO_REL( cell ));
    }
  else if(offset != -1)
    {
#ifdef TRACE
      fprintf(stderr,"[%i] -- TCP DIRECT SEND : sent PARTIAL  MSG (%i offset, payload is %i)\n",rank,offset, pkt->mpich2.datalen);
#endif

      cell->pkt.mpich2.source = rank;
      cell->pkt.mpich2.dest   = dest;
      nodes[dest].left2write      = offset;
      internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
      n_pending_send++;
      n_pending_sends[dest]++;
    }
  else
    {
      if(errno == EAGAIN)
	{

#ifdef TRACE
      fprintf(stderr,"[%i] -- TCP DIRECT SEND : Direct EnQ \n",rank );
#endif
	  cell->pkt.mpich2.source = rank;
	  cell->pkt.mpich2.dest   = dest;
	  nodes[dest].left2write      = 0;
	  internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
	  n_pending_send++;
	  n_pending_sends[dest]++;
	}
      else
	{
	  return;
	}
    }
}

void
tcp_module_send (int dest, MPID_nem_cell_t *cell, int datalen)
{
  DO_PAPI3 (PAPI_reset (PAPI_EventSet));
  cell->pkt.mpich2.datalen = datalen;
  if ( n_pending_sends[dest] == 0 )
    {
      DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
      send_cell (dest, cell, datalen);
      DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues16));
    }
  else
    {
      DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues15));
      cell->pkt.mpich2.source  = rank;
      cell->pkt.mpich2.dest    = dest;
      internal_queue_enqueue (nodes[dest].internal_recv_queue, cell);
      n_pending_send++;
      n_pending_sends[dest]++;
      tcp_module_poll_send();
      DO_PAPI3 (PAPI_accum_var (PAPI_EventSet, PAPI_vvalues17));
    }
}
