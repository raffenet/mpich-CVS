/******************** dlog2slog.c ****************************/
/*
    This program converts a dlog file generated using 
    DLOG Logging calls into an slog file.
*/

/*
a dlog file format:
 a record consists of a header which contains the timestamp, record type and
 process id.
 the headers are the same for all record types but the records themselves are
 different.
  DLOG_STATEDEF,
  DLOG_RAWEVENT,
  DLOG_COMMEVENT.
*/

#ifdef HAVE_SLOG_WINCONFIG_H
#include "slog_winconfig.h"
#else
#include "slog_config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dlog2slog.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINDOWS_H
#include "getopt.h"
#endif

void PrintHelp(void)
{
    fprintf(stdout,"Usage : dlog2slog [ -d=FrameNum ] [ -f=FrameSize ]"
	    " [ -h ] file.dlog\n"
	    "        where file.dlog is a dlog file\n"
	    "Options:\n"
	    "\td : \"FrameNum\" specifies the number of frames per"
	    " directory\n"
	    "\tf : \"FrameSize\" specifies the size of a frame in Kilobytes\n"
	    "\th : help menu\n\n"
            "Due to the limitations of the current implementation of SLOG-API\n"
            "If the default or supplied frame size is too small, it may cause\n"
            "problems in generation of the SLOG file.  If one encounters\n"
            "some strange errors in using dlog2slog, like complaints about\n"
            "frame has been filled up or the maximin allowable number of\n"
            "frames has been reached, try to set the frame size bigger.\n"
            "e.g.  dlog2slog -f=NewFrameSizeInKiloByte filename.dlog\n"
            "If this does NOT work when your frame size reaches 4MB,\n"
            "try set the maximum number of frames to a bigger number than the\n"
            "guess shown in the error message from the previous run of\n" 
            "dlog2slog.  e.g. dlog2slog -d=NewFrameNumber filename.dlog\n"
            "The default frame size is 64 KB.\n");
    fflush(stdout);
}

int main (int argc, char **argv) 
{
    long frame_size = D2S_FRAME_BYTE_SIZE,   /* default frame kilo-byte size.*/
         num_frames = 0;    /* stores the no of frames per directory in slog.*/

    int num_args = 0;       /* number of options specified on command line.  */
    
    char dlog_file[1024];
    char slog_file[1024];
    char *optstring = "d:f:h";
    char optchar;
    
    optchar = getopt(argc, argv, optstring);
    while (strchr(optstring, optchar) != NULL) 
    {
	if (num_args <= 2)
	{
	    switch (optchar) 
	    {
	    case 'd':
		if ((optarg != NULL) && (*optarg == '=')) 
		{
		    num_frames = atol(optarg+1);
		}
		num_args++;
		break;
	    case 'f':
		if ((optarg != NULL) && (*optarg == '=')) 
		{
		    frame_size = atol(optarg+1);
		}
		num_args++;
		break;
	    default:
		PrintHelp();
		exit(0);
	    }
	}
	else {
	    PrintHelp();
	    exit(0);
	}
	optchar = getopt(argc, argv, optstring);
    }

    if ((argc-num_args) > 1)
    {
	strcpy(dlog_file, argv[optind]);
    }
    else 
    {
	fprintf(stderr, "No dlog file specified.\n");
	PrintHelp();
	exit(1);
    }

    /* initialize stuff */
    if (init_dlog2slog(dlog_file, slog_file) == D2S_ERROR)
	exit(1);

    /* get the state definitions from the file */
    if (DLOG_init_state_defs(dlog_file) == D2S_ERROR)
    {
	DLOG_freeStateInfo();
	exit(1);
    }

    /* initialize slog tables */
    if (init_SLOG(num_frames, frame_size, slog_file) == D2S_ERROR) 
	exit(1);
    
    /*
    making second pass of dlog file to log slog intervals using dlog events.
    the function used here is DLOG_makeSLOG()
    */
    if (DLOG_makeSLOG(dlog_file) == D2S_ERROR)
	    exit(1);

    DLOG_free_resources();
    return 0;
}

