#include <mpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "romio_dataloop.h"
#include "typesize_support.h"

typedef struct MPIO_Datatype_s {
    int   valid, refct;
    int   dloop_size; /* size of actual dataloop structure */
    int   dloop_depth;
    DLOOP_Offset size, extent; /* size and extent of type */
    void *dloop;
} MPIO_Datatype;

static int MPIO_Datatype_keyval = MPI_KEYVAL_INVALID;

static MPIO_Datatype *MPIO_Datatype_allocate(MPI_Datatype type);
static void MPIO_Datatype_set_szext(MPI_Datatype type, MPIO_Datatype *dtp);

#define MPIO_DATATYPE_VALID_PTR       1
#define MPIO_DATATYPE_VALID_SIZE      2
#define MPIO_DATATYPE_VALID_DEPTH     4
#define MPIO_DATATYPE_VALID_TYPESZEXT 8

int MPIO_Datatype_initialize(void);
void MPIO_Datatype_finalize(void);

int MPIO_Datatype_copy_attr_function(MPI_Datatype type, int type_keyval,
				     void *extra_state, void *attribute_val_in,
				     void *attribute_val_out, int *flag);
int MPIO_Datatype_delete_attr_function(MPI_Datatype type, int type_keyval,
				       void *attribute_val, void *extra_state);

#if 0
int main(int argc, char *argv[])
{
    int mpi_errno;
    MPI_Datatype type, duptype;

    MPI_Init(&argc, &argv);

    MPIO_Datatype_initialize();

    mpi_errno = MPI_Type_contiguous(2, MPI_INT, &type);
    assert(mpi_errno == MPI_SUCCESS);

    MPIO_Datatype_set_loopptr(type, (void *) 7, 0);

    mpi_errno = MPI_Type_dup(type, &duptype);
    assert(mpi_errno == MPI_SUCCESS);

    mpi_errno = MPI_Type_free(&type);

    mpi_errno = MPI_Type_free(&duptype);

    foo_finalize();

    MPI_Finalize();
    return 0;
}
#endif

int MPIO_Datatype_copy_attr_function(MPI_Datatype type,
				     int type_keyval,
				     void *extra_state,
				     void *attribute_val_in,
				     void *attribute_val_out,
				     int *flag)
{
    MPIO_Datatype *dtp = (MPIO_Datatype *) attribute_val_in;

    printf("copy fn. called\n");

    assert(dtp->refct);

    dtp->refct++;

    * (MPIO_Datatype **) attribute_val_out = dtp;
    *flag = 1;

    printf("inc'd refct.\n");

    return MPI_SUCCESS;
}

int MPIO_Datatype_delete_attr_function(MPI_Datatype type,
				       int type_keyval,
				       void *attribute_val,
				       void *extra_state)
{
    MPIO_Datatype *dtp = (MPIO_Datatype *) attribute_val;

    printf("delete fn. called\n");

    assert(dtp->refct);

    printf("dec'd refct\n");
    
    dtp->refct--;
    if (dtp->refct == 0) {
	free(dtp);
	printf("freed attr structure\n");
    }

    return MPI_SUCCESS;
}

int MPIO_Datatype_initialize(void)
{
    int mpi_errno;

    assert(MPIO_Datatype_keyval == MPI_KEYVAL_INVALID);

    /* create keyval for use later */
    mpi_errno = MPI_Type_create_keyval(MPIO_Datatype_copy_attr_function,
				       MPIO_Datatype_delete_attr_function,
				       &MPIO_Datatype_keyval,
				       NULL);
    assert(mpi_errno == MPI_SUCCESS);

    printf("created keyval\n");

    return 0;
}

void MPIO_Datatype_finalize(void)
{
    int mpi_errno;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    /* remove keyval */
    mpi_errno = MPI_Type_free_keyval(&MPIO_Datatype_keyval);
    assert(mpi_errno == MPI_SUCCESS);

    printf("freed keyval\n");
    
    return;
}

void MPIO_Datatype_get_size(MPI_Datatype type, MPI_Offset *size_p)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    
    if (!attrflag) {
	dtp = MPIO_Datatype_allocate(type);
    }

    if (!(dtp->valid & MPIO_DATATYPE_VALID_TYPESZEXT)) {
	MPIO_Datatype_set_szext(type, dtp);
    }

    *size_p = dtp->size;
    return;
}

void MPIO_Datatype_get_extent(MPI_Datatype type, MPI_Offset *extent_p)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    
    if (!attrflag) {
	dtp = MPIO_Datatype_allocate(type);
    }

    if (!(dtp->valid & MPIO_DATATYPE_VALID_TYPESZEXT)) {
	MPIO_Datatype_set_szext(type, dtp);
    }

    *extent_p = dtp->extent;
    return;
}

