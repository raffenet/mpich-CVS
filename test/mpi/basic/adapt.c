#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "GetOpt.h"

#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define  TRIALS 	7
#define  REPEAT 	1000
int 	 g_NSAMP =	250;
#define  PERT		3
#define  LONGTIME	1e99
#define  CHARSIZE	8
#define  RUNTM		0.25
double	 g_STOPTM = 	0.1;
#define  MAXINT 	2147483647
int      g_latency012_reps = 1000;

#define MIN(x, y)	(((x) < (y))?(x):(y))
#define MAX(x, y)	(((x) > (y))?(x):(y))

#ifdef HAVE_WINDOWS_H
#define POINTER_TO_INT(a)   ( ( int )( 0xffffffff & (__int64) ( a ) ) )
#else
#define POINTER_TO_INT(a)   ( ( int )( a ) )
#endif

#define When MPI_Wtime

int g_nIproc = 0, g_nNproc = 0;

typedef struct argstruct ArgStruct;
struct argstruct 
{
    char *buff;	        /* Send buffer      */
    char *buff1;        /* Recv buffer      */
    int  bufflen;       /* Length of buffer */
    int  nbor,          /* neighbor */
	 iproc,         /* rank */
	 tr,            /* transmitter/receiver flag */
	 latency_reps;  /* reps needed to time latency */
};

typedef struct data Data;
struct data
{
    double t;
    double bps;
    int    bits;
    int    repeat;
};

int Setup(ArgStruct *p01, ArgStruct *p12);
void Sync(ArgStruct *p);
void Sync012();
#define SendData( p ) MPI_Send( p . buff,  p . bufflen, MPI_BYTE,  p . nbor, 1, MPI_COMM_WORLD)
#define RecvData( p ) MPI_Recv( p . buff1,  p . bufflen, MPI_BYTE,  p . nbor, 1, MPI_COMM_WORLD, &status)
void SendRecvData(ArgStruct *p);
void SendTime(ArgStruct *p, double *t);
void RecvTime(ArgStruct *p, double *t);
void SendReps(ArgStruct *p, int *rpt);
void RecvReps(ArgStruct *p, int *rpt);
double TestLatency(ArgStruct *p);
double TestLatency012();
void PrintOptions(void);
int DetermineLatencyReps(ArgStruct *p);
int DetermineLatencyReps012();

