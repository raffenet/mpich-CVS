#include "adi3.h"

/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated August 30, 2000
 ***********************************************************************/
/*
 The following are routines that a device must implement; these are used
 by the code that implements the MPI communication operations.

 Note that some of these operations may be implemented using the others; for 
 example, MPID_Rhcv can use MPID_Rhc and a temporary memory buffer.  The
 routines that *must* be implemented are marked as MPID_CORE.  The other
 routines can be implemented (at some loss in efficiency) in terms of these
 or other standard operations.

 Most "routines" may also be implemented as macros in cases (such as memory 
 allocation) where performance is critical.  Any function that must be a
 function (because a pointer to it is needed) is marked ?how?

 This is MPID, not just MPID_CORE.
 */

/*
 * Section 1: Data Structures
 * 
 * Routines to manage and interrogate the data structures 
 */

/*
 * Datatypes 
 */

MPID_Datatype *MPID_Datatype_new( )
{
}

void MPID_Datatype_free( MPID_Datatype *datatype )
{
}

void MPID_Datatype_get_envelope()
void MPID_Datatype_get_contents()


/*@
  Notes:
  A 'rank' of 'MPI_ANY_SOURCE' matches the behavior of 'MPI_Pack'.
  @*/
int MPID_Pack( const void *inbuf, int count, MPID_Datatype type, 
	       void *outbuf, int *position, int outbuf_size, 
	       MPID_Comm, int rank )
{
}

/*@
  @*/
MPID_Unpack( )
{
}

/*
 * Groups
 *
 * For now, groups will *not* be scalable structures.
 */

MPID_Group *MPID_Group_new( int size )
{
}

MPID_Group_free( MPID_Group * )
{
}

/* 
 * Communicators 
 */
/*@

  Input Parameters:
+  old_comm - Call is collective over old_comm.  
.  new_group - Group of new communicator.
-  context_id - Context id for this communicator.

  Notes:
  This call initializes any device-specific information as well.
  @*/
MPID_Comm *MPID_Comm_create( MPID_Comm *old_comm, 
		      MPID_Group *new_group, int context_id )


int MPID_Comm_free( MPID_Comm *comm )

/*
 * Communicator attributes
 *
 * These provide a way for the user to pass data to the MPI implementation,
 * on a communicator-by-communicator basis.  To allow an implementation to
 * see changes to attributes, the following routines are called.  All may
 * be defined as macros that do nothing.
 *
 * Attributes on other objects are defined by MPI but not of interest (as yet)
 * to the device.
 */
void MPID_Comm_Attr_set( MPID_Comm, int keyval, void *attr_val )
{
}
void MPID_Comm_Attr_delete( MPID_Comm, int keyval )
{
}

/*
 * Requests 
 * 
 * Requests are fundamental objects, and are used throughout the device.
 * A typical implementation of the ADI will use the request to store 
 * information in a form that is easily communicated (e.g., can be sent
 * directly without copying or placed in shared memory).
 *
 * Requests contain a block of storage that may be used in an MPID_Rhc 
 * call as the argument data ((ptr,n) in the call).  
 */

/*@
  MPID_Request_FOA - Find or allocate a request matching the conditions.

  Output Parameter:
. found - Set to true if the request was found, else false

  Notes:
  A request that is not found is added to the queue of pending requests.

  A request that is created will have the field 'ready' set to false; this
  allows 'MPID_Request_FOA', called from another thread, to match this request
  and wait for it to become ready.  The routine 'MPID_Request_ready' must
  be called to set the ready field.

  As written, this is only for the receive queue.

  This design allows the device to choose either a single receive queue for
  all communicators or separate queues for each communicator.
  @*/
MPID_Request *MPID_Request_FOA( int tag, int rank, MPID_Comm comm, int *found )
{
}

/*@
  MPID_Request_new - Create a new request for send operations.
  @*/
MPID_Request *MPID_Request_new( int rank, MPID_Comm comm )
{
}

