/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 22, 2000
 ***********************************************************************/

/*
 * Data Structures
 * These describe the *required* fields and their meanings for each structure.
 * A device implementation is free to extend these structures.
 * The `ordering` of the entries is `not` defined; a device may choose to
 * optimize cache performance by grouping the most commonly used fields
 * together near the top of the structure.
 *
 * Some fields are needed only if the device uses ADI3 support routines.
 * These are marked 'Friend' (no elements marked Friend yet).
 *
 * Question:
 * Do we want to mark parts 'Public' or 'Private'
 */

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects are specified by integers in the range [0,...] (in
  the MPICH2 implementation).
  Out of range values are invalid; the value 0 is reserved for use as
  the 'MPI_xxx_NULL' value.  In the debugging case, we may want to reserve
  0 for an invalid value and 1 for 'MPI_xxx_NULL'.

  MPID objects are not opaque.

  Question: Should they be?  More precisely, should some of them be?  
  For example, should 'MPID_List *' really be 'MPID_List_t', 
  with something like\:
.vb
 typedef struct MPID_List *MPID_List_t;
.ve
 so that the form of 'MPID_List' is isolated to the list management routines?
 That is, if the typedef is to a pointer to a structure, the internals of the
 structure can be left unspecified except to the routines that actually 
 manipulate them.  In the 'MPID_List' case, this would allow the implementation
 to use simple lists, hash tables, or HB trees, or skip lists without 
 affecting or needing to recompile any code that simply refered to 'MPID_List'.
 
 Current vote: No, no objects should be opaque.  However, more rationale 
 has been added and this should be revisited.

 
  T*/

/*
  Attributes and keyvals must be maintained.

  The definition of the list structure is made in the device to 
  simplify the construction of the data-structures that depend on
  it.  Most if not all devices will use the library code for
  managing these lists that is included with MPICH.
  */
typedef struct {
    /* other, device-specific information */
}MPID_List;

/*E
  MPID_Lang_t - Known language bindings for MPI

  A few operations in MPI need to know what language they were called from
  or created by.  This type enumerates the possible languages so that
  the MPI implementation can choose the correct behavior.  An example of this
  are the keyval attribute copy and delete functions.

  Question:
  Do we want to include Java or Ada, even though there is no official
  binding?  Do we want to specify values for these (e.g., 'MPID_LANG_C' is
  zero)?

  Module:
  Attribute
  E*/
typedef enum { MPID_LANG_C, MPID_LANG_FORTRAN, 
	       MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;

/* Keyval functions - Data types for attribute copy and delete routines. */
/*TKyOverview.tex

  Each keyval has a copy and a delete function associated with it.
  Unfortunately, these have a slightly different calling sequence for
  each language, particularly when the size of a pointer is 
  different from the size of a Fortran integer.  The unions 
  'MPID_Copy_function' and 'MPID_Delete_function' capture the differences
  in a single union type.

  Notes:
  One potential user error is to access an attribute in one language (say
  Fortran) that was created in another (say C).  We could add a check and
  generate an error message in this case; note that this would have to 
  be an option, because (particularly when accessing the attribute from C), 
  it may be what the user intended, and in any case, it is a valid operation.

  T*/

/*E
  MPID_Object_kind - Object kind (communicator, window, or file)

  Notes:
  This enum is used by keyvals and errhandlers to indicate the type of
  object for which MPI opaque types the data is valid.  These are defined
  as bits to allow future expansion to the case where an object is value for
  multiple types (for example, we may want a universal error handler for 
  errors return).
  E*/
typedef enum { 
  MPID_COMM=1, MPID_WIN=2, MPID_FILE=4, MPID_DATATYPE=8 } MPID_Object_kind;

/*E
  MPID_Copy_function - MPID Structure to hold an attribute copy function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Question:
  Do we want to create typedefs for the various functions?

  Module:
  Attribute

  E*/
typedef union {
  int  (*C_CommCopyFunction)( MPI_Comm, int, void *, void *, void *, int * );
  void (*F77_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint *, MPI_Fint *, MPI_Fint * );
  void (*F90_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
			      MPI_Aint *, MPI_Fint *, MPI_Fint * );
  int (*C_FileCopyFunction) ( MPI_Comm, int, void *, void *, void *, int * );
  int (*C_TypeCopyFunction) ( MPI_Datatype, int, 
			      void *, void *, void *, int * );
  /* The C++ function is the same as the C function */
} MPID_Copy_function;

/*E
  MPID_Delete_function - MPID Structure to hold an attribute delete function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  Module:
  Attribute

  E*/
typedef union {
  int  (*C_DeleteFunction)  ( MPI_Comm, int, void *, void * );
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
			      MPI_Fint * );
  int  (*C_FileDeleteFunction)  ( MPI_File, int, void *, void * );
  int  (*C_TypeDeleteFunction)  ( MPI_Datatype, int, void *, void * );
  
} MPID_Delete_function;