void MPIO_Datatype_get_loopptr(MPI_Datatype type,
			       MPIO_Dataloop **ptr_p,
			       int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    assert(dtp->valid & MPIO_DATATYPE_VALID_PTR);

    *ptr_p = dtp->dloop;
    return;
}

void MPIO_Datatype_get_loopsize(MPI_Datatype type, int *size_p, int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    assert(dtp->valid & MPIO_DATATYPE_VALID_SIZE);

    *size_p = dtp->dloop_size;
    return;
}

void MPIO_Datatype_get_loopdepth(MPI_Datatype type, int *depth_p, int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    assert(dtp->valid & MPIO_DATATYPE_VALID_DEPTH);

    *depth_p = dtp->dloop_depth;
    return;
}

void MPIO_Datatype_set_loopptr(MPI_Datatype type, MPIO_Dataloop *ptr, int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    if (!attrflag) {
	dtp = MPIO_Datatype_allocate(type);
    }

    printf("set loopptr = %x\n", (int) ptr);

    dtp->dloop  = ptr;
    dtp->valid |= MPIO_DATATYPE_VALID_PTR;
    return;
}

void MPIO_Datatype_set_loopsize(MPI_Datatype type, int size, int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    if (!attrflag) {
	dtp = MPIO_Datatype_allocate(type);
    }

    dtp->dloop_size  = size;
    dtp->valid      |= MPIO_DATATYPE_VALID_SIZE;
    return;
}

void MPIO_Datatype_set_loopdepth(MPI_Datatype type, int depth, int flag)
{
    int mpi_errno, attrflag;
    MPIO_Datatype *dtp;

    assert(MPIO_Datatype_keyval != MPI_KEYVAL_INVALID);

    mpi_errno = MPI_Type_get_attr(type, MPIO_Datatype_keyval, &dtp, &attrflag);
    assert(mpi_errno == MPI_SUCCESS);
    if (!attrflag) {
	dtp = MPIO_Datatype_allocate(type);
    }

    dtp->dloop_depth  = depth;
    dtp->valid       |= MPIO_DATATYPE_VALID_DEPTH;
    return;
}

int MPIO_Datatype_is_nontrivial(MPI_Datatype type)
{
    int nr_ints, nr_aints, nr_types, combiner;

    PMPI_Type_get_envelope(type, &nr_ints, &nr_aints, &nr_types, &combiner);
    if (combiner != MPI_COMBINER_NAMED ||
	type == MPI_FLOAT_INT ||
	type == MPI_DOUBLE_INT ||
	type == MPI_LONG_INT ||
	type == MPI_SHORT_INT ||
	type == MPI_LONG_DOUBLE_INT) return 1;
    else return 0;
}

/* internal functions */

static MPIO_Datatype *MPIO_Datatype_allocate(MPI_Datatype type)
{
    int mpi_errno;
    MPIO_Datatype *dtp;

    dtp = (MPIO_Datatype *) malloc(sizeof(MPIO_Datatype));
    assert(dtp != NULL);
    dtp->valid       = 0;
    dtp->refct       = 1;
    dtp->dloop       = NULL;
    dtp->dloop_size  = -1;
    dtp->dloop_depth = -1;
    
    mpi_errno = MPI_Type_set_attr(type, MPIO_Datatype_keyval, dtp);
    assert(mpi_errno == MPI_SUCCESS);

    printf("allocated attr struct\n");

    return dtp;
}

/* MPIO_Datatype_set_szext()
 *
 * Calculates size and extent of type, fills in values in MPIO_Datatype
 * structure, and sets valid flag.
 *
 * Note: This code currently checks for compatible variable sizes at
 *       runtime, while this check could instead be performed at configure
 *       time to save a few instructions. This seems like micro-optimization,
 *       so I skipped it for now. -- RobR, 03/22/2007
 */
static void MPIO_Datatype_set_szext(MPI_Datatype type, MPIO_Datatype *dtp)
{
    int mpi_errno;

    if (sizeof(int) == sizeof(MPI_Offset) &&
	sizeof(MPI_Aint) == sizeof(MPI_Offset))
    {
	int size;
	MPI_Aint lb, extent;
	
	mpi_errno = MPI_Type_size(type, &size);
	assert(mpi_errno == MPI_SUCCESS);
	
	mpi_errno = MPI_Type_get_extent(type, &lb, &extent);
	assert(mpi_errno == MPI_SUCCESS);
	
	dtp->size   = (MPI_Offset) size;
	dtp->extent = (MPI_Offset) extent;
    }
    else {
	MPIO_Type_footprint tfp;
	
	MPIO_Type_calc_footprint(type, &tfp);
	dtp->size   = tfp.size;
	dtp->extent = tfp.extent;
    }

    dtp->valid |= MPIO_DATATYPE_VALID_TYPESZEXT;
    return;
}
