/* -*- Mode: C; c-basic-offset:4 ; -*- */
#include "adi3.h"

/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 22, 2000
 ***********************************************************************/
/*TOverview.tex
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
/*TDSOverview.tex
  
  MPI has a number of data structures, most of which are represented by 
  an opaque handle in an MPI program.  In the MPICH implementation of MPI, 
  these handles are represented
  as integers; this makes implementation of the C/Fortran handle transfer 
  calls (part of MPI-2) easy.  
 
  MPID objects (again with the possible exception of 'MPI_Request's) 
  are allocated by a common set of object allocation functions.
  These are 
.vb
    void *MPIU_Handle_obj_create( MPIU_Object_alloc_t *objmem )
    void MPIU_Handle_obj_destroy( MPIU_Object_alloc_t *objmem, void *object )
.ve
  where 'objmem' is a pointer to a memory allocation object that knows 
  enough to allocate objects, including the
  size of the object and the location of preallocated memory, as well 
  as the type of memory allocator.  By providing the routines to allocate and
  free the memory, we make it easy to use the same interface to allocate both
  local and shared memory for objects (always using the same kind for each 
  type of object).

  The names create/destroy were chosen because they are different from 
  new/delete (C++ operations) and malloc/free.  
  Any name choice will have some conflicts with other uses, of course.

  Reference Counts:
  Many MPI objects have reference count semantics.  
  The semantics of MPI require that many objects that have been freed by the 
  user 
  (e.g., with 'MPI_Type_free' or 'MPI_Comm_free') remain valid until all 
  pending
  references to that object (e.g., by an 'MPI_Irecv') are complete.  There
  are several ways to implement this; MPICH uses `reference counts` in the
  objects.  To support the 'MPI_THREAD_MULTIPLE' level of thread-safety, these
  reference counts must be accessed and updated atomically.  
  A reference count for
  `any` object can be incremented (atomically) 
  with 'MPID_Object_add_ref(objptr)'
  and decremented with 'MPID_Object_release_ref(objptr,newval_ptr)'.  
  These have been designed so that then can be implemented as inlined 
  macros rather than function calls, even in the multithreaded case, and
  can use special processor instructions that guarantee atomicity to 
  avoid thread locks.
  The decrement routine returns the postdecrement value of the reference 
  counter.  If this value is zero, then the routine that decremented the
  reference count should free the object.  This may be as simple as 
  calling 'MPIU_Handle_obj_destroy' (for simple objects with no other allocated
  storage) or may require calling a separate routine to destroy the object.
  Because MPI uses 'MPI_xxx_free' to both decrement the reference count and 
  free the object if the reference count is zero, we avoid the use of 'free'
  in the MPID routines.

  Structure Definitions:
  The structure definitions in this document define `only` that part of
  a structure that may be used by code that is making use of the ADI.
  Thus, some structures, such as 'MPID_Comm', have many defined fields;
  these are used to support MPI routines such as 'MPI_Comm_size' and
  'MPI_Comm_remote_group'.  Other structures may have few or no defined
  members; these structures have no fields used outside of the ADI.  
  In C++ terms,
  all members of these structures are 'private'.  One example of such a
  structure is 'MPID_Stream'.

  For the initial implementation, we expect that the structure definitions 
  will be designed for the multimethod device.  However, all items that are
  specific to a particular device (including the multi-method device) 
  will be placed at the end of the structure;
  the document will clearly identify the members that all implementations
  will provide.  This simplifies much of the code in both the ADI and the 
  implementation of the MPI routines because structure member can be directly
  accessed rather than using some macro or C++ style method interface.
  
 T*/

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
    fields, such as 'has_mpi1_ub' and 'loopinfo', will be set by the 
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

/*@
   MPIU_Object_add_ref - Increment the reference count for an MPI object

   Input Parameter:
.  ptr - Pointer to the object.

   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic increment operation, using, for 
   example, a lock on datatypes or special assembly code such as 
.vb
   try-again:
      load-link          refcount-address to r2
      add                1 to r2
      store-conditional  r2 to refcount-address
      if failed branch to try-again:
.ve
   on RISC architectures or
.vb
   lock
   inc                   refcount-address or
.ve
   on IA32; "lock" is a special opcode prefix that forces atomicity.  This 
   is not a separate instruction; however, the GNU assembler expects opcode
   prefixes on a separate line.

   Module: 
   MPID_CORE

   Question:
   This accesses the 'ref_count' member of all MPID objects.  Currently,
   that member is typed as 'volatile int'.  However, for a purely polling,
   thread-funnelled application, the 'volatile' is unnecessary.  Should
   MPID objects use a 'typedef' for the 'ref_count' that can be defined
   as 'volatile' only when needed?  For now, the answer is no; there isn''t
   enough to be gained in that case.
@*/
void MPID_Object_add_ref( MPIU_Object_head *ptr )
{}

