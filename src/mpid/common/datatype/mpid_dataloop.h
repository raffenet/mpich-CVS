/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_DATALOOP_H
#define MPID_DATALOOP_H

#include "mpi.h"

/* Note: this is where you define the prefix that will be prepended on
 * all externally visible generic dataloop and segment functions.
 */
#define PREPEND_PREFIX(fn) MPID_ ## fn

/* These following dataloop-specific types will be used throughout the DLOOP
 * instance:
 */
#define DLOOP_Offset     MPI_Aint
#define DLOOP_Count      int
#define DLOOP_Handle     MPI_Datatype
#define DLOOP_Type       MPI_Datatype
#define DLOOP_Buffer     void *
#define DLOOP_VECTOR     MPID_IOV
#define DLOOP_VECTOR_LEN MPID_IOV_LEN
#define DLOOP_VECTOR_BUF MPID_IOV_BUF

/* The following accessor functions must also be defined:
 *
 * DLOOP_Handle_extent()    *
 * DLOOP_Handle_size()      *
 * DLOOP_Handle_loopptr()
 * DLOOP_Handle_loopdepth() *
 * DLOOP_Handle_hasloop()
 * DLOOP_Handle_true_lb()
 * DLOOP_Handle_mpi1_lb()
 * DLOOP_Handle_mpi1_ub()
 *
 * Q: Do we really need ALL of these? *'d ones we need for sure.
 */

/* USE THE NOTATION THAT BILL USED IN MPIIMPL.H AND MAKE THESE MACROS */

/* NOTE: put get size into mpiimpl.h; the others go here until such time
 * as we see that we need them elsewhere.
 */
#define DLOOP_Handle_get_loopdepth_macro(__handle,__depth,__hetero) \
    MPID_Datatype_get_loopdepth_macro(__handle,__depth,__hetero)

#define DLOOP_Handle_get_loopsize_macro(__handle,__size,__hetero) \
    MPID_Datatype_get_loopsize_macro(__handle,__size,__hetero)

#define DLOOP_Handle_get_loopptr_macro(__handle,__lptr,__hetero) \
    MPID_Datatype_get_loopptr_macro(__handle,__lptr,__hetero)

#define DLOOP_Handle_get_size_macro(__handle,__size) \
    MPID_Datatype_get_size_macro(__handle,__size)

#define DLOOP_Handle_get_basic_type_macro(__handle,__eltype) \
    MPID_Datatype_get_basic_type(__handle, __eltype)

#define DLOOP_Handle_get_extent_macro(__handle,__extent) \
    MPID_Datatype_get_extent_macro(__handle,__extent)

#define DLOOP_Handle_hasloop_macro(__handle)                           \
    ((HANDLE_GET_KIND(__handle) == HANDLE_KIND_BUILTIN) ? 0 : 1)

/* allocate and free functions must also be defined. */
#define DLOOP_Malloc MPIU_Malloc
#define DLOOP_Free   MPIU_Free

/* debugging output function */
#define DLOOP_dbg_printf MPIU_dbg_printf

/* assert function */
#define DLOOP_Assert MPIU_Assert

/* Include gen_dataloop.h at the end to get the rest of the prototypes
 * and defines, in terms of the prefixes and types above.
 */
#include <gen_dataloop.h>

/* NOTE: WE MAY WANT TO UNDEF EVERYTHING HERE FOR NON-INTERNAL COMPILATIONS */

/* dataloop construction functions */
int MPID_Dataloop_create_contiguous(int count,
				    MPI_Datatype oldtype,
				    MPID_Dataloop **dlp_p,
				    int *dlsz_p,
				    int *dldepth_p,
				    int flags);
int MPID_Dataloop_create_vector(int count,
				int blocklength,
				MPI_Aint stride,
				int strideinbytes,
				MPI_Datatype oldtype,
				MPID_Dataloop **dlp_p,
				int *dlsz_p,
				int *dldepth_p,
				int flags);
int MPID_Dataloop_create_blockindexed(int count,
				      int blklen,
				      void *disp_array,
				      int dispinbytes,
				      MPI_Datatype oldtype,
				      MPID_Dataloop **dlp_p,
				      int *dlsz_p,
				      int *dldepth_p,
				      int flags);
int MPID_Dataloop_create_indexed(int count,
				 int *blocklength_array,
				 void *displacement_array,
				 int dispinbytes,
				 MPI_Datatype oldtype,
				 MPID_Dataloop **dlp_p,
				 int *dlsz_p,
				 int *dldepth_p,
				 int flags);
int MPID_Dataloop_create_struct(int count,
				int *blklen_array,
				MPI_Aint *disp_array,
				MPI_Datatype *oldtype_array,
				MPID_Dataloop **dlp_p,
				int *dlsz_p,
				int *dldepth_p,
				int flags);

/* flags for MPID_Dataloop_create_xxx calls */
#define MPID_DATALOOP_HOMOGENEOUS 1
#define MPID_DATALOOP_ALL_BYTES   2

#endif
