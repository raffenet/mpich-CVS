/* -*- Mode: C; c-basic-offset:4 ; -*- */
#include "adi3.h"

/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 22, 2000
 * Updated May 21, 2002
 *
 * May 16, 2003 - Moving text from this file into mpiimpl.h and related
 * files as appropriate, so that the ADI3 documentation is attached 
 * directly to the development source code.
 ***********************************************************************/
/* old Overview.tex
 The following are routines that a device must implement; these are used
 by the code that implements the MPI communication operations.

 Note that some of these operations may be implemented using the others; for 
 example, 'MPID_Get_contig' can use 'MPID_Rhcv' and 'MPID_Put_contig'.  The
 routines that `must` be implemented are marked as 'MPID_CORE'.  The other
 routines can be implemented (at some loss in efficiency) in terms of these
 or other standard operations.  The MPICH2000 implementation will include
 an implementation of all of these routines, using 'MPID_CORE' for 
 communication and other code for local operations (such as the datatype 
 operations).  
*/

/*TOverview.tex

 The original ADI design had the notion of a Core set of routines that
 could implement the remaining routines.  Those routines could be directly
 implemented for greater performance.  The current ADI3 design does not
 have that structure; instead, the ADI3 is a fairly rich design, with the
 understanding that many implementations of the ADI will choose to build 
 some of the ADI routines from a smaller set of base routines.  For 
 example, the new ADI3 "channel" device defines a much smaller set of 
 functions out of which all ADI3 routines are implemented. 

 Most "routines" may also be implemented as macros in cases (such as memory 
 allocation) where performance is critical.  Any function that must be a
 function (because a pointer to it is needed) is marked 'NO MACRO ALLOWED'.
 (So far, there are no functions in this category).  For some functions, such
 as the timer routines, macros are recommended.

 In a few places, a decision hasn't been made.  These are marked as "Question".
 When answered, these will turn into "Rationales".

 Error handling and reporting:
 
 The ADI routines assume that they are provided with
 valid arguments and do no checking that 
 the arguments are valid.  The only exception is for errors that are difficult
 to check for in advance, such as an invalid buffer encountered because of a 
 complex datatype
 or message truncation.  Routines that involve communication may also return 
 errors caused by improper use (e.g., mismatched collective calls, invalid 
 ready send).  When an error is detected, an error code is created for it.
 This code contains an MPI error class and an instance-specific handle, used 
 to record more detailed information about the error.  Where an error code
 is returned, the value '0' signifies no error ('MPI_SUCCESS' is '0').

 Where possible, all errors should be recoverable.  That is, it should be
 possible for an MPI program to continue to run correctly.  When an error has 
 significant consequences, these should be limited to the smallest communicator
 possible.  For example, the failure of a process in 'MPI_COMM_WORLD'
 should make
 'MPI_COMM_WORLD' invalid, but 'MPI_COMM_SELF' and communicators that do not 
 involve the failed process should remain valid.  

 Many routines have no error returns.  This is a tough call, but the reason 
 is that the routines with no error returns have no opportunity to detect
 an error that should not have been caught by some other routine (remember,
 these aren't user-level routines).

 Semantics of Routines:
 The semantics of these routines normally follow the semantics of the 
 related MPI routines, and the MPI standard should be consulted for any 
 details.  Differences in the semantics are noted in the "Notes" section
 of each routine.
 T*/

/*
 * Section 1: Data Structures
 * 
 * Routines to manage and interrogate the data structures.
 *
 */

/*
 * Datatypes 
 */

/* @
    MPID_Datatype_new - Create a new datatype structure

    Return value: Pointer to the new data structure.  The following
    fields are already set\:
+   id        - 'MPI_Datatype' handle for this datatype.
-   ref_count - set to 1.

    Notes:
    If no datatype can be allocated, a null is returned.
    This routine only provides a pointer to the structure.  All ``public''
    fields, such as 'has_sticky_ub' and 'dataloop', will be set by the 
    implementation of the various MPI datatype construction routines.

    Module:
    Datatype
  @ */
MPID_Datatype *MPID_Datatype_new( void )
{
}

/* @
   MPID_Datatype_free - Free a datatype stucture.

   Input Parameter:
.  datatype - Pointer to datatype to free

   Notes:
   Unlike the MPI version, this does not assign 'NULL' to the input parameter.
   Any special operations that a device may need to take when a datatype
   is freed.

   Module:
   Datatype
  @ */
void MPID_Datatype_free( MPID_Datatype *datatype )
{
}

void MPIU_Object_add_ref( MPIU_Object_head *ptr );

int MPIU_Object_release_ref( MPIU_Object_head *ptr, int *newval_ptr )
{}


/* @
  MPID_Datatype_commit - Commits a datatype for use in communication

  Input Parameter:
. datatype - Datatype to commit

  Notes:
  All datatypes must be committed before they can be used in any MPID
  communication call.

  This routine can process the datatype inorder to optimize operations with it.

  Module:
  Datatype
  @*/
int MPID_Datatype_commit( MPID_Datatype *datatype )
{}

/* 
 * There is no datatype_get_envelope and datatype_get_contents because the
 * structure of this part of the datatype is defined by the MPI implementation.
 */


/* @
  MPID_Pack - Pack a message into a specified buffer as type 'MPI_PACKED'

  Input Parameters:
+ inbuf - Source data to pack
. count - Number of items to pack
. type - datatype of items to pack
. outbuf_size - size of output buffer
. comm - Communicator
- rank - Rank of destination in communicator

  Inout Parameters:
+ position - position of first free location in 'outbuf' (updated)
- outbuf - buffer holding packed data

  Return Value:
  Error code.  
  Notes:
  A 'rank' of 'MPI_ANY_SOURCE' matches the behavior of 'MPI_Pack'.

  This is really a special case of the routine 'MPID_Segment_pack', where the
  destination buffer is large enough to hold the entire message. (is it? See
  questions.)

  Module:
  Datatype

  Questions:
  Do we really want this as a separate routine, or can we use the 
  'MPID_Segment_xxx'
  routines?  If we can''t use the segment routines, does that mean that we 
  don''t have the right API yet, or is there an important difference?
  Current thinking is yes, we should.  We should try to build an
  implementation of 'MPI_Pack' using 'MPID_Segment_pack'.  However,
  the implementation of 'MPID_Segment_pack' should have an internal
  routine that is optimized for the case that a sufficiently large 
  destination buffer exists.  This allows us to hoist the buffer overrun
  tests out of the loops, improving performance of the data pack and 
  unpack operations.

  (The remaining apply only the the case of heterogeneous (in data 
  representation) systems.)

  In ADI-2, the packing format was determined first from '(comm,rank)' and
  then passed to the ADI-2 version of this routine.  That format was also
  sent to the receiver (as part of the message envelope), so that the data
  could be correctly interpreted.

  If we use receiver-makes-right, then we don''t need to do this, and we 
  don''t even need the '(comm,rank)' arguments.  That also matches the MPI
  notion of allowing data sent with 'MPI_PACKED' to be received with a 
  specific (and type-signature-conforming) datatype. However, 
  receiver-makes-right requires more code at the Unpack end (as well
  as '(comm,rank)').

  Do we want to mandate receiver-makes-right?  Globus guys, speak up.

  The answer appears to be no, we do not want to mandate receiver-makes-right. 
  MPICH-G2 uses a clever approach that makes use of the vendor MPI wherever
  possible, and this cannot require any particular format.

  Note that using XDR by itself isn''t really an option, because it doesn''t 
  handle all of the 
  C datatypes (e.g., 'long double') or Fortran (e.g., 'LOGICAL'), though we
  could use XDR with some supplementary types (e.g., encoding 'LOGICAL' as
  an integer).

  Another option is to add the packed type to the head of the output buffer;
  then all that is needed is a bit in the envelope telling the receiver that 
  the data is packed (and that can be set when the datatype is made up of 
  'MPI_PACKED').

  @*/
int MPID_Pack( const void *inbuf, int count, MPID_Datatype *type, 
	       void *outbuf, int *position, int outbuf_size, 
	       MPID_Comm *comm, int rank )
{
}

/* @
  MPID_Unpack - Unpack a buffer created with MPID_Pack

  Notes:
  See 'MPID_Pack'.  As for that routine, this routine may be a
  special case of 'MPID_Segment_unpack'.

  Module:
  Datatype
  @*/
MPID_Unpack( void *outbuf, int count, MPID_Datatype *type,
             const void *inbuf, int *positin, int inbuf_size, 
	     MPID_Comm *comm, int rank )
{
}

/* @
   MPID_Pack_size - Return a bound on the size of packed data

   Input Parameters:
+  count - Number of items to pack
.  type  - Datatype of items to pack
.  comm  - Communicator to pack for
-  rank  - rank of destination to pack for (may be 'MPI_ANY_SOURCE')

   Return value:
   An upper bound on the number of items of type 'MPI_PACKED' that this
   much data will take when packed.

   The size returned may include the size required to include a header
   in the buffer created by 'MPI_Pack'.  

   Notes:
   This is very similar to the 'MPI_Pack_size' routine except for the 'rank'
   argument.  A 'rank' of 'MPI_ANY_SOURCE' should give the same value
   as 'MPI_Pack_size'.

   Module:
   Datatype
  @*/
int MPID_Pack_size( int count, MPID_Datatype *type, MPID_Comm *comm, int rank )
{}

/*
 * Groups
 *
 * For now, groups will *not* be scalable structures; that is, they will
 * enumerate their members, rather than using a scalable structure.
 */

/* @
  MPID_Group_new - Create a new group of the specified size

  Input Parameter:
. size - size of the group (number of processes)

  Notes:
  Returns a pointer to a group structure.  The field 'lrank_to_lpid' (a 
  pointer to an array) is allocated by this routine and the reference count
  'ref_count' is set to one.
  
  Module:
  Group
  @ */
MPID_Group *MPID_Group_new( int size )
{
}

/* @
  MPID_Group_free - Free a group

  Input Parameter:
. group - Group to free

  Notes:
  This is the counterpart of 'MPID_Group_new'.  It should be called only
  to free the device''s representation of a group.  

  Module:
  Group
  @ */
void MPID_Group_free( MPID_Group *group )
{
}

/* 
 * Communicators 
 */
/* @
  MPID_Comm_create - Create a new communicator from an old one and a group

  Input Parameters:
+  old_comm - Call is collective over old_comm.  
.  new_group - Group of new communicator.  This may be an existing 
   group (with an incremented reference count), a new group created
   with 'MPID_Group_new', or 'NULL' (see below).
-  context_id - Context id for this communicator.

  Return value:
  A pointer to the new 'MPID_Comm' if the calling process is a member of
  'new_group' or null otherwise.

  Notes:
  This call initializes any device-specific information as well.

  The reason that this routine is labeled 'create' instead of 'new' is that 
  this is a `collective` routine; it not only allocates a new communicator
  but is called `collectively` so that a device may update any information
  about communicators that requires collective communication or operations.
  The call is collective over the old group, so some of the processes 
  calls may not be in the new group.  In that case, 'new_group' is null.

  Module:
  Communicator

  Question: 
  Should there be an error return in case something more global
  goes wrong?

  Remark:
  Newer thinking is that the group isn''t necessary, only the virtual
  connection table.  Thus, this really isn''t the right routine.
  @ */
