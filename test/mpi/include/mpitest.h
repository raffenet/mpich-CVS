#ifndef MTEST_INCLUDED
#define MTEST_INCLUDED
/*
 * Init and finalize test 
 */
void MTest_Init( int **, char *(*argv)[] );
void MTest_Finalize( int );

/*
 * This structure contains the information used to test datatypes
 */
typedef struct _MTest_Datatype {
    MPI_Datatype *datatype;
    void *(*InitBuf)( struct _MTest_Datatype * );
    void *(*FreeBuf)( struct _MTest_datatype * );
    void *(*CheckBuf)( struct _MTest_datatype * );
    void *buf;
    int  count;
} MTest_datatype;

int MTest_Get_datatypes( MTest_datatype *, MTest_datatype * );
void MTest_Reset_datatypes( void );

#endif