/*S
  MPID_Keyval - Structure of an MPID keyval

  Module:
  Attribute

  Question:
  Because this structure contains pointers to user-functions, there is always
  the danger that a user-error that overwrites the memory locations storing 
  the function pointers.  Should there be sentinals around either the entire
  keyval entry or around each function pointer that would be tested before 
  invoking the function?  
  S*/
typedef struct {
    int                  id;
    volatile int         ref_count;
    MPID_Lang_t          language;
    MPID_Object_kind     kind;
    void                 *extra_state;
    MPID_Copy_function   copyfn;
    MPID_Delete_function delfn;
  /* other, device-specific information */
} MPID_Keyval;

/*S
  MPID_Attribute - Structure of an MPID attribute

  Notes:
  Attributes don''t have ref_counts because they don''t have reference
  count semantics.  That is, there are no shallow copies or duplicates
  of an attibute.  An attribute is copied when the communicator that
  it is attached to is duplicated.  Subsequent operations, such as
  'MPI_Comm_attr_free', can change the attribute list for one of the
  communicators but not the other, making it impractical to keep the
  same list.  (We could defer making the copy until the list is changed,
  but even then, there would be no reference count on the individual
  attributes.)
 
  A pointer to the keyval, rather than the (integer) keyval itself is
  used since there is no need within the attribute structure to make
  it any harder to find the keyval structure.
 
  Module:
  Attribute

 S*/
typedef struct {
    void *      value;              /* Stored value */
    MPID_Keyval *keyval;            /* Keyval structure for this attribute */
    /* other, device-specific information */
} MPID_Attribute;

/*S
  MPID_Info - Structure of an MPID info

  Notes:
  There is no reference count because 'MPI_Info' values, unlike other MPI 
  objects, may be changed after they are passed to a routine without 
  changing the routine''s behavior.  In other words, any routine that uses
  an 'MPI_Info' object must make a copy or otherwise act on any info value
  that it needs.

  Module:
  Attribute

  Question:
  Do we really want to mandate that this is a linked list, or do we want
  to make use of the 'MPID_List'?

  Do we want to have a separate info header and info element?  We may want 
  that inorder to implement the access locks needed by a multithreaded 
  implementation.
  S*/
typedef struct MPID_Info_s {
    int                id;
    char               *key;
    char               *value;
    struct MPID_Info_s *next;
} MPID_Info;

/*D
  LocalPID - Description of the local process ids

  (yet to do: write text describing local pids.  These are basically the 
  id by which the calling process knows the other processes, and is not
  guaranteed to be number as a different process may use to identify
  the same (third) process.  This is the reason that this is a `local`
  process id.

  Question:
  Do we want to have a local pid type that maps a virtual local process 
  number to a specific process or link?
  D*/