void PrintOptions()
{
    printf("\n");
    printf("Usage: adapt flags\n");
    printf(" flags:\n");
    printf("       -reps #iterations\n");
    printf("       -time stop_time\n");
    printf("       -start initial_msg_size\n");
    printf("       -end final_msg_size\n");
    printf("       -out outputfile\n");
    printf("       -nocache\n");
    printf("       -pert\n");
    printf("       -noprint\n");
    printf("Requires exactly three processes\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    FILE *out=0;		/* Output data file 				*/
    char s[255]; 		/* Generic string				*/
    char *memtmp;
    char *memtmp1;
    MPI_Status status;

    int i, j, n, nq,		/* Loop indices					*/
	bufoffset = 0,		/* Align buffer to this				*/
	bufalign = 16*1024,	/* Boundary to align buffer to			*/
	nrepeat,		/* Number of time to do the transmission	*/
	len,			/* Number of bytes to be transmitted		*/
	inc = 1,		/* Increment value				*/
	pert,			/* Perturbation value				*/
        ipert,                  /* index of the perturbation loop		*/
	start = 0,		/* Starting value for signature curve 		*/
	end = MAXINT,		/* Ending value for signature curve		*/
	printopt = 1;		/* Debug print statements flag			*/
    
    ArgStruct	args01, args12, args012;/* Argumentsfor all the calls		*/
    
    double t, t0, t1, t2,	/* Time variables				*/
	tlast01, tlast12, tlast012,/* Time for the last transmission		*/
	latency01, latency12,	/* Network message latency			*/
	latency012;             /* Network message latency to go from 0 -> 1 -> 2 */

    Data *bwdata01, *bwdata12, *bwdata012;/* Bandwidth curve data 		*/
    
    BOOL bNoCache = FALSE;
    BOOL bSavePert = FALSE;
    BOOL bUseMegaBytes = FALSE;

    MPI_Init(&argc, &argv);
    
    MPI_Comm_size(MPI_COMM_WORLD, &g_nNproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &g_nIproc);
    
    if (g_nNproc != 3)
    {
	if (g_nIproc == 0)
	    PrintOptions();
	MPI_Finalize();
	exit(0);
    }

    GetOptDouble(&argc, &argv, "-time", &g_STOPTM);
    GetOptInt(&argc, &argv, "-reps", &g_NSAMP);
    GetOptInt(&argc, &argv, "-start", &start);
    GetOptInt(&argc, &argv, "-end", &end);
    bNoCache = GetOpt(&argc, &argv, "-nocache");
    bUseMegaBytes = GetOpt(&argc, &argv, "-mb");
    if (GetOpt(&argc, &argv, "-noprint"))
	printopt = 0;
    bSavePert = GetOpt(&argc, &argv, "-pert");
    
    bwdata01 = malloc((g_NSAMP+1) * sizeof(Data));
    bwdata12 = malloc((g_NSAMP+1) * sizeof(Data));
    bwdata012 = malloc((g_NSAMP+1) * sizeof(Data));

    if (g_nIproc == 0)
	strcpy(s, "adapt.out");
    GetOptString(&argc, &argv, "-out", s);
    
    if (start > end)
    {
	fprintf(stdout, "Start MUST be LESS than end\n");
	exit(420132);
    }

    Setup(&args01, &args12);

    if (g_nIproc == 0)
    {
	if ((out = fopen(s, "w")) == NULL)
	{
	    fprintf(stdout,"Can't open %s for output\n", s);
	    exit(1);
	}
    }

    /* Calculate latency */
    switch (g_nIproc)
    {
    case 0:
	latency01 = TestLatency(&args01);
	/*printf("[0] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	RecvTime(&args01, &latency12);
	/*printf("[0] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    case 1:
	latency01 = TestLatency(&args01);
	/*printf("[1] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	SendTime(&args12, &latency01);
	latency12 = TestLatency(&args12);
	/*printf("[1] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	SendTime(&args01, &latency12);
	break;
    case 2:
	RecvTime(&args12, &latency01);
	/*printf("[2] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	latency12 = TestLatency(&args12);
	/*printf("[2] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    }

    latency012 = TestLatency012();

    if ((g_nIproc == 0) && printopt)
    {
	printf("Latency01 : %0.9f\n", latency01);
	printf("Latency12 : %0.9f\n", latency12);
	printf("Latency012: %0.9f\n", latency012);
	fflush(stdout);
	printf("Now starting main loop\n");
	fflush(stdout);
    }
    tlast01 = latency01;
    tlast12 = latency12;
    tlast012 = latency012;
    inc = (start > 1) ? start/2: inc;
    args01.bufflen = start;
    args12.bufflen = start;
    args012.bufflen = start;

    /* Main loop of benchmark */
    for (nq = n = 0, len = start; 
         n < g_NSAMP && tlast012 < g_STOPTM && len <= end; 
	 len = len + inc, nq++)
    {
	if (nq > 2)
	    inc = (nq % 2) ? inc + inc : inc;
	
	/* This is a perturbation loop to test nearby values */
	for (ipert = 0, pert = (inc > PERT + 1) ? -PERT : 0;
	     pert <= PERT; 
	     ipert++, n++, pert += (inc > PERT + 1) ? PERT : PERT + 1)
	{


	    /*****************************************************/
	    /*         Run a trial between rank 0 and 1          */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    if (g_nIproc == 2)
		goto skip_01_trial;

	    /* Calculate howmany times to repeat the experiment. */
	    if (args01.tr)
	    {
		if (args01.bufflen == 0)
		    nrepeat = args01.latency_reps;
		else
		    nrepeat = (int)(MAX((RUNTM / ((double)args01.bufflen /
			           (args01.bufflen - inc + 1.0) * tlast01)), TRIALS));
		SendReps(&args01, &nrepeat);
	    }
	    else
	    {
		RecvReps(&args01, &nrepeat);
	    }

	    /* Allocate the buffer */
	    args01.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args01.bufflen * nrepeat + bufalign); */
	    if (bNoCache)
	    {
		if ((args01.buff = (char *)malloc(args01.bufflen * nrepeat + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args01.buff = (char *)malloc(args01.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args01.buff1 = (char *)malloc(args01.bufflen * nrepeat + bufalign)) == (char *)NULL) */
	    if ((args01.buff1 = (char *)malloc(args01.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args01.buff;
	    memtmp1 = args01.buff1;

	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args01.buff += (bufalign - (POINTER_TO_INT(args01.buff) % bufalign) + bufoffset) % bufalign;
		    /* args01.buff1 += (bufalign - ((int)args01.buff1 % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args01.buff1 += (bufalign - (POINTER_TO_INT(args01.buff1) % bufalign) + bufoffset) % bufalign;
	    
	    if (args01.tr && printopt)
	    {
		fprintf(stdout,"%3d: %9d bytes %4d times --> ",
		    n, args01.bufflen, nrepeat);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args01.tr)
	    {
		bwdata01[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.buff = memtmp + ((bufalign - (POINTER_TO_INT(args01.buff) % bufalign) + bufoffset) % bufalign);
			    /* args01.buff1 = memtmp1 + ((bufalign - ((int)args01.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args01.buff = memtmp;
			    /* args01.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args01);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			SendData(args01);
			RecvData(args01);
			if (bNoCache)
			{
			    args01.buff += args01.bufflen;
			    /* args01.buff1 += args01.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);

		    t2 += t*t;
		    t1 += t;
		    bwdata01[n].t = MIN(bwdata01[n].t, t);
		}
		SendTime(&args01, &bwdata01[n].t);
	    }
	    else
	    {
		bwdata01[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.buff = memtmp + ((bufalign - (POINTER_TO_INT(args01.buff) % bufalign) + bufoffset) % bufalign);
			    /* args01.buff1 = memtmp1 + ((bufalign - ((int)args01.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args01.buff = memtmp;
			    /* args01.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args01);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			RecvData(args01);
			SendData(args01);
			if (bNoCache)
			{
			    args01.buff += args01.bufflen;
			    /* args01.buff1 += args01.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);
		}
		RecvTime(&args01, &bwdata01[n].t);
	    }
	    tlast01 = bwdata01[n].t;
	    bwdata01[n].bits = args01.bufflen * CHARSIZE;
	    bwdata01[n].bps = bwdata01[n].bits / (bwdata01[n].t * 1024 * 1024);
	    bwdata01[n].repeat = nrepeat;
	    
	    if (args01.tr)
	    {
		if (bSavePert)
		{
		    if (bUseMegaBytes)
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata01[n].bits / 8, bwdata01[n].bps / 8, bwdata01[n].t);
		    else
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata01[n].bits / 8, bwdata01[n].bps, bwdata01[n].t);
		    fflush(out);
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (args01.tr && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata01[n].bps / 8, tlast01);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata01[n].bps, tlast01);
		fflush(stdout);
	    }

skip_01_trial:


	    /*****************************************************/
	    /*         Run a trial between rank 1 and 2          */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    if (g_nIproc == 0)
		goto skip_12_trial;

	    /* Calculate howmany times to repeat the experiment. */
	    if (args12.tr)
	    {
		if (args12.bufflen == 0)
		    nrepeat = args12.latency_reps;
		else
		    nrepeat = (int)(MAX((RUNTM / ((double)args12.bufflen /
			           (args12.bufflen - inc + 1.0) * tlast12)), TRIALS));
		SendReps(&args12, &nrepeat);
	    }
	    else
	    {
		RecvReps(&args12, &nrepeat);
	    }
	    
	    /* Allocate the buffer */
	    args12.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args12.bufflen * nrepeat + bufalign); */
	    if (bNoCache)
	    {
		if ((args12.buff = (char *)malloc(args12.bufflen * nrepeat + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args12.buff = (char *)malloc(args12.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args12.buff1 = (char *)malloc(args12.bufflen * nrepeat + bufalign)) == (char *)NULL) */
	    if ((args12.buff1 = (char *)malloc(args12.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args12.buff;
	    memtmp1 = args12.buff1;
	    
	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args12.buff += (bufalign - (POINTER_TO_INT(args12.buff) % bufalign) + bufoffset) % bufalign;
		    /* args12.buff1 += (bufalign - ((int)args12.buff1 % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args12.buff1 += (bufalign - (POINTER_TO_INT(args12.buff1) % bufalign) + bufoffset) % bufalign;
	    
	    if (args12.tr && printopt)
	    {
		printf("%3d: %9d bytes %4d times --> ", n, args12.bufflen, nrepeat);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args12.tr)
	    {
		bwdata12[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args12.buff = memtmp + ((bufalign - (POINTER_TO_INT(args12.buff) % bufalign) + bufoffset) % bufalign);
			    /* args12.buff1 = memtmp1 + ((bufalign - ((int)args12.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args12.buff = memtmp;
			    /* args12.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args12);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			SendData(args12);
			RecvData(args12);
			if (bNoCache)
			{
			    args12.buff += args12.bufflen;
			    /* args12.buff1 += args12.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);

		    t2 += t*t;
		    t1 += t;
		    bwdata12[n].t = MIN(bwdata12[n].t, t);
		}
		SendTime(&args12, &bwdata12[n].t);
	    }
	    else
	    {
		bwdata12[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args12.buff = memtmp + ((bufalign - (POINTER_TO_INT(args12.buff) % bufalign) + bufoffset) % bufalign);
			    /* args12.buff1 = memtmp1 + ((bufalign - ((int)args12.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args12.buff = memtmp;
			    /* args12.buff1 = memtmp1; */
			}
		    }
		    
		    Sync(&args12);
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			RecvData(args12);
			SendData(args12);
			if (bNoCache)
			{
			    args12.buff += args12.bufflen;
			    /* args12.buff1 += args12.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);
		}
		RecvTime(&args12, &bwdata12[n].t);
	    }
	    tlast12 = bwdata12[n].t;
	    bwdata12[n].bits = args12.bufflen * CHARSIZE;
	    bwdata12[n].bps = bwdata12[n].bits / (bwdata12[n].t * 1024 * 1024);
	    bwdata12[n].repeat = nrepeat;

	    /*
	    if (args12.tr)
	    {
		if (bSavePert)
		{
		    if (bUseMegaBytes)
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata12[n].bits / 8, bwdata12[n].bps / 8, bwdata12[n].t);
		    else
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata12[n].bits / 8, bwdata12[n].bps, bwdata12[n].t);
		    fflush(out);
		}
	    }
	    */
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (args12.tr && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata12[n].bps / 8, tlast12);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata12[n].bps, tlast12);
		fflush(stdout);
	    }

skip_12_trial:


	    /*****************************************************/
	    /*         Run a trial between rank 0, 1 and 2       */
	    /*****************************************************/

	    MPI_Barrier(MPI_COMM_WORLD);


	    /* Calculate howmany times to repeat the experiment. */
	    if (g_nIproc == 0)
	    {
		if (args012.bufflen == 0)
		    nrepeat = g_latency012_reps;
		else
		    nrepeat = (int)(MAX((RUNTM / ((double)args012.bufflen /
			           (args012.bufflen - inc + 1.0) * tlast012)), TRIALS));
		MPI_Bcast(&nrepeat, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }
	    else
	    {
		MPI_Bcast(&nrepeat, 1, MPI_INT, 0, MPI_COMM_WORLD);
	    }

	    /* Allocate the buffer */
	    args012.bufflen = len + pert;
	    /* printf("allocating %d bytes\n", args12.bufflen * nrepeat + bufalign); */
	    if (bNoCache)
	    {
		if ((args012.buff = (char *)malloc(args012.bufflen * nrepeat + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    else
	    {
		if ((args012.buff = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
		{
		    fprintf(stdout,"Couldn't allocate memory\n");
		    fflush(stdout);
		    break;
		}
	    }
	    /* if ((args012.buff1 = (char *)malloc(args012.bufflen * nrepeat + bufalign)) == (char *)NULL) */
	    if ((args012.buff1 = (char *)malloc(args012.bufflen + bufalign)) == (char *)NULL)
	    {
		fprintf(stdout,"Couldn't allocate memory\n");
		fflush(stdout);
		break;
	    }

	    /* save the original pointers in case alignment moves them */
	    memtmp = args012.buff;
	    memtmp1 = args012.buff1;
	    
	    /* Possibly align the data buffer */
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args012.buff += (bufalign - (POINTER_TO_INT(args012.buff) % bufalign) + bufoffset) % bufalign;
		    /* args12.buff1 += (bufalign - ((int)args12.buff1 % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args012.buff1 += (bufalign - (POINTER_TO_INT(args012.buff1) % bufalign) + bufoffset) % bufalign;
	    
	    if (g_nIproc == 0 && printopt)
	    {
		printf("%3d: %9d bytes %4d times --> ", n, args012.bufflen, nrepeat);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    switch (g_nIproc)
	    {
	    case 0:
		bwdata012[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.buff = memtmp + ((bufalign - (POINTER_TO_INT(args012.buff) % bufalign) + bufoffset) % bufalign);
			    /* args012.buff1 = memtmp1 + ((bufalign - ((int)args012.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.buff = memtmp;
			    /* args012.buff1 = memtmp1; */
			}
		    }
		    
		    Sync012();
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			MPI_Send(args012.buff, args012.bufflen, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
			MPI_Recv(args012.buff1, args012.bufflen, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
			if (bNoCache)
			{
			    args012.buff += args012.bufflen;
			    /* args012.buff1 += args012.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);

		    t2 += t*t;
		    t1 += t;
		    bwdata012[n].t = MIN(bwdata012[n].t, t);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		/*SendTime(&args012, &bwdata012[n].t);*/
		break;
	    case 1:
		bwdata012[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.buff = memtmp + ((bufalign - (POINTER_TO_INT(args012.buff) % bufalign) + bufoffset) % bufalign);
			    /* args012.buff1 = memtmp1 + ((bufalign - ((int)args012.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.buff = memtmp;
			    /* args012.buff1 = memtmp1; */
			}
		    }
		    
		    Sync012();
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			MPI_Recv(args012.buff1, args012.bufflen, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
			MPI_Send(args012.buff, args012.bufflen, MPI_BYTE, 2, 1, MPI_COMM_WORLD);
			MPI_Recv(args012.buff1, args012.bufflen, MPI_BYTE, 2, 1, MPI_COMM_WORLD, &status);
			MPI_Send(args012.buff, args012.bufflen, MPI_BYTE, 0, 1, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args012.buff += args012.bufflen;
			    /* args012.buff1 += args012.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		/*RecvTime(&args012, &bwdata012[n].t);*/
		break;
	    case 2:
		bwdata012[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args012.buff = memtmp + ((bufalign - (POINTER_TO_INT(args012.buff) % bufalign) + bufoffset) % bufalign);
			    /* args012.buff1 = memtmp1 + ((bufalign - ((int)args012.buff1 % bufalign) + bufoffset) % bufalign); */
			}
			else
			{
			    args012.buff = memtmp;
			    /* args012.buff1 = memtmp1; */
			}
		    }
		    
		    Sync012();
		    t0 = When();
		    for (j = 0; j < nrepeat; j++)
		    {
			MPI_Recv(args012.buff1, args012.bufflen, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
			MPI_Send(args012.buff, args012.bufflen, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
			if (bNoCache)
			{
			    args012.buff += args012.bufflen;
			    /* args012.buff1 += args012.bufflen; */
			}
		    }
		    t = (When() - t0)/(2 * nrepeat);
		}
		MPI_Bcast(&bwdata012[n].t, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		/*RecvTime(&args012, &bwdata012[n].t);*/
		break;
	    }
	    tlast012 = bwdata012[n].t;
	    bwdata012[n].bits = args012.bufflen * CHARSIZE;
	    bwdata012[n].bps = bwdata012[n].bits / (bwdata012[n].t * 1024 * 1024);
	    bwdata012[n].repeat = nrepeat;

	    /*
	    if (g_nIproc == 0)
	    {
		if (bSavePert)
		{
		    if (bUseMegaBytes)
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata012[n].bits / 8, bwdata012[n].bps / 8, bwdata012[n].t);
		    else
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata012[n].bits / 8, bwdata012[n].bps, bwdata012[n].t);
		    fflush(out);
		}
	    }
	    */
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (g_nIproc == 0 && printopt)
	    {
		if (bUseMegaBytes)
		    printf(" %6.2f MBps in %0.9f sec\n", bwdata012[n].bps / 8, tlast012);
		else
		    printf(" %6.2f Mbps in %0.9f sec\n", bwdata012[n].bps, tlast012);
		fflush(stdout);
	    }
	} /* End of perturbation loop */

	if (!bSavePert && g_nIproc == 0)
	{
	    /* if we didn't save all of the perturbation loops, find the max and save it */
	    int index = 1;
	    double dmax = bwdata01[n-1].bps;
	    for (; ipert > 1; ipert--)
	    {
		if (bwdata01[n-ipert].bps > dmax)
		{
		    index = ipert;
		    dmax = bwdata01[n-ipert].bps;
		}
	    }
	    if (bUseMegaBytes)
		fprintf(out, "%d\t%f\t%0.9f\n", bwdata01[n-index].bits / 8, bwdata01[n-index].bps / 8, bwdata01[n-index].t);
	    else
		fprintf(out, "%d\t%f\t%0.9f\n", bwdata01[n-index].bits / 8, bwdata01[n-index].bps, bwdata01[n-index].t);
	    fflush(out);
	}
#if 0
	if (!bSavePert && args12.tr)
	{
	    /* if we didn't save all of the perturbation loops, find the max and save it */
	    int index = 1;
	    double dmax = bwdata12[n-1].bps;
	    for (; ipert > 1; ipert--)
	    {
		if (bwdata12[n-ipert].bps > dmax)
		{
		    index = ipert;
		    dmax = bwdata12[n-ipert].bps;
		}
	    }
	    if (bUseMegaBytes)
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata12[n-index].bits / 8, bwdata12[n-index].bps / 8, bwdata12[n-index].t);
	    else
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata12[n-index].bits / 8, bwdata12[n-index].bps, bwdata12[n-index].t);
	    fflush(out);
	}
#endif
    } /* End of main loop  */
	
    if (g_nIproc == 0)
	fclose(out);
    /* THE_END:		 */
    MPI_Finalize();
    free(bwdata01);
    free(bwdata12);
    return 0;
}

int Setup(ArgStruct *p01, ArgStruct *p12)
{
    char s[255];
    int len = 255;
    
    p01->iproc = p12->iproc = g_nIproc;
    
    MPI_Get_processor_name(s, &len);
    /*gethostname(s, len);*/
    printf("%d: %s\n", p01->iproc, s);
    fflush(stdout);

    switch (g_nIproc)
    {
    case 0:
	p01->nbor = 1;
	p01->tr = TRUE;
	p12->nbor = -1;
	p12->tr = FALSE;
	break;
    case 1:
	p01->nbor = 0;
	p01->tr = FALSE;
	p12->nbor = 2;
	p12->tr = TRUE;
	break;
    case 2:
	p01->nbor = -1;
	p01->tr = FALSE;
	p12->nbor = 1;
	p12->tr = FALSE;
	break;
    default:
	MPI_Abort(MPI_COMM_WORLD, 0);
	break;
    }

    return 1;	
}	

void Sync(ArgStruct *p)
{
    MPI_Status status;
    if (p->tr)
    {
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
    }
    else
    {
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
    }
}

void Sync012()
{
    MPI_Status status;
    switch (g_nIproc)
    {
    case 0:
	MPI_Send(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	break;
    case 1:
	MPI_Recv(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, 2, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, 2, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, 2, 1, MPI_COMM_WORLD);
	break;
    case 2:
	MPI_Recv(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
	MPI_Send(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	MPI_Recv(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
	break;
    }
}

int DetermineLatencyReps(ArgStruct *p)
{
    MPI_Status status;
    double t0, duration = 0;
    int reps = 1, prev_reps = 0;
    int i;

    /* prime the send/receive pipes */
    Sync(p);
    Sync(p);
    Sync(p);

    /* test how long it takes to send n messages 
     * where n = 1, 2, 4, 8, 16, 32, ...
     */
    t0 = When();
    t0 = When();
    t0 = When();
    while ( (duration < 1) ||
	    (duration < 3 && reps < 1000))
    {
	t0 = When();
	for (i=0; i<reps-prev_reps; i++)
	{
	    Sync(p);
	}
	duration += When() - t0;
	prev_reps = reps;
	reps = reps * 2;

	/* use duration from the root only */
	if (p->tr)
	    MPI_Send(&duration, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD);
	else
	    MPI_Recv(&duration, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD, &status);
    }

    return reps;
}

int DetermineLatencyReps012()
{
    double t0, duration = 0;
    int reps = 1, prev_reps = 0;
    int i;

    /* prime the send/receive pipes */
    Sync012();
    Sync012();
    Sync012();

    /* test how long it takes to send n messages 
     * where n = 1, 2, 4, 8, 16, 32, ...
     */
    t0 = When();
    t0 = When();
    t0 = When();
    while ( (duration < 1) ||
	    (duration < 3 && reps < 1000))
    {
	t0 = When();
	for (i=0; i<reps-prev_reps; i++)
	{
	    Sync012();
	}
	duration += When() - t0;
	prev_reps = reps;
	reps = reps * 2;

	/* use duration from the root only */
	MPI_Bcast(&duration, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    }

    return reps;
}

double TestLatency(ArgStruct *p)
{
    double latency, t0;
    int i;
    MPI_Status status;

    /* calculate the latency between rank 0 and rank 1 */
    p->latency_reps = DetermineLatencyReps(p);
    if (/*p->latency_reps < 1024 &&*/ p->tr)
    {
	printf("To determine %s latency, using %d reps\n", p->iproc == 0 ? "0 -> 1     " : "     1 -> 2", p->latency_reps);
	fflush(stdout);
    }

    p->bufflen = 0;
    p->buff = NULL;
    p->buff1 = NULL;
    Sync(p);
    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    for (i = 0; i < p->latency_reps; i++)
    {
	if (p->tr)
	{
	    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	}
	else
	{
	    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
	    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
	}
    }
    latency = (When() - t0)/(2 * p->latency_reps);

    return latency;
}

double TestLatency012()
{
    double latency, t0;
    int i;
    MPI_Status status;

    /* calculate the latency between rank 0 and rank 1 */
    g_latency012_reps = DetermineLatencyReps012();
    if (/*g_latency012_reps < 1024 &&*/ g_nIproc == 0)
    {
	printf("To determine 0 -> 1 -> 2 latency, using %d reps\n", g_latency012_reps);
	fflush(stdout);
    }

    Sync012();
    t0 = When();
    t0 = When();
    t0 = When();
    t0 = When();
    for (i = 0; i < g_latency012_reps; i++)
    {
	switch (g_nIproc)
	{
	case 0:
	    MPI_Send(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	    MPI_Recv(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
	    break;
	case 1:
	    MPI_Recv(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &status);
	    MPI_Send(NULL, 0, MPI_BYTE, 2, 1, MPI_COMM_WORLD);
	    MPI_Recv(NULL, 0, MPI_BYTE, 2, 1, MPI_COMM_WORLD, &status);
	    MPI_Send(NULL, 0, MPI_BYTE, 0, 1, MPI_COMM_WORLD);
	    break;
	case 2:
	    MPI_Recv(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD, &status);
	    MPI_Send(NULL, 0, MPI_BYTE, 1, 1, MPI_COMM_WORLD);
	    break;
	}
    }
    latency = (When() - t0)/(2 * g_latency012_reps);

    return latency;
}

void SendRecvData(ArgStruct *p)
{
    MPI_Status status;
    
    /*MPI_Sendrecv(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);*/
    
    /*
    MPI_Request request;
    MPI_Irecv(p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &request);
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
    MPI_Wait(&request, &status);
    */
    
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
}

void SendTime(ArgStruct *p, double *t)
{
    MPI_Send(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD);
}

void RecvTime(ArgStruct *p, double *t)
{
    MPI_Status status;
    MPI_Recv(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD, &status);
}

void SendReps(ArgStruct *p, int *rpt)
{
    MPI_Send(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD);
}

void RecvReps(ArgStruct *p, int *rpt)
{
    MPI_Status status;
    MPI_Recv(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD, &status);
}