/*@
   MPIU_Object_release_ref - Decrement the reference count for an MPI object

   Input Parameter:
.  objptr - Pointer to the object.

   Output Parameter:
.  newval_ptr - Pointer to the value of the reference count after decrementing.
   This value is either zero or non-zero. See below for details.
   
   Notes:
   In an unthreaded implementation, this function will usually be implemented
   as a single-statement macro.  In an 'MPI_THREAD_MULTIPLE' implementation,
   this routine must implement an atomic decrement operation, using, for 
   example, a lock on datatypes or special assembly code such as 
.vb
   try-again:
      load-link          refcount-address to r2
      sub                1 to r2
      store-conditional  r2 to refcount-address
      if failed branch to try-again:
      store              r2 to newval_ptr
.ve
   on RISC architectures or
.vb
      lock
      dec                   refcount-address 
      if zf store 0 to newval_ptr else store 1 to newval_ptr
.ve
   on IA32; "lock" is a special opcode prefix that forces atomicity.  This 
   is not a separate instruction; however, the GNU assembler expects opcode
   prefixes on a separate line.  'zf' is the zero flag; this is set if the
   result of the operation is zero.  Implementing a full decrement-and-fetch
   would require more code and the compare and swap instruction.

   Once the reference count is decremented to zero, it is an error to 
   change it.  A correct MPI program will never do that, but an incorrect one 
   (particularly a multithreaded program with a race condition) might.  

   The following code is `invalid`\:
.vb
   MPID_Object_release_ref( datatype_ptr );
   if (datatype_ptr->ref_count == 0) MPID_Datatype_free( datatype_ptr );
.ve
   In a multi-threaded implementation, the value of 'datatype_ptr->ref_count'
   may have been changed by another thread, resulting in both threads calling
   'MPID_Datatype_free'.  Instead, use
.vb
   if (MPID_Object_release_ref( datatype_ptr ) == 0) 
       MPID_Datatype_free( datatype_ptr );
.ve

   Module: 
   MPID_CORE
  @*/
int MPID_Object_release_ref( MPIU_Object_head *ptr )
{}


/*@
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


/*@
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

/*@
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

/*@
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

/*TAttrOverview.tex
 *
 * The MPI standard allows `attributes`, essentially an '(integer,pointer)'
 * pair, to be attached to communicators, windows, and datatypes.  
 * The integer is a `keyval`, which is allocated by a call (at the MPI level)
 * to 'MPI_Comm/Type/Win_create_keyval'.  The pointer is the value of 
 * the attribute.
 * Attributes are primarily intended for use by the user, for example, to save
 * information on a communicator, but can also be used to pass data to the
 * MPI implementation.  For example, an attribute may be used to pass 
 * Quality of Service information to an implementation to be used with 
 * communication on a particular communicator.  
 * To provide the most general access by the ADI to all attributes, the
 * ADI defines a collection of routines that are used by the implementation
 * of the MPI attribute routines (such as 'MPI_Comm_get_attr').
 * In addition, the MPI routines involving attributes will invoke the 
 * corresponding 'hook' functions (e.g., 'MPID_Dev_comm_attr_set_hook') 
 * should the device define them.
 *
 * Attributes on windows and datatypes are defined by MPI but not of interest (as yet) 
 * to the device.
 *
 * In addition, there are seven predefined attributes that the device must
 * supply to the implementation.  This is accomplished through 
 * data values that are part of the 'MPIR_Process' data block.
 *  The predefined keyvals on 'MPI_COMM_WORLD' are\:
 *.vb
 * Keyval                     Related Module
 * MPI_APPNUM                 Dynamic
 * MPI_HOST                   Core
 * MPI_IO                     Core
 * MPI_LASTUSEDCODE           Error
 * MPI_TAG_UB                 Communication
 * MPI_UNIVERSE_SIZE          Dynamic
 * MPI_WTIME_IS_GLOBAL        Timer
 *.ve
 * The values stored in the 'MPIR_Process' block are the actual values.  For 
 * example, the value of 'MPI_TAG_UB' is the integer value of the largest tag.
 * The
 * value of 'MPI_WTIME_IS_GLOBAL' is a '1' for true and '0' for false.  Likely
 * values for 'MPI_IO' and 'MPI_HOST' are 'MPI_ANY_SOURCE' and 'MPI_PROC_NULL'
 * respectively.
 *
 T*/
/*@
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

  Note that there is also a common per-process lock ('common_lock').  
  That lock should be used instead of a lock on lock on 'MPI_COMM_WORLD' when
  a lock across all threads is required.

  A high-quality implementation may wish to provide fair access to the lock.

  In general, the MPICH implementation tries to avoid using locks because 
  they can cause problems such as livelock and deadlock, particularly when
  an error occurs.  However, the semantics of MPI collective routines make 
  it difficult to avoid using locks.  Further, good programming practice by
  MPI programmers should be to avoid having multiple threads using the
  same communicator.

  Module:
  Communicator

  See Also: 
  'MPID_Comm_thread_unlock'

  Questions:
  Do we also need versions of this for datatypes and window objects?  
  For example, communicators, datatypes, and window objects all have 
  attributes; do we need a thread lock for each type?  Should we instead have
  an MPI Object, on which some common operations, such as thread lock, 
  reference count, and name are implemented?
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

  See Also: 
  'MPID_Comm_thread_lock'
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

/*@
   MPID_Request_iprobe - Look for a matching request in the receive queue 
   but do not remove or return it

  Input Parameters:
+ tag - Tag to match (or 'MPI_ANY_TAG')
. rank - rank to match (or 'MPI_ANY_SOURCE')
. comm - communicator to match.
- context_id - context id of communicator to match

  Output Parameter:
. status - 'MPI_Status' set as defined by 'MPI_Iprobe' (only when return 
  value is true).

  Return Value:
  True if a matching request was found, false otherwise.

  Notes:
  This is used to implement 'MPI_Iprobe'.

  Note that the values returned in 'status' will be valid for a subsequent
  MPI receive operation only if no other thread attempts to receive the same
  message.  
  (See the 
  discussion of probe in Section 8.7.2 (Clarifications) of the MPI-2 standard.)

  Providing the 'context_id' is necessary at this level to support the 
  way in which the MPICH implementation uses context ids in the implementation
  of other operations.  The communicator is present to allow the device 
  to use message-queues attached to particular communicators or connections
  between processes.

  Module:
  Request

  @*/