/*S
  MPID_Lpidmask - Description of the Processor mask data strucure

  Allows quick determination whether a designated processor is within the
  set of active processes.

  Typical implementation is a bitvector.

  This is used to help manage datatypes for remote memory operations.

  Note that groups can not use these because groups need ordering
  information (e.g., pid 0 might have rank in the group of 245).

  This structure is manipulated with the routines\:
.vb
  void MPID_Lpid_set( MPID_Lpidmask *, int pid )
  void MPID_Lpid_init( MPID_Lpidmask * )
  int MPID_Lpid_isset( MPID_Lpidmask *, int pid )
  int MPID_Lpid_nextset( MPID_Lpidmask *, int lastpid ) 
.ve  

  Module:
  Group
  S*/
typedef struct {
    /* other, device-specific information */
} MPID_Lpidmask;


/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 *    count  blocksize[] offset[]/stride datatype[]
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/*S
  MPID_Datatype_contig - Description of a contiguous datatype

  Fields:
+ count - Number of elements
- datatype - datatype that this datatype consists of

  Module:
  Datatype
  S*/
typedef struct {
    int count;
    struct dataloop_ *datatype;
} MPID_Datatype_contig;

/*S
  MPID_Datatype_vector - Description of a vector or strided datatype

  Fields:
+ count - Number of elements
. blocksize - Number of datatypes in each element
. stride - Stride (in bytes) between each block
- datatype - Datatype of each element

  Module:
  Datatype
  S*/
typedef struct { 
    int count;
    int blocksize;
    int stride;
    struct dataloop_ *datatype;
} MPID_Datatype_vector;

/*S
  MPID_Datatype_blockindexed - Description of a block-indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Datatype of each element

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    int blocksize;
    int *offset;
    struct dataloop_ *datatype;
} MPID_Datatype_blockindexed;

/*S
  MPID_Datatype_indexed - Description of an indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Datatype of each element

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    int *blocksize;
    int *offset;
    struct dataloop_ *datatype;
} MPID_Datatype_indexed;

/*S
  MPID_Datatype_struct - Description of a structure datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in bytes) to each block
- datatype - Array of datatypes describing the elements of each block

  Module:
  Datatype

  S*/
typedef struct {
    int count;
    int *blocksize;
    int *offset;
    struct dataloop_ *datatype;
} MPID_Datatype_struct;

/*S
  MPID_Dataloop - Description of the structure used to hold a datatype
  description

  Fields:
+  kind - Indicates what type of datatype.  May have the value
  'MPID_Contig', 'MPID_Vector', 'MPID_BlockIndexed', 'MPID_Indexed', or
  'MPID_Struct'.  
. loop_parms - A union containing the 5 datatype structures, e.g., 
  'MPID_Datatype_contig', 'MPID_Datatype_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five datatype structure.
. extent - The extent of the datatype
- id     - id for the corresponding 'MPI_Datatype'.

  Module:
  Datatype

  S*/
typedef struct datatloop_ { 
    int kind;                  /* Contains both the loop type 
				  (contig, vector, blockindexed, indexed,
				  or struct) and a bit that indicates 
				  whether the datatype is a leaf type. */
    union {
	int                        count;
	MPID_Datatype_contig       c_t;
	MPID_Datatype_vector       v_t;
	MPID_Datatype_blockindexed bi_t;
	MPID_Datatype_indexed      i_t;
	MPID_Datatype_struct       s_t;
    } loop_params;
    MPI_Aint extent;
    int id;                       /* Having the id here allows us to find the
				     full datatype structure from the 
				     Dataloop description */
} MPID_Dataloop;

