#include "adi3.h"

/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 8, 2000
 ***********************************************************************/
/*T
 The following are routines that a device must implement; these are used
 by the code that implements the MPI communication operations.

 Note that some of these operations may be implemented using the others; for 
 example, MPID_Rhcv can use MPID_Rhc and a temporary memory buffer.  The
 routines that *must* be implemented are marked as MPID_CORE.  The other
 routines can be implemented (at some loss in efficiency) in terms of these
 or other standard operations.  The MPICH2000 implementation will include
 an implementation of all of these routines, using MPID_CORE for communication
 and other code for local operations (such as the datatype operations).

 Most "routines" may also be implemented as macros in cases (such as memory 
 allocation) where performance is critical.  Any function that must be a
 function (because a pointer to it is needed) is marked NO MACRO ALLOWED.
 (So far, there are no functions in this category).  For some functions, such
 as the timer routines, macros are recommended.

 In a few places, a decision hasn't been made.  These are marked as "Question".
 When answered, these will turn into "Rationales".

 This is MPID, not just MPID_CORE.

 Error handling and reporting:
 
 These routines assume that they have valid arguments, and do no checking that
 the arguments are valid.  The only exception is for errors that are difficult
 to check, such as an invalid buffer encountered because of a complex datatype,
 or message truncation.  Routines that involve communication may also return 
 errors caused by improper use (e.g., mismatched collective calls, invalid 
 ready send).  When an error is detected, an error code is created for it.
 This code contains an MPI error class and an instance-specific handle, used 
 to record more detailed information about the error.  Where an error code
 is returned, the value '0' signifies no error ('MPI_SUCCESS' is '0').

 Where possible, all errors should be recoverable.  That is, it should be
 possible for an MPI program to continue to run correctly.  When an error has 
 significant consequences, these should be limited to the smallest communicator
 possible.  For example, the failure of a process in MPI_COMM_WORLD should make
 MPI_COMM_WORLD invalid, but MPI_COMM_SELF and communicators that do not 
 involve the failed process should remain valid.  

 Many routines have no error returns.  This is a tough call, but the reason 
 is that the routines with no error returns have no opportunity to detect
 an error that should not have been caught by some other routine (remember,
 these aren't user-level routines).
 T*/

/*T
 * Section 1: Data Structures
 * 
 * Routines to manage and interrogate the data structures.
 *
 T*/
/*D
  Datastructure - Overview of MPID data structures

  MPI has a number of data structures, most of which are represented by 
  an opaque handle in an MPI program.  In MPID, these handles are represented
  as integers; this makes implementation of the C/Fortran handle transfer 
  calls (part of MPI-2) easy.  
 
  Because the data structures themselves may need to be in special memory 
  (e.g., shared) and/or need to contain device-specific information, the
  data structures are allocated and freed with MPID calls of the form
  'MPID_<datastructure>_new' and 'MPID_<datastructure>_free'.

  Reference Counts:
  The semantics of MPI require that objects that have been freed by the user
  (e.g., with 'MPI_Type_free' or 'MPI_Comm_free') remain valid until all 
  pending
  references to that object (e.g., by an 'MPI_Irecv') are complete.  There
  are several ways to implement this; MPICH uses `reference counts` in the
  objects.  To support the 'MPI_THREAD_MULTIPLE' level of thread-safety, these
  reference counts must be accessed and updated atomically.  Thus, each
  data structure has an 'MPID_<datastructure>_incr' routine that performs an
  atomic fetch and increment operation.  

 D*/

/*
 * Datatypes 
 */

/*@
    MPID_Datatype_new - Create a new datatype structure

    Return value: Pointer to the new data structure.  The following
    fields are already set:
+   id        - MPI_Datatype handle for this datatype.
-   ref_count - set to 1.

    Notes:
    If no datatype can be allocated, a null is returned.

    Module:
    Datatype
  @*/
MPID_Datatype *MPID_Datatype_new( void )
{
}

/*@
   MPID_Datatype_free - Free a datatype stucture.

   Input Parameter:
.  datatype - Pointer to datatype to free

   Notes:
   Unlike the MPI version, this does not assign NULL to the input parameter.
   Any special operations that a device may need to take when a datatype
   is freed.

   Module:
   Datatype
  @*/
void MPID_Datatype_free( MPID_Datatype *datatype )
{
}

/*@
   MPID_Datatype_incr - Change the reference count for a datatype

   Input Parameters:
+  datatype - Pointer to the datatype
-  incr     - Amount to change the reference count by (typically '1' or '-1')

   Return value:
   Value of the updated reference count.

   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an MPI_THREAD_MULTIPLE implementation,
   this routine must implement an atomic increment operation, using, for 
   example, a lock on datatypes or special assembly code (such as 
.vb
   try-again:
      load-link          refcount-address to r2
      add                incr to r2
      store-conditional  r2 to refcount-address
      if failed branch to try-again:
.ve
   on RISC architectures).  

   Once the reference count is decremented to zero, it is an error to 
   change it.  A correct MPI program will never do that, but an incorrect, 
   (particularly a multithreaded program with a race condition), might.  
   
   The following code is `invalid`:
.vb
   MPID_Datatype_incr( datatype, -1 );
   if (datatype->ref_count == 0) MPID_Datatype_free( datatype );
.ve
   In a multi-threaded implementation, the value of 'datatype->ref_count'
   may have been changed by another thread, resulting in both threads calling
   'MPID_Datatype_free'.  Instead, use
.vb
   if (MPID_Datatype_incr( datatype, -1 ) == 0) 
       MPID_Datatype_free( datatype );
.ve

   Module:
   Datatype

   Question:
   Do we want incr to free a datatype that has a reference count of zero?
   The pro is that any incr(-1) will then automatically check for a need
   to free the storage.  The con is that the incr routine must now know 
   about freeing a datatype.  And it really is only one line of code (and
   it isn''t hard to check).
  @*/
int MPID_Datatype_incr( MPID_Datatype *datatype, int incr )
{
}

/* 
 * There is no datatype_get_envelope and datatype_get_contents because the
 * structure of this part of the datatype is defined by the MPI implementation.
 */