MPID_Comm *MPID_Comm_create( MPID_Comm *old_comm, 
			     MPID_Group *new_group, int context_id )
{
}

/* @
  MPID_Comm_context_id - Determine a new context id for a communicator

  Input Parameters:
+  old_comm - Call is collective over old_comm.  
-  new_group - Group of new communicator.  This may be an existing 
   group (with an incremented reference count), a new group created
   with 'MPID_Group_new', or 'NULL' (see below).

   Return value:
   A context id that may be used, along with 'old_comm' and 'new_group', 
   in a call to 'MPID_Comm_create' to create a new group.


  Notes:
  Determining a value for 'context_id' in the multithreaded case is not easy.
  The simplest algorithms can dead-lock in a valid program.  Consider this 
  case\:
  (this isn''t right yet)
.vb
  process 1                      process 2
  thread 1        thread2          thread1        thread2
  Comm_dup(comm1)                                 Comm_dup(comm2)
                  Comm_dup(comm2)  Comm_dup(comm1)       
.ve
  If, inside the implementation of 'Comm_dup', each `process` holds a 
  lock on a global 'context_id' stack while a 
  `thread` does a collective operation (such as an 'MPI_Allreduce'), this
  valid code will hang.  

  There is a way to avoid the process lock at the cost of additional
  collective operations.  The steps necessary to get a new 'context_id'
  are sketched below\:
.vb
  MPI_Allreduce( over bitmask of available values )
  Find candidate value.
  if not MPI_THREAD_MULTIPLE, reserve and return that value.
  Try to reserve that value (another thread may already have, so this
  step may fail)
  MPI_Allreduce( over success at reserving value )
  if all processes successful, return that value
  release that value
  Return to the top  
.ve
  This can live-lock; a more sophisticated algorithm would keep track of
  who was reserving context ids and provide a deterministic way to break
  livelock.  See the MPICH Design Document for a more sophisticated version.

  This routine is part of ADI-3 because some devices may be able to 
  create new context ids with lower overhead than the generic implemention.
  Consider a shared-memory system.
  Let there be a global queue of available context ids.  The lowest rank 
  process in the old communicator uses an atomic operation to remove the
  next value from the queue, and then broadcasts it to the members of the
  (old) communicator using a private context id (e.g., the private communicator
  used in MPICH-1).  Freeing a communicator returns the context id to the 
  shared pool.

  Rather than provide a way to return multiple context values, this routine 
  should be used to return a single basic value that can be multiplied by
  a power of two in order to provide a cluster of consequtive values.

  Module:
  Communicator

@*/
int MPID_Comm_context_id( MPID_Comm *old_comm, MPID_Group *new_group )
{
}

/* @
  MPID_Comm_free - Frees a communicator allocated by 'MPID_Comm_create'

  Input Parameter:
. comm - Pointer to communicator to free.

  Module:
  Communicator
  @ */
void MPID_Comm_free( MPID_Comm *comm )
{
}

/*
 * Section : Communicator attributes
 */

/* @
  MPID_Dev_comm_attr_set_hook - Inform the device about a change to an attribute
  value for a particular communicator.

  Input Parameters:
+ comm  - Communicator with attribute
. keyval - Key value for attribute
. attr_val - Value of attribute
. lang - Language that attribute was set from (see notes).
- was_set - Indicates whether the attribute was set (1) or deleted (0)

  Notes:
  This routine allows the device to find out when an attribute value changes.
  This can be used by a device that defines its own keyvals (see 
  'MPID_Init') to allow the MPI user to communicate preferences to 
  the device.  This has already been used in MPICH-G2 to pass 
  quality-of-service information to the device.

  The language value is necessary because Fortran 77 attributes are integers,
  while C, C++, and Fortran 90 attributes are pointers (addresses).  These
  may not be the same length, so the address 'attr_val' must be interpreted
  differently.  Note that MPI uses 4 separate routines since each language
  has its own binding (the Fortran 90 one is 'MPI_Comm_attr_set'; the 
  Fortran 77 one is 'MPI_Attr_set').

  By default, this is '#define'd as empty in 'mpiimpl.h'.  A device that 
  wishes to use 
  this function should '#undef' it in the device include file ('mpidevpost.h').

  Module:
  Attribute

  Question:
  We may want to register keyvals with the device so that only attributes
  associated with the device call into it.  (e.g., 
  'MPID_Create_keyval( ... )').
  That also allows us to define
  "enhanced" keyvals that contain functions that validate any changes to the
  attribute values (e.g. an 
.vb
  MPID_Attr_validate_function( int keyval, void *attr_val, 
                               MPID_Lang_t lang )
.ve
  ). This also argues that 'MPID_Dev_comm_attr_set_hook' routine should
  return an MPI error code, allowing the device to cleanly signal an error
  to the user.
  @*/
void MPID_Dev_comm_attr_set_hook( MPID_Comm *comm, int keyval, void *attr_val, 
                                  MPID_Lang_t lang, int was_set )
{
}

/* @
    MPID_Dev_xxx_create_hook - Inform the device about the creation of xxx

    Input Parameter:
.   obj - Pointer to the object that has been created

    Notes:
    This is a pseudo routine.  The device may define routines of this
    for for each MPID object (e.g., 'MPID_Comm', 'MPID_Datatype', 
    'MPID_File', etc.).  The 'xxx' is replaced with the name of the
    object; e.g., 'comm', 'datatype', 'file', etc.

    This may be defined as an empty macro.

    The exact parameter list may depend on the object.  These parameter lists
    have not yet been defined.

    If a device defines either the create or destroy hooks, it must define
    both.  In addition, the C preprocessor symbol 'HAVE_DEV_xxx_HOOK' must
    be defined.  If this is not defined, then 'MPID_Dev_xxx_create_hook'
    and 'MPID_Dev_xxx_destroy_hook' will be defined as empty macros.

    See also 'MPID_Dev_xxx_destroy_hook'

  @*/
void MPID_Dev_xxx_create_hook( MPID_xxx *obj )
{}

/* @
    MPID_Dev_xxx_destroy_hook - Inform the device about the destruction of xxx

    Input Parameter:
.   obj - Pointer to the object that has been created

    Notes:
    This is a pseudo routine.  The device may define routines of this
    for for each MPID object (e.g., 'MPID_Comm', 'MPID_Datatype', 
    'MPID_File', etc.).  The 'xxx' is replaced with the name of the
    object; e.g., 'comm', 'datatype', 'file', etc.

    This may be defined as an empty macro.

    See also 'MPID_Dev_xxx_create_hook'
  @*/
void MPID_Dev_xxx_create_hook( MPID_xxx *obj )
{}

/*
 * Communicator locks for thread safety (?)
 * For example, in the MPI_THREAD_MULTIPLE mode, we must ensure that
 * two threads don't try to execute a collective communication routine
 * on the same communicator at the same time.  Thus, we need a communicator
 * lock for each communicator, and this lock is needed only when 
 * MPI_THREAD_MULTIPLE is provided.
 *
 * Actually, the above is not true.  According to the MPI-2 specification,
 * it is the user's responsibility not to call different 
 * collective operations on the same communicator from different threads
 * We do have a design for a debugging mode that will help identify this 
 * error and notify the user.
 *
 * As a special case, we might allow, on a communicator by communicator
 * basis, the setting of the threadedness of communication on the 
 * communicator (this provides better modularity).  
 */

void MPID_Comm_thread_lock( MPID_Comm *comm )
{
}

void MPID_Comm_thread_unlock( MPID_Comm *comm )
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
 * Requests contain a block of storage that may be used in an MPID_Rhcv 
 * call as the argument data (vector[0](ptr,n) in that call).  
 *
 * MPI has a number of different types of requests.  For example,
 * persistent requests and generalized (or user) requests.  The 
 * requests described here apply only to the common requests used in 
 * MPI_Isend, MPI_Irecv, etc.  The implementation of the MPI functions
 * such as MPI_Waitsome will handle these different kinds of requests.
 *
 * Actually, this isn't quite true.  Now that we have MPID_Irecv etc., 
 * MPID_Waitsome will be presented with all kinds of requests.
 */

/* @
  MPID_Request_recv_FOA - Find or allocate a request matching the conditions 
  for receive operations

  Input Parameters:
+ tag - Tag to match (or 'MPI_ANY_TAG')
. rank - rank to match (or 'MPI_ANY_SOURCE')
- comm - communicator to match.

  Output Parameter:
. found - Set to true if the request was found, else false.  Found means that
  the returned communicator was already in the queue; not found means that 
  it was not found, but was inserted.

  Return value:
  A request is always returned.  If the request was found, it is removed from
  the queue.

  Notes:
  A request that is not found is added to the queue of pending requests.
  A request that is found is removed from the queue of pending requests.

  A request that is created will have the field 'ready' set to false; this
  allows 'MPID_Request_recv_FOA', called from another thread, to match this 
  request
  and wait for it to become ready.  The routine 'MPID_Request_ready' must
  be called to set the ready field.

  This design allows the device to choose either a single receive queue for
  all communicators or separate queues for each communicator.

  Module:
  Request

  Question:
  Another design makes the queues explicit, and then makes the queue to
  act on an argument.  This format leaves the queues implicit, but 
  requires either more functions or another argument to select which queue
  if more than one (e.g., both pending sends and receives) are used.

  An advantage of this form is that the receive queue can be spread across
  the communicators, since there is no wild-card on communicator id.  Doing
  so would help isolate the impact of communication on one communicator with
  another.

  Do we really need this routine?  If we have 'MPID_Irecv', is this 
  needed at all?  Is it a routine that is used to implement 'MPID_Irecv', 
  using only the 'MPID_CORE' routines?
  @ */
MPID_Request *MPID_Request_recv_FOA( int tag, int rank, MPID_Comm *comm, 
				     int *found )
{
}

/* @
  MPID_Request_send_FOA - Find or allocate a request matching the conditions 
  for send operations

  Input Parameters:
+ tag - Tag to match (must be a valid tag; 'MPI_ANY_TAG' is not allowed)
. rank - rank to match (must be a valid rank; 'MPI_ANY_SOURCE' is not allowed)
- comm - communicator to match.

  Output Parameter:
. found - Set to true if the request was found, else false.  Found means that
  the returned communicator was already in the queue; not found means that 
  it was not found, but was inserted.

  Return value:
  A request is always returned.  If the request was found, it is removed from
  the queue.  If a request cannot be returned, 'MPID_Abort' is called on
  'MPI_COMM_SELF'.

  Notes:
  A request that is not found is added to the queue of pending requests.
  A request that is found is removed from the queue of pending requests.

  A request that is created will have the field 'ready' set to false; this
  allows 'MPID_Request_send_FOA', called from another thread, to match this 
  request 
  and wait for it to become ready.  The routine 'MPID_Request_ready' must
  be called to set the ready field.

  This design allows the device to choose either a single send queue for
  all communicators or separate queues for each communicator.

  This routine allows a device to implement the "speculative receive", where
  a receive operation such as 'MPI_Irecv' sends a rendezvous message to the
  specified source.  Particularly for wide-area-networks and 
  shared-memory-only devices, this can reduce the latency for point-to-point
  communications.  A device is free not to implement a speculative receive, 
  in which case this routine simply allocates a request (i.e., it never 
  finds one in the queue already because no code ever inserts one).

  Module:
  Request

  Question:
  Another design makes the queues explicit, and then makes the queue to
  act on an argument.  This format leaves the queues implicit, but 
  requires either more functions or another argument to select which queue
  if more than one (e.g., both pending sends and receives) are used.

  An advantage of this form is that the send queue can be spread across
  the communicators, since there is no wild-card on communicator id.  Doing
  so would help isolate the impact of communication on one communicator with
  another.

  @ */
