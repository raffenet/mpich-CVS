/* $Id$ */

/*****
    First pass at an MPI based forward projector which distributes the volume via MPI_Bcast
    and then has all slave nodes compute plane ranges for a specified group/subset. The
    overlapping planes are distributed to other nodes.
*****/

#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

static char progid[]="$Id$";

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>

#include "mpi.h"


#define EPS 1.0e-9
#define PLANE_DATA 199

typedef struct {
    int nslaves;
    int nplanes;
    int noverlap;
    char *pop;
} Pop;

typedef struct {
    int low;
    int high;
} Range;

typedef struct volume {
    unsigned int xdim, ydim, zdim;
    float xy_voxel_size, z_voxel_size;
    float *data;
} Volume;

typedef struct view2d {
    int xdim, ydim;
    float x_pixel_size, y_pixel_size;
    float *data;
} View;

/* For this program the angles phi and theta have the following meanings:
    phi is the axial angle, the angle between the LORs in a plane and the vertical
    theta is the sinogram view angle with values from 0 to pi.
*/

double sin_phi, cos_phi;
double *sin_theta, *cos_theta;
float xoff=0.0;
float yoff=0.0;
float zoom=1.0;     /* XY Magnification - for iterative work, needs to be 1.0 */
float zscale=1.0;   /* Z axis zoom */
float xyscale;      /* ratio of image pixels to sinogram bins */
float zxscale;      /* ratio of sinogram bin size to plane separation */

float pixel_size;
float bin_size;
float plane_separation;
char *pmask=NULL;

int nprojs=256;
int nviews=288;
int nrings=104;
float rspace=0.24375;
float rdiam=46.9;
int span=9;
int maxrd=67;
int local_files=1;

Pop *compute_pop( int nslaves, int feather, int nplanes, int noverlap);
int get_compute_plane( Pop *pop, int plane);
int compute_range( Pop *pop, int slave, int *lo, int *hi);
int active_range( Pop *pop, int slave, int *lo, int *hi);
void get_pop_counts( Pop *pop, int slave, int *ncomputes, int *nwrites, int *nreads);
void free_pop (Pop *pop);

void crash( char * fmt, ...);
char *datestamp(void);
char *timestamp( int rank, float dt);
void usage(void);
int *subset_order( int nss);

char *datestamp(void)
{
    time_t curtime;
    struct tm *loctime;
    static char bufr[256];

    curtime = time(NULL);
    loctime = localtime( &curtime);
    strftime( bufr, 256, "%c ", loctime);
    return (bufr);
}

char *timestamp( int rank, float dt)
{
    static char tsbufr[256];
    int min; float sec;

    min=dt/60.;
    sec=dt-60*min;

    sprintf( tsbufr, "<%02d> %02d:%05.2f %.2f", rank, min, sec, dt);
    return (tsbufr);
}

void usage()
{
    printf("%s scan_file shift_file norm_file image_file isize,nss,niter\n", progid);
    exit(1);
}

int *subset_order( int nss)
{
    int i, j;
    static int*list=NULL;

    j=nss/2;
    if (!list) list=(int*) malloc( nss*sizeof(int));
    if (j==0) list[j] = 0;
    else
    {
      list=subset_order(j);
      for (i=0; i<j; i++) list[i] = 2*list[i], list[j+i] = 1+list[i];
    }
    return list;
}

#define TSTAMP timestamp( myrank, MPI_Wtime()-t0)

int myrank=0;
double t0;
int isize=256;
int nsubsets=8;
int niters=1;
int nthreads=1;
int save_fpdata=0;
int save_nvol=0;
int save_group_vols=0;

Pop *pop=NULL;