/*@
  MPID_Pack - Pack a message into a specified buffer as type 'MPI_PACK'

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
  Do we really want this as a seperate routine, or can use the MPID_Segment_xxx
  routines?  If we can''t use the segment routines, does that mean that we 
  don''t have the right API yet, or is there an important difference?

  (The remaining apply only the the case of heterogeneous (in data 
  representation) systems.)

  In ADI-2, the packing format was determined first from '(comm,rank)' and
  then passed to the ADI-2 version of this routine.  That format was also
  sent to the receiver (as part of the message envelope), so that the data
  could be correctly interpreted.

  If we use receiver-makes-right, then we don''t need to do this, and we 
  don''t even need the '(comm,rank)' arguments.  That also matches the MPI
  notion of allowing data sent with 'MPI_PACKED' to be received with a 
  specific (an type-signature-conforming) datatype. However, 
  receiver-makes-right requires more code at the Unpack end (as well
  as '(comm,rank)').

  Do we want to mandate receiver-makes-right?  Globus guys, speak up.

  Note that XDR isn''t really an option, because it doesn''t handle all of the
  C datatypes (e.g., 'long double') or Fortran (e.g., 'LOGICIAL').

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

/*@
  MPID_Unpack - Unpack a buffer created with MPID_Pack

  Module:
  Datatype
  @*/
MPID_Unpack( void *outbuf, int count, MPID_Datatype *type,
             const void *inbuf, int *positin, int inbuf_size, 
	     MPID_Comm *comm, int rank )
{
}

/*@
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
{
}

/*
 * Groups
 *
 * For now, groups will *not* be scalable structures; that is, they will
 * enumerate their members, rather than using a scalable structure.
 */

/*@
  MPID_Group_new - Create a new group of the specified size

  Input Parameter:
. size - size of the group (number of processes)

  Notes:
  Returns a pointer to a group structure.  The field 'lrank_to_lpid' (a 
  pointer to an array) is allocated by this routine and the reference count
  'ref_count' is set to one.
  
  Module:
  Group
  @*/
MPID_Group *MPID_Group_new( int size )
{
}

/*@
  MPID_Group_free - Free a group

  Input Parameter:
. group - Group to free

  Notes:
  This is the counterpart of 'MPID_Group_new'.  It should be called only
  to free the device''s representation of a group.  

  Module:
  Group
  @*/
void MPID_Group_free( MPID_Group *group )
{
}

/*@
  MPID_Group_incr - Atomically update the reference count for a group.

   Input Parameters:
+  datatype - Pointer to the group
-  incr     - Amount to change the reference count by (typically '1' or '-1')

   Return value:
   Value of the updated reference count.

  Notes:
  'MPI_Group_dup' may be implemented by '(void) MPID_Group_incr( group, 1 )'.

  Module:
  Group
  @*/
int MPID_Group_incr( MPID_Group *group, int incr )
{
}

/* 
 * Communicators 
 */
/*@
  MPID_Comm_create - Create a new communicator from an old one and a group

  Input Parameters:
+  old_comm - Call is collective over old_comm.  
.  new_group - Group of new communicator.  This may be an existing 
   group (with an incremented reference count), a new group created
   with 'MPID_Group_new', or NULL (see below).
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
  collective operations.  The steps necessary to get a new context_id
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
  livelock.

  Module:
  Communicator

  Question:
  Do we want to define an MPID_Comm_context_get that provides an algorithm
  to return a new context?  Do we want to make this a part of MPID_Comm_create?
  We''ll do an MPIR one if it isn''t part of MPID.  Here''s a reason why we
  should make it part of the MPID level.  Consider a shared-memory system.
  Let there be a global queue of available context ids.  The lowest rank 
  process in the old communicator uses an atomic operation to remove the
  next value from the queue, and then broadcasts it to the members of the
  (old) communicator using a private context id (e.g., the private communicator
  used in MPICH-1).  Freeing a communicator returns the context id to the 
  shared pool.

  If we''re going to have a collective call to find a context id, should we
  just have a single call that has context_id as an output argument?

  Question: 
  should there be an error return in case something more global
  goes wrong?
  @*/
MPID_Comm *MPID_Comm_create( MPID_Comm *old_comm, 
			     MPID_Group *new_group, int context_id )
{
}

/*@
  MPID_Comm_free - Frees a communicator allocated by 'MPID_Comm_create'

  Input Parameter:
. comm - Pointer to communicator to free.

  Module:
  Communicator
  @*/
void MPID_Comm_free( MPID_Comm *comm )
{
}

/*@
  MPID_Comm_incr - Change the reference count for a communicator

  Input Parameters:
+ comm - Communicator to update 
- incr - value of change (positive to increment, negative to decrement)

  Return value:
  Returns the updated value of the reference count.

  Module:
  Communicator
  @*/
int MPID_Comm_incr( MPID_Comm *comm, int incr )
{
}

/*T
 *
 * Section x - Communicator attributes
 *
 * These provide a way for the user to pass data to the MPI implementation,
 * on a communicator-by-communicator basis.  To allow an implementation to
 * see changes to attributes, the following routines are called.  All may
 * be defined as macros that do nothing.
 *
 * Attributes on other objects are defined by MPI but not of interest (as yet)
 * to the device.
 *
 * In addition, there are 4 predefined attributes that the device must
 * supply to the implementation.  This is accomplished through 
 * 'MPID_Attr_predefined'.
 T*/
/*@
  MPID_Comm_attr_set - Inform the device about a change to an attribute
  value for a particular communicator.

  Input Parameters:
+ comm  - Communicator with attribute
. keyval - Key value for attribute
. attr_val - Value of attribute
- lang - Language that attribute was set from (see notes).

  Notes:
  This routine allows the device to find out when an attribute value changes.
  This can be used by a device that defines its own keyvals (see 
  MPID_Init_thread) to allow the MPI user to communicate preferences to 
  the device.  This has already been used in MPICH-G to pass quality-of-service
  information to the device.

  The language value is necessary because Fortran 77 attributes are integers,
  while C, C++, and Fortran 90 attributes are pointers (addresses).  These
  may not be the same length, so the address 'attr_val' must be interpreted
  differently.  Note that MPI uses 4 separate routines since each language
  has its own binding (the Fortran 90 one is 'MPI_Comm_attr_set'; the 
  Fortran 77 one is 'MPI_Attr_set').

  Module:
  Attributes

  Question:
  We may want to register keyvals with the device so that only attributes
  associated with the device call into it.  (e.g., MPID_Create_keyval( ... )).
  That also allows us to define
  "enhanced" keyvals that contain functions that validate any changes to the
  attribute values (e.g. an 'MPID_Attr_validate_function( int keyval, 
  void *attr_val, MPID_Lang_t lang )'.  
  This also argues that MPID_Comm_attr_set routine should
  return an MPI error code, allowing the device to cleanly signal an error
  to the user.
  @*/
