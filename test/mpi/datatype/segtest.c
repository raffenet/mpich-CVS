/* 
 * Simple segment test, including timing code
 */

/* 
 * Build datatype structures
 *
 * Contiguous
 *     n = 1, 4, 16, 64, 128, 512, 2048, 8196 ints
 * Vector
 *     blocksize = 1, 4, 64 ints
 *     stride    = 1, 64, 127
 * Block Indexed
 *     blocksize = 1, 4 ints
 *     offsets   = i*24 for i = 0 to n, n = 0, 64, 512
 * Indexed
 *     blocksizes = 1, 2, 4, 3, 7, 5, 6
 *     offsets    = i*24 for i = 0 to n, n = 0, 4, 7, 64, 512 
 *     (Wrap blocksizes to match offsets)
 *
 * Also need a few nested datatypes, such as vector of vectors
 * Do the versions in Using MPI
 * 
 */

/*
 * Routines to create dataloops for basic dataloops
 */
/*
 *  Contig
 */
MPID_Dataloop *ct;

ct = (MPID_Dataloop *)MPIU_Malloc( sizeof(MPID_Dataloop );
ct.kind                     = MPID_CONTIG;
ct.loop_params.c_t.count    = count;
ct.loop_params.c_t.datatype = 0;
ct.extent                   = count;
ct.id                       = 0;

/*
 * Vector
 */
MPID_Dataloop *MPID_Dataloop_init_vector( int count, int blocksize, 
					  int stride )
{
    v = (MPID_Dataloop *)MPIU_Malloc( sizeof(MPID_Dataloop) );
    v->kind                      = MPID_VECTOR;
    v->loop_params.v_t.count     = count;
    v->loop_params.v_t.blocksize = blocksize;
    v->loop_params.v_t.stride    = stride;
    v->loop_params.v_t.datatype  = 0;
    v->extent                    = (count-1)*stride + blocksize;
    v->id                        = 0;

    return v;
}

/* 
 * Block indexed
 */
MPID_Dataloop *MPID_Dataloop_init_blockindexed( int count, int blocksize, 
						MPI_Aint *offset )
{
    MPID_Dataloop *bi;

    bi = (MPID_Dataloop *)MPIU_Malloc( sizeof(MPID_Dataloop) );
    bi->kind                       = MPID_BLOCKINDEXED;
    bi->loop_params.bi_t.count     = count;
    bi->loop_params.bi_t.blocksize = blocksize;
    bi->loop_params.bi_t.offset    = 
	(MPI_Aint *)MPIU_MALLOC( sizeof(MPI_Aint) * count );
    for (i=0; i<count; i++) {
	bi->loop_params.bi_t.offset[i] = offset[i];
	if (offset[i] + blocksize > extent) 
	    extent = offset[i] + blocksize;
    }
    bi->loop_params.bi_t.datatype  = 0;
    bi->extent                     = extent;
    bi->id                         = 0;

    return bi;
}

/*
 * Indexed 
 */
MPID_Dataloop *MPID_Dataloop_init_indexed( int count, int *blocksize, 
					   MPI_Aint *offset )
{
    MPID_Dataloop *it;
    MPI_Aint      extent = 0;

    it = (MPID_Dataloop *)MPIU_Malloc( sizeof(MPID_Dataloop) );
    it->kind                      = MPID_INDEXED;
    it->loop_params.i_t.count     = count;
    it->loop_params.i_t.blocksize = (int *)MPIU_Malloc( sizeof(int) * count );
    it->loop_params.i_t.offset    = 
	(MPI_Aint *)MPIU_Malloc( sizeof(MPI_Aint) * count );
    for (i=0; i<count; i++) {
	it->loop_params.i_t.offset[i]    = offset[i];
	it->loop_params.i_t.blocksize[i] = blocksize[i];
	if (offset[i] + blocksize[i] > extent) 
	    extent = offset[i] + blocksize[i];
    }
    it->loop_params.i_t.datatype  = 0;
    it->extent                    = extent;
    it->id                        = 0;

    return it;
}
