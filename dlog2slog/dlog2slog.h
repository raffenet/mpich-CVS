#ifndef _DLOG2SLOG
#define _DLOG2SLOG

#include "dlogimpl.h" 
#include "dlog.h"

/* dlog2slog structure */

/**** state_info
      A structure defined as a node in a list of DLOG-type state definitions.
      Contains all relevant information to convert a DLOG-type state into
      an SLOG-type interval.
      This list remains constant after all states definitions have been 
      initialized when the first pass through the dlog file is made.
****/
struct state_info
{
    int state_id;
    int start_event_num;
    int end_event_num;
    char color[DLOG_COLOR_LENGTH];
    char description[DLOG_DESCRIPTION_LENGTH];
    struct state_info *next;
};

/**** list_elemnt
      A structure defined as a node in a list of DLOG-type events. Only the
      start events of states are added to this list and whenever a matching 
      end event is found that start event is removed from the list and an 
      slog interval logged. The list keeps growing and shrinking - its maximum
      size is the total number of processes in the logged parallel program 
      if there were no threads in the program. 
****/
struct list_elemnt
{
    int state_id;
    int data;
    int process_id;
    int rectype;
    double start_time;
    struct list_elemnt *next;
};

/* dlog2slog constants */
#define MSG_STATE      9999     /* for state_info list - not for SLOG */
#define MSG_RECORD     SLOG_RECTYPE_STATIC_OFFDIAG
#define NON_MSG_RECORD SLOG_RECTYPE_STATIC_DIAG
#define SLOG_PREVIEW_NAME "/dev/null"
#define D2S_ERROR      0
#define D2S_SUCCESS    1

#define CLASS_TYPE     "state"
#define FORWARD_MSG    10001
#define BACKWARD_MSG   10002
#define FORWARD_MSG_CLASSTYPE  "message"
#define BACKWARD_MSG_CLASSTYPE "message"
#define FORWARD_MSG_LABEL      "forward arrow"
#define BACKWARD_MSG_LABEL     "backward arrow"
#define FORWARD_MSG_COLOR      "white"
#define BACKWARD_MSG_COLOR     "grey"

#define EXTRA_STATES   40 
#define D2S_NUM_FRAMES     0
#define D2S_FRAME_BYTE_SIZE 64

/* dlog2slog prototyping begins */ 

/** initializations **/

int  DLOG_init_state_defs ( char * );
int  DLOG_init_all_mpi_state_defs( void );
int  init_SLOG ( long, long, char * );
int  init_SLOG_TTAB ( void );
int  init_SLOG_PROF_RECDEF ( void );
int  init_dlog2slog ( char*, char* );
void DLOG_init_essential_values ( long, int );

/** actual logging */

void DLOG_freeStateInfo(void);
int  DLOG_makeSLOG ( char * ); 
void DLOG_free_resources ( void );
void DLOG_printHelp ( void );
void DLOG_printStateInfo ( void );

#endif