void MPID_Comm_attr_set( MPID_Comm *comm, int keyval, void *attr_val, 
			 MPID_Lang_t lang )
{
}
/*@
   MPID_Comm_attr_delete - Inform the device that an attribute has been 
   removed from a communicator

  Input Parameters:
+ comm - Communicator
- keyval - key value of attribute.

  Module:
  Attributes
  @*/
void MPID_Comm_attr_delete( MPID_Comm *comm, int keyval )
{
}

/*@
  MPID_Attr_predefined - Return the value of a predefined attribute

  Input Parameter:
. keyval - A predefined keyval (the C version).  Note that these are part of 
  'mpi.h', and thus are known to the device.

  Return value:
  Value of the attribute (the value itself, not a pointer to it).  For example,
  the value of 'MPI_TAG_UB' is the integer value of the largest tag.  The
  value of 'MPI_WTIME_IS_GLOBAL' is a '1' for true and '0' for false.  Likely
  values for 'MPI_IO' and 'MPI_HOST' are 'MPI_ANY_SOURCE' and 'MPI_PROC_NULL'
  respectively.

  Module:
  Attributes

  Question:
  The value of 'MPI_WTIME_IS_GLOBAL' is better known by the timer.  Should
  we let the timer package provide this value?

  Shouldn''t this be part of MPID_CORE, since it talks about basic
  device functionality?

  @*/
int MPID_Attr_predefined( int keyval )
{
}

/*
 * Communicator locks for thread safety (?)
 * For example, in the MPI_THREAD_MULTIPLE mode, we must ensure that
 * two threads don't try to execute a collective communication routine
 * on the same communicator at the same time.  Thus, we need a communicator
 * lock for each communicator, and this lock is needed only when 
 * MPI_THREAD_MULTIPLE is provided.
 *
 * As a special case, we might allow, on a communicator by communicator
 * basis, the setting of the threadedness of communication on the 
 * communicator (this provides better modularity).  
 */

/*@
  MPID_Comm_thread_lock - Acquire a thread lock for a communicator

  Input Parameter:
. comm - Communicator to lock

  Notes:
  This routine acquires a lock among threads in the same MPI process that
  may use this communicator.  In all MPI thread modes except for
  'MPI_THREAD_MULTIPLE', this can be a no-op.  In an MPI implementation
  that does not provide 'MPI_THREAD_MULTIPLE', this may be a macro.

  It is invalid for a thread that has acquired the lock to attempt to 
  acquire it again.  The lock must be released by 'MPID_Comm_thread_unlock'.

  A high-quality implementation may wish to provide fair access to the lock.

  In general, the MPICH implementation tries to avoid using locks because 
  they can cause problems such as livelock and deadlock, particularly when
  an error occurs.  However, the semantics of MPI collective routines make 
  it difficult to avoid using locks.  Further, good programming practice by
  MPI programmers should be to avoid having multiple threads using the
  same communicator.

  Module:
  Communicator
  @*/
void MPID_Comm_thread_lock( MPID_Comm *comm )
{
}

/*@
  MPID_Comm_thread_unlock - Release a thread lock for a communicator

  Input Parameter:
. comm - Communicator to unlock

  Module:
  Communicator
@*/
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
 * Requests contain a block of storage that may be used in an MPID_Rhc 
 * call as the argument data ((ptr,n) in that call).  
 *
 * MPI has a number of different types of requests.  For example,
 * persistent requests and generalized (or user) requests.  The 
 * requests described here apply only to the common requests used in 
 * MPI_Isend, MPI_Irecv, etc.  The implementation of the MPI functions
 * such as MPI_Waitsome will handle these different kinds of requests.
 */

/*@
  MPID_Request_FOA - Find or allocate a request matching the conditions.

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
  allows 'MPID_Request_FOA', called from another thread, to match this request
  and wait for it to become ready.  The routine 'MPID_Request_ready' must
  be called to set the ready field.

  As written, this is only for the receive queue.

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

  @*/
MPID_Request *MPID_Request_FOA( int tag, int rank, MPID_Comm *comm, 
				int *found )
{
}

/*@
   MPID_Request_iprobe - Look for a matching request in but do not remove or 
   return it

  Input Parameters:
+ tag - Tag to match (or 'MPI_ANY_TAG')
. rank - rank to match (or 'MPI_ANY_SOURCE')
- comm - communicator to match.

  Output Parameter:
. status - 'MPI_Status' set as defined by 'MPI_Iprobe' (only when return 
  value is true).

  Return Value:
  True if a matching request was found, false otherwise.

  Notes:
  This is used to implement 'MPI_Iprobe' and possibly 'MPI_Probe'.

  Note that the values returned in 'status' will be valid for a subsequent
  MPI receive operation only if no other thread attempts to receive the same
  message.  See the discussion on this in the MPI-2 standard.

  Module:
  Request

  Question:
  Do we want a blocking version of this?  Or should there be a general
  block-until-something-happens call?
  @*/
int MPID_Request_iprobe( int tag, int rank, MPID_Comm *comm, 
			 MPI_Status *status )
{
}

/*@
  MPID_Request_cancel - Cancel the indicated request

  Input Parameter:
. request - Request to cancel

  Notes:
  Cancel is a tricky operation, particularly for sends.  Read the
  discussion in the MPI-1 and MPI-2 documents carefully.  This call
  only requests that the request be canceled; a subsequent wait (?what
  MPID call) or test (?ditto) must first succeed.

  Module:
  Request

  Question:
  Does this apply only the an FOA-allocated (e.g., receive) request?  If
  so, how are send requests cancelled?

  @*/
void MPID_Request_cancel( MPID_Request *request )
{
}

/*@
  MPID_Request_new - Create a new request for send operations only.

  Module:
  Request

  Question:
  Should this also be a FOA?  That is necessary if we permit receives to 
  posted to the requested sender (as we might want for WAN operation).

  My feeling is that it should be an FOA.  A single method device
  could use the speculative receive; alternately, even the fast
  multimethod device could allow it on communicators where the user
  has promised not to use 'MPI_ANY_SOURCE'.  For other devices, the
  FOA version is simply implemented as a new request operation.
  @*/