MPID_Request *MPID_Request_send_FOA( int tag, int rank, MPID_Comm *comm, 
				     int *found )
{
}

MPID_Request *MPID_Request_create( void )
{
}

/* @ MPID_Request_set_completed - Mark a request as completed 

  Input Parameter:
. request_ptr - Pointer to request to mark as completed.

  Notes:
  This routine is intended for use by 'MPI_Grequest_complete' only.  It is 
  provided to ensure that any MPI routine that is blocked waiting for a 
  request to complete will be unblocked.
  @*/
void MPID_Request_set_completed( MPID_Request *request )
{
}

int MPID_Iprobe( int source, int tag, MPID_Comm *comm, int context_offset, 
		 int * flag, MPI_Status *status )
{
}

int MPID_probe( int source, int tag, MPID_Comm *comm,
		int context_offset, MPI_Status *status )
{
}

int MPID_Cancel_send( MPID_Request *request )
{
}

int MPID_Cancel_recv( MPID_Request *request )
{
}

/* @
  MPID_Request_ready - Mark a request as ready for use

  Input Parameter:
. request - request to update

  Notes:
  This is used to modify a request returned from 'MPID_Request_FOA' to 
  indicate that the request is now ready for use.  This allows us to 
  insert a request into the queue without first setting all of the
  necessary fields ('MPID_Request_FOA' sets only the fields involved in 
  matching, i.e., tag, source, and context_id).  

  This may be a macro that is as simple as
.vb
    (request)->ready = 1;
.ve
  particularly on single-threaded systems.
  Systems without write-ordering, however, may require an additional
  operation, such as a write flush.

  In addition, non-cache-coherent systems must flush the request to memory
  to ensure that other threads will read the updated request values.

  This routine is not in 'MPID_CORE'; however, a general solution appropriate
  for multi-threaded systems involves a thread-lock and is thus rather
  expensive.  Devices supporting or using multiple threads should provide
  an efficient implementation of this routine.

  Module:
  Request
  @ */
void MPID_Request_ready( MPID_Request *request )
{
}

void MPID_Request_release( MPID_Request *request )
{
}

/*
 * Windows 
 */


/*
 * Section 2: Communication
 */

/*TCMOverview.tex
 *
 * Communication operations are non-blocking, with a completion
 * counter.  In some cases, the address of the counter may be specified as 
 * 'NULL'; where this is
 * true, completion must be ensured by some other test.  Counters are used
 * instead of boolean flags to provide a way to handle operations that 
 * involve multiple (internal) steps.
 *
 * Flow control is the responsibility of the device; these routines are
 * non-blocking, allowing the device to queue communication for
 * later completion.
 * 
 * Because the communication/data transfer routines (e.g., 'MPID_Put_contig', 
 * 'MPID_Get_contig', and 'MPID_Isend') are nonblocking, the buffers 
 * containing the data to 
 * be transfered must not be modified until the operation has (locally) 
 * completed (as marked by the appropriate counter).
 *
 * Because it may be necessary to update the reference count on the MPI 
 * objects, there are few parameters that may be specified as 'const'.
 *
 T*/

/* old text -
 * I am unconvinced that we want to define 'MPID_Isend', 'MPID_Irecv', etc.,
 * as part of the ADI, because of the complexities in handling general 
 * datatypes.  Further, this doesn't offer much of an abstraction, since an 
 * implementor can simply replace that part of the code in the MPI version
 * of the routine.  We may instead want to define either some routines for
 * simpler datatypes (e.g., contiguous, strided, or Unix-style iovec blocks), 
 * or enhance or change the stream interface.
 */

/* @
  MPID_Put_contig - Copy local memory to a remote process

  Input Parameters:
+ buf - Pointer to local memory to be copied
. n   - Number of bytes to move
. target_offset - Location of destination 
. target_rank - Rank of the destination process in the communicator
. comm - Communicator
. local_flag - Address of a flag to be set when this call is locally complete.
- target_flag - This is an id of a flag at the target process.  This value
  must have been specified by the target process in a previous communication
  (either with 'MPID_Put_contig' or 'MPID_Rhcv').  A value of '0' indicates no
  target completion flag.

  Notes:
  This routine is non-blocking.  It is responsible for any flow control.

  The 'origin_buf' `must` be acquired through 'MPID_Segment_init_pack'.
  This restriction allows 'MPID_Put_contig' to assume that the origin 
  memory is appropriate for the put operation.  However, the generic
  implementation of 'MPID_Segment_init_pack' will not take any special steps;
  an implementation of 'MPID_Put_contig' that requires such must also
  provide the matching 'MPID_Segment_xxx' routines.

  The 'target_offset' is relative to 'MPI_BOTTOM', and for all expected
  ADI implementations is the same as the address of the target memory location.

  This routine uses a communicator instead of a window because it may be
  used to implement `any` kind of data transfer between processes.  For
  example, the generic implementation of 'MPID_Isend' may use 'MPID_Rhcv'
  to implement a rendezvous with the destination process and 
  'MPID_Put_contig' to move the data to the destination once the receive
  buffer is allocated.  

  Module:
  MPID_CORE

  Questions:
  What addresses are valid for local flags?  What offsets (offsets
  because remote locations are specified by offset rather than address, 
  since a remote system may have a different-sized address) are valid for 
  target flags?  Should the type of a flag be 'MPID_flag_t'?  
  Do we require a test call on the flag, or only that the local flag be 
  declared as volatile (as in the prototype)?

  My current thinking is to require 'MPID_Flag_t', and provide a routine
  to allocate and free them.  The device can easily include one within
  an 'MPID_Request' structure.  The group voted yes on this, but the
  document has not yet been changed to reflect this.

  Do we want to require that the buffer be setup with the segment routine, 
  or is there a separate routine in 'MPID_CORE' that should be called for
  any data moved with 'MPID_Put_contig'?

  Do we want more information to be associated with the flags?  
  For example, each flag could have a list of the processes waiting
  on a flag.  My current feeling is that this is up to the 
  ADI implementation and need not be required of all devices.  That is,
  it could be part of the ``private'' (implementation-specific) data in a flag.
  @*/
int MPID_Put_contig( const void *origin_buf, int n, 
		     MPI_Aint target_offset, int target_rank, MPID_Comm *comm,
		     volatile int *local_flag, MPI_Aint target_flag )
{
}

/* @
  MPID_Flags_waitall - Wait for completion of flags specified with
  MPID_Put_contig, MPID_Get_contig, MPID_Rhcv, or similar routine.

  Input Parameters:
+ count - Number of flags to wait on
- flag_ptrs - Array of pointers to flags.  See notes.

  Note:
  These are local flags only (including local flags that were specified 
  as target flags by a remote process with this process as target).

  This routine allows a device to optimize waiting on completion.  For example,
  a TCP device could use select with a null timeout while waiting for an 
  operation, such as a write to a socket, to complete.  A shared-memory
  device could sleep (e.g., using a 'sem_op' of 0 and a 'sem_flg' of 0).
  
  This also allows devices that have special addresses for completion to
  transfer those completion values to the specified local flags.

  Module:
  Communication
  @ */
int MPID_Flags_waitall( int count, int *(flag_ptrs[]) )
{
}

/* @
  MPID_Put_sametype - Implement a put operation where the source and 
  destination datatypes are (roughly) the same

+ buf - Pointer to local memory to be copied
. n   - Count of datatype to move
. dtype - Datatype describing both the source and destination datatype. 
  See notes.
. target_offset - Location of destination 
. target_rank - Rank of the destination process in the communicator
. comm - Communicator
. local_flag - Address of a flag to be set when this call is locally complete.
- target_flag - This is an id of a flag at the target process.  This value
  must have been specified by the target process in a previous communication
  (either with 'MPID_Put_contig' or 'MPID_Rhcv').  A value of '0' indicates no
  target completion flag.
  Notes:
  This routine allows the efficient implementation of strided copies, where
  both the origin and the target have the same strides.  

  Question:
  We may want to generalize this to two related types, where there are 
  only a few differences between the datatypes.  For example, both are
  strided types, but with different strides.  Or both are indexed types, but
  with different offset arrays.  This does not support the general case,
  but it does support an important special case, such as the halo exchange 
  operation, without requiring an intermediate contiguous buffer.  

  We may also want to restrict this to strided types, instead of sametype.
  My reason for picking sametype is that this allows slighly more complex 
  datatypes, such as a 2-d face of a 3-d cube.

  Do we want 'Put_contig_to_general' and 'Put_general_to_contig' so we can 
  implement the general case?

  Module:
  Communication
  @*/
int MPID_Put_sametype( const void *origin_buf, int n, MPID_Datatype *dtype,
		       MPI_Aint target_offset, int target_rank, MPID_Comm *comm,
		       volatile int *local_flag, MPI_Aint target_flag )
{}

/* @
  MPID_Flags_testall - Test for the completion of flags specified with 
  MPID_Put_contig, MPID_Rhcv, or similar routine.

  Input Parameters:
+ count - Number of flags to wait on
- flag_ptrs - Array of pointers to flags.  See notes.

  Notes:
  See the discussion in 'MPID_Flags_waitall'.  

  Module:
  Communication
  @ */
int MPID_Flags_testall( int count, int *(flag_ptrs[]), int *found )
{
}

/* @
  MPID_Flags_waitsome - Wait for the completion of one or more flags.

  Input Parameters:
+ count - number of completion flags to wait on
- flag_ptrs - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.

  Notes:
  
  In a very rough sense, it corresponds to the Unix 'select', with a timeout
  of null (infinite).

  See Also:
  MPID_Flags_waitall, MPID_Flags_testall

  Module:
  MPID_CORE

  @ */
int MPID_Flags_waitsome( int count, int *(flag_ptrs[]) )
{
}

/* @
  MPID_Flags_testsome - Tests for the completion of one or more flags

  Input Parameters:
+ count - number of completion flags to wait on
- flag_ptrs - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.
  If no flag is completed, 0 is returned.

  Notes:
  
  In a very rough sense, it corresponds to the Unix 'select', with a timeout
  of zero.

  Module:
  MPID_CORE

  @ */
int MPID_Flags_testsome( int count, int *(flag_ptrs[]) )
{
}

/*
 * Should there be MPID_Win_lock, unlock, and fence?
 * For fence, the argument is that this is a simple and already
 * specialized operation; it is hard to see how a simpler operation
 * could be defined that would still make possible the collective
 * semantics of fence (particularly with the optional assertions)
 * Of course, fence need not be in 'MPID_CORE', since it can be implemented
 * by waiting for completion of outstanding put/get operations and
 * executing MPI_Barrier to ensure that all remote operations targeting
 * this process have also completed.
 *
 * Lock and unlock are more problematic; These look a lot like 
 * Rhcv calls.
 */