int MPID_Request_iprobe( int tag, int rank, MPID_Comm *comm, 
			 MPI_Status *status )
{
}

/*@
   MPID_Request_probe - Block until a matching request is found
   return it

  Input Parameters:
+ tag - Tag to match (or 'MPI_ANY_TAG')
. rank - rank to match (or 'MPI_ANY_SOURCE')
. comm - communicator to match.
- context_id - context id of communicator to match

  Output Parameter:
. status - 'MPI_Status' set as defined by 'MPI_Iprobe' (only when return 
  value is true).

  Notes:
  This is used to implement 'MPI_Probe'.

  Note that the values returned in 'status' will be valid for a subsequent
  MPI receive operation only if no other thread attempts to receive the same
  message.  
  (See the 
  discussion of probe in Section 8.7.2 Clarifications of the MPI-2 standard.)

  Providing the 'context_id' is necessary at this level to support the 
  way in which the MPICH implementation uses context ids in the implementation
  of other operations.  The communicator is present to allow the device 
  to use message-queues attached to particular communicators or connections
  between processes.

  Question:
  Should this have an error return in case a failure on the communicator
  is detected?

  Module:
  Request

  @*/
void MPID_Request_probe( int tag, int rank, MPID_Comm *comm, 
			  MPI_Status *status )
{
}

/*@
  MPID_Request_cancel_send - Cancel the indicated send request

  Input Parameter:
. request - Send request to cancel

  Notes:
  Cancel is a tricky operation, particularly for sends.  Read the
  discussion in the MPI-1 and MPI-2 documents carefully.  This call
  only requests that the request be cancelled; a subsequent wait 
  or test must first succeed.

  Module:
  Request

  Question:
  What MPID calls should be used to perform the wait or test mentioned above?
  @*/
void MPID_Request_cancel_send( MPID_Request *request )
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

/* @
  MPID_Request_free - Free a request 

  Input Parameter:
. request - request to free

  Notes:
  This routine may only be used for requests created with 'MPID_Request_new' or  one of the find-or-allocate routines.

  Module:
  Request
@ */
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
  MPID_Mem_alloc - Allocate memory suitable for passive target RMA operations

  Input Parameter:
+ size - Number of types to allocate.
- info - Info object

  Return value:
  Pointer to the allocated memory.  If the memory is not available, 
  returns null.

  Notes:
  This routine is used to implement 'MPI_Alloc_mem'.  It is for that reason
  that there is no communicator argument.  

  This memory may `only` be freed with 'MPID_Mem_free'.

  This is a `local`, not a collective operation.  It functions more like a
  good form of 'malloc' than collective shared-memory allocators such as
  the 'shmalloc' found on SGI systems.

  Implementations of this routine may wish to use 'MPID_Memory_register'.  
  However, this routine has slighly different requirements, so a separate
  entry point is provided.

  Module:
  Win
  @*/
void *MPID_Mem_alloc( size_t size, MPID_Info *info )
{
}

/*@
  MPID_Mem_free - Frees memory allocated with 'MPID_Mem_alloc'

  Input Parameter:
. ptr - Pointer to memory allocated by 'MPID_Mem_alloc'.

  Return value:
  'MPI_SUCCESS' if memory was successfully freed; an MPI error code otherwise.

  Notes:
  The return value is provided because it may not be easy to validate the
  value of 'ptr' without attempting to free the memory.

  Module:
  Win
  @*/
int MPID_Mem_free( void *ptr )
{
}

/*@
  MPID_Mem_was_alloced - Return true if this memory was allocated with 
  'MPID_Mem_alloc'

  Input Parameters:
+ ptr  - Address of memory
- size - Size of reqion in bytes.

  Return value:
  True if the memory was allocated with 'MPID_Mem_alloc', false otherwise.

  Notes:
  This routine may be needed by 'MPI_Win_create' to ensure that the memory 
  for passive target RMA operations was allocated with 'MPI_Mem_alloc'.
  Module:
  Win
  @*/
int MPID_Mem_free( void *ptr )
{
}

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

/*@
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

/*@
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

/*@
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

/*@
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

/*@
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

/*@
  MPID_Isend - MPID entry point for MPI_Isend

  Notes:
  The only difference between this and 'MPI_Isend' is that the basic
  error checks (e.g., valid communicator, datatype, rank, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id is provided in addition to the communicator.

  Module:
  Communication

  @*/
