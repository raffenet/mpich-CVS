/*
 * Copyright (C) 1997, Silicon Graphics, Inc.
 * All Rights Reserved
 */

#ifndef __HAS_MPI_COMBINERS

#define MPI_COMBINER_NAMED	(-1)
#define MPI_COMBINER_CONTIGUOUS	0
#define MPI_COMBINER_VECTOR	1
#define MPI_COMBINER_HVECTOR	2
#define MPI_COMBINER_INDEXED	3
#define MPI_COMBINER_HINDEXED	4
#define MPI_COMBINER_STRUCT	5

#ifndef __MPISGI31
   int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers,
             int *num_addresses, int *num_datatypes, int *combiner); 
   int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int
             max_addresses, int max_datatypes, int *array_of_integers, 
             MPI_Aint *array_of_addresses, MPI_Datatype *array_of_datatypes);
#endif

#endif