/*S
  MPID_Datatype - Description of the MPID Datatype structure

  Notes:
  The 'ref_count' is needed for nonblocking operations such as
.vb
   MPI_Type_struct( ... , &newtype );
   MPI_Irecv( buf, 1000, newtype, ..., &request );
   MPI_Type_free( &newtype );
   ...
   MPI_Wait( &request, &status );
.ve

  Module:
  Datatype

  Question:
  For some of the boolean information (e.g., is_contig), should we just use
  a single bit to help reduce the size of the structure.

  For 'MPI_Type_dup', we may want to do a shallow copy.  For example, 
  if the original type was created with 'MPI_Type_indexed' with a large, 
  user-provided array, then we''d like not to copy that large array.
  That arguments for another pointer: 'MPID_Datatype *dup_parent;' 
  When 'MPI_Type_dup' is executed, a new datatype is created and setup, but
  the dup_parent is set to the previous datatype (or its parent if it has 
  one).  The cost is that all (non-basic) datatype operations involve another
  test (do I have a parent?).

  For datatypes built from other datatypes, do I want to copy the loop
  information from the old datatypes?  This is the 'MPID_Dataloop'
  information (e.g., precopy the entire dataloop stack)?

  We may want to have separate 'dataloop' fields for heterogeneous and
  homogeneous communication, along with a dataloop field used to 
  implement fast pack and unpack operations.  These would be 
  among the device-dependent fields.

  We may want to place the booleans like 'has_mpi1_ub' into a bit-vector.
  Question: do we want to define a type like 'boolean' instead of 'int'
  for values that are true/false?

  S*/
typedef struct { 
    int           id;            /* value of MPI_Datatype for this structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
				    a (count,datatype) pair) */
    int           is_perm;       /* True if datatype is a predefined type */
    MPID_Dataloop loopinfo;      /* Describes the arguments that the
                                    user provided for creating the datatype;
				    these are used to implement the
				    MPI-2 MPI_Type_get_contents functions */
    int           size;
    MPI_Aint      extent;        /* MPI-2 allows a type to created by
				    resizing (the extent of) an existing 
				    type.  Note that the extent of the
				    datatype may be different from the
				    extent information in loopinfo */

    /* The remaining fields are required but less frequently used, and
       are placed after the more commonly used fields */
    int           has_mpi1_ub;   /* The MPI_UB and MPI_LB are sticky */
    int           has_mpi1_lb;
    int           is_permanent;  /* e.g., MPI_DOUBLE*/
    int           is_committed;  /* See MPID_Datatype_commit */

    int           loopinfo_depth; /* Depth of dataloop stack needed
				     to process this datatype.  This 
				     information is used to ensure that
				     no datatype is constructed that
				     cannot be processed (see MPID_Segment) */

    MPID_List     attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    MPID_Lpidmask mask;          /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

  /* The following describes a generate datatype */
  /* other, device-specific information */
} MPID_Datatype;

/*S
 MPID_Group - Description of the Group data structure

 The processes in the group of 'MPI_COMM_WORLD' have lpid values 0 to size-1,
 where size is the size of 'MPI_COMM_WORLD'.  Processes created by 
 'MPI_Comm_spawn', 'MPI_Comm_spawn_multiple', 'MPI_Attach', or 'MPI_Connect'
 are numbered greater than 'size - 1' (on the calling process).

 Note that when dynamic process creation is used, the pids are `not` unique
 across the universe of connected MPI processes.  This is ok, as long as
 pids are interpreted `only` on the process that owns them.

 Only for MPI-1 are the lpid''s equal to the `global` pids.  The local pids
 can be thought of as a reference not to the remote process itself, but
 how the remote process can be reached from this process.  We may want to 
 have a structure 'MPID_Lpid_t' that contains information on the remote
 process, such as (for TCP) the hostname, ip address (it may be different if
 multiple interfaces are supported; we may even want plural ip addresses for
 stripping communication), and port (or ports).  For shared memory connected
 processes, it might have the address of a remote queue.  The lpid number 
 is an index into a table of 'MPID_Lpid_t'''s that contain this (device- and
 method-specific) information.

 Module:
 Group

 Questions:
 Do we want a rank of this process in the group (if any)?
 S*/
typedef struct {
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          *lrank_to_lpid; /* Array mapping a local rank to local 
				    process number */
  /* other, device-specific information */
} MPID_Group;

/*E
  MPID_Errhander_fn - MPID Structure to hold an error handler function

  Notes:
  The MPI-1 Standard declared only the C version of this, implicitly 
  assuming that 'int' and 'MPI_Fint' were the same. 

  Questions:
  What do we want to do about C++?  Do we want a hook for a routine that can
  be called to throw an exception in C++, particularly if we give C++ access
  to this structure?  Does the C++ handler need to be different (not part
  of the union)?

  What is the interface for the Fortran version of the error handler?  
  E*/
