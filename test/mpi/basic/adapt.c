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

#define MIN(x, y)	(((x) < (y))?(x):(y))
#define MAX(x, y)	(((x) > (y))?(x):(y))

#define When MPI_Wtime

int g_nIproc = 0, g_nNproc = 0;

typedef struct argstruct ArgStruct;
struct argstruct 
{
    /* This is the common information that is needed for all tests		*/
    char    *buff;	/* Transmitted buffer					*/
    char    *buff1;	/* Transmitted buffer					*/
    int     bufflen;	/* Length of transmitted buffer 			*/
	    /*tr;*/ 	/* Transmit flag 					*/
    int nbor, iproc, tr, latency_reps;
    /*ProtocolStruct prot1, prot2;*/
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
void SendData(ArgStruct *p);
void RecvData(ArgStruct *p);
void SendRecvData(ArgStruct *p);
void SendTime(ArgStruct *p, double *t, int *rpt);
void RecvTime(ArgStruct *p, double *t, int *rpt);
double TestLatency(ArgStruct *p);
void PrintOptions(void);
int DetermineLatencyReps(ArgStruct *p);

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
    printf("       -headtohead\n");
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
    
    int i, j, n, nq,		/* Loop indices					*/
	bufoffset = 0,		/* Align buffer to this				*/
	bufalign = 16*1024,	/* Boundary to align buffer to			*/
	nrepeat,		/* Number of time to do the transmission	*/
	nzero = 0,
	len,			/* Number of bytes to be transmitted		*/
	inc = 1,		/* Increment value				*/
	detailflag = 0,		/* Set to examine the signature curve detail*/
	pert,			/* Perturbation value				*/
        ipert,                  /* index of the perturbation loop		*/
	start = 0,		/* Starting value for signature curve 		*/
	end = MAXINT,		/* Ending value for signature curve		*/
	printopt = 1;		/* Debug print statements flag			*/
    
    ArgStruct	args01, args12;		/* Argumentsfor all the calls			*/
    
    double t, t0, t1, t2,	/* Time variables				*/
	tlast,			/* Time for the last transmission		*/
	tzero = 0,
	latency01, latency12;		/* Network message latency			*/
    
    Data *bwdata;		/* Bandwidth curve data 			*/
    
    BOOL bNoCache = FALSE;
    BOOL bHeadToHead = FALSE;
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
    bHeadToHead = GetOpt(&argc, &argv, "-headtohead");
    bUseMegaBytes = GetOpt(&argc, &argv, "-mb");
    if (GetOpt(&argc, &argv, "-noprint"))
	printopt = 0;
    bSavePert = GetOpt(&argc, &argv, "-pert");
    
    bwdata = malloc((g_NSAMP+1) * sizeof(Data));

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

    switch (g_nIproc)
    {
    case 0:
	latency01 = TestLatency(&args01);
	/*printf("[0] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	RecvTime(&args01, &latency12, &nzero);
	/*printf("[0] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    case 1:
	latency01 = TestLatency(&args01);
	/*printf("[1] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	SendTime(&args12, &latency01, &nzero);
	latency12 = TestLatency(&args12);
	/*printf("[1] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	SendTime(&args01, &latency12, &nzero);
	break;
    case 2:
	RecvTime(&args12, &latency01, &nzero);
	/*printf("[2] latency01 = %0.9f\n", latency01);fflush(stdout);*/
	latency12 = TestLatency(&args12);
	/*printf("[2] latency12 = %0.9f\n", latency12);fflush(stdout);*/
	break;
    }

    if ((g_nIproc == 0) && printopt)
    {
	printf("Latency01: %0.9f\n", latency01);
	printf("Latency12: %0.9f\n", latency12);
	fflush(stdout);
	printf("Now starting main loop\n");
	fflush(stdout);
    }
    tlast = latency01;
    inc = (start > 1 && !detailflag) ? start/2: inc;
    args01.bufflen = start;