int MPID_Isend( const void *buf, int count, MPID_Datatype *datatype,
		int tag, int rank, MPID_Comm *comm, int context_id,
		MPID_Request **request )
{
}

/*@
  MPID_Issend - MPID entry point for MPI_Issend

  Notes:
  The only difference between this and 'MPI_Issend' is that the basic
  error checks (e.g., valid communicator, datatype, rank, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id is provided in addition to the communicator.

  Module:
  Communication

  @*/
int MPID_Issend( const void *buf, int count, MPID_Datatype *datatype,
		int tag, int rank, MPID_Comm *comm, int context_id,
		MPID_Request **request )
{
}

/*@
  MPID_Irsend - MPID entry point for MPI_Irsend

  Notes:
  The only difference between this and 'MPI_Irsend' is that the basic
  error checks (e.g., valid communicator, datatype, rank, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id is provided in addition to the communicator.

  Module:
  Communication

  @*/
int MPID_Irsend( const void *buf, int count, MPID_Datatype *datatype,
		int tag, int rank, MPID_Comm *comm, int context_id,
		MPID_Request **request )
{
}

/*@
  MPID_Irecv - MPID entry point for MPI_Irecv

  Notes:
  The only difference between this and 'MPI_Irecv' is that the basic
  error checks (e.g., valid communicator, datatype, rank, and tag)
  have been made, the MPI opaque objects have been replaced by
  MPID objects, and a context id is provided in addition to the communicator.

  Module:
  Communication

  @*/
int MPID_Irecv( void *buf, int count, MPID_Datatype *datatype,
		int tag, int rank, MPID_Comm *comm, int context_id,
		MPID_Request **request )
{
}


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
  
/*@
  MPID_tBsend - Attempt a send and return if it would block

  Notes:
  This has the semantics of 'MPI_Bsend', except that it returns the internal
  error code 'MPID_WOULD_BLOCK' if the message can''t be sent immediately
  (t is for "try").  
 
  The reason that this interface is chosen over a query to check whether
  a message `can` be sent is that the query approach is not
  thread-safe.  Since the decision on whether a message can be sent
  without blocking depends (among other things) on the state of flow
  control managed by the device, this approach also gives the device
  the greatest freedom in implementing flow control.  In particular,
  if another MPI process can change the flow control parameters, then
  even in a single-threaded implementation, it would not be safe to
  return, for example, a message size that could be sent with 'MPI_Bsend'.

  This routine allows an MPI implementation to optimize 'MPI_Bsend'
  for the case when the message can be delivered without blocking the
  calling process.  An ADI implementation is free to have this routine
  always return 'MPID_WOULD_BLOCK', but is encouraged not to.

  Question:
  Should there be a compile-time capability for this?  E.g., an
  'MPID_HAS_TBSEND' or should the device just do
.vb
  #define MPID_tBsend( a, b, c, d, e, f, g ) MPID_WOULD_BLOCK
.ve

  Module:
  Communication
  @*/
int MPID_tBsend( const void *buf, int count, MPID_Datatype *datatype,
		 int tag, int rank, MPID_Comm *comm, int context_id )
{
}

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

/*@
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

/*@
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

/*@
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

/*@
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

/*@
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

/*@
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
/*@
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

/*@
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

/*TStmOverview.tex
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

/*TTopoOverview.tex
 *
 * The MPI collective and topology routines can benefit from information 
 * about the topology of the underlying interconnect.  Unfortunately, there
 * is no best form for the representation (the MPI-1 Forum tried to define
 * such a representation, but was unable to).  One useful decomposition
 * that has been used in cluster enviroments is a hierarchical decomposition.
 *
 * The other obviously useful topology information would match the needs of 
 * 'MPI_Cart_create'.  However, it may be simpler to for the device to 
 * implement this routine directly.
 *
 * Other useful information could be the topology information that matches
 * the needs of the collective operation, such as spanning trees and rings.
 * These may be added to ADI3 later.
 *
 * Question: Should we define a cart create function?  Dims create?
 *
 * Usage:
 * This routine has nothing to do with the choice of communication method
 * that a implementation of the ADI may make.  It is intended only to
 * communicate information on the heirarchy of processes, if any, to 
 * the implementation of the collective communication routines.  This routine
 * may also be useful for the MPI Graph topology functions.
 *
 T*/

/*@
  MPID_Topo_cluster_info - Return information on the hierarchy of 
  interconnections

  Input Parameter:
. comm - Communicator to study.  May be 'NULL', in which case 'MPI_COMM_WORLD'
  is the effective communicator.

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

  The communicator argument allows this routine to be used in the dynamic
  process case (i.e., with communicators that are created after 'MPI_Init' 
  and that involve processes that are not part of 'MPI_COMM_WORLD').

  For non-hierarchical systems, this routine simply returns a single 
  level containing all processes.

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
int MPID_Topo_cluster_info( MPID_Comm *comm, 
			    int *levels, int my_cluster[], int my_rank[] )
{
}

/*
 * Section : Miscellaneous
 */