int main (int argc, char **argv)
{

    int master, slave, nslaves;
    int nprocs;
    MPI_Status status;
    int io_busy=0;
    MPI_Request *io_requests;



    int maxseg, np, z0, sp2, nsegs, segnzs, ngroups, maxgroup, mingroup, gdir=1;
    int *segz0, *segnz, *segzoff, *segfoff, *segnum, *segolap;
    int i, j, rsq, ix, iy;

    Volume vol, cvol, nvol;
    View vbufr;

    float *scan_data, *shift_data, *norm_data;
    int psize, nr, nt, nz, p, v;
    int ap0, ap1, cp0, cp1;

    int seg,ierr,feather=0;

    FILE *scan_file, *shift_file, *norm_file, *nvol_file;
    FILE *local_scan, *local_shift, *local_norm, *local_nvol;

    long fpos, curgroup=-1;

    unsigned int nvals, nread, nout;

    int rank0, ss, plane, view, mypid;
    int iter, group, subset, ss_view;

    double t1, t2, t3, a, c;
    double times[8];
    char pname[256];
    char fname[256];
    int namelen;
    float dt, cdt, xdt, rdt, bdt, wdt, ndt, rate;

    off_t *foffs, poff;
    int retval;
    float *ftemp, *fbufr1, *fbufr2, *p1, *p2;
    int *my_planes_start, *my_planes_np;

    int *ss_order, doff;
    float *dptr;
    float stats[3];

    MPI_Request *writeque=NULL, *readque=NULL;
    int nwq, nrq, wqsize, rqsize;

    int nreads, nputs, ncomps, mypop, cnode;

    MPI_Init( &argc, &argv);
    MPI_Comm_rank( MPI_COMM_WORLD, &myrank);
    MPI_Comm_size( MPI_COMM_WORLD, &nprocs);
    MPI_Get_processor_name( pname, &namelen);
    pname[namelen]=0;
    MPI_Barrier( MPI_COMM_WORLD);
    /*    if (myrank == 0 && argc < 2) usage(); */
    if (myrank == 0) printf("%s\n", progid);
    t0=MPI_Wtime();
    mypid = getpid();
    printf("%s Hello...I am rank %d of %d procs, my processor name is %s and my pid is %d\n",
        datestamp(), myrank, nprocs, pname, mypid);
    fflush(stdout);
    master=rank0=(myrank==0);
    for (i=0; i<8; i++) times[i]=0.0;
    fflush(stdout);
    maxseg=maxrd/span;
    ngroups=maxseg+1;
    nsegs=2*maxseg+1;
    np=2*nrings-1;
    sp2=(span+1)/2;
    segz0=(int*) malloc( nsegs*sizeof(int));
    segnz=(int*) malloc( nsegs*sizeof(int));
    segzoff=(int*) malloc( nsegs*sizeof(int));
    segfoff=(int*) malloc( nsegs*sizeof(int));
    segnum=(int*) malloc( nsegs*sizeof(int));
    segolap=(int*) malloc( nsegs*sizeof(int));
    segnzs=0;
    for (i=0; i<nsegs; i++)
    {
      segnum[i]=(1-2*(i%2))*(i+1)/2;
      if (i==0) segz0[0]=0;
      segolap[i]=6*((i+1)/2)-1;
      if (i==0) segolap[i]=0;
      else segz0[i]=sp2+span*((i-1)/2);
      segnz[i]=np-2*segz0[i];
      segnzs+=segnz[i];
      if (i==0) segzoff[0]=segfoff[0]=0;
      else segzoff[i] = segzoff[i-1] + segnz[i-1];
      if (i%2) segfoff[i]=segfoff[i-1]+segnz[i-1];
      if ((i>0) && (i%2 == 0)) segfoff[i] = segfoff[i-1];
      if (rank0) printf("seg %2d num=%2d z0=%4d nz=%4d olap=%4d zoff=%4d foff=%4d\n",
            i, segnum[i], segz0[i], segnz[i], segolap[i], segzoff[i], segfoff[i]);
    }
    bin_size = rspace*0.5;
    plane_separation = rspace*0.5;
    /*    printf("Total sinogram planes=%d\n", segnzs); */
    sin_theta=(double*) calloc( nviews, sizeof(double));
    cos_theta=(double*) calloc( nviews, sizeof(double));
    for (i=0; i<nviews; i++)
      sincos( M_PI*i/nviews, sin_theta+i, cos_theta+i);
    /*    printf("sin/cos calculated for rank %d\n", myrank); */
    maxgroup=ngroups-1;
    mingroup=0;
    niters=2;
    feather=span;
    if (rank0)
        printf("Parameters are isize=%d, nsubsets=%d, nthreads=%d, niters=%d, maxgroup=%d, mingroup=%d\n",
            isize, nsubsets, nthreads, niters, maxgroup, mingroup);
    if (niters == 0) save_group_vols=1;
    if (maxgroup < mingroup) gdir=-1;
    switch (niters)
    {
        case  0:    save_group_vols = 1; break;
        case -1:    save_fpdata = 1; break;
        case -2:    save_nvol = 1; break;
    }
    if (save_fpdata) nsubsets=1; /* Force nsubsets to 1 for forward projection function */
    ss_order = subset_order( nsubsets);
    if (rank0) {
        printf("Subset Order = ");
        for (i=0; i<nsubsets; i++) printf("%d ", ss_order[i]);
        printf("\n");
        fflush(stdout);
    }
    psize=isize*isize;
    vol.xdim = vol.ydim = isize;
    vol.zdim = np;
    vol.data = (float*) malloc( psize*np*sizeof(float));
    if (!vol.data) crash("malloc failed in node %d for vol.data\n", myrank);
    cvol.xdim = cvol.ydim = isize;
    cvol.zdim = np;
    cvol.data = (float*) malloc( psize*np*sizeof(float));
    if (!cvol.data) crash("malloc failed in node %d for cvol data\n", myrank);
    nvol.xdim = nvol.ydim = isize;
    nvol.zdim = np;
    nvol.data = (float*) malloc( psize*np*sizeof(float));
    if (!nvol.data) crash("malloc failed in node %d for nvol data\n", myrank);
    vbufr.data=(float*) NULL;
    xyscale = (float) nprojs / ((float) isize*zoom);
    zxscale = 1.0;
    vol.xy_voxel_size = xyscale * rspace/2.0;
    vol.z_voxel_size = rspace/2.0;
    scan_data = (float*) malloc( nprojs*nviews*np*2*sizeof(float));
    if (!scan_data) crash("malloc failed for scan_data\n");
    shift_data = (float*) malloc( nprojs*nviews*np*2*sizeof(float));
    if (!shift_data) crash("malloc failed for shift_data\n");
    norm_data = (float*) malloc( nprojs*nviews*np*2*sizeof(float));
    if (!norm_data) crash("malloc failed for norm_data\n");
    pmask = (char*) calloc( psize, sizeof(char));
    if (!pmask) crash("calloc failed for pmask\n");
    rsq = (isize)*(isize)/4;
    for (i=0,iy=0; iy<isize; iy++)
      for (ix=0; ix<isize; ix++, i++)
        pmask[i] = (((ix-isize/2)*(ix-isize/2)+(iy-isize/2)*(iy-isize/2)) < rsq) ? 0 : 1;
    /*    printf("Memory allocated for rank=%d\n", myrank); */
    nvals = psize*np;
    /* Initialize the volume - will be broadcast if initial_image is specified */
    for (i=0,p=0; p<np; p++)
      for (j=0; j<psize; j++, i++)
        vol.data[i] = 1-pmask[j];
    iter = 0;
    /*    if (local_files) goto next_iter; */
next_iter:
    group = mingroup;
    printf("%s Starting Iteration %d\n", TSTAMP, iter); fflush(stdout);
next_group:
    t2=MPI_Wtime();
    if (pop) free_pop(pop);
    pop = compute_pop( nprocs, feather, segnz[2*group], segolap[2*group]);
    get_pop_counts( pop, myrank, &ncomps, &nputs, &nreads);

    active_range( pop, myrank, &ap0, &ap1);
    compute_range( pop, myrank, &cp0, &cp1);

    np = segnz[2*group];
    z0 = segz0[2*group];
    if (writeque) free(writeque);
    /*    writeque = (MPI_Request*) malloc( nputs*sizeof(MPI_Request)); */
    wqsize=ncomps*(nprocs-1);
    writeque = (MPI_Request*) malloc( wqsize*sizeof(MPI_Request));
    if (readque) free(readque);
    /*    readque = (MPI_Request*) malloc( nreads*sizeof(MPI_Request)); */
    rqsize=np-ncomps;
    readque = (MPI_Request*) malloc( rqsize*sizeof(MPI_Request));
    nvals = np * nprojs * nviews;
    if (group>0) nvals *= 2;
    subset=0;
next_subset:
    ss_view = ss_order[subset];
    t1=MPI_Wtime();
    /*  We will now forward project the group subset and distribute the planes to other slaves. */
    nrq=nwq=0;  /* we have nothing in read request queue or write request queue */
    t2 = MPI_Wtime();
    memset( cvol.data, 0, psize*cvol.zdim*sizeof(float));
    memset( nvol.data, 0, psize*nvol.zdim*sizeof(float));
    /*    printf("%s ap0=%d, ap1=%d, cp0=%d, cp1=%d\n", TSTAMP, ap0, ap1, cp0, cp1); fflush( stdout); */
    nz = np;
    t1=MPI_Wtime();
    nrq=nwq=0;
    for (p=feather; p<np-feather; p++)
    {
      mypop = pop->pop[p*pop->nslaves+myrank];
      doff = (p+z0)*psize;
      dptr = vol.data+doff;
      nvals = psize;
      if (mypop == 'C')    /* are we supposed to compute this plane? */
      {
        for (i=0; i<nvals; i++) dptr[i]=1.0*p;
        for (i=0; i<pop->nslaves; i++)
            if (myrank != i) MPI_Isend( dptr, nvals, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &writeque[nwq++]);
      } else
      {
        cnode = get_compute_plane( pop, p);
        MPI_Irecv( dptr, nvals, MPI_FLOAT, cnode, 0, MPI_COMM_WORLD, &readque[nrq++]);
      }
      if (nwq>wqsize) crash("%s ERROR nwq[%d] > wqsize[%d]\n", TSTAMP, nwq, wqsize);
      if (nrq>rqsize) crash("%s ERROR nrq[%d] > rqsize[%d]\n", TSTAMP, nrq, rqsize);
    }
    /* Wait for all incoming data to be transfered (if any) */
    if (nrq)
    {
        printf("%s Group %d Subset %d readque=%d MPI_Waitall executing\n", TSTAMP, group, subset, nrq);
        fflush(stdout);
        MPI_Waitall( nrq, readque, MPI_STATUS_IGNORE);
    }
    /* and wait for all outgoing data to be transferred (if any) */
    if (nwq)
    {
        printf("%s Group %d Subset %d writeque=%d MPI_Waitall executing\n", TSTAMP, group, subset, nwq);
        fflush(stdout);
        MPI_Waitall( nwq, writeque, MPI_STATUS_IGNORE);
    }
    xdt = MPI_Wtime()-t1; times[5] += xdt;
    printf("%s Group %d Subset %d Image Distribution Finished nwq=%d, nrq=%d in %0.1f sec\n",
        TSTAMP, group, subset, nwq, nrq, xdt);
    fflush(stdout);
    subset++;
    if (subset < nsubsets) goto next_subset;
    printf("%s Group_done %d np=%d Times={%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f}\n",
        TSTAMP, group, ncomps, times[0], times[1], times[2], times[3], times[4], times[5], times[6]);
    fflush(stdout);

group_advance:
    group+=gdir;
    if (group != maxgroup+gdir) goto next_group;
    iter++;
    if (iter < niters) goto next_iter;
    times[6] = MPI_Wtime()-t0;
    MPI_Barrier( MPI_COMM_WORLD);
    printf("%s Times for rank %d  %s %d Times={%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f}\n",
        TSTAMP, myrank, pname, getpid(),
        times[0], times[1], times[2], times[3], times[4], times[5], times[6]);
    fflush(stdout);
    MPI_Finalize();
    exit(0);
}


