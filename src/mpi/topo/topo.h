
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

#define MAX_CART_DIM 16
