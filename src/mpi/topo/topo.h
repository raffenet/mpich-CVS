/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

typedef struct {
  int nnodes;
  int nedges;
  int *index;
  int *edges;
} MPIR_Graph_topology;

typedef struct {
  int nnodes;     /* Product of dims[*], gives the size of the topology */
  int ndims;
  int *dims;
  int *periodic;
  int *position;
} MPIR_Cart_topology;

typedef struct { 
  MPIR_Topo_type kind;
  union { 
    MPIR_Graph_topology graph;
    MPIR_Cart_topology  cart;
  } topo;
} MPIR_Topology;

MPIR_Topology *MPIR_Topology_get( MPID_Comm * );
int MPIR_Topology_put( MPID_Comm *, MPIR_Topology * );

#define MAX_CART_DIM 16