MPID_Request *MPID_Request_new( int rank, MPID_Comm *comm )
{
}

/*@
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

  This routine is not in MPID_CORE; however, a general solution appropriate
  for multi-threaded systems involves a thread-lock and is thus rather
  expensive.  Devices supporting or using multiple threads should provide
  an efficient implementation of this routine.

  Module:
  Request
  @*/
void MPID_Request_ready( MPID_Request *request )
{
}

/*@
  MPID_Request_free - Free a request 

  Input Parameter:
. request - request to free

  Notes:
  This routine may only be used for requests created with 'MPID_Request_new'.

  Module:
  Request
@*/
void MPID_Request_free( MPID_Request *request )
{
}

/*
 * Windows 
 */

/*
 * Good Memory (may be required for passive target operations on MPI_Win)
 */

/*@
  MPID_Alloc_mem - Allocate memory suitable for passive target RMA operations

  Input Parameter:
. size - Number of types to allocate.

  Return value:
  Pointer to the allocated memory.  If the memory is not available, 
  returns null.

  Notes:
  This memory may `only` be freed with 'MPID_Free_mem'.

  This is a `local`, not a collective operation.  It functions more like a
  good form of 'malloc' than collective shared-memory allocators such as
  the 'shmalloc' found on SGI systems.
  @*/
void *MPID_Alloc_mem( size_t size )
{
}

/*@
  MPID_Free_mem - Frees memory allocated with 'MPID_Alloc_mem'

  Input Parameter:
. ptr - Pointer to memory allocated by 'MPID_Alloc_mem'.

  Return value:
  'MPI_SUCCESS' if memory was successfully freed; an MPI error code otherwise.

  Notes:
  The return value is provided because it may not be easy to validate the
  value of 'ptr' without attempting to free the memory.
  @*/
int MPID_Free_mem( void *ptr )
{
}

/*T
 * Section 2: Communication
 *
 * General Notes: Communication operations are non-blocking, with a completion
 * flag.  In some cases, the flag may be specified as NULL; where this is
 * true, completion must be ensured by some other test.
 *
 * Flow control is the responsibility of the device; these routines are
 * non-blocking, allowing the device to queue communication for
 * later completion.
 * 
 * Because the communication/data transfer routines (MPID_Put, MPID_Rhc, 
 * MPID_Get, MPID_Rhcv) are nonblocking, the buffers containing the data to
 * be transfered must not be modified until the operation has (locally) 
 * completed (as marked by the appropriate flag).
 T*/
/*@
  MPID_Put - Copy local memory to a remote process

  Input Parameters:
+ buf - Pointer to local memory to be copied
. n   - Number of bytes to move
. target_offset - Location of destination relative to the origin of the
 window
. target_rank - Rank of the destination process in the group of the window
. window - Memory window object
. local_flag - Address of a flag to be set when this call is locally complete.
- target_flag - This is an id of a flag at the target process.  This value
  must have been specified by the target process in a previous communication
  (either with 'MPID_Put' or 'MPID_Rhc').  A value of '0' indicates no
  target completion flag.

  Notes:
  This routine is non-blocking.  It is responsible for any flow control.

  The origin_buf `must` be acquired through 'MPID_Segment_init_pack'.

  Module:
  MPID_CORE

  Questions:
  What addresses are valid for local flags?  What offsets are valid for 
  target flags?  Should the type of a flag be MPID_flag_t?  
  Do we require a test call on the flag, or only that the local flag be 
  declared as volatile (as in the prototype)?

  (My current thinking is to require MPID_Flag_t, and provide a routine
  to allocate and free them.  The device can easily include one within
  a MPID_Request structure.)

  Is there a special window for all of memory?  How can 'MPID_Put' be
  used when there is no user-specified window (e.g., in implementing
  'MPI_Isend')?  In general, what window is used for point-to-point
  communication (note that this appears in both the implementation of 
  the `generic` point-to-point routines and in the implementation of the
  generic collective routines.  Because of the latter, most devices
  will see whatever we''ve done for the collective case.

  @*/
