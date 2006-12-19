/*
 * $COPYRIGHT$
 */

typedef struct mpimutex {
	int nprocs, myrank, homerank;
	MPI_Comm comm;
	MPI_Win waitlistwin;
	MPI_Datatype waitlisttype;
	unsigned char *waitlist; /* only allocated on home rank */
} *mpimutex_t;

typedef struct fp_data {
	MPI_Offset fp;
	unsigned char waitlist[1];
} fp_data_t;

typedef struct mpimutex_fp {
	int nprocs, myrank, homerank;
	MPI_Comm comm;
	MPI_Win waitlistwin;
	MPI_Datatype waitlisttype, fptype;
	fp_data_t *data;
	int fd;    /* XXX: unused? */
} *mpimutex_fp_t;



int ADIOI_MPIMUTEX_Create(int homerank, MPI_Comm comm, mpimutex_t *mutex_p);
int ADIOI_MPIMUTEX_Lock(mpimutex_t mutex);
int ADIOI_MPIMUTEX_Unlock(mpimutex_t mutex);
int ADIOI_MPIMUTEX_Free(mpimutex_t *mutex_p);

/* need to find a better way to integerate the two slightly different data
 * types */
int ADIOI_MPIMUTEX_FP_Create(int homerank, MPI_Comm comm, 
		mpimutex_fp_t *mutex_p);
int ADIOI_MPIMUTEX_FP_Lock(mpimutex_fp_t mutex);
int ADIOI_MPIMUTEX_FP_Unlock(mpimutex_fp_t mutex);
int ADIOI_MPIMUTEX_FP_Free(mpimutex_fp_t *mutex_p);
int ADIOI_MPIMUTEX_FP_Fetch_and_increment(mpimutex_fp_t mutex, 
		MPI_Offset *current, MPI_Offset increment);
int ADIOI_MPIMUTEX_FP_Set(mpimutex_fp_t mutex, MPI_Offset value);

int ADIOI_MPIMUTEX_FP_Get(mpimutex_fp_t mutex, MPI_Offset *value);