typedef union {
   void (*C_Comm_Handler_function) ( MPI_Comm *, int *, ... );
   void (*F77_Handler_function) ( MPI_Fint *, MPI_Fint *, ... );
   void (*C_Win_Handler_function) ( MPI_Win *, int *, ... );
   void (*C_File_Handler_function) ( MPI_File *, int *, ... );
} MPID_Errhandler_fn;

/*S
  MPID_Errhandler - Description of the error handler structure

  Notes:
  Device-specific information may indicate whether the error handler is active;
  this can help prevent infinite recursion in error handlers caused by 
  user-error without requiring the user to be as careful.  We might want to 
  make this part of the interface so that the 'MPI_xxx_call_errhandler' 
  routines would check.

  It is useful to have a way to indicate that the errhandler is no longer
  valid, to help catch the case where the user has freed the errhandler but
  is still using a copy of the 'MPI_Errhandler' value.  We may want to 
  define the 'id' value for deleted errhandlers.
  S*/
typedef struct {
  int                id;
  volatile int       ref_count;
  MPID_Lang_t        language;
  MPID_Object_kind   kind;
  MPID_Errhandler_fn errfn;
  /* Other, device-specific information */
} MPID_Errhandler;

/*S
  MPID_Comm - Description of the Communicator data structure

  Notes:
  Note that the size (and possibly rank) duplicate data in the groups that
  make up this communicator.  These are used often enough that this
  optimization is valuable.
  We may also want the local-rank to lpid mapping to be included as well,
  skipping the indirection through the (remote) group (using a pointer to
  the same storage).

  Module:
  Communicator

  Question:
  Do we want a communicator type (intra or inter) or do we use
  'comm->local_group == comm->remote_group'?

  Do we want to have the collective operations pointer here?
  Do we want to optimize for the common case of "use the standard
  routines"?  We could do this by having a pointer to a table (with its
  own reference count so that we can do a lazy copy), and in the null-pointer
  case, we use the default routines.  The advantage of this, besides the
  reduction in space used by communicators, is that a small MPI application
  need not load all of the collective routines.

  Do we want to make the context id available to other tools?  For example,
  having a global context id makes message matching for tools like Jumpshot 
  `much` easier.  In fact, the ADI could provide a function to provide a 
  unique context id, which most implementations would implement directly from 
  the given id.  Note that this is a unique, not a globally unique, id, since 
  the value needs only be unique relative to the processes that can share in 
  communication.  In other words, only the tuple (source/destination, 
  context_id) must identify a unique communicator at each MPI process.

  For fault tolerance, do we want to have a standard field for communicator 
  health?  For example, ok, failure detected, all (live) members of failed 
  communicator have acked.
  S*/
typedef struct { 
    volatile int ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           size;          /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    int           id;            /* value of MPI_Comm for this structure */
    MPID_List     attributes;    /* List of attributes */
    MPID_Group    *local_group,  /* Groups in communicator. */
                  *remote_group; /* The local and remote groups are the
				    same for intra communicators */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
  /* other, device-specific information */
    int           is_singlemethod; /* An example, device-specific field,
				      this is used in a multi-method
				      device to indicate that all processes
				      in this communicator belong to the
				      same method */
} MPID_Comm;

/*S
  MPID_Win - Description of the Window Object data structure.

  Module:
  Win

  Question:
  Should a win be defined after 'MPID_Segment' in case the device wants to 
  store a queue of pending put/get operations, described with 'MPID_Segment'
  (or 'MPID_Request')s?
   
  What is the communicator of the window?  Is this the same communicator passed
  to 'MPI_Win_create'?  Is it a (shallow) copy; that is, a dup without 
  carrying the (user) attributes?

  Should there be a separate 'MPID_Group', or will we use the group of 
  the communicator?
  S*/