void crash( char * fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf( stderr, fmt, ap);
    va_end(ap);
    
    exit(1);
}

/* compute_pop */

/*  This routine computes the plane operation matrix for 3D iterative reconstruction.
    pop = (char*) compute_pop( nslaves, nplanes, noverlap)

    where nslaves is the number of slave nodes,
          nplanes is the number of planes (in the segment),
          noverlap is the number of additional planes needed for each slave.
    The results are returned in a 2D char array [nslaves,nplanes] where each
    element contains NULL (no operation for this slave)
                     'C' - compute this plane
                     'R' - read this plane from another slave
    The calling routine is responsible for freeing the memory (if needed).
*/

Pop *compute_pop( int nslaves, int feather, int nplanes, int noverlap)
{
    int i,j,slave,plane,np,start,fplanes;
    Pop *pop;

    start = feather;
    fplanes = nplanes-2*feather;
    pop = (Pop*) malloc( sizeof (Pop));
    pop->nslaves = nslaves;
    pop->nplanes = nplanes;
    pop->noverlap = noverlap;
    pop->pop = (char*) calloc( nslaves*nplanes, 1);
    for (slave=0; slave<nslaves; slave++)
    {
        np = fplanes/nslaves;
        if (nslaves-1-slave<fplanes%nslaves) np++;
        for (i=0; i<np; i++)
            pop->pop[(i+start)*nslaves+slave] = 'C';
        for (i=0; i<noverlap; i++)
        {
            j = start-i-1;
            if (j>=0) pop->pop[j*nslaves+slave] = 'R';
            j = start+np+i;
            if (j<fplanes) pop->pop[j*nslaves+slave] = 'R';
        }
        start += np;
    }
    return (pop);
}

