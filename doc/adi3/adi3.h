/* -*- Mode: C; c-basic-offset:4 ; -*- */
/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 24, 2001
 *
 * As of May 16, 2003, text is being moved into the "live" files so that
 * the documentation on the structures and interfaces stays current.
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
 *
 * I have removed some items from the documentation by inserting a space 
 * in the structured comment identifier.  If these are reintroduced, 
 * just delete the blanks.
 */


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

typedef enum { MPID_LANG_C, MPID_LANG_FORTRAN, 
	       MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;

/* Keyval functions - Data types for attribute copy and delete routines. */

typedef enum { 
  MPID_COMM       = 0x1, 
  MPID_GROUP      = 0x2,
  MPID_DATATYPE   = 0x3,
  MPID_FILE       = 0x4,
  MPID_ERRHANDLER = 0x5,
  MPID_OP         = 0x6,
  MPID_INFO       = 0x7,
  MPID_WIN        = 0x8,
  MPID_KEYVAL     = 0x9 
} MPID_Object_kind;

typedef union {
  MPI_Comm_copy_attr_function *C_CommCopyFunction;
  MPI_Win_copy_attr_function  *C_WinCopyFunction;
  MPI_Type_copy_attr_function *C_TypeCopyFunction;
  void (*F77_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint *, MPI_Fint *, MPI_Fint * );
  void (*F90_CopyFunction)  ( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
			      MPI_Aint *, MPI_Fint *, MPI_Fint * );
  /* The C++ function is the same as the C function */
} MPID_Copy_function;

typedef union {
  MPI_Comm_delete_attr_function *C_CommDeleteFunction;
  MPI_Win_delete_attr_function  *C_WinDeleteFunction;
  MPI_Type_delete_attr_function *C_TypeDeleteFunction;
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
			      MPI_Fint * );
  
} MPID_Delete_function;

typedef struct {
    int                  handle;
    volatile int         ref_count;
    MPID_Lang_t          language;
    MPID_Object_kind     kind;
    void                 *extra_state;
    MPID_Copy_function   copyfn;
    MPID_Delete_function delfn;
  /* other, device-specific information */
} MPID_Keyval;

typedef struct {
    void *      value;              /* Stored value */
    MPID_Keyval *keyval;            /* Keyval structure for this attribute */
    /* other, device-specific information */
} MPID_Attribute;

typedef struct MPID_Info_s {
    int                handle;
    struct MPID_Info_s *next;
    char               *key;
    char               *value;
} MPID_Info;

/*D
  LocalPID - Description of the local process ids

  MPI specifies communication among processes.  The MPI object that 
  describes collections of processes is the 'MPI_Group'.  In MPI-1, 
  processes are uniquely identified by their rank in 'MPI_COMM_WORLD'.
  In MPI-2, the situation is more complex, since processes (beyond
  'MPI_COMM_WORLD') may come and go as a result of calls to 
  routines such as 'MPI_Comm_spawn' and 'MPI_Comm_detach'.  To 
  implement the various operations on MPI groups, it is very helpful to
  have an enumeration of the processes.  This enumeration need not be 
  consequtive (gaps are acceptable) and it need not correspond to the 
  enumeration of the same processes on a different MPI process.  That is,
  such an enumeration need only be local.  These are called 'LocalPID's,
  and are allocated by the MPI routines that change the number of 
  processes that are connected (for most applications, a simple counter
  could be used).  

  Local process ids are used only by the 'MPI_Group' routines and for caching
  datatypes in the RMA routines.  Communication
  uses `virtual connections` to select the appropriate communication path 
  from one process to another.  This is different from the implementation 
  of MPICH using ADI-1 and ADI-2 where the mapping from communicator rank
  to rank in 'MPI_COMM_WORLD' (through the 'MPI_Group' associated with 
  a communicator) was used to identify the process with which to communicate.
  See the discussion of the 'MPI_Group' routines in the MPICH Design 
  Document for more details on the use of local process ids.  
  D*/