/*@
  This may be a macro that is as simple as
.vb
    (r)->ready = 1;
.ve
  particularly on single-threaded systems.
  Systems without write-ordering, however, may require an additional
  operation, such as a write flush.

  @*/
void MPID_Request_ready( MPID_Request * )
{
}

/*@
@*/
void MPID_Request_free( MPID_Request * )
{
}

/*
 * Windows 
 */

/*
 * Good Memory (for passive target operations on MPI_Win)
 */

/*@
  If memory is not available, returns null.
  @*/
void *MPID_Alloc_mem( size_t size )
{
}
/*@
  @*/
void MPID_Free_mem( void *ptr )
{
}

/*
 * Section 2: Communication
 *
 * General Notes: Communication operations are non-blocking, with a completion
 * flag.  In some cases, the flag may be specified as NULL; where this is
 * true, completion must be ensured by some other test.
 */
/*@

  Notes:
  This routine is non-blocking.  It is responsible for any flow control.

  The origin_buf `must` be acquired through 'MPID_Buffer_get_put'.

. target_flag - This is an id of a flag at the target process.  This value
  must have been specified by the target process in a previous communication
  (either with 'MPID_Put' or 'MPID_Rhc').  A value of '0' indicates no
  target completion flag.

  This routine is in MPID_CORE.
  @*/
int MPID_Put( const void * origin_buf, int n, 
	      MPI_Aint target_offset, int target_rank, MPID_Win window, 
	      volatile int *local_flag, MPI_Aint target_flag )
{
}

/*@
  MPID_Rhc - Remote handler call.

  Input Parameters:
+ rank - Rank of process to invoke handler on
. comm - Communicator that rank is relative to
. id   - Id of handler
. ptr  - Pointer to buffer to be sent to the handler.  
- n    - length of buffer pointed at by 'ptr'

  Output Parameter:
. local_flag - The location specified by this pointer is set when the operation
  has completed.

  Notes:
  Invokes one of the known handlers at the target rank.
  This call is non-blocking.

  This routine is in MPID_CORE.
  @*/
int MPID_Rhc( int rank, MPID_Comm comm, MPID_Handler_id id, void *ptr, int n,
	      volatile int *local_flag )
{
}

/*@
  Wait for completion of flags specified with MPID_Put, MPID_Get, or MPID_Rhc.
  These are local flags only (including local flags that were specified 
  as target flags by a remote process with this process as target).

  ? Do we want waitsome or waitany instead?

  Note:
  This routine allows a device to optimize waiting on completion.  For example,
  a TCP device could use select with a null timeout while waiting for an 
  operation, such as a write to a socket, to complete.  A shared-memory
  device could sleep (e.g., using a sem_op of 0 and a sem_flg of 0).
  
  This also allows devices that have special addresses for completion to
  transfer those completion values to the specified local flags.
  @*/
int MPID_Waitall( int count, int *(flags[]) )
int MPID_Testall( int count, int *(flags[]), int *found )

/*@
  Input Parameters:
+ count - number of completion flags to wait on
- flags - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.

  Notes:
  
  This routine is in MPID_CORE.
  @*/
int MPID_Waitsome( int count, int *(flags[]) )
{
}

/*@
  Input Parameters:
+ count - number of completion flags to wait on
- flags - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.
  If no flag is completed, 0 is returned.

  Notes:
  
  This routine is in MPID_CORE.
  @*/
int MPID_Testsome( int count, int *(flags[]) )
{
}



/*@

  @*/
int MPID_Get( void * origin_buf, int n, 
	      MPI_Aint target_offset, int target_rank, MPID_Win window, 
	      volatile void *local_flag, MPI_Aint target_flag )
{
}

/*@

    Vector version of MPID_Rhc
  @*/
int MPID_Rhcv( int rank, MPID_Comm comm, MPID_Handler_id id, 
	       const struct iovec *vector, int count )
{
}

/* 
 * Add MPID_Isend etc. here?
 */

/* 
 * Section 3: Buffer management
 */