typedef struct {
    int          id;             /* value of MPI_Win for this structure */
    volatile int ref_count;
    void         *start_address; /* Address and length of *local* window */
    MPID_Aint    length;
    MPID_List    attributes;
    MPID_Comm    *comm;         /* communicator of window */
    char         name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    /* other, device-specific information 
       This is likely to include a list of pending RMA operations
       (i.e., MPI_Put, MPI_Get, and MPI_Accumulate)
     */
} MPID_Win;

/* 
   The max datatype depth is the maximum depth of the stack used to 
   evaluate datatypes.  It represents the length of the chain of 
   datatype dependencies.  Defining this and testing when a datatype
   is created removes a test for the datatype evaluation loop. 
 */
#define MPID_MAX_DATATYPE_DEPTH 8

/*S
  MPID_Dataloop_stackelm - Structure for an element of the stack used
  to process datatypes

  Fields:
+ curcount - Current loop count value (between 0 and 
             loopinfo.loop_params.count-1) 
. curoffset - Offset for relative offsets in datatypes 
- loopinfo  - Loop-based description of the datatype

S*/
typedef struct {
    int           curcount;
    MPI_Aint      curoffset;
    MPID_Dataloop loopinfo;
} MPID_Dataloop_stackelm;

/*S
  MPID_Segment - Description of the Segment datatype

  Notes:
  This has no corresponding MPI datatype.

  The dataloop stack works as follows (the actual code will of course
  optimize the access to the stack elements and eliminate, for example,
  the various array references)\:
.vb
  cur_sp=valid_sp=0;
  stackelm.loopinfo[cur_sp].cur_count = 0;
  stackelm.loopinfo[cur_sp].loopinfo  = datatype->loopinfo;
  while (cur_sp >= 0) {
     if stackelm[cur_sp].loopinfo.kind is final then
        // final means simple, consisting of basic datatypes, such 
        // as a vector datatype made up of bytes or doubles)
        process datatype (this uses loopinfo.kind to pick the correct
            code fragments; we may also include some alignment tests
            so that longer word moves may be used for short (e.g.,
            one or two word) blocks).
	    We can also choose to stop and return here when, for example,
	    we have filled an output buffer.
        cur_sp--;
     else if stackelm[cur_sp].cur_count == stackelm[cur_sp].loopinfo.cm_t.count
         then {
         // We are done with the datatype.
         cur_sp--;
         }
     else {
        // need to push a datatype.  Two cases: struct or other
        if (stack_elm[cur_sp].loopinfo.kind == struct_type) {
           stack_elm[cur_sp+1].loopinfo = 
           stack_elm[cur_sp].loopinfo.s_t.dataloop[stackelm[cur_sp].cur_count];
           }
        else {
           if (valid_sp <= cur_sp) {
               stack_elm[cur_sp+1].loopinfo = 
               stack_elm[cur_sp].loopinfo.cm_t.dataloop;
               valid_sp = cur_sp + 1;
           }
        }
        stack_elm[cur_sp].cur_count++;
        cur_sp ++;
        stack_elm[cur_sp].cur_count = 0;
     }
  }
.ve 

  This may look a little bit messy (and not all of the code is here),
  but it shows how 'valid_sp' is used to avoid recopying the
  information on a datatype in the non-struct case.  For example, a
  vector of vector has the datatype description read only once, not
  once for each count of the outer vector.

  Note that a relatively small value of 'MPID_MAX_DATATYPE_DEPTH', such as
  '16', will allow the processing of extremely complex datatypes that 
  describe huge amounts of memory.  Thus, an 'MPID_Segment' is a small 
  object.

  Module:
  Segment

  Questions:
  Should this have an id for allocation and similarity purposes?

  Do we really need to specify the contents of the 'MPID_Segment'
  structure?  

  Should we allow the 'MPID_Dataloop_stackelm' to be a pointer, so 
  that we can allocate a larger stack?
  S*/