/* S
  MPID_Lpidmask - Description of the Processor mask data strucure

  Allows quick determination whether a designated processor is within the
  set of active processes.

  A typical implementation is a bitvector.

  This is used to help manage datatypes for remote memory operations (for
  efficient implementation of 'MPI_Put', 'MPI_Get', or 'MPI_Accumulate' 
  when using nontrivial datatypes, it is necessary to cache a copy of the 
  datatype on the remote process.  Using a processor mask makes it easy to 
  check whether a remote process already has a copy of a particular MPI 
  datatype).

  Note that groups can not use processor masks because groups need ordering
  information (e.g., pid 0 might have rank in the group of 245).

  This structure is manipulated with the routines\:
.vb
  void MPID_Lpid_set( MPID_Lpidmask *, int pid )
  void MPID_Lpid_init( MPID_Lpidmask * )
  int MPID_Lpid_isset( MPID_Lpidmask *, int pid )
  int MPID_Lpid_nextset( MPID_Lpidmask *, int lastpid ) 
.ve  

  Module:
  Group-DS
  S*/
typedef struct {
    /* other, device-specific information */
    /* Typically int pidbits[MPID_MAX_PROCS_BY_INT] */
} MPID_Lpidmask;


/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 *    count  blocksize[] offset[]/stride datatype[]
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/* S
  MPID_Dataloop_contig - Description of a contiguous datatype

  Fields:
+ count - Number of elements
- datatype - datatype that this datatype consists of

  Notes:
  'Count' may be in terms of the number of elements stored in the dataloop 
  'kind' field, particularly for leaf dataloops.

  Module:
  Datatype-DS
  S*/
typedef struct {
    int count;
    struct dataloop_ *dataloop;
} MPID_Dataloop_contig;

/* S
  MPID_Dataloop_vector - Description of a vector or strided datatype

  Fields:
+ count - Number of blocks
. blocksize - Number of datatypes in each block
. stride - Stride (in elements) between each block
- datatype - Datatype of each element

  Notes:
  'Stride' may be in terms of the number of elements stored in 
  the dataloop 'kind' field, particularly for leaf dataloops.

  Module:
  Datatype-DS
  S*/
typedef struct { 
    int      count;
    struct dataloop_ *dataloop;
    int      blocksize;
    MPI_Aint stride;
} MPID_Dataloop_vector;

/* S
  MPID_Dataloop_blockindexed - Description of a block-indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Number of elements in each block
. offset - Array of offsets (in elements) to each block
- datatype - Datatype of each element

  Notes:
  'Offset' and 'blocksize' may be in terms of the number of elements stored in 
  the dataloop 'kind' field, particularly for leaf dataloops.

  Module:
  Datatype-DS

  S*/
