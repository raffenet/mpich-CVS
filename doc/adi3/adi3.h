/***********************************************************************
 * This is a DRAFT
 * All parts of this document are subject to (and expected to) change
 * This DRAFT dated August 30, 2000
 ***********************************************************************/

/*
 * Data Structures
 * These describe the *required* fields and there meanings for each structure.
 * A device implementation is free to extend these structures.
 *
 * Some fields are needed only if the device uses ADI3 support routines.
 * These are marked ?how?
 */

/*D
  MPI Opaque objects are specified by integers in the range [0,...].
  Out of range values are invalid; the value 0 is reserved for use as
  the MPI_xxx_NULL value.

  MPID objects are not opaque.
  D*/

/*D
  D*/
typedef struct { }MPID_List;
typedef enum { MPID_C, MPID_FORTRAN, MPID_CXX } MPID_Lang_t;
typedef struct {
    MPID_Lang_t language;           /* Language that created attribute */
    MPI_Aint    value;              /* Stored value */
    int         keyval;             /* Keyval for this attribute */
} MPID_Attribute;

/*D
  Processor mask 

  Allows quick determination whether a designated processor is within the
  set of active processes.

  Typical implementation is a bitvector.
  D*/
typedef struct {
} MPID_Procmask;


/*D
  MPID_Datatype - .
  D*/
typedef struct { 
    int           id;            /* value of MPI_Datatype for this structure */
    int           is_contig;     /* True if data is contiguous (even with 
				    a (count,datatype) pair) */
    MPID_List     attributes;

    int32_t       cache_id;      /* These are used to track which processes */
    MPID_Procmask mask;          /* have cached values of this datatype */
} MPID_Datatype;

/*D
 MPID_Group - .

 The processes in the group of 'MPI_COMM_WORLD' have pid values 0 to size-1,
 where size is the size of 'MPI_COMM_WORLD'.  Processes created by 
 'MPI_Comm_spawn', 'MPI_Comm_spawn_multiple', 'MPI_Attach', or 'MPI_Connect'
 are numbered greater than 'size - 1'.

 Note that when dynamic process creation is used, the pids are `not` unique
 across the universe of connected MPI processes.  This is ok, as long as
 pids are interpreted `only` on the process that owns them.
 D*/
typedef struct {
    int          size;          /* Size of a group */
    int          *lrank_to_pid; /* Array mapping a local rank to process 
				   number */
} MPID_Group;

/*D
  MPID_Comm - .
  D*/
typedef struct { 
    int16_t       context_id;    /* Assigned context id */
    int           size;          /* Value of MPI_Comm_(remote)_size */
    int           id;            /* value of MPI_Comm for this structure */
    MPID_List     attributes;    /* List of attributes */
    MPID_Group    *local_group,  /* Groups in communicator. */
	*remote_group;
} MPID_Comm;

/*D
  MPID_Win - .
  D*/
typedef struct {
    int  id;                     /* value of MPI_Win for this structure */
    void *start_address;         /* Address and length of *local* window */
    int  length;
    MPID_List     attributes;
} MPID_Win;

/*D
  MPID_Buffer - .
  D*/
typedef struct { 
    void *ptr;
    int  bytes;
    int  alloc_bytes;    /* For a receive buffer, this may > bytes */
    /* stuff to manage pack/unpack */
} MPID_Buffer;

/*D
  MPID_Request
  D*/
typedef struct {
    int ready;           /* Set to true when the request may be used */
} MPID_Request;

typedef enum { MPID_Hid_Request_to_send = 1, 
	       MPID_Hid_Cancel = 27,
               MPID_Hid_Put_emulation } MPID_Handler_id id;

/*
 * Handler Definitions
 */

/*D 
  MPID_Hid_Request_to_send

  struct {
  int32_t tag;
  int16_t context_id;
  int16_t lrank_sender, rank_dest;
  int16_t request_id;
  int32_t length;        /* In bytes */
  char    data[];        /* Message Payload */
  }
  D*/
/*D
   MPID_Hid_Cancel - Cancel a communication operation

   struct {
   int16_t request_id;
   }
  D*/