/*@
  MPID_Init - Initialize the device

  Input Parameters:
+ argc_p - Pointer to the argument count
. argv_p - Pointer to the argument list
- requested - Requested level of thread support.  Values are the same as
  for the 'required' argument to 'MPI_Init_thread', except that we define
  an enum for these values.

  Output Parameter:
+ provided - Provided level of thread support.  May be less than the 
  requested level of support.
. parent_group - MPID group of parent.  This is null for all MPI-1 uses and 
  for processes that are `not` started with 'MPI_Comm_spawn' or 
  'MPI_Comm_spawn_multiple'.
. has_args - Set to true if 'argc_p' and 'argv_p' contain the command
  line arguments.  See below.
- has_env  - Set to true if the environment of the process has been 
  set as the user expects.  See below.

  Return value:
  Returns '0' on success and an MPI error code on failure.  Failure can happen
  when, for example, the device is unable to start or contact the number of
  processes specified by the 'mpiexec' command.

  Notes:
  Null arguments for 'argc_p' and 'argv_p' `must` be valid (see MPI-2, section
  4.2)

  Multi-method devices should initialize each method within this call.
  They can use environment variables and/or command-line arguments
  to decide which methods to initialize (but note that they must not
  `depend` on using command-line arguments).

  This call also initializes all MPID datastructures.  Some of these (e.g.,
  'MPID_Request') may be in shared memory; others (e.g., 'MPID_Datatype') 
   may be allocated from an block of array values.  

  The arguments 'has_args' and 'has_env' indicates whether the process was
  started with command-line arguments or environment variables.  In some
  cases, only the ``root'' process is started with these values; in others, 
  the startup environment ensures that each process receives the 
  command-line arguments and environment variables that the user expects. 
  While the MPI standard makes no requirements that command line arguments or 
  environment variables are provided to all processes, most users expect a
  common environment.  These variables allow an MPI implementation (that is
  based on ADI-3) to provide both of these.

  This routine is used to implement both 'MPI_Init' and 'MPI_Init_thread'.

  Setting the environment requires a 'setenv' function.  Some
  systems may not have this.  In that case, the documentation must make 
  clear that the environment may not be propagated to the generated processes.

  Module:
  MPID_CORE

  Questions:

  The values for 'has_args' and 'has_env' are boolean.  
  They could be more specific.  For 
  example, the value could indicate the rank in 'MPI_COMM_WORLD' of a 
  process that has the values; the value 'MPI_ANY_SOURCE' (or a '-1') could
  indicate that the value is available on all processes (including this one).
  We may want this since otherwise the processes may need to determine whether
  any process needs the command line.  Another option would be to use positive 
  values in the same way that the 'color' argument is used in 'MPI_Comm_split';
  a negative value indicates the member of the processes with that color that 
  has the values of the command line arguments (or environment).  This allows
  for non-SPMD programs.

  Do we require that the startup environment (e.g., whatever 'mpiexec' is 
  using to start processes) is responsible for delivering
  the command line arguments and environment variables that the user expects?
  That is, if the user is running an SPMD program, and expects each process
  to get the same command line argument, who is responsible for this?  
  The 'has_args' and 'has_env' values are intended to allow the ADI to 
  handle this while taking advantage of any support that the process 
  manager framework may provide.

  Can we fix the Fortran command-line arguments?  That is, can we arrange for
  'iargc' and 'getarg' (and the POSIX equivalents) to return the correct 
  values?  See, for example, the Absoft implementations of 'getarg'.  
  We could also contact PGI about the Portland Group compilers, and of 
  course the 'g77' source code is available.
  Does each process have the same values for the environment variables 
  when this routine returns?

  If we don''t require that all processes get the same argument list, 
  we need to find out if they did anyway so that 'MPI_Init_thread' can
  fixup the list for the user.  This argues for another return value that
  flags how much of the environment the 'MPID_Init' routine set up
  so that the 'MPI_Init_thread' call can provide the rest.  The reason
  for this is that, even though the MPI standard does not require it, 
  a user-friendly implementation should, in the SPMD mode, give each
  process the same environment and argument lists unless the user 
  explicitly directed otherwise.

  How does this interface to BNR?  Do we need to know anything?  Should
  this call have an info argument to support BNR?

  The following questions involve how environment variables and command
  line arguments are used to control the behavior of the implementation. 
  Many of these values must be determined at the time that 'MPID_Init' 
  is called.  These all should be considered in the context of the 
  parameter routines described in the MPICH2 Design Document.

  Are there recommended environment variable names?  For example, in ADI-2,
  there are many debugging options that are part of the common device.
  In MPI-2, we can''t require command line arguments, so any such options
  must also have environment variables.  E.g., 'MPICH_ADI_DEBUG' or
  'MPICH_ADI_DB'.

  Names that are explicitly prohibited?  For example, do we want to 
  reserve any names that 'MPI_Init_thread' (as opposed to 'MPID_Init')
  might use?  

  How does information on command-line arguments and environment variables
  recognized by the device get added to the documentation?

  What about control for other impact on the environment?  For example,
  what signals should the device catch (e.g., 'SIGFPE'? 'SIGTRAP'?)?  
  Which of these should be optional (e.g., ignore or leave signal alone) 
  or selectable (e.g., port to listen on)?  For example, catching 'SIGTRAP'
  causes problems for 'gdb', so we''d like to be able to leave 'SIGTRAP' 
  unchanged in some cases.

  Another environment variable should control whether fault-tolerance is 
  desired.  If fault-tolerance is selected, then some collective operations 
  will need to use different algorithms and most fatal errors detected by the 
  MPI implementation should abort only the affected process, not all processes.
  @*/
