#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int verbose = 0;

/* tests */
int blockindexed_contig_test(void);

/* helper functions */
int parse_args(int argc, char **argv);
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
    err = blockindexed_contig_test();
    if (err && verbose) fprintf(stderr, "%d errors in blockindexed test.\n",
				err);
    errs += err;

    /* print message and exit */
    if (errs) {
	fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
	printf(" No errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* blockindexed_contig_test()
 *
 * Tests behavior with a blockindexed that can be converted to a 
 * contig easily.  This is specifically for coverage.
 *
 * Returns the number of errors encountered.
 */
int blockindexed_contig_test(void)
{
    int bufsz;
    char *buf;
    int err, errs = 0;

    int count = 1;
    MPI_Aint disp = 0;
    MPI_Datatype newtype;

    int size, int_size;
    MPI_Aint extent;

    err = MPI_Type_create_indexed_block(count,
					1,
					&disp,
					MPI_INT,
					&newtype);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating struct type in blockindexed_contig_test()\n");
	}
	errs++;
    }

    MPI_Type_size(MPI_INT, &int_size);

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error obtaining type size in blockindexed_contig_test()\n");
	}
	errs++;
    }
    
    if (size != int_size) {
	if (verbose) {
	    fprintf(stderr,
		    "error: size != int_size in blockindexed_contig_test()\n");
	}
	errs++;
    }    

    err = MPI_Type_extent(newtype, &extent);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error obtaining type extent in blockindexed_contig_test()\n");
	}
	errs++;
    }
    
    if (extent != int_size) {
	if (verbose) {
	    fprintf(stderr,
		    "error: extent != int_size in blockindexed_contig_test()\n");
	}
	errs++;
    }    

    MPI_Type_commit(&newtype);

    MPI_Pack_size(1, newtype, MPI_COMM_SELF, &bufsz);
    buf = (char *) malloc(bufsz);

    err = pack_and_unpack(buf, 1, newtype, bufsz);
    if (err != 0) {
	if (verbose) {
	    fprintf(stderr,
		    "error packing/unpacking in blockindexed_contig_test()\n");
	}
	errs += err;
    }

    free(buf);

    return errs;
}


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

int parse_args(int argc, char **argv)
{
    /*
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0)
    {
	switch (ret) {
	    case 'v':
		verbose = 1;
		break;
	}
    }
    */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
	verbose = 1;
    return 0;
}