typedef struct { 
    void *ptr;
    int  bytes;
    int  alloc_bytes;    /* For a receive buffer, this may > bytes */

    /* stuff to manage pack/unpack */
    MPID_Dataloop_stackelm loopinfo[MPID_MAX_DATATYPE_DEPTH];
    int  cur_sp;   /* Current stack pointer when using loopinfo */
    int  valid_sp; /* maximum valid stack pointer.  This is used to 
		      maintain information on the stack after it has
		      been placed there by following the datatype field
		      in a MPID_Dataloop for any type except struct */

    /* other, device-specific information */
} MPID_Segment;

/*S
  MPID_Request - Description of the Request data structure

  Module:
  Request

  Question:
  Do we need an 'MPID_Datatype *' to hold the datatype in the event of a 
  nonblocking (and not yet completed) operation involving a complex datatype,
  or do we need a pointer to an 'MPID_Buffer'?  Or is it device-specific?
  S*/
typedef struct {
    volatile int ready;   /* Set to true when the request may be used */
    volatile int complete; /* Set to true when the request is completed */
    /* other, device-specific information */
} MPID_Request;

/*E
  Handlers - Description of the remote handlers and their arguments
  
  Enumerate the possible Remote Handler Call types.  For each of these
  handler id values, there is a corresponding structure of 'type <name>_t';
  e.g., for handler id 'MPID_Hid_Cancel', there is a typedef defining
  'MPID_Hid_Cancel_t'.

  Notes:
  This is a typedef enum that lists the possible call types
  for 'MPID_Rhcv'.  The exact list will depend on the generic implementation 
  of the other (i.e., not 'MPID_CORE') modules.

  Module:
  MPID_CORE

  E*/
typedef enum { MPID_Hid_Request_to_send = 1, 
	       ... many other predefined handler ids ...,
	       MPID_Hid_Cancel,
	       MPID_Hid_Define_Datatype,
	       MPID_Hid_Lock_Request,
	       MPID_Hid_Lock_Grant,
	       MPID_Hid_Unlock,
               MPID_Hid_Put_emulation } MPID_Handler_id id;

/*
 * Handler Definitions
 */

/*S 
  MPID_Hid_Request_to_send_t - Handler type for point-to-point communication

  Notes: 
  This is an example of a message the may be sent using 'MPID_Rhcv'.

  The specific type lengths are not required; however, the type lengths
  used by the device must be consistent with the rest of the code.

  For example, if an 'int16_t' is used for tag, then the value of 
  'MPI_TAG_UB' must be no larger than 32K-1.

  Other fields may also be included.  For example, a message-sequence
  number is helpful for message-matching tools when there are multiple
  MPI threads.  Another example is the hash value of the datatype signature,
  used to provide more complete error checking. 

  Polling:
  This handler may be processed with polling.

  Module:
  MPID_CORE

  Question: 
  Should there be a bit to indicate an rsend message so that "no posted
  receive" errors can be detected?

 S*/
typedef struct {
  int32_t tag;
  int16_t context_id;
  int16_t lrank_sender, rank_dest;
  int16_t request_id;
  int32_t length;        /* In bytes */
  /* other, device-specific information */
} MPID_Hid_Request_to_send_t;

/*S
   MPID_Hid_Cancel_t - Cancel a communication operation

   Notes: 
   This is an example of a message the may be sent using 'MPID_Rhcv'.

  Polling:
  This handler may be processed with polling.  However, an implementation
  that does not require polling is prefered.

   Module:
   MPID_CORE

  S*/
typedef struct {
   int16_t request_id;
   /* other, device-specific information */
   } MPID_Hid_Cancel_t;

/*
 * Other Datastructures
 */
/*E
  MPID_Op_kind - Enumerates types of MPI_Op types

  Notes:
  These are needed for implementing 'MPI_Accumulate', since only predefined
  operations are allowed for that operation.  

  A gap in the enum values was made allow additional predefined operations
  to be inserted.  This might include future additions to MPI or experimental
  extensions (such as a Read-Modify-Write operation).

  Module:
  Win  
  E*/