int MPID_Init( int *argc_p, char *(*argv_p)[], 
	       int requested, int *provided,
	       MPID_Group **parent_group, int *has_args, int *has_env )
{
}

/*@
  MPID_Abort - Abort at least the processes in the specified communicator.

  Input Parameters:
+ comm        - Communicator of processes to abort
- return_code - Return code to return to the calling environment.  See notes.

  Return value:
  'MPI_SUCCESS' or an MPI error code.  Normally, this routine should not 
  return, since the calling process must be a member of the communicator.  
  However, under some circumstances, the 'MPID_Abort' might fail; in this 
  case, returning an error indication is appropriate.

  Notes:

  In a fault-tolerant MPI implementation, this operation should abort `only` 
  the processes in the specified communicator.  Any communicator that shares
  processes with the aborted communicator becomes invalid.  For more 
  details, see (paper not yet written on fault-tolerant MPI).

  In particular, if the communicator is 'MPI_COMM_SELF', only the calling 
  process should be aborted.

  The 'return_code' is the return code that this particular process will 
  attempt to provide to the 'mpiexec' or other program invocation 
  environment.  See 'mpiexec' for a discussion of how return codes from 
  many processes may be combined.

  An external agent that is aborting processes can invoke this with either
  'MPI_COMM_WORLD' or 'MPI_COMM_SELF'.  For example, if the process manager
  wishes to abort a group of processes, it should cause 'MPID_Abort' to 
  be invoked with 'MPI_COMM_SELF' on each process in the group.

  Question:
  An alternative design is to provide an 'MPID_Group' instead of a
  communicator.  This would allow a process manager to ask the ADI 
  to kill an entire group of processes without needing a communicator.
  However, the implementation of 'MPID_Abort' will either do this by
  communicating with other processes or by requesting the process manager
  to kill the processes.  That brings up this question: should 
  'MPID_Abort' use 'BNR' to kill processes?  Should it be required to
  notify the process manager?  What about persistent resources (such 
  as SYSV segments or forked processes)?  

  This suggests that for any persistent resource, an exit handler be
  defined.  These would be executed by 'MPID_Abort' or 'MPID_Finalize'.  
  See the implementation of 'MPI_Finalize' for an example of exit callbacks.
  In addition, code that registered persistent resources could use persistent
  storage (i.e., a file) to record that information, allowing cleanup 
  utilities (such as 'mpiexec') to remove any resources left after the 
  process exits.

  'MPI_Finalize' requires that attributes on 'MPI_COMM_SELF' be deleted 
  before anything else happens; this allows libraries to attach end-of-job
  actions to 'MPI_Finalize'.  It is valuable to have a similar 
  capability on 'MPI_Abort', with the caveat that 'MPI_Abort' may not 
  guarantee that the run-on-abort routines were called.  This provides a
  consistent way for the MPICH implementation to handle freeing any 
  persistent resources.  However, such callbacks must be limited since
  communication may not be possible once 'MPI_Abort' is called.  Further,
  any callbacks must guarantee that they have finite termination.  
  
  One possible extension would be to allow `users` to add actions to be 
  run when 'MPI_Abort' is called, perhaps through a special attribute value
  applied to 'MPI_COMM_SELF'.  Note that is is incorrect to call the delete 
  functions for the normal attributes on 'MPI_COMM_SELF' because MPI
  only specifies that those are run on 'MPI_Finalize' (i.e., normal 
  termination). 

  Module:
  MPID_CORE
  @*/
int MPID_Abort( MPID_Comm *comm, int return_code )
{}

/*@
  MPID_Finalize - Perform the device-specific termination of an MPI job

  Return Value:
  'MPI_SUCCESS' or a valid MPI error code.  Normally, this routine will
  return 'MPI_SUCCESS'.  Only in extrordinary circumstances can this
  routine fail; for example, if some process stops responding during the
  finalize step.  In this case, 'MPID_Finalize' should return an MPI 
  error code indicating the reason that it failed.

  Notes:

  Module:
  MPID_CORE

  Questions:
  Need to check the MPI-2 requirements on 'MPI_Finalize' with respect to
  things like which process must remain after 'MPID_Finalize' is called.
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
? MPID_Put_contig and MPID_Rhcv.

/*T
 * Section : Service Routines
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
  Instead, you must use 'MPIU_Malloc' etc.; if these are defined
  as 'malloc', that is allowed, but an explicit use of 'malloc' instead of
  'MPIU_Malloc' in the source code is not allowed.  This restriction is
  made to simplify the use of portable tools to test for memory leaks, 
  overwrites, and other consistency checks.

  Most memory should be allocated at the time that 'MPID_Init' is 
  called and released with 'MPID_Finalize' is called.  If at all possible,
  no other MPID routine should fail because memory could not be allocated
  (for example, because the user has allocated large arrays after 'MPI_Init').
  
  The implementation of the MPI routines will strive to avoid memory allocation
  as well; however, operations such as 'MPI_Type_index' that create a new
  data type that reflects data that must be copied from an array of arbitrary
  size will have to allocate memory (and can fail; note that there is an
  MPI error class for out-of-memory).

  Question:
  Do we want to have an aligned allocation routine?  E.g., one that
  aligns memory on a cache-line.
  D*/

