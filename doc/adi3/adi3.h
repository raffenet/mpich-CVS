/* -*- Mode: C; c-basic-offset:4 ; -*- */
/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated September 24, 2001
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

/*TOpaqOverview.tex
  MPI Opaque Objects:

  MPI Opaque objects such as 'MPI_Comm' or 'MPI_Datatype' are specified by 
  integers (in the MPICH2 implementation); the MPI standard calls these
  handles.  
  Out of range values are invalid; the value 0 is reserved.
  For most (with the possible exception of 
  'MPI_Request' for performance reasons) MPI Opaque objects, the integer
  encodes both the kind of object (allowing runtime tests to detect a datatype
  passed where a communicator is expected) and important properties of the 
  object.  Even the 'MPI_xxx_NULL' values should be encoded so that 
  different null handles can be distinguished.  The details of the encoding
  of the handles is covered in more detail in the MPICH2 Design Document.
  For the most part, the ADI uses pointers to the underlying structures
  rather than the handles themselves.  However, each structure contains an 
  'id' field that is the corresponding integer handle for the MPI object.

  MPID objects (objects used within the implementation of MPI) are not opaque.

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
  Attribute-DS
  E*/
typedef enum { MPID_LANG_C, MPID_LANG_FORTRAN, 
	       MPID_LANG_CXX, MPID_LANG_FORTRAN90 } MPID_Lang_t;

/* Keyval functions - Data types for attribute copy and delete routines. */
/*TKyOverview.tex

  Keyvals are MPI objects that, unlike most MPI objects, are defined to be
  integers rather than a handle (e.g., 'MPI_Comm').  However, they really
  `are` MPI opaque objects and are handled by the MPICH implementation in
  the same way as all other MPI opaque objects.  The only difference is that
  there is no 'typedef int MPI_Keyval;' in 'mpi.h'.  In particular, keyvals
  are encoded (for direct and indirect references) in the same way that 
  other MPI opaque objects are

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
  errors return).  This is also used to indicate the type of MPI object a 
  MPI handle represents.

  Module:
  Attribute-DS
  E*/
typedef enum { 
  MPID_COMM       = 0x0, 
  MPID_GROUP      = 0x1,
  MPID_DATATYPE   = 0x2,
  MPID_FILE       = 0x3,
  MPID_ERRHANDLER = 0x4,
  MPID_OP         = 0x5,
  MPID_INFO       = 0x6,
  MPID_WIN        = 0x7,
  MPID_KEYVAL     = 0x8 
} MPID_Object_kind;

/*E
  MPID_Copy_function - MPID Structure to hold an attribute copy function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  The function types used in the union are provided by 'mpi.h'.  The
  correspond to\:
.vb
  int  (*C_CommCopyFunction)( MPI_Comm, int, void *, void *, void *, int * );
  int (*C_WinCopyFunction) ( MPI_Win, int, void *, void *, void *, int * );
  int (*C_TypeCopyFunction) ( MPI_Datatype, int, 
			      void *, void *, void *, int * );
.ve
  There are no corresponding typedefs for the Fortran functions.  The 
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Question:
  Do we want to create typedefs for the two Fortran functions?

  Module:
  Attribute-DS

  E*/
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

/*E
  MPID_Delete_function - MPID Structure to hold an attribute delete function

  Notes:
  The appropriate element of this union is selected by using the language
  field of the 'keyval'.

  The function types used in the union are provided by 'mpi.h'.  The
  correspond to\:
.vb
  int  (*C_DeleteFunction)  ( MPI_Comm, int, void *, void * );
  int  (*C_WinDeleteFunction)  ( MPI_Win, int, void *, void * );
  int  (*C_TypeDeleteFunction)  ( MPI_Datatype, int, void *, void * );
.ve
  There are no corresponding typedefs for the Fortran functions.  The 
  F77 function corresponds to the Fortran 77 binding used in MPI-1 and the
  F90 function corresponds to the Fortran 90 binding used in MPI-2.

  Module:
  Attribute-DS

  E*/
typedef union {
  MPI_Comm_delete_attr_function *C_CommDeleteFunction;
  MPI_Win_delete_attr_function  *C_WinDeleteFunction;
  MPI_Type_delete_attr_function *C_TypeDeleteFunction;
  void (*F77_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Fint *, MPI_Fint *, 
			      MPI_Fint * );
  void (*F90_DeleteFunction)( MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *, 
			      MPI_Fint * );
  
} MPID_Delete_function;