/*@
  MPID_Buffer_get_put - Get a buffer ready for 'MPID_Put'.

  Input Parameters:
+ buf - Buffer to setup for sending
. count - Number of items
. dtype - Datatype of items
. comm  - Communicator for communication
- rank  - Rank of target.  Use 'MPI_ANY_SOURCE' for any member of the 
          communicator

  Output Parameter:
. buf_desc - Buffer descriptor.  If null, the original buffer is
  contiguous and can be used as is.

  Notes:
  If 'buf_desc' is non-null, the routine 'MPID_Buffer_fill' must be used
  to fill a memory location or return a pointer to a contiguous buffer.

  Questions: 
  Add an `intent` so that sending all of the data can use device 
  knowledge about complex data layouts?  E.g., intent == MPID_BUFFER_ALL
  or intend == MPID_BUFFER_SEGMENT?

  @*/
int MPID_Buffer_get_put( const void *buf, int count, MPID_Datatype dtype,
			 MPID_Comm comm, int rank, 
			 MPID_Buf **buf_desc )
{
}

/*@
  MPID_Buffer_fill - Pack up the designated buffer with a range of bytes.

  Input Parameters:
+ buf_desc - Buffer descriptor (initialized with 'MPID_Buffer_get_send')
- send_buffer - Pointer to buffer to place data in.  May be 'NULL', in which 
  case 'MPID_Buffer_fill' uses an internal buffer (and returns it).

  Inout Parameters:
+ first - Pointer to first byte index to be packed (on input) or actually
          packed (on output)
- last - Pointer to last byte index to be packed (on input) or actually
         packed (on output)

  Return value: 
  Pointer to buffer filled.

  Notes:
   
  @*/
void *MPID_Buffer_fill( MPID_Buf *buf_desc, int *first, int *last, 
		      void *send_buffer ) 
{
}

/*@
  
  @*/
void * MPID_Buffer_get_recv( void *buf, int count, MPID_Datatype dtype,
			     MPID_Comm comm, int rank, 
			     MPID_Buf **buf_desc )
{
}

/*@
  @*/
int MPID_Buffer_empty( MPID_Buf *buf_desc, int *first, int *last, 
		       void *recv_buffer )

/*@
  @*/
int MPID_Buffer_free( MPID_Buf *buf_desc )
{
}

/*@
 

 Notes: 
 This call is made by 'MPI_Send_init' and 'MPI_Recv_init' to mark memory
 as used for communication.

 This call allows devices that can optimize transfers for predefined 
 memory to take advantage of these calls.
  @*/
int MPID_Buffer_register( void *buf, int len, int rdwt )

/*@
  @*/
int MPID_Buffer_unregister( void *buf, int len )


/*
 * Section n: Miscellaneous
 */
/*@

  Notes:
  Null arguments `must` be valid.

  Multi-method devices should initialize each method within this call.

  This call also initializes all MPID datastructures.  Some of these (e.g.,
  'MPID_Request') may be in shared memory; others (e.g., 'MPID_Datatype') 
   may be allocated from an block of array values.  

  This routine is in MPID_CORE.
  @*/
int MPID_Init( int *argc, char *(*argv)[] )
{
}

/*@

  Notes:

  This routine is in MPID_CORE.
  @*/
int MPID_Abort( MPID_Comm comm )
{
}
/*@

  Notes:

  This routine is in MPID_CORE.
  @*/
int MPID_Finalize( )
{
}

? Information on data representations for heterogeneous code.

? Information on "good" message break points? (e.g., short/eager/rendezous)?

/*
 * Section n+1: Service Routines
 *
 * Many of these may be implemented (as macros) by their underlying routine
 * However, in some cases, it may be advantageous to use special routines.
 * Alternatives can include routines that trace or record operations.
 */

void *MPID_Malloc( size_t len )
{
}

void MPID_Free( void * ptr )
{
}

void *MPID_Memcpy( void *dest, const void *src, size_t n )
{
}

etc.