/* @
  MPID_Get_contig - Get data from a target window to a local buffer

  Input Parameters:

  Output Parameter:

  Notes:
  This is very similar to the 'MPI_Get' routine, restricted to
  datatype 'MPI_BYTE'.  The major differences are the completion
  flags 'local_flag' and 'target_flag', which behave just as for 
  'MPID_Put_contig'. 

  Module:
  Communication
  @*/
int MPID_Get_contig( void * origin_buf, int n, 
	      MPI_Aint target_offset, int target_rank, MPID_Comm *comm,
	      volatile void *local_flag, MPI_Aint target_flag )
{
}

/* @
  MPID_Get_sametype - Implement a get operation where the source and 
  destination datatypes are (roughly) the same

+ buf - Pointer to local memory to be to hold fetched data
. n   - Count of datatype to move
. dtype - Datatype describing both the source and destination datatype. 
  See notes.
. target_offset - Location of destination 
. target_rank - Rank of the destination process in the communicator
. comm - Communicator
. local_flag - Address of a flag to be set when this call is locally complete.
- target_flag - This is an id of a flag at the target process.  This value
  must have been specified by the target process in a previous communication
  (either with 'MPID_Put_contig' or 'MPID_Rhcv').  A value of '0' indicates no
  target completion flag.
  Notes:
  This routine allows the efficient implementation of strided copies, where
  both the origin and the target have the same strides.  

  See the discussion under 'MPID_Put_sametype'.

  Module:
  Communication
  @*/
int MPID_Get_sametype( void *origin_buf, int n, MPID_Datatype *dtype,
		       MPI_Aint target_offset, int target_rank, MPID_Comm *comm,
		       volatile int *local_flag, MPI_Aint target_flag )
{}

/* @
   MPID_Rhcv - Invoke a predefined handler on another process

  Input Parameters:
+ rank - Rank of process to invoke handler on
. comm - Communicator that rank is relative to
. id   - Id of handler
. vector - vector of 'struct iovec' elements containing information to
  be delivered to the remote handler. 
- count - Number of elements in 'vector'

  Output Parameter:
. local_flag - The location specified by this pointer is set to one
  when the operation has completed.

  Notes:
  The first element of 'vector' points to a predefined handler datatype that
  corresponds to the handler 'id'; the remaining elements are data that 
  the handler operation needs.  The handler 'id' can restrict the type of 
  memory that 'vector' may point at (e.g., it may require memory allocated 
  with 'MPID_Mem_alloc').

  The C struct type 'iovec' is defined in '<sys/uio.h>' and is the 
  same structure used by the Unix 'readv' and 'writev' routines.  It has the
  members
+ iov_base - Address of block
- iov_len  - Length of block in bytes.

  'MPID_Rhcv' is a nonblocking operation.  No buffer described by the 'vector'
  argument may be modified until the 'local_flag' is set.

  When used to invoke the predefined handlers, the first element (that is, 
  'vector[0]') specifies the handler structure.  For example, to issue a
  request-to-send operation, the code might look something like the following\:
.vb
  MPID_Hid_Request_to_send_t *ptr = <code to allocate this type>
  struct iovec vector[1];
  ...
  ptr->tag = tag;
  ptr->context_id = context_id;
  ...
  vector[0].iov_base = ptr;
  vector[0].iov_len  = sizeof(*ptr);
  ...
  err = MPID_Rhcv( rank, comm, MPID_Hid_Request_to_send, 
                   vector, 1, &local_flag );
.ve
  
  There is no requirement the that requested handler be invoked during
  the call to the routine.  In fact, this routine can be implemented,
  particularly in a TCP method, by simply sending a message to the
  designated process.  In fact, since this routine may locally complete
  (in the MPI sense), in the case where messages must be temporarily
  buffered at the sender (because of flow control limits or a full
  socket buffer), the message may not even have been sent by the time
  it returns.

  When this routine returns, all parameters may be reused.  For
  example, the 'vector' argument can be allocated off of the stack; if
  'MPID_Rhcv' hasn''t sent the message or invoked the handler by the
  time it returns, 'MPID_Rhcv' will make an internal copy of the
  contents of 'vector' (that is, the elements of the vector, not what
  they point at).  

  Module:
  MPID_CORE

  Discussion:
  Earlier drafts of ADI-3 contained the routine 'MPID_Rhc' as a special
  case of 'MPID_Rhcv'; this special case provided a slightly more 
  convenient interface for the case of a single block (e.g., for a 'count'
  of '1').  However, many operations would not use this special case.
  Here are 2 examples\:

  The Stream routines define a separate header which is used to support
  the generic implementation of the MPI collective operations.  To support
  this, a count of at least 2 will be needed, with 'vector[0]' pointing at
  the packet describing the stream operation and 'vector[1]' pointing at the
  separate header.

  An 'MPI_Put' with a (noncontiguous) datatype will needs
  a count of at least 3: 'vector[0]' points to the header that 
  describes the basic put operation (window and offset), 'vector[1]' points
  to a description of the datatype (if not cached at the destination), 
  and 'vector[2]' points to the actual data.

  In terms of implementation, for a put to a strided datatype using TCP,
  we''d want to send the header describing the destination, the datatype
  description, and the data.  In a shared-memory case, where the destination
  window is in shared-memory, this routine may run locally.  Alternately,
  or where the window is not in shared-memory, the shared-memory verison
  could move some of the data into a shared-memory destination location and
  enqueue an handler-activation request at the destination process.

  Threads and Polling:
  This routine is designed to allow (for most operations) either a 
  threaded or a polling implementation.  For each 'id' (i.e., operation
  type), the operation specifies whether a polling implementation is
  allowed.  Since each operation is defined by an id rather than a
  function pointer, the implementation of 'MPID_Rhcv' can use the 'id' 
  value to decide how to implement each operation.  In the simplest 
  implementation, 'MPID_Rhcv' could send a message to a thread running 
  in the destination process and that thread could execute a function 
  to process the request.  However, other implementations are possible.
  For example, based on the 'id' value, messages could be sent to a 
  polling routine or a thread agent.  In the simple TCP case, separate
  sockets could be used for actions requiring a thread and those that 
  can be handled by polling.  In an implementation with shared memory, 
  some operations can be handled directly without invoking any code at the 
  destination process (for example, a window lock could access the 
  window directly).
  
  Questions and Discussion:
  How many handler functions need be written to implement the core?
  What handler types are in the core?  

  Should there be a multisend version of this (one that can send the
  same data to multiple destinations)?

  Should this routine know how to recover memory (e.g., the request-to-send
  structure in the example above) when the operation completes?  Should it 
  return an indication that the message has been sent or not, allowing the 
  calling routine to figure out whether it needs to handle the non-blocking
  nature of the operation?
  @*/
int MPID_Rhcv( int rank, MPID_Comm *comm, MPID_Handler_id id, 
	       const struct iovec vector[], int count, int *local_flag )
{
}

/* 
 * Here go MPID versions of important point-to-point operations, just like the
 * full ADI-2. This list is expected to include:
 *
 * MPID_Waitsome
 * MPID_Testsome
 *
 * None of these is in 'MPID_CORE'.
 * As for the other routines in MPID, these only check for errors that are 
 * difficult to check for without performing the operation.
 * 
 * A special case is MPID_tBsend (see below)
 * 
 */

/* @
  MPID_Waitsome - MPID entry point for MPI_Waitsome

  Notes:
  The only difference between this and 'MPI_Waitsome' is that the basic
  error checks (e.g., request and count)
  have been made, and the MPI opaque objects have been replaced by
  MPID objects.

  Module:
  Communication
  @ */
int MPID_Waitsome( int incount, MPID_Request *(array_of_requests[]),
		   int *outcount, int array_of_indices[],
		   MPI_Status array_of_statuses[] )
{}

/* @
  MPID_Testsome - MPID entry point for MPI_Testsome

  Notes:
  The only difference between this and 'MPI_Testsome' is that the basic
  error checks (e.g., request and count)
  have been made, and the MPI opaque objects have been replaced by
  MPID objects.

  Module:
  Communication
  @ */
int MPID_Testsome( int incount, MPID_Request *(array_of_requests[]),
		   int *outcount, int array_of_indices[],
		   MPI_Status array_of_statuses[] )
{}


/*TSGOverview.tex 
 * Section 3: Communication Buffer management
 *
 * The routines in this section fill two roles.  In the implementation of
 * routines such as 'MPID_Isend' where the only device-specific routines are 
 * the few in 'MPID_CORE', it is necessary to have the ability to pack and 
 * unpack partial messages.  Consider the case where the user calls
 * 'MPI_Isend( buf, 1, my_huge_indexed_type, ... )'
 * where the total amount of data being sent is 100 MB.  Since 'MPID_CORE'
 * can only move contiguous data, we need to convert the described data
 * into a sequence of contiguous memory segments of some reasonable size
 * (we don''t want to have to allocate a 100 MB temporary buffer).
 * Thus, to implement this operation, we need a routine that can be called 
 * repeatedly to get the next m bytes from the user''s described data buffer.
 * Further, we need to be able to pause and remember where we are in processing
 * a datatype.  The MPI unpack routine does not have this flexibility.
 *
 * The other place where these routines are needed is in the implementation
 * of efficient versions of the MPI collective communication routines.  
 * These algorithms often need to look at the message as a range of bytes
 * from which segments are extracted and moved.  The implementation of the
 * MPI collective routines provided with MPICH will use these routines
 * express the algorithms.
 *
 * These routines also provide a way to specify a data area that may be
 * used in store-and-forward algorithms, without requiring copies to and
 * from an intermediate device buffer.  
 *
 * Note that 'MPID_Request's include a segment descriptoin within them.
 T*/

/* @
  MPID_Segment_init_pack - Get a segment ready for packing and sending (put,
  Rhcv, and/or stream) data

  Input Parameters:
+ buf   - Buffer to setup for sending
. count - Number of items
. dtype - Datatype of items
. comm  - Communicator for communication
- rank  - Rank of target.  Use 'MPI_ANY_SOURCE' for any member of the 
          communicator

  Output Parameter:
. segment - Segment descriptor.  

  Notes:
  Initializes a 'MPID_Segment' from a specified user buffer which is 
  described by '(buf,count,datatype)' and hence may represent noncontiguous
  data or contiguous data in a heterogeneous system.

  Module:
  Segment

  Questions: 
  Add an `intent` so that sending all of the data can use device 
  knowledge about complex data layouts?  E.g., 'intent == MPID_BUFFER_ALL'
  or 'intent == MPID_BUFFER_SEGMENT'?

  The original description contained this text\:

  If null, the original buffer is contiguous and can be used as is.

  If 'buf_desc' is non-null, the routine 'MPID_Segment_pack' must be used
  to fill a memory location or return a pointer to a contiguous buffer.

  However, the code is somewhat simplified if a segment is always used
  (in an case where a segment `may` be used).  
  
  @*/
int MPID_Segment_init_pack( const void *buf, int count, MPID_Datatype *dtype,
                            MPID_Comm *comm, int rank, 
                            MPID_Segment *segment )
{
}

