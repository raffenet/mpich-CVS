#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mpi.h"

static int verbose = 0;

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);
int single_struct_test(void);
int array_of_structs_test(void);


struct test_struct_1 {
    int a,b;
    char c,d;
    int e;
};

int main(int argc, char *argv[])
{
    int err, errs = 0;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    err = single_struct_test();
    if (verbose && err) fprintf(stderr, "error in single_struct_test\n");
    errs += err;

    err = array_of_structs_test();
    if (verbose && err) fprintf(stderr, "error in array_of_structs_test\n");
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

int single_struct_test(void)
{
    int err, errs = 0;
    int bufsize, position = 0;
    struct test_struct_1 ts1, ts2;
    MPI_Datatype mystruct;
    char *buffer;

    MPI_Aint disps[3] = {0, 2*sizeof(int), 3*sizeof(int)}; /* guessing... */
    int blks[3] = { 2, 2, 1 };
    MPI_Datatype types[3] = { MPI_INT, MPI_CHAR, MPI_INT };

    ts1.a = 1;
    ts1.b = 2;
    ts1.c = 3;
    ts1.d = 4;
    ts1.e = 5;

    err = MPI_Type_struct(3, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_struct returned error\n");
	}
    }

    MPI_Type_commit(&mystruct);

    MPI_Pack_size(1, mystruct, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc(bufsize);

    err = MPI_Pack(&ts1,
		   1,
		   mystruct,
		   buffer,
		   bufsize,
		   &position,
		   MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Pack returned error\n");
	}
    }

    position = 0;
    err = MPI_Unpack(buffer,
		     bufsize,
		     &position,
		     &ts2,
		     1,
		     mystruct,
		     MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Unpack returned error\n");
	}
    }

    MPI_Type_free(&mystruct);
    free(buffer);

    if (ts1.a != ts2.a) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.a = %d; should be %d\n", ts2.a, ts1.a);
	}
    }
    if (ts1.b != ts2.b) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.b = %d; should be %d\n", ts2.b, ts1.b);
	}
    }
    if (ts1.c != ts2.c) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.c = %d; should be %d\n",
		    (int) ts2.c, (int) ts1.c);
	}
    }
    if (ts1.d != ts2.d) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.d = %d; should be %d\n",
		    (int) ts2.d, (int) ts1.d);
	}
    }
    if (ts1.e != ts2.e) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.e = %d; should be %d\n", ts2.e, ts1.e);
	}
    }

    return errs;
}

int array_of_structs_test(void)
{
    int i, err, errs = 0;
    int bufsize, position = 0;
    struct test_struct_1 ts1[10], ts2[10];
    MPI_Datatype mystruct;
    char *buffer;

    MPI_Aint disps[3] = {0, 2*sizeof(int), 3*sizeof(int)}; /* guessing... */
    int blks[3] = { 2, 2, 1 };
    MPI_Datatype types[3] = { MPI_INT, MPI_CHAR, MPI_INT };

    for (i=0; i < 10; i++) {
	ts1[i].a = 10*i + 1;
	ts1[i].b = 10*i + 2;
	ts1[i].c = 10*i + 3;
	ts1[i].d = 10*i + 4;
	ts1[i].e = 10*i + 5;

	ts2[i].a = -13;
	ts2[i].b = -13;
	ts2[i].c = -13;
	ts2[i].d = -13;
	ts2[i].e = -13;
    }

    err = MPI_Type_struct(3, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_struct returned error\n");
	}
    }

    MPI_Type_commit(&mystruct);

    MPI_Pack_size(10, mystruct, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc(bufsize);

    err = MPI_Pack(ts1,
		   10,
		   mystruct,
		   buffer,
		   bufsize,
		   &position,
		   MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Pack returned error\n");
	}
    }

    position = 0;
    err = MPI_Unpack(buffer,
		     bufsize,
		     &position,
		     ts2,
		     10,
		     mystruct,
		     MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Unpack returned error\n");
	}
    }

    MPI_Type_free(&mystruct);
    free(buffer);

    for (i=0; i < 10; i++) {
	if (ts1[i].a != ts2[i].a) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].a = %d; should be %d\n",
			i, ts2[i].a, ts1[i].a);
	    }
	}
	if (ts1[i].b != ts2[i].b) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].b = %d; should be %d\n",
			i, ts2[i].b, ts1[i].b);
	    }
	}
	if (ts1[i].c != ts2[i].c) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].c = %d; should be %d\n",
			i, (int) ts2[i].c, (int) ts1[i].c);
	    }
	}
	if (ts1[i].d != ts2[i].d) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].d = %d; should be %d\n",
			i, (int) ts2[i].d, (int) ts1[i].d);
	    }
	}
	if (ts1[i].e != ts2[i].e) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].e = %d; should be %d\n",
			i, ts2[i].e, ts1[i].e);
	    }
	}
    }

    return errs;
}


int parse_args(int argc, char **argv)
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