typedef enum { MPID_MAX_OP=1, MPID_MIN_OP=2, MPID_SUM_OP=3, MPID_PROD_OP=4, 
	       MPID_LAND_OP=5, MPID_BAND_OP=6, MPID_LOR_OP=7, MPID_BOR_OP=8,
	       MPID_LXOR_OP=9, MPID_BXOR_OP=10, MPID_MAXLOC_OP=11, 
               MPID_MINLOC_OP=12, MPID_REPLACE_OP=13, 
               MPID_USER_COMMUTE_OP=32, MPID_USER_OP=33 }
  MPID_Op_kind;

/*S
  MPID_User_function - Definition of a user function for MPI_OP types.

  Notes:
  This includes a 'const' to make clear which is the 'in' argument and 
  which the 'inout' argument, and to indicate that the 'count' and 'datatype'
  arguments are unchanged (they are addresses in an attempt to allow 
  interoperation with Fortran).  It includes 'restrict' to emphasize that 
  no overlapping operations are allowed.

  We need to include a Fortran version, since those arguments will
  have type 'MPI_Fint *' instead.  We also need to add a test to the
  test suite for this case; in fact, we need tests for each of the handle
  types to ensure that the transfered handle works correctly.

  This is part of the collective module because user-defined operations
  are valid only for the collective computation routines and not for 
  RMA accumulate.

  Yes, the 'restrict' is in the correct location.  C compilers that 
  support 'restrict' should be able to generate code that is as good as a
  Fortran compiler would for these functions.

  We should note on the manual pages for user-defined operations that
  'restrict' should be used when available, and that a cast may be 
  required when passing such a function to 'MPI_Op_create'.

  Question:
  Should each of these function types have an associated typedef?

  Should there be a C++ function here?

  Module:
  Collective
  S*/
typedef union {
    void (*c_function) ( const void restrict *, void restrict *, 
			 const int *, const MPI_Datatype * ); 
    void (f77_function) ( const void restrict *, void restrict *,
			  const MPI_Fint *, const MPI_Fint * );
} MPID_User_function;

/*S
  MPID_Op - MPI_Op structure

  Notes:
  All of the predefined functions are cummutative.  Only user functions may 
  be noncummutative, so that property is included in the 'MPID_Op_kind'.

  Operations do not require reference counts because there are no nonblocking
  operations that accept user-defined operations.  Thus, there is no way that
  a valid program can free an 'MPI_Op' while it is in use.

  Module:
  Win
  S*/
typedef struct {
     MPID_Op_kind       kind;
     MPID_Lang_t        language;
     MPID_User_function function;
  } MPID_Op;

/*
 * Section x: Enviroment and Global Values
 */

/*D
  Constants - Description of constants defined by the device.

  The thread levels are 'define'd rather than enumerated so that they 
  can be used in preprocessor tests.

  Question: 
  Do we want enumerated values as well?

  Module:
  Environment
  D*/
#define MPID_THREAD_SINGLE     0
#define MPID_THREAD_FUNNELLED  1
#define MPID_THREAD_SERIALIZED 2
#define MPID_THREAD_MULTIPLE   3

/*D
  MPID_MAX_THREAD_LEVEL - Indicates the maximum level of thread
  support provided at compile time.
 
  Values:
  Any of the 'MPI_THREAD_xxx' values (but as preprocessor-time constants)

  Notes:
  The macro 'MPID_MAX_THREAD_LEVEL' defines the maximum level of
  thread support provided, and may be used at compile time to remove
  thread locks and other code needed only in a multithreaded environment.

  A typical use is 
.vb
  #if MPID_MAX_THREAD_LEVEL >= MPID_THREAD_MULTIPLE
     lock((r)->lock_ptr);
     (r)->ref_count++;
     unlock((r)->lock_ptr);
  #else
     (r)->ref_count ++;
  #fi
.ve

  Module:
  Environment
  D*/
#define MPID_MAX_THREAD_LEVEL 

/*
 * Function Prototypes go here
 */