/* @
  MPID_Segment_pack - Pack up the designated buffer with a range of bytes.

  Input Parameters:
+ segment - Segment descriptor (initialized with 'MPID_Segment_init_pack')
- send_buffer - Pointer to buffer to place data in.  May be 'NULL', in which 
  case 'MPID_Segment_pack' uses an internal buffer (and returns it).

  Inout Parameters:
+ first - Pointer to first byte index to be packed (on input) or actually
          packed (on output)
- last - Pointer to last byte index to be packed (on input) or actually
         packed (on output)

  Return value: 
  Pointer to memory containing packed data.

  Notes:
  
  Providing a send buffer allows implementations to make use of double 
  buffering without requiring that the segment code keep track of multiple 
  buffers.

  The values of 'first' and 'last' that are returned must satisfy the 
  following restrictions\:
+ 1 -  (last-first) on output must be less than or equal to (last-first) on 
  input.
. 2 -  (last+1) on output must have the property that when used as first on a 
  subsequent input, the value is unchanged.
- 3 - If first is zero on input, then first must be 0 on output.

  In addition, the `change` in first should be as small as possible.  
  For example, moving first so as to align the first byte on a word or 
  cache-line boundary (subject to the 3 requirements above) is allowed 
  (and is the reason to make first an in/out parameter).  However, as 
  rule three requires, if first is zero on input, no change to first is 
  allowed even for alignment purposes.  However, it is permissible to require
  that the buffers satisfy language- or processor-specified alignment rules.

  Module:
  Segment

  Questions:
  If 'MPID_Segment_pack' returns an internal buffer, who is responsible for
  freeing it?  It can''t be a single static buffer because that isn''t 
  thread-safe.  Is it freed as part of 'MPID_Segment_free'?

  If a send buffer is going to be provided, do we want to tell 
  'MPID_Segment_init_pack' so that no internal buffer is allocated?

  Should we return the number of bytes packed in a variable, just
  to simplify the use of the result (which, after all, is a pointer `and`
  a length)?
  @*/
void *MPID_Segment_pack( MPID_Segment *segment, int *first, int *last, 
			 void *send_buffer ) 
{
}

/* @
  MPID_Segment_init_unpack - Get a segment ready for receiving and unpacking 
  data 
  
  Input Parameters:
+ buf   - Buffer to setup for receiving
. maxcount - Number of items
. dtype - Datatype of items
. comm  - Communicator for communication
- rank  - Rank of source.  Use 'MPI_ANY_SOURCE' for any member of the 
          communicator

  Output Parameter:
. segment - Segment descriptor.  

  Module:
  Segment

  Questions:
  Should there be a flag indicating that the buffer may be used in a 
  put operation (as part of a receive and forward operation)?

  Do we need both an init pack and init unpack?
  @*/
void * MPID_Segment_init_unpack( const void *buf, int maxcount, 
				 MPID_Datatype *dtype,
                                 MPID_Comm *comm, int rank, 
                                 MPID_Segment *segment )
{
}

/* @
  MPID_Segment_unpack - Unpack the designated buffer with a range of bytes

  Input Parameters:
+ segment - Segment descriptor (initialized with 'MPID_Segment_init_unpack')
- recv_buffer - Pointer to buffer to place data in.  May be 'NULL', in which 
  case 'MPID_Segment_unpack' uses an internal buffer (and returns it).

  Inout Parameters:
+ first - Pointer to first byte index to be unpacked (on input) or actually
          unpacked (on output)
- last - Pointer to last byte index to be unpacked (on input) or actually
         unpacked (on output)

  Return value: 
  Pointer to memory containing unpacked data.

  Module:
  Segment
  @*/
void *MPID_Segment_unpack( MPID_Segment *segment, int *first, int *last, 
			   void *recv_buffer )
{
}

/* @
  MPID_Segment_free - Free a segment

  Input Parameter:
. segment - Segment to free

  Notes:
  Unlike the MPI routines to free objects, the MPID routines typically do not
  also set the object handle to 'NULL'.

  Question:
  Should we just use the generic object allocator and deallocator
  for segments ('MPIU_Handle_obj_destroy')?  Note that segments, like 
  requests, are performance-critical.

  Module:
  Segment

  @*/
int MPID_Segment_free( MPID_Segment *segment )
{
}

/* @
  MPID_Memory_register - Register a memory buffer 

  Input Parameters:
+ buf - Pointer to start of memory
. len - Length of buffer, in bytes
. comm - Communicator
. rank - rank 
- rdwt - Indicate whether memory will be read ('MPID_MEM_READ'), 
         written ('MPID_MEM_WRITE'), 
         or both ('MPID_MEM_READ' | 'MPID_MEM_WRITE')

  Return value:
  'MPI_SUCCESS' if registration was successful, a valid MPI error code
  otherwise.  This error is non-fatal; the error return is made to 
  allow for more detailed information on why the memory registration 
  failed.

 Notes: 
 This call is made by 'MPI_Send_init', 'MPI_Ssend_init', 'MPI_Rsend_init',
 'MPI_Bsend_init', and 'MPI_Recv_init' to mark memory
 as used for communication; other MPI routines may also call it.

 This call allows devices that can optimize transfers for predefined 
 memory to take advantage of these calls.

 The 'comm' and 'rank' arguments allow this routine to pick the best way
 to specify the memory based on how it will be used.  This may be important
 for devices that use different methods for interprocess communication based
 on the source or destination process.  The value 'MPI_ANY_SOURCE' is valid
 for 'rank' and indicates that the memory may be used when communicating with
 any process in 'comm'.

  It is possible and valid for the same memory region to be registered 
  several times.  It is not an error to attempt to register the same 
  memory; the return code should indicate success if the memory is
  registered, even if this particular call was not responsible for actually
  registering the memory.  

  Module:
  Communication
  @*/
int MPID_Memory_register( void *buf, int len, MPID_Comm *comm, int rank, 
			  int rdwt )
{
}
/* @
  MPID_Memory_unregister - Remote previous registration

  Input Parameters:
+ buf - Pointer to start of memory
. len - Length of buffer, in bytes
. comm - Communicator
. rank - rank
- rdwt - Indicate whether memory was registered to be read ('MPID_MEM_READ'), 
         written ('MPID_MEM_WRITE'), 
         or both ('MPID_MEM_READ' | 'MPID_MEM_WRITE').

  Return value:
  'MPI_SUCCESS' if deregistration was successful, a valid MPI error code
  otherwise.  This error is non-fatal; the error return is made to 
  allow for more detailed information on why the memory deregistration 
  failed.

  Notes:
  The arguments must be exactly the same as those used to register memory.

  It is valid to call this routine as many times as 'MPID_Memory_register'
  returned success on the calling process.  If the same memory location is
  registered multiple times, then only when 'MPID_Memory_unregister' is called
  the `same` number of times should the memory be deregistered with the 
  operating system.  
  'MPID_Memory_unregister' thus has the same reference-count
  semantics as other MPI routines.  Note that this may require implementing
  a reference count mechanism for registered memory.

  Question:
  How do routines that free requests (such as 'MPI_Wait' and 
  'MPI_Request_free') know to call this routine?  
  Is there a registered memory pointer?  Do we need to keep track of 
  what type of memory (user-provide, registered, mpich-provided) is used
  in different operations?

  Module:
  Communication
  @*/
int MPID_Memory_unregister( void *buf, int len, MPID_Comm *comm, int rank,
			    int rdwt )
{
}

/* @
  MPID_Memory_isregistered - Indicate whether the specified memory region is 
  currently registered

  Input Parameters:
+ buf - Pointer to start of memory
- len - Length of buffer, in bytes

  Question:
  Do we also need to know the communicator or process group?

  @*/
int MPID_Memory_isregistered( void *buf, int len )
{
}

/*
 * Section 4: Store, Process, and Forward Communication
 */

/* TStmOverview.tex
 *
 * Algorithms for collective communication are often of the
 * store and forward or store and scatter (to one or more processes); 
 * collective computation (such 
 * as reduce or scan) adds a processing step before forwarding.
 * This section describes the routines needed for these operations.
 * They rely on the 'MPID_Segment' to provide an interface between the 
 * general MPI datatypes and the contiguous byte array form in which 
 * most store and forward operations are described.
 *
 * Because the collective routines are all blocking, these routines can
 * use a simplified strategy for initiating the "next" block in a store
 * and forward pipeline.
 *
 * Another difference between these and the point-to-point routines is
 * that we may need more information that just the "tag" that point-to-point
 * provides.  Thus, at the beginning of a store-and-forward operation,
 * a small amount of additional data may be sent.
 *
 * Streams are actually delivered in blocks; as each block is
 * delivered, the application has the option to process and/or forward
 * the block to another process (or processes).  The block size is
 * determined by the device.  The 'MPID_Stream_iforward' routine allows a 
 * code to receive the data into the local destination buffer according to
 * the specified (possibly noncontiguous) datatype while forwarding the
 * block.  This avoids the unpack/pack cycle that is required when only
 * send/receive routines are used.
 *
 * Notes on xfer versus stream.  The original stream interface allowed the 
 * programmer to explicitly describe the steps used to process each section
 * (or segment) of the stream.  This gave the programmer a great deal of 
 * control and flexibility over the handling of each part of a stream.  
 * However, it also made it very difficult for the device to efficiently 
 * handle a stream transfer, particularly for non-polling devices.  The xfer
 * approach essentially builds a simple data transfer program that is then
 * executed by the device.  This is not as flexible as the stream interface,
 * but the device may be able to more efficiently implement xfer.
 *
 * Note to Implementors:
 * In determining the block size, you cannot look at the datatype,
 * since the datatypes used by the sender and the receiver may be
 * different.  For example, even though the sender has a contiguous
 * buffer and thus could send a large block without allocating memory
 * or copying data, the receiver may have specified a non-contiguous 
 * data buffer (in the segment definition), thus limiting the size of
 * the block for receiving.
 *
 * An implementation should also consider at least double buffering
 * the communication of a stream.  In other words, once one block is
 * delivered, allowing 'MPID_Stream_wait' to return with that block,
 * begin delivering the next block into a separate buffer.  Where it
 * makes sense, if there is storage for the entire message, an
 * implementation may choose to deliver the entire message as quickly
 * as possible, updating the 'stream->cur_length' as data is delivered.
 * This points out that while the stream routines discuss motion in blocks,
 * there is no particular limit to the number of blocks that are delivered
 * each time.
 *
 * Questions:
 * 1. Should a 'MPID_Stream' be an 'MPID_Request'?  That would allow us
 * to use the same completion routines, and to mix stream and non-stream
 * communication.  The current choice was made to make the stream module
 * independent of the other modules, and exploits the fact that these are
 * intended for implementing the collective operations, all of which are
 * blocking (except for the two-phase collective in MPI-IO).
 * Note that if we do make these requests, then the description of 
 * 'MPID_Waitsome' etc. will become more complex; we may need to provide
 * a routine to be called by the 'MPID_Waitsome' in that case.  Viewed in
 * that light, streams are similar to persistant, user-defined MPI requests.
 *
 * 2. Should there be a test as well as a wait on a stream?  If a stream
 * is a kind of request, then we get this automatically.
 *
 * 3. The examples outlined here rely on explicit advancement of the stream
 * by the process the initiates it.  Another approach is to allow the 
 * communication agent to send each piece of the stream as it becomes possible.
 *
 * 4. Should there be a one-sided stream operation?  E.g., just as we have 
 * streams as a generalization of message-passing, do we want the same thing 
 * for RMA operations?
 T*/

