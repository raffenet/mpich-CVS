/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
#include "mpe_logging_conf.h"
#include "clogimpl.h"


/*
    CLOG_dumplog() is NOT used by any programs.  Not removing CLOG_dumplog()
    creates problem in using serial CC in making clog_print and clog2alog
*/
/*
void CLOG_dumplog()
{
    CLOG_BLOCK *block;

    block = CLOG_first;
    while (block) {
        printf("block at 0x%lx: \n", (long)((char *) block));
        CLOG_dumpblock(block->data);
        block = block->next;
    }
}
*/

void CLOG_outblock( double *p )
{
    CLOG_dumpblock(p);                /* for the time being */
}

void CLOG_dumpblock( double *p )
{
    int         rtype;
    CLOG_HEADER *h;

    rtype = CLOG_UNDEF;
    while (rtype != CLOG_ENDBLOCK && rtype != CLOG_ENDLOG) {
        h = (CLOG_HEADER *) p;
#ifndef WORDS_BIGENDIAN
        adjust_CLOG_HEADER(h);
#endif
        rtype = h->rectype;
        printf("ts=%f type=", h->timestamp);
        CLOG_rectype(h->rectype); /* print record type */
        printf(" len=%d, proc=%d ", h->length, h->procID);
        p = (double *) (h->rest);        /* skip to end of header */
        switch (rtype) {
        case CLOG_STATEDEF:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_STATE ((CLOG_STATE *)p);
#endif
            printf("state=%d ",  ((CLOG_STATE *) p)->stateID);
            printf("s_et=%d ",   ((CLOG_STATE *) p)->startetype);
            printf("e_et=%d ",   ((CLOG_STATE *) p)->finaletype);
            printf("color=%s ",  ((CLOG_STATE *) p)->color);
            printf("name=%s ",   ((CLOG_STATE *) p)->name);
            printf("fmt=%s\n",   ((CLOG_STATE *) p)->format);
            p = (double *)      (((CLOG_STATE *) p)->end);
            break;
        case CLOG_EVENTDEF:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_EVENT ((CLOG_EVENT *)p);
#endif
            printf("et=%d ",     ((CLOG_EVENT *) p)->etype);
            printf("color=%s ",  ((CLOG_EVENT *) p)->color);
            printf("name=%s ",   ((CLOG_EVENT *) p)->name);
            printf("fmt=%s\n",   ((CLOG_EVENT *) p)->format);
            p = (double *)      (((CLOG_EVENT *) p)->end);
            break;
        case CLOG_CONSTDEF:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_CONST ((CLOG_CONST *)p);
#endif
            printf("et=%d ",       ((CLOG_CONST *) p)->etype);
            printf("name=%s ",     ((CLOG_CONST *) p)->name);
            printf("value=%d\n",   ((CLOG_CONST *) p)->value);
            p = (double *)        (((CLOG_CONST *) p)->end);
            break;
        case CLOG_BAREEVENT:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_BARE ((CLOG_BARE *)p);
#endif
            printf("et=%d\n",      ((CLOG_BARE *) p)->etype);
            p = (double *)        (((CLOG_BARE *) p)->end);
            break;
        case CLOG_CARGOEVENT:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_CARGO ((CLOG_CARGO *)p);
#endif
            printf("et=%d ",        ((CLOG_CARGO *) p)->etype);
            printf("bytes=%.32s\n", ((CLOG_CARGO *) p)->bytes);
                                   /* CLOG_BYTES is 32 bytes long*/
            p = (double *)         (((CLOG_CARGO *) p)->end);
            break;
        case CLOG_MSGEVENT:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_MSG ((CLOG_MSG *)p);
#endif
            printf("et="); CLOG_msgtype(((CLOG_MSG *) p)->etype);
            printf(" tg=%d ",      ((CLOG_MSG *) p)->tag);
            printf("prt=%d ",      ((CLOG_MSG *) p)->partner);
            printf("cm=%d ",       ((CLOG_MSG *) p)->comm);
            printf("sz=%d\n",      ((CLOG_MSG *) p)->size);
            p = (double *)        (((CLOG_MSG *) p)->end);
            break;
        case CLOG_COLLEVENT:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_COLL ((CLOG_COLL *)p);
#endif
            printf("et="); CLOG_colltype(((CLOG_COLL *) p)->etype);
            printf(" root=%d ",   ((CLOG_COLL *) p)->root);
            printf("cm=%d ",      ((CLOG_COLL *) p)->comm);
            printf("sz=%d\n",     ((CLOG_COLL *) p)->size);
            p = (double *)       (((CLOG_COLL *) p)->end);
            break;
        case CLOG_COMMEVENT:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_COMM ((CLOG_COMM *)p);
#endif
            printf("et="); CLOG_commtype(((CLOG_MSG *) p)->etype);
            printf(" pt=%d ",       ((CLOG_COMM *) p)->parent);
            printf("ncomm=%d\n ",   ((CLOG_COMM *) p)->newcomm);
            p = (double *)         (((CLOG_COMM *) p)->end);
            break;
        case CLOG_SRCLOC:
#ifndef WORDS_BIGENDIAN
            adjust_CLOG_SRC ((CLOG_SRC *)p);
#endif
            printf("srcid=%d ",    ((CLOG_SRC *) p)->srcloc);
            printf("line=%d ",     ((CLOG_SRC *) p)->lineno);
            printf("file=%s\n",    ((CLOG_SRC *) p)->filename);
            p = (double *)        (((CLOG_SRC *) p)->end);
            break;
        case CLOG_SHIFT:
            printf("shift=%f\n",   ((CLOG_TSHIFT *) p)->timeshift);
            p = (double *)        (((CLOG_TSHIFT *) p)->end);
            break;
        case CLOG_ENDBLOCK:
            printf("end of block\n");
            break;
        case CLOG_ENDLOG:
            printf("end of log\n");
            break;
        default:
            printf("unrecognized record type\n");
        }
    }
}