/*S
  MPID_Keyval - Structure of an MPID keyval

  Module:
  Attribute-DS

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
  Attributes don''t have 'ref_count's because they don''t have reference
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

  The attribute value is a 'void *'.  If 'sizeof(MPI_Fint)' > 'sizeof(void*)',
  then this must be changed (no such system has been encountered yet).
  For the Fortran 77 routines in the case where 'sizeof(MPI_Fint)' < 
  'sizeof(void*)', the high end of the 'void *' value is used.  That is,
  we cast it to 'MPI_Fint *' and use that value.
 
  Module:
  Attribute-DS

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

  A linked list is used because the typical 'MPI_Info' list will be short
  and a simple linked list is easy to implement and to maintain.  Similarly,
  a single structure rather than separate header and element structures are
  defined for simplicity.  No separate thread lock is provided because
  info routines are not performance critical; they use the 'common_lock' 
  in the 'MPIR_Process' structure when they need a thread lock.
  
  This particular form of linked list (in particular, with this particular
  choice of the first two members) is used because it allows us to use 
  the same routines to manage this list as are used to manage the 
  list of free objects (in the file 'src/util/mem/handlemem.c').  In 
  particular, if lock-free routines for updating a linked list are 
  provided, they can be used for managing the 'MPID_Info' structure as well.

  The MPI standard requires that keys can be no less that 32 characters and
  no more than 255 characters.  There is no mandated limit on the size 
  of values.

  Module:
  Info-DS
  S*/