/* @
  MPID_Stream_isend - Initialize a stream and begin sending.

  Input Parameters:
+ segment - The buffer to be sent, defined by a segment type.
. header  - An optional pointer to additional data to send.  If provided,
            the stream receivers must provide a matching buffer
. header_size - Number of bytes in 'header' to send.  Use zero if 'header' is 
  null.
. tag - Message tag.
. rank - Rank of destination process
. comm - Communicator to send stream in
- first,last - recommended byte range from 'segment' to send first.
  See notes for details.
 
  Output Parameter:
. stream - Pointer to a stream object.  This is similar to an
  'MPID_Request', but contains information needed to process streams.

  Notes:

  This communication defines a `stream` consisting of several
  communication steps.  The data sent by this routine may only be
  received with 'MPID_Stream_irecv'; this contrasts with other MPI
  point-to-point communication calls.

  Module:
  Stream

  Question:
  Can we modify the stream buffer?  E.g., for allreduce or scan?  
  See 'MPID_Stream_iforward' below.

  We may sometimes want to send the stream starting from the last byte
  rather than the first byte, particularly in cases where we will be
  forwarding.

  See the discussion on 'MPID_Stream_wait'.  Do we want to also define
  an 'MPID_Stream_send'?  How often will we want the nonblocking version?

  In many cases, we may want the device to start the next block when it
  can.  For example, if we use this to implement 'MPID_Isend'.  How do
  we indicate whether that should happen, or only the designated part of the
  segment should be sent before 'MPID_Stream_wait' is called?
  @ */
int MPID_Stream_isend( MPID_Segment *segment, 		       
		       void *header, int header_size,
		       int tag, int rank, MPID_Comm *comm, 
		       int first, int last, MPID_Stream **stream )
{
}

/* @
  MPID_Stream_irecv - Initialize a stream for receiving

  Input Parameters:
+ segment - The buffer to be sent, defined by a segment type.
. offset  - offset of the segment within the stream.  If the segment
            describes the entire stream, use zero for the offset
. header  - An optional pointer to additional data to send.  If provided,
            the stream receivers must provide a matching buffer
. header_size - Number of bytes in 'header' to send.  Use zero if 'header' is 
  null.
. tag - Message tag.
. rank - Rank of destination process
. comm - Communicator to send stream in
- will_forward - Set to true if the stream will be forwarded to other
  processes, otherwise set it to false.   See 'MPID_Stream_iforward'.
 
  Output Parameter:
. stream - Pointer to a stream object.  This is similar to an
  'MPID_Request', but contains information needed to process streams.

  Notes:
  Because of the store and forward semantics, each receiver of a
  stream may receive more data than will locally be stored; the rest
  of the data is forwarded.  This is why the location of the segment
  within the stream must be specified.

  Module:
  Stream

  Question:
  Where do I find the delivered header size?  

  Should 'will_forward' be an enum value?

  Do I allow 'MPI_ANY_SOURCE' or 'MPI_ANY_TAG'?  I''d like not to, but
  I need them for 'MPID_Irecv'.

  If I do `not` allow 'MPI_ANY_TAG', do we want to make tag mismatch
  an error?  That would happen if there was a collective operation
  mismatch on the communicator.

  @ */
int MPID_Stream_irecv( MPID_Segment *segment, int offset, 
		       void *header, int max_header_size, int tag, int rank, 
		       MPID_Comm *comm, int will_forward, 
		       MPID_Stream **stream )
{
}

/* @
  MPID_Stream_wait - Wait for a stream 
  
  Input Parameter:
. stream - Stream to wait on.  See notes.

  Return value:
  Stream status; returns 0 normally, -1 when the end of the stream is 
  reached, and an MPI error code for other errors.  The end of stream 
  value is set when the last part of the stream has been received.

  Notes:  
  The semantics of 'MPID_Stream_wait' are very similar to those for
  'MPI_Wait'.  In particular, when 'MPID_Stream_wait' returns, the
  buffer is ready for use.  The difference from 'MPI_Wait' is in the
  definition of the buffer; in the case of 'MPID_Stream_wait', it
  refers to the current data block.  

  Rather than use a separate 'status' variable, the length of the data
  (in bytes) is part of the stream structure, and can be accessed as
  'stream->cur_length'.  The address of the buffer containing the
  stream is 'stream->cur_ptr'.

  A classic "store and forward" communication could be written as follows\:
.vb
    MPID_Segment_init_unpack( buf, count, datatype, comm, rank,
                              &segment );
    MPID_Stream_irecv( segment, NULL, 0, tag, from_rank, comm, 1, 
                       &stream );
    first = 0;		       
    do {
        eos = MPID_Stream_wait( stream );
        if (eos > 0) error();
        MPID_Stream_iforward( stream, NULL, 0, tag, 
                              &to_rank, 1, comm	);
        last = first + stream->cur_length;		      
        MPID_Segment_unpack( segment, &first, &last, stream->cur_ptr );
        first = last + 1;
    } while( eos == 0 );
    MPID_Segment_free( segment );
.ve
  Once 'MPID_Stream_wait' returns '1', the stream is freed and the
  pointer becomes invalid.

  For a sending stream (e.g., one created with 'MPID_Stream_isend',
  'MPID_Stream_wait' has the same semantics: it returns when the
  current chunck has been sent, and that part of the stream is
  (logically) available for reuse.

  Module:
  Stream

  Question:
  Since wait frees the stream, do we want it to set the pointer to
  null, i.e., use 'MPID_Stream **stream' instead?
  @ */
int MPID_Stream_wait( MPID_Stream *stream )
{
}	       

/* @
  MPID_Stream_iforward - Forward a stream

  Input Parameters:
+ stream - A stream created with 'MPID_Stream_irecv'
. header  - An optional pointer to additional data to send.  If provided,
            the stream receivers must provide a matching buffer
. header_size - Number of bytes in 'header' to send.  Use zero if 'header' is 
  null.
. tag - Message tag.
. ranks - Array of ranks of destination processes
. nranks - Number of processes in 'ranks'
- comm - Communicator to send stream in

  Notes:
  This routine describes a common operation in collective communication and
  computation and it allows the device to optimize for this case.  For example,
  the data that was received (with 'MPID_Stream_irecv') can be left in a 
  location and format convienent for further communication.   In a 
  shared-memory device, the message could be left in shared memory, allowing it
  to be sent to other destinations without making another copy of the data.
  High-performance interconnects with on-board memory can leave the data 
  in the interconnect.  Heterogeneous systems can avoid data conversion 
  on the buffer to forward.

  Module:
  Stream

  Note:
  There is no output stream because this is a forward of an existing 
  stream. 
  @ */
int MPID_Stream_iforward( MPID_Stream *stream, void *header, 
			  int header_size, int tag, int ranks[],
			  int nranks, MPID_Comm *comm )
{
}

/*
 * Section : Topology
 */


/*
 * Section : Miscellaneous
 */


/*
? Information on data representations for heterogeneous code.

? We probably need both compile-time and run-time information on whether
the system is homogeneous or not (note that in an MPI-2 setting, the run-time
value might be "not yet heterogeneous").

? Information on "good" message break points? (e.g., short/eager/rendezous)?
? This is useful for the "generic" implementations of Isend etc., based on
? MPID_Put_contig and MPID_Rhcv.
*/

/* T
 * Section : Service Routines
 *
 * Many of these may be implemented (as macros) by their underlying routine
 * However, in some cases, it may be advantageous to use special routines.
 * Alternatives can include routines that trace or record operations.
 T */


/* @
  MPIU_Memcpy - Copy memory

  Input Parmeters:
+ src - Pointer to memory to copy
- n   - Number of bytes to copy

  Output Parameters:
. dest - Pointer to destination to copy 'src' into.

  Return value:
  Also 'dest'.

  Notes:
  'MPID_Memcpy' is included because on some systems, faster memory copies
  can be written than are provided by the system libraries.  For example,
  system routines may be prohibited from using wide load and store operations,
  while user-code and other libraries (such as MPICH) are allowed to use
  these faster instructions.  On systems where the 'memcpy' routine is 
  efficient, this routine may be defined as
.vb
 #define MPID_Memcpy( dest, src, n ) memcpy( dest, src, n )
.ve

  Module:
  Utility
  @*/
void *MPIU_Memcpy( void *dest, const void *src, size_t n )
{
}


/* @
  MPIU_Trdump - Provide information on memory that is allocated

  Input Parameters:
. file - File opened for writing onto which the output should be sent.

  Module:
  Utility

  Question:
  Should this have a different output model to allow for a windows-style
  interface?  Should it take an output function and void pointer for
  extra data.
  @*/
void MPIU_Trdump( FILE *file )
{}
/*
 * Information about the device and environment 
 */

/*
 * Timers
 */


/* @
  MPID_Wtime_finalize - Shut down the timer

  Note:
  This routine should perform any steps needed to shutdown the timer.
  In most cases, this routine is a no-op, but it is provided for those 
  cases where the timer must allocate (and hence free) an operating-system 
  managed resource.

  Module:
  Timer

  See Also:
  'MPID_Wtime_init'
  @ */
void MPID_Wtime_init( void )
{}


/* @
  MPID_Gwtime_init - Initialize a global timer
  
  Input Parameters:
. comm - Communicator to initialize global timer over

  Return Value:
  '1' for success and '0' for failure.  

  Notes:
  This is a collective call over 'comm'.  This allows this routine to
  make communication calls in order to synthesize the global timer.
  In an MPI-1 environment, 'comm' would just be the 'MPID_Comm'
  specified by 'MPI_COMM_WORLD'; this routine would normally be called
  from within the implementation of 'MPI_Init_thread' (not
  'MPID_Init').  In an MPI-2 setting, this routine may be
  called within 'MPI_Comm_spawn' and friends to establish a global
  timer even when new processes are added to the MPI job.

  Module:
  Timer
  @ */
int MPID_Gwtime_init( MPID_Comm *comm )
{
}

/* @
  MPID_Gwtime_diff - Compute the difference between two time stamps from
  different processes

  Input Parameters:
+ t1 - A time stamp set by 'MPID_Wtime' on this process.
. t2 - A time `value` set with 'MPID_Gwtime' on process 'pid2'
- pid2 - Process number corresponding to 't2'.
 
  Output Parameter:
. diff - The different in time between t2 and t1, measured in seconds.

  Notes:
  This routine provides access to a global timer for all MPI processes.
  If the attribute 'MPI_WTIME_IS_GLOBAL' is true, then it is the same
  as 'MPID_Wtime_diff'.  

  If no global timer is available, it is possible to synthesize one.
  Such an implementation is provided as part of the utilities 
  library with the MPICH MPID implementation.  Note that
  the resolution of this timer (at least between processes) may be 
  significantly lower than for the local timer.

  Module:
  Timer

  Rationale:
  Time stamps are local to the process that generated them.  This routine
  allows time stamps on different processes to be compared.  

  In a heterogeneous system, the interpretation of the 'MPID_Wtime_t' value 
  may depend on the process that set the value.  This is why the value from
  the remote process ('t2') is converted first into a double with 
  'MPID_Gwtime'.  As a 'double', the value can be sent with the usual MPI
  communication.

  In a system made up of a cluster of SMPs, the timestamps on a single SMP
  may in fact be directly comparible without any adjustments, while 
  timestamps from different nodes may require adjustments.  The design
  of this routine allows full accuracy.

  Question:
  Do we really want this or just a routine to convert to a common time?

  @ */