/*@
  MPIU_Malloc - Allocate memory

  Input Parameter:
. len - Length of memory to allocate in bytes

  Return Value:
  Pointer to allocated memory, or null if memory could not be allocated.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Malloc(n) malloc(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Malloc(n) MPIU_trmalloc(n,__FILE__,__LINE__)
.ve
  where 'MPIU_trmalloc' is a tracing version of 'malloc' that is included with 
  MPICH.

  Module:
  Utility
  @*/
void *MPIU_Malloc( size_t len )
{
}

/*@
  MPIU_Free - Free memory

  Input Parameter:
. ptr - Pointer to memory to be freed.  This memory must have been allocated
  with 'MPIU_Malloc'.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPIU_Free(n) free(n)
.ve
  However, it can also be defined as 
.vb
  #define MPIU_Free(n) MPIU_trfree(n,__FILE__,__LINE__)
.ve
  where 'MPIU_trfree' is a tracing version of 'free' that is included with 
  MPICH.

  Module:
  Utility
  @*/
void MPIU_Free( void * ptr )
{
}

/*@
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

/*@
  MPIU_Calloc - Allocate memory that is initialized to zero.

  Input Parameters:
+ nelm - Number of elements to allocate
- elsize - Size of each element.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trcalloc' to provide a tracing version.

  Module:
  Utility
  @*/
void *MPIU_Calloc( size_t nelm, size_t elsize)
{
}
 
/*@ 
  MPIU_Strdup - Duplicate a string

  Input Parameter:
. str - null-terminated string to duplicate

  Return value:
  A pointer to a copy of the string, including the terminating null.  A
  null pointer is returned on error, such as out-of-memory.

  Notes:
  Like 'MPIU_Malloc' and 'MPIU_Free', this will often be implemented as a 
  macro but may use 'MPIU_trstrdup' to provide a tracing version.

  Module:
  Utility
  @*/
char *MPIU_Strdup( const char *str )
{
}

/*@
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
 * Timers
 */

/*@
  MPID_Wtime - Return a time stamp
  
  Output Parameter:
. timeval - A pointer to an 'MPID_Wtime_t' variable.

  Notes:
  This routine returns an `opaque` time value.  This difference between two
  time values returned by 'MPID_Wtime' can be converted into an elapsed time
  in seconds with the routine 'MPID_Wtime_diff'.

  This routine is defined this way to simplify its implementation as a macro.
  For example, for Intel x86 and gcc, 
.vb
#define MPID_Wtime(timeval) \
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (*timeval))
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

  Question:
  MPI-2 allows 'MPI_Wtime' to be a macro.  We should make that easy; this
  version does not accomplish that.
  @*/
void MPID_Wtime( MPID_Wtime_t *timeval )
{
}

/*@
  MPID_Wtime_diff - Compute the difference between two time stamps

  Input Parameters:
. t1, t2 - Two time values set by 'MPID_Wtime' on this process.
 

  Output Parameter:
. diff - The different in time between t2 and t1, measured in seconds.

  Note: 
  If 't1' is null, then 't2' is assumed to be differences accumulated with
  'MPID_Wtime_acc', and the output value gives the number of seconds that
  were accumulated.

  Question:
  Instead of handling a null value of 't1', should we have a separate
  routing 'MPID_Wtime_todouble' that converts a single timestamp to a 
  double value?  

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
  MPID_Wtime_init - Initialize the timer

  Note:
  This routine should perform any steps needed to initialize the timer.
  In addition, it should set the value of the attribute 'MPI_WTIME_IS_GLOBAL'
  if the timer is known to be the same for all processes in 'MPI_COMM_WORLD'
  (the value is zero by default).

  If any operations need to be performed when the MPI program calls 
  'MPI_Finalize' this routine should register a handler with 'MPI_Finalize'
  (see the MPICH Design Document).
  
  Module:
  Timer

  @*/
void MPID_Wtime_init( void )
{}

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

/*@
  MPID_Wtime_acc - Accumulate time values

  Input Parameters:
. t1,t2,t3 - Three time values.  't3' is updated with the difference between 
             't2' and 't1': '*t3 += (t2 - t1)'.

  Notes:
  This routine is used to accumulate the time spent with a block of code 
  without first converting the time stamps into a particular arithmetic
  type such as a 'double'.  For example, if the 'MPID_Wtime' routine accesses
  a cycle counter, this routine (or macro) can perform the accumulation using
  integer arithmetic.  

  To convert a time value accumulated with this routine, use 'MPID_Wtime_diff' 
  with a 't1' of zero.  

  Module:
  Timer
  @*/
void MPID_Wtime_acc( MPID_Time_t t1, MPID_Time_t t2, MPID_Time_t *t3 )
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