typedef struct MPID_Info_s {
    int                id;
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

/*S
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
} MPID_Lpidmask;


/*
 * Each of the MPI datatypes can be mapped into one of 5 very simple
 * loops.  This loop has the following parameters:
 *    count  blocksize[] offset[]/stride datatype[]
 * where each [] indicates that a field may be *either* an array or a scalar.
 * For each such type, we define a struct that describes these parameters
 */

/*S
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

/*S
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

/*S
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

/*S
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

/*S
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

/*S
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
  Datatype-DS

  Notes:

  Alternatives:
  The following alternatives for the layout of this structure were considered.
  Most were not chosen because any benefit in performance or memory 
  efficiency was outweighed by the added complexity of the implementation.

  A number of fields contain only boolean inforation ('is_contig', 
  'has_mpi1_ub', 'has_mpi1_lb', 'is_permanent', 'is_committed').  These 
  could be combined and stored in a single bit vector.  

  'MPI_Type_dup' could be implemented with a shallow copy, where most of the
  data fields, particularly the 'opt_dataloop' field, would not be copied into
  the new object created by 'MPI_Type_dup'; instead, the new object could 
  point to the data fields in the old object.  However, this requires 
  more code to make sure that fields are found in the correct objects and that
  deleting the old object doesn't invalidate the dup'ed datatype.

  A related optimization would point to the 'opt_dataloop' and 'dataloop' 
  fields in other datatypes.  This has the same problems as the shallow 
  copy implementation.

  In addition to the separate 'dataloop' and 'opt_dataloop' fields, we could
  in addition have a separate 'hetero_dataloop' optimized for heterogeneous
  communication for systems with different data representations. 

  Earlier versions of the ADI used a single API to change the 'ref_count', 
  with each MPI object type having a separate routine.  Since reference
  count changes are always up or down one, and since all MPI objects 
  are defined to have the 'ref_count' field in the same place, the current
  ADI3 API uses two routines, 'MPID_Object_add_ref' and 
  'MPID_Object_release_ref', to increment and decrement the reference count.

  S*/
typedef struct { 
    int           id;            /* value of MPI_Datatype for this structure */
    volatile int  ref_count;
    int           is_contig;     /* True if data is contiguous (even with 
				    a (count,datatype) pair) */
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

    MPI_Aint      ub, lb,        /* MPI-1 upper and lower bounds */
                  true_ub, true_lb; /* MPI-2 true upper and lower bounds */
    MPID_List     attributes;    /* MPI-2 adds datatype attributes */

    int32_t       cache_id;      /* These are used to track which processes */
    MPID_Lpidmask mask;          /* have cached values of this datatype */

    char          name[MPI_MAX_OBJECT_NAME];  /* Required for MPI-2 */

  /* other, device-specific information */
} MPID_Datatype;

/*S
 MPID_Group - Description of the Group data structure

 The processes in the group of 'MPI_COMM_WORLD' have lpid values 0 to 'size'-1,
 where 'size' is the size of 'MPI_COMM_WORLD'.  Processes created by 
 'MPI_Comm_spawn' or 'MPI_Comm_spawn_multiple' or added by 'MPI_Comm_attach' 
 or  
 'MPI_Comm_connect'
 are numbered greater than 'size - 1' (on the calling process). See the 
 discussion of LocalPID values.

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
 Group-DS

 S*/
typedef struct {
    int          id;
    volatile int ref_count;
    int          size;           /* Size of a group */
    int          rank;           /* Rank of this process in this group */
    int          *lrank_to_lpid; /* Array mapping a local rank to local 
				    process number */
  /* other, device-specific information */
} MPID_Group;

/*E
  MPID_Errhander_fn - MPID Structure to hold an error handler function

  Notes:
  The MPI-1 Standard declared only the C version of this, implicitly 
  assuming that 'int' and 'MPI_Fint' were the same. 

  Module:
  ErrHand-DS
  
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

  Module:
  ErrHand-DS
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

  The virtual connection table is an explicit member of this structure.
  This contains the information used to contact a particular process,
  indexed by the rank relative to this communicator.

  Groups may be allocated lazily.  That is, the group pointers may be
  null, created only when needed by a routine such as 'MPI_Comm_group'.
  The local process ids needed to form the group are available within
  the virtual connection table.
  For intercommunicators, we may want to always have the groups.  If not, 
  we either need the 'local_group' or we need a virtual connection table
  corresponding to the 'local_group' (we may want this anyway to simplify
  the implementation of the intercommunicator collective routines).

  The pointer to the structure containing pointers to the collective 
  routines allows an implementation to replace each routine on a 
  routine-by-routine basis.  By default, this pointer is null, as are the 
  pointers within the structure.  If either pointer is null, the implementation
  uses the generic provided implementation.  This choice, rather than
  initializing the table with pointers to all of the collective routines,
  is made to reduce the space used in the communicators and to eliminate the
  need to include the implementation of all collective routines in all MPI 
  executables, even if the routines are not used.

  
  Module:
  Communicator-DS


  Question:
  For fault tolerance, do we want to have a standard field for communicator 
  health?  For example, ok, failure detected, all (live) members of failed 
  communicator have acked.
  S*/
typedef struct { 
    int           id;            /* value of MPI_Comm for this structure */
    volatile int ref_count;
    int16_t       context_id;    /* Assigned context id */
    int           size;          /* Value of MPI_Comm_(remote)_size */
    int           rank;          /* Value of MPI_Comm_rank */
    MPID_VC *(*virtural connection)[]; /* Virtual connection table */
    int           comm_kind;     /* MPID_INTRACOMM or MPID_INTERCOMM */
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

/*S
  MPID_Win - Description of the Window Object data structure.

  Module:
  Win-DS

  Notes:
  The following 3 keyvals are defined for attributes on all MPI 
  Window objects\:
.vb
 MPI_WIN_SIZE
 MPI_WIN_BASE
 MPI_WIN_DISP_UNIT
.ve
  These correspond to the values in 'length', 'start_address', and 
  'disp_unit'.

  The communicator in the window is the same communicator that the user
  provided to 'MPI_Win_create' (not a dup).  However, each intracommunicator
  has a special context id that may be used if MPI communication is used 
  by the implementation to implement the RMA operations.

  There is no separate window group; the group of the communicator should be
  used.

  Question:
  Should a 'MPID_Win' be defined after 'MPID_Segment' in case the device 
  wants to 
  store a queue of pending put/get operations, described with 'MPID_Segment'
  (or 'MPID_Request')s?

  S*/
typedef struct {
    int          id;             /* value of MPI_Win for this structure */
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

/*S
  MPID_Dataloop_stackelm - Structure for an element of the stack used
  to process datatypes

  Fields:
+ curcount - Current loop count value (between 0 and 
             loopinfo.loop_params.count-1) 
. curoffset - Offset for relative offsets in datatypes 
- loopinfo  - Loop-based description of the datatype

  Module:
  Datatype-DS
S*/
typedef struct {
    int           curcount;
    MPI_Aint      curoffset;
    MPID_Dataloop loopinfo;
} MPID_Dataloop_stackelm;

/*S
  MPID_Segment - Description of the Segment data structure

  Notes:
  This has no corresponding MPI datatype.

  The dataloop stack works as follows (the actual code will of course
  optimize the access to the stack elements and eliminate, for example,
  the various array references)\:
.vb
  cur_sp=valid_sp=0;
  stackelm[cur_sp].loopinfo  = datatype->loopinfo;
  stackelm[cur_sp].loopinfo.curcount = 0;
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
     else if stackelm[cur_sp].curcount == stackelm[cur_sp].loopinfo.cm_t.count
         then {
         // We are done with the datatype.
         cur_sp--;
         }
     else {
        // need to push a datatype.  Two cases: struct or other
        if (stackelm[cur_sp].loopinfo.kind == struct_type) {
           stackelm[cur_sp+1].loopinfo = 
           stackelm[cur_sp].loopinfo.s_t.dataloop[stackelm[cur_sp].curcount];
           }
        else {
           if (valid_sp <= cur_sp) {
               stackelm[cur_sp+1].loopinfo = 
               stackelm[cur_sp].loopinfo.cm_t.dataloop;
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
    int  cur_sp;   /* Current stack pointer when using loopinfo */
    int  valid_sp; /* maximum valid stack pointer.  This is used to 
		      maintain information on the stack after it has
		      been placed there by following the datatype field
		      in a MPID_Dataloop for any type except struct */

    /* other, device-specific information */
} MPID_Segment;

/*E
  MPID_Request_kind - Kinds of MPI Requests

  Module:
  Request-DS

  E*/
typedef { MPID_REQ_SEND, MPID_REQ_RECV, MPID_REQ_PERSISTENT_SEND, 
	      MPID_REQ_PERSISTENT_RECV, MPID_REQ_USER } MPID_Request_kind;

/*S
  MPID_Request - Description of the Request data structure

  Module:
  Request-DS

  Notes:
  If it is necessary to remember the MPI datatype, this information is 
  saved within the 'segment', not as part of a separate 'MPID_Datatype' 
  entry.
  
  S*/
typedef struct {
    int          id;      /* Value of MPI_Request for this structure */
    volatile int ref_count;
    volatile int busy;    /* Set to zero when the request is completed */
    MPID_Request_kind kind; /* Kind of request */
    /* The various types of requests may define subclasses for each 
       kind.  In particular, the user and persistent requests need
       special information */
    MPID_Segment segment;
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
/*E
  MPID_Op_kind - Enumerates types of MPI_Op types

  Notes:
  These are needed for implementing 'MPI_Accumulate', since only predefined
  operations are allowed for that operation.  

  A gap in the enum values was made allow additional predefined operations
  to be inserted.  This might include future additions to MPI or experimental
  extensions (such as a Read-Modify-Write operation).

  Module:
  Collective-DS
  E*/
typedef enum { MPID_OP_MAX=1, MPID_OP_MIN=2, MPID_OP_SUM=3, MPID_OP_PROD=4, 
	       MPID_OP_LAND=5, MPID_OP_BAND=6, MPID_OP_LOR=7, MPID_OP_BOR=8,
	       MPID_OP_LXOR=9, MPID_OP_BXOR=10, MPID_OP_MAXLOC=11, 
               MPID_OP_MINLOC=12, MPID_OP_REPLACE=13, 
               MPID_OP_USER_NONCOMMUTE=32, MPID_OP_USER=33 }
  MPID_Op_kind;

/*S
  MPID_User_function - Definition of a user function for MPI_Op types.

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
  Collective-DS
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
  All of the predefined functions are commutative.  Only user functions may 
  be noncummutative, so there are two separate op types for commutative and
  non-commutative user-defined operations.

  Operations do not require reference counts because there are no nonblocking
  operations that accept user-defined operations.  Thus, there is no way that
  a valid program can free an 'MPI_Op' while it is in use.

  Module:
  Collective-DS
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
  can be used in preprocessor tests.  These are defined within the 'mpi.h'
  file.

  Module:
  Environment-DS
  D*/

/*D
  MPID_MAX_THREAD_LEVEL - Indicates the maximum level of thread
  support provided at compile time.
 
  Values:
  Any of the 'MPI_THREAD_xxx' values (these are preprocessor-time constants)

  Notes:
  The macro 'MPID_MAX_THREAD_LEVEL' defines the maximum level of
  thread support provided, and may be used at compile time to remove
  thread locks and other code needed only in a multithreaded environment.

  A typical use is 
.vb
  #if MPID_MAX_THREAD_LEVEL >= MPI_THREAD_MULTIPLE
     lock((r)->lock_ptr);
     (r)->ref_count++;
     unlock((r)->lock_ptr);
  #else
     (r)->ref_count ++;
  #fi
.ve

  Module:
  Environment-DS
  D*/
#define MPID_MAX_THREAD_LEVEL 

/*
 * Function Prototypes go here
 */