/*@
     CLOG_reclen - get length (in doubles) of log record by type
@*/
int CLOG_reclen( int type )
{
    int restlen;

    switch (type) {
    case CLOG_ENDLOG:
        restlen = 1;
        break;
    case CLOG_ENDBLOCK:
        restlen = 1;
        break;
    case CLOG_STATEDEF:
        restlen = sizeof(CLOG_STATE) / sizeof(double);
        break;
    case CLOG_EVENTDEF:
        restlen = sizeof(CLOG_EVENT) / sizeof(double);
        break;
    case CLOG_CONSTDEF:
        restlen = sizeof(CLOG_CONST) / sizeof(double);
        break;
    case CLOG_BAREEVENT:
        restlen = sizeof(CLOG_BARE) / sizeof(double);
        break;
    case CLOG_CARGOEVENT:
        restlen = sizeof(CLOG_CARGO) / sizeof(double);
        break;
    case CLOG_MSGEVENT:
        restlen = sizeof(CLOG_MSG) / sizeof(double);
        break;
    case CLOG_COLLEVENT:
        restlen = sizeof(CLOG_COLL) / sizeof(double);
        break;
    case CLOG_COMMEVENT:
        restlen = sizeof(CLOG_COMM) / sizeof(double);
        break;
    case CLOG_SRCLOC:
        restlen = sizeof(CLOG_SRC) / sizeof(double);
        break;
    case CLOG_SHIFT:
        restlen = sizeof(CLOG_TSHIFT) / sizeof(double);
        break;
    default:
        printf("CLOG: Can't get length of unknown record type %d\n", type);
        restlen = 1;  /* Best that we can guess */
    }
    /* The above caculation is off by 2 because of the "rest" and "end"
       markers which are overwritten, so we subtract 2 in the next line.
       (For ENDLOG and ENDBLOCK restlen is set to 1 to make this work.) */

    return sizeof(CLOG_HEADER) / sizeof(double) + restlen - 2;
}

/*@
     CLOG_msgtype - print communication event type

. etype - event type for pt2pt communication event

@*/
void CLOG_msgtype( int etype )
{
    switch (etype) {
    case CLOG_MSG_SEND:
        printf("send");
        break;
    case CLOG_MSG_RECV:
        printf("recv");
        break;
        /* none predefined */
    default:
        printf("unk(%d)", etype);
    }
}