/*@
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
  'MPI_Comm_dup' and 'MPI_Type_dup'), as well as for the object free functions
  (e.g., 'MPI_Comm_free', 'MPI_Type_free', and 'MPI_Win_free').

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
  @*/
void *MPID_Attr_delete( MPID_List *list, int keyval )
{
}

/*TInfoOverview.tex

  'MPI_Info' provides a way to create a list of '(key,value)' pairs
  where the 'key' and 'value' are both strings.  Because many routines, both
  in the MPI implementation and in related APIs such as the BNR process
  management interface, require 'MPI_Info' arguments, we define a simple 
  structure for each 'MPI_Info' element.  Elements are allocated by the 
  generic object allocator; the head element is always empty (no 'key'
  or 'value' is defined on the head element).  
  
  The routines listed here assume that an info is just a linked list of 
  info items.  Another implementation would make 'MPI_Info' an 'MPID_List',
  and hang 'MPID_Info' records off of the list.  For simplicity, we have 
  not abstracted the info data structures; routines that want to work
  with the linked list may do so directly.  Because the 'MPI_Info' type is
  a handle and not a pointer, MPIU (utility) routines are provided to handle
  the allocation and deallocation of 'MPID_Info' elements.

  Thread Safety:

  The info interface itself is not thread-robust.  In particular, the routines
  'MPI_INFO_GET_NKEYS' and 'MPI_INFO_GET_NTHKEY' assume that no other 
  thread modifies the info key.  (If the info routines had the concept
  of a next value, they would not be thread safe.  As it stands, a user
  must be careful if several threads have access to the same info object.) 
  Further, 'MPI_INFO_DUP', while not 
  explicitly advising implementers to be careful of one thread modifying the
  'MPI_Info' structure while 'MPI_INFO_DUP' is copying it, requires that the
  operation take place in a thread-safe manner.
  There isn'' much that we can do about these cases.  There are other cases
  that must be handled.  In particular, multiple threads are allowed to 
  update the same info value.  Thus, all of the update routines must be thread
  safe; the simple implementation used in the MPICH implementation uses locks.
  Note that the 'MPI_Info_delete' call does not need a lock; the defintion of
  thread-safety means that any order of the calls functions correctly; since
  it invalid either to delete the same 'MPI_Info' twice or to modify an
  'MPI_Info' that has been deleted, only one thread at a time can call 
  'MPI_Info_free' on any particular 'MPI_Info' value.  

  T*/
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

/*@
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

/*T
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

/*TDyOverview.tex
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
/*@
  MPID_Err_create_code - Create an error code and associated message
  to report an error

  Input Parameters:
+ class - Error class
. generic_msg - A generic message to be used if not instance-specific
 message is available
. instance_msg - A message containing printf-style formatting commands
  that, when combined with the instance_parameters, specify an error
  message containing instance-specific data.
- instance_parameters - The remaining parameters.  These must match
 the formatting commands in 'instance_msg'.

 Notes:
 A typical use is\:
.vb
   mpi_errno = MPID_Err_create_code( MPI_ERR_RANK, "Invalid Rank",
                                "Invalid rank %d", rank );
.ve
 
  Predefined message may also be used.  Any message that uses the
  prefix '"**"' will be looked up in a table.  This allows standardized 
  messages to be used for a message that is used in several different locations
  in the code.  For example, the name '"**rank"' might be used instead of
  '"Invalid Rank"'; this would also allow the message to be made more
  specific and useful, such as 
.vb
   Invalid rank provided.  The rank must be between 0 and the 1 less than
   the size of the communicator in this call.
.ve
  
  This interface is compatible with the 'gettext' interface for 
  internationalization, in the sense that the 'generic_msg' and 'instance_msg' 
  may be used as arguments to 'gettext' to return a string in the appropriate 
  language; the implementation of 'MPID_Err_create_code' can then convert
  this text into the appropriate code value.

  Module:
  Error

  @*/
int MPID_Err_create_code( int class, const char *generic_msg, 
                          const char *instance_msg, ... )
{}

/*@
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

/*@
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

/*@
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

/*@
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

/*@
  MPID_Err_delete_class - Delete an error class and its associated string

  Input Parameter:
. class - Class to delete.
 
  Module:
  Error
  @*/
void MPID_Err_delete_class( int class )
{}

/*@
  MPID_Err_get_string - Get the message string that corresponds to an error
  class or code

  Input Parameter:
+ code - An error class or code.  If a code, it must have been created by 
  'MPID_Err_create_code'.
- msg_len - Length of 'msg'.

  Output Parameter:
. msg - A null-terminated text string of length (including the null) of no
  more than 'msg_len'.  

  Return value:
  Zero on success.  Non-zero returns indicate either (a) 'msg_len' is too
  small for the message or (b) the value of 'code' is neither a valid 
  error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.

  Module:
  Error 

  Question:
  What values should be used for the error returns?  Should they be
  valid error codes?

  How do we get a good value for 'MPI_MAX_ERROR_STRING' for 'mpi.h'?
  See 'errgetmsg' for one idea.

  @*/
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

/*@
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

/*@
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

/*@
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

