#include "mpid_nem_queue.h"
#include "my_papi_defs.h"
#include "rdtsc.h"

void MPID_nem_network_poll();

#define USE_MPICH2


inline void MPID_nem_cell_init( MPID_nem_cell_ptr_t cell)
{
  cell->next = NULL;
  memset (&cell->pkt, 0, sizeof (MPID_nem_pkt_header_t));
}

inline void MPID_nem_rel_cell_init( MPID_nem_cell_ptr_t cell)
{
  cell->next =  (MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL ;
  memset (&cell->pkt, 0, 4*1024);
  /*  memset (&cell->pkt, 0, sizeof (MPID_nem_pkt_header_t)); */
}


inline void MPID_nem_dump_cell_mpich ( MPID_nem_cell_ptr_t cell, int master)
{
  MPID_nem_pkt_mpich2_t     *mpkt     = &(cell->pkt.mpich2);
  int              *cell_buf = (int *)(mpkt->payload);
  int               mark;

  if (master)
    mark = 111;
  else 
    mark = 777;

  
  fprintf(stdout,"Cell[%i  @ %p (rel %p), next @ %p (rel %p)]\n ",
	  mark,
	  cell,
	  MPID_NEM_ABS_TO_REL(cell), 
	  MPID_NEM_REL_TO_ABS( cell->next ),
	  cell->next );
  
  fprintf(stdout,"%i  [Source:%i] [dest : %i] [dlen : %i] [seqno : %i]\n",
	  mark,
	  mpkt->source,
	  mpkt->dest,
	  mpkt->datalen, 
	  mpkt->seqno);	       
  
  if (cell_buf[0] == 0)
    {
      fprintf(stdout,"%i [type: MPIDI_CH3_PKT_EAGER_SEND ] [tag : %i] [dsz : %i]\n",
	      mark,
	      cell_buf[1],
	      cell_buf[4]);
      {
	int index;
	char *buf = (char *)cell_buf;

	for (index = 0 ; index < 40 ; index++)
	  fprintf(stdout," -- %i ",buf[index]);
	fprintf(stdout,"\n");
      }


    }
  else if (cell_buf[0] == 5)
    fprintf(stdout,"%i [type: MPIDI_CH3_PKT_RNDv_CLR_TO_SEND ] [tag : %i] [dsz : %i]\n",
	    mark,
	    cell_buf[1],
	    cell_buf[4]);

  else
    fprintf(stdout,"%i [type:%i]\n", mark, cell_buf[0] );

}

/*inline */
void __MPID_nem_dump_cell_mpich2 ( MPID_nem_cell_ptr_t cell, int master, char *file, int line)
{
  int mark;

  if (master)
    mark = 111;
  else
    mark = 777;
 
  fprintf(stderr,"%i called from file %s at line %i \n",mark,file,line);

  MPID_nem_dump_cell_mpich(cell,master); 
}


#if 0
void MPID_nem_dump_queue(  MPID_nem_queue_ptr_t qhead )
{
    int index        = 0;    
#ifndef MPID_NEM_USE_SHADOW_HEAD    
    MPID_nem_cell_ptr_t ptr =  qhead->head;    
    fprintf(stderr,"[HEAD: %p][TAIL: %p]\n" ,
	    qhead->head,
	    qhead->tail);
#else /*SHADOW_HEAD */
    MPID_nem_cell_ptr_t ptr =  qhead->my_head;   
    fprintf(stderr,
	    "[HEAD: %p][MYHEAD: %p][TAIL: %p]\n" ,
	    qhead->head,
	    qhead->my_head,
	    qhead->tail);

    if (ptr == NULL)
      ptr = qhead->head;   

#endif /*SHADOW_HEAD */

    while( ptr != NULL )
    {
	pkt_type_t type = ptr->pkt.type;

	fprintf(stderr,"Cell[%i @ %p, next @ %p]\n ",
		index,
		ptr,
		ptr->next );

	if ( type == PKT_HEAD ){
	  p_pkt_header_t pkt_head = &ptr->pkt.head;
	  match_t      match    = pkt_head->match ;	
	  
	  fprintf(stderr," [type:%i] [Match:%i %i %i] [dest : %i]\n",
		  pkt_head->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_head->dest );	
	} else if ( type == PKT_DATA) {
	  p_pkt_data_t pkt_data = &ptr->pkt.data;
	  match_t       match   = pkt_data->match ;
	  
	  fprintf(stderr," [type:%i] [Match:%i %i %i] [Data %i]\n",
		  pkt_data->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_data->data[0]);
	  
	  fprintf(stderr,"  [Dest: %i][dlen: %i][seqno: %i]\n",
		  pkt_data->dest,
		  pkt_data->datalen,
		  pkt_data->seqno );
	  
	} else if ( type == PKT_NODATA) {
	  p_pkt_nodata_t pkt_nodata = &ptr->pkt.nodata;
	  match_t        match      = pkt_nodata->match ;

	  fprintf(stderr," [type:%i] [Match:%i %i %i] [dest : %i] [Data_ptr : %p]\n",
		  pkt_nodata->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_nodata->dest, 
		  pkt_nodata->data_ptr );	
	} else {
	fprintf(stderr," [type:%i] [Match: ???] [dest : ?] [Data_ptr : ?]\n",
		type);
	  
	}
	ptr = ptr->next ;

	index++;
    }  
    if (index != (qhead->size))
	fprintf ( stderr, "Sizes don't match : %i, %i\n",index, qhead->size);
}

void MPID_nem_rel_dump_queue(  MPID_nem_queue_ptr_t rel_qhead )
{
    int            index  = 0;    
    MPID_nem_queue_ptr_t qhead  = MPID_NEM_REL_TO_ABS( rel_qhead );
    fprintf(stderr,"===== [Abs Q: %p (rel %p)] ===== \n" , qhead, rel_qhead);

#ifndef MPID_NEM_USE_SHADOW_HEAD    
    MPID_nem_cell_ptr_t ptr     = MPID_NEM_SET_ABS_NULL( qhead->head );  
    fprintf(stderr,"[HEAD: %p (rel %p)][TAIL: (rel %p)]\n" ,
	    MPID_NEM_SET_ABS_NULL( qhead->head ),
	    qhead->head,
	    MPID_NEM_SET_ABS_NULL( qhead->tail ),
            qhead->tail);
#else /*SHADOW_HEAD */
    MPID_nem_cell_ptr_t ptr     = MPID_NEM_SET_ABS_NULL( qhead->my_head );  
    fprintf(stderr,
	    "[HEAD: %p (rel %p)][MYHEAD: %p (rel %p)][TAIL: %p (rel %p)]\n" ,
	    MPID_NEM_SET_ABS_NULL(qhead->head),
	    qhead->head,
	    MPID_NEM_SET_ABS_NULL(qhead->my_head),
	    qhead->my_head,
	    MPID_NEM_SET_ABS_NULL(qhead->tail),
            qhead->tail);

    if (ptr == NULL)
      ptr = MPID_NEM_REL_TO_ABS( qhead->head );   

#endif /*SHADOW_HEAD */

    while( ptr != NULL )
    {

	fprintf(stderr,"Cell[%i @ %p (rel %p), next @ %p (rel %p)]\n ",
		index,
		ptr,
   	        MPID_NEM_ABS_TO_REL(ptr), 
		MPID_NEM_REL_TO_ABS(  ptr->next ),
	        ptr->next );
	{
#ifdef USE_MPICH2
	  MPID_nem_pkt_mpich2_t     *mpkt     = &(ptr->pkt.mpich2);
	  int              *cell_buf = (int *)(mpkt->payload);

	  fprintf(stderr," [Source:%i] [dest : %i] [dlen : %i] [seqno : %i]\n",
		  mpkt->source,
		  mpkt->dest,
		  mpkt->datalen, 
		  mpkt->seqno);	       
	  
	  if (cell_buf[0] == 0)
	    fprintf(stderr," [type: MPIDI_CH3_PKT_EAGER_SEND ] [tag : %i] [dsz : %i]\n",
		    cell_buf[1],
		    cell_buf[4]);
	  else if (cell_buf[0] == 5)
	    fprintf(stderr,"[type: MPIDI_CH3_PKT_RNDv_CLR_TO_SEND ] [tag : %i] [dsz : %i]\n",
		    cell_buf[1],
		    cell_buf[4]);
	  else
	    fprintf(stderr," [type:%i]\n", cell_buf[0] );
	  
#else /*MPICH2 */
	
	pkt_type_t type = ptr->pkt.type;

	if ( type == PKT_HEAD ){
	  p_pkt_header_t pkt_head = &ptr->pkt.head;
	  match_t      match    = pkt_head->match ;	
	  
	  fprintf(stderr," [type:%i] [Match:%i %i %i] [dest : %i]\n",
		  pkt_head->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_head->dest );	
	} else if ( type == PKT_DATA) {
	  p_pkt_data_t pkt_data = &ptr->pkt.data;
	  match_t       match   = pkt_data->match ;
	  
	  fprintf(stderr," [type:%i] [Match:%i %i %i] [Data %i]\n",
		  pkt_data->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_data->data[0]);
	  
	  fprintf(stderr,"  [Dest: %i][dlen: %i][seqno: %i]\n",
		  pkt_data->dest,
		  pkt_data->datalen,
		  pkt_data->seqno );
	  
	} else if ( type == PKT_NODATA) {
	  p_pkt_nodata_t pkt_nodata = &ptr->pkt.nodata;
	  match_t        match      = pkt_nodata->match ;

	  fprintf(stderr," [type:%i] [Match:%i %i %i] [dest : %i] [Data_ptr : %p]\n",
		  pkt_nodata->type,
		  match.part.source, 
		  match.part.tag, 
		  match.part.context, 
		  pkt_nodata->dest, 
		  pkt_nodata->data_ptr );	
	} else {
	fprintf(stderr," [type:%i] [Match: ???] [dest : ?] [Data_ptr : ?]\n",
		type);	  
	}
#endif /* MPICH2 */
	}

        ptr = MPID_NEM_SET_ABS_NULL(  ptr->next );

	index++;
    }  
    /*if (index != (qhead->size)) */
    fprintf ( stderr, "[] RecvQ Size is : %i \n",index);
}
#endif

inline void MPID_nem_rel_queue_init (MPID_nem_queue_ptr_t rel_qhead )
{
    MPID_nem_queue_ptr_t qhead = MPID_NEM_REL_TO_ABS( rel_qhead );
    qhead->head    = (MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL;
    qhead->my_head = (MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL;
    qhead->tail    = (MPID_nem_cell_ptr_t) MPID_NEM_ASYMM_NULL;
}

inline void MPID_nem_queue_init (MPID_nem_queue_ptr_t qhead )
{
  qhead->head    = NULL;
  qhead->my_head = NULL;
  qhead->tail    = NULL;
}














