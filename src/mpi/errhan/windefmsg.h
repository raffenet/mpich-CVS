/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This file automatically created by extracterrmsgs
 * DO NOT EDIT
 */
typedef struct msgpair {
        const unsigned int sentinal1;
        const char *short_name, *long_name; 
        const unsigned int sentinal2; } msgpair;
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
/* The names are in sorted order, allowing the use of a simple
  linear search or bisection algorithm to find the message corresponding to
  a particular message */
static const char short_gen0[] = "**CreateFileMapping";
static const char long_gen0[]  = "CreateFileMapping failed";
static const char short_gen1[] = "**CreateThread";
static const char long_gen1[]  = "CreateThread failed";
static const char short_gen2[] = "**FindWindowEx";
static const char long_gen2[]  = "FindWindowEx failed";
static const char short_gen3[] = "**GetMemTwice";
static const char long_gen3[]  = "Global shared memory initializer called more than once";
static const char short_gen4[] = "**MPIDI_CH3I_SHM_Attach_to_mem";
static const char long_gen4[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed";
static const char short_gen5[] = "**MPIU_Strdup";
static const char long_gen5[]  = "MPIU_Strdup failed";
static const char short_gen6[] = "**MapViewOfFileEx";
static const char long_gen6[]  = "MapViewOfFileEx failed";
static const char short_gen7[] = "**OpenProcess";
static const char long_gen7[]  = "OpenProcess failed";
static const char short_gen8[] = "**abort";
static const char long_gen8[]  = "application called MPI_ABORT";
static const char short_gen9[] = "**allocmem";
static const char long_gen9[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short_gen10[] = "**arg";
static const char long_gen10[]  = "Invalid argument";
static const char short_gen11[] = "**argarrayneg";
static const char long_gen11[]  = "Negative value in array ";
static const char short_gen12[] = "**argerrcode";
static const char long_gen12[]  = "Invalid error code";
static const char short_gen13[] = "**argneg";
static const char long_gen13[]  = "Invalid argument; must be non-negative";
static const char short_gen14[] = "**argnonpos";
static const char long_gen14[]  = "Invalid argument; must be positive";
static const char short_gen15[] = "**argrange";
static const char long_gen15[]  = "Argument is not within valid range";
static const char short_gen16[] = "**argstr_hostd";
static const char long_gen16[]  = "no space for the host description";
static const char short_gen17[] = "**argstr_port";
static const char long_gen17[]  = "no space for the listener port";
static const char short_gen18[] = "**argstr_shmhost";
static const char long_gen18[]  = "no space for the host name";
static const char short_gen19[] = "**argstr_shmq";
static const char long_gen19[]  = "no space for the shared memory queue name";
static const char short_gen20[] = "**assert";
static const char long_gen20[]  = "Invalid assert argument";
static const char short_gen21[] = "**attach_to_mem";
static const char long_gen21[]  = "attach to shared memory segment failed";
static const char short_gen22[] = "**attrsentinal";
static const char long_gen22[]  = "Internal fields in an attribute have been overwritten; possible errors in using the attribute value in user code.";
static const char short_gen23[] = "**bad_conn";
static const char long_gen23[]  = "bad conn structure pointer";
static const char short_gen24[] = "**bad_set";
static const char long_gen24[]  = "bad set parameter";
static const char short_gen25[] = "**bad_sock";
static const char long_gen25[]  = "bad sock";
static const char short_gen26[] = "**badpacket";
static const char long_gen26[]  = "Received a packet of unknown type";
static const char short_gen27[] = "**base";
static const char long_gen27[]  = "Invalid base address";
static const char short_gen28[] = "**boot_attach";
static const char long_gen28[]  = "failed to attach to a bootstrap queue";
static const char short_gen29[] = "**boot_create";
static const char long_gen29[]  = "unable to create a bootstrap message queue";
static const char short_gen30[] = "**boot_detach";
static const char long_gen30[]  = "detaching from message queue failed";
static const char short_gen31[] = "**boot_recv";
static const char long_gen31[]  = "receiving bootstrap message failed";
static const char short_gen32[] = "**boot_send";
static const char long_gen32[]  = "sending bootstrap message failed";
static const char short_gen33[] = "**boot_tostring";
static const char long_gen33[]  = "unable to get a string representation of the boostrap queue";
static const char short_gen34[] = "**boot_unlink";
static const char long_gen34[]  = "unable to unlink the shared memory message queue";
static const char short_gen35[] = "**bootqmsg";
static const char long_gen35[]  = "invalid bootstrap queue message size";
static const char short_gen36[] = "**bsendbufsmall";
static const char long_gen36[]  = "Buffer size is smaller than MPI_BSEND_OVERHEAD";
static const char short_gen37[] = "**bsendnobuf";
static const char long_gen37[]  = "No buffer to detach. ";
static const char short_gen38[] = "**buf_inplace";
static const char long_gen38[]  = "buffer cannot be MPI_IN_PLACE";
static const char short_gen39[] = "**bufalias";
static const char long_gen39[]  = "Buffers must not be aliased";
static const char short_gen40[] = "**bufbsend";
static const char long_gen40[]  = "Insufficient space in Bsend buffer";
static const char short_gen41[] = "**bufexists";
static const char long_gen41[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short_gen42[] = "**buffer";
static const char long_gen42[]  = "Invalid buffer pointer";
static const char short_gen43[] = "**bufnull";
static const char long_gen43[]  = "Null buffer pointer";
static const char short_gen44[] = "**buscard";
static const char long_gen44[]  = "unable to create a business card";
static const char short_gen45[] = "**buscard_len";
static const char long_gen45[]  = "no space left in the business card to add a parameter";
static const char short_gen46[] = "**business_card";
static const char long_gen46[]  = "Invalid business card";
static const char short_gen47[] = "**ca_guids";
static const char long_gen47[]  = "unable to get the guids from the infiniband access layer";
static const char short_gen48[] = "**cancelunknown";
static const char long_gen48[]  = "Attempt to cancel an unknown type of request";
static const char short_gen49[] = "**cartcoordinvalid";
static const char long_gen49[]  = "Cartesian coordinate is invalid (not in range)";
static const char short_gen50[] = "**cartdim";
static const char long_gen50[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short_gen51[] = "**ch3_finalize";
static const char long_gen51[]  = "Channel finalization failed";
static const char short_gen52[] = "**ch3_init";
static const char long_gen52[]  = "Channel init failed";
static const char short_gen53[] = "**ch3progress";
static const char long_gen53[]  = "Unable to make message passing progress";
static const char short_gen54[] = "**ch3|badca";
static const char long_gen54[]  = "specified completion action in not known";
static const char short_gen55[] = "**ch3|badmsgtype";
static const char long_gen55[]  = "request contained an invalid message type";
static const char short_gen56[] = "**ch3|badreqtype";
static const char long_gen56[]  = "request contained an invalid request type";
static const char short_gen57[] = "**ch3|canceleager";
static const char long_gen57[]  = "failure occurred while performing local cancellation of a eager message";
static const char short_gen58[] = "**ch3|cancelreq";
static const char long_gen58[]  = "failure occurred while sending remote cancellation request packet";
static const char short_gen59[] = "**ch3|cancelresp";
static const char long_gen59[]  = "failure occurred while attempting to send cancel response packet";
static const char short_gen60[] = "**ch3|cancelrndv";
static const char long_gen60[]  = "failure occurred while performing local cancellation of a rendezvous message";
static const char short_gen61[] = "**ch3|ctspkt";
static const char long_gen61[]  = "failure occurred while attempting to send CTS packet";
static const char short_gen62[] = "**ch3|eagermsg";
static const char long_gen62[]  = "failure occurred while attempting to send an eager message";
static const char short_gen63[] = "**ch3|flowcntlpkt";
static const char long_gen63[]  = "UNIMPLEMENTED: unable to handle flow control packets";
static const char short_gen64[] = "**ch3|loadrecviov";
static const char long_gen64[]  = "failure occurred while loading the receive I/O vector";
static const char short_gen65[] = "**ch3|loadsendiov";
static const char long_gen65[]  = "failure occurred while loading the send I/O vector";
static const char short_gen66[] = "**ch3|nopktcontainermem";
static const char long_gen66[]  = "failed to allocate memory for a packet reorder container";
static const char short_gen67[] = "**ch3|ooocancelreq";
static const char long_gen67[]  = "UNIMPLEMENTED: unable to process out-of-order cancellation requests";
static const char short_gen68[] = "**ch3|pktordered";
static const char long_gen68[]  = "failure occurred while processing a reordered packet";
static const char short_gen69[] = "**ch3|postrecv";
static const char long_gen69[]  = "failure occurred while posting a receive for message data";
static const char short_gen70[] = "**ch3|recvdata";
static const char long_gen70[]  = "failure occurred while attempting to receive message data";
static const char short_gen71[] = "**ch3|rmamsg";
static const char long_gen71[]  = "failure occurred while attempting to send an RMA message";
static const char short_gen72[] = "**ch3|rtspkt";
static const char long_gen72[]  = "failure occurred while attempting to send RTS packet";
static const char short_gen73[] = "**ch3|selfsenddeadlock";
static const char long_gen73[]  = "DEADLOCK: attempting to send a message to the local process without a prior matching receive";
static const char short_gen74[] = "**ch3|senddata";
static const char long_gen74[]  = "failure occurred while attempting to send message data";
static const char short_gen75[] = "**ch3|sock|accept";
static const char long_gen75[]  = "[ch3:sock] accepting a new connection failed";
static const char short_gen76[] = "**ch3|sock|addrinuse";
static const char long_gen76[]  = "[ch3:sock] tcp port already in use";
static const char short_gen77[] = "**ch3|sock|badbuscard";
static const char long_gen77[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen78[] = "**ch3|sock|badhost";
static const char long_gen78[]  = "[ch3:soc] invalid host description";
static const char short_gen79[] = "**ch3|sock|badpacket";
static const char long_gen79[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen80[] = "**ch3|sock|badsock";
static const char long_gen80[]  = "[ch3:sock] internal error - bad sock";
static const char short_gen81[] = "**ch3|sock|connalloc";
static const char long_gen81[]  = "[ch3:sock] unable to allocate an internal connection structure";
static const char short_gen82[] = "**ch3|sock|connallocfailed";
static const char long_gen82[]  = "[ch3:sock] unable to allocate a connection structure";
static const char short_gen83[] = "**ch3|sock|connclose";
static const char long_gen83[]  = "[ch3:sock] active connection unexpectedly closed";
static const char short_gen84[] = "**ch3|sock|connfailed";
static const char long_gen84[]  = "[ch3:sock] failed to connnect to remote process";
static const char short_gen85[] = "**ch3|sock|connrefused";
static const char long_gen85[]  = "[ch3:sock] connection refused";
static const char short_gen86[] = "**ch3|sock|connterm";
static const char long_gen86[]  = "[ch3:sock] active connection unexpectedly terminated";
static const char short_gen87[] = "**ch3|sock|failure";
static const char long_gen87[]  = "[ch3:sock] unknown failure";
static const char short_gen88[] = "**ch3|sock|hostlookup";
static const char long_gen88[]  = "[ch3:sock] hostname lookup failed";
static const char short_gen89[] = "**ch3|sock|open_lrecv_data";
static const char long_gen89[]  = "[ch3:sock] failed to handle open lrecv data packet";
static const char short_gen90[] = "**ch3|sock|openlsend";
static const char long_gen90[]  = "[ch3:sock] failed to handle open lsend packet";
static const char short_gen91[] = "**ch3|sock|pmi_finalize";
static const char long_gen91[]  = "PMI_Finalize failed";
static const char short_gen92[] = "**ch3|sock|postconnect";
static const char long_gen92[]  = "[ch3:sock] post connect failed";
static const char short_gen93[] = "**ch3|sock|postread";
static const char long_gen93[]  = "attempt to post a read operation failed";
static const char short_gen94[] = "**ch3|sock|postwrite";
static const char long_gen94[]  = "attempt to post a write operation failed";
static const char short_gen95[] = "**ch3|sock|progress_finalize";
static const char long_gen95[]  = "[ch3:sock] progress_finalize failed";
static const char short_gen96[] = "**ch3|sock|scconnaccept";
static const char long_gen96[]  = "[ch3:sock] failed to handle connection accept packet";
static const char short_gen97[] = "**ch3|sock|scopenresp";
static const char long_gen97[]  = "[ch3:sock] failed to handle open response packet";
static const char short_gen98[] = "**ch3|sock|strdup";
static const char long_gen98[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen99[] = "**ch3|syncack";
static const char long_gen99[]  = "failure occurred while attempting to send eager synchronization packet";
static const char short_gen100[] = "**ch3|unknownpkt";
static const char long_gen100[]  = "received unknown packet type";
static const char short_gen101[] = "**comm";
static const char long_gen101[]  = "Invalid communicator";
static const char short_gen102[] = "**commnotinter";
static const char long_gen102[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen103[] = "**commnotintra";
static const char long_gen103[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen104[] = "**commnull";
static const char long_gen104[]  = "Null communicator";
static const char short_gen105[] = "**commperm";
static const char long_gen105[]  = "Cannot free permanent communicator";
static const char short_gen106[] = "**conn_still_active";
static const char long_gen106[]  = "connection closed while still active";
static const char short_gen107[] = "**connfailed";
static const char long_gen107[]  = "Failed to connect to remote process";
static const char short_gen108[] = "**connrefused";
static const char long_gen108[]  = "Connection refused";
static const char short_gen109[] = "**conversion";
static const char long_gen109[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen110[] = "**count";
static const char long_gen110[]  = "Invalid count";
static const char short_gen111[] = "**countneg";
static const char long_gen111[]  = "Negative count";
static const char short_gen112[] = "**darrayblock";
static const char long_gen112[]  = "Value of m must be positive for block(m) distribution";
static const char short_gen113[] = "**darrayblock2";
static const char long_gen113[]  = "m * nprocs is less than array_size and is not valid for block(m) distribution (optional arguments: {m*nprocs}, {array_size} (int))";
static const char short_gen114[] = "**darraycyclic";
static const char long_gen114[]  = "Value of m must be positive for a cyclic(m) distribution";
static const char short_gen115[] = "**darraydist";
static const char long_gen115[]  = "For MPI_DISTRIBUTE_NONE, the number of processes in that dimension of the grid must be 1";
static const char short_gen116[] = "**darrayunknown";
static const char long_gen116[]  = "Unknown distribution type ";
static const char short_gen117[] = "**datarep";
static const char long_gen117[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen118[] = "**datarepunsupported";
static const char long_gen118[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen119[] = "**desc_len";
static const char long_gen119[]  = "host description buffer too small";
static const char short_gen120[] = "**dims";
static const char long_gen120[]  = "Invalid dimension argument";
static const char short_gen121[] = "**dimsmany";
static const char long_gen121[]  = "Number of dimensions is too large ";
static const char short_gen122[] = "**dimspartition";
static const char long_gen122[]  = "Cannot partition nodes as requested ";
static const char short_gen123[] = "**dtype";
static const char long_gen123[]  = "Invalid datatype";
static const char short_gen124[] = "**dtypecommit";
static const char long_gen124[]  = "Datatype has not been committed ";
static const char short_gen125[] = "**dtypemismatch";
static const char long_gen125[]  = "Receiving data with a datatype whose signature does not match that of the sending datatype.";
static const char short_gen126[] = "**dtypenull";
static const char long_gen126[]  = "Null datatype";
static const char short_gen127[] = "**dtypeperm";
static const char long_gen127[]  = "Cannot free permanent data type ";
static const char short_gen128[] = "**duphandle";
static const char long_gen128[]  = "unable to duplicate a handle";
static const char short_gen129[] = "**dupprocesses";
static const char long_gen129[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen130[] = "**edgeoutrange";
static const char long_gen130[]  = "Edge index in graph topology is out of range";
static const char short_gen131[] = "**errhandnotfile";
static const char long_gen131[]  = "Error handler is not a file error handler";
static const char short_gen132[] = "**errhandnotwin";
static const char long_gen132[]  = "Error handler is not a win error handler";
static const char short_gen133[] = "**fail";
static const char long_gen133[]  = "failure";
static const char short_gen134[] = "**file";
static const char long_gen134[]  = "Invalid MPI_File";
static const char short_gen135[] = "**fileaccess";
static const char long_gen135[]  = "Access denied to file";
static const char short_gen136[] = "**fileamode";
static const char long_gen136[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen137[] = "**fileamodeone";
static const char long_gen137[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen138[] = "**fileamoderead";
static const char long_gen138[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen139[] = "**fileamodeseq";
static const char long_gen139[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen140[] = "**fileexist";
static const char long_gen140[]  = "File exists";
static const char short_gen141[] = "**fileinuse";
static const char long_gen141[]  = "File in use by some process";
static const char short_gen142[] = "**filename";
static const char long_gen142[]  = "Invalid file name";
static const char short_gen143[] = "**filenamedir";
static const char long_gen143[]  = "Invalid or missing directory";
static const char short_gen144[] = "**filenamelong";
static const char long_gen144[]  = "Pathname too long";
static const char short_gen145[] = "**filenoexist";
static const char long_gen145[]  = "File does not exist";
static const char short_gen146[] = "**filenospace";
static const char long_gen146[]  = "Not enough space for file ";
static const char short_gen147[] = "**fileopunsupported";
static const char long_gen147[]  = "Unsupported file operation ";
static const char short_gen148[] = "**filequota";
static const char long_gen148[]  = "Quota exceeded for files";
static const char short_gen149[] = "**filerdonly";
static const char long_gen149[]  = "Read-only file or filesystem name";
static const char short_gen150[] = "**finalize_boot";
static const char long_gen150[]  = "destroying the message queue failed";
static const char short_gen151[] = "**finalize_progress";
static const char long_gen151[]  = "finalizing the progress engine failed";
static const char short_gen152[] = "**finalize_progress_finalize";
static const char long_gen152[]  = "Progress finalize failed";
static const char short_gen153[] = "**finalize_release_mem";
static const char long_gen153[]  = "Release shared memory failed";
static const char short_gen154[] = "**finish_qp";
static const char long_gen154[]  = "unable to establish a queue pair connection";
static const char short_gen155[] = "**ftok";
static const char long_gen155[]  = "failed to create a sysv key from a file name";
static const char short_gen156[] = "**ftruncate";
static const char long_gen156[]  = "unable to resize the shared memory object";
static const char short_gen157[] = "**get_guids";
static const char long_gen157[]  = "unable to get the guids for the infiniband channel adapter";
static const char short_gen158[] = "**gethostbyname";
static const char long_gen158[]  = "gethostbyname failed";
static const char short_gen159[] = "**getinfo";
static const char long_gen159[]  = "getaddrinfo failed";
static const char short_gen160[] = "**graphnnodes";
static const char long_gen160[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen161[] = "**group";
static const char long_gen161[]  = "Invalid group";
static const char short_gen162[] = "**groupnotincomm";
static const char long_gen162[]  = "Specified group is not within the communicator";
static const char short_gen163[] = "**handle_sock_op";
static const char long_gen163[]  = "handle_sock_op failed";
static const char short_gen164[] = "**handle_written";
static const char long_gen164[]  = "unable to handle written data";
static const char short_gen165[] = "**hostlookup";
static const char long_gen165[]  = "Host lookup failed";
static const char short_gen166[] = "**ibu_op";
static const char long_gen166[]  = "invalid infiniband operation";
static const char short_gen167[] = "**ibu_wait";
static const char long_gen167[]  = "ibu_wait failed";
static const char short_gen168[] = "**ibwrite";
static const char long_gen168[]  = "infiniband write failed";
static const char short_gen169[] = "**indexneg";
static const char long_gen169[]  = "Index value in graph topology must be nonnegative";
static const char short_gen170[] = "**indexnonmonotone";
static const char long_gen170[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen171[] = "**info";
static const char long_gen171[]  = "Invalid MPI_Info";
static const char short_gen172[] = "**infokey";
static const char long_gen172[]  = "Invalid key for MPI_Info ";
static const char short_gen173[] = "**infokeyempty";
static const char long_gen173[]  = "Empty or blank key ";
static const char short_gen174[] = "**infokeylong";
static const char long_gen174[]  = "Key is too long";
static const char short_gen175[] = "**infokeynull";
static const char long_gen175[]  = "Null key";
static const char short_gen176[] = "**infonkey";
static const char long_gen176[]  = "Requested nth key does not exist";
static const char short_gen177[] = "**infonokey";
static const char long_gen177[]  = "MPI_Info key is not defined ";
static const char short_gen178[] = "**infoval";
static const char long_gen178[]  = "Invalid MPI_Info value ";
static const char short_gen179[] = "**infovallong";
static const char long_gen179[]  = "Value is too long ";
static const char short_gen180[] = "**infovalnull";
static const char long_gen180[]  = "Null value";
static const char short_gen181[] = "**init";
static const char long_gen181[]  = "Initialization failed";
static const char short_gen182[] = "**init_buscard";
static const char long_gen182[]  = "failed to get my business card";
static const char short_gen183[] = "**init_description";
static const char long_gen183[]  = "unable to get the host description";
static const char short_gen184[] = "**init_getptr";
static const char long_gen184[]  = "failed to get the vcr";
static const char short_gen185[] = "**init_ibu";
static const char long_gen185[]  = "infiniband initialization failed";
static const char short_gen186[] = "**init_ibu_set";
static const char long_gen186[]  = "unable to create an infinband completion queue";
static const char short_gen187[] = "**init_progress";
static const char long_gen187[]  = "progress_init failed";
static const char short_gen188[] = "**init_vcrdup";
static const char long_gen188[]  = "failed to duplicate the virtual connection reference";
static const char short_gen189[] = "**init_vcrt";
static const char long_gen189[]  = "failed to create VCRT";
static const char short_gen190[] = "**initialized";
static const char long_gen190[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen191[] = "**inittwice";
static const char long_gen191[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen192[] = "**inpending";
static const char long_gen192[]  = "Pending request (no error)";
static const char short_gen193[] = "**instatus";
static const char long_gen193[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen194[] = "**intern";
static const char long_gen194[]  = "Internal MPI error!";
static const char short_gen195[] = "**inttoosmall";
static const char long_gen195[]  = "An address does not fit into a Fortran INTEGER.  Use MPI_Get_address instead";
static const char short_gen196[] = "**invalid_handle";
static const char long_gen196[]  = "invalid handle";
static const char short_gen197[] = "**invalid_listener";
static const char long_gen197[]  = "invalid listener";
static const char short_gen198[] = "**invalid_refcount";
static const char long_gen198[]  = "invalid reference count";
static const char short_gen199[] = "**io";
static const char long_gen199[]  = "Other I/O error ";
static const char short_gen200[] = "**ioRMWrdwr";
static const char long_gen200[]  = "Must open file with MPI_MODE_RDWR for read-modify-write ";
static const char short_gen201[] = "**ioagnomatch";
static const char long_gen201[]  = "No aggregators match";
static const char short_gen202[] = "**ioamodeseq";
static const char long_gen202[]  = "Cannot use this function when the file is opened with amode MPI_MODE_SEQUENTIAL ";
static const char short_gen203[] = "**iobadcount";
static const char long_gen203[]  = "Invalid count argument";
static const char short_gen204[] = "**iobaddisp";
static const char long_gen204[]  = "Invalid displacement argument";
static const char short_gen205[] = "**iobadfh";
static const char long_gen205[]  = "Invalid file handle";
static const char short_gen206[] = "**iobadoffset";
static const char long_gen206[]  = "Invalid offset argument";
static const char short_gen207[] = "**iobadsize";
static const char long_gen207[]  = "Invalid size argument";
static const char short_gen208[] = "**iobadwhence";
static const char long_gen208[]  = "Invalid whence argument";
static const char short_gen209[] = "**iocp";
static const char long_gen209[]  = "unable to create an I/O completion port";
static const char short_gen210[] = "**iodatarepnomem";
static const char long_gen210[]  = "User must allocate memory for datarep";
static const char short_gen211[] = "**iodispifseq";
static const char long_gen211[]  = "disp must be set to MPI_DISPLACEMENT_CURRENT since file was opened with MPI_MODE_SEQUENTIAL";
static const char short_gen212[] = "**ioetype";
static const char long_gen212[]  = "Only an integral number of etypes can be accessed";
static const char short_gen213[] = "**iofilecorrupt";
static const char long_gen213[]  = "File corrupt";
static const char short_gen214[] = "**iofilenull";
static const char long_gen214[]  = "Null file handle";
static const char short_gen215[] = "**iofiletype";
static const char long_gen215[]  = "Filetype must be constructed out of one or more etypes";
static const char short_gen216[] = "**iofstype";
static const char long_gen216[]  = "Cannot determine filesystem type";
static const char short_gen217[] = "**iofstypeunsupported";
static const char long_gen217[]  = "Specified filesystem is not available";
static const char short_gen218[] = "**ioneedrd";
static const char long_gen218[]  = "Read access is required to this file";
static const char short_gen219[] = "**ionegoffset";
static const char long_gen219[]  = "Negative offset argument";
static const char short_gen220[] = "**iopreallocrdwr";
static const char long_gen220[]  = "Must open file with MPI_MODE_RDWR to preallocate disk space";
static const char short_gen221[] = "**iosequnsupported";
static const char long_gen221[]  = "MPI_MODE_SEQUENTIAL not supported on this file system";
static const char short_gen222[] = "**iosharedfailed";
static const char long_gen222[]  = "Could not access shared file pointer";
static const char short_gen223[] = "**iosharedunsupported";
static const char long_gen223[]  = "Shared file pointers not supported";
static const char short_gen224[] = "**iosplitcoll";
static const char long_gen224[]  = "Only one active split collective I/O operation is allowed per file handle";
static const char short_gen225[] = "**iosplitcollnone";
static const char long_gen225[]  = "No split collective I/O operation is active";
static const char short_gen226[] = "**iov_offset";
static const char long_gen226[]  = "invalid iov offset";
static const char short_gen227[] = "**keyval";
static const char long_gen227[]  = "Invalid keyval";
static const char short_gen228[] = "**keyvalinvalid";
static const char long_gen228[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen229[] = "**keyvalnotcomm";
static const char long_gen229[]  = "Keyval was not defined for communicators";
static const char short_gen230[] = "**keyvalnotdatatype";
static const char long_gen230[]  = "Keyval was not defined for datatype";
static const char short_gen231[] = "**keyvalnotwin";
static const char long_gen231[]  = "Keyval was not defined for window objects";
static const char short_gen232[] = "**listen";
static const char long_gen232[]  = "listen failed";
static const char short_gen233[] = "**locktype";
static const char long_gen233[]  = "Invalid locktype";
static const char short_gen234[] = "**mmap";
static const char long_gen234[]  = "unable to map memory, mmap failed";
static const char short_gen235[] = "**mpi_accumulate";
static const char long_gen235[]  = "MPI_Accumulate failed";
static const char short_gen236[] = "**mpi_add_error_string";
static const char long_gen236[]  = "MPI_Add_error_string failed";
static const char short_gen237[] = "**mpi_allgather";
static const char long_gen237[]  = "MPI_Allgather failed";
static const char short_gen238[] = "**mpi_allgatherv";
static const char long_gen238[]  = "MPI_Allgatherv failed";
static const char short_gen239[] = "**mpi_allreduce";
static const char long_gen239[]  = "MPI_Allreduce failed";
static const char short_gen240[] = "**mpi_alltoall";
static const char long_gen240[]  = "MPI_Alltoall failed";
static const char short_gen241[] = "**mpi_alltoallv";
static const char long_gen241[]  = "MPI_Alltoallv failed";
static const char short_gen242[] = "**mpi_alltoallw";
static const char long_gen242[]  = "MPI_Alltoallw failed";
static const char short_gen243[] = "**mpi_attr_delete";
static const char long_gen243[]  = "MPI_Attr_delete failed";
static const char short_gen244[] = "**mpi_attr_get";
static const char long_gen244[]  = "MPI_Attr_get failed";
static const char short_gen245[] = "**mpi_attr_put";
static const char long_gen245[]  = "MPI_Attr_put failed";
static const char short_gen246[] = "**mpi_barrier";
static const char long_gen246[]  = "MPI_Barrier failed";
static const char short_gen247[] = "**mpi_bcast";
static const char long_gen247[]  = "MPI_Bcast failed";
static const char short_gen248[] = "**mpi_bsend";
static const char long_gen248[]  = "MPI_Bsend failed";
static const char short_gen249[] = "**mpi_bsend_init";
static const char long_gen249[]  = "MPI_Bsend_init failed";
static const char short_gen250[] = "**mpi_buffer_attach";
static const char long_gen250[]  = "MPI_Buffer_attach failed";
static const char short_gen251[] = "**mpi_buffer_detach";
static const char long_gen251[]  = "MPI_Buffer_detach failed";
static const char short_gen252[] = "**mpi_cancel";
static const char long_gen252[]  = "MPI_Cancel failed";
static const char short_gen253[] = "**mpi_cart_create";
static const char long_gen253[]  = "MPI_Cart_create failed";
static const char short_gen254[] = "**mpi_close_port";
static const char long_gen254[]  = "MPI_Close_port failed";
static const char short_gen255[] = "**mpi_comm_accept";
static const char long_gen255[]  = "MPI_Comm_accept failed";
static const char short_gen256[] = "**mpi_comm_connect";
static const char long_gen256[]  = "MPI_Comm_connect failed";
static const char short_gen257[] = "**mpi_comm_disconnect";
static const char long_gen257[]  = "MPI_Comm_disconnect failed";
static const char short_gen258[] = "**mpi_comm_dup";
static const char long_gen258[]  = "MPI_Comm_dup failed";
static const char short_gen259[] = "**mpi_comm_free";
static const char long_gen259[]  = "MPI_Comm_free failed";
static const char short_gen260[] = "**mpi_comm_group";
static const char long_gen260[]  = "MPI_Comm_group failed";
static const char short_gen261[] = "**mpi_comm_join";
static const char long_gen261[]  = "MPI_Comm_join failed";
static const char short_gen262[] = "**mpi_comm_remote_group";
static const char long_gen262[]  = "MPI_Comm_remote_group failed";
static const char short_gen263[] = "**mpi_comm_set_attr";
static const char long_gen263[]  = "MPI_Comm_set_attr failed";
static const char short_gen264[] = "**mpi_comm_spawn";
static const char long_gen264[]  = "MPI_Comm_spawn failed";
static const char short_gen265[] = "**mpi_comm_spawn_multiple";
static const char long_gen265[]  = "MPI_Comm_spawn_multiple failed";
static const char short_gen266[] = "**mpi_comm_split";
static const char long_gen266[]  = "MPI_Comm_split failed";
static const char short_gen267[] = "**mpi_errhandler_create";
static const char long_gen267[]  = "MPI_Errhandler_create failed";
static const char short_gen268[] = "**mpi_errhandler_get";
static const char long_gen268[]  = "MPI_Errhandler_get failed";
static const char short_gen269[] = "**mpi_errhandler_set";
static const char long_gen269[]  = "MPI_Errhandler_set failed";
static const char short_gen270[] = "**mpi_exscan";
static const char long_gen270[]  = "MPI_Exscan failed";
static const char short_gen271[] = "**mpi_file_create_errhandler";
static const char long_gen271[]  = "MPI_File_create_errhandler failed";
static const char short_gen272[] = "**mpi_gather";
static const char long_gen272[]  = "MPI_Gather failed";
static const char short_gen273[] = "**mpi_gatherv";
static const char long_gen273[]  = "MPI_Gatherv failed";
static const char short_gen274[] = "**mpi_get";
static const char long_gen274[]  = "MPI_Get failed";
static const char short_gen275[] = "**mpi_get_processor_name";
static const char long_gen275[]  = "MPI_Get_processor_name failed";
static const char short_gen276[] = "**mpi_graph_create";
static const char long_gen276[]  = "MPI_Graph_create failed";
static const char short_gen277[] = "**mpi_graph_map";
static const char long_gen277[]  = "MPI_Graph_map failed";
static const char short_gen278[] = "**mpi_grequest_start";
static const char long_gen278[]  = "MPI_Grequest_start failed";
static const char short_gen279[] = "**mpi_group_difference";
static const char long_gen279[]  = "MPI_Group_difference failed";
static const char short_gen280[] = "**mpi_group_excl";
static const char long_gen280[]  = "MPI_Group_excl failed";
static const char short_gen281[] = "**mpi_group_free";
static const char long_gen281[]  = "MPI_Group_free failed";
static const char short_gen282[] = "**mpi_group_incl";
static const char long_gen282[]  = "MPI_Group_incl failed";
static const char short_gen283[] = "**mpi_group_intersection";
static const char long_gen283[]  = "MPI_Group_intersection failed";
static const char short_gen284[] = "**mpi_group_range_excl";
static const char long_gen284[]  = "MPI_Group_range_excl failed";
static const char short_gen285[] = "**mpi_group_range_incl";
static const char long_gen285[]  = "MPI_Group_range_incl failed";
static const char short_gen286[] = "**mpi_group_union";
static const char long_gen286[]  = "MPI_Group_union failed";
static const char short_gen287[] = "**mpi_ibsend";
static const char long_gen287[]  = "MPI_Ibsend failed";
static const char short_gen288[] = "**mpi_info_create";
static const char long_gen288[]  = "MPI_Info_create failed";
static const char short_gen289[] = "**mpi_info_delete";
static const char long_gen289[]  = "MPI_Info_delete failed";
static const char short_gen290[] = "**mpi_info_dup";
static const char long_gen290[]  = "MPI_Info_dup failed";
static const char short_gen291[] = "**mpi_info_get_nthkey";
static const char long_gen291[]  = "MPI_Info_get_nthkey failed";
static const char short_gen292[] = "**mpi_info_set";
static const char long_gen292[]  = "MPI_Info_set failed";
static const char short_gen293[] = "**mpi_init";
static const char long_gen293[]  = "MPI_Init failed";
static const char short_gen294[] = "**mpi_init_thread";
static const char long_gen294[]  = "MPI_Init_thread failed";
static const char short_gen295[] = "**mpi_intercomm_create";
static const char long_gen295[]  = "MPI_Intercomm_create failed";
static const char short_gen296[] = "**mpi_intercomm_merge";
static const char long_gen296[]  = "MPI_Intercomm_merge failed";
static const char short_gen297[] = "**mpi_iprobe";
static const char long_gen297[]  = "MPI_Iprobe failed";
static const char short_gen298[] = "**mpi_irecv";
static const char long_gen298[]  = "MPI_Irecv failed";
static const char short_gen299[] = "**mpi_irsend";
static const char long_gen299[]  = "MPI_Irsend failed";
static const char short_gen300[] = "**mpi_isend";
static const char long_gen300[]  = "MPI_Isend failed";
static const char short_gen301[] = "**mpi_issend";
static const char long_gen301[]  = "MPI_Issend failed";
static const char short_gen302[] = "**mpi_keyval_create";
static const char long_gen302[]  = "MPI_Keyval_create failed";
static const char short_gen303[] = "**mpi_keyval_free";
static const char long_gen303[]  = "MPI_Keyval_free failed";
static const char short_gen304[] = "**mpi_lookup_name";
static const char long_gen304[]  = "MPI_Lookup_name failed";
static const char short_gen305[] = "**mpi_op_create";
static const char long_gen305[]  = "MPI_Op_create failed";
static const char short_gen306[] = "**mpi_open_port";
static const char long_gen306[]  = "MPI_Open_port failed";
static const char short_gen307[] = "**mpi_pack";
static const char long_gen307[]  = "MPI_Pack failed";
static const char short_gen308[] = "**mpi_pack_external";
static const char long_gen308[]  = "MPI_Pack_external failed";
static const char short_gen309[] = "**mpi_probe";
static const char long_gen309[]  = "MPI_Probe failed";
static const char short_gen310[] = "**mpi_publish_name";
static const char long_gen310[]  = "MPI_Publish_name failed";
static const char short_gen311[] = "**mpi_put";
static const char long_gen311[]  = "MPI_Put failed";
static const char short_gen312[] = "**mpi_recv";
static const char long_gen312[]  = "MPI_Recv failed";
static const char short_gen313[] = "**mpi_recv_init";
static const char long_gen313[]  = "MPI_Recv_init failed";
static const char short_gen314[] = "**mpi_reduce";
static const char long_gen314[]  = "MPI_Reduce failed";
static const char short_gen315[] = "**mpi_reduce_scatter";
static const char long_gen315[]  = "MPI_Reduce_scatter failed";
static const char short_gen316[] = "**mpi_register_datarep";
static const char long_gen316[]  = "MPI_Register_datarep failed";
static const char short_gen317[] = "**mpi_request_free";
static const char long_gen317[]  = "MPI_Request_free failed";
static const char short_gen318[] = "**mpi_request_get_status";
static const char long_gen318[]  = "MPI_Request_get_status failed";
static const char short_gen319[] = "**mpi_rsend";
static const char long_gen319[]  = "MPI_Rsend failed";
static const char short_gen320[] = "**mpi_rsend_init";
static const char long_gen320[]  = "MPI_Rsend_init failed";
static const char short_gen321[] = "**mpi_scan";
static const char long_gen321[]  = "MPI_Scan failed";
static const char short_gen322[] = "**mpi_scatter";
static const char long_gen322[]  = "MPI_Scatter failed";
static const char short_gen323[] = "**mpi_scatterv";
static const char long_gen323[]  = "MPI_Scatterv failed";
static const char short_gen324[] = "**mpi_send";
static const char long_gen324[]  = "MPI_Send failed";
static const char short_gen325[] = "**mpi_send_init";
static const char long_gen325[]  = "MPI_Send_init failed";
static const char short_gen326[] = "**mpi_sendrecv";
static const char long_gen326[]  = "MPI_Sendrecv failed";
static const char short_gen327[] = "**mpi_sendrecv_replace";
static const char long_gen327[]  = "MPI_Sendrecv_replace failed";
static const char short_gen328[] = "**mpi_ssend";
static const char long_gen328[]  = "MPI_Ssend failed";
static const char short_gen329[] = "**mpi_ssend_init";
static const char long_gen329[]  = "MPI_Ssend_init failed";
static const char short_gen330[] = "**mpi_start";
static const char long_gen330[]  = "MPI_Start failed";
static const char short_gen331[] = "**mpi_startall";
static const char long_gen331[]  = "MPI_Start_all failed";
static const char short_gen332[] = "**mpi_test";
static const char long_gen332[]  = "MPI_Test failed";
static const char short_gen333[] = "**mpi_testall";
static const char long_gen333[]  = "MPI_Testall failed";
static const char short_gen334[] = "**mpi_testany";
static const char long_gen334[]  = "MPI_Testany failed";
static const char short_gen335[] = "**mpi_testsome";
static const char long_gen335[]  = "MPI_Testsome failed";
static const char short_gen336[] = "**mpi_type_commit";
static const char long_gen336[]  = "MPI_Type_commit failed";
static const char short_gen337[] = "**mpi_type_contiguous";
static const char long_gen337[]  = "MPI_Type_continuous failed";
static const char short_gen338[] = "**mpi_type_create_hindexed";
static const char long_gen338[]  = "MPI_Type_create_hindexed failed";
static const char short_gen339[] = "**mpi_type_create_hvector";
static const char long_gen339[]  = "MPI_Type_create_hvector failed";
static const char short_gen340[] = "**mpi_type_create_indexed_block";
static const char long_gen340[]  = "MPI_Type_create_indexed_block failed";
static const char short_gen341[] = "**mpi_type_create_keyval";
static const char long_gen341[]  = "MPI_Type_create_keyval failed";
static const char short_gen342[] = "**mpi_type_create_resized";
static const char long_gen342[]  = "MPI_Type_create_resized failed";
static const char short_gen343[] = "**mpi_type_create_struct";
static const char long_gen343[]  = "MPI_Type_create_struct failed";
static const char short_gen344[] = "**mpi_type_delete_attr";
static const char long_gen344[]  = "MPI_Type_delete_attr failed";
static const char short_gen345[] = "**mpi_type_dup";
static const char long_gen345[]  = "MPI_Type_dup failed";
static const char short_gen346[] = "**mpi_type_get_contents";
static const char long_gen346[]  = "MPI_Type_get_contents failed";
static const char short_gen347[] = "**mpi_type_get_envelope";
static const char long_gen347[]  = "MPI_Type_get_envelope failed";
static const char short_gen348[] = "**mpi_type_hindexed";
static const char long_gen348[]  = "MPI_Type_hindexed failed";
static const char short_gen349[] = "**mpi_type_hvector";
static const char long_gen349[]  = "MPI_Type_hvector failed";
static const char short_gen350[] = "**mpi_type_indexed";
static const char long_gen350[]  = "MPI_Type_indexed failed";
static const char short_gen351[] = "**mpi_type_match_size";
static const char long_gen351[]  = "MPI_Type_match_size failed";
static const char short_gen352[] = "**mpi_type_set_attr";
static const char long_gen352[]  = "MPI_Type_set_attr failed";
static const char short_gen353[] = "**mpi_type_struct";
static const char long_gen353[]  = "MPI_Type_struct failed";
static const char short_gen354[] = "**mpi_type_vector";
static const char long_gen354[]  = "MPI_Type_vector failed";
static const char short_gen355[] = "**mpi_unpack_external";
static const char long_gen355[]  = "MPI_Unpack_external failed";
static const char short_gen356[] = "**mpi_unpublish_name";
static const char long_gen356[]  = "MPI_Unpublish_name failed";
static const char short_gen357[] = "**mpi_wait";
static const char long_gen357[]  = "MPI_Wait failed";
static const char short_gen358[] = "**mpi_waitall";
static const char long_gen358[]  = "MPI_Waitall failed";
static const char short_gen359[] = "**mpi_waitany";
static const char long_gen359[]  = "MPI_Waitany failed";
static const char short_gen360[] = "**mpi_waitsome";
static const char long_gen360[]  = "MPI_Waitsome failed";
static const char short_gen361[] = "**mpi_win_complete";
static const char long_gen361[]  = "MPI_Win_complete failed";
static const char short_gen362[] = "**mpi_win_create";
static const char long_gen362[]  = "MPI_Win_create failed";
static const char short_gen363[] = "**mpi_win_create_errhandler";
static const char long_gen363[]  = "MPI_Win_create_errhandler failed";
static const char short_gen364[] = "**mpi_win_create_keyval";
static const char long_gen364[]  = "MPI_Win_create_keyval failed";
static const char short_gen365[] = "**mpi_win_delete_attr";
static const char long_gen365[]  = "MPI_Win_delete_attr failed";
static const char short_gen366[] = "**mpi_win_fence";
static const char long_gen366[]  = "MPI_Win_fence failed";
static const char short_gen367[] = "**mpi_win_free";
static const char long_gen367[]  = "MPI_Win_free failed";
static const char short_gen368[] = "**mpi_win_get_group";
static const char long_gen368[]  = "MPI_Win_get_group failed";
static const char short_gen369[] = "**mpi_win_lock";
static const char long_gen369[]  = "MPI_Win_lock failed";
static const char short_gen370[] = "**mpi_win_post";
static const char long_gen370[]  = "MPI_Win_post failed";
static const char short_gen371[] = "**mpi_win_set_attr";
static const char long_gen371[]  = "MPI_Win_set_attr failed";
static const char short_gen372[] = "**mpi_win_test";
static const char long_gen372[]  = "MPI_Win_test failed";
static const char short_gen373[] = "**mpi_win_unlock";
static const char long_gen373[]  = "MPI_Win_unlock failed";
static const char short_gen374[] = "**mpi_win_wait";
static const char long_gen374[]  = "MPI_Win_wait failed";
static const char short_gen375[] = "**mq_close";
static const char long_gen375[]  = "failed to close a posix message queue";
static const char short_gen376[] = "**mq_open";
static const char long_gen376[]  = "failed to open a posix message queue";
static const char short_gen377[] = "**mq_receive";
static const char long_gen377[]  = "failed to receive a posix message queue message";
static const char short_gen378[] = "**mq_send";
static const char long_gen378[]  = "failed to send a posix message queue message";
static const char short_gen379[] = "**mqp_failure";
static const char long_gen379[]  = "failed to make progress on the shared memory bootstrap message queue";
static const char short_gen380[] = "**mqshm_create";
static const char long_gen380[]  = "failed to create a shared memory message queue";
static const char short_gen381[] = "**mqshm_receive";
static const char long_gen381[]  = "failed to receive a bootstrap message";
static const char short_gen382[] = "**mqshm_send";
static const char long_gen382[]  = "failed to send a bootstrap message";
static const char short_gen383[] = "**mqshm_unlink";
static const char long_gen383[]  = "unable to unlink the shared memory message queue";
static const char short_gen384[] = "**msgctl";
static const char long_gen384[]  = "msgctl failed";
static const char short_gen385[] = "**msgget";
static const char long_gen385[]  = "msgget failed";
static const char short_gen386[] = "**msgrcv";
static const char long_gen386[]  = "msgrcv failed";
static const char short_gen387[] = "**msgsnd";
static const char long_gen387[]  = "msgsnd failed";
static const char short_gen388[] = "**namepubfile";
static const char long_gen388[]  = "Unable to publish service name";
static const char short_gen389[] = "**namepubnotpub";
static const char long_gen389[]  = "Lookup failed for service name ";
static const char short_gen390[] = "**nameservice";
static const char long_gen390[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen391[] = "**needthreads";
static const char long_gen391[]  = "This function needs threads and threads have not been enabled";
static const char short_gen392[] = "**nextbootmsg";
static const char long_gen392[]  = "failed to get the next bootstrap message";
static const char short_gen393[] = "**noca";
static const char long_gen393[]  = "unable to find an active infiniband channel adapter";
static const char short_gen394[] = "**noerrclasses";
static const char long_gen394[]  = "No more user-defined error classes";
static const char short_gen395[] = "**noerrcodes";
static const char long_gen395[]  = "No more user-defined error codes";
static const char short_gen396[] = "**nomem";
static const char long_gen396[]  = "Out of memory";
static const char short_gen397[] = "**nomemreq";
static const char long_gen397[]  = "failure occurred while allocating memory for a request object";
static const char short_gen398[] = "**nonamepub";
static const char long_gen398[]  = "No name publishing service available";
static const char short_gen399[] = "**notcarttopo";
static const char long_gen399[]  = "No Cartesian topology associated with this communicator";
static const char short_gen400[] = "**notcstatignore";
static const char long_gen400[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_c2f()";
static const char short_gen401[] = "**notfstatignore";
static const char long_gen401[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_f2c()";
static const char short_gen402[] = "**notgenreq";
static const char long_gen402[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen403[] = "**notgraphtopo";
static const char long_gen403[]  = "No Graph topology associated with this communicator";
static const char short_gen404[] = "**notimpl";
static const char long_gen404[]  = "Function not implemented";
static const char short_gen405[] = "**notopology";
static const char long_gen405[]  = "No topology associated with this communicator";
static const char short_gen406[] = "**notsame";
static const char long_gen406[]  = "Inconsistent arguments to collective routine ";
static const char short_gen407[] = "**nouniquehigh";
static const char long_gen407[]  = "Could not determine which group to place first in merged intracommunicator.  Please use the parameter high to choose which group should be placed first.";
static const char short_gen408[] = "**nulledge";
static const char long_gen408[]  = "Edge detected from a node to the same node";
static const char short_gen409[] = "**nullptr";
static const char long_gen409[]  = "Null pointer";
static const char short_gen410[] = "**nullptrtype";
static const char long_gen410[]  = "Null pointer";
static const char short_gen411[] = "**op";
static const char long_gen411[]  = "Invalid MPI_Op";
static const char short_gen412[] = "**open";
static const char long_gen412[]  = "open failed";
static const char short_gen413[] = "**opnotpredefined";
static const char long_gen413[]  = "only predefined ops are valid";
static const char short_gen414[] = "**opundefined";
static const char long_gen414[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen415[] = "**opundefined_rma";
static const char long_gen415[]  = "RMA target received unknown RMA operation";
static const char short_gen416[] = "**other";
static const char long_gen416[]  = "Other MPI error";
static const char short_gen417[] = "**pctwice";
static const char long_gen417[]  = "post connect called twice";
static const char short_gen418[] = "**pd_alloc";
static const char long_gen418[]  = "unable to allocate a protection domain";
static const char short_gen419[] = "**permattr";
static const char long_gen419[]  = "Cannot set permanent attribute";
static const char short_gen420[] = "**permop";
static const char long_gen420[]  = "Cannot free permanent MPI_Op ";
static const char short_gen421[] = "**pfinal_sockclose";
static const char long_gen421[]  = "sock_close failed";
static const char short_gen422[] = "**pkt_ptr";
static const char long_gen422[]  = "invalid shm queue packet pointer";
static const char short_gen423[] = "**pmi_barrier";
static const char long_gen423[]  = "PMI_Barrier failed";
static const char short_gen424[] = "**pmi_finalize";
static const char long_gen424[]  = "PMI_Finalize failed";
static const char short_gen425[] = "**pmi_get_id";
static const char long_gen425[]  = "PMI_Get_id failed";
static const char short_gen426[] = "**pmi_get_id_length_max";
static const char long_gen426[]  = "PMI_Get_id_length_max failed";
static const char short_gen427[] = "**pmi_get_rank";
static const char long_gen427[]  = "PMI_Get_rank failed";
static const char short_gen428[] = "**pmi_get_size";
static const char long_gen428[]  = "PMI_Get_size failed";
static const char short_gen429[] = "**pmi_init";
static const char long_gen429[]  = "PMI_Init failed";
static const char short_gen430[] = "**pmi_kvs_commit";
static const char long_gen430[]  = "PMI_KVS_Commit failed";
static const char short_gen431[] = "**pmi_kvs_create";
static const char long_gen431[]  = "PMI_KVS_Create failed";
static const char short_gen432[] = "**pmi_kvs_get";
static const char long_gen432[]  = "PMI_KVS_Get failed";
static const char short_gen433[] = "**pmi_kvs_get_key_length_max";
static const char long_gen433[]  = "PMI_KVS_Get_key_length_max failed";
static const char short_gen434[] = "**pmi_kvs_get_my_name";
static const char long_gen434[]  = "PMI_KVS_Get_my_name failed";
static const char short_gen435[] = "**pmi_kvs_get_name_length_max";
static const char long_gen435[]  = "PMI_KVS_Get_name_length_max failed";
static const char short_gen436[] = "**pmi_kvs_get_value_length_max";
static const char long_gen436[]  = "PMI_KVS_Get_value_length_max failed";
static const char short_gen437[] = "**pmi_kvs_put";
static const char long_gen437[]  = "PMI_KVS_Put failed";
static const char short_gen438[] = "**pmi_spawn_multiple";
static const char long_gen438[]  = "PMI_Spawn_multiple failed";
static const char short_gen439[] = "**poke";
static const char long_gen439[]  = "progress_poke failed";
static const char short_gen440[] = "**port";
static const char long_gen440[]  = "Invalid port";
static const char short_gen441[] = "**post_accept";
static const char long_gen441[]  = "post accept failed";
static const char short_gen442[] = "**post_connect";
static const char long_gen442[]  = "failed to post a connection";
static const char short_gen443[] = "**postpkt";
static const char long_gen443[]  = "Unable to post a read for the next packet header";
static const char short_gen444[] = "**process_group";
static const char long_gen444[]  = "Process group initialization failed";
static const char short_gen445[] = "**progress";
static const char long_gen445[]  = "progress engine failure";
static const char short_gen446[] = "**progress_finalize";
static const char long_gen446[]  = "finalization of the progress engine failed";
static const char short_gen447[] = "**progress_handle_sock_op";
static const char long_gen447[]  = "handle_sock_op failed";
static const char short_gen448[] = "**progress_init";
static const char long_gen448[]  = "unable to initialize the progress engine";
static const char short_gen449[] = "**progress_sock_wait";
static const char long_gen449[]  = "sock_wait failed";
static const char short_gen450[] = "**progress_test";
static const char long_gen450[]  = "progress_test engine failure";
static const char short_gen451[] = "**rangedup";
static const char long_gen451[]  = "The range array specifies duplicate entries";
static const char short_gen452[] = "**rangeendinvalid";
static const char long_gen452[]  = "Some element of a range array is either negative or too large";
static const char short_gen453[] = "**rangestartinvalid";
static const char long_gen453[]  = "Some element of a range array is either negative or too large";
static const char short_gen454[] = "**rank";
static const char long_gen454[]  = "Invalid rank";
static const char short_gen455[] = "**rankarray";
static const char long_gen455[]  = "Invalid rank in rank array";
static const char short_gen456[] = "**rankdup";
static const char long_gen456[]  = "Duplicate ranks in rank array ";
static const char short_gen457[] = "**ranklocal";
static const char long_gen457[]  = "Error specifying local_leader ";
static const char short_gen458[] = "**rankremote";
static const char long_gen458[]  = "Error specifying remote_leader ";
static const char short_gen459[] = "**rdma_finalize";
static const char long_gen459[]  = "Channel rdma finalization failed";
static const char short_gen460[] = "**rdma_init";
static const char long_gen460[]  = "Channel rdma initialization failed";
static const char short_gen461[] = "**read_progress";
static const char long_gen461[]  = "Unable to make read progress";
static const char short_gen462[] = "**recvbuf_inplace";
static const char long_gen462[]  = "recvbuf cannot be MPI_IN_PLACE";
static const char short_gen463[] = "**request";
static const char long_gen463[]  = "Invalid MPI_Request";
static const char short_gen464[] = "**requestnotpersist";
static const char long_gen464[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen465[] = "**requestpersistactive";
static const char long_gen465[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen466[] = "**rmaconflict";
static const char long_gen466[]  = "Conflicting accesses to window ";
static const char short_gen467[] = "**rmadisp";
static const char long_gen467[]  = "Invalid displacement argument in RMA call ";
static const char short_gen468[] = "**rmasize";
static const char long_gen468[]  = "Invalid size argument in RMA call";
static const char short_gen469[] = "**rmasync";
static const char long_gen469[]  = "Wrong synchronization of RMA calls ";
static const char short_gen470[] = "**root";
static const char long_gen470[]  = "Invalid root";
static const char short_gen471[] = "**rsendnomatch";
static const char long_gen471[]  = "Ready send had no matching receive ";
static const char short_gen472[] = "**sendbuf_inplace";
static const char long_gen472[]  = "sendbuf cannot be MPI_IN_PLACE";
static const char short_gen473[] = "**servicename";
static const char long_gen473[]  = "Attempt to lookup an unknown service name ";
static const char short_gen474[] = "**shm_op";
static const char long_gen474[]  = "invalid shm operation";
static const char short_gen475[] = "**shm_open";
static const char long_gen475[]  = "unable to open a shared memory object";
static const char short_gen476[] = "**shm_read_progress";
static const char long_gen476[]  = "shared memory read progress failed";
static const char short_gen477[] = "**shm_unlink";
static const char long_gen477[]  = "failed to unlink shared memory";
static const char short_gen478[] = "**shm_wait";
static const char long_gen478[]  = "wait function failed";
static const char short_gen479[] = "**shmat";
static const char long_gen479[]  = "shmat failed";
static const char short_gen480[] = "**shmconnect_getmem";
static const char long_gen480[]  = "failed to allocate shared memory for a write queue";
static const char short_gen481[] = "**shmctl";
static const char long_gen481[]  = "failed to mark the sysv segment for removal";
static const char short_gen482[] = "**shmget";
static const char long_gen482[]  = "shmget failed";
static const char short_gen483[] = "**shmgetmem";
static const char long_gen483[]  = "Unable to allocate shared memory";
static const char short_gen484[] = "**shmhost";
static const char long_gen484[]  = "process not on the same host";
static const char short_gen485[] = "**shmq_index";
static const char long_gen485[]  = "invalid shm queue index";
static const char short_gen486[] = "**shmsize";
static const char long_gen486[]  = "Shared memory subsystem unable to allocate a block of the requested size";
static const char short_gen487[] = "**shmwrite";
static const char long_gen487[]  = "shared memory write failed";
static const char short_gen488[] = "**snprintf";
static const char long_gen488[]  = "snprintf returned an invalid number";
static const char short_gen489[] = "**sock_byname";
static const char long_gen489[]  = "gethostbyname failed";
static const char short_gen490[] = "**sock_closed";
static const char long_gen490[]  = "socket closed";
static const char short_gen491[] = "**sock_connect";
static const char long_gen491[]  = "connect failed";
static const char short_gen492[] = "**sock_create";
static const char long_gen492[]  = "unable to create a socket";
static const char short_gen493[] = "**sock_gethost";
static const char long_gen493[]  = "gethostname failed";
static const char short_gen494[] = "**sock_init";
static const char long_gen494[]  = "unable to initialize the sock library";
static const char short_gen495[] = "**sock_nop_accept";
static const char long_gen495[]  = "accept called without having received an op_accept";
static const char short_gen496[] = "**sock_post_close";
static const char long_gen496[]  = "posting a close of the socket failed";
static const char short_gen497[] = "**socket";
static const char long_gen497[]  = "WSASocket failed";
static const char short_gen498[] = "**sock|badbuf";
static const char long_gen498[]  = "the supplied buffer contains invalid memory";
static const char short_gen499[] = "**sock|badhandle";
static const char long_gen499[]  = "sock contains an invalid handle";
static const char short_gen500[] = "**sock|badhdbuf";
static const char long_gen500[]  = "a memory fault occurred while accessing the host description string";
static const char short_gen501[] = "**sock|badhdlen";
static const char long_gen501[]  = "host description string to small to store description";
static const char short_gen502[] = "**sock|badhdmax";
static const char long_gen502[]  = "the length of the host description string must be non-negative";
static const char short_gen503[] = "**sock|badiovn";
static const char long_gen503[]  = "size of iov is invalid";
static const char short_gen504[] = "**sock|badlen";
static const char long_gen504[]  = "bad length parameter(s)";
static const char short_gen505[] = "**sock|badport";
static const char long_gen505[]  = "port number is out of range";
static const char short_gen506[] = "**sock|badsock";
static const char long_gen506[]  = "supplied sock is corrupt";
static const char short_gen507[] = "**sock|closed";
static const char long_gen507[]  = "sock has been closed locally";
static const char short_gen508[] = "**sock|closing";
static const char long_gen508[]  = "sock is in the process of being closed locally";
static const char short_gen509[] = "**sock|connclosed";
static const char long_gen509[]  = "connection closed by peer";
static const char short_gen510[] = "**sock|connfailed";
static const char long_gen510[]  = "connection failure";
static const char short_gen511[] = "**sock|connrefused";
static const char long_gen511[]  = "connection refused";
static const char short_gen512[] = "**sock|getport";
static const char long_gen512[]  = "failed to obtain port number of the listener";
static const char short_gen513[] = "**sock|hostres";
static const char long_gen513[]  = "unable to resolve host name to an address";
static const char short_gen514[] = "**sock|nosock";
static const char long_gen514[]  = "no new sock was available to accept";
static const char short_gen515[] = "**sock|notconnected";
static const char long_gen515[]  = "sock is not connected";
static const char short_gen516[] = "**sock|oserror";
static const char long_gen516[]  = "unknown operating system error";
static const char short_gen517[] = "**sock|osnomem";
static const char long_gen517[]  = "operating system routine failed due to lack of memory";
static const char short_gen518[] = "**sock|poll|accept";
static const char long_gen518[]  = "accept failed to acquire a new socket";
static const char short_gen519[] = "**sock|poll|bind";
static const char long_gen519[]  = "unable to bind socket to port";
static const char short_gen520[] = "**sock|poll|eqfail";
static const char long_gen520[]  = "fatal error: failed to enqueue an event; event was lost";
static const char short_gen521[] = "**sock|poll|eqmalloc";
static const char long_gen521[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen522[] = "**sock|poll|listen";
static const char long_gen522[]  = "listen() failed";
static const char short_gen523[] = "**sock|poll|nodelay";
static const char long_gen523[]  = "unable to set TCP no delay attribute on socket";
static const char short_gen524[] = "**sock|poll|nonblock";
static const char long_gen524[]  = "unable to set socket to nonblocking";
static const char short_gen525[] = "**sock|poll|pipe";
static const char long_gen525[]  = "unable to allocate pipe to wakeup a blocking poll()";
static const char short_gen526[] = "**sock|poll|pipenonblock";
static const char long_gen526[]  = "unable to set wakeup pipe to nonblocking";
static const char short_gen527[] = "**sock|poll|reuseaddr";
static const char long_gen527[]  = "unable to set reuseaddr attribute on socket";
static const char short_gen528[] = "**sock|poll|socket";
static const char long_gen528[]  = "unable to obtain new socket";
static const char short_gen529[] = "**sock|poll|unhandledstate";
static const char long_gen529[]  = "encountered an unhandled";
static const char short_gen530[] = "**sock|reads";
static const char long_gen530[]  = "attempt to perform multiple simultaneous reads";
static const char short_gen531[] = "**sock|setalloc";
static const char long_gen531[]  = "unable to allocate a new sock set object";
static const char short_gen532[] = "**sock|sockalloc";
static const char long_gen532[]  = "unable to allocate a new sock object";
static const char short_gen533[] = "**sock|uninit";
static const char long_gen533[]  = "Sock library has not been initialized";
static const char short_gen534[] = "**sock|writes";
static const char long_gen534[]  = "attempt to perform multiple simultaneous writes";
static const char short_gen535[] = "**spawn";
static const char long_gen535[]  = "Error in spawn call";
static const char short_gen536[] = "**ssmwrite";
static const char long_gen536[]  = "sock/shared memory write failed";
static const char short_gen537[] = "**ssmwritev";
static const char long_gen537[]  = "sock/shared memory writev failed";
static const char short_gen538[] = "**stride";
static const char long_gen538[]  = "Range does not terminate";
static const char short_gen539[] = "**stridezero";
static const char long_gen539[]  = "Zero stride is invalid";
static const char short_gen540[] = "**strncpy";
static const char long_gen540[]  = "insufficient buffer length to complete strncpy";
static const char short_gen541[] = "**success";
static const char long_gen541[]  = "No MPI error";
static const char short_gen542[] = "**tag";
static const char long_gen542[]  = "Invalid tag";
static const char short_gen543[] = "**test_sock_wait";
static const char long_gen543[]  = "sock_wait failed";
static const char short_gen544[] = "**toomanycomm";
static const char long_gen544[]  = "Too many communicators";
static const char short_gen545[] = "**topology";
static const char long_gen545[]  = "Invalid topology";
static const char short_gen546[] = "**topotoolarge";
static const char long_gen546[]  = "Topology size is greater than communicator size";
static const char short_gen547[] = "**truncate";
static const char long_gen547[]  = "Message truncated";
static const char short_gen548[] = "**typematchnoclass";
static const char long_gen548[]  = "The value of typeclass is not one of MPI_TYPECLASS_REAL, MPI_TYPECLASS_INTEGER, or MPI_TYPECLASS_COMPLEX";
static const char short_gen549[] = "**typematchsize";
static const char long_gen549[]  = "No MPI datatype available for the given typeclass and size";
static const char short_gen550[] = "**typenamelen";
static const char long_gen550[]  = " Specified datatype name is too long";
static const char short_gen551[] = "**unknown";
static const char long_gen551[]  = "Unknown error.  Please file a bug report.";
static const char short_gen552[] = "**unsupporteddatarep";
static const char long_gen552[]  = "Only native data representation currently supported";
static const char short_gen553[] = "**vc_state";
static const char long_gen553[]  = "invalid vc state";
static const char short_gen554[] = "**win";
static const char long_gen554[]  = "Invalid MPI_Win";
static const char short_gen555[] = "**winwait";
static const char long_gen555[]  = "WaitForSingleObject failed";
static const char short_gen556[] = "**write_progress";
static const char long_gen556[]  = "Write progress failed";
static const char short_gen557[] = "**wsasock";
static const char long_gen557[]  = "WSAStartup failed";

static const int generic_msgs_len = 558;
static const msgpair generic_err_msgs[] = {
{ 0xacebad03, short_gen0, long_gen0, 0xcb0bfa11 },
{ 0xacebad03, short_gen1, long_gen1, 0xcb0bfa11 },
{ 0xacebad03, short_gen2, long_gen2, 0xcb0bfa11 },
{ 0xacebad03, short_gen3, long_gen3, 0xcb0bfa11 },
{ 0xacebad03, short_gen4, long_gen4, 0xcb0bfa11 },
{ 0xacebad03, short_gen5, long_gen5, 0xcb0bfa11 },
{ 0xacebad03, short_gen6, long_gen6, 0xcb0bfa11 },
{ 0xacebad03, short_gen7, long_gen7, 0xcb0bfa11 },
{ 0xacebad03, short_gen8, long_gen8, 0xcb0bfa11 },
{ 0xacebad03, short_gen9, long_gen9, 0xcb0bfa11 },
{ 0xacebad03, short_gen10, long_gen10, 0xcb0bfa11 },
{ 0xacebad03, short_gen11, long_gen11, 0xcb0bfa11 },
{ 0xacebad03, short_gen12, long_gen12, 0xcb0bfa11 },
{ 0xacebad03, short_gen13, long_gen13, 0xcb0bfa11 },
{ 0xacebad03, short_gen14, long_gen14, 0xcb0bfa11 },
{ 0xacebad03, short_gen15, long_gen15, 0xcb0bfa11 },
{ 0xacebad03, short_gen16, long_gen16, 0xcb0bfa11 },
{ 0xacebad03, short_gen17, long_gen17, 0xcb0bfa11 },
{ 0xacebad03, short_gen18, long_gen18, 0xcb0bfa11 },
{ 0xacebad03, short_gen19, long_gen19, 0xcb0bfa11 },
{ 0xacebad03, short_gen20, long_gen20, 0xcb0bfa11 },
{ 0xacebad03, short_gen21, long_gen21, 0xcb0bfa11 },
{ 0xacebad03, short_gen22, long_gen22, 0xcb0bfa11 },
{ 0xacebad03, short_gen23, long_gen23, 0xcb0bfa11 },
{ 0xacebad03, short_gen24, long_gen24, 0xcb0bfa11 },
{ 0xacebad03, short_gen25, long_gen25, 0xcb0bfa11 },
{ 0xacebad03, short_gen26, long_gen26, 0xcb0bfa11 },
{ 0xacebad03, short_gen27, long_gen27, 0xcb0bfa11 },
{ 0xacebad03, short_gen28, long_gen28, 0xcb0bfa11 },
{ 0xacebad03, short_gen29, long_gen29, 0xcb0bfa11 },
{ 0xacebad03, short_gen30, long_gen30, 0xcb0bfa11 },
{ 0xacebad03, short_gen31, long_gen31, 0xcb0bfa11 },
{ 0xacebad03, short_gen32, long_gen32, 0xcb0bfa11 },
{ 0xacebad03, short_gen33, long_gen33, 0xcb0bfa11 },
{ 0xacebad03, short_gen34, long_gen34, 0xcb0bfa11 },
{ 0xacebad03, short_gen35, long_gen35, 0xcb0bfa11 },
{ 0xacebad03, short_gen36, long_gen36, 0xcb0bfa11 },
{ 0xacebad03, short_gen37, long_gen37, 0xcb0bfa11 },
{ 0xacebad03, short_gen38, long_gen38, 0xcb0bfa11 },
{ 0xacebad03, short_gen39, long_gen39, 0xcb0bfa11 },
{ 0xacebad03, short_gen40, long_gen40, 0xcb0bfa11 },
{ 0xacebad03, short_gen41, long_gen41, 0xcb0bfa11 },
{ 0xacebad03, short_gen42, long_gen42, 0xcb0bfa11 },
{ 0xacebad03, short_gen43, long_gen43, 0xcb0bfa11 },
{ 0xacebad03, short_gen44, long_gen44, 0xcb0bfa11 },
{ 0xacebad03, short_gen45, long_gen45, 0xcb0bfa11 },
{ 0xacebad03, short_gen46, long_gen46, 0xcb0bfa11 },
{ 0xacebad03, short_gen47, long_gen47, 0xcb0bfa11 },
{ 0xacebad03, short_gen48, long_gen48, 0xcb0bfa11 },
{ 0xacebad03, short_gen49, long_gen49, 0xcb0bfa11 },
{ 0xacebad03, short_gen50, long_gen50, 0xcb0bfa11 },
{ 0xacebad03, short_gen51, long_gen51, 0xcb0bfa11 },
{ 0xacebad03, short_gen52, long_gen52, 0xcb0bfa11 },
{ 0xacebad03, short_gen53, long_gen53, 0xcb0bfa11 },
{ 0xacebad03, short_gen54, long_gen54, 0xcb0bfa11 },
{ 0xacebad03, short_gen55, long_gen55, 0xcb0bfa11 },
{ 0xacebad03, short_gen56, long_gen56, 0xcb0bfa11 },
{ 0xacebad03, short_gen57, long_gen57, 0xcb0bfa11 },
{ 0xacebad03, short_gen58, long_gen58, 0xcb0bfa11 },
{ 0xacebad03, short_gen59, long_gen59, 0xcb0bfa11 },
{ 0xacebad03, short_gen60, long_gen60, 0xcb0bfa11 },
{ 0xacebad03, short_gen61, long_gen61, 0xcb0bfa11 },
{ 0xacebad03, short_gen62, long_gen62, 0xcb0bfa11 },
{ 0xacebad03, short_gen63, long_gen63, 0xcb0bfa11 },
{ 0xacebad03, short_gen64, long_gen64, 0xcb0bfa11 },
{ 0xacebad03, short_gen65, long_gen65, 0xcb0bfa11 },
{ 0xacebad03, short_gen66, long_gen66, 0xcb0bfa11 },
{ 0xacebad03, short_gen67, long_gen67, 0xcb0bfa11 },
{ 0xacebad03, short_gen68, long_gen68, 0xcb0bfa11 },
{ 0xacebad03, short_gen69, long_gen69, 0xcb0bfa11 },
{ 0xacebad03, short_gen70, long_gen70, 0xcb0bfa11 },
{ 0xacebad03, short_gen71, long_gen71, 0xcb0bfa11 },
{ 0xacebad03, short_gen72, long_gen72, 0xcb0bfa11 },
{ 0xacebad03, short_gen73, long_gen73, 0xcb0bfa11 },
{ 0xacebad03, short_gen74, long_gen74, 0xcb0bfa11 },
{ 0xacebad03, short_gen75, long_gen75, 0xcb0bfa11 },
{ 0xacebad03, short_gen76, long_gen76, 0xcb0bfa11 },
{ 0xacebad03, short_gen77, long_gen77, 0xcb0bfa11 },
{ 0xacebad03, short_gen78, long_gen78, 0xcb0bfa11 },
{ 0xacebad03, short_gen79, long_gen79, 0xcb0bfa11 },
{ 0xacebad03, short_gen80, long_gen80, 0xcb0bfa11 },
{ 0xacebad03, short_gen81, long_gen81, 0xcb0bfa11 },
{ 0xacebad03, short_gen82, long_gen82, 0xcb0bfa11 },
{ 0xacebad03, short_gen83, long_gen83, 0xcb0bfa11 },
{ 0xacebad03, short_gen84, long_gen84, 0xcb0bfa11 },
{ 0xacebad03, short_gen85, long_gen85, 0xcb0bfa11 },
{ 0xacebad03, short_gen86, long_gen86, 0xcb0bfa11 },
{ 0xacebad03, short_gen87, long_gen87, 0xcb0bfa11 },
{ 0xacebad03, short_gen88, long_gen88, 0xcb0bfa11 },
{ 0xacebad03, short_gen89, long_gen89, 0xcb0bfa11 },
{ 0xacebad03, short_gen90, long_gen90, 0xcb0bfa11 },
{ 0xacebad03, short_gen91, long_gen91, 0xcb0bfa11 },
{ 0xacebad03, short_gen92, long_gen92, 0xcb0bfa11 },
{ 0xacebad03, short_gen93, long_gen93, 0xcb0bfa11 },
{ 0xacebad03, short_gen94, long_gen94, 0xcb0bfa11 },
{ 0xacebad03, short_gen95, long_gen95, 0xcb0bfa11 },
{ 0xacebad03, short_gen96, long_gen96, 0xcb0bfa11 },
{ 0xacebad03, short_gen97, long_gen97, 0xcb0bfa11 },
{ 0xacebad03, short_gen98, long_gen98, 0xcb0bfa11 },
{ 0xacebad03, short_gen99, long_gen99, 0xcb0bfa11 },
{ 0xacebad03, short_gen100, long_gen100, 0xcb0bfa11 },
{ 0xacebad03, short_gen101, long_gen101, 0xcb0bfa11 },
{ 0xacebad03, short_gen102, long_gen102, 0xcb0bfa11 },
{ 0xacebad03, short_gen103, long_gen103, 0xcb0bfa11 },
{ 0xacebad03, short_gen104, long_gen104, 0xcb0bfa11 },
{ 0xacebad03, short_gen105, long_gen105, 0xcb0bfa11 },
{ 0xacebad03, short_gen106, long_gen106, 0xcb0bfa11 },
{ 0xacebad03, short_gen107, long_gen107, 0xcb0bfa11 },
{ 0xacebad03, short_gen108, long_gen108, 0xcb0bfa11 },
{ 0xacebad03, short_gen109, long_gen109, 0xcb0bfa11 },
{ 0xacebad03, short_gen110, long_gen110, 0xcb0bfa11 },
{ 0xacebad03, short_gen111, long_gen111, 0xcb0bfa11 },
{ 0xacebad03, short_gen112, long_gen112, 0xcb0bfa11 },
{ 0xacebad03, short_gen113, long_gen113, 0xcb0bfa11 },
{ 0xacebad03, short_gen114, long_gen114, 0xcb0bfa11 },
{ 0xacebad03, short_gen115, long_gen115, 0xcb0bfa11 },
{ 0xacebad03, short_gen116, long_gen116, 0xcb0bfa11 },
{ 0xacebad03, short_gen117, long_gen117, 0xcb0bfa11 },
{ 0xacebad03, short_gen118, long_gen118, 0xcb0bfa11 },
{ 0xacebad03, short_gen119, long_gen119, 0xcb0bfa11 },
{ 0xacebad03, short_gen120, long_gen120, 0xcb0bfa11 },
{ 0xacebad03, short_gen121, long_gen121, 0xcb0bfa11 },
{ 0xacebad03, short_gen122, long_gen122, 0xcb0bfa11 },
{ 0xacebad03, short_gen123, long_gen123, 0xcb0bfa11 },
{ 0xacebad03, short_gen124, long_gen124, 0xcb0bfa11 },
{ 0xacebad03, short_gen125, long_gen125, 0xcb0bfa11 },
{ 0xacebad03, short_gen126, long_gen126, 0xcb0bfa11 },
{ 0xacebad03, short_gen127, long_gen127, 0xcb0bfa11 },
{ 0xacebad03, short_gen128, long_gen128, 0xcb0bfa11 },
{ 0xacebad03, short_gen129, long_gen129, 0xcb0bfa11 },
{ 0xacebad03, short_gen130, long_gen130, 0xcb0bfa11 },
{ 0xacebad03, short_gen131, long_gen131, 0xcb0bfa11 },
{ 0xacebad03, short_gen132, long_gen132, 0xcb0bfa11 },
{ 0xacebad03, short_gen133, long_gen133, 0xcb0bfa11 },
{ 0xacebad03, short_gen134, long_gen134, 0xcb0bfa11 },
{ 0xacebad03, short_gen135, long_gen135, 0xcb0bfa11 },
{ 0xacebad03, short_gen136, long_gen136, 0xcb0bfa11 },
{ 0xacebad03, short_gen137, long_gen137, 0xcb0bfa11 },
{ 0xacebad03, short_gen138, long_gen138, 0xcb0bfa11 },
{ 0xacebad03, short_gen139, long_gen139, 0xcb0bfa11 },
{ 0xacebad03, short_gen140, long_gen140, 0xcb0bfa11 },
{ 0xacebad03, short_gen141, long_gen141, 0xcb0bfa11 },
{ 0xacebad03, short_gen142, long_gen142, 0xcb0bfa11 },
{ 0xacebad03, short_gen143, long_gen143, 0xcb0bfa11 },
{ 0xacebad03, short_gen144, long_gen144, 0xcb0bfa11 },
{ 0xacebad03, short_gen145, long_gen145, 0xcb0bfa11 },
{ 0xacebad03, short_gen146, long_gen146, 0xcb0bfa11 },
{ 0xacebad03, short_gen147, long_gen147, 0xcb0bfa11 },
{ 0xacebad03, short_gen148, long_gen148, 0xcb0bfa11 },
{ 0xacebad03, short_gen149, long_gen149, 0xcb0bfa11 },
{ 0xacebad03, short_gen150, long_gen150, 0xcb0bfa11 },
{ 0xacebad03, short_gen151, long_gen151, 0xcb0bfa11 },
{ 0xacebad03, short_gen152, long_gen152, 0xcb0bfa11 },
{ 0xacebad03, short_gen153, long_gen153, 0xcb0bfa11 },
{ 0xacebad03, short_gen154, long_gen154, 0xcb0bfa11 },
{ 0xacebad03, short_gen155, long_gen155, 0xcb0bfa11 },
{ 0xacebad03, short_gen156, long_gen156, 0xcb0bfa11 },
{ 0xacebad03, short_gen157, long_gen157, 0xcb0bfa11 },
{ 0xacebad03, short_gen158, long_gen158, 0xcb0bfa11 },
{ 0xacebad03, short_gen159, long_gen159, 0xcb0bfa11 },
{ 0xacebad03, short_gen160, long_gen160, 0xcb0bfa11 },
{ 0xacebad03, short_gen161, long_gen161, 0xcb0bfa11 },
{ 0xacebad03, short_gen162, long_gen162, 0xcb0bfa11 },
{ 0xacebad03, short_gen163, long_gen163, 0xcb0bfa11 },
{ 0xacebad03, short_gen164, long_gen164, 0xcb0bfa11 },
{ 0xacebad03, short_gen165, long_gen165, 0xcb0bfa11 },
{ 0xacebad03, short_gen166, long_gen166, 0xcb0bfa11 },
{ 0xacebad03, short_gen167, long_gen167, 0xcb0bfa11 },
{ 0xacebad03, short_gen168, long_gen168, 0xcb0bfa11 },
{ 0xacebad03, short_gen169, long_gen169, 0xcb0bfa11 },
{ 0xacebad03, short_gen170, long_gen170, 0xcb0bfa11 },
{ 0xacebad03, short_gen171, long_gen171, 0xcb0bfa11 },
{ 0xacebad03, short_gen172, long_gen172, 0xcb0bfa11 },
{ 0xacebad03, short_gen173, long_gen173, 0xcb0bfa11 },
{ 0xacebad03, short_gen174, long_gen174, 0xcb0bfa11 },
{ 0xacebad03, short_gen175, long_gen175, 0xcb0bfa11 },
{ 0xacebad03, short_gen176, long_gen176, 0xcb0bfa11 },
{ 0xacebad03, short_gen177, long_gen177, 0xcb0bfa11 },
{ 0xacebad03, short_gen178, long_gen178, 0xcb0bfa11 },
{ 0xacebad03, short_gen179, long_gen179, 0xcb0bfa11 },
{ 0xacebad03, short_gen180, long_gen180, 0xcb0bfa11 },
{ 0xacebad03, short_gen181, long_gen181, 0xcb0bfa11 },
{ 0xacebad03, short_gen182, long_gen182, 0xcb0bfa11 },
{ 0xacebad03, short_gen183, long_gen183, 0xcb0bfa11 },
{ 0xacebad03, short_gen184, long_gen184, 0xcb0bfa11 },
{ 0xacebad03, short_gen185, long_gen185, 0xcb0bfa11 },
{ 0xacebad03, short_gen186, long_gen186, 0xcb0bfa11 },
{ 0xacebad03, short_gen187, long_gen187, 0xcb0bfa11 },
{ 0xacebad03, short_gen188, long_gen188, 0xcb0bfa11 },
{ 0xacebad03, short_gen189, long_gen189, 0xcb0bfa11 },
{ 0xacebad03, short_gen190, long_gen190, 0xcb0bfa11 },
{ 0xacebad03, short_gen191, long_gen191, 0xcb0bfa11 },
{ 0xacebad03, short_gen192, long_gen192, 0xcb0bfa11 },
{ 0xacebad03, short_gen193, long_gen193, 0xcb0bfa11 },
{ 0xacebad03, short_gen194, long_gen194, 0xcb0bfa11 },
{ 0xacebad03, short_gen195, long_gen195, 0xcb0bfa11 },
{ 0xacebad03, short_gen196, long_gen196, 0xcb0bfa11 },
{ 0xacebad03, short_gen197, long_gen197, 0xcb0bfa11 },
{ 0xacebad03, short_gen198, long_gen198, 0xcb0bfa11 },
{ 0xacebad03, short_gen199, long_gen199, 0xcb0bfa11 },
{ 0xacebad03, short_gen200, long_gen200, 0xcb0bfa11 },
{ 0xacebad03, short_gen201, long_gen201, 0xcb0bfa11 },
{ 0xacebad03, short_gen202, long_gen202, 0xcb0bfa11 },
{ 0xacebad03, short_gen203, long_gen203, 0xcb0bfa11 },
{ 0xacebad03, short_gen204, long_gen204, 0xcb0bfa11 },
{ 0xacebad03, short_gen205, long_gen205, 0xcb0bfa11 },
{ 0xacebad03, short_gen206, long_gen206, 0xcb0bfa11 },
{ 0xacebad03, short_gen207, long_gen207, 0xcb0bfa11 },
{ 0xacebad03, short_gen208, long_gen208, 0xcb0bfa11 },
{ 0xacebad03, short_gen209, long_gen209, 0xcb0bfa11 },
{ 0xacebad03, short_gen210, long_gen210, 0xcb0bfa11 },
{ 0xacebad03, short_gen211, long_gen211, 0xcb0bfa11 },
{ 0xacebad03, short_gen212, long_gen212, 0xcb0bfa11 },
{ 0xacebad03, short_gen213, long_gen213, 0xcb0bfa11 },
{ 0xacebad03, short_gen214, long_gen214, 0xcb0bfa11 },
{ 0xacebad03, short_gen215, long_gen215, 0xcb0bfa11 },
{ 0xacebad03, short_gen216, long_gen216, 0xcb0bfa11 },
{ 0xacebad03, short_gen217, long_gen217, 0xcb0bfa11 },
{ 0xacebad03, short_gen218, long_gen218, 0xcb0bfa11 },
{ 0xacebad03, short_gen219, long_gen219, 0xcb0bfa11 },
{ 0xacebad03, short_gen220, long_gen220, 0xcb0bfa11 },
{ 0xacebad03, short_gen221, long_gen221, 0xcb0bfa11 },
{ 0xacebad03, short_gen222, long_gen222, 0xcb0bfa11 },
{ 0xacebad03, short_gen223, long_gen223, 0xcb0bfa11 },
{ 0xacebad03, short_gen224, long_gen224, 0xcb0bfa11 },
{ 0xacebad03, short_gen225, long_gen225, 0xcb0bfa11 },
{ 0xacebad03, short_gen226, long_gen226, 0xcb0bfa11 },
{ 0xacebad03, short_gen227, long_gen227, 0xcb0bfa11 },
{ 0xacebad03, short_gen228, long_gen228, 0xcb0bfa11 },
{ 0xacebad03, short_gen229, long_gen229, 0xcb0bfa11 },
{ 0xacebad03, short_gen230, long_gen230, 0xcb0bfa11 },
{ 0xacebad03, short_gen231, long_gen231, 0xcb0bfa11 },
{ 0xacebad03, short_gen232, long_gen232, 0xcb0bfa11 },
{ 0xacebad03, short_gen233, long_gen233, 0xcb0bfa11 },
{ 0xacebad03, short_gen234, long_gen234, 0xcb0bfa11 },
{ 0xacebad03, short_gen235, long_gen235, 0xcb0bfa11 },
{ 0xacebad03, short_gen236, long_gen236, 0xcb0bfa11 },
{ 0xacebad03, short_gen237, long_gen237, 0xcb0bfa11 },
{ 0xacebad03, short_gen238, long_gen238, 0xcb0bfa11 },
{ 0xacebad03, short_gen239, long_gen239, 0xcb0bfa11 },
{ 0xacebad03, short_gen240, long_gen240, 0xcb0bfa11 },
{ 0xacebad03, short_gen241, long_gen241, 0xcb0bfa11 },
{ 0xacebad03, short_gen242, long_gen242, 0xcb0bfa11 },
{ 0xacebad03, short_gen243, long_gen243, 0xcb0bfa11 },
{ 0xacebad03, short_gen244, long_gen244, 0xcb0bfa11 },
{ 0xacebad03, short_gen245, long_gen245, 0xcb0bfa11 },
{ 0xacebad03, short_gen246, long_gen246, 0xcb0bfa11 },
{ 0xacebad03, short_gen247, long_gen247, 0xcb0bfa11 },
{ 0xacebad03, short_gen248, long_gen248, 0xcb0bfa11 },
{ 0xacebad03, short_gen249, long_gen249, 0xcb0bfa11 },
{ 0xacebad03, short_gen250, long_gen250, 0xcb0bfa11 },
{ 0xacebad03, short_gen251, long_gen251, 0xcb0bfa11 },
{ 0xacebad03, short_gen252, long_gen252, 0xcb0bfa11 },
{ 0xacebad03, short_gen253, long_gen253, 0xcb0bfa11 },
{ 0xacebad03, short_gen254, long_gen254, 0xcb0bfa11 },
{ 0xacebad03, short_gen255, long_gen255, 0xcb0bfa11 },
{ 0xacebad03, short_gen256, long_gen256, 0xcb0bfa11 },
{ 0xacebad03, short_gen257, long_gen257, 0xcb0bfa11 },
{ 0xacebad03, short_gen258, long_gen258, 0xcb0bfa11 },
{ 0xacebad03, short_gen259, long_gen259, 0xcb0bfa11 },
{ 0xacebad03, short_gen260, long_gen260, 0xcb0bfa11 },
{ 0xacebad03, short_gen261, long_gen261, 0xcb0bfa11 },
{ 0xacebad03, short_gen262, long_gen262, 0xcb0bfa11 },
{ 0xacebad03, short_gen263, long_gen263, 0xcb0bfa11 },
{ 0xacebad03, short_gen264, long_gen264, 0xcb0bfa11 },
{ 0xacebad03, short_gen265, long_gen265, 0xcb0bfa11 },
{ 0xacebad03, short_gen266, long_gen266, 0xcb0bfa11 },
{ 0xacebad03, short_gen267, long_gen267, 0xcb0bfa11 },
{ 0xacebad03, short_gen268, long_gen268, 0xcb0bfa11 },
{ 0xacebad03, short_gen269, long_gen269, 0xcb0bfa11 },
{ 0xacebad03, short_gen270, long_gen270, 0xcb0bfa11 },
{ 0xacebad03, short_gen271, long_gen271, 0xcb0bfa11 },
{ 0xacebad03, short_gen272, long_gen272, 0xcb0bfa11 },
{ 0xacebad03, short_gen273, long_gen273, 0xcb0bfa11 },
{ 0xacebad03, short_gen274, long_gen274, 0xcb0bfa11 },
{ 0xacebad03, short_gen275, long_gen275, 0xcb0bfa11 },
{ 0xacebad03, short_gen276, long_gen276, 0xcb0bfa11 },
{ 0xacebad03, short_gen277, long_gen277, 0xcb0bfa11 },
{ 0xacebad03, short_gen278, long_gen278, 0xcb0bfa11 },
{ 0xacebad03, short_gen279, long_gen279, 0xcb0bfa11 },
{ 0xacebad03, short_gen280, long_gen280, 0xcb0bfa11 },
{ 0xacebad03, short_gen281, long_gen281, 0xcb0bfa11 },
{ 0xacebad03, short_gen282, long_gen282, 0xcb0bfa11 },
{ 0xacebad03, short_gen283, long_gen283, 0xcb0bfa11 },
{ 0xacebad03, short_gen284, long_gen284, 0xcb0bfa11 },
{ 0xacebad03, short_gen285, long_gen285, 0xcb0bfa11 },
{ 0xacebad03, short_gen286, long_gen286, 0xcb0bfa11 },
{ 0xacebad03, short_gen287, long_gen287, 0xcb0bfa11 },
{ 0xacebad03, short_gen288, long_gen288, 0xcb0bfa11 },
{ 0xacebad03, short_gen289, long_gen289, 0xcb0bfa11 },
{ 0xacebad03, short_gen290, long_gen290, 0xcb0bfa11 },
{ 0xacebad03, short_gen291, long_gen291, 0xcb0bfa11 },
{ 0xacebad03, short_gen292, long_gen292, 0xcb0bfa11 },
{ 0xacebad03, short_gen293, long_gen293, 0xcb0bfa11 },
{ 0xacebad03, short_gen294, long_gen294, 0xcb0bfa11 },
{ 0xacebad03, short_gen295, long_gen295, 0xcb0bfa11 },
{ 0xacebad03, short_gen296, long_gen296, 0xcb0bfa11 },
{ 0xacebad03, short_gen297, long_gen297, 0xcb0bfa11 },
{ 0xacebad03, short_gen298, long_gen298, 0xcb0bfa11 },
{ 0xacebad03, short_gen299, long_gen299, 0xcb0bfa11 },
{ 0xacebad03, short_gen300, long_gen300, 0xcb0bfa11 },
{ 0xacebad03, short_gen301, long_gen301, 0xcb0bfa11 },
{ 0xacebad03, short_gen302, long_gen302, 0xcb0bfa11 },
{ 0xacebad03, short_gen303, long_gen303, 0xcb0bfa11 },
{ 0xacebad03, short_gen304, long_gen304, 0xcb0bfa11 },
{ 0xacebad03, short_gen305, long_gen305, 0xcb0bfa11 },
{ 0xacebad03, short_gen306, long_gen306, 0xcb0bfa11 },
{ 0xacebad03, short_gen307, long_gen307, 0xcb0bfa11 },
{ 0xacebad03, short_gen308, long_gen308, 0xcb0bfa11 },
{ 0xacebad03, short_gen309, long_gen309, 0xcb0bfa11 },
{ 0xacebad03, short_gen310, long_gen310, 0xcb0bfa11 },
{ 0xacebad03, short_gen311, long_gen311, 0xcb0bfa11 },
{ 0xacebad03, short_gen312, long_gen312, 0xcb0bfa11 },
{ 0xacebad03, short_gen313, long_gen313, 0xcb0bfa11 },
{ 0xacebad03, short_gen314, long_gen314, 0xcb0bfa11 },
{ 0xacebad03, short_gen315, long_gen315, 0xcb0bfa11 },
{ 0xacebad03, short_gen316, long_gen316, 0xcb0bfa11 },
{ 0xacebad03, short_gen317, long_gen317, 0xcb0bfa11 },
{ 0xacebad03, short_gen318, long_gen318, 0xcb0bfa11 },
{ 0xacebad03, short_gen319, long_gen319, 0xcb0bfa11 },
{ 0xacebad03, short_gen320, long_gen320, 0xcb0bfa11 },
{ 0xacebad03, short_gen321, long_gen321, 0xcb0bfa11 },
{ 0xacebad03, short_gen322, long_gen322, 0xcb0bfa11 },
{ 0xacebad03, short_gen323, long_gen323, 0xcb0bfa11 },
{ 0xacebad03, short_gen324, long_gen324, 0xcb0bfa11 },
{ 0xacebad03, short_gen325, long_gen325, 0xcb0bfa11 },
{ 0xacebad03, short_gen326, long_gen326, 0xcb0bfa11 },
{ 0xacebad03, short_gen327, long_gen327, 0xcb0bfa11 },
{ 0xacebad03, short_gen328, long_gen328, 0xcb0bfa11 },
{ 0xacebad03, short_gen329, long_gen329, 0xcb0bfa11 },
{ 0xacebad03, short_gen330, long_gen330, 0xcb0bfa11 },
{ 0xacebad03, short_gen331, long_gen331, 0xcb0bfa11 },
{ 0xacebad03, short_gen332, long_gen332, 0xcb0bfa11 },
{ 0xacebad03, short_gen333, long_gen333, 0xcb0bfa11 },
{ 0xacebad03, short_gen334, long_gen334, 0xcb0bfa11 },
{ 0xacebad03, short_gen335, long_gen335, 0xcb0bfa11 },
{ 0xacebad03, short_gen336, long_gen336, 0xcb0bfa11 },
{ 0xacebad03, short_gen337, long_gen337, 0xcb0bfa11 },
{ 0xacebad03, short_gen338, long_gen338, 0xcb0bfa11 },
{ 0xacebad03, short_gen339, long_gen339, 0xcb0bfa11 },
{ 0xacebad03, short_gen340, long_gen340, 0xcb0bfa11 },
{ 0xacebad03, short_gen341, long_gen341, 0xcb0bfa11 },
{ 0xacebad03, short_gen342, long_gen342, 0xcb0bfa11 },
{ 0xacebad03, short_gen343, long_gen343, 0xcb0bfa11 },
{ 0xacebad03, short_gen344, long_gen344, 0xcb0bfa11 },
{ 0xacebad03, short_gen345, long_gen345, 0xcb0bfa11 },
{ 0xacebad03, short_gen346, long_gen346, 0xcb0bfa11 },
{ 0xacebad03, short_gen347, long_gen347, 0xcb0bfa11 },
{ 0xacebad03, short_gen348, long_gen348, 0xcb0bfa11 },
{ 0xacebad03, short_gen349, long_gen349, 0xcb0bfa11 },
{ 0xacebad03, short_gen350, long_gen350, 0xcb0bfa11 },
{ 0xacebad03, short_gen351, long_gen351, 0xcb0bfa11 },
{ 0xacebad03, short_gen352, long_gen352, 0xcb0bfa11 },
{ 0xacebad03, short_gen353, long_gen353, 0xcb0bfa11 },
{ 0xacebad03, short_gen354, long_gen354, 0xcb0bfa11 },
{ 0xacebad03, short_gen355, long_gen355, 0xcb0bfa11 },
{ 0xacebad03, short_gen356, long_gen356, 0xcb0bfa11 },
{ 0xacebad03, short_gen357, long_gen357, 0xcb0bfa11 },
{ 0xacebad03, short_gen358, long_gen358, 0xcb0bfa11 },
{ 0xacebad03, short_gen359, long_gen359, 0xcb0bfa11 },
{ 0xacebad03, short_gen360, long_gen360, 0xcb0bfa11 },
{ 0xacebad03, short_gen361, long_gen361, 0xcb0bfa11 },
{ 0xacebad03, short_gen362, long_gen362, 0xcb0bfa11 },
{ 0xacebad03, short_gen363, long_gen363, 0xcb0bfa11 },
{ 0xacebad03, short_gen364, long_gen364, 0xcb0bfa11 },
{ 0xacebad03, short_gen365, long_gen365, 0xcb0bfa11 },
{ 0xacebad03, short_gen366, long_gen366, 0xcb0bfa11 },
{ 0xacebad03, short_gen367, long_gen367, 0xcb0bfa11 },
{ 0xacebad03, short_gen368, long_gen368, 0xcb0bfa11 },
{ 0xacebad03, short_gen369, long_gen369, 0xcb0bfa11 },
{ 0xacebad03, short_gen370, long_gen370, 0xcb0bfa11 },
{ 0xacebad03, short_gen371, long_gen371, 0xcb0bfa11 },
{ 0xacebad03, short_gen372, long_gen372, 0xcb0bfa11 },
{ 0xacebad03, short_gen373, long_gen373, 0xcb0bfa11 },
{ 0xacebad03, short_gen374, long_gen374, 0xcb0bfa11 },
{ 0xacebad03, short_gen375, long_gen375, 0xcb0bfa11 },
{ 0xacebad03, short_gen376, long_gen376, 0xcb0bfa11 },
{ 0xacebad03, short_gen377, long_gen377, 0xcb0bfa11 },
{ 0xacebad03, short_gen378, long_gen378, 0xcb0bfa11 },
{ 0xacebad03, short_gen379, long_gen379, 0xcb0bfa11 },
{ 0xacebad03, short_gen380, long_gen380, 0xcb0bfa11 },
{ 0xacebad03, short_gen381, long_gen381, 0xcb0bfa11 },
{ 0xacebad03, short_gen382, long_gen382, 0xcb0bfa11 },
{ 0xacebad03, short_gen383, long_gen383, 0xcb0bfa11 },
{ 0xacebad03, short_gen384, long_gen384, 0xcb0bfa11 },
{ 0xacebad03, short_gen385, long_gen385, 0xcb0bfa11 },
{ 0xacebad03, short_gen386, long_gen386, 0xcb0bfa11 },
{ 0xacebad03, short_gen387, long_gen387, 0xcb0bfa11 },
{ 0xacebad03, short_gen388, long_gen388, 0xcb0bfa11 },
{ 0xacebad03, short_gen389, long_gen389, 0xcb0bfa11 },
{ 0xacebad03, short_gen390, long_gen390, 0xcb0bfa11 },
{ 0xacebad03, short_gen391, long_gen391, 0xcb0bfa11 },
{ 0xacebad03, short_gen392, long_gen392, 0xcb0bfa11 },
{ 0xacebad03, short_gen393, long_gen393, 0xcb0bfa11 },
{ 0xacebad03, short_gen394, long_gen394, 0xcb0bfa11 },
{ 0xacebad03, short_gen395, long_gen395, 0xcb0bfa11 },
{ 0xacebad03, short_gen396, long_gen396, 0xcb0bfa11 },
{ 0xacebad03, short_gen397, long_gen397, 0xcb0bfa11 },
{ 0xacebad03, short_gen398, long_gen398, 0xcb0bfa11 },
{ 0xacebad03, short_gen399, long_gen399, 0xcb0bfa11 },
{ 0xacebad03, short_gen400, long_gen400, 0xcb0bfa11 },
{ 0xacebad03, short_gen401, long_gen401, 0xcb0bfa11 },
{ 0xacebad03, short_gen402, long_gen402, 0xcb0bfa11 },
{ 0xacebad03, short_gen403, long_gen403, 0xcb0bfa11 },
{ 0xacebad03, short_gen404, long_gen404, 0xcb0bfa11 },
{ 0xacebad03, short_gen405, long_gen405, 0xcb0bfa11 },
{ 0xacebad03, short_gen406, long_gen406, 0xcb0bfa11 },
{ 0xacebad03, short_gen407, long_gen407, 0xcb0bfa11 },
{ 0xacebad03, short_gen408, long_gen408, 0xcb0bfa11 },
{ 0xacebad03, short_gen409, long_gen409, 0xcb0bfa11 },
{ 0xacebad03, short_gen410, long_gen410, 0xcb0bfa11 },
{ 0xacebad03, short_gen411, long_gen411, 0xcb0bfa11 },
{ 0xacebad03, short_gen412, long_gen412, 0xcb0bfa11 },
{ 0xacebad03, short_gen413, long_gen413, 0xcb0bfa11 },
{ 0xacebad03, short_gen414, long_gen414, 0xcb0bfa11 },
{ 0xacebad03, short_gen415, long_gen415, 0xcb0bfa11 },
{ 0xacebad03, short_gen416, long_gen416, 0xcb0bfa11 },
{ 0xacebad03, short_gen417, long_gen417, 0xcb0bfa11 },
{ 0xacebad03, short_gen418, long_gen418, 0xcb0bfa11 },
{ 0xacebad03, short_gen419, long_gen419, 0xcb0bfa11 },
{ 0xacebad03, short_gen420, long_gen420, 0xcb0bfa11 },
{ 0xacebad03, short_gen421, long_gen421, 0xcb0bfa11 },
{ 0xacebad03, short_gen422, long_gen422, 0xcb0bfa11 },
{ 0xacebad03, short_gen423, long_gen423, 0xcb0bfa11 },
{ 0xacebad03, short_gen424, long_gen424, 0xcb0bfa11 },
{ 0xacebad03, short_gen425, long_gen425, 0xcb0bfa11 },
{ 0xacebad03, short_gen426, long_gen426, 0xcb0bfa11 },
{ 0xacebad03, short_gen427, long_gen427, 0xcb0bfa11 },
{ 0xacebad03, short_gen428, long_gen428, 0xcb0bfa11 },
{ 0xacebad03, short_gen429, long_gen429, 0xcb0bfa11 },
{ 0xacebad03, short_gen430, long_gen430, 0xcb0bfa11 },
{ 0xacebad03, short_gen431, long_gen431, 0xcb0bfa11 },
{ 0xacebad03, short_gen432, long_gen432, 0xcb0bfa11 },
{ 0xacebad03, short_gen433, long_gen433, 0xcb0bfa11 },
{ 0xacebad03, short_gen434, long_gen434, 0xcb0bfa11 },
{ 0xacebad03, short_gen435, long_gen435, 0xcb0bfa11 },
{ 0xacebad03, short_gen436, long_gen436, 0xcb0bfa11 },
{ 0xacebad03, short_gen437, long_gen437, 0xcb0bfa11 },
{ 0xacebad03, short_gen438, long_gen438, 0xcb0bfa11 },
{ 0xacebad03, short_gen439, long_gen439, 0xcb0bfa11 },
{ 0xacebad03, short_gen440, long_gen440, 0xcb0bfa11 },
{ 0xacebad03, short_gen441, long_gen441, 0xcb0bfa11 },
{ 0xacebad03, short_gen442, long_gen442, 0xcb0bfa11 },
{ 0xacebad03, short_gen443, long_gen443, 0xcb0bfa11 },
{ 0xacebad03, short_gen444, long_gen444, 0xcb0bfa11 },
{ 0xacebad03, short_gen445, long_gen445, 0xcb0bfa11 },
{ 0xacebad03, short_gen446, long_gen446, 0xcb0bfa11 },
{ 0xacebad03, short_gen447, long_gen447, 0xcb0bfa11 },
{ 0xacebad03, short_gen448, long_gen448, 0xcb0bfa11 },
{ 0xacebad03, short_gen449, long_gen449, 0xcb0bfa11 },
{ 0xacebad03, short_gen450, long_gen450, 0xcb0bfa11 },
{ 0xacebad03, short_gen451, long_gen451, 0xcb0bfa11 },
{ 0xacebad03, short_gen452, long_gen452, 0xcb0bfa11 },
{ 0xacebad03, short_gen453, long_gen453, 0xcb0bfa11 },
{ 0xacebad03, short_gen454, long_gen454, 0xcb0bfa11 },
{ 0xacebad03, short_gen455, long_gen455, 0xcb0bfa11 },
{ 0xacebad03, short_gen456, long_gen456, 0xcb0bfa11 },
{ 0xacebad03, short_gen457, long_gen457, 0xcb0bfa11 },
{ 0xacebad03, short_gen458, long_gen458, 0xcb0bfa11 },
{ 0xacebad03, short_gen459, long_gen459, 0xcb0bfa11 },
{ 0xacebad03, short_gen460, long_gen460, 0xcb0bfa11 },
{ 0xacebad03, short_gen461, long_gen461, 0xcb0bfa11 },
{ 0xacebad03, short_gen462, long_gen462, 0xcb0bfa11 },
{ 0xacebad03, short_gen463, long_gen463, 0xcb0bfa11 },
{ 0xacebad03, short_gen464, long_gen464, 0xcb0bfa11 },
{ 0xacebad03, short_gen465, long_gen465, 0xcb0bfa11 },
{ 0xacebad03, short_gen466, long_gen466, 0xcb0bfa11 },
{ 0xacebad03, short_gen467, long_gen467, 0xcb0bfa11 },
{ 0xacebad03, short_gen468, long_gen468, 0xcb0bfa11 },
{ 0xacebad03, short_gen469, long_gen469, 0xcb0bfa11 },
{ 0xacebad03, short_gen470, long_gen470, 0xcb0bfa11 },
{ 0xacebad03, short_gen471, long_gen471, 0xcb0bfa11 },
{ 0xacebad03, short_gen472, long_gen472, 0xcb0bfa11 },
{ 0xacebad03, short_gen473, long_gen473, 0xcb0bfa11 },
{ 0xacebad03, short_gen474, long_gen474, 0xcb0bfa11 },
{ 0xacebad03, short_gen475, long_gen475, 0xcb0bfa11 },
{ 0xacebad03, short_gen476, long_gen476, 0xcb0bfa11 },
{ 0xacebad03, short_gen477, long_gen477, 0xcb0bfa11 },
{ 0xacebad03, short_gen478, long_gen478, 0xcb0bfa11 },
{ 0xacebad03, short_gen479, long_gen479, 0xcb0bfa11 },
{ 0xacebad03, short_gen480, long_gen480, 0xcb0bfa11 },
{ 0xacebad03, short_gen481, long_gen481, 0xcb0bfa11 },
{ 0xacebad03, short_gen482, long_gen482, 0xcb0bfa11 },
{ 0xacebad03, short_gen483, long_gen483, 0xcb0bfa11 },
{ 0xacebad03, short_gen484, long_gen484, 0xcb0bfa11 },
{ 0xacebad03, short_gen485, long_gen485, 0xcb0bfa11 },
{ 0xacebad03, short_gen486, long_gen486, 0xcb0bfa11 },
{ 0xacebad03, short_gen487, long_gen487, 0xcb0bfa11 },
{ 0xacebad03, short_gen488, long_gen488, 0xcb0bfa11 },
{ 0xacebad03, short_gen489, long_gen489, 0xcb0bfa11 },
{ 0xacebad03, short_gen490, long_gen490, 0xcb0bfa11 },
{ 0xacebad03, short_gen491, long_gen491, 0xcb0bfa11 },
{ 0xacebad03, short_gen492, long_gen492, 0xcb0bfa11 },
{ 0xacebad03, short_gen493, long_gen493, 0xcb0bfa11 },
{ 0xacebad03, short_gen494, long_gen494, 0xcb0bfa11 },
{ 0xacebad03, short_gen495, long_gen495, 0xcb0bfa11 },
{ 0xacebad03, short_gen496, long_gen496, 0xcb0bfa11 },
{ 0xacebad03, short_gen497, long_gen497, 0xcb0bfa11 },
{ 0xacebad03, short_gen498, long_gen498, 0xcb0bfa11 },
{ 0xacebad03, short_gen499, long_gen499, 0xcb0bfa11 },
{ 0xacebad03, short_gen500, long_gen500, 0xcb0bfa11 },
{ 0xacebad03, short_gen501, long_gen501, 0xcb0bfa11 },
{ 0xacebad03, short_gen502, long_gen502, 0xcb0bfa11 },
{ 0xacebad03, short_gen503, long_gen503, 0xcb0bfa11 },
{ 0xacebad03, short_gen504, long_gen504, 0xcb0bfa11 },
{ 0xacebad03, short_gen505, long_gen505, 0xcb0bfa11 },
{ 0xacebad03, short_gen506, long_gen506, 0xcb0bfa11 },
{ 0xacebad03, short_gen507, long_gen507, 0xcb0bfa11 },
{ 0xacebad03, short_gen508, long_gen508, 0xcb0bfa11 },
{ 0xacebad03, short_gen509, long_gen509, 0xcb0bfa11 },
{ 0xacebad03, short_gen510, long_gen510, 0xcb0bfa11 },
{ 0xacebad03, short_gen511, long_gen511, 0xcb0bfa11 },
{ 0xacebad03, short_gen512, long_gen512, 0xcb0bfa11 },
{ 0xacebad03, short_gen513, long_gen513, 0xcb0bfa11 },
{ 0xacebad03, short_gen514, long_gen514, 0xcb0bfa11 },
{ 0xacebad03, short_gen515, long_gen515, 0xcb0bfa11 },
{ 0xacebad03, short_gen516, long_gen516, 0xcb0bfa11 },
{ 0xacebad03, short_gen517, long_gen517, 0xcb0bfa11 },
{ 0xacebad03, short_gen518, long_gen518, 0xcb0bfa11 },
{ 0xacebad03, short_gen519, long_gen519, 0xcb0bfa11 },
{ 0xacebad03, short_gen520, long_gen520, 0xcb0bfa11 },
{ 0xacebad03, short_gen521, long_gen521, 0xcb0bfa11 },
{ 0xacebad03, short_gen522, long_gen522, 0xcb0bfa11 },
{ 0xacebad03, short_gen523, long_gen523, 0xcb0bfa11 },
{ 0xacebad03, short_gen524, long_gen524, 0xcb0bfa11 },
{ 0xacebad03, short_gen525, long_gen525, 0xcb0bfa11 },
{ 0xacebad03, short_gen526, long_gen526, 0xcb0bfa11 },
{ 0xacebad03, short_gen527, long_gen527, 0xcb0bfa11 },
{ 0xacebad03, short_gen528, long_gen528, 0xcb0bfa11 },
{ 0xacebad03, short_gen529, long_gen529, 0xcb0bfa11 },
{ 0xacebad03, short_gen530, long_gen530, 0xcb0bfa11 },
{ 0xacebad03, short_gen531, long_gen531, 0xcb0bfa11 },
{ 0xacebad03, short_gen532, long_gen532, 0xcb0bfa11 },
{ 0xacebad03, short_gen533, long_gen533, 0xcb0bfa11 },
{ 0xacebad03, short_gen534, long_gen534, 0xcb0bfa11 },
{ 0xacebad03, short_gen535, long_gen535, 0xcb0bfa11 },
{ 0xacebad03, short_gen536, long_gen536, 0xcb0bfa11 },
{ 0xacebad03, short_gen537, long_gen537, 0xcb0bfa11 },
{ 0xacebad03, short_gen538, long_gen538, 0xcb0bfa11 },
{ 0xacebad03, short_gen539, long_gen539, 0xcb0bfa11 },
{ 0xacebad03, short_gen540, long_gen540, 0xcb0bfa11 },
{ 0xacebad03, short_gen541, long_gen541, 0xcb0bfa11 },
{ 0xacebad03, short_gen542, long_gen542, 0xcb0bfa11 },
{ 0xacebad03, short_gen543, long_gen543, 0xcb0bfa11 },
{ 0xacebad03, short_gen544, long_gen544, 0xcb0bfa11 },
{ 0xacebad03, short_gen545, long_gen545, 0xcb0bfa11 },
{ 0xacebad03, short_gen546, long_gen546, 0xcb0bfa11 },
{ 0xacebad03, short_gen547, long_gen547, 0xcb0bfa11 },
{ 0xacebad03, short_gen548, long_gen548, 0xcb0bfa11 },
{ 0xacebad03, short_gen549, long_gen549, 0xcb0bfa11 },
{ 0xacebad03, short_gen550, long_gen550, 0xcb0bfa11 },
{ 0xacebad03, short_gen551, long_gen551, 0xcb0bfa11 },
{ 0xacebad03, short_gen552, long_gen552, 0xcb0bfa11 },
{ 0xacebad03, short_gen553, long_gen553, 0xcb0bfa11 },
{ 0xacebad03, short_gen554, long_gen554, 0xcb0bfa11 },
{ 0xacebad03, short_gen555, long_gen555, 0xcb0bfa11 },
{ 0xacebad03, short_gen556, long_gen556, 0xcb0bfa11 },
{ 0xacebad03, short_gen557, long_gen557, 0xcb0bfa11 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_GENERIC
static const char short_spc0[] = "**CreateFileMapping %d";
static const char long_spc0[]  = "CreateFileMapping failed, error %d";
static const char short_spc1[] = "**CreateThread %d";
static const char long_spc1[]  = "CreateThread failed, error %d";
static const char short_spc2[] = "**FindWindowEx %d";
static const char long_spc2[]  = "FindWindowEx failed, error %d";
static const char short_spc3[] = "**MPIDI_CH3I_SHM_Attach_to_mem %d";
static const char long_spc3[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed, error %d";
static const char short_spc4[] = "**MapViewOfFileEx %d";
static const char long_spc4[]  = "MapViewOfFileEx failed, error %d";
static const char short_spc5[] = "**OpenProcess %d %d";
static const char long_spc5[]  = "OpenProcess failed for process %d, error %d";
static const char short_spc6[] = "**arg %s";
static const char long_spc6[]  = "Invalid argument %s";
static const char short_spc7[] = "**argarrayneg %s %d %d";
static const char long_spc7[]  = "Negative value in array %s[%d] (value is %d)";
static const char short_spc8[] = "**argerrcode %d";
static const char long_spc8[]  = "Invalid error code %d";
static const char short_spc9[] = "**argneg %s %d";
static const char long_spc9[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc10[] = "**argnonpos %s %d";
static const char long_spc10[]  = "Invalid value for %s; must be positive but is %d";
static const char short_spc11[] = "**argrange %s %d %d";
static const char long_spc11[]  = "Argument %s has value %d but must be within [0,%d]";
static const char short_spc12[] = "**attach_to_mem %d";
static const char long_spc12[]  = "attach to shared memory returned error %d";
static const char short_spc13[] = "**bad_conn %p %p";
static const char long_spc13[]  = "bad conn structure pointer (%p != %p)";
static const char short_spc14[] = "**bad_sock %d %d";
static const char long_spc14[]  = "bad sock (%d != %d)";
static const char short_spc15[] = "**badpacket %d";
static const char long_spc15[]  = "Received a packet of unknown type (%d)";
static const char short_spc16[] = "**boot_attach %s";
static const char long_spc16[]  = "failed to attach to a bootstrap queue - %s";
static const char short_spc17[] = "**bootqmsg %d";
static const char long_spc17[]  = "invalid bootstrap queue message size (%d bytes)";
static const char short_spc18[] = "**bsendbufsmall %d %d";
static const char long_spc18[]  = "Buffer size of %d is smaller than MPI_BSEND_OVERHEAD (%d)";
static const char short_spc19[] = "**bufbsend %d %d";
static const char long_spc19[]  = "Insufficient space in Bsend buffer; requested %d; total buffer size is %d";
static const char short_spc20[] = "**business_card %s";
static const char long_spc20[]  = "Invalid business card (%s)";
static const char short_spc21[] = "**ca_guids %s";
static const char long_spc21[]  = "unable to get the infiniband guids - %s";
static const char short_spc22[] = "**cartcoordinvalid %d %d %d";
static const char long_spc22[]  = " Cartesian coordinate for the %d coordinate is %d but must be between 0 and %d";
static const char short_spc23[] = "**cartdim %d %d";
static const char long_spc23[]  = "Size of the communicator (%d) is smaller than the size of the Cartesian topology (%d)";
static const char short_spc24[] = "**ch3|badca %d";
static const char long_spc24[]  = "specified completion action in not known (%d)";
static const char short_spc25[] = "**ch3|badmsgtype %d";
static const char long_spc25[]  = "request contained an invalid message type (%d)";
static const char short_spc26[] = "**ch3|badreqtype %d";
static const char long_spc26[]  = "request contained an invalid request type (%d)";
static const char short_spc27[] = "**ch3|loadrecviov %s";
static const char long_spc27[]  = "failure occurred while loading the receive I/O vector (%s)";
static const char short_spc28[] = "**ch3|postrecv %s";
static const char long_spc28[]  = "failure occurred while posting a receive for message data (%s)";
static const char short_spc29[] = "**ch3|recvdata %s";
static const char long_spc29[]  = "failure occurred while attempting to receive message data (%s)";
static const char short_spc30[] = "**ch3|sock|badbuscard %s";
static const char long_spc30[]  = "[ch3:sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc31[] = "**ch3|sock|badhost %s %d %s";
static const char long_spc31[]  = "[ch3:sock] invalid host description, %s:%d (%s)";
static const char short_spc32[] = "**ch3|sock|badpacket %d";
static const char long_spc32[]  = "[ch3:sock] received packet of unknown type (%d)";
static const char short_spc33[] = "**ch3|sock|connfailed %d %d";
static const char long_spc33[]  = "[ch3:sock] failed to connnect to remote process %d:%d";
static const char short_spc34[] = "**ch3|sock|connrefused %s %d %s";
static const char long_spc34[]  = "[ch3:sock] failed to connect to process %s:%d (%s)";
static const char short_spc35[] = "**ch3|sock|failure %d";
static const char long_spc35[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc36[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc36[]  = "PMI_Finalize failed, error %d";
static const char short_spc37[] = "**ch3|sock|postconnect %d %d %s";
static const char long_spc37[]  = "[ch3:sock] rank %d unable to connect to rank %d using business card <%s>";
static const char short_spc38[] = "**ch3|unknownpkt %d";
static const char long_spc38[]  = "received unknown packet type (type=%d)";
static const char short_spc39[] = "**commperm %s";
static const char long_spc39[]  = "Cannot free permanent communicator %s";
static const char short_spc40[] = "**connfailed %d %d";
static const char long_spc40[]  = "Failed to connect to remote process %d-%d";
static const char short_spc41[] = "**connrefused %d %d %s";
static const char long_spc41[]  = "Connection refused for process group %d, rank %d, business card <%s>";
static const char short_spc42[] = "**countneg %d";
static const char long_spc42[]  = "Negative count, value is %d";
static const char short_spc43[] = "**darrayblock %d";
static const char long_spc43[]  = " Value of m in block(m) distribution is %d must must be positive";
static const char short_spc44[] = "**darrayblock2 %d %d";
static const char long_spc44[]  = "m * nprocs is %d but must equal the array size %d and is not valid for block(m) distribution";
static const char short_spc45[] = "**darraycyclic %d";
static const char long_spc45[]  = "Value of m is %d but must be positive for a cyclic(m) distribution";
static const char short_spc46[] = "**darraydist %d %d";
static const char long_spc46[]  = "For MPI_DISTRIBUTE_NONE, the value of array_of_psizes[%d] is %d but must have value 1";
static const char short_spc47[] = "**dims %d";
static const char long_spc47[]  = "Invalid dimension argument (value is %d)";
static const char short_spc48[] = "**dimsmany %d %d";
static const char long_spc48[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc49[] = "**duphandle %d";
static const char long_spc49[]  = "unable to duplicate a handle (errno %d)";
static const char short_spc50[] = "**dupprocesses %d";
static const char long_spc50[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes; both contain process %d";
static const char short_spc51[] = "**edgeoutrange %d %d %d";
static const char long_spc51[]  = "Edge index edges[%d] is %d but must be nonnegative and less than %d";
static const char short_spc52[] = "**fail %d";
static const char long_spc52[]  = "failure (errno %d)";
static const char short_spc53[] = "**fail %s";
static const char long_spc53[]  = "failure - %s";
static const char short_spc54[] = "**fileaccess %s";
static const char long_spc54[]  = "Access denied to file %s";
static const char short_spc55[] = "**filenamedir %s";
static const char long_spc55[]  = "Invalid or missing directory %s";
static const char short_spc56[] = "**filenamelong %s %d";
static const char long_spc56[]  = "Pathname %s too long (%d characters)";
static const char short_spc57[] = "**filenoexist %s";
static const char long_spc57[]  = "File %s does not exist";
static const char short_spc58[] = "**filerdonly %s";
static const char long_spc58[]  = "Read-only file or filesystem name %s";
static const char short_spc59[] = "**ftok %s %d %d";
static const char long_spc59[]  = "failed to create a sysv key from the file '%s' and id %d, error %d";
static const char short_spc60[] = "**ftruncate %s %d %d";
static const char long_spc60[]  = "unable to resize the shared memory object %s to size %d (errno %d)";
static const char short_spc61[] = "**gethostbyname %d";
static const char long_spc61[]  = "gethostbyname failed (errno %d)";
static const char short_spc62[] = "**getinfo %d";
static const char long_spc62[]  = "getaddrinfo failed (errno %d)";
static const char short_spc63[] = "**groupnotincomm %d";
static const char long_spc63[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc64[] = "**hostlookup %d %d %s";
static const char long_spc64[]  = "Host lookup failed for process group %d, rank %d, business card <%s>";
static const char short_spc65[] = "**ibu_op %d";
static const char long_spc65[]  = "invalid infiniband operation (%d)";
static const char short_spc66[] = "**ibu_wait %d";
static const char long_spc66[]  = "ibu_wait failed (errno %d)";
static const char short_spc67[] = "**indexneg %d %d";
static const char long_spc67[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc68[] = "**indexnonmonotone %d %d %d";
static const char long_spc68[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc69[] = "**infonkey %d %d";
static const char long_spc69[]  = "Requested key %d but this MPI_Info only has %d keys";
static const char short_spc70[] = "**infonokey %s";
static const char long_spc70[]  = "MPI_Info key %s is not defined ";
static const char short_spc71[] = "**intern %s";
static const char long_spc71[]  = "Internal MPI error!  %s";
static const char short_spc72[] = "**invalid_handle %d";
static const char long_spc72[]  = "invalid handle (%d)";
static const char short_spc73[] = "**invalid_listener %p";
static const char long_spc73[]  = "invalid listener (%p)";
static const char short_spc74[] = "**invalid_refcount %d";
static const char long_spc74[]  = "invalid reference count (%d)";
static const char short_spc75[] = "**io %s";
static const char long_spc75[]  = "Other I/O error %s";
static const char short_spc76[] = "**iocp %d";
static const char long_spc76[]  = "unable to create an I/O completion port (errno %d)";
static const char short_spc77[] = "**iov_offset %d %d";
static const char long_spc77[]  = "invalid iov offset (%d > %d)";
static const char short_spc78[] = "**listen %d";
static const char long_spc78[]  = "listen failed (errno %d)";
static const char short_spc79[] = "**mmap %d";
static const char long_spc79[]  = " unable to map memory, mmap failed (errno %d)";
static const char short_spc80[] = "**mpi_accumulate %p %d %D %d %d %d %D %O %W";
static const char long_spc80[]  = "MPI_Accumulate(%p, %d, %D, %d, %d, %d, %D, %O, %W) failed";
static const char short_spc81[] = "**mpi_add_error_string %d %s";
static const char long_spc81[]  = "MPI_Add_error_string(code=%d, str=\"%s\") failed";
static const char short_spc82[] = "**mpi_allgather %p %d %D %p %d %D %C";
static const char long_spc82[]  = "MPI_Allgather(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, %C) failed";
static const char short_spc83[] = "**mpi_allgatherv %p %d %D %p %p %p %D %C";
static const char long_spc83[]  = "MPI_Allgatherv(sbuf=%p, scount=%d, %D, rbuf=%p, rcounts=%p, displs=%p, %D, %C) failed";
static const char short_spc84[] = "**mpi_allreduce %p %p %d %D %O %C";
static const char long_spc84[]  = "MPI_Allreduce(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc85[] = "**mpi_alltoall %p %d %D %p %d %D %C";
static const char long_spc85[]  = "MPI_Alltoall(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, %C) failed";
static const char short_spc86[] = "**mpi_alltoallv %p %p %p %D %p %p %p %D %C";
static const char long_spc86[]  = "MPI_Alltoallv(sbuf=%p, scnts=%p, sdispls=%p, %D, rbuf=%p, rcnts=%p, rdispls=%p, %D, %C) failed";
static const char short_spc87[] = "**mpi_alltoallw %p %p %p %p %p %p %p %p %C";
static const char long_spc87[]  = "MPI_Alltoallw(sbuf=%p, scnts=%p, sdispls=%p, stypes=%p, rbuf=%p, rcnts=%p, rdispls=%p, rtypes=%p, %C) failed";
static const char short_spc88[] = "**mpi_attr_delete %C %d";
static const char long_spc88[]  = "MPI_Attr_delete(%C, %d) failed";
static const char short_spc89[] = "**mpi_attr_get %C %d %p %p";
static const char long_spc89[]  = "MPI_Atr_get(%C, %d, %p, %p) failed";
static const char short_spc90[] = "**mpi_attr_put %C %d %p";
static const char long_spc90[]  = "MPI_Attr_put(%C, %d, %p) failed";
static const char short_spc91[] = "**mpi_barrier %C";
static const char long_spc91[]  = "MPI_Barrier(%C) failed";
static const char short_spc92[] = "**mpi_bcast %p %d %D %d %C";
static const char long_spc92[]  = "MPI_Bcast(buf=%p, count=%d, %D, root=%d, %C) failed";
static const char short_spc93[] = "**mpi_bsend %p %d %D %d %d %C";
static const char long_spc93[]  = "MPI_Bsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc94[] = "**mpi_bsend_init %p %d %D %d %d %C %p";
static const char long_spc94[]  = "MPI_Bsend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc95[] = "**mpi_buffer_attach %p %d";
static const char long_spc95[]  = "MPI_Buffer_attach(buf=%p, size=%d) failed";
static const char short_spc96[] = "**mpi_buffer_detach %p %p";
static const char long_spc96[]  = "MPI_Buffer_detach(buf=%p, size=%p) failed";
static const char short_spc97[] = "**mpi_cancel %p";
static const char long_spc97[]  = "MPI_Cancel(request=%p) failed";
static const char short_spc98[] = "**mpi_cart_create %C %d %p %p %d %p";
static const char long_spc98[]  = "MPI_Cart_create(%C, %d, %p, %p, %d, %p) failed";
static const char short_spc99[] = "**mpi_close_port %s";
static const char long_spc99[]  = "MPI_Close_port(port=\"%s\") failed";
static const char short_spc100[] = "**mpi_comm_accept %s %I %d %C %p";
static const char long_spc100[]  = "MPI_Comm_accept(port=\"%s\", %I, root=%d, %C, newcomm=%p) failed";
static const char short_spc101[] = "**mpi_comm_connect %s %I %d %C %p";
static const char long_spc101[]  = "MPI_Comm_connect(port=\"%s\", %I, root=%d, %C, newcomm=%p) failed";
static const char short_spc102[] = "**mpi_comm_disconnect %p";
static const char long_spc102[]  = "MPI_Comm_disconnect(comm=%p) failed";
static const char short_spc103[] = "**mpi_comm_dup %C %p";
static const char long_spc103[]  = "MPI_Comm_dup(%C, %p) failed";
static const char short_spc104[] = "**mpi_comm_free %p";
static const char long_spc104[]  = "MPI_Comm_free(%p) failed";
static const char short_spc105[] = "**mpi_comm_group %C %p";
static const char long_spc105[]  = "MPI_Comm_group(%C, %p) failed";
static const char short_spc106[] = "**mpi_comm_join %d %p";
static const char long_spc106[]  = "MPI_Comm_join(fd=%d, intercomm=%p) failed";
static const char short_spc107[] = "**mpi_comm_remote_group %C %p";
static const char long_spc107[]  = "MPI_Comm_remote_group(%C, %p) failed";
static const char short_spc108[] = "**mpi_comm_set_attr %C %d %p";
static const char long_spc108[]  = "MPI_Comm_set_attr(%C, %d, %p) failed";
static const char short_spc109[] = "**mpi_comm_spawn %s %p %d %I %d %C %p %p";
static const char long_spc109[]  = "MPI_Comm_spawn(cmd=\"%s\", argv=%p, maxprocs=%d, %I, root=%d, %C, intercomm=%p, errors=%p) failed";
static const char short_spc110[] = "**mpi_comm_spawn_multiple %d %p %p %p %p %d %C %p %p";
static const char long_spc110[]  = "MPI_Comm_spawn_multiple(count=%d, cmds=%p, argvs=%p, maxprocs=%p, infos=%p, root=%d, %C, intercomm=%p, errors=%p) failed";
static const char short_spc111[] = "**mpi_comm_split %C %d %d %p";
static const char long_spc111[]  = "MPI_Comm_split(%C, %d, %d, %p) failed";
static const char short_spc112[] = "**mpi_errhandler_create %p %p";
static const char long_spc112[]  = "MPI_Errhandler_create(%p) failed";
static const char short_spc113[] = "**mpi_errhandler_get %C %p";
static const char long_spc113[]  = "MPI_Errhandler_get(%C, %p) failed";
static const char short_spc114[] = "**mpi_errhandler_set %C %E";
static const char long_spc114[]  = "MPI_Errhandler_set(%C, %E) failed";
static const char short_spc115[] = "**mpi_exscan %p %p %d %D %O %C";
static const char long_spc115[]  = "MPI_Exscan(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc116[] = "**mpi_file_create_errhandler %p %p";
static const char long_spc116[]  = "MPI_File_create_errhandler(%p, %p) failed";
static const char short_spc117[] = "**mpi_gather %p %d %D %p %d %D %d %C";
static const char long_spc117[]  = "MPI_Gather(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc118[] = "**mpi_gatherv %p %d %D %p %p %p %D %d %C";
static const char long_spc118[]  = "MPI_Gatherv failed(sbuf=%p, scount=%d, %D, rbuf=%p, rcnts=%p, displs=%p, %D, root=%d, %C) failed";
static const char short_spc119[] = "**mpi_get %p %d %D %d %d %d %D %W";
static const char long_spc119[]  = "MPI_Get(%p, %d, %D, %d, %d, %d, %D, %W) failed";
static const char short_spc120[] = "**mpi_get_processor_name %p %p";
static const char long_spc120[]  = "MPI_Get_processor_name(%p, %p) failed";
static const char short_spc121[] = "**mpi_graph_create %C %d %p %p %d %p";
static const char long_spc121[]  = "MPI_Graph_create(%C, %d, %p, %p, %d, %p) failed";
static const char short_spc122[] = "**mpi_graph_map %C %d %p %p %p";
static const char long_spc122[]  = "MPI_Graph_map(%C, %d, %p, %p, %p) failed";
static const char short_spc123[] = "**mpi_grequest_start %p %p %p %p %p";
static const char long_spc123[]  = "MPI_Grequest_start(%p, %p, %p, %p, %p) failed";
static const char short_spc124[] = "**mpi_group_difference %G %G %p";
static const char long_spc124[]  = "MPI_Group_difference(%G, %G, %p) failed";
static const char short_spc125[] = "**mpi_group_excl %G %d %p %p";
static const char long_spc125[]  = "MPI_Group_excl(%G, %d, %p, %p) failed";
static const char short_spc126[] = "**mpi_group_free %p";
static const char long_spc126[]  = "MPI_Group_free(%p) failed";
static const char short_spc127[] = "**mpi_group_incl %G %d %p %p";
static const char long_spc127[]  = "MPI_Group_incl(%G, %d, %p, %p) failed";
static const char short_spc128[] = "**mpi_group_intersection %G %G %p";
static const char long_spc128[]  = "MPI_Group_intersection(%G, %G, %p) failed";
static const char short_spc129[] = "**mpi_group_range_excl %G %d %p %p";
static const char long_spc129[]  = "MPI_Group_range_excl(%G, %d, %p, %p) failed";
static const char short_spc130[] = "**mpi_group_range_incl %G %d %p %p";
static const char long_spc130[]  = "MPI_Group_range_incl(%G, %d, %p, %p) failed";
static const char short_spc131[] = "**mpi_group_union %G %G %p";
static const char long_spc131[]  = "MPI_Group_union(%G, %G, %p) failed";
static const char short_spc132[] = "**mpi_ibsend %p %d %D %d %d %C %p";
static const char long_spc132[]  = "MPI_Ibsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc133[] = "**mpi_info_create %p";
static const char long_spc133[]  = "MPI_Info_create(%p) failed";
static const char short_spc134[] = "**mpi_info_delete %I %p";
static const char long_spc134[]  = "MPI_Info_delete(%I, %p) failed";
static const char short_spc135[] = "**mpi_info_dup %I %p";
static const char long_spc135[]  = "MPI_Info_dup(%I, %p) failed";
static const char short_spc136[] = "**mpi_info_get_nthkey %I %d %p";
static const char long_spc136[]  = "MPI_Info_get_nthkey(%I, %d, %p) failed";
static const char short_spc137[] = "**mpi_info_set %I %p %p";
static const char long_spc137[]  = "MPI_Info_set(%I, %p, %p) failed";
static const char short_spc138[] = "**mpi_init %p %p";
static const char long_spc138[]  = "MPI_Init(%p, %p) failed";
static const char short_spc139[] = "**mpi_init_thread %p %p %d %p";
static const char long_spc139[]  = "MPI_Init_thread(%p, %p, %d, %p)";
static const char short_spc140[] = "**mpi_intercomm_create %C %d %C %d %d %p";
static const char long_spc140[]  = "MPI_Intercomm_create(%C, %d, %C, %d, %d, %p) failed";
static const char short_spc141[] = "**mpi_intercomm_merge %C %d %p";
static const char long_spc141[]  = "MPI_Intercomm_merge(%C, %d, %p) failed";
static const char short_spc142[] = "**mpi_iprobe %d %d %C %p %p";
static const char long_spc142[]  = "MPI_Iprobe(src=%d, tag=%d, %C, flag=%p, status=%p) failed";
static const char short_spc143[] = "**mpi_irecv %p %d %D %d %d %C %p";
static const char long_spc143[]  = "MPI_Irecv(buf=%p, count=%d, %D, src=%d, tag=%d, %C, request=%p) failed";
static const char short_spc144[] = "**mpi_irsend %p %d %D %d %d %C %p";
static const char long_spc144[]  = "MPI_Irsend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc145[] = "**mpi_isend %p %d %D %d %d %C %p";
static const char long_spc145[]  = "MPI_Isend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc146[] = "**mpi_issend %p %d %D %d %d %C %p";
static const char long_spc146[]  = "MPI_Issend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc147[] = "**mpi_keyval_create %p %p %p %p";
static const char long_spc147[]  = "MPI_Keyval_create(%p, %p, %p, %p) failed";
static const char short_spc148[] = "**mpi_keyval_free %p";
static const char long_spc148[]  = "MPI_Keyval_free(%p) failed";
static const char short_spc149[] = "**mpi_lookup_name %s %I %p";
static const char long_spc149[]  = "MPI_Lookup_name(service=\"%s\", %I, port=%p) failed";
static const char short_spc150[] = "**mpi_op_create %p %d %p";
static const char long_spc150[]  = "MPI_Op_create(fn=%p, commute=%d, op=%p) failed";
static const char short_spc151[] = "**mpi_open_port %I %p";
static const char long_spc151[]  = "MPI_Open_port(%I, port=%p) failed";
static const char short_spc152[] = "**mpi_pack %p %d %D %p %d %p %C";
static const char long_spc152[]  = "MPI_Pack(inbuf=%p, incount=%d, %D, outbuf=%p, outcount=%d, position=%p, %C) failed";
static const char short_spc153[] = "**mpi_pack_external %s %p %d %D %p %d %p";
static const char long_spc153[]  = "MPI_Pack_external(%s, %p, %d, %D, %p, %d, %p) failed";
static const char short_spc154[] = "**mpi_probe %d %d %C %p";
static const char long_spc154[]  = "MPI_Probe(src=%d, tag=%d, %C, status=%p) failed";
static const char short_spc155[] = "**mpi_publish_name %s %I %s";
static const char long_spc155[]  = "MPI_Publish_name(service=\"%s\", %I, port=\"%s\") failed";
static const char short_spc156[] = "**mpi_put %p %d %D %d %d %d %D %W";
static const char long_spc156[]  = "MPI_Put(%p, %d, %D, %d, %d, %d, %D, %W) failed";
static const char short_spc157[] = "**mpi_recv %p %d %D %d %d %C %p";
static const char long_spc157[]  = "MPI_Recv(buf=%p, count=%d, %D, src=%d, %C, status=%p) failed";
static const char short_spc158[] = "**mpi_recv_init %p %d %D %d %d %C %p";
static const char long_spc158[]  = "MPI_Recv_init(buf=%p, count=%d, %D, src=%d, tag=%d, %C, request=%p) failed";
static const char short_spc159[] = "**mpi_reduce %p %p %d %D %O %d %C";
static const char long_spc159[]  = "MPI_Reduce(sbuf=%p, rbuf=%p, count=%d, %D, %O, root=%d, %C) failed";
static const char short_spc160[] = "**mpi_reduce_scatter %p %p %p %D %O %C";
static const char long_spc160[]  = "MPI_Reduce_scatter(sbuf=%p, rbuf=%p, rcnts=%p, %D, %O, %C) failed";
static const char short_spc161[] = "**mpi_register_datarep %s %p %p %p %p";
static const char long_spc161[]  = "MPI_Register_Datarep(%s %p %p %p %p) failed";
static const char short_spc162[] = "**mpi_request_free %p";
static const char long_spc162[]  = "MPI_Request_free(request=%p) failed";
static const char short_spc163[] = "**mpi_request_get_status %R %p %p";
static const char long_spc163[]  = "MPI_Request_get_status(%R, %p, %p) failed";
static const char short_spc164[] = "**mpi_rsend %p %d %D %d %d %C";
static const char long_spc164[]  = "MPI_Rsend(buf=%p, count=%d, %D, src=%d, tag=%d, %C) failed";
static const char short_spc165[] = "**mpi_rsend_init %p %d %D %d %d %C %p";
static const char long_spc165[]  = "MPI_Rsend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc166[] = "**mpi_scan %p %p %d %D %O %C";
static const char long_spc166[]  = "MPI_Scan(sbuf=%p, rbuf=%p, count=%d, %D, %O, %C) failed";
static const char short_spc167[] = "**mpi_scatter %p %d %D %p %d %D %d %C";
static const char long_spc167[]  = "MPI_Scatter(sbuf=%p, scount=%d, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc168[] = "**mpi_scatterv %p %p %p %D %p %d %D %d %C";
static const char long_spc168[]  = "MPI_Scatterv(sbuf=%p, scnts=%p, displs=%p, %D, rbuf=%p, rcount=%d, %D, root=%d, %C) failed";
static const char short_spc169[] = "**mpi_send %p %d %D %d %d %C";
static const char long_spc169[]  = "MPI_Send(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc170[] = "**mpi_send_init %p %d %D %d %d %C %p";
static const char long_spc170[]  = "MPI_Send_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc171[] = "**mpi_sendrecv %p %d %D %d %d %p %d %D %d %d %C %p";
static const char long_spc171[]  = "MPI_Sendrecv(sbuf=%p, scount=%d, %D, dest=%d, stag=%d, rbuf=%p, rcount=%d, %D, src=%d, rtag=%d, %C, status=%p) failed";
static const char short_spc172[] = "**mpi_sendrecv_replace %p %d %D %d %d %d %d %C %p";
static const char long_spc172[]  = "MPI_Sendrecv_replace(buf=%p, count=%d, %D, dest=%d, stag=%d, src=%d, rtag=%d, %C, status=%p) failed";
static const char short_spc173[] = "**mpi_ssend %p %d %D %d %d %C";
static const char long_spc173[]  = "MPI_Ssend(buf=%p, count=%d, %D, dest=%d, tag=%d, %C) failed";
static const char short_spc174[] = "**mpi_ssend_init %p %d %D %d %d %C %p";
static const char long_spc174[]  = "MPI_Ssend_init(buf=%p, count=%d, %D, dest=%d, tag=%d, %C, request=%p) failed";
static const char short_spc175[] = "**mpi_start %p";
static const char long_spc175[]  = "MPI_Start(request=%p) failed";
static const char short_spc176[] = "**mpi_startall %d %p";
static const char long_spc176[]  = "MPI_Startall(count=%d, req_array=%p) failed";
static const char short_spc177[] = "**mpi_test %p %p %p";
static const char long_spc177[]  = "MPI_Test(request=%p, flag=%p, status=%p) failed";
static const char short_spc178[] = "**mpi_testall %d %p %p %p";
static const char long_spc178[]  = "MPI_Testall(count=%d, req_array=%p, flag=%p, status_array=%p) failed";
static const char short_spc179[] = "**mpi_testany %d %p %p %p %p";
static const char long_spc179[]  = "MPI_Testany(count=%d, req_array=%p, index=%p, flag=%p, status=%p) failed";
static const char short_spc180[] = "**mpi_testsome %d %p %p %p %p";
static const char long_spc180[]  = "MPI_Testsome(count=%d, req_array=%p, out_count=%p, indices=%p, status_array=%p) failed";
static const char short_spc181[] = "**mpi_type_commit %p";
static const char long_spc181[]  = "MPI_Type_commit(%p) failed";
static const char short_spc182[] = "**mpi_type_contiguous %d %D %p";
static const char long_spc182[]  = "MPI_Type_contiguous(%d, %D, %p) failed";
static const char short_spc183[] = "**mpi_type_create_hindexed %d %p %p %D %p";
static const char long_spc183[]  = "MPI_Type_create_hindexed(%d, %p, %p, %D, %p) failed";
static const char short_spc184[] = "**mpi_type_create_hvector %d %d %d %D %p";
static const char long_spc184[]  = "MPI_Type_create_hvector(%d, %d, %d, %D, %p) failed";
static const char short_spc185[] = "**mpi_type_create_indexed_block %d %d %p %D %p";
static const char long_spc185[]  = "MPI_Type_create_indexed_block(%d, %d, %p, %D, %p) failed";
static const char short_spc186[] = "**mpi_type_create_keyval %p %p %p %p";
static const char long_spc186[]  = "MPI_Type_create_keyval(%p, %p, %p, %p) failed";
static const char short_spc187[] = "**mpi_type_create_resized %D %d %d %p";
static const char long_spc187[]  = "MPI_Type_create_resized(%D, %d, %d, %p) failed";
static const char short_spc188[] = "**mpi_type_create_struct %d %p %p %p %p";
static const char long_spc188[]  = "MPI_Type_create_struct(%d, %p, %p, %p, %p) failed";
static const char short_spc189[] = "**mpi_type_delete_attr %D %d";
static const char long_spc189[]  = "MPI_Type_delete_atrr(%D, %d) failed";
static const char short_spc190[] = "**mpi_type_dup %D %p";
static const char long_spc190[]  = "MPI_Type_dup(%D, %p) failed";
static const char short_spc191[] = "**mpi_type_get_contents %D %d %d %d %p %p %p";
static const char long_spc191[]  = "MPI_Type_get_contents(%D, %d, %d, %d, %p, %p, %p) failed";
static const char short_spc192[] = "**mpi_type_get_envelope %D %p %p %p %p";
static const char long_spc192[]  = "MPI_Type_get_envelope(%D, %p, %p, %p, %p) failed";
static const char short_spc193[] = "**mpi_type_hindexed %d %p %p %D %p";
static const char long_spc193[]  = "MPI_Type_hindexed(%d, %p, %p, %D, %p) failed";
static const char short_spc194[] = "**mpi_type_hvector %d %d %d %D %p";
static const char long_spc194[]  = "MPI_Type_hvector(%d, %d, %d, %D, %p) failed";
static const char short_spc195[] = "**mpi_type_indexed %d %p %p %D %p";
static const char long_spc195[]  = "MPI_Type_indexed(%d, %p, %p, %D, %p) failed";
static const char short_spc196[] = "**mpi_type_match_size %d %d %p";
static const char long_spc196[]  = "MPI_Type_match_size(%d, %d, %p) failed";
static const char short_spc197[] = "**mpi_type_set_attr %D %d %p";
static const char long_spc197[]  = "MPI_Type_set_attr(%D, %d, %p) failed";
static const char short_spc198[] = "**mpi_type_struct %d %p %p %p %p";
static const char long_spc198[]  = "MPI_Type_struct(%d, %p, %p, %p, %p) failed";
static const char short_spc199[] = "**mpi_type_vector %d %d %d %D %p";
static const char long_spc199[]  = "MPI_Type_vector(%d, %d, %d, %D, %p) failed";
static const char short_spc200[] = "**mpi_unpack_external %s %p %d %p %p %d %D";
static const char long_spc200[]  = "MPI_Unpack_external(%s, %p, %d, %p, %p, %d, %D) failed";
static const char short_spc201[] = "**mpi_unpublish_name %s %I %s";
static const char long_spc201[]  = "MPI_Unpublish_name(service=\"%s\", %I, port=\"%s\") failed";
static const char short_spc202[] = "**mpi_wait %p %p";
static const char long_spc202[]  = "MPI_Wait(request=%p, status%p) failed";
static const char short_spc203[] = "**mpi_waitall %d %p %p";
static const char long_spc203[]  = "MPI_Waitall(count=%d, req_array=%p, status_array=%p) failed";
static const char short_spc204[] = "**mpi_waitany %d %p %p %p";
static const char long_spc204[]  = "MPI_Waitany(count=%d, req_array=%p, index=%p, status=%p) failed";
static const char short_spc205[] = "**mpi_waitsome %d %p %p %p %p";
static const char long_spc205[]  = "MPI_Waitsome(count=%d, req_array=%p, out_count=%p, indices=%p, status_array=%p) failed";
static const char short_spc206[] = "**mpi_win_complete %W";
static const char long_spc206[]  = "MPI_Win_complete(%W) failed";
static const char short_spc207[] = "**mpi_win_create %p %d %d %I %C %p";
static const char long_spc207[]  = "MPI_Win_create(%p, %d, %d, %I, %C, %p) failed";
static const char short_spc208[] = "**mpi_win_create_errhandler %p %p";
static const char long_spc208[]  = "MPI_Win_create_errhandler(%p, %p) failed";
static const char short_spc209[] = "**mpi_win_create_keyval %p %p %p %p";
static const char long_spc209[]  = "MPI_Win_create_keyval(%p, %p, %p, %p) failed";
static const char short_spc210[] = "**mpi_win_delete_attr %W %d";
static const char long_spc210[]  = "MPI_Win_delete_attr(%W, %d) failed";
static const char short_spc211[] = "**mpi_win_fence %A %W";
static const char long_spc211[]  = "MPI_Win_fence(%A, %W) failed";
static const char short_spc212[] = "**mpi_win_free %p";
static const char long_spc212[]  = "MPI_Win_free(%p) failed";
static const char short_spc213[] = "**mpi_win_get_group %W %p";
static const char long_spc213[]  = "MPI_Win_get_group(%W, %p) failed";
static const char short_spc214[] = "**mpi_win_lock %d %d %d %W";
static const char long_spc214[]  = "MPI_Win_lock(%d, %d, %d, %W) failed";
static const char short_spc215[] = "**mpi_win_post %G %d %W";
static const char long_spc215[]  = "MPI_Win_post(%G, %d, %W) failed";
static const char short_spc216[] = "**mpi_win_set_attr %W %d %p";
static const char long_spc216[]  = "MPI_Win_set_attr(%W, %d, %p) failed";
static const char short_spc217[] = "**mpi_win_test %W %p";
static const char long_spc217[]  = "MPI_Win_test(%W, %p) failed";
static const char short_spc218[] = "**mpi_win_unlock %d %W";
static const char long_spc218[]  = "MPI_Win_unlock(%d, %W) failed";
static const char short_spc219[] = "**mpi_win_wait %W";
static const char long_spc219[]  = "MPI_Win_wait(%W) failed";
static const char short_spc220[] = "**mq_close %d";
static const char long_spc220[]  = "failed to close a posix message queue, error %d";
static const char short_spc221[] = "**mq_open %d";
static const char long_spc221[]  = "failed to open a posix message queue, error %d";
static const char short_spc222[] = "**mq_receive %d";
static const char long_spc222[]  = "failed to receive a posix message queue message, error %d";
static const char short_spc223[] = "**mq_send %d";
static const char long_spc223[]  = "failed to send a posix message queue message, error %d";
static const char short_spc224[] = "**msgctl %d";
static const char long_spc224[]  = "msgctl returned %d";
static const char short_spc225[] = "**msgget %d";
static const char long_spc225[]  = "msgget returned %d";
static const char short_spc226[] = "**msgrcv %d";
static const char long_spc226[]  = "msgrcv returned %d";
static const char short_spc227[] = "**msgsnd %d";
static const char long_spc227[]  = "msgsnd returned %d";
static const char short_spc228[] = "**namepubfile %s %s %s";
static const char long_spc228[]  = "Unable to publish service name %s using file %s for reason %s";
static const char short_spc229[] = "**namepubnotpub %s";
static const char long_spc229[]  = "Lookup failed for service name %s";
static const char short_spc230[] = "**nomem %d";
static const char long_spc230[]  = "Out of memory (unable to allocate %d bytes)";
static const char short_spc231[] = "**nomem %s %d";
static const char long_spc231[]  = "Out of memory (unable to allocate a '%s' of size %d)";
static const char short_spc232[] = "**nomem %s";
static const char long_spc232[]  = "Out of memory (unable to allocate a '%s')";
static const char short_spc233[] = "**notsame %s %s";
static const char long_spc233[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc234[] = "**nulledge %d %d";
static const char long_spc234[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc235[] = "**nullptr %s";
static const char long_spc235[]  = "Null pointer in parameter %s";
static const char short_spc236[] = "**nullptrtype %s";
static const char long_spc236[]  = "Null %s pointer";
static const char short_spc237[] = "**open %s %d %d";
static const char long_spc237[]  = "open(%s) failed for process %d, error %d";
static const char short_spc238[] = "**opnotpredefined %d";
static const char long_spc238[]  = "only predefined ops are valid (op = %d)";
static const char short_spc239[] = "**opundefined %s";
static const char long_spc239[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc240[] = "**opundefined_rma %d";
static const char long_spc240[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc241[] = "**pd_alloc %s";
static const char long_spc241[]  = "unable to allocate a protection domain - %s";
static const char short_spc242[] = "**pkt_ptr %p %p";
static const char long_spc242[]  = "invalid shm queue packet pointer (%p != %p)";
static const char short_spc243[] = "**pmi_barrier %d";
static const char long_spc243[]  = "PMI_Barrier returned %d";
static const char short_spc244[] = "**pmi_finalize %d";
static const char long_spc244[]  = "PMI_Finalize returned %d";
static const char short_spc245[] = "**pmi_get_id %d";
static const char long_spc245[]  = "PMI_Get_id returned %d";
static const char short_spc246[] = "**pmi_get_id_length_max %d";
static const char long_spc246[]  = "PMI_Get_id_length_max returned %d";
static const char short_spc247[] = "**pmi_get_rank %d";
static const char long_spc247[]  = "PMI_Get_rank returned %d";
static const char short_spc248[] = "**pmi_get_size %d";
static const char long_spc248[]  = "PMI_Get_size returned %d";
static const char short_spc249[] = "**pmi_init %d";
static const char long_spc249[]  = "PMI_Init returned %d";
static const char short_spc250[] = "**pmi_kvs_commit %d";
static const char long_spc250[]  = "PMI_KVS_Commit returned %d";
static const char short_spc251[] = "**pmi_kvs_create %d";
static const char long_spc251[]  = "PMI_KVS_Create returned %d";
static const char short_spc252[] = "**pmi_kvs_get %d";
static const char long_spc252[]  = "PMI_KVS_Get returned %d";
static const char short_spc253[] = "**pmi_kvs_get_key_length_max %d";
static const char long_spc253[]  = "PMI_KVS_Get_key_length_max returned %d";
static const char short_spc254[] = "**pmi_kvs_get_my_name %d";
static const char long_spc254[]  = "PMI_KVS_Get_my_name returned %d";
static const char short_spc255[] = "**pmi_kvs_get_name_length_max %d";
static const char long_spc255[]  = "PMI_KVS_Get_name_length_max returned %d";
static const char short_spc256[] = "**pmi_kvs_get_parent %d";
static const char long_spc256[]  = "unable to get the PARENT_ROOT_PORT_NAME from the keyval space (pmi_error %d)";
static const char short_spc257[] = "**pmi_kvs_get_value_length_max %d";
static const char long_spc257[]  = "PMI_KVS_Get_value_length_max returned %d";
static const char short_spc258[] = "**pmi_kvs_put %d";
static const char long_spc258[]  = "PMI_KVS_Put returned %d";
static const char short_spc259[] = "**pmi_spawn_multiple %d";
static const char long_spc259[]  = "PMI_Spawn_multiple returned %d";
static const char short_spc260[] = "**post_connect %s";
static const char long_spc260[]  = "%s failed in VC_post_connect";
static const char short_spc261[] = "**rangedup %d %d %d";
static const char long_spc261[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc262[] = "**rangeendinvalid %d %d %d";
static const char long_spc262[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc263[] = "**rangestartinvalid %d %d %d";
static const char long_spc263[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc264[] = "**rank %d %d";
static const char long_spc264[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc265[] = "**rankarray %d %d %d";
static const char long_spc265[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc266[] = "**rankdup %d %d %d";
static const char long_spc266[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc267[] = "**ranklocal %d %d";
static const char long_spc267[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc268[] = "**rankremote %d %d";
static const char long_spc268[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc269[] = "**rmasize %d";
static const char long_spc269[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc270[] = "**root %d";
static const char long_spc270[]  = "Invalid root (value given was %d)";
static const char short_spc271[] = "**rsendnomatch %d %d";
static const char long_spc271[]  = "Ready send from source %d and with tag %d had no matching receive";
static const char short_spc272[] = "**shm_op %d";
static const char long_spc272[]  = "invalid shm operation (%d)";
static const char short_spc273[] = "**shm_open %s %d";
static const char long_spc273[]  = "unable to open shared memory object %s (errno %d)";
static const char short_spc274[] = "**shm_unlink %s %d";
static const char long_spc274[]  = "failed to unlink shared memory object %s, error %d";
static const char short_spc275[] = "**shmat %d";
static const char long_spc275[]  = "shmat failed, error %d";
static const char short_spc276[] = "**shmctl %d %d";
static const char long_spc276[]  = "failed to mark the sysv segment %d for removal, error %d";
static const char short_spc277[] = "**shmget %d";
static const char long_spc277[]  = "shmget failed, error %d";
static const char short_spc278[] = "**shmhost %s %s";
static const char long_spc278[]  = "process not on the same host (%s != %s)";
static const char short_spc279[] = "**shmq_index %d %d";
static const char long_spc279[]  = "invalid shm queue index (%d > %d)";
static const char short_spc280[] = "**shmsize %d";
static const char long_spc280[]  = "Shared memory subsystem unable to allocate a block of the requested size, %d bytes";
static const char short_spc281[] = "**snprintf %d";
static const char long_spc281[]  = "snprintf returned %d";
static const char short_spc282[] = "**sock_byname %d";
static const char long_spc282[]  = "gethostbyname failed (errno %d)";
static const char short_spc283[] = "**sock_connect %s %d %d";
static const char long_spc283[]  = "unable to connect to %s on port %d, error %d";
static const char short_spc284[] = "**sock_create %d";
static const char long_spc284[]  = "unable to create a socket (errno %d)";
static const char short_spc285[] = "**sock_gethost %d";
static const char long_spc285[]  = "gethostname failed (errno %d)";
static const char short_spc286[] = "**socket %d";
static const char long_spc286[]  = "WSASocket failed (errno %d)";
static const char short_spc287[] = "**sock|badbuf %d %d";
static const char long_spc287[]  = "the supplied buffer contains invalid memory (set=%d,sock=%d)";
static const char short_spc288[] = "**sock|badiovn %d %d %d";
static const char long_spc288[]  = "size of iov is invalid (set=%d,sock=%d,iov_n=%d)";
static const char short_spc289[] = "**sock|badlen %d %d %d %d";
static const char long_spc289[]  = "bad length parameter(s) (set=%d,sock=%d,min=%d,max=%d)";
static const char short_spc290[] = "**sock|badport %d";
static const char long_spc290[]  = "port number is out of range (sock=%d)";
static const char short_spc291[] = "**sock|closing %d %d";
static const char long_spc291[]  = "sock is in the process of being closed locally (set=%d,sock=%d)";
static const char short_spc292[] = "**sock|connclosed %d %d";
static const char long_spc292[]  = "connection closed by peer (set=%d,sock=%d)";
static const char short_spc293[] = "**sock|connfailed %d %d";
static const char long_spc293[]  = "connection failure (set=%d,sock=%d)";
static const char short_spc294[] = "**sock|notconnected %d %d";
static const char long_spc294[]  = "sock is not connected (set=%d,sock=%d)";
static const char short_spc295[] = "**sock|osnomem %d %d";
static const char long_spc295[]  = "operating system routine failed due to lack of memory (set=%d,sock=%d)";
static const char short_spc296[] = "**sock|poll|accept %d";
static const char long_spc296[]  = "accept failed to acquire a new socket (errno=%d)";
static const char short_spc297[] = "**sock|poll|badhandle %d %d %d";
static const char long_spc297[]  = "sock contains an invalid handle (set=%d,sock=%d,fd=%d)";
static const char short_spc298[] = "**sock|poll|bind %d %d";
static const char long_spc298[]  = "unable to bind socket to port (port=%d,errno=%d)";
static const char short_spc299[] = "**sock|poll|connfailed %d %d %d";
static const char long_spc299[]  = "connection failure (set=%d,sock=%d,errno=%d)";
static const char short_spc300[] = "**sock|poll|connfailed %d";
static const char long_spc300[]  = "connection failure (errno=%d)";
static const char short_spc301[] = "**sock|poll|connrefused %d %d %s";
static const char long_spc301[]  = "connection refused (set=%d,sock=%d,host=%s)";
static const char short_spc302[] = "**sock|poll|eqfail %d %d %d";
static const char long_spc302[]  = "fatal error: failed to enqueue an event; event was lost (set=%d,sock=%d,op=%d)";
static const char short_spc303[] = "**sock|poll|eqfail %d";
static const char long_spc303[]  = "fatal error: failed to enqueue an event; event was lost (op=%d)";
static const char short_spc304[] = "**sock|poll|getport %d";
static const char long_spc304[]  = "failed to obtain port number of listener (errno=%d)";
static const char short_spc305[] = "**sock|poll|hostres %d %d %s";
static const char long_spc305[]  = "unable to resolve host name to an address (set=%d,sock=%d,host=%s)";
static const char short_spc306[] = "**sock|poll|listen %d";
static const char long_spc306[]  = "listen() failed (errno=%d)";
static const char short_spc307[] = "**sock|poll|nodelay %d";
static const char long_spc307[]  = "unable to set TCP no delay attribute on socket (errno=%d)";
static const char short_spc308[] = "**sock|poll|nonblock %d";
static const char long_spc308[]  = "unable to set socket to nonblocking (errno=%d)";
static const char short_spc309[] = "**sock|poll|oserror %d %d %d";
static const char long_spc309[]  = "unknown operating system error (set=%d,sock=%d,errno=%d)";
static const char short_spc310[] = "**sock|poll|oserror %d";
static const char long_spc310[]  = "unknown operating system error (errno=%d)";
static const char short_spc311[] = "**sock|poll|pipe %d";
static const char long_spc311[]  = "unable to allocate pipe to wakeup a blocking poll() (errno=%d)";
static const char short_spc312[] = "**sock|poll|pipenonblock %d";
static const char long_spc312[]  = "unable to set wakeup pipe to nonblocking (errno=%d)";
static const char short_spc313[] = "**sock|poll|reuseaddr %d";
static const char long_spc313[]  = "unable to set reuseaddr attribute on socket (errno=%d)";
static const char short_spc314[] = "**sock|poll|socket %d";
static const char long_spc314[]  = "unable to obtain new socket (errno=%d)";
static const char short_spc315[] = "**sock|poll|unhandledstate %d %s";
static const char long_spc315[]  = "encountered an unhandled state (%d) while processing %s";
static const char short_spc316[] = "**sock|reads %d %d";
static const char long_spc316[]  = "attempt to perform multiple simultaneous reads (set=%d,sock=%d)";
static const char short_spc317[] = "**sock|writes %d %d";
static const char long_spc317[]  = "attempt to perform multiple simultaneous writes (set=%d,sock=%d)";
static const char short_spc318[] = "**stride %d %d %d";
static const char long_spc318[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc319[] = "**tag %d";
static const char long_spc319[]  = "Invalid tag, value is %d";
static const char short_spc320[] = "**topotoolarge %d %d";
static const char long_spc320[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc321[] = "**truncate %d %d %d %d";
static const char long_spc321[]  = "Message from rank %d and tag %d truncated; %d bytes received but buffer size is %d";
static const char short_spc322[] = "**truncate %d %d";
static const char long_spc322[]  = "Message truncated; %d bytes received but buffer size is %d";
static const char short_spc323[] = "**typematchsize %s %d";
static const char long_spc323[]  = "No MPI datatype available for typeclass %s and size %d";
static const char short_spc324[] = "**typenamelen %d";
static const char long_spc324[]  = " Specified datatype name is too long (%d characters)";
static const char short_spc325[] = "**vc_state %d";
static const char long_spc325[]  = "invalid vc state (%d)";
static const char short_spc326[] = "**wsasock %d";
static const char long_spc326[]  = "WSAStartup failed (errno %d)";

static const int specific_msgs_len = 327;
static const msgpair specific_err_msgs[] = {
{ 0xacebad03, short_spc0, long_spc0, 0xcb0bfa11 },
{ 0xacebad03, short_spc1, long_spc1, 0xcb0bfa11 },
{ 0xacebad03, short_spc2, long_spc2, 0xcb0bfa11 },
{ 0xacebad03, short_spc3, long_spc3, 0xcb0bfa11 },
{ 0xacebad03, short_spc4, long_spc4, 0xcb0bfa11 },
{ 0xacebad03, short_spc5, long_spc5, 0xcb0bfa11 },
{ 0xacebad03, short_spc6, long_spc6, 0xcb0bfa11 },
{ 0xacebad03, short_spc7, long_spc7, 0xcb0bfa11 },
{ 0xacebad03, short_spc8, long_spc8, 0xcb0bfa11 },
{ 0xacebad03, short_spc9, long_spc9, 0xcb0bfa11 },
{ 0xacebad03, short_spc10, long_spc10, 0xcb0bfa11 },
{ 0xacebad03, short_spc11, long_spc11, 0xcb0bfa11 },
{ 0xacebad03, short_spc12, long_spc12, 0xcb0bfa11 },
{ 0xacebad03, short_spc13, long_spc13, 0xcb0bfa11 },
{ 0xacebad03, short_spc14, long_spc14, 0xcb0bfa11 },
{ 0xacebad03, short_spc15, long_spc15, 0xcb0bfa11 },
{ 0xacebad03, short_spc16, long_spc16, 0xcb0bfa11 },
{ 0xacebad03, short_spc17, long_spc17, 0xcb0bfa11 },
{ 0xacebad03, short_spc18, long_spc18, 0xcb0bfa11 },
{ 0xacebad03, short_spc19, long_spc19, 0xcb0bfa11 },
{ 0xacebad03, short_spc20, long_spc20, 0xcb0bfa11 },
{ 0xacebad03, short_spc21, long_spc21, 0xcb0bfa11 },
{ 0xacebad03, short_spc22, long_spc22, 0xcb0bfa11 },
{ 0xacebad03, short_spc23, long_spc23, 0xcb0bfa11 },
{ 0xacebad03, short_spc24, long_spc24, 0xcb0bfa11 },
{ 0xacebad03, short_spc25, long_spc25, 0xcb0bfa11 },
{ 0xacebad03, short_spc26, long_spc26, 0xcb0bfa11 },
{ 0xacebad03, short_spc27, long_spc27, 0xcb0bfa11 },
{ 0xacebad03, short_spc28, long_spc28, 0xcb0bfa11 },
{ 0xacebad03, short_spc29, long_spc29, 0xcb0bfa11 },
{ 0xacebad03, short_spc30, long_spc30, 0xcb0bfa11 },
{ 0xacebad03, short_spc31, long_spc31, 0xcb0bfa11 },
{ 0xacebad03, short_spc32, long_spc32, 0xcb0bfa11 },
{ 0xacebad03, short_spc33, long_spc33, 0xcb0bfa11 },
{ 0xacebad03, short_spc34, long_spc34, 0xcb0bfa11 },
{ 0xacebad03, short_spc35, long_spc35, 0xcb0bfa11 },
{ 0xacebad03, short_spc36, long_spc36, 0xcb0bfa11 },
{ 0xacebad03, short_spc37, long_spc37, 0xcb0bfa11 },
{ 0xacebad03, short_spc38, long_spc38, 0xcb0bfa11 },
{ 0xacebad03, short_spc39, long_spc39, 0xcb0bfa11 },
{ 0xacebad03, short_spc40, long_spc40, 0xcb0bfa11 },
{ 0xacebad03, short_spc41, long_spc41, 0xcb0bfa11 },
{ 0xacebad03, short_spc42, long_spc42, 0xcb0bfa11 },
{ 0xacebad03, short_spc43, long_spc43, 0xcb0bfa11 },
{ 0xacebad03, short_spc44, long_spc44, 0xcb0bfa11 },
{ 0xacebad03, short_spc45, long_spc45, 0xcb0bfa11 },
{ 0xacebad03, short_spc46, long_spc46, 0xcb0bfa11 },
{ 0xacebad03, short_spc47, long_spc47, 0xcb0bfa11 },
{ 0xacebad03, short_spc48, long_spc48, 0xcb0bfa11 },
{ 0xacebad03, short_spc49, long_spc49, 0xcb0bfa11 },
{ 0xacebad03, short_spc50, long_spc50, 0xcb0bfa11 },
{ 0xacebad03, short_spc51, long_spc51, 0xcb0bfa11 },
{ 0xacebad03, short_spc52, long_spc52, 0xcb0bfa11 },
{ 0xacebad03, short_spc53, long_spc53, 0xcb0bfa11 },
{ 0xacebad03, short_spc54, long_spc54, 0xcb0bfa11 },
{ 0xacebad03, short_spc55, long_spc55, 0xcb0bfa11 },
{ 0xacebad03, short_spc56, long_spc56, 0xcb0bfa11 },
{ 0xacebad03, short_spc57, long_spc57, 0xcb0bfa11 },
{ 0xacebad03, short_spc58, long_spc58, 0xcb0bfa11 },
{ 0xacebad03, short_spc59, long_spc59, 0xcb0bfa11 },
{ 0xacebad03, short_spc60, long_spc60, 0xcb0bfa11 },
{ 0xacebad03, short_spc61, long_spc61, 0xcb0bfa11 },
{ 0xacebad03, short_spc62, long_spc62, 0xcb0bfa11 },
{ 0xacebad03, short_spc63, long_spc63, 0xcb0bfa11 },
{ 0xacebad03, short_spc64, long_spc64, 0xcb0bfa11 },
{ 0xacebad03, short_spc65, long_spc65, 0xcb0bfa11 },
{ 0xacebad03, short_spc66, long_spc66, 0xcb0bfa11 },
{ 0xacebad03, short_spc67, long_spc67, 0xcb0bfa11 },
{ 0xacebad03, short_spc68, long_spc68, 0xcb0bfa11 },
{ 0xacebad03, short_spc69, long_spc69, 0xcb0bfa11 },
{ 0xacebad03, short_spc70, long_spc70, 0xcb0bfa11 },
{ 0xacebad03, short_spc71, long_spc71, 0xcb0bfa11 },
{ 0xacebad03, short_spc72, long_spc72, 0xcb0bfa11 },
{ 0xacebad03, short_spc73, long_spc73, 0xcb0bfa11 },
{ 0xacebad03, short_spc74, long_spc74, 0xcb0bfa11 },
{ 0xacebad03, short_spc75, long_spc75, 0xcb0bfa11 },
{ 0xacebad03, short_spc76, long_spc76, 0xcb0bfa11 },
{ 0xacebad03, short_spc77, long_spc77, 0xcb0bfa11 },
{ 0xacebad03, short_spc78, long_spc78, 0xcb0bfa11 },
{ 0xacebad03, short_spc79, long_spc79, 0xcb0bfa11 },
{ 0xacebad03, short_spc80, long_spc80, 0xcb0bfa11 },
{ 0xacebad03, short_spc81, long_spc81, 0xcb0bfa11 },
{ 0xacebad03, short_spc82, long_spc82, 0xcb0bfa11 },
{ 0xacebad03, short_spc83, long_spc83, 0xcb0bfa11 },
{ 0xacebad03, short_spc84, long_spc84, 0xcb0bfa11 },
{ 0xacebad03, short_spc85, long_spc85, 0xcb0bfa11 },
{ 0xacebad03, short_spc86, long_spc86, 0xcb0bfa11 },
{ 0xacebad03, short_spc87, long_spc87, 0xcb0bfa11 },
{ 0xacebad03, short_spc88, long_spc88, 0xcb0bfa11 },
{ 0xacebad03, short_spc89, long_spc89, 0xcb0bfa11 },
{ 0xacebad03, short_spc90, long_spc90, 0xcb0bfa11 },
{ 0xacebad03, short_spc91, long_spc91, 0xcb0bfa11 },
{ 0xacebad03, short_spc92, long_spc92, 0xcb0bfa11 },
{ 0xacebad03, short_spc93, long_spc93, 0xcb0bfa11 },
{ 0xacebad03, short_spc94, long_spc94, 0xcb0bfa11 },
{ 0xacebad03, short_spc95, long_spc95, 0xcb0bfa11 },
{ 0xacebad03, short_spc96, long_spc96, 0xcb0bfa11 },
{ 0xacebad03, short_spc97, long_spc97, 0xcb0bfa11 },
{ 0xacebad03, short_spc98, long_spc98, 0xcb0bfa11 },
{ 0xacebad03, short_spc99, long_spc99, 0xcb0bfa11 },
{ 0xacebad03, short_spc100, long_spc100, 0xcb0bfa11 },
{ 0xacebad03, short_spc101, long_spc101, 0xcb0bfa11 },
{ 0xacebad03, short_spc102, long_spc102, 0xcb0bfa11 },
{ 0xacebad03, short_spc103, long_spc103, 0xcb0bfa11 },
{ 0xacebad03, short_spc104, long_spc104, 0xcb0bfa11 },
{ 0xacebad03, short_spc105, long_spc105, 0xcb0bfa11 },
{ 0xacebad03, short_spc106, long_spc106, 0xcb0bfa11 },
{ 0xacebad03, short_spc107, long_spc107, 0xcb0bfa11 },
{ 0xacebad03, short_spc108, long_spc108, 0xcb0bfa11 },
{ 0xacebad03, short_spc109, long_spc109, 0xcb0bfa11 },
{ 0xacebad03, short_spc110, long_spc110, 0xcb0bfa11 },
{ 0xacebad03, short_spc111, long_spc111, 0xcb0bfa11 },
{ 0xacebad03, short_spc112, long_spc112, 0xcb0bfa11 },
{ 0xacebad03, short_spc113, long_spc113, 0xcb0bfa11 },
{ 0xacebad03, short_spc114, long_spc114, 0xcb0bfa11 },
{ 0xacebad03, short_spc115, long_spc115, 0xcb0bfa11 },
{ 0xacebad03, short_spc116, long_spc116, 0xcb0bfa11 },
{ 0xacebad03, short_spc117, long_spc117, 0xcb0bfa11 },
{ 0xacebad03, short_spc118, long_spc118, 0xcb0bfa11 },
{ 0xacebad03, short_spc119, long_spc119, 0xcb0bfa11 },
{ 0xacebad03, short_spc120, long_spc120, 0xcb0bfa11 },
{ 0xacebad03, short_spc121, long_spc121, 0xcb0bfa11 },
{ 0xacebad03, short_spc122, long_spc122, 0xcb0bfa11 },
{ 0xacebad03, short_spc123, long_spc123, 0xcb0bfa11 },
{ 0xacebad03, short_spc124, long_spc124, 0xcb0bfa11 },
{ 0xacebad03, short_spc125, long_spc125, 0xcb0bfa11 },
{ 0xacebad03, short_spc126, long_spc126, 0xcb0bfa11 },
{ 0xacebad03, short_spc127, long_spc127, 0xcb0bfa11 },
{ 0xacebad03, short_spc128, long_spc128, 0xcb0bfa11 },
{ 0xacebad03, short_spc129, long_spc129, 0xcb0bfa11 },
{ 0xacebad03, short_spc130, long_spc130, 0xcb0bfa11 },
{ 0xacebad03, short_spc131, long_spc131, 0xcb0bfa11 },
{ 0xacebad03, short_spc132, long_spc132, 0xcb0bfa11 },
{ 0xacebad03, short_spc133, long_spc133, 0xcb0bfa11 },
{ 0xacebad03, short_spc134, long_spc134, 0xcb0bfa11 },
{ 0xacebad03, short_spc135, long_spc135, 0xcb0bfa11 },
{ 0xacebad03, short_spc136, long_spc136, 0xcb0bfa11 },
{ 0xacebad03, short_spc137, long_spc137, 0xcb0bfa11 },
{ 0xacebad03, short_spc138, long_spc138, 0xcb0bfa11 },
{ 0xacebad03, short_spc139, long_spc139, 0xcb0bfa11 },
{ 0xacebad03, short_spc140, long_spc140, 0xcb0bfa11 },
{ 0xacebad03, short_spc141, long_spc141, 0xcb0bfa11 },
{ 0xacebad03, short_spc142, long_spc142, 0xcb0bfa11 },
{ 0xacebad03, short_spc143, long_spc143, 0xcb0bfa11 },
{ 0xacebad03, short_spc144, long_spc144, 0xcb0bfa11 },
{ 0xacebad03, short_spc145, long_spc145, 0xcb0bfa11 },
{ 0xacebad03, short_spc146, long_spc146, 0xcb0bfa11 },
{ 0xacebad03, short_spc147, long_spc147, 0xcb0bfa11 },
{ 0xacebad03, short_spc148, long_spc148, 0xcb0bfa11 },
{ 0xacebad03, short_spc149, long_spc149, 0xcb0bfa11 },
{ 0xacebad03, short_spc150, long_spc150, 0xcb0bfa11 },
{ 0xacebad03, short_spc151, long_spc151, 0xcb0bfa11 },
{ 0xacebad03, short_spc152, long_spc152, 0xcb0bfa11 },
{ 0xacebad03, short_spc153, long_spc153, 0xcb0bfa11 },
{ 0xacebad03, short_spc154, long_spc154, 0xcb0bfa11 },
{ 0xacebad03, short_spc155, long_spc155, 0xcb0bfa11 },
{ 0xacebad03, short_spc156, long_spc156, 0xcb0bfa11 },
{ 0xacebad03, short_spc157, long_spc157, 0xcb0bfa11 },
{ 0xacebad03, short_spc158, long_spc158, 0xcb0bfa11 },
{ 0xacebad03, short_spc159, long_spc159, 0xcb0bfa11 },
{ 0xacebad03, short_spc160, long_spc160, 0xcb0bfa11 },
{ 0xacebad03, short_spc161, long_spc161, 0xcb0bfa11 },
{ 0xacebad03, short_spc162, long_spc162, 0xcb0bfa11 },
{ 0xacebad03, short_spc163, long_spc163, 0xcb0bfa11 },
{ 0xacebad03, short_spc164, long_spc164, 0xcb0bfa11 },
{ 0xacebad03, short_spc165, long_spc165, 0xcb0bfa11 },
{ 0xacebad03, short_spc166, long_spc166, 0xcb0bfa11 },
{ 0xacebad03, short_spc167, long_spc167, 0xcb0bfa11 },
{ 0xacebad03, short_spc168, long_spc168, 0xcb0bfa11 },
{ 0xacebad03, short_spc169, long_spc169, 0xcb0bfa11 },
{ 0xacebad03, short_spc170, long_spc170, 0xcb0bfa11 },
{ 0xacebad03, short_spc171, long_spc171, 0xcb0bfa11 },
{ 0xacebad03, short_spc172, long_spc172, 0xcb0bfa11 },
{ 0xacebad03, short_spc173, long_spc173, 0xcb0bfa11 },
{ 0xacebad03, short_spc174, long_spc174, 0xcb0bfa11 },
{ 0xacebad03, short_spc175, long_spc175, 0xcb0bfa11 },
{ 0xacebad03, short_spc176, long_spc176, 0xcb0bfa11 },
{ 0xacebad03, short_spc177, long_spc177, 0xcb0bfa11 },
{ 0xacebad03, short_spc178, long_spc178, 0xcb0bfa11 },
{ 0xacebad03, short_spc179, long_spc179, 0xcb0bfa11 },
{ 0xacebad03, short_spc180, long_spc180, 0xcb0bfa11 },
{ 0xacebad03, short_spc181, long_spc181, 0xcb0bfa11 },
{ 0xacebad03, short_spc182, long_spc182, 0xcb0bfa11 },
{ 0xacebad03, short_spc183, long_spc183, 0xcb0bfa11 },
{ 0xacebad03, short_spc184, long_spc184, 0xcb0bfa11 },
{ 0xacebad03, short_spc185, long_spc185, 0xcb0bfa11 },
{ 0xacebad03, short_spc186, long_spc186, 0xcb0bfa11 },
{ 0xacebad03, short_spc187, long_spc187, 0xcb0bfa11 },
{ 0xacebad03, short_spc188, long_spc188, 0xcb0bfa11 },
{ 0xacebad03, short_spc189, long_spc189, 0xcb0bfa11 },
{ 0xacebad03, short_spc190, long_spc190, 0xcb0bfa11 },
{ 0xacebad03, short_spc191, long_spc191, 0xcb0bfa11 },
{ 0xacebad03, short_spc192, long_spc192, 0xcb0bfa11 },
{ 0xacebad03, short_spc193, long_spc193, 0xcb0bfa11 },
{ 0xacebad03, short_spc194, long_spc194, 0xcb0bfa11 },
{ 0xacebad03, short_spc195, long_spc195, 0xcb0bfa11 },
{ 0xacebad03, short_spc196, long_spc196, 0xcb0bfa11 },
{ 0xacebad03, short_spc197, long_spc197, 0xcb0bfa11 },
{ 0xacebad03, short_spc198, long_spc198, 0xcb0bfa11 },
{ 0xacebad03, short_spc199, long_spc199, 0xcb0bfa11 },
{ 0xacebad03, short_spc200, long_spc200, 0xcb0bfa11 },
{ 0xacebad03, short_spc201, long_spc201, 0xcb0bfa11 },
{ 0xacebad03, short_spc202, long_spc202, 0xcb0bfa11 },
{ 0xacebad03, short_spc203, long_spc203, 0xcb0bfa11 },
{ 0xacebad03, short_spc204, long_spc204, 0xcb0bfa11 },
{ 0xacebad03, short_spc205, long_spc205, 0xcb0bfa11 },
{ 0xacebad03, short_spc206, long_spc206, 0xcb0bfa11 },
{ 0xacebad03, short_spc207, long_spc207, 0xcb0bfa11 },
{ 0xacebad03, short_spc208, long_spc208, 0xcb0bfa11 },
{ 0xacebad03, short_spc209, long_spc209, 0xcb0bfa11 },
{ 0xacebad03, short_spc210, long_spc210, 0xcb0bfa11 },
{ 0xacebad03, short_spc211, long_spc211, 0xcb0bfa11 },
{ 0xacebad03, short_spc212, long_spc212, 0xcb0bfa11 },
{ 0xacebad03, short_spc213, long_spc213, 0xcb0bfa11 },
{ 0xacebad03, short_spc214, long_spc214, 0xcb0bfa11 },
{ 0xacebad03, short_spc215, long_spc215, 0xcb0bfa11 },
{ 0xacebad03, short_spc216, long_spc216, 0xcb0bfa11 },
{ 0xacebad03, short_spc217, long_spc217, 0xcb0bfa11 },
{ 0xacebad03, short_spc218, long_spc218, 0xcb0bfa11 },
{ 0xacebad03, short_spc219, long_spc219, 0xcb0bfa11 },
{ 0xacebad03, short_spc220, long_spc220, 0xcb0bfa11 },
{ 0xacebad03, short_spc221, long_spc221, 0xcb0bfa11 },
{ 0xacebad03, short_spc222, long_spc222, 0xcb0bfa11 },
{ 0xacebad03, short_spc223, long_spc223, 0xcb0bfa11 },
{ 0xacebad03, short_spc224, long_spc224, 0xcb0bfa11 },
{ 0xacebad03, short_spc225, long_spc225, 0xcb0bfa11 },
{ 0xacebad03, short_spc226, long_spc226, 0xcb0bfa11 },
{ 0xacebad03, short_spc227, long_spc227, 0xcb0bfa11 },
{ 0xacebad03, short_spc228, long_spc228, 0xcb0bfa11 },
{ 0xacebad03, short_spc229, long_spc229, 0xcb0bfa11 },
{ 0xacebad03, short_spc230, long_spc230, 0xcb0bfa11 },
{ 0xacebad03, short_spc231, long_spc231, 0xcb0bfa11 },
{ 0xacebad03, short_spc232, long_spc232, 0xcb0bfa11 },
{ 0xacebad03, short_spc233, long_spc233, 0xcb0bfa11 },
{ 0xacebad03, short_spc234, long_spc234, 0xcb0bfa11 },
{ 0xacebad03, short_spc235, long_spc235, 0xcb0bfa11 },
{ 0xacebad03, short_spc236, long_spc236, 0xcb0bfa11 },
{ 0xacebad03, short_spc237, long_spc237, 0xcb0bfa11 },
{ 0xacebad03, short_spc238, long_spc238, 0xcb0bfa11 },
{ 0xacebad03, short_spc239, long_spc239, 0xcb0bfa11 },
{ 0xacebad03, short_spc240, long_spc240, 0xcb0bfa11 },
{ 0xacebad03, short_spc241, long_spc241, 0xcb0bfa11 },
{ 0xacebad03, short_spc242, long_spc242, 0xcb0bfa11 },
{ 0xacebad03, short_spc243, long_spc243, 0xcb0bfa11 },
{ 0xacebad03, short_spc244, long_spc244, 0xcb0bfa11 },
{ 0xacebad03, short_spc245, long_spc245, 0xcb0bfa11 },
{ 0xacebad03, short_spc246, long_spc246, 0xcb0bfa11 },
{ 0xacebad03, short_spc247, long_spc247, 0xcb0bfa11 },
{ 0xacebad03, short_spc248, long_spc248, 0xcb0bfa11 },
{ 0xacebad03, short_spc249, long_spc249, 0xcb0bfa11 },
{ 0xacebad03, short_spc250, long_spc250, 0xcb0bfa11 },
{ 0xacebad03, short_spc251, long_spc251, 0xcb0bfa11 },
{ 0xacebad03, short_spc252, long_spc252, 0xcb0bfa11 },
{ 0xacebad03, short_spc253, long_spc253, 0xcb0bfa11 },
{ 0xacebad03, short_spc254, long_spc254, 0xcb0bfa11 },
{ 0xacebad03, short_spc255, long_spc255, 0xcb0bfa11 },
{ 0xacebad03, short_spc256, long_spc256, 0xcb0bfa11 },
{ 0xacebad03, short_spc257, long_spc257, 0xcb0bfa11 },
{ 0xacebad03, short_spc258, long_spc258, 0xcb0bfa11 },
{ 0xacebad03, short_spc259, long_spc259, 0xcb0bfa11 },
{ 0xacebad03, short_spc260, long_spc260, 0xcb0bfa11 },
{ 0xacebad03, short_spc261, long_spc261, 0xcb0bfa11 },
{ 0xacebad03, short_spc262, long_spc262, 0xcb0bfa11 },
{ 0xacebad03, short_spc263, long_spc263, 0xcb0bfa11 },
{ 0xacebad03, short_spc264, long_spc264, 0xcb0bfa11 },
{ 0xacebad03, short_spc265, long_spc265, 0xcb0bfa11 },
{ 0xacebad03, short_spc266, long_spc266, 0xcb0bfa11 },
{ 0xacebad03, short_spc267, long_spc267, 0xcb0bfa11 },
{ 0xacebad03, short_spc268, long_spc268, 0xcb0bfa11 },
{ 0xacebad03, short_spc269, long_spc269, 0xcb0bfa11 },
{ 0xacebad03, short_spc270, long_spc270, 0xcb0bfa11 },
{ 0xacebad03, short_spc271, long_spc271, 0xcb0bfa11 },
{ 0xacebad03, short_spc272, long_spc272, 0xcb0bfa11 },
{ 0xacebad03, short_spc273, long_spc273, 0xcb0bfa11 },
{ 0xacebad03, short_spc274, long_spc274, 0xcb0bfa11 },
{ 0xacebad03, short_spc275, long_spc275, 0xcb0bfa11 },
{ 0xacebad03, short_spc276, long_spc276, 0xcb0bfa11 },
{ 0xacebad03, short_spc277, long_spc277, 0xcb0bfa11 },
{ 0xacebad03, short_spc278, long_spc278, 0xcb0bfa11 },
{ 0xacebad03, short_spc279, long_spc279, 0xcb0bfa11 },
{ 0xacebad03, short_spc280, long_spc280, 0xcb0bfa11 },
{ 0xacebad03, short_spc281, long_spc281, 0xcb0bfa11 },
{ 0xacebad03, short_spc282, long_spc282, 0xcb0bfa11 },
{ 0xacebad03, short_spc283, long_spc283, 0xcb0bfa11 },
{ 0xacebad03, short_spc284, long_spc284, 0xcb0bfa11 },
{ 0xacebad03, short_spc285, long_spc285, 0xcb0bfa11 },
{ 0xacebad03, short_spc286, long_spc286, 0xcb0bfa11 },
{ 0xacebad03, short_spc287, long_spc287, 0xcb0bfa11 },
{ 0xacebad03, short_spc288, long_spc288, 0xcb0bfa11 },
{ 0xacebad03, short_spc289, long_spc289, 0xcb0bfa11 },
{ 0xacebad03, short_spc290, long_spc290, 0xcb0bfa11 },
{ 0xacebad03, short_spc291, long_spc291, 0xcb0bfa11 },
{ 0xacebad03, short_spc292, long_spc292, 0xcb0bfa11 },
{ 0xacebad03, short_spc293, long_spc293, 0xcb0bfa11 },
{ 0xacebad03, short_spc294, long_spc294, 0xcb0bfa11 },
{ 0xacebad03, short_spc295, long_spc295, 0xcb0bfa11 },
{ 0xacebad03, short_spc296, long_spc296, 0xcb0bfa11 },
{ 0xacebad03, short_spc297, long_spc297, 0xcb0bfa11 },
{ 0xacebad03, short_spc298, long_spc298, 0xcb0bfa11 },
{ 0xacebad03, short_spc299, long_spc299, 0xcb0bfa11 },
{ 0xacebad03, short_spc300, long_spc300, 0xcb0bfa11 },
{ 0xacebad03, short_spc301, long_spc301, 0xcb0bfa11 },
{ 0xacebad03, short_spc302, long_spc302, 0xcb0bfa11 },
{ 0xacebad03, short_spc303, long_spc303, 0xcb0bfa11 },
{ 0xacebad03, short_spc304, long_spc304, 0xcb0bfa11 },
{ 0xacebad03, short_spc305, long_spc305, 0xcb0bfa11 },
{ 0xacebad03, short_spc306, long_spc306, 0xcb0bfa11 },
{ 0xacebad03, short_spc307, long_spc307, 0xcb0bfa11 },
{ 0xacebad03, short_spc308, long_spc308, 0xcb0bfa11 },
{ 0xacebad03, short_spc309, long_spc309, 0xcb0bfa11 },
{ 0xacebad03, short_spc310, long_spc310, 0xcb0bfa11 },
{ 0xacebad03, short_spc311, long_spc311, 0xcb0bfa11 },
{ 0xacebad03, short_spc312, long_spc312, 0xcb0bfa11 },
{ 0xacebad03, short_spc313, long_spc313, 0xcb0bfa11 },
{ 0xacebad03, short_spc314, long_spc314, 0xcb0bfa11 },
{ 0xacebad03, short_spc315, long_spc315, 0xcb0bfa11 },
{ 0xacebad03, short_spc316, long_spc316, 0xcb0bfa11 },
{ 0xacebad03, short_spc317, long_spc317, 0xcb0bfa11 },
{ 0xacebad03, short_spc318, long_spc318, 0xcb0bfa11 },
{ 0xacebad03, short_spc319, long_spc319, 0xcb0bfa11 },
{ 0xacebad03, short_spc320, long_spc320, 0xcb0bfa11 },
{ 0xacebad03, short_spc321, long_spc321, 0xcb0bfa11 },
{ 0xacebad03, short_spc322, long_spc322, 0xcb0bfa11 },
{ 0xacebad03, short_spc323, long_spc323, 0xcb0bfa11 },
{ 0xacebad03, short_spc324, long_spc324, 0xcb0bfa11 },
{ 0xacebad03, short_spc325, long_spc325, 0xcb0bfa11 },
{ 0xacebad03, short_spc326, long_spc326, 0xcb0bfa11 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
541,42,110,123,542,101,454,470,161,411,
545,120,10,551,547,416,194,193,192,463,
135,136,142,109,117,140,141,134,171,172,
178,177,199,390,9,406,146,145,440,148,
149,473,535,118,147,554,27,233,227,466,
469,468,467,20};
#endif
