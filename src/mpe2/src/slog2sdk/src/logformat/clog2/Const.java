/*
 *  (C) 2001 by Argonne National Laboratory
 *      See COPYRIGHT in top-level directory.
 */

/*
 *  @author  Anthony Chan
 */

package logformat.clog2;

public class Const
{
           static final byte    INVALID_byte   = Byte.MIN_VALUE;
           static final short   INVALID_short  = Short.MIN_VALUE;
           static final int     INVALID_int    = Integer.MIN_VALUE;
           static final long    INVALID_long   = Long.MIN_VALUE;
           static final int     NULL_int       = 0;
           static final int     NULL_iaddr     = 0;
           static final long    NULL_fptr      = 0;
           static final float   INVALID_float  = Float.MIN_VALUE;
           static final double  INVALID_double = Double.MIN_VALUE;
           static final int     TRUE           = 1;
           static final int     FALSE          = 0;

    //  Define constants listed in <MPE>/include/clog.h

           static final int     BLOCK_SIZE = 65536;     // CLOG_BLOCK_SIZE

    //  Define constants listed in <MPE>/src/log_wrap.c

    //  MPE_MAX_STATES is the MPE's max. MPI system states.
    //  Let's be generous, instead of defining MPE_MAX_STATES = 128,
    //  define MPE_MAX_STATES = 180 to include MPI-IO routines
    //  define MPE_MAX_STATES = 200 to include MPI-RMA routines

           static final int     MPE_MAX_STATES       = 200;

    // !arbitary! defined for max. user-defined states. Increase it if necessary

           static final int     MPE_USER_MAX_STATES  = 30;

    // variable "stateid" is used in MPI_Init() of <MPE>/src/log_wrap.c

           static final int     MPE_1ST_EVENT        = 1;  // i.e. stateid

    //  The initial event_number return by MPE_Log_get_event_number();
    //  Ref: clog.h, mpe_log.c

           static final int     MPE_USER_1ST_EVENT   = 500;  // CLOG_MAXEVENT

    public class AllType
    {
        public static final int UNDEF     =  0;         // CLOG_UNDEF
    }

    public class RecType
    {
        public static final int ENDLOG     = -2;         // CLOG_ENDLOG
        public static final int ENDBLOCK   = -1;         // CLOG_ENDBLOCK
        // public static final int UNDEF      =  0;         // CLOG_UNDEF
               static final int STATEDEF   =  1;         // CLOG_STATEDEF
               static final int EVENTDEF   =  2;         // CLOG_EVENTDEF
               static final int CONSTDEF   =  3;         // CLOG_CONSTDEF
               static final int BAREEVENT  =  4;         // CLOG_BAREEVENT
               static final int CARGOEVENT =  5;         // CLOG_CARGOEVENT
               static final int MSGEVENT   =  6;         // CLOG_MSGEVENT
               static final int COLLEVENT  =  7;         // CLOG_COLLEVENT
               static final int COMMEVENT  =  8;         // CLOG_COMMEVENT
               static final int SRCLOC     =  9;         // CLOG_SRCLOC
               static final int SHIFT      = 10;         // CLOG_SHIFT
    }

    class MsgType // is used to define send and recv events
    {
               static final int SEND      = -101;       // CLOG_MSG_SEND
               static final int RECV      = -102;       // CLOG_MSG_RECV
    }
}
