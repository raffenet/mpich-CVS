#include "mpi.h"
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

static int verbose = 0;

/* tests */
int subarray_2d_c_test1(void);
int subarray_2d_c_test2(void);
int subarray_2d_fortran_test1(void);

/* helper functions */
static int parse_args(int argc, char **argv);
static int pack_and_unpack(char *typebuf,
			   int count,
			   MPI_Datatype datatype,
			   int typebufsz);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv); /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* perform some tests */
    err = subarray_2d_c_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray c test 1.\n", err);
    errs += err;

    err = subarray_2d_fortran_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray fortran test 1.\n",
				err);
    errs += err;

    err = subarray_2d_c_test2();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray c test 2.\n", err);
    errs += err;

    /* print message and exit */
    if (errs) {
	fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
	printf("No errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* subarray_2d_test()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_c_test1(void)
{
    MPI_Datatype subarray;
    int array[9] = { -1, -2, -3,
		     -4,  1,  2,
		     -5,  3,  4 };
    int array_size[2] = {3, 3};
    int array_subsize[2] = {2, 2};
    int array_start[2] = {1, 1};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 4*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, 4*sizeof(int));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 9*sizeof(int));

    for (i=0; i < 9; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
		break;
	    case 7:
		goodval = 3;
		break;
	    case 8:
		goodval = 4;
		break;
	    default:
		goodval = 0;
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&subarray);
    return errs;
}

/* subarray_2d_c_test2()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_c_test2(void)
{
    MPI_Datatype subarray;
    int array[12] = { -1, -2, -3, -4,  1,   2,
		      -5, -6, -7, -8, -9, -10 };
    int array_size[2] = {2, 6};
    int array_subsize[2] = {1, 2};
    int array_start[2] = {0, 4};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 2*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, 2*sizeof(int));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 12*sizeof(int));

    for (i=0; i < 12; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
		break;
	    default:
		goodval = 0;
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&subarray);
    return errs;
}

/* subarray_2d_fortran_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_fortran_test1(void)
{
    MPI_Datatype subarray;
    int array[12] = { -1, -2, -3, -4,  1,   2,
		      -5, -6, -7, -8, -9, -10 };
    int array_size[2] = {6, 2};
    int array_subsize[2] = {2, 1};
    int array_start[2] = {4, 0};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_FORTRAN,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 2*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, 2*sizeof(int));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 12*sizeof(int));

    for (i=0; i < 12; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
		break;
	    default:
		goodval = 0;
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&subarray);
    return errs;
}

/******************************************************************/

/* pack_and_unpack()
 *
 * Perform packing and unpacking of a buffer for the purposes of checking
 * to see if we are processing a type correctly.  Zeros the buffer between
 * these two operations, so the data described by the type should be in
 * place upon return but all other regions of the buffer should be zero.
 *
 * Parameters:
 * typebuf - pointer to buffer described by datatype and count that
 *           will be packed and then unpacked into
 * count, datatype - description of typebuf
 * typebufsz - size of typebuf; used specifically to zero the buffer
 *             between the pack and unpack steps
 *
 */
static int pack_and_unpack(char *typebuf,
			   int count,
			   MPI_Datatype datatype,
			   int typebufsz)
{
    char *packbuf;
    int err, errs = 0, pack_size, type_size, position;

    err = MPI_Type_size(datatype, &type_size);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_size call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    type_size *= count;

    err = MPI_Pack_size(count, datatype, MPI_COMM_SELF, &pack_size);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Pack_size call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }
    packbuf = (char *) malloc(pack_size);
    if (packbuf == NULL) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in malloc call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    position = 0;
    err = MPI_Pack(typebuf,
		   count,
		   datatype,
		   packbuf,
		   type_size,
		   &position,
		   MPI_COMM_SELF);

    if (position != type_size) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (pack)\n",
			     position, type_size);
    }

    memset(typebuf, 0, typebufsz);
    position = 0;
    err = MPI_Unpack(packbuf,
		     type_size,
		     &position,
		     typebuf,
		     count,
		     datatype,
		     MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Unpack call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }
    free(packbuf);

    if (position != type_size) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (unpack)\n",
			     position, type_size);
    }

    return errs;
}

static int parse_args(int argc, char **argv)
{
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0)
    {
	switch (ret) {
	    case 'v':
		verbose = 1;
		break;
	}
    }
    return 0;
}