void MPID_Gwtime_diff( MPID_Wtime_t *t1, 
		       double t2, int pid2, double *diff )
{
}

/* @
  MPID_Gwtick - Return an upper bound on the resolution of the global timer

  Input Parameters:
. pid1,pid2 - Process numbers.  See "return value".

  Return value:
  An upper bound on the resolution of the 'MPID_Gwtime_diff' routine for
  process ids 'pid1' and 'pid2'.  If the attribute 'MPI_WTIME_IS_GLOBAL' is
  true and either 'pid1' or 'pid2' are negative, then this
  returns the same value as 'MPID_Wtick'. 

  It is permissible for this routine to return a conservative bound; for 
  example, it may return the same value for all input parameters.
  Some implementations may return a tighter estimate when one of the
  process numbers is the same as the calling process.

  If either process id is negative, then this returns a value that gives
  an upper bound of the clock resolution of all processes.

  Module:
  Timer

  Question:
  Should this only have one pid, and always refer to the resolution
  between the calling process and the designated process?

  @ */
double MPID_Gwtick( int pid1, int pid2 )
{
}

/*
 * Lists (attributes)
 *
 * Each communicator (and in MPI-2, datatypes and windows) has a list of 
 * attributes (a list in the general sense; no specific data structure is
 * specified by MPI).  With each attribute is a key value. 
 * To maintain flexibility in representing the attributes, we do not 
 * specify the data structure used but instead specify only the methods
 * used to access, modify, and copy the list.  While the default implementation
 * uses a simple ordered list to represent the collection of attributes 
 * (because few if any programs use a significant number of attributes),
 * more sophisticated approaches can be used (in MPICH-1, an HB tree was
 * used).
 *
 * Note that for a multithreaded ('MPI_THREAD_MULTIPLE') implementation,
 * these routines and/or their use require more care because of 
 * possible race conditions.  For example, if one thread is retrieving an 
 * attribute value while another is deleting the attribute, a consistent 
 * state must be preserved.
 *
 * Question:
 * One model is to provide only atomic operations.  Then we need
 * attr get and put, rather than a single find.  An alternative is
 * a lock on the list or on the communicator.  If a lock on the list is
 * used, any step that frees the communicator must also acquire a lock
 * on the list.  
 *
 * Note that we may want to lock communicators as well, so we may 
 * want to use the communicator lock instead.
 * But what happens if we free a communicator while another thread is
 * waiting for the lock?  That is not a correct program, but can we
 * set things up so that the error can be detected?  For example,
 * once in the lock, check that the communicator is still valid, and 
 * arrange things so that a comm_free/comm_create won't reuse the same
 * communicator right away?  
 *
 * With a lock, it is also imperative that the lock be released even if the
 * attribute functions return an error; in a perfect world, they'd also
 * release the lock when throwing an exception.  This may be too hard to
 * handle, at least from C (because there is no way to force the lock to
 * be released if a signal such as SIGKILL is made).
 *
 * Grumble.  Note that this requires thread locks for datatypes and windows!
 *
 */

/* @
  MPID_Attr_find - Find and optionally insert an attribute for a given keyval

  Input Parameters:
+ list - Attribute list to search
. keyval - keyval to look for
- insert - True if attribute should be inserted if it is not found.

  Return Value:
  A pointer to the matching attribute entry, or null if no attribute matching 
  the keyval was found.

  Notes:
  This routine should only be used in a context where no other thread
  may modify the attribute list of the underlying object.  It may be
  necessary to use 'MPID_Thread_lock' and 'MPID_Thread_unlock'
  with the per-process 'common_lock' around uses of this routine.

  Using a value of 0 for 'insert' makes it easy to implement 'MPI_Attr_get';
  a value of 1 is used for 'MPI_Attr_put' (and the MPI-2 versions of
  these).

  Module:
  Attribute
  @ */
MPID_Attribute *MPID_Attr_find( MPID_List *list, int keyval, int insert )
{
}

/* If we use the attr-find model, we also need a routine to get all of the
   attributes in a list, so that we can implement Comm_dup and Type_dup
*/

/* @
  MPID_Attr_list_walk - Walk through a list of attributes

  Input Parameters:
+ list - Attribute list to walk through
- prev - Previous attribute returned by this routine, or null.

  Notes:
  This routine returns each element of an attribute list in turn.
  The first call should use 'NULL' for the value of 'prev'.  

  Just as for 'MPID_Attr_find', this routine should be called in a 
  thread-safe fashion, most likely using 'MPID_xxx_thread_lock' and
  'MPID_xxx_thread_unlock' (where 'xxx' denotes 'Comm', 'Datatype', or 'Win').

  This function is needed for the object duplicate functions (e.g., 
  'MPI_Comm_dup' and 'MPI_Type_dup'), as well as for the object free functions
  (e.g., 'MPI_Comm_free', 'MPI_Type_free', and 'MPI_Win_free').

  Module:
  Attribute
  @ */
MPID_Attribute *MPID_Attr_list_walk( MPID_List *list, MPID_Attribute *prev )
{
}

/* @
  MPID_Attr_delete - Remote an attribute from a list

  Input Parameters:
+ list - Attribute list to search
- keyval - keyval to look for
 
  Output Parameter:
. flag - true if the attribute was found, false otherwise.

  Return value:
  The attribute value (not the 'MPID_Attribute' entry, the user''s attribute
  value.  Note that the attribute entry has been deleted from 
  the list but the attribute itself is property of the user.  This allows us
  to invoke the appropriate attribute delete function, if one is defined for 
  this keyval.  If the attribute was not found, return null.  However, since 
  null is also a valid value, a return flag is used.

  Note that there are no error conditions possible (other that internal
  corruption in the list datastructure).

  Notes: 
  We return the attribute value instead of the flag indicating that the 
  attribute was found because returning the pointer to the void pointer 
  through the argumnet list is ugly in C.
  
  Module:
  Attribute
  @ */
void *MPID_Attr_delete( MPID_List *list, int keyval )
{
}

/* @
  MPIU_Info_create - Create a new MPID_Info element

  Returns:
  Pointer to an 'MPID_Info' structure.  The 'id' field (used as the handle)
  of this structure is already set.

  Notes:
  This routine handles all of the details of allocating storage for 'MPID_Info'
  structures.  It initializes itself on first use and uses a 'MPI_Finalize' 
  handler to free any allocated storage when MPI exits.  This helps maintain
  smaller and faster MPI executables for those applications that only use
  MPI-1 routines.
  @ */
MPID_Info *MPIU_Info_create( void )
{}

/* @
  MPIU_Info_destroy - Free an info structure and everything that it contains
 
  Input Parameters:
. info_ptr - Pointer to an info list

  Notes:
  Frees all members of an info object.  In a multithreaded environment,
  it ensures that 'MPID_Info' storage is reclaimed in a thread-safe fashion.

  Module:
  Info
 @*/
void MPID_Info_destroy( MPID_Info *info_ptr )
{}

/* @
  MPIU_Info_destroy - Reclaim a single info element

  Input Parameter:
. info_ptr - Element to reclaim

  Notes:
  Only the info element itself is reclaimed.  The string storage for the
  'key' and 'value' elements should be freed before calling this routine.
  In a multi-threaded environment, this frees info storage in a thread-safe
  manner.
  @ */
void MPIU_Info_destroy( MPID_Info *info_ptr )
{}

/* T
 * Section : Environment
 *
 * These are the global defines and variables that other modules may
 * refer to.  
 *
  T*/

/* D
  MPID_THREAD_LEVEL - Indicates the level of thread support provided
 
  Values:
  Any of the 'MPI_THREAD_xxx' values (these are preprocessor-time constants)

  Notes:
  This variable allows implementations that support
  'MPI_THREAD_MULTIPLE' to bypass thread locks when the user has
  requested (and received) a lower level of thread support.

  The macro 'MPID_MAX_THREAD_LEVEL' defines the maximum level of
  thread support provided, and may be used at compile time to remove
  thread locks and other code needed only in a multithreaded environment.

  Remark:
  Superceeded by the 'thread_provided' field in 'MPIR_Process'.

  Module:
  Environment-DS
  D */
extern int MPID_THREAD_LEVEL;

/* TDyOverview.tex
 * Dynamic Processes:
 *
 * The challenge here is to define a core set of routines that aren't
 * too complicated to implement.  Unfortunately, these are at the core of
 * the device-specific operations.  The only function that the MPI level
 * can really provide is the construction of the intercommunicator from 
 * a list of local process id's.
 *
 T*/
/* @
  MPID_Comm_spawn_multiple - Spawm and connect to new processes

  Module:
  Dynamic
  @ */
int MPID_Comm_spawn_multiple( int count, const char *commands[],
			      const char *argv_p[][], 
			      const int max_procs[],
			      MPID_Info *info[],
			      int root, MPID_Comm *comm, 
			      MPID_Lpid lpids[], int err_codes[] )
{
}

/* @
  MPID_Port_open - Open a port for accepting connections from MPI processes

  Module:
  Dynamic
  @ */
int MPID_Port_open( MPID_Info *info, const char *port_name )
{
}

/* @
  MPID_Port_close - Close a port

  Module:
  Dynamic
  @ */
int MPID_Port_close( const char *port_name )
{
}

/* @
  MPID_Comm_accept - Accept a connection from an MPI process

  Module:
  Dynamic

  @ */
int MPID_Comm_accept( const char *port_name, MPID_Info *info, int root,
		      MPID_Comm *comm, MPID_Lpid (*lpids)[] )
{
}

/* @
  MPID_Comm_connect - Connect to an MPI process

  Module:
  Dynamic

  @ */
int MPID_Comm_connect( const char *port_name, MPID_Info *info, int root,
		       MPID_Comm *comm, MPID_Lpid (*lpids)[] )
{
}

/* @
  MPID_Comm_disconnect - Disconnect from MPI processes

  Module:
  Dynamic

  @ */
int MPID_Comm_disconnect( MPID_Comm *comm )
{
}

/* @
   MPID_Comm_get_contextid - Return the internal context id used in the
   communicator

  Input Parameter:
. comm - Communicator to get context value for.

  Return value:
  Context id used in this communicator

  Notes:
  This routine makes the context id available to other tools.  For example,
  having a global context id makes message matching for tools like Jumpshot 
  `much` easier.  In fact, the ADI could provide a function to provide a 
  unique context id, which most implementations would implement directly from 
  the given id.  Note that this is a unique, not a globally unique, id, since 
  the value needs only be unique relative to the processes that can share in 
  communication.  In other words, only the tuple (source/destination, 
  context_id) must identify a unique communicator at each MPI process.

@ */
int MPID_Comm_get_contextid( MPID_Comm * )