/*@
     CLOG_commtype - print communicator creation event type

. etype - event type for communicator creation event

@*/
void CLOG_commtype( int etype )
{
    switch (etype) {
    case INIT:   printf("init"); break;
    case DUP:    printf("dup "); break;
    case SPLIT:  printf("splt"); break;
    case CARTCR: printf("crtc"); break;
    case COMMCR: printf("cmmc"); break;
    case CFREE:  printf("free"); break;
    default:     printf("unknown(%d)", etype);
    }
}

void CLOG_colltype( int etype )
{
    switch (etype) {
        /* none predefined */
    default:      printf("unk(%d)", etype);
    }
}

/*@
     CLOG_rectype - print log record type

. rtype - record type

@*/
void CLOG_rectype( int type )
{
    switch (type) {
    case CLOG_ENDLOG:      printf("elog"); break;
    case CLOG_ENDBLOCK:    printf("eblk"); break;
    case CLOG_UNDEF:       printf("udef"); break;
    case CLOG_STATEDEF:    printf("sdef"); break;
    case CLOG_EVENTDEF:    printf("edef"); break;
    case CLOG_CONSTDEF:    printf("cdef"); break;
    case CLOG_BAREEVENT:   printf("bare"); break;
    case CLOG_CARGOEVENT:  printf("cago"); break;
    case CLOG_MSGEVENT:    printf("msg "); break;
    case CLOG_COLLEVENT:   printf("coll"); break;
    case CLOG_COMMEVENT:   printf("comm"); break;
    case CLOG_SRCLOC:      printf("loc "); break;
    case CLOG_SHIFT:       printf("shft"); break;
    default:               printf("unknown(%d)", type);
    }
}

/*
    Functions given below change the byte ordering of data in the various
    structs to make sure that always data is written out in accordance to 
    the MPI standard. Only datatypes of int and doubles may be changed and
    also in the case of doubles we are only concerned with the byte ordering
    assuming that all machined follow the IEEE storage convention
*/
void adjust_CLOG_HEADER( CLOG_HEADER *h )
{
    CLOG_byteswap( &(h->timestamp), sizeof(double), 1 );
    CLOG_byteswap( &(h->rectype), sizeof(int), 3 );
    /* We do not adjust the 'pad' field */
}

void adjust_CLOG_STATE( CLOG_STATE *state )
{
    CLOG_byteswap( &(state->stateID), sizeof(int), 3 );
    /* We do not adjust the 'pad' field */
    /* 'color' and 'name' fields are not adjusted */
}

void adjust_CLOG_EVENT( CLOG_EVENT *event )
{
    CLOG_byteswap( &(event->etype), sizeof(int), 1 );
    /* 'pad' and 'name' are not adjusted */
}

void adjust_CLOG_CONST( CLOG_CONST *constant )
{
    CLOG_byteswap( &(constant->etype), sizeof(int), 2 );
    /* 'name' are not adjusted */
}

void adjust_CLOG_BARE( CLOG_BARE *bare )
{
    CLOG_byteswap( &(bare->etype), sizeof(int), 1 );
}

void adjust_CLOG_CARGO( CLOG_CARGO *cargo )
{
    CLOG_byteswap( &(cargo->etype), sizeof(int), 1 );
    /* 'cargo->bytes[]' is adjusted by user */
}

void adjust_CLOG_MSG( CLOG_MSG *msg )
{
    CLOG_byteswap( &(msg->etype), sizeof(int), 5 );
    /* We do not adjust the 'pad' field */
}

void adjust_CLOG_COLL( CLOG_COLL *coll )
{
    CLOG_byteswap( &(coll->etype), sizeof(int), 4 );
}

void adjust_CLOG_COMM( CLOG_COMM *comm )
{
    CLOG_byteswap( &(comm->etype), sizeof(int), 3 );
    /* We do not adjust the 'pad' field */
}

void adjust_CLOG_SRC( CLOG_SRC *src )
{
    CLOG_byteswap( &(src->srcloc), sizeof(int), 2 );
    /* 'filename' is not adjusted */
}
