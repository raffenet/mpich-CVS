#ifndef MTEST_INCLUDED
#define MTEST_INCLUDED
/*
 * Init and finalize test 
 */
void MTest_Init( int *, char ***argv );
void MTest_Finalize( int );

/*
 * This structure contains the information used to test datatypes
 */
typedef struct _MTest_Datatype {
    MPI_Datatype datatype;
    void *(*InitBuf)( struct _MTest_Datatype * );
    void *(*FreeBuf)( struct _MTest_Datatype * );
    void *(*CheckBuf)( struct _MTest_Datatype * );
    void *buf;
    int  count;
} MTest_Datatype;

int MTest_Check_recv( MPI_Status *, MTest_Datatype * );
int MTest_Get_datatypes( MTest_Datatype *, MTest_Datatype * );
void MTest_Reset_datatypes( void );

#endif