/*
 * Error Reporting
 */

/* @
  MPID_Err_set_msg - Change the message for an error code or class

  Input Parameter:
+ code - Error code or class
- msg  - New message to use

  Notes:
  This routine is needed to implement 'MPI_Add_error_string'
 
  Module:
  Error
@*/
int MPID_Err_set_msg( int code, const char *msg_string )
{}

/* @
  MPID_Err_add_class - Add an error class with an associated string

  Input Parameters:
+ msg_string - Message text for this class.  A null string may be used, in
  which case 'MPID_Err_set_msg' should be called later.
- instance_msg_string - Instance-specific message string.  See the error 
  reporting overview.  May be the null string.

  Return value:
  An error class.

  Notes:
  This is used to implement 'MPI_Add_error_class'; it may also be used by a 
  device to add device-specific error classes.  Unlike the MPI version, this 
  combines returning a class with setting the associated message.  

  Predefined classes are handled directly; this routine is not used to 
  initialize the predefined MPI error classes.  This is done to reduce the
  number of steps that must be executed when starting an MPI program.

  Module:
  Error
  @*/
int MPID_Err_add_class( const char *msg_string, 
			const char *instance_msg_string )
{}

/* @
  MPID_Err_add_code - Add an error code with an associated string

  Input Parameters:
+ class - Class that the code belongs to
. msg_string - Message text for this code
- instance_msg_string - Instance-specific message string.  See the error 
  reporting overview.  May be the null string.

  Return value:
  An error code.

  Notes:
  This is used to implement 'MPI_Add_error_code'; it may also be used by a 
  device to add device-specific error codes.  Unlike the MPI version, this 
  combines returning a code with setting the associated message.  
 
  Module:
  Error
  @*/
int MPID_Err_add_code( int class, const char *msg_string, 
		       const char *instance_msg_string )
{}

/* @
  MPID_Err_delete_code - Delete an error code and its associated string

  Input Parameter:
. code - Code to delete.
 
  Notes:
  This routine is not needed to implement any MPI routine (there are no
  routines for deleting error codes or classes in MPI-2), but it is 
  included both for completeness and to remind the implementation to 
  carefully manage the memory used for dynamically create error codes and
  classes.

  Module:
  Error
  @*/
void MPID_Err_delete_code( int code )
{}

/* @
  MPID_Err_delete_class - Delete an error class and its associated string

  Input Parameter:
. class - Class to delete.
 
  Module:
  Error
  @*/
void MPID_Err_delete_class( int class )
{}

int MPID_Err_get_string( int code, char *msg, int msg_len )
{}

/*D
  errmsgcat - Generate the message catalog from the source files

  Usage:
  errmsgcat [ -append ] [ -msgcat filename ] [ -ignore filename ] 
  [ -basefile filename ] [ -deffile filename ] [ -validate ] 
  [ -msglenfile filename ] path

  Command line options:
+ -append - Add entries to the message catalog.  Without this option,
  a new file is created
. -msgcat filename - Specify the name of the message catalog.  If no name is
  specified, 'msg.cat' is used.
. -deffile filename - Specify the name of a file to contain the messages as 
  a C array.  This file may be included in an application to ensure that
  error messages can be generated even if the error message catalog cannot be
  found or read.  By default, the name 'msgcat.h' is used.  If no file is
  desired, use '/dev/null'.
. -ignore filename - Specify a file containing a list of files or directories
  to skip.  The names must not contain a '/' .
.  -basefile filename - Specifiy a file containing predefined messages.  See 
  notes.
.  -validate - If this is specified, the message catalog is validated by 
  ensuring that all references to a predefined message are matched by 
  a message definition and that the instance-specific messages are supplied 
  with the correct number of arguments in the 'MPID_Err_create_code' call.
. -msglenfile filename - Filename to create containing the maximum length 
  of any error message.  This file contains a line of the form
.vb
  #define MAX_ERROR_MSG 231
.ve
  If no filename is provided, 'msgcatlen.h' is used.  If no file is desired,
  specify '/dev/null'
- path - File or directory to search for messages.  If 'path' is a directory,
 all files in the directory and any subdirectories are searched, except for
 any files specified using the '-ignore' option.
  
  Notes:
  This is the program that reads source files and creates a message catalog 
  from the source file.  It searches for 'MPID_Err_create_code' and extracts 
  the class value and messages and creates a message catalog file 'msg.cat',
  along with files used to allow an application to generate error messages
  even if the message catalog file cannot be found.

  The basefile allows the use of standardized messages.  This should contain
  lines of the form
.vb
  **shortname
  long version of message \
  using backslashes at the end of\
  the line to continue
.ve
  The form '**shortname' is used in 'MPID_Err_create_code' calls.
 
  The 'msgcatlen.h' file is provided to allow a tool to define an appropriate
  maximum message length or to check that a specified maximum message length
  is in fact sufficient.  The value in this file can be used to 
  specify 'MPI_MAX_ERROR_STRING'.

  The computation of the maximum message length requires that any formatting
  command contain a maximum field length.  If no lengths are provided, the
  following defaults will be used\:
.vb
  %d   10
  %s   32
  %ld  16
  %x   8
  %lx  16
.ve
  These values must also be enforced by the implementation of 
  'MPID_Err_get_string'.  For example, the standard '%.32s' can be used to 
  control the length of '%s' formatting.  For the other controls, we may
  need to introduce a non-standard syntax.

  Module:
  Error
  D*/

/* @
  MPID_Err_link - Indicate an error on a communication link

  Input Parameters:
+ comm - Communicator on which failure was discovered.  If none, use
  'MPI_COMM_NULL'
. rank - Rank in 'comm' of failed link.  Ignored if 'comm' is 'MPI_COMM_NULL'.
. lpid - Local process id of failed link.  Required.
- is_fatal - True if the error is not recoverable, false otherwise.  See notes.

  Notes:
  This routine should be called by the ADI when it discovers that a 
  communication link to another process has failed in a non-recoverable way.
  This routine is used to provide a consistent hook for supporting 
  fault-tolerance in an MPI implementation.  

  The value 'is_fatal' is used to indicate whether the ADI is able to recover 
  from this error.  If 'is_fatal' is true, 'MPID_Err_link' should not return 
  but should invoke 'MPID_Abort'.

  Question:
  If 'is_fatal' is true, how do we pass error messages back to the 
  environment?  Should we pass an error code to this routine?  Should it 
  return instead, allowing the routine that called it to call abort?
  Is the rationale for calling it that it may want to help take down 
  other processes (or at least notify them of failure)?

  Module:
  Error-handling
  @*/
int MPID_Err_link( MPID_Comm *comm, int rank, MPID_Lpid lpid, 
		   int is_fatal )
{}

/* @
  MPID_Err_partner - Indicate an error has been detected on a member of 
  a communicator.

  Input Parameters:
+ comm - Communicator on which failure was discovered.
. rank - Rank in 'comm' of a process with a failed link. 
- is_fatal - True if the error is not recoverable, false otherwise.  See notes.

  Notes:
  This routine is called when the ADI discovers that a communicator has become
  invalid because some process in the communicator has lost contact with 
  another process in the communicator.  Typically, the process will receive
  this information through a remote handler invocation ('MPID_Rhcv') from
  another process.

  Module:
  Error-handling

  See also:
  MPID_Err_link

  @*/
int MPID_Err_partner( MPID_Comm *comm, int rank, int is_fatal )
{}

/* @
  MPICH_Quiet - Call a user-specified function when a communicator is
  quiet

  Input Parameters:
+ comm - Pointer to a communicator
. checkpointfunction - User function to call.  The parameters are 'comm'
  and 'extra_data'.  
- extra_data - A pointer that is passed to 'checkpointfunction'.

  Return value:  
  The function 'checkpointfunction' returns '0' on success and a negative
  integer on failure.  This value is returned by 'MPICH_Quiet'.  If 
  'MPICH_Quiet' fails for some other reason, it returns an MPI error code
  (a positive value).

  Notes:
  This routine can be used to provide for checkpointing operations where
  the user and the MPI implementation together guarantee that no communication
  is taking place, even by the low levels of the implementation, on the
  specified communicator while 'checkpointfunction' is running.  In addition,
  all communication is completed if possible.  That is, if any 'MPI_Request'
  (made in the specified communicator) given to 'MPI_Test' would cause 
  'MPI_Test' to return 'true' for the 'flag' value (i.e., the communication
  described by the request was completed), the communication must be completed
  before 'checkpointfunction' is called.   

  This is a collective call with semantics similiar to 'MPI_Barrier', with 
  the change that after all processes have entered 'MPICH_Quiet', the 
  user-specified function 'checkpointfunction' is called (collectively), and 
  once that routine returns, a process can leave 'MPICH_Quiet'. 

  Question:
  An alternative to this is a variation on the two-phase collective routines,
  using a begin and end pair to bracket the quiet period.  This allows the
  user to call almost any function between the begin and end.  One advantage
  to this approach is that it does not require that the user create a
  structure for the 'extra_data', and it can make it easier to handle 
  problems encountered during 'checkpointfunction' (e.g., out of disk space
  while writing the checkpoint).  This could be handled in a way similar to
  attribute copying in 'MPI_Comm_dup', but this is more convenient than the
  MPI error handling. 
  
  Module:
  Extension
  @*/
int MPICH_Quiet( MPID_Comm *comm, 
		  int (*checkpointfunction)(MPI_Comm *, void *), 
		  void *extra_data )
{}

/*
 * Virtual Connection Reference Table (VCRT)
 */
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{}
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{}
int MPID_VCRT_Release(MPID_VCRT vcrt)
{}
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{}

int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{}     
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{}

/*
  Description from Brian Toonen

NOTE: MPID_VCRT_Release() must release the VCRs held by the table when the
reference count on the table reaches zero.

With the above routines, you should be able to create, duplicate, and free
communicators.  See code fragments below.


MPI_Comm_create(comm, group, newcomm)
{
    ...
    MPID_VCRT_Create(group_ptr->size, &comm_ptr->vcrt)
    MPID_VCRT_Get_ptr(comm_ptr->vcrt, &comm_ptr->vcr)
    for (i = 0; i < group_ptr->size, i++)
    {
         MPID_VCR_Dup(comm_ptr->vcr[mapping[i]], newcomm_ptr->vcr[i])
    }
    ...
}

MPI_Comm_dup(oldcomm, newcomm)
{
    ...
    MPID_VCRT_Add_ref(oldcomm_ptr->vcrt)
    newcomm_ptr->vcrt = oldcomm_ptr->vcrt
    newcomm_ptr->vcr = oldcomm_ptr->vcr
    ...
}

MPI_Comm_free(comm)
{
    ...
    MPID_VCRT_Release(comm_ptr->vcrt)
    ...
}
*/

/*
 * ToDo:
 * Complete list
 * All handler types description of actions
 * Add MPI routines that may call each MPID routine.
 *
 * I'd still like to have a fast way to perform MPI_Reduce on a single
 * double precision value, perhaps by providing a fast concatenate
 * operation in the device (an Rhcv handler for it?) and then locally
 * applying the operation.
 *
 * Is there an easy way to make use of IP multicast?
 */