int MPID_Put( const void *origin_buf, int n, 
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
. n    - length of buffer pointed at by 'ptr'
- local_flag - The location specified by this pointer is set when the operation
  has completed.  (what does completed mean? Does it only mean that ptr is
  available for reused (locally completed, not globally completed)?)

  Notes:
  Invokes one of the known handlers at the target rank.
  This call is non-blocking.
  Note that it takes a communicator as an argument.  When used on behalf of
  an 'MPI_Win' object, the communicator associated with the window is used.

  Questions: 
  The same as for MPID_Put on the local flag.
  @*/
int MPID_Rhc( int rank, MPID_Comm *comm, MPID_Handler_id id, void *ptr, int n,
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
int MPID_Flags_waitall( int count, int *(flags[]) )
{
}
int MPID_Flags_testall( int count, int *(flags[]), int *found )
{
}

/*@
  MPID_Flags_waitsome - Wait for the completion of one or more flags.

  Input Parameters:
+ count - number of completion flags to wait on
- flags - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.

  Notes:
  
  In a very rough sense, it corresponds to the Unix 'select', with a timeout
  of null (infinite).

  Module:
  MPID_CORE

  Question:
  By adding a min and max count to return, we can combine all
  wait/test functions and add some useful generalizations.  For
  example,
.vb
      min_count   max_count    equivalent routine
      0           1            test or testany
      0           count        testsome or testall (what is the difference?)
      1           1            wait or waitany
      1           count        waitsome
      count       count        waitall
.ve
  This allows the generalization\:
.vb
      0           4            testforatmost(4)
.ve
  This may be useful for some uses where the flags to complete on must be 
  copied first into a temporary (allocated on the stack) variable.
  @*/
int MPID_Flags_waitsome( int count, int *(flags[]) )
{
}

/*@
  MPID_Flags_testsome - Tests for the completion of one or more flags

  Input Parameters:
+ count - number of completion flags to wait on
- flags - Array of pointers to the flags.  Null pointers are ignored.

  Return value:
  The number of flags completed.  Each completed flag is set to null.
  If no flag is completed, 0 is returned.

  Notes:
  
  In a very rough sense, it corresponds to the Unix 'select', with a timeout
  of zero.
  See question posed to 'MPID_Flags_waitsome'.

  Module:
  MPID_CORE

  @*/
int MPID_Flags_testsome( int count, int *(flags[]) )
{
}

/*
 * Should there be MPID_Win_lock, unlock, and fence?
 * For fence, the argument is that this is a simple and already
 * specialized operation; it is hard to see how a simpler operation
 * could be defined that would still make possible the collective
 * semantics of fence (particularly with the optional assertions)
 * Of course, fence need not be in MPID_CORE, since it can be implemented
 * by waiting for completion of outstanding put/get operations and
 * executing MPI_Barrier to ensure that all remote operations targeting
 * this process have also completed.
 *
 * Lock and unlock are more problematic; These look a lot like 
 * Rhc calls.
 */

/*@
  MPID_Get - Get data from a target window to a local buffer

  Input Parameters:

  Output Parameter:

  Notes:
  This is very similar to the 'MPI_Get' routine, restricted to
  datatype 'MPI_BYTE'.  The major differences are the completion
  flags 'local_flag' and 'target_flag', which behave just as for 'MPID_Put'.

  @*/
int MPID_Get( void * origin_buf, int n, 
	      MPI_Aint target_offset, int target_rank, MPID_Win window, 
	      volatile void *local_flag, MPI_Aint target_flag )
{
}

/*@
   MPID_Rhcv - A vector version of 'MPID_Rhc'

  Input Parameters:
+ rank - Rank of process to invoke handler on
. comm - Communicator that rank is relative to
. id   - Id of handler
- vector - vector of 'struct iovec' elements containing information to
  be delivered to the remote handler.

  Output Parameter:
. local_flag - The location specified by this pointer is set when the operation
  has completed.

   Input Parameters:

   Notes:
   The first element of 'vector' points to a defined handler datatype that
   corresponds to the handler 'id'; the remaining elements are data that 
   the handler operation needs.  The handler 'id' can restrict the type of 
   memory that 'vector' may point at (e.g., it may require memory allocated 
   with 'MPID_Alloc_mem').

  This is a nonblocking operation.  No buffer described by the 'vector'
  argument may be modified until the 'local_flag' is set.

  Module:
  MPID_CORE

  Questions and Discussion:
  The more that I think about it, the more I want rhcv, not rhc, to be the
  fundemental (MPID_CORE) routine.  For example, the collective calls
  that are bypassing the send/receive routines will want to use this.
  An MPI_Put with a (noncontiguous) datatype will want to used this
  with a count of at least 3: vector[0] points to the header that 
  describes the basic put operation (window and offset), vector[1] points
  to a description of the datatype (if not cached at the destination), 
  and vector[3] points to the actual data.

  In terms of implementation, for a put to a strided datatype using TCP,
  we''d want to send the header describing the destination, the datatype
  description, and the data.  In a shared-memory case, where the destination
  window is in shared-memory, this routine may run locally.  Alternately,
  or where the window is not in shared-memory, the shared-memory verison
  could move some of the data into a shared-memory destination location and
  enqueue an handler-activation request at the destination process.

  Big question:  
  How many handler functions need be written to implement the core?
  What handler types are in the core?  
  @*/
int MPID_Rhcv( int rank, MPID_Comm *comm, MPID_Handler_id id, 
	       const struct iovec vector[], int count, int *local_flag )
{
}

/* 
 * Here go MPID versions of important point-to-point operations, just like the
 * full ADI-2. This list is expected to include:
 *
 * MPID_Isend
 * MPID_Irecv
 * MPID_Issend
 * MPID_Irsend
 * MPID_Waitsome
 * MPID_Testsome
 *
 * None of these is in MPID_CORE.
 * As for the other routines in MPID, these only check for errors that are 
 * difficult to check for without performing the operation.
 * 
 * A special case is
 * MPID_tBsend.
 * This has the semantics of MPI_Bsend, except that it returns the internal
 * error code MPID_WOULD_BLOCK if the message can't be sent immediately.
 * (t is for "try").  
 *
 * The reason that this interface is chosen over a query to check whether
 * a message *can* be sent is that the query approach is not thread-safe.
 * 
 */

/*T 
 * Section 3: Communication Buffer management
 *
 * The routines in this section fill two roles.  In the implementation of
 * routines such as MPID_Isend where the only device-specific routines are 
 * the few in MPID_CORE, it is necessary to have the ability to pack and unpack
 * partial messages.  Consider the case where the user calls
 * MPI_Isend( buf, 1, my_huge_indexed_type, ... )
 * where the total amount of data being sent is 100 MB.  Since MPID_CORE
 * can only move contiguous data, we need to convert the described data
 * into a sequence of contiguous memory segments of some reasonable size
 * (we don't want to have to allocate a 100 MB temporary buffer).
 * Thus, to implement this operation, we need a routine that can be called 
 * repeatedly to get the next m bytes from the user's described data buffer.
 * The MPI unpack routine does not have this flexibility.
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
 T*/

/*@
  MPID_Segment_init_pack - Get a buffer ready for packing and sending (put
  and/or Rhcv) data

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
  described by (buf,count,datatype) and hence may represent noncontiguous
  data or contiguous data in a heterogeneous system.

  Questions: 
  Add an `intent` so that sending all of the data can use device 
  knowledge about complex data layouts?  E.g., intent == MPID_BUFFER_ALL
  or intend == MPID_BUFFER_SEGMENT?

  The original description contained this text

  If null, the original buffer is contiguous and can be used as is.

  and

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

/*@
  MPID_Segment_pack - Pack up the designated buffer with a range of bytes.

  Input Parameters:
+ buf_desc - Buffer descriptor (initialized with 'MPID_Buffer_get_send')
- send_buffer - Pointer to buffer to place data in.  May be 'NULL', in which 
  case 'MPID_Segment_fill' uses an internal buffer (and returns it).

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
  (and is the reason to make first an in/out parameter).

  Questions:
  If 'MPID_Segment_pack' returns an internal buffer, who is responsible for
  freeing it?  It can''t be a single static buffer because that isn''t 
  thread-safe.

  If a send buffer is going to be provided, do we want to tell 
  MPID_Segment_init so that no internal buffer is allocated?

  Should we return the number of bytes packed in a variable, just
  to simplify the use of the result (which, after all, is a pointer `and`
  a length)?
  @*/
void *MPID_Segment_pack MPID_Segment *buf_desc, int *first, int *last, 
			void *send_buffer ) 
{
}

/*@
  MPID_Segment_init_unpack - .
  
  Questions:
  Should there be a flag indicating that the buffer may be used in a 
  put operation (as part of a receive and forward operation)?
  @*/
void * MPID_Segment_init_unpack( void *buf, int count, MPID_Datatype *dtype,
			     MPID_Comm *comm, int rank, 
			     MPID_Segment *segment )
{
}

/*@
  MPID_Segment_unpack - .
  @*/
int MPID_Segment_unpack( MPID_Segment *segment, int *first, int *last, 
		       void *recv_buffer )
{
}

/*@
  MPID_Segment_free - .
  @*/
int MPID_Segment_free( MPID_Segment *segment )
{
}

/*@
  MPID_Memory_register - Register a memory buffer 

  Input Parameters:
+ buf - Pointer to start of memory
. len - Length of buffer, in bytes
- rdwt - Indicate whether memory will be read ('MPID_MEM_READ'), 
         written ('MPID_MEM_WRITE'), 
         or both ('MPID_MEM_READ' | 'MPID_MEM_WRITE')

  Return value:
  'MPI_SUCCESS' if registration was successful, a valid MPI error code
  otherwise.  This error is non-fatal; the error return is made to 
  allow for more detailed information on why the memory registration 
  failed.

 Notes: 
 This call is made by 'MPI_Send_init' and 'MPI_Recv_init' to mark memory
 as used for communication; other MPI routines may also call it.

 This call allows devices that can optimize transfers for predefined 
 memory to take advantage of these calls.

  It is possible and valid for the same memory region to be registered 
  several times.  It is not an error to attempt to register the same 
  memory; the return code should indicate success if the memory is
  registered, even if this particular call was not responsible for actually
  registering the memory.  
  @*/
int MPID_Memory_register( void *buf, int len, int rdwt )
{
}
/*@
  MPID_Memory_unregister - Remote previous registration

  Input Parameters:
+ buf - Pointer to start of memory
. len - Length of buffer, in bytes
- rdwt - Indicate whether memory was registered to be read ('MPID_MEM_READ'), 
         written ('MPID_MEM_WRITE'), 
         or both ('MPID_MEM_READ' | 'MPID_MEM_WRITE').

  Return value:
  'MPI_SUCCESS' if deregistration was successful, a valid MPI error code
  otherwise.  This error is non-fatal; the error return is made to 
  allow for more detailed information on why the memory deregistration 
  failed.

  Notes:
  The arguments must be exactly the same as thous used to register memory.

  It is valid to call this routine as many times as 'MPID_Memory_register'
  returned success on the calling process.  If the same memory location is
  registered multiple times, then only when 'MPID_Memory_unregister' is called
  the `same` number of times should the memory be deregistered with the 
  operating system.  
  'MPID_Memory_unregister' thus has the same reference-count
  semantics as other MPI routines.  Note that this may require implementing
  a reference count mechanism for registered memory.
  @*/
int MPID_Memory_unregister( void *buf, int len, int rdwt )
{
}

/*T
 * Section 4: Store, Process, and Forward Communication
 *
 * Algorithms for collective communication are often of the
 * store and forward (to one or more processes); collective computation (such 
 * as reduce or scan) adds a processing step before forwarding.
 * This section describes the routines needed for these operations.
 * They rely on the MPID_Segment to provide an interface between the 
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
 T*/

/*@
  MPID_Isend_stream - Initialize a stream and begin sending.

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
  received with 'MPID_Irecv_stream'; th

  Question:
  Can we modify the stream buffer?  E.g., for allreduce or scan?  
  See 'MPID_Iforward_stream' below.
  @*/
int MPID_Isend_stream( MPID_Segment *segment, void *header, int header_size,
		       int tag, int rank, MPID_Comm *comm, 
		       int first, int last, MPID_Stream **stream )
{
}

/*@
  MPID_Irecv_stream - Initialize a stream for receiving

  Question:
  Where do I find the delivered header size?  

  Do I allow 'MPI_ANY_SOURCE' or 'MPI_ANY_TAG'?  I''d like not to.

  If I do `not` allow 'MPI_ANY_TAG', do we want to make tag mismatch
  an error?  That would happen if there was a collective operation
  mismatch on the communicator.

  @*/
int MPID_Irecv_stream( MPID_Segment *segment, void *header, int
		       max_header_size, int tag, int rank, 
		       MPID_Comm *comm, MPID_Stream **stream )
{
}

/*@
  MPID_Wait_stream - Wait for a stream to deliver the next segment of data.
  @*/
int MPID_Wait_stream( MPID_Stream *stream )
{
}	       

/*@
  MPID_Iforward_stream - Forward a stream

  Question:
  There is no output stream because this is a forward of an existing 
  stream. Is this the right thing to do?
  @*/
int MPID_Iforward_stream( MPID_Stream *stream, void *header, 
			   int header_size, int tag, int rank, 
			   MPID_Comm *comm )
{
}



/*T
 * Section x: Topology
 *
 * The MPI collective and topology routines can benefit from information 
 * about the topology of the underlying interconnect.  Unfortunately, there
 * is not best form for the representation (the MPI-1 Forum tried to define
 * such a representation, but was unable to).  One useful decomposition
 * that has been used in cluster enviroments is a hierarchical decomposition
 *
 * The other obviously useful topology information would match the needs of 
 * MPI_Cart_create.  However, it may be simpler to for the device to implement
 * this routine directly.
 *
 * Other useful information could be the topology information that matches
 * the needs of the collective operation, such as spanning trees and rings.
 * These may be added to adi3 later.
 T*/

/*@
  MPID_Topo_cluster_info - Return information on the hierarchy of 
  interconnections

  Output Parameters:
+ levels - The number of levels in the hierarchy.  
  To simplify the use of this routine, the maximum value is 
  'MPID_TOPO_CLUSTER_MAX_LEVELS' (typically 8 or less).
. my_cluster - For each level, the id of the cluster that the calling process
  belongs to.
- my_rank - For each level, the rank of the calling process in its cluster

  Notes:
  This routine returns a description of the system in terms of nested 
  clusters of processes.  Levels are numbered from zero.  At each level,
  each process may belong to no more than cluster; if a process is in any
  cluster at level i, it must be in some cluster at level i-1.


  Sample Outputs:
  For a single, switch-connected cluster or a uniform-memory-access (UMA)
  symmetric multiprocessor (SMP), the return values could be
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
.ve
  This is also a valid response for `any` device.

  For a switch-connected cluster of 2 processor SMPs
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
    1           0 to p/2           0 or 1
.ve
 where the value each process on the same SMP has the same value for
 'my_cluster[1]' and a different value for 'my_rank[1]'.

  For two SMPs connected by a network,
.vb
    level       my_cluster         my_rank
    0           0                  rank in comm_world
    1           0 or 1             0 to # on SMP
.ve

  An example with more than 2 levels is a collection of clusters, each with
  SMP nodes.  

  Limitations:
  This approach does not provide a representations for topologies that
  are not hierarchical.  For example, a mesh interconnect is a single-level
  cluster in this view.

  Module: 
  Topology
  @*/
int MPID_Topo_cluster_info( int *levels, int my_cluster[], int my_rank[] )
{
}

/*T
 * Section n: Miscellaneous
 T*/
/*@
  MPID_Thread_init - Initialize the device

  Input Parameters:
+ argc_p - Pointer to the argument count
. argv_p - Pointer to the argument list
- requested - Requested level of thread support.  Values are the same as
  for the 'required' argument to 'MPI_Init_thread'.

  Output Parameter:
. provided - Provided level of thread support.  May be less than the 
  requested level of support.

  Return value:
  Returns '0' on success and an MPI error code on failure.  Failure can happen
  when, for example, the device is unable to start or contact the number of
  processes specified by the 'mpiexec' command.

  Notes:
  Null arguments for 'argc_p' and 'argv_p' `must` be valid.

  Multi-method devices should initialize each method within this call.
  They can use environment variables and/or command-line arguments
  to decide which methods to initialize (but note that they must not
  `depend` on using command-line arguments).

  This call also initializes all MPID datastructures.  Some of these (e.g.,
  'MPID_Request') may be in shared memory; others (e.g., 'MPID_Datatype') 
   may be allocated from an block of array values.  

  Module:
  MPID_CORE

   Questions:
  Should the thread support value be an enum type instead of an int?

  Do we require that all processes get the same argument lists?
  Return the same argument lists?  Can we fix the Fortran command-line
  arguments?
  Does each process have the same values for the environment variables 
  when this routine returns?

  Are there recommended environment variable names?  Names that are
  explicitly prohibited?

  How does this interface to BNR?  Do we need to know anything?  Should
  this call have an info argument to support BNR?
  @*/
int MPID_Thread_init( int *argc_p, char *(*argv_p)[], 
		      int requested, int *provided )
{
}

/*@
  MPID_Abort - Abort the at least the processes in the specified communicator.

  Return value:
  MPI_SUCCESS or an MPI error code.  Normally, this routine should not return,
  since the calling process must be a member of the communicator.  However, 
  under some circumstances, the 'MPID_Abort' might fail; in this case,
  returning an error indication is appropriate.

  Notes:

  In a fault-tolerant MPI implementation, this operation should abort `only` 
  the processes in the specified communicator.  Any communicator that shares
  processes with the aborted communicator becomes invalid.  For more 
  details, see (paper not yet written on fault-tolerant MPI).

  Module:
  MPID_CORE
  @*/
int MPID_Abort( MPID_Comm *comm )
{
}

/*@
  MPID_Finalize - Perform the device-specific termination of an MPI job

  Return Value:
  MPI_SUCCESS or a valid MPI error code.  Normally, this routine will
  return 'MPI_SUCCESS'.  Only in extrordinary circumstances can this
  routine fail; for example, is some process stops responding during the
  finalize step.  In this case, 'MPID_Finalize' should return an MPI 
  error code indicating the reason that it failed.

  Notes:

  Module:
  MPID_CORE

  Questions:
  Need to check the MPI-2 requirements on MPI_Finalize with respect to
  things like which process must remain after MPID_Finalize is called.
  @*/
int MPID_Finalize( void )
{
}

? Information on data representations for heterogeneous code.

? We probably need both compile-time and run-time information on whether
the system is homogeneous or not (note that in an MPI-2 setting, the run-time
value might be "not yet heterogeneous").

? Information on "good" message break points? (e.g., short/eager/rendezous)?
? This is useful for the "generic" implementations of Isend etc., based on
? MPID_Put and MPID_Rhc.

/*T
 * Section n+1: Service Routines
 *
 * Many of these may be implemented (as macros) by their underlying routine
 * However, in some cases, it may be advantageous to use special routines.
 * Alternatives can include routines that trace or record operations.
 T*/

/*D
  Memory - Memory Management Routines

  Rules for memory management:

  MPICH explicity prohibits the appearence of 'malloc', 'free', 
  'calloc', 'realloc', or 'strdup' in any code implementing a device or 
  MPI call (of course, users may use any of these calls in their code).  
  That is, you must use 'MPID_Malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPID_Malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks, 
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Thread_init' is 
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other MPID routine should fail because memory could not be allocated
  (for example, because the user has allocated large arrays after 'MPI_Init').
  
  The implementation of the MPI routines will strive to avoid memory allocation
  as well; however, operations such as 'MPI_Type_index' that create a new
  data type that reflects data that must be copied from an array of arbitrary
  size will have to allocate memory (and can fail; note that there is an
  MPI error class for out-of-memory).
  D*/

/*@
  MPID_Malloc - Allocated memory

  Input Parameter:
. len - Length of memory to allocate in bytes

  Return Value:
  Pointer to allocated memory, or null if memory could not be allocated.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPID_Malloc(n) malloc(n)
.ve
  However, it can also be defined as 
.vb
  #define MPID_Malloc(n) trmalloc(n,__FILE__,__LINE__)
.ve
  where 'trmalloc' is a tracing version of malloc that is included with 
  MPICH.

  @*/
void *MPID_Malloc( size_t len )
{
}

/*@
  MPID_Free - Free memory

  Input Parameter:
. ptr - Pointer to memory to be freed.  This memory must have been allocated
  with 'MPID_Malloc'.

  @*/
void MPID_Free( void * ptr )
{
}

/*@
  MPID_Memcpy - Copy memory

  Input Parmeters:
+ src - Pointer to memory to copy
- n   - Number of bytes to copy

  Output Parameters:
. dest - Pointer to destination to copy 'src' into.

  Return value:
  ? dest ?

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
  @*/
void *MPID_Memcpy( void *dest, const void *src, size_t n )
{
}

/*@
  MPID_Calloc - .
  @*/
void *MPID_Calloc( )
{
}
 
/*@ 
  MPID_Strdup - Duplicate a string

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.
  @*/
char *MPID_Strdup( const char *str )
{
}

/*
 * Timers
 */

/*@
  MPID_Wtime - Return a time value
  
  Output Parameter:
. timeval - A pointer to an 'MPID_Wtime_t' variable.

  Notes:
  This routine returns an `opaque` time value.  This difference between two
  time values returned by 'MPID_Wtime' can be converted into an elapsed time
  in seconds with the routine 'MPID_Wtime_diff'.

  This routine is defined this way to simplify its implementation as a macro.
  For example, (make the following correct for Linux)
.vb
#define MPID_Wtime(timeval) { asm(read timer,r1); asm(store r1,timeval); }
.ve

  For some purposes, it is important
  that the timer calls change the timing of the code as little as possible.
  This form of a timer routine provides for a very fast timer that has
  minimal impact on the rest of the code.  

  From a semantic standpoint, this format emphasizes that any particular
  timer value has no meaning; only the difference between two values is 
  meaningful.

  Module:
  Timer
  @*/
void MPID_Wtime( MPID_Wtime_t *timeval )
{
}

/*@
  MPID_Wtime_diff - Compute the difference between two time values

  Input Parameters:
. t1,t2 - Two time values set by 'MPID_Wtime'.
 

  Output Parameter:
. diff - The different in time between t2 and t1, measured in seconds.

  Module:
  Timer
  @*/
void MPID_Wtime_diff( MPID_Wtime_t *t1, MPID_Wtime_t *t2, double *diff )
{
}

/*@
  MPID_Wtick - Provide the resolution of the 'MPID_Wtime' timer

  Return value:
  Resolution of the timer in seconds.  In many cases, this is the 
  time between ticks of the clock that 'MPID_Wtime' returns.  In other
  words, the minimum significant difference that can be computed by 
  'MPID_Wtime_diff'.

  Note that in some cases, the resolution may be estimated.  No application
  should expect either the same estimate in different runs or the same
  value on different processes.

  Module:
  Timer
  @*/
double MPID_Wtick( void )
{
}

/*@
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
  'MPID_Init_thread').  In an MPI-2 setting, this routine may be
  called within 'MPI_Comm_spawn' and friends to establish a global
  timer even when new processes are added to the MPI job.

  Module:
  Timer
  *@/
int MPID_Gwtime_init( MPID_Comm *comm )
{
}

/*@
  MPID_Gwtime_diff - Compute the difference between two time values from
  different processes

  Input Parameters:
+ t1,t2 - Two time values set by 'MPID_Wtime'.
- pid1,pid2 - Process numbers corresponding to 't1' and 't2' respectively.
 
  Output Parameter:
. diff - The different in time between t2 and t1, measured in seconds.

  Notes:
  This routine provides access to a global timer for all MPI processes.
  If the attribute 'MPI_WTIME_IS_GLOBAL' is true, then it is the same
  as 'MPID_Wtime_diff'.  

  If no global timer is available, it is possible to synthesize one.
  Such an implementation is provided as part of the utilities 
  library with the MPICH MPID implementation.  Note that
  the resolution of this timer (at least between processes) is significantly
  lower than for the local timer.

  Module:
  Timer
  @*/
void MPID_Gwtime_diff( MPID_Wtime_t *t1, int pid1, 
		       MPID_Wtime_t *t2, int pid2, double *diff )
{
}

/*@
  MPID_Gwtick - Return an upper bound on the resolution of the global timer

  Input Parameters:
. pid1,pid2 - Process numbers.  See "return value".

  Return value:
  An upper bound on the resolution of the 'MPID_Gwtime_diff' routine for
  process ids 'pid1' and 'pid2'.  If the attribute 'MPI_WTIME_IS_GLOBAL' is
  true and eith 'pid1' or 'pid2' are negative, then this
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

  @*/
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
 * Note that for a multithreaded (MPI_THREAD_MULTIPLE) implementation,
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

/*@
  MPID_Attr_find - Find a attribute for a given keyval

  Input Parameters:
+ list - Attribute list to search
. keyval - keyval to look for
- insert - True if attribute should be inserted if it is not found.

  Return Value:
  A pointer to the matching attribute, or null if no attribute matching the
  keyval was found.

  Notes:
  This routine should only be used in a context where no other thread
  may modify the attribute list of the underlying object.  It may be
  necessary to use 'MPID_xxx_thread_lock' and 'MPID_xxx_thread_unlock'
  around uses of this routine.

  Using a value of 0 for 'insert' makes it easy to implement 'MPI_Attr_get';
  a value of 1 is used for 'MPI_Attr_put' (and the MPI-2 versions of
  these).

  Module:
  Attribute
  @*/
MPID_Attribute *MPID_Attr_find( MPID_List *list, int keyval, int insert )
{
}

/* If we use the attr-find model, we also need a routine to get all of the
   attributes in a list, so that we can implement Comm_dup and Type_dup
*/

/*@
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
  'MPI_Comm_dup' and 'MPI_Type_dup').

  Module:
  Attribute
  @*/
MPID_Attribute *MPID_Attr_list_walk( MPID_List *list, MPID_Attribute *prev )
{
}

/*@
  MPID_Attr_delete - Remote an attribute from a list

  Input Parameters:
+ list - Attribute list to search
- keyval - keyval to look for
 
  Return value:
  zero if the attribute was found and deleted, an MPI error code otherwise.
  
  Module:
  Attribute
  @*/
int MPID_Attr_delete( MPID_List *list, int keyval )
{
}

/*T
 * Section x: Environment
 *
 * These are the global defines and variables that other modules may
 * refer to.  
 *
  T*/

/*D
  MPID_THREAD_LEVEL - Indicates the level of thread support provided
 
  Values:
. Any of the 'MPI_THREAD_xxx' values.  

  Notes:
  This variable allows implementations that support
  'MPI_THREAD_MULTIPLE' to bypass thread locks when the user has
  requested (and received) a lower level of thread support.

  The macro 'MPID_MAX_THREAD_LEVEL' defines the maximum level of
  thread support provided, and may be used at compile time to remove
  thread locks and other code needed only in a multithreaded environment.

  D*/
extern int MPID_THREAD_LEVEL;

/*
 * ToDo:
 * Complete list
 * Add MPI routines that may call each MPID routine.
 *
 * I'd still like to have a fast way to perform MPI_Reduce on a single
 * double precision value, perhaps by providing a fast concatenate
 * operation in the device (an Rhc handler for it?) and then locally
 * applying the operation.
 */