int get_compute_plane( Pop *pop, int plane)
    /* This routine returns the node number which computes the specified plane */
{
    int i;
    for (i=0; i<pop->nslaves; i++)
      if (pop->pop[plane*pop->nslaves+i] == 'C') return (i);
    return (-1);
}

int compute_range( Pop *pop, int slave, int *lo, int *hi)
{
    int i, j;

    *lo = *hi = -1;
    if ((slave < 0) || (slave > pop->nslaves-1)) return 1;
    for (i=0; i<pop->nplanes; i++)
    {
        j=pop->nplanes-1-i;
        if (pop->pop[j*pop->nslaves+slave] == 'C') *lo = j;
        if (pop->pop[i*pop->nslaves+slave] == 'C') *hi = i;
    }
    return 0;
}

int active_range( Pop *pop, int slave, int *lo, int *hi)
{
    int i, j, c;

    *lo = *hi = -1;
    if ((slave < 0) || (slave > pop->nslaves-1)) return 1;
    for (i=0; i<pop->nplanes; i++)
    {
        j=pop->nplanes-1-i;
        c = pop->pop[j*pop->nslaves+slave];
        if ((c == 'R') || (c == 'C')) *lo = j;
        c = pop->pop[i*pop->nslaves+slave];
        if ((c == 'R') || (c == 'C')) *hi = i;
    }
    return 0;
}

void get_pop_counts( Pop *pop, int slave, int *ncomputes, int *nwrites, int *nreads)
{
    int i, j, k, op, ncomps, nputs, ngets;

    ncomps=nputs=ngets=0;
    for (i=0; i<pop->nplanes; i++)
    {
        op = pop->pop[i*pop->nslaves+slave];
        if (op == 'R') ngets++;
        else if (op == 'C')
        {
            ncomps++;
            for (k=0; k<pop->nslaves; k++)
            {
                op = pop->pop[i*pop->nslaves+k];
                if (op == 'R') nputs++;
            }
        }
    }
    *ncomputes=ncomps;
    *nwrites=nputs;
    *nreads=ngets;
}

void free_pop (Pop *pop)
{
    free(pop->pop);
    free(pop);
}