    if (g_nIproc == 2)
    {
	MPI_Finalize();
	exit(0);
    }
    /* Main loop of benchmark */
    for (nq = n = 0, len = start; 
         n < g_NSAMP && tlast < g_STOPTM && len <= end; 
	 len = len + inc, nq++)
    {
	if (nq > 2 && !detailflag)
	    inc = ((nq % 2))? inc + inc: inc;
	
	/* This is a perturbation loop to test nearby values */
	for (ipert = 0, pert = (!detailflag && inc > PERT + 1)? -PERT: 0;
	     pert <= PERT; 
	     ipert++, n++, pert += (!detailflag && inc > PERT + 1)? PERT: PERT + 1)
	{
	    
	    /* Calculate howmany times to repeat the experiment. */
	    if (args01.tr)
	    {
		if (args01.bufflen == 0)
		    nrepeat = args01.latency_reps;
		else
		    nrepeat = (int)(MAX((RUNTM / ((double)args01.bufflen /
			           (args01.bufflen - inc + 1.0) * tlast)), TRIALS));
		SendTime(&args01, &tzero, &nrepeat);
	    }
	    else
	    {
		nrepeat = 1; /* Just needs to be greater than zero */
		RecvTime(&args01, &tzero, &nrepeat);
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
	    /* Possibly align the data buffer */
	    memtmp = args01.buff;
	    memtmp1 = args01.buff1;
	    
	    if (!bNoCache)
	    {
		if (bufalign != 0)
		{
		    args01.buff += (bufalign - ((int)args01.buff % bufalign) + bufoffset) % bufalign;
		    /* args01.buff1 += (bufalign - ((int)args01.buff1 % bufalign) + bufoffset) % bufalign; */
		}
	    }
	    args01.buff1 += (bufalign - ((int)args01.buff1 % bufalign) + bufoffset) % bufalign;
	    
	    if (args01.tr && printopt)
	    {
		fprintf(stdout,"%3d: %9d bytes %4d times --> ",
		    n, args01.bufflen, nrepeat);
		fflush(stdout);
	    }
	    
	    /* Finally, we get to transmit or receive and time */
	    if (args01.tr)
	    {
		bwdata[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.buff = memtmp + ((bufalign - ((int)args01.buff % bufalign) + bufoffset) % bufalign);
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
			if (bHeadToHead)
			    SendRecvData(&args01);
			else
			{
			    SendData(&args01);
			    RecvData(&args01);
			}
			if (bNoCache)
			{
			    args01.buff += args01.bufflen;
			    /* args01.buff1 += args01.bufflen; */
			}
		    }
		    t = (When() - t0)/((1 + !0) * nrepeat);

		    t2 += t*t;
		    t1 += t;
		    bwdata[n].t = MIN(bwdata[n].t, t);
		}
		SendTime(&args01, &bwdata[n].t, &nzero);
	    }
	    else
	    {
		bwdata[n].t = LONGTIME;
		t2 = t1 = 0;
		for (i = 0; i < TRIALS; i++)
		{
		    if (bNoCache)
		    {
			if (bufalign != 0)
			{
			    args01.buff = memtmp + ((bufalign - ((int)args01.buff % bufalign) + bufoffset) % bufalign);
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
			if (bHeadToHead)
			    SendRecvData(&args01);
			else
			{
			    RecvData(&args01);
			    SendData(&args01);
			}
			if (bNoCache)
			{
			    args01.buff += args01.bufflen;
			    /* args01.buff1 += args01.bufflen; */
			}
		    }
		    t = (When() - t0)/((1 + !0) * nrepeat);
		}
		RecvTime(&args01, &bwdata[n].t, &nzero);
	    }
	    tlast = bwdata[n].t;
	    bwdata[n].bits = args01.bufflen * CHARSIZE;
	    bwdata[n].bps = bwdata[n].bits / (bwdata[n].t * 1024 * 1024);
	    bwdata[n].repeat = nrepeat;
	    
	    if (args01.tr)
	    {
		if (bSavePert)
		{
		    if (bUseMegaBytes)
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n].bits / 8, bwdata[n].bps / 8, bwdata[n].t);
		    else
			fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n].bits / 8, bwdata[n].bps, bwdata[n].t);
		    fflush(out);
		}
	    }
	    
	    free(memtmp);
	    free(memtmp1);
	    
	    if (args01.tr && printopt)
	    {
		if (bUseMegaBytes)
		    fprintf(stdout," %6.2f MBps in %0.9f sec\n", bwdata[n].bps / 8, tlast);
		else
		    fprintf(stdout," %6.2f Mbps in %0.9f sec\n", bwdata[n].bps, tlast);
		fflush(stdout);
	    }
	} /* End of perturbation loop */	
	if (!bSavePert && args01.tr)
	{
	    /* if we didn't save all of the perturbation loops, find the max and save it */
	    int index = 1;
	    double dmax = bwdata[n-1].bps;
	    for (; ipert > 1; ipert--)
	    {
		if (bwdata[n-ipert].bps > dmax)
		{
		    index = ipert;
		    dmax = bwdata[n-ipert].bps;
		}
	    }
	    if (bUseMegaBytes)
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n-index].bits / 8, bwdata[n-index].bps / 8, bwdata[n-index].t);
	    else
		fprintf(out,"%d\t%f\t%0.9f\n", bwdata[n-index].bits / 8, bwdata[n-index].bps, bwdata[n-index].t);
	    fflush(out);
	}
    } /* End of main loop  */
	
    if (args01.tr)
	fclose(out);
    /* THE_END:		 */
    MPI_Finalize();
    free(bwdata);
    return 0;
}


/* Return the current time in seconds, using a double precision number. 	 */
/*
double When()
{
    return MPI_Wtime();
}
*/

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

double TestLatency(ArgStruct *p)
{
    double latency, t0;
    int i;
    MPI_Status status;

    /* calculate the latency between rank 0 and rank 1 */
    p->latency_reps = DetermineLatencyReps(p);
    if (p->latency_reps < 1024 && p->tr)
    {
	printf("Using %d reps to determine latency\n", p->latency_reps);
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

void SendData(ArgStruct *p)
{
    MPI_Send(p->buff, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD);
}

void RecvData(ArgStruct *p)
{
    MPI_Status status;
    MPI_Recv(p->buff1, p->bufflen, MPI_BYTE, p->nbor, 1, MPI_COMM_WORLD, &status);
}


void SendTime(ArgStruct *p, double *t, int *rpt)
{
    if (*rpt > 0)
	MPI_Send(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD);
    else
	MPI_Send(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD);
}

void RecvTime(ArgStruct *p, double *t, int *rpt)
{
    MPI_Status status;
    if (*rpt > 0)
	MPI_Recv(rpt, 1, MPI_INT, p->nbor, 2, MPI_COMM_WORLD, &status);
    else
	MPI_Recv(t, 1, MPI_DOUBLE, p->nbor, 2, MPI_COMM_WORLD, &status);
}