typedef struct {
    int      count;
    struct dataloop_ *dataloop;
    int      blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_blockindexed;

/* S
  MPID_Dataloop_indexed - Description of an indexed datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in elements) to each block
- datatype - Datatype of each element

  Notes:
  'Offset' and 'blocksize' may be in terms of the number of elements stored in 
  the dataloop 'kind' field, particularly for leaf dataloops.

  Module:
  Datatype-DS

  S*/
typedef struct {
    int      count;
    struct dataloop_ *dataloop;
    int      *blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_indexed;

/* S
  MPID_Dataloop_struct - Description of a structure datatype

  Fields:
+ count - Number of blocks
. blocksize - Array giving the number of elements in each block
. offset - Array of offsets (in elements) to each block
- datatype - Array of datatypes describing the elements of each block

  Notes:
  'Blocksize' and 'offset' may be in terms of the number of elements stored in 
  the dataloop 'kind' field, particularly for leaf dataloops.

  Module:
  Datatype-DS

  S*/
typedef struct {
    int      count;
    struct dataloop_ *dataloop;
    int      *blocksize;
    MPI_Aint *offset;
} MPID_Dataloop_struct;

/* S
  MPID_Dataloop - Description of the structure used to hold a datatype
  description

  Fields:
+  kind - Indicates what type of datatype.  May have the value
  'MPID_CONTIG', 'MPID_VECTOR', 'MPID_BLOCKINDEXED', 'MPID_INDEXED', or
  'MPID_STRUCT'.  
. loop_parms - A union containing the 5 dataloop structures, e.g., 
  'MPID_Dataloop_contig', 'MPID_Dataloop_vector', etc.  A sixth element in
  this union, 'count', allows quick access to the shared 'count' field in the
  five dataloop structure.
. extent - The extent of the datatype
- id     - id for the corresponding 'MPI_Datatype'.

  Module:
  Datatype-DS

  S*/
typedef struct dataloop_ { 
    int kind;                  /* Contains both the loop type 
				  (contig, vector, blockindexed, indexed,
				  or struct), a bit that indicates 
				  whether the datatype is a leaf type, and
				  the natural element size */
    union {
	int                        count;
	MPID_Dataloop_contig       c_t;
	MPID_Dataloop_vector       v_t;
	MPID_Dataloop_blockindexed bi_t;
	MPID_Dataloop_indexed      i_t;
	MPID_Dataloop_struct       s_t;
    } loop_params;
    MPI_Aint extent;
    int handle;                   /* Having the id here allows us to find the
				     full datatype structure from the 
				     Dataloop description */
} MPID_Dataloop;

typedef struct { 
    int           handle;        /* value of MPI_Datatype for this structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
				    a (count,datatype) pair) */
    MPID_Dataloop dataloop;      /* Describes the arguments that the
                                    user provided for creating the datatype;
				    these are used to implement the
				    MPI-2 MPI_Type_get_contents functions */
    int           size;
    MPI_Aint      extent;        /* MPI-2 allows a type to created by
				    resizing (the extent of) an existing 
				    type.  Note that the extent of the
				    datatype may be different from the
				    extent information in dataloop */

    /* The remaining fields are required but less frequently used, and
       are placed after the more commonly used fields */
    int           has_sticky_ub;   /* The MPI_UB and MPI_LB are sticky */
    int           has_sticky_lb;
    int           is_permanent;  /* e.g., MPI_DOUBLE*/
    int           is_committed;  /* See MPID_Datatype_commit */

    int           dataloop_depth; /* Depth of dataloop stack needed
				     to process this datatype.  This 
				     information is used to ensure that
				     no datatype is constructed that
				     cannot be processed (see MPID_Segment) */

    MPI_Aint      ub, lb,        /* MPI-1 upper and lower bounds */
                  true_ub, true_lb; /* MPI-2 true upper and lower bounds */
    MPID_List     attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    MPID_Lpidmask mask;          /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

  /* other, device-specific information */
} MPID_Datatype;

typedef struct {
    int          handle;
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          rank;           /* Rank of this process in this group */
    int          *lrank_to_lpid; /* Array mapping a local rank in this
                                    group to local process number */
  /* other, device-specific information */
} MPID_Group;

typedef union {
   void (*C_Comm_Handler_function) ( MPI_Comm *, int *, ... );
   void (*F77_Handler_function) ( MPI_Fint *, MPI_Fint *, ... );
   void (*C_Win_Handler_function) ( MPI_Win *, int *, ... );
   void (*C_File_Handler_function) ( MPI_File *, int *, ... );
} MPID_Errhandler_fn;

typedef struct {
  int                handle;
  volatile int       ref_count;
  MPID_Lang_t        language;
  MPID_Object_kind   kind;
  MPID_Errhandler_fn errfn;
  /* Other, device-specific information */
} MPID_Errhandler;

typedef enum { MPID_INTRACOMM = 0, MPID_INTERCOMM = 1 } MPID_Comm_kind_t;

typedef struct { 
    int           handle;        /* value of MPI_Comm for this structure */
    volatile int ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           remote_size;   /* Value of MPI_Comm_(remote)_size */
    int           local_size;    /* Value of MPI_Comm_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_VC *(*virtural connection)[]; /* Virtual connection table */
    MPID_Comm_kind_t comm_kind;  /* MPID_INTRACOMM or MPID_INTERCOMM */
    MPID_List     attributes;    /* List of attributes */
    MPID_Group    *local_group,  /* Groups in communicator. */
                  *remote_group; /* The local and remote groups are the
				    same for intra communicators */
    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */
    MPID_Errhandler *errhandler;  /* Pointer to the error handler structure */
    struct MPID_Collops_struct  *coll_fns;    /* Pointer to a table of 
						 functions implementing the 
						 collective routines */
  /* other, device-specific information */
#ifdef MPID_DEV_COMM_DECL
    MPID_DEV_COMM_DECL
#endif
} MPID_Comm;

typedef struct {
    int          handle;         /* value of MPI_Win for this structure */
    volatile int ref_count;
    void         *start_address; /* Address and length of *local* window */
    MPI_Aint     length;        
    int          disp_unit;      /* Displacement unit of *local* window */
    MPID_List    attributes;
    MPID_Comm    *comm;         /* communicator of window */
    char         name[MPI_MAX_OBJECT_NAME];  
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

/* S
  MPID_Dataloop_stackelm - Structure for an element of the stack used
  to process datatypes

  Fields:
+ curcount - Current loop count value (between 0 and 
             dataloop.loop_params.count-1) 
. curoffset - Offset for relative offsets in datatypes 
- dataloop  - Loop-based description of the datatype

  Module:
  Datatype-DS
S*/
typedef struct {
    int           curcount;
    MPI_Aint      curoffset;
    MPID_Dataloop dataloop;
} MPID_Dataloop_stackelm;

/* S
  MPID_Segment - Description of the Segment data structure

  Notes:
  This has no corresponding MPI object.

  The dataloop stack works as follows (the actual code will of course
  optimize the access to the stack elements and eliminate, for example,
  the various array references)\:
.vb
  cur_sp=valid_sp=0;
  stackelm[cur_sp].dataloop  = datatype->dataloop;
  stackelm[cur_sp].dataloop.curcount = 0;
  while (cur_sp >= 0) {
     if stackelm[cur_sp].dataloop.kind is final then
        // final means simple, consisting of basic datatypes, such 
        // as a vector datatype made up of bytes or doubles)
        process datatype (this uses dataloop.kind to pick the correct
            code fragments; we may also include some alignment tests
            so that longer word moves may be used for short (e.g.,
            one or two word) blocks).
	    We can also choose to stop and return here when, for example,
	    we have filled an output buffer.
        cur_sp--;
     else if stackelm[cur_sp].curcount == stackelm[cur_sp].dataloop.cm_t.count
         then {
         // We are done with the datatype.
         cur_sp--;
         }
     else {
        // need to push a datatype.  Two cases: struct or other
        if (stackelm[cur_sp].dataloop.kind == struct_type) {
           stackelm[cur_sp+1].dataloop = 
           stackelm[cur_sp].dataloop.s_t.dataloop[stackelm[cur_sp].curcount];
           }
        else {
           if (valid_sp <= cur_sp) {
               stackelm[cur_sp+1].dataloop = 
               stackelm[cur_sp].dataloop.cm_t.dataloop;
               valid_sp = cur_sp + 1;
           }
        }
        stackelm[cur_sp].curcount++;
        cur_sp ++;
        stackelm[cur_sp].curcount = 0;
     }
  }
.ve 

  This may look a little bit messy (and not all of the code is here),
  but it shows how 'valid_sp' is used to avoid recopying the
  information contained in a dataloop in the non-struct case.  For example, a
  vector of vector has the dataloop description read only once, not
  once for each count of the outer vector.

  Note that a relatively small value of 'MPID_MAX_DATATYPE_DEPTH', such as
  '16', will allow the processing of extremely complex datatypes that 
  describe huge amounts of memory.  Thus, an 'MPID_Segment' is a small 
  object.

  Module:
  Segment-DS

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
    MPID_Dataloop_stackelm loopstack[MPID_MAX_DATATYPE_DEPTH];
    int  cur_sp;   /* Current stack pointer when using dataloop */
    int  valid_sp; /* maximum valid stack pointer.  This is used to 
		      maintain information on the stack after it has
		      been placed there by following the datatype field
		      in a MPID_Dataloop for any type except struct */

    /* other, device-specific information */
} MPID_Segment;

typedef enum { MPID_REQ_SEND, MPID_REQ_RECV, MPID_REQ_PERSISTENT_SEND, 
	      MPID_REQ_PERSISTENT_RECV, MPID_REQ_USER } MPID_Request_kind;

typedef struct {
    int          handle;  /* Value of MPI_Request for this structure */
    volatile int ref_count;
    MPID_Request_kind kind; /* Kind of request */
    /* The various types of requests may define subclasses for each 
       kind.  In particular, the user and persistent requests need
       special information */
    volatile int cc;      /* Completion counter */
    int volatile *cc_ptr;
    /* Pointer to the completion counter associated with a group of requests.
       When the counter reaches zero, the group of requests is complete. */
    MPI_Status status;
    MPID_Segment segment;
    MPID_Comm    *comm;   /* Originating Comm; needed to find error
                             handler if necessary */
    /* other, device-specific information */
} MPID_Request;

/* E
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
  MPID_CORE-DS

  E */
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

/* S 
  MPID_Hid_Request_to_send_t - Handler type for point-to-point communication

  Notes: 
  This is an example of a message the may be sent using 'MPID_Rhcv'.

  The specific type lengths are not required; however, the type lengths
  used by the device must be consistent with the rest of the code.

  For example, if an 'int16_t' is used for tag, then the value of 
  'MPI_TAG_UB' must be no larger than 32K-1.

  Other fields may also be included.  For example, a message-sequence
  number is helpful for message-matching tools when there are multiple
  MPI threads.  Two other examples are  the hash value of the datatype 
  signature,   used to provide more complete error checking, and  
  a bit to indicate an rsend message so that "no posted
  receive" errors can be detected.

  Polling:
  This handler may be processed with polling.

  Module:
  MPID_CORE-DS

 S */
typedef struct {
  int32_t tag;
  int16_t context_id;
  int16_t lrank_sender, rank_dest;
  int16_t request_id;
  int32_t length;        /* In bytes */
  /* other, device-specific information */
} MPID_Hid_Request_to_send_t;

/* S
   MPID_Hid_Cancel_t - Cancel a communication operation

   Notes: 
   This is an example of a message the may be sent using 'MPID_Rhcv'.

  Polling:
  This handler may be processed with polling.  However, an implementation
  that does not require polling is prefered.

   Module:
   MPID_CORE-DS

  S */
typedef struct {
   int16_t request_id;
   /* other, device-specific information */
   } MPID_Hid_Cancel_t;

/*
 * Other Datastructures
 */
typedef enum { MPID_OP_MAX=1, MPID_OP_MIN=2, MPID_OP_SUM=3, MPID_OP_PROD=4, 
	       MPID_OP_LAND=5, MPID_OP_BAND=6, MPID_OP_LOR=7, MPID_OP_BOR=8,
	       MPID_OP_LXOR=9, MPID_OP_BXOR=10, MPID_OP_MAXLOC=11, 
               MPID_OP_MINLOC=12, MPID_OP_REPLACE=13, 
               MPID_OP_USER_NONCOMMUTE=32, MPID_OP_USER=33 }
  MPID_Op_kind;

typedef union {
    void (*c_function) ( const void restrict *, void restrict *, 
			 const int *, const MPI_Datatype * ); 
    void (f77_function) ( const void restrict *, void restrict *,
			  const MPI_Fint *, const MPI_Fint * );
} MPID_User_function;

typedef struct {
     MPID_Op_kind       kind;
     MPID_Lang_t        language;
     MPID_User_function function;
  } MPID_Op;

/*
 * Section x: Enviroment and Global Values
 */

/* D
  Constants - Description of constants defined by the device.

  The thread levels are 'define'd rather than enumerated so that they 
  can be used in preprocessor tests.  These are defined within the 'mpi.h'
  file.

  Module:
  Environment-DS
  D*/

#define MPID_MAX_THREAD_LEVEL 

/*
 * Function Prototypes go here
 */

