/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This file automatically created by extracterrmsgs
 * DO NOT EDIT
 */
typedef struct msgpair {
        const char *short_name, *long_name; } msgpair;
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
static const char short_gen47[] = "**ca";
static const char long_gen47[]  = "invalid completion action";
static const char short_gen48[] = "**ca_guids";
static const char long_gen48[]  = "unable to get the guids from the infiniband access layer";
static const char short_gen49[] = "**cancelunknown";
static const char long_gen49[]  = "Attempt to cancel an unknown type of request";
static const char short_gen50[] = "**cartcoordinvalid";
static const char long_gen50[]  = "Cartesian coordinate is invalid (not in range)";
static const char short_gen51[] = "**cartdim";
static const char long_gen51[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short_gen52[] = "**ch3_finalize";
static const char long_gen52[]  = "Channel finalization failed";
static const char short_gen53[] = "**ch3_init";
static const char long_gen53[]  = "Channel init failed";
static const char short_gen54[] = "**ch3_send";
static const char long_gen54[]  = "send failed";
static const char short_gen55[] = "**ch3ireadaggressive";
static const char long_gen55[]  = "aggressive reading failed";
static const char short_gen56[] = "**ch3progress";
static const char long_gen56[]  = "Unable to make message passing progress";
static const char short_gen57[] = "**ch3|badca";
static const char long_gen57[]  = "specified completion action in not known";
static const char short_gen58[] = "**ch3|badmsgtype";
static const char long_gen58[]  = "request contained an invalid message type";
static const char short_gen59[] = "**ch3|badreqtype";
static const char long_gen59[]  = "request contained an invalid request type";
static const char short_gen60[] = "**ch3|canceleager";
static const char long_gen60[]  = "failure occurred while performing local cancellation of a eager message";
static const char short_gen61[] = "**ch3|cancelreq";
static const char long_gen61[]  = "failure occurred while sending remote cancellation request packet";
static const char short_gen62[] = "**ch3|cancelresp";
static const char long_gen62[]  = "failure occurred while attempting to send cancel response packet";
static const char short_gen63[] = "**ch3|cancelrndv";
static const char long_gen63[]  = "failure occurred while performing local cancellation of a rendezvous message";
static const char short_gen64[] = "**ch3|ctspkt";
static const char long_gen64[]  = "failure occurred while attempting to send CTS packet";
static const char short_gen65[] = "**ch3|eagermsg";
static const char long_gen65[]  = "failure occurred while attempting to send an eager message";
static const char short_gen66[] = "**ch3|flowcntlpkt";
static const char long_gen66[]  = "UNIMPLEMENTED: unable to handle flow control packets";
static const char short_gen67[] = "**ch3|loadrecviov";
static const char long_gen67[]  = "failure occurred while loading the receive I/O vector";
static const char short_gen68[] = "**ch3|loadsendiov";
static const char long_gen68[]  = "failure occurred while loading the send I/O vector";
static const char short_gen69[] = "**ch3|nopktcontainermem";
static const char long_gen69[]  = "failed to allocate memory for a packet reorder container";
static const char short_gen70[] = "**ch3|ooocancelreq";
static const char long_gen70[]  = "UNIMPLEMENTED: unable to process out-of-order cancellation requests";
static const char short_gen71[] = "**ch3|pktordered";
static const char long_gen71[]  = "failure occurred while processing a reordered packet";
static const char short_gen72[] = "**ch3|postrecv";
static const char long_gen72[]  = "failure occurred while posting a receive for message data";
static const char short_gen73[] = "**ch3|recvdata";
static const char long_gen73[]  = "failure occurred while attempting to receive message data";
static const char short_gen74[] = "**ch3|rmamsg";
static const char long_gen74[]  = "failure occurred while attempting to send an RMA message";
static const char short_gen75[] = "**ch3|rtspkt";
static const char long_gen75[]  = "failure occurred while attempting to send RTS packet";
static const char short_gen76[] = "**ch3|selfsenddeadlock";
static const char long_gen76[]  = "DEADLOCK: attempting to send a message to the local process without a prior matching receive";
static const char short_gen77[] = "**ch3|senddata";
static const char long_gen77[]  = "failure occurred while attempting to send message data";
static const char short_gen78[] = "**ch3|sock|addrinuse";
static const char long_gen78[]  = "[ch3:sock] tcp port already in use";
static const char short_gen79[] = "**ch3|sock|badbuscard";
static const char long_gen79[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen80[] = "**ch3|sock|badpacket";
static const char long_gen80[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen81[] = "**ch3|sock|badsock";
static const char long_gen81[]  = "[ch3:sock] internal error - bad sock";
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
static const char short_gen89[] = "**ch3|sock|pmi_finalize";
static const char long_gen89[]  = "PMI_Finalize failed";
static const char short_gen90[] = "**ch3|sock|post_write";
static const char long_gen90[]  = "[ch3:sock] posting a write failed";
static const char short_gen91[] = "**ch3|sock|progress_finalize";
static const char long_gen91[]  = "[ch3:sock] progress_finalize failed";
static const char short_gen92[] = "**ch3|sock|strdup";
static const char long_gen92[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen93[] = "**ch3|syncack";
static const char long_gen93[]  = "failure occurred while attempting to send eager synchronization packet";
static const char short_gen94[] = "**ch3|unknownpkt";
static const char long_gen94[]  = "received unknown packet type";
static const char short_gen95[] = "**comm";
static const char long_gen95[]  = "Invalid communicator";
static const char short_gen96[] = "**commnotinter";
static const char long_gen96[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen97[] = "**commnotintra";
static const char long_gen97[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen98[] = "**commnull";
static const char long_gen98[]  = "Null communicator";
static const char short_gen99[] = "**commperm";
static const char long_gen99[]  = "Cannot free permanent communicator";
static const char short_gen100[] = "**conn_still_active";
static const char long_gen100[]  = "connection closed while still active";
static const char short_gen101[] = "**connfailed";
static const char long_gen101[]  = "Failed to connect to remote process";
static const char short_gen102[] = "**connrefused";
static const char long_gen102[]  = "Connection refused";
static const char short_gen103[] = "**conversion";
static const char long_gen103[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen104[] = "**count";
static const char long_gen104[]  = "Invalid count";
static const char short_gen105[] = "**countneg";
static const char long_gen105[]  = "Negative count";
static const char short_gen106[] = "**datarep";
static const char long_gen106[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen107[] = "**datarepunsupported";
static const char long_gen107[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen108[] = "**desc_len";
static const char long_gen108[]  = "host description buffer too small";
static const char short_gen109[] = "**dims";
static const char long_gen109[]  = "Invalid dimension argument";
static const char short_gen110[] = "**dimsmany";
static const char long_gen110[]  = "Number of dimensions is too large ";
static const char short_gen111[] = "**dimspartition";
static const char long_gen111[]  = "Cannot partition nodes as requested ";
static const char short_gen112[] = "**dtype";
static const char long_gen112[]  = "Invalid datatype";
static const char short_gen113[] = "**dtypecommit";
static const char long_gen113[]  = "Datatype has not been committed ";
static const char short_gen114[] = "**dtypemismatch";
static const char long_gen114[]  = "Receiving data with a datatype whose signature does not match that of the sending datatype.";
static const char short_gen115[] = "**dtypenull";
static const char long_gen115[]  = "Null datatype";
static const char short_gen116[] = "**dtypeperm";
static const char long_gen116[]  = "Cannot free permanent data type ";
static const char short_gen117[] = "**duphandle";
static const char long_gen117[]  = "unable to duplicate a handle";
static const char short_gen118[] = "**dupprocesses";
static const char long_gen118[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen119[] = "**edgeoutrange";
static const char long_gen119[]  = "Edge index in graph topology is out of range";
static const char short_gen120[] = "**errhandnotfile";
static const char long_gen120[]  = "Error handler is not a file error handler";
static const char short_gen121[] = "**errhandnotwin";
static const char long_gen121[]  = "Error handler is not a win error handler";
static const char short_gen122[] = "**fail";
static const char long_gen122[]  = "failure";
static const char short_gen123[] = "**file";
static const char long_gen123[]  = "Invalid MPI_File";
static const char short_gen124[] = "**fileaccess";
static const char long_gen124[]  = "Access denied to file";
static const char short_gen125[] = "**fileamode";
static const char long_gen125[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen126[] = "**fileamodeone";
static const char long_gen126[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen127[] = "**fileamoderead";
static const char long_gen127[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen128[] = "**fileamodeseq";
static const char long_gen128[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen129[] = "**fileexist";
static const char long_gen129[]  = "File exists";
static const char short_gen130[] = "**fileinuse";
static const char long_gen130[]  = "File in use by some process";
static const char short_gen131[] = "**filename";
static const char long_gen131[]  = "Invalid file name";
static const char short_gen132[] = "**filenamedir";
static const char long_gen132[]  = "Invalid or missing directory";
static const char short_gen133[] = "**filenamelong";
static const char long_gen133[]  = "Pathname too long";
static const char short_gen134[] = "**filenoexist";
static const char long_gen134[]  = "File does not exist";
static const char short_gen135[] = "**filenospace";
static const char long_gen135[]  = "Not enough space for file ";
static const char short_gen136[] = "**fileopunsupported";
static const char long_gen136[]  = "Unsupported file operation ";
static const char short_gen137[] = "**filequota";
static const char long_gen137[]  = "Quota exceeded for files";
static const char short_gen138[] = "**filerdonly";
static const char long_gen138[]  = "Read-only file or filesystem name";
static const char short_gen139[] = "**finalize_boot";
static const char long_gen139[]  = "destroying the message queue failed";
static const char short_gen140[] = "**finalize_progress";
static const char long_gen140[]  = "finalizing the progress engine failed";
static const char short_gen141[] = "**finalize_progress_finalize";
static const char long_gen141[]  = "Progress finalize failed";
static const char short_gen142[] = "**finalize_release_mem";
static const char long_gen142[]  = "Release shared memory failed";
static const char short_gen143[] = "**finish_qp";
static const char long_gen143[]  = "unable to establish a queue pair connection";
static const char short_gen144[] = "**ftok";
static const char long_gen144[]  = "failed to create a sysv key from a file name";
static const char short_gen145[] = "**ftruncate";
static const char long_gen145[]  = "unable to resize the shared memory object";
static const char short_gen146[] = "**get_guids";
static const char long_gen146[]  = "unable to get the guids for the infiniband channel adapter";
static const char short_gen147[] = "**gethostbyname";
static const char long_gen147[]  = "gethostbyname failed";
static const char short_gen148[] = "**getinfo";
static const char long_gen148[]  = "getaddrinfo failed";
static const char short_gen149[] = "**graphnnodes";
static const char long_gen149[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen150[] = "**group";
static const char long_gen150[]  = "Invalid group";
static const char short_gen151[] = "**groupnotincomm";
static const char long_gen151[]  = "Specified group is not within the communicator";
static const char short_gen152[] = "**handle_read";
static const char long_gen152[]  = "Unable to handle the read data";
static const char short_gen153[] = "**handle_sock_op";
static const char long_gen153[]  = "handle_sock_op failed";
static const char short_gen154[] = "**handle_written";
static const char long_gen154[]  = "unable to handle written data";
static const char short_gen155[] = "**hostlookup";
static const char long_gen155[]  = "Host lookup failed";
static const char short_gen156[] = "**ibu_op";
static const char long_gen156[]  = "invalid infiniband operation";
static const char short_gen157[] = "**ibu_wait";
static const char long_gen157[]  = "ibu_wait failed";
static const char short_gen158[] = "**ibwrite";
static const char long_gen158[]  = "infiniband write failed";
static const char short_gen159[] = "**indexneg";
static const char long_gen159[]  = "Index value in graph topology must be nonnegative";
static const char short_gen160[] = "**indexnonmonotone";
static const char long_gen160[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen161[] = "**info";
static const char long_gen161[]  = "Invalid MPI_Info";
static const char short_gen162[] = "**infokey";
static const char long_gen162[]  = "Invalid key for MPI_Info ";
static const char short_gen163[] = "**infokeyempty";
static const char long_gen163[]  = "Empty or blank key ";
static const char short_gen164[] = "**infokeylong";
static const char long_gen164[]  = "Key is too long";
static const char short_gen165[] = "**infokeynull";
static const char long_gen165[]  = "Null key";
static const char short_gen166[] = "**infonkey";
static const char long_gen166[]  = "Requested nth key does not exist";
static const char short_gen167[] = "**infonokey";
static const char long_gen167[]  = "MPI_Info key is not defined ";
static const char short_gen168[] = "**infoval";
static const char long_gen168[]  = "Invalid MPI_Info value ";
static const char short_gen169[] = "**infovallong";
static const char long_gen169[]  = "Value is too long ";
static const char short_gen170[] = "**infovalnull";
static const char long_gen170[]  = "Null value";
static const char short_gen171[] = "**init";
static const char long_gen171[]  = "Initialization failed";
static const char short_gen172[] = "**init_buscard";
static const char long_gen172[]  = "failed to get my business card";
static const char short_gen173[] = "**init_description";
static const char long_gen173[]  = "unable to get the host description";
static const char short_gen174[] = "**init_getptr";
static const char long_gen174[]  = "failed to get the vcr";
static const char short_gen175[] = "**init_ibu";
static const char long_gen175[]  = "infiniband initialization failed";
static const char short_gen176[] = "**init_ibu_set";
static const char long_gen176[]  = "unable to create an infinband completion queue";
static const char short_gen177[] = "**init_progress";
static const char long_gen177[]  = "progress_init failed";
static const char short_gen178[] = "**init_vcrdup";
static const char long_gen178[]  = "failed to duplicate the virtual connection reference";
static const char short_gen179[] = "**init_vcrt";
static const char long_gen179[]  = "failed to create VCRT";
static const char short_gen180[] = "**initialized";
static const char long_gen180[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen181[] = "**inittwice";
static const char long_gen181[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen182[] = "**inpending";
static const char long_gen182[]  = "Pending request (no error)";
static const char short_gen183[] = "**instatus";
static const char long_gen183[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen184[] = "**intern";
static const char long_gen184[]  = "Internal MPI error!";
static const char short_gen185[] = "**inttoosmall";
static const char long_gen185[]  = "An address does not fit into a Fortran INTEGER.  Use MPI_Get_address instead";
static const char short_gen186[] = "**invalid_handle";
static const char long_gen186[]  = "invalid handle";
static const char short_gen187[] = "**invalid_listener";
static const char long_gen187[]  = "invalid listener";
static const char short_gen188[] = "**invalid_refcount";
static const char long_gen188[]  = "invalid reference count";
static const char short_gen189[] = "**invalid_shmq";
static const char long_gen189[]  = "invalid shm queue pointer";
static const char short_gen190[] = "**io";
static const char long_gen190[]  = "Other I/O error ";
static const char short_gen191[] = "**ioRMWrdwr";
static const char long_gen191[]  = "Must open file with MPI_MODE_RDWR for read-modify-write ";
static const char short_gen192[] = "**ioagnomatch";
static const char long_gen192[]  = "No aggregators match";
static const char short_gen193[] = "**ioamodeseq";
static const char long_gen193[]  = "Cannot use this function when the file is opened with amode MPI_MODE_SEQUENTIAL ";
static const char short_gen194[] = "**iobadcount";
static const char long_gen194[]  = "Invalid count argument";
static const char short_gen195[] = "**iobaddisp";
static const char long_gen195[]  = "Invalid displacement argument";
static const char short_gen196[] = "**iobadfh";
static const char long_gen196[]  = "Invalid file handle";
static const char short_gen197[] = "**iobadoffset";
static const char long_gen197[]  = "Invalid offset argument";
static const char short_gen198[] = "**iobadsize";
static const char long_gen198[]  = "Invalid size argument";
static const char short_gen199[] = "**iobadwhence";
static const char long_gen199[]  = "Invalid whence argument";
static const char short_gen200[] = "**iocp";
static const char long_gen200[]  = "unable to create an I/O completion port";
static const char short_gen201[] = "**iodatarepnomem";
static const char long_gen201[]  = "User must allocate memory for datarep";
static const char short_gen202[] = "**iodispifseq";
static const char long_gen202[]  = "disp must be set to MPI_DISPLACEMENT_CURRENT since file was opened with MPI_MODE_SEQUENTIAL";
static const char short_gen203[] = "**ioetype";
static const char long_gen203[]  = "Only an integral number of etypes can be accessed";
static const char short_gen204[] = "**iofiletype";
static const char long_gen204[]  = "Filetype must be constructed out of one or more etypes";
static const char short_gen205[] = "**iofstype";
static const char long_gen205[]  = "Cannot determine filesystem type";
static const char short_gen206[] = "**iofstypeunsupported";
static const char long_gen206[]  = "Specified filesystem is not available";
static const char short_gen207[] = "**ioneedrd";
static const char long_gen207[]  = "Read access is required to this file";
static const char short_gen208[] = "**ionegoffset";
static const char long_gen208[]  = "Negative offset argument";
static const char short_gen209[] = "**iopreallocrdwr";
static const char long_gen209[]  = "Must open file with MPI_MODE_RDWR to preallocate disk space";
static const char short_gen210[] = "**iosequnsupported";
static const char long_gen210[]  = "MPI_MODE_SEQUENTIAL not supported on this file system";
static const char short_gen211[] = "**iosharedfailed";
static const char long_gen211[]  = "Could not access shared file pointer";
static const char short_gen212[] = "**iosharedunsupported";
static const char long_gen212[]  = "Shared file pointers not supported";
static const char short_gen213[] = "**iosplitcoll";
static const char long_gen213[]  = "Only one active split collective I/O operation is allowed per file handle";
static const char short_gen214[] = "**iosplitcollnone";
static const char long_gen214[]  = "No split collective I/O operation is active";
static const char short_gen215[] = "**iov_offset";
static const char long_gen215[]  = "invalid iov offset";
static const char short_gen216[] = "**keyval";
static const char long_gen216[]  = "Invalid keyval";
static const char short_gen217[] = "**keyvalinvalid";
static const char long_gen217[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen218[] = "**keyvalnotcomm";
static const char long_gen218[]  = "Keyval was not defined for communicators";
static const char short_gen219[] = "**keyvalnotdatatype";
static const char long_gen219[]  = "Keyval was not defined for datatype";
static const char short_gen220[] = "**keyvalnotwin";
static const char long_gen220[]  = "Keyval was not defined for window objects";
static const char short_gen221[] = "**listen";
static const char long_gen221[]  = "listen failed";
static const char short_gen222[] = "**locktype";
static const char long_gen222[]  = "Invalid locktype";
static const char short_gen223[] = "**mmap";
static const char long_gen223[]  = "unable to map memory, mmap failed";
static const char short_gen224[] = "**mpi_accumulate";
static const char long_gen224[]  = "MPI_Accumulate failed";
static const char short_gen225[] = "**mpi_add_error_string";
static const char long_gen225[]  = "MPI_Add_error_string failed";
static const char short_gen226[] = "**mpi_allgather";
static const char long_gen226[]  = "MPI_Allgather failed";
static const char short_gen227[] = "**mpi_allgatherv";
static const char long_gen227[]  = "MPI_Allgatherv failed";
static const char short_gen228[] = "**mpi_allreduce";
static const char long_gen228[]  = "MPI_Allreduce failed";
static const char short_gen229[] = "**mpi_alltoall";
static const char long_gen229[]  = "MPI_Alltoall failed";
static const char short_gen230[] = "**mpi_alltoallv";
static const char long_gen230[]  = "MPI_Alltoallv failed";
static const char short_gen231[] = "**mpi_alltoallw";
static const char long_gen231[]  = "MPI_Alltoallw failed";
static const char short_gen232[] = "**mpi_attr_delete";
static const char long_gen232[]  = "MPI_Attr_delete failed";
static const char short_gen233[] = "**mpi_attr_get";
static const char long_gen233[]  = "MPI_Attr_get failed";
static const char short_gen234[] = "**mpi_attr_put";
static const char long_gen234[]  = "MPI_Attr_put failed";
static const char short_gen235[] = "**mpi_barrier";
static const char long_gen235[]  = "MPI_Barrier failed";
static const char short_gen236[] = "**mpi_bcast";
static const char long_gen236[]  = "MPI_Bcast failed";
static const char short_gen237[] = "**mpi_bsend";
static const char long_gen237[]  = "MPI_Bsend failed";
static const char short_gen238[] = "**mpi_bsend_init";
static const char long_gen238[]  = "MPI_Bsend_init failed";
static const char short_gen239[] = "**mpi_buffer_attach";
static const char long_gen239[]  = "MPI_Buffer_attach failed";
static const char short_gen240[] = "**mpi_buffer_detach";
static const char long_gen240[]  = "MPI_Buffer_detach failed";
static const char short_gen241[] = "**mpi_cancel";
static const char long_gen241[]  = "MPI_Cancel failed";
static const char short_gen242[] = "**mpi_cart_create";
static const char long_gen242[]  = "MPI_Cart_create failed";
static const char short_gen243[] = "**mpi_close_port";
static const char long_gen243[]  = "MPI_Close_port failed";
static const char short_gen244[] = "**mpi_comm_accept";
static const char long_gen244[]  = "MPI_Comm_accept failed";
static const char short_gen245[] = "**mpi_comm_connect";
static const char long_gen245[]  = "MPI_Comm_connect failed";
static const char short_gen246[] = "**mpi_comm_disconnect";
static const char long_gen246[]  = "MPI_Comm_disconnect failed";
static const char short_gen247[] = "**mpi_comm_dup";
static const char long_gen247[]  = "MPI_Comm_dup failed";
static const char short_gen248[] = "**mpi_comm_free";
static const char long_gen248[]  = "MPI_Comm_free failed";
static const char short_gen249[] = "**mpi_comm_group";
static const char long_gen249[]  = "MPI_Comm_group failed";
static const char short_gen250[] = "**mpi_comm_join";
static const char long_gen250[]  = "MPI_Comm_join failed";
static const char short_gen251[] = "**mpi_comm_remote_group";
static const char long_gen251[]  = "MPI_Comm_remote_group failed";
static const char short_gen252[] = "**mpi_comm_set_attr";
static const char long_gen252[]  = "MPI_Comm_set_attr failed";
static const char short_gen253[] = "**mpi_comm_spawn";
static const char long_gen253[]  = "MPI_Comm_spawn failed";
static const char short_gen254[] = "**mpi_comm_spawn_multiple";
static const char long_gen254[]  = "MPI_Comm_spawn_multiple failed";
static const char short_gen255[] = "**mpi_comm_split";
static const char long_gen255[]  = "MPI_Comm_split failed";
static const char short_gen256[] = "**mpi_errhandler_create";
static const char long_gen256[]  = "MPI_Errhandler_create failed";
static const char short_gen257[] = "**mpi_errhandler_get";
static const char long_gen257[]  = "MPI_Errhandler_get failed";
static const char short_gen258[] = "**mpi_errhandler_set";
static const char long_gen258[]  = "MPI_Errhandler_set failed";
static const char short_gen259[] = "**mpi_exscan";
static const char long_gen259[]  = "MPI_Exscan failed";
static const char short_gen260[] = "**mpi_file_create_errhandler";
static const char long_gen260[]  = "MPI_File_create_errhandler failed";
static const char short_gen261[] = "**mpi_gather";
static const char long_gen261[]  = "MPI_Gather failed";
static const char short_gen262[] = "**mpi_gatherv";
static const char long_gen262[]  = "MPI_Gatherv failed";
static const char short_gen263[] = "**mpi_get";
static const char long_gen263[]  = "MPI_Get failed";
static const char short_gen264[] = "**mpi_get_processor_name";
static const char long_gen264[]  = "MPI_Get_processor_name failed";
static const char short_gen265[] = "**mpi_graph_create";
static const char long_gen265[]  = "MPI_Graph_create failed";
static const char short_gen266[] = "**mpi_graph_map";
static const char long_gen266[]  = "MPI_Graph_map failed";
static const char short_gen267[] = "**mpi_grequest_start";
static const char long_gen267[]  = "MPI_Grequest_start failed";
static const char short_gen268[] = "**mpi_group_difference";
static const char long_gen268[]  = "MPI_Group_difference failed";
static const char short_gen269[] = "**mpi_group_excl";
static const char long_gen269[]  = "MPI_Group_excl failed";
static const char short_gen270[] = "**mpi_group_free";
static const char long_gen270[]  = "MPI_Group_free failed";
static const char short_gen271[] = "**mpi_group_incl";
static const char long_gen271[]  = "MPI_Group_incl failed";
static const char short_gen272[] = "**mpi_group_intersection";
static const char long_gen272[]  = "MPI_Group_intersection failed";
static const char short_gen273[] = "**mpi_group_range_excl";
static const char long_gen273[]  = "MPI_Group_range_excl failed";
static const char short_gen274[] = "**mpi_group_range_incl";
static const char long_gen274[]  = "MPI_Group_range_incl failed";
static const char short_gen275[] = "**mpi_group_union";
static const char long_gen275[]  = "MPI_Group_union failed";
static const char short_gen276[] = "**mpi_ibsend";
static const char long_gen276[]  = "MPI_Ibsend failed";
static const char short_gen277[] = "**mpi_info_create";
static const char long_gen277[]  = "MPI_Info_create failed";
static const char short_gen278[] = "**mpi_info_delete";
static const char long_gen278[]  = "MPI_Info_delete failed";
static const char short_gen279[] = "**mpi_info_dup";
static const char long_gen279[]  = "MPI_Info_dup failed";
static const char short_gen280[] = "**mpi_info_get_nthkey";
static const char long_gen280[]  = "MPI_Info_get_nthkey failed";
static const char short_gen281[] = "**mpi_info_set";
static const char long_gen281[]  = "MPI_Info_set failed";
static const char short_gen282[] = "**mpi_init";
static const char long_gen282[]  = "MPI_Init failed";
static const char short_gen283[] = "**mpi_init_thread";
static const char long_gen283[]  = "MPI_Init_thread failed";
static const char short_gen284[] = "**mpi_intercomm_create";
static const char long_gen284[]  = "MPI_Intercomm_create failed";
static const char short_gen285[] = "**mpi_intercomm_merge";
static const char long_gen285[]  = "MPI_Intercomm_merge failed";
static const char short_gen286[] = "**mpi_iprobe";
static const char long_gen286[]  = "MPI_Iprobe failed";
static const char short_gen287[] = "**mpi_irecv";
static const char long_gen287[]  = "MPI_Irecv failed";
static const char short_gen288[] = "**mpi_irsend";
static const char long_gen288[]  = "MPI_Irsend failed";
static const char short_gen289[] = "**mpi_isend";
static const char long_gen289[]  = "MPI_Isend failed";
static const char short_gen290[] = "**mpi_issend";
static const char long_gen290[]  = "MPI_Issend failed";
static const char short_gen291[] = "**mpi_keyval_create";
static const char long_gen291[]  = "MPI_Keyval_create failed";
static const char short_gen292[] = "**mpi_keyval_free";
static const char long_gen292[]  = "MPI_Keyval_free failed";
static const char short_gen293[] = "**mpi_lookup_name";
static const char long_gen293[]  = "MPI_Lookup_name failed";
static const char short_gen294[] = "**mpi_op_create";
static const char long_gen294[]  = "MPI_Op_create failed";
static const char short_gen295[] = "**mpi_open_port";
static const char long_gen295[]  = "MPI_Open_port failed";
static const char short_gen296[] = "**mpi_pack";
static const char long_gen296[]  = "MPI_Pack failed";
static const char short_gen297[] = "**mpi_pack_external";
static const char long_gen297[]  = "MPI_Pack_external failed";
static const char short_gen298[] = "**mpi_probe";
static const char long_gen298[]  = "MPI_Probe failed";
static const char short_gen299[] = "**mpi_publish_name";
static const char long_gen299[]  = "MPI_Publish_name failed";
static const char short_gen300[] = "**mpi_put";
static const char long_gen300[]  = "MPI_Put failed";
static const char short_gen301[] = "**mpi_recv";
static const char long_gen301[]  = "MPI_Recv failed";
static const char short_gen302[] = "**mpi_recv_init";
static const char long_gen302[]  = "MPI_Recv_init failed";
static const char short_gen303[] = "**mpi_reduce";
static const char long_gen303[]  = "MPI_Reduce failed";
static const char short_gen304[] = "**mpi_reduce_scatter";
static const char long_gen304[]  = "MPI_Reduce_scatter failed";
static const char short_gen305[] = "**mpi_register_datarep";
static const char long_gen305[]  = "MPI_Register_datarep failed";
static const char short_gen306[] = "**mpi_request_free";
static const char long_gen306[]  = "MPI_Request_free failed";
static const char short_gen307[] = "**mpi_request_get_status";
static const char long_gen307[]  = "MPI_Request_get_status failed";
static const char short_gen308[] = "**mpi_rsend";
static const char long_gen308[]  = "MPI_Rsend failed";
static const char short_gen309[] = "**mpi_rsend_init";
static const char long_gen309[]  = "MPI_Rsend_init failed";
static const char short_gen310[] = "**mpi_scan";
static const char long_gen310[]  = "MPI_Scan failed";
static const char short_gen311[] = "**mpi_scatter";
static const char long_gen311[]  = "MPI_Scatter failed";
static const char short_gen312[] = "**mpi_scatterv";
static const char long_gen312[]  = "MPI_Scatterv failed";
static const char short_gen313[] = "**mpi_send";
static const char long_gen313[]  = "MPI_Send failed";
static const char short_gen314[] = "**mpi_send_init";
static const char long_gen314[]  = "MPI_Send_init failed";
static const char short_gen315[] = "**mpi_sendrecv";
static const char long_gen315[]  = "MPI_Sendrecv failed";
static const char short_gen316[] = "**mpi_sendrecv_replace";
static const char long_gen316[]  = "MPI_Sendrecv_replace failed";
static const char short_gen317[] = "**mpi_ssend";
static const char long_gen317[]  = "MPI_Ssend failed";
static const char short_gen318[] = "**mpi_ssend_init";
static const char long_gen318[]  = "MPI_Ssend_init failed";
static const char short_gen319[] = "**mpi_start";
static const char long_gen319[]  = "MPI_Start failed";
static const char short_gen320[] = "**mpi_startall";
static const char long_gen320[]  = "MPI_Start_all failed";
static const char short_gen321[] = "**mpi_test";
static const char long_gen321[]  = "MPI_Test failed";
static const char short_gen322[] = "**mpi_testall";
static const char long_gen322[]  = "MPI_Testall failed";
static const char short_gen323[] = "**mpi_testany";
static const char long_gen323[]  = "MPI_Testany failed";
static const char short_gen324[] = "**mpi_testsome";
static const char long_gen324[]  = "MPI_Testsome failed";
static const char short_gen325[] = "**mpi_type_commit";
static const char long_gen325[]  = "MPI_Type_commit failed";
static const char short_gen326[] = "**mpi_type_contiguous";
static const char long_gen326[]  = "MPI_Type_continuous failed";
static const char short_gen327[] = "**mpi_type_create_hindexed";
static const char long_gen327[]  = "MPI_Type_create_hindexed failed";
static const char short_gen328[] = "**mpi_type_create_hvector";
static const char long_gen328[]  = "MPI_Type_create_hvector failed";
static const char short_gen329[] = "**mpi_type_create_indexed_block";
static const char long_gen329[]  = "MPI_Type_create_indexed_block failed";
static const char short_gen330[] = "**mpi_type_create_keyval";
static const char long_gen330[]  = "MPI_Type_create_keyval failed";
static const char short_gen331[] = "**mpi_type_create_resized";
static const char long_gen331[]  = "MPI_Type_create_resized failed";
static const char short_gen332[] = "**mpi_type_create_struct";
static const char long_gen332[]  = "MPI_Type_create_struct failed";
static const char short_gen333[] = "**mpi_type_delete_attr";
static const char long_gen333[]  = "MPI_Type_delete_attr failed";
static const char short_gen334[] = "**mpi_type_dup";
static const char long_gen334[]  = "MPI_Type_dup failed";
static const char short_gen335[] = "**mpi_type_get_contents";
static const char long_gen335[]  = "MPI_Type_get_contents failed";
static const char short_gen336[] = "**mpi_type_get_envelope";
static const char long_gen336[]  = "MPI_Type_get_envelope failed";
static const char short_gen337[] = "**mpi_type_hindexed";
static const char long_gen337[]  = "MPI_Type_hindexed failed";
static const char short_gen338[] = "**mpi_type_hvector";
static const char long_gen338[]  = "MPI_Type_hvector failed";
static const char short_gen339[] = "**mpi_type_indexed";
static const char long_gen339[]  = "MPI_Type_indexed failed";
static const char short_gen340[] = "**mpi_type_match_size";
static const char long_gen340[]  = "MPI_Type_match_size failed";
static const char short_gen341[] = "**mpi_type_set_attr";
static const char long_gen341[]  = "MPI_Type_set_attr failed";
static const char short_gen342[] = "**mpi_type_struct";
static const char long_gen342[]  = "MPI_Type_struct failed";
static const char short_gen343[] = "**mpi_type_vector";
static const char long_gen343[]  = "MPI_Type_vector failed";
static const char short_gen344[] = "**mpi_unpack_external";
static const char long_gen344[]  = "MPI_Unpack_external failed";
static const char short_gen345[] = "**mpi_unpublish_name";
static const char long_gen345[]  = "MPI_Unpublish_name failed";
static const char short_gen346[] = "**mpi_wait";
static const char long_gen346[]  = "MPI_Wait failed";
static const char short_gen347[] = "**mpi_waitall";
static const char long_gen347[]  = "MPI_Waitall failed";
static const char short_gen348[] = "**mpi_waitany";
static const char long_gen348[]  = "MPI_Waitany failed";
static const char short_gen349[] = "**mpi_waitsome";
static const char long_gen349[]  = "MPI_Waitsome failed";
static const char short_gen350[] = "**mpi_win_complete";
static const char long_gen350[]  = "MPI_Win_complete failed";
static const char short_gen351[] = "**mpi_win_create";
static const char long_gen351[]  = "MPI_Win_create failed";
static const char short_gen352[] = "**mpi_win_create_errhandler";
static const char long_gen352[]  = "MPI_Win_create_errhandler failed";
static const char short_gen353[] = "**mpi_win_create_keyval";
static const char long_gen353[]  = "MPI_Win_create_keyval failed";
static const char short_gen354[] = "**mpi_win_delete_attr";
static const char long_gen354[]  = "MPI_Win_delete_attr failed";
static const char short_gen355[] = "**mpi_win_fence";
static const char long_gen355[]  = "MPI_Win_fence failed";
static const char short_gen356[] = "**mpi_win_free";
static const char long_gen356[]  = "MPI_Win_free failed";
static const char short_gen357[] = "**mpi_win_get_group";
static const char long_gen357[]  = "MPI_Win_get_group failed";
static const char short_gen358[] = "**mpi_win_lock";
static const char long_gen358[]  = "MPI_Win_lock failed";
static const char short_gen359[] = "**mpi_win_post";
static const char long_gen359[]  = "MPI_Win_post failed";
static const char short_gen360[] = "**mpi_win_set_attr";
static const char long_gen360[]  = "MPI_Win_set_attr failed";
static const char short_gen361[] = "**mpi_win_test";
static const char long_gen361[]  = "MPI_Win_test failed";
static const char short_gen362[] = "**mpi_win_unlock";
static const char long_gen362[]  = "MPI_Win_unlock failed";
static const char short_gen363[] = "**mpi_win_wait";
static const char long_gen363[]  = "MPI_Win_wait failed";
static const char short_gen364[] = "**mq_close";
static const char long_gen364[]  = "failed to close a posix message queue";
static const char short_gen365[] = "**mq_open";
static const char long_gen365[]  = "failed to open a posix message queue";
static const char short_gen366[] = "**mq_receive";
static const char long_gen366[]  = "failed to receive a posix message queue message";
static const char short_gen367[] = "**mq_send";
static const char long_gen367[]  = "failed to send a posix message queue message";
static const char short_gen368[] = "**mqp_failure";
static const char long_gen368[]  = "failed to make progress on the shared memory bootstrap message queue";
static const char short_gen369[] = "**mqshm_create";
static const char long_gen369[]  = "failed to create a shared memory message queue";
static const char short_gen370[] = "**mqshm_receive";
static const char long_gen370[]  = "failed to receive a bootstrap message";
static const char short_gen371[] = "**mqshm_send";
static const char long_gen371[]  = "failed to send a bootstrap message";
static const char short_gen372[] = "**mqshm_unlink";
static const char long_gen372[]  = "unable to unlink the shared memory message queue";
static const char short_gen373[] = "**msgctl";
static const char long_gen373[]  = "msgctl failed";
static const char short_gen374[] = "**msgget";
static const char long_gen374[]  = "msgget failed";
static const char short_gen375[] = "**msgrcv";
static const char long_gen375[]  = "msgrcv failed";
static const char short_gen376[] = "**msgsnd";
static const char long_gen376[]  = "msgsnd failed";
static const char short_gen377[] = "**multi_post_read";
static const char long_gen377[]  = "posting a read while a previously posted read is outstanding";
static const char short_gen378[] = "**multi_post_write";
static const char long_gen378[]  = "posting a write while a previously posted write is outstanding";
static const char short_gen379[] = "**namepublish";
static const char long_gen379[]  = "Unable to publish service name";
static const char short_gen380[] = "**namepubnotpub";
static const char long_gen380[]  = "Lookup failed for service name ";
static const char short_gen381[] = "**nameservice";
static const char long_gen381[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen382[] = "**needthreads";
static const char long_gen382[]  = "This function needs threads and threads have not been enabled";
static const char short_gen383[] = "**nextbootmsg";
static const char long_gen383[]  = "failed to get the next bootstrap message";
static const char short_gen384[] = "**noca";
static const char long_gen384[]  = "unable to find an active infiniband channel adapter";
static const char short_gen385[] = "**noerrclasses";
static const char long_gen385[]  = "No more user-defined error classes";
static const char short_gen386[] = "**noerrcodes";
static const char long_gen386[]  = "No more user-defined error codes";
static const char short_gen387[] = "**nomem";
static const char long_gen387[]  = "Out of memory";
static const char short_gen388[] = "**nomemreq";
static const char long_gen388[]  = "failure occurred while allocating memory for a request object";
static const char short_gen389[] = "**nonamepub";
static const char long_gen389[]  = "No name publishing service available";
static const char short_gen390[] = "**notcarttopo";
static const char long_gen390[]  = "No Cartesian topology associated with this communicator";
static const char short_gen391[] = "**notcstatignore";
static const char long_gen391[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_c2f()";
static const char short_gen392[] = "**notfstatignore";
static const char long_gen392[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_f2c()";
static const char short_gen393[] = "**notgenreq";
static const char long_gen393[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen394[] = "**notgraphtopo";
static const char long_gen394[]  = "No Graph topology associated with this communicator";
static const char short_gen395[] = "**notimpl";
static const char long_gen395[]  = "Function not implemented";
static const char short_gen396[] = "**notopology";
static const char long_gen396[]  = "No topology associated with this communicator";
static const char short_gen397[] = "**notsame";
static const char long_gen397[]  = "Inconsistent arguments to collective routine ";
static const char short_gen398[] = "**nulledge";
static const char long_gen398[]  = "Edge detected from a node to the same node";
static const char short_gen399[] = "**nullptr";
static const char long_gen399[]  = "Null pointer";
static const char short_gen400[] = "**nullptrtype";
static const char long_gen400[]  = "Null pointer";
static const char short_gen401[] = "**op";
static const char long_gen401[]  = "Invalid MPI_Op";
static const char short_gen402[] = "**open";
static const char long_gen402[]  = "open failed";
static const char short_gen403[] = "**opnotpredefined";
static const char long_gen403[]  = "only predefined ops are valid";
static const char short_gen404[] = "**opundefined";
static const char long_gen404[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen405[] = "**opundefined_rma";
static const char long_gen405[]  = "RMA target received unknown RMA operation";
static const char short_gen406[] = "**other";
static const char long_gen406[]  = "Other MPI error";
static const char short_gen407[] = "**pctwice";
static const char long_gen407[]  = "post connect called twice";
static const char short_gen408[] = "**pd_alloc";
static const char long_gen408[]  = "unable to allocate a protection domain";
static const char short_gen409[] = "**permattr";
static const char long_gen409[]  = "Cannot set permanent attribute";
static const char short_gen410[] = "**permop";
static const char long_gen410[]  = "Cannot free permanent MPI_Op ";
static const char short_gen411[] = "**pfinal_sockclose";
static const char long_gen411[]  = "sock_close failed";
static const char short_gen412[] = "**pkt_ptr";
static const char long_gen412[]  = "invalid shm queue packet pointer";
static const char short_gen413[] = "**pmi_barrier";
static const char long_gen413[]  = "PMI_Barrier failed";
static const char short_gen414[] = "**pmi_finalize";
static const char long_gen414[]  = "PMI_Finalize failed";
static const char short_gen415[] = "**pmi_get_id";
static const char long_gen415[]  = "PMI_Get_id failed";
static const char short_gen416[] = "**pmi_get_id_length_max";
static const char long_gen416[]  = "PMI_Get_id_length_max failed";
static const char short_gen417[] = "**pmi_get_rank";
static const char long_gen417[]  = "PMI_Get_rank failed";
static const char short_gen418[] = "**pmi_get_size";
static const char long_gen418[]  = "PMI_Get_size failed";
static const char short_gen419[] = "**pmi_init";
static const char long_gen419[]  = "PMI_Init failed";
static const char short_gen420[] = "**pmi_kvs_commit";
static const char long_gen420[]  = "PMI_KVS_Commit failed";
static const char short_gen421[] = "**pmi_kvs_create";
static const char long_gen421[]  = "PMI_KVS_Create failed";
static const char short_gen422[] = "**pmi_kvs_get";
static const char long_gen422[]  = "PMI_KVS_Get failed";
static const char short_gen423[] = "**pmi_kvs_get_key_length_max";
static const char long_gen423[]  = "PMI_KVS_Get_key_length_max failed";
static const char short_gen424[] = "**pmi_kvs_get_my_name";
static const char long_gen424[]  = "PMI_KVS_Get_my_name failed";
static const char short_gen425[] = "**pmi_kvs_get_name_length_max";
static const char long_gen425[]  = "PMI_KVS_Get_name_length_max failed";
static const char short_gen426[] = "**pmi_kvs_get_value_length_max";
static const char long_gen426[]  = "PMI_KVS_Get_value_length_max failed";
static const char short_gen427[] = "**pmi_kvs_put";
static const char long_gen427[]  = "PMI_KVS_Put failed";
static const char short_gen428[] = "**pmi_spawn_multiple";
static const char long_gen428[]  = "PMI_Spawn_multiple failed";
static const char short_gen429[] = "**poke";
static const char long_gen429[]  = "progress_poke failed";
static const char short_gen430[] = "**port";
static const char long_gen430[]  = "Invalid port";
static const char short_gen431[] = "**post_accept";
static const char long_gen431[]  = "post accept failed";
static const char short_gen432[] = "**post_connect";
static const char long_gen432[]  = "failed to post a connection";
static const char short_gen433[] = "**post_sock_write_on_shm";
static const char long_gen433[]  = "posting a socket read on a shm connection";
static const char short_gen434[] = "**postpkt";
static const char long_gen434[]  = "Unable to post a read for the next packet header";
static const char short_gen435[] = "**process_group";
static const char long_gen435[]  = "Process group initialization failed";
static const char short_gen436[] = "**progress";
static const char long_gen436[]  = "progress engine failure";
static const char short_gen437[] = "**progress_finalize";
static const char long_gen437[]  = "finalization of the progress engine failed";
static const char short_gen438[] = "**progress_handle_sock_op";
static const char long_gen438[]  = "handle_sock_op failed";
static const char short_gen439[] = "**progress_init";
static const char long_gen439[]  = "unable to initialize the progress engine";
static const char short_gen440[] = "**progress_sock_wait";
static const char long_gen440[]  = "sock_wait failed";
static const char short_gen441[] = "**progress_test";
static const char long_gen441[]  = "progress_test engine failure";
static const char short_gen442[] = "**rangedup";
static const char long_gen442[]  = "The range array specifies duplicate entries";
static const char short_gen443[] = "**rangeendinvalid";
static const char long_gen443[]  = "Some element of a range array is either negative or too large";
static const char short_gen444[] = "**rangestartinvalid";
static const char long_gen444[]  = "Some element of a range array is either negative or too large";
static const char short_gen445[] = "**rank";
static const char long_gen445[]  = "Invalid rank";
static const char short_gen446[] = "**rankarray";
static const char long_gen446[]  = "Invalid rank in rank array";
static const char short_gen447[] = "**rankdup";
static const char long_gen447[]  = "Duplicate ranks in rank array ";
static const char short_gen448[] = "**ranklocal";
static const char long_gen448[]  = "Error specifying local_leader ";
static const char short_gen449[] = "**rankremote";
static const char long_gen449[]  = "Error specifying remote_leader ";
static const char short_gen450[] = "**rdma_finalize";
static const char long_gen450[]  = "Channel rdma finalization failed";
static const char short_gen451[] = "**rdma_init";
static const char long_gen451[]  = "Channel rdma initialization failed";
static const char short_gen452[] = "**read_progress";
static const char long_gen452[]  = "Unable to make read progress";
static const char short_gen453[] = "**recvbuf_inplace";
static const char long_gen453[]  = "recvbuf cannot be MPI_IN_PLACE";
static const char short_gen454[] = "**request";
static const char long_gen454[]  = "Invalid MPI_Request";
static const char short_gen455[] = "**requestnotpersist";
static const char long_gen455[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen456[] = "**requestpersistactive";
static const char long_gen456[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen457[] = "**rmaconflict";
static const char long_gen457[]  = "Conflicting accesses to window ";
static const char short_gen458[] = "**rmadisp";
static const char long_gen458[]  = "Invalid displacement argument in RMA call ";
static const char short_gen459[] = "**rmasize";
static const char long_gen459[]  = "Invalid size argument in RMA call";
static const char short_gen460[] = "**rmasync";
static const char long_gen460[]  = "Wrong synchronization of RMA calls ";
static const char short_gen461[] = "**root";
static const char long_gen461[]  = "Invalid root";
static const char short_gen462[] = "**rsendnomatch";
static const char long_gen462[]  = "Ready send had no matching receive ";
static const char short_gen463[] = "**sendbuf_inplace";
static const char long_gen463[]  = "sendbuf cannot be MPI_IN_PLACE";
static const char short_gen464[] = "**servicename";
static const char long_gen464[]  = "Attempt to lookup an unknown service name ";
static const char short_gen465[] = "**shm_op";
static const char long_gen465[]  = "invalid shm operation";
static const char short_gen466[] = "**shm_open";
static const char long_gen466[]  = "unable to open a shared memory object";
static const char short_gen467[] = "**shm_read_progress";
static const char long_gen467[]  = "shared memory read progress failed";
static const char short_gen468[] = "**shm_unlink";
static const char long_gen468[]  = "failed to unlink shared memory";
static const char short_gen469[] = "**shm_wait";
static const char long_gen469[]  = "wait function failed";
static const char short_gen470[] = "**shmat";
static const char long_gen470[]  = "shmat failed";
static const char short_gen471[] = "**shmconnect_getmem";
static const char long_gen471[]  = "failed to allocate shared memory for a write queue";
static const char short_gen472[] = "**shmctl";
static const char long_gen472[]  = "failed to mark the sysv segment for removal";
static const char short_gen473[] = "**shmget";
static const char long_gen473[]  = "shmget failed";
static const char short_gen474[] = "**shmgetmem";
static const char long_gen474[]  = "Unable to allocate shared memory";
static const char short_gen475[] = "**shmhost";
static const char long_gen475[]  = "process not on the same host";
static const char short_gen476[] = "**shmq";
static const char long_gen476[]  = "invalid shm queue pointer";
static const char short_gen477[] = "**shmq_index";
static const char long_gen477[]  = "invalid shm queue index";
static const char short_gen478[] = "**shmwrite";
static const char long_gen478[]  = "shared memory write failed";
static const char short_gen479[] = "**snprintf";
static const char long_gen479[]  = "snprintf returned an invalid number";
static const char short_gen480[] = "**sock_byname";
static const char long_gen480[]  = "gethostbyname failed";
static const char short_gen481[] = "**sock_closed";
static const char long_gen481[]  = "socket closed";
static const char short_gen482[] = "**sock_connect";
static const char long_gen482[]  = "connect failed";
static const char short_gen483[] = "**sock_create";
static const char long_gen483[]  = "unable to create a socket";
static const char short_gen484[] = "**sock_gethost";
static const char long_gen484[]  = "gethostname failed";
static const char short_gen485[] = "**sock_init";
static const char long_gen485[]  = "unable to initialize the sock library";
static const char short_gen486[] = "**sock_nop_accept";
static const char long_gen486[]  = "accept called without having received an op_accept";
static const char short_gen487[] = "**sock_post_close";
static const char long_gen487[]  = "posting a close of the socket failed";
static const char short_gen488[] = "**socket";
static const char long_gen488[]  = "WSASocket failed";
static const char short_gen489[] = "**sock|badbuf";
static const char long_gen489[]  = "the supplied buffer contains invalid memory";
static const char short_gen490[] = "**sock|badhandle";
static const char long_gen490[]  = "sock contains an invalid handle";
static const char short_gen491[] = "**sock|badhdbuf";
static const char long_gen491[]  = "a memory fault occurred while accessing the host description string";
static const char short_gen492[] = "**sock|badhdlen";
static const char long_gen492[]  = "host description string to small to store description";
static const char short_gen493[] = "**sock|badhdmax";
static const char long_gen493[]  = "the length of the host description string must be non-negative";
static const char short_gen494[] = "**sock|badiovn";
static const char long_gen494[]  = "size of iov is invalid";
static const char short_gen495[] = "**sock|badlen";
static const char long_gen495[]  = "bad length parameter(s)";
static const char short_gen496[] = "**sock|badport";
static const char long_gen496[]  = "port number is out of range";
static const char short_gen497[] = "**sock|badsock";
static const char long_gen497[]  = "supplied sock is corrupt";
static const char short_gen498[] = "**sock|closed";
static const char long_gen498[]  = "sock has been closed locally";
static const char short_gen499[] = "**sock|closing";
static const char long_gen499[]  = "sock is in the process of being closed locally";
static const char short_gen500[] = "**sock|connclosed";
static const char long_gen500[]  = "connection closed by peer";
static const char short_gen501[] = "**sock|connfailed";
static const char long_gen501[]  = "connection failure";
static const char short_gen502[] = "**sock|connrefused";
static const char long_gen502[]  = "connection refused";
static const char short_gen503[] = "**sock|getport";
static const char long_gen503[]  = "failed to obtain port number of the listener";
static const char short_gen504[] = "**sock|hostres";
static const char long_gen504[]  = "unable to resolve host name to an address";
static const char short_gen505[] = "**sock|nosock";
static const char long_gen505[]  = "no new sock was available to accept";
static const char short_gen506[] = "**sock|notconnected";
static const char long_gen506[]  = "sock is not connected";
static const char short_gen507[] = "**sock|oserror";
static const char long_gen507[]  = "unknown operating system error";
static const char short_gen508[] = "**sock|osnomem";
static const char long_gen508[]  = "operating system routine failed due to lack of memory";
static const char short_gen509[] = "**sock|poll|accept";
static const char long_gen509[]  = "accept failed to acquire a new socket";
static const char short_gen510[] = "**sock|poll|bind";
static const char long_gen510[]  = "unable to bind socket to port";
static const char short_gen511[] = "**sock|poll|eqfail";
static const char long_gen511[]  = "fatal error: failed to enqueue an event; event was lost";
static const char short_gen512[] = "**sock|poll|eqmalloc";
static const char long_gen512[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen513[] = "**sock|poll|listen";
static const char long_gen513[]  = "listen() failed";
static const char short_gen514[] = "**sock|poll|nodelay";
static const char long_gen514[]  = "unable to set TCP no delay attribute on socket";
static const char short_gen515[] = "**sock|poll|nonblock";
static const char long_gen515[]  = "unable to set socket to nonblocking";
static const char short_gen516[] = "**sock|poll|pipe";
static const char long_gen516[]  = "unable to allocate pipe to wakeup a blocking poll()";
static const char short_gen517[] = "**sock|poll|pipenonblock";
static const char long_gen517[]  = "unable to set wakeup pipe to nonblocking";
static const char short_gen518[] = "**sock|poll|reuseaddr";
static const char long_gen518[]  = "unable to set reuseaddr attribute on socket";
static const char short_gen519[] = "**sock|poll|socket";
static const char long_gen519[]  = "unable to obtain new socket";
static const char short_gen520[] = "**sock|poll|unhandledstate";
static const char long_gen520[]  = "encountered an unhandled";
static const char short_gen521[] = "**sock|reads";
static const char long_gen521[]  = "attempt to perform multiple simultaneous reads";
static const char short_gen522[] = "**sock|setalloc";
static const char long_gen522[]  = "unable to allocate a new sock set object";
static const char short_gen523[] = "**sock|sockalloc";
static const char long_gen523[]  = "unable to allocate a new sock object";
static const char short_gen524[] = "**sock|uninit";
static const char long_gen524[]  = "Sock library has not been initialized";
static const char short_gen525[] = "**sock|writes";
static const char long_gen525[]  = "attempt to perform multiple simultaneous writes";
static const char short_gen526[] = "**spawn";
static const char long_gen526[]  = "Error in spawn call";
static const char short_gen527[] = "**ssmwrite";
static const char long_gen527[]  = "sock/shared memory write failed";
static const char short_gen528[] = "**ssmwritev";
static const char long_gen528[]  = "sock/shared memory writev failed";
static const char short_gen529[] = "**stride";
static const char long_gen529[]  = "Range does not terminate";
static const char short_gen530[] = "**stridezero";
static const char long_gen530[]  = "Zero stride is invalid";
static const char short_gen531[] = "**strncpy";
static const char long_gen531[]  = "insufficient buffer length to complete strncpy";
static const char short_gen532[] = "**success";
static const char long_gen532[]  = "No MPI error";
static const char short_gen533[] = "**tag";
static const char long_gen533[]  = "Invalid tag";
static const char short_gen534[] = "**test_sock_wait";
static const char long_gen534[]  = "sock_wait failed";
static const char short_gen535[] = "**toomanycomm";
static const char long_gen535[]  = "Too many communicators";
static const char short_gen536[] = "**topology";
static const char long_gen536[]  = "Invalid topology";
static const char short_gen537[] = "**topotoolarge";
static const char long_gen537[]  = "Topology size is greater than communicator size";
static const char short_gen538[] = "**truncate";
static const char long_gen538[]  = "Message truncated";
static const char short_gen539[] = "**typematchnoclass";
static const char long_gen539[]  = "The value of typeclass is not one of MPI_TYPECLASS_REAL, MPI_TYPECLASS_INTEGER, or MPI_TYPECLASS_COMPLEX";
static const char short_gen540[] = "**typematchsize";
static const char long_gen540[]  = "No MPI datatype available for the given typeclass and size";
static const char short_gen541[] = "**typenamelen";
static const char long_gen541[]  = " Specified datatype name is too long";
static const char short_gen542[] = "**unknown";
static const char long_gen542[]  = "Unknown error.  Please file a bug report.";
static const char short_gen543[] = "**unsupporteddatarep";
static const char long_gen543[]  = "Only native data representation currently supported";
static const char short_gen544[] = "**vc_state";
static const char long_gen544[]  = "invalid vc state";
static const char short_gen545[] = "**win";
static const char long_gen545[]  = "Invalid MPI_Win";
static const char short_gen546[] = "**winwait";
static const char long_gen546[]  = "WaitForSingleObject failed";
static const char short_gen547[] = "**write_progress";
static const char long_gen547[]  = "Write progress failed";
static const char short_gen548[] = "**wsasock";
static const char long_gen548[]  = "WSAStartup failed";

static const int generic_msgs_len = 549;
static msgpair generic_err_msgs[] = {
{ short_gen0, long_gen0 },
{ short_gen1, long_gen1 },
{ short_gen2, long_gen2 },
{ short_gen3, long_gen3 },
{ short_gen4, long_gen4 },
{ short_gen5, long_gen5 },
{ short_gen6, long_gen6 },
{ short_gen7, long_gen7 },
{ short_gen8, long_gen8 },
{ short_gen9, long_gen9 },
{ short_gen10, long_gen10 },
{ short_gen11, long_gen11 },
{ short_gen12, long_gen12 },
{ short_gen13, long_gen13 },
{ short_gen14, long_gen14 },
{ short_gen15, long_gen15 },
{ short_gen16, long_gen16 },
{ short_gen17, long_gen17 },
{ short_gen18, long_gen18 },
{ short_gen19, long_gen19 },
{ short_gen20, long_gen20 },
{ short_gen21, long_gen21 },
{ short_gen22, long_gen22 },
{ short_gen23, long_gen23 },
{ short_gen24, long_gen24 },
{ short_gen25, long_gen25 },
{ short_gen26, long_gen26 },
{ short_gen27, long_gen27 },
{ short_gen28, long_gen28 },
{ short_gen29, long_gen29 },
{ short_gen30, long_gen30 },
{ short_gen31, long_gen31 },
{ short_gen32, long_gen32 },
{ short_gen33, long_gen33 },
{ short_gen34, long_gen34 },
{ short_gen35, long_gen35 },
{ short_gen36, long_gen36 },
{ short_gen37, long_gen37 },
{ short_gen38, long_gen38 },
{ short_gen39, long_gen39 },
{ short_gen40, long_gen40 },
{ short_gen41, long_gen41 },
{ short_gen42, long_gen42 },
{ short_gen43, long_gen43 },
{ short_gen44, long_gen44 },
{ short_gen45, long_gen45 },
{ short_gen46, long_gen46 },
{ short_gen47, long_gen47 },
{ short_gen48, long_gen48 },
{ short_gen49, long_gen49 },
{ short_gen50, long_gen50 },
{ short_gen51, long_gen51 },
{ short_gen52, long_gen52 },
{ short_gen53, long_gen53 },
{ short_gen54, long_gen54 },
{ short_gen55, long_gen55 },
{ short_gen56, long_gen56 },
{ short_gen57, long_gen57 },
{ short_gen58, long_gen58 },
{ short_gen59, long_gen59 },
{ short_gen60, long_gen60 },
{ short_gen61, long_gen61 },
{ short_gen62, long_gen62 },
{ short_gen63, long_gen63 },
{ short_gen64, long_gen64 },
{ short_gen65, long_gen65 },
{ short_gen66, long_gen66 },
{ short_gen67, long_gen67 },
{ short_gen68, long_gen68 },
{ short_gen69, long_gen69 },
{ short_gen70, long_gen70 },
{ short_gen71, long_gen71 },
{ short_gen72, long_gen72 },
{ short_gen73, long_gen73 },
{ short_gen74, long_gen74 },
{ short_gen75, long_gen75 },
{ short_gen76, long_gen76 },
{ short_gen77, long_gen77 },
{ short_gen78, long_gen78 },
{ short_gen79, long_gen79 },
{ short_gen80, long_gen80 },
{ short_gen81, long_gen81 },
{ short_gen82, long_gen82 },
{ short_gen83, long_gen83 },
{ short_gen84, long_gen84 },
{ short_gen85, long_gen85 },
{ short_gen86, long_gen86 },
{ short_gen87, long_gen87 },
{ short_gen88, long_gen88 },
{ short_gen89, long_gen89 },
{ short_gen90, long_gen90 },
{ short_gen91, long_gen91 },
{ short_gen92, long_gen92 },
{ short_gen93, long_gen93 },
{ short_gen94, long_gen94 },
{ short_gen95, long_gen95 },
{ short_gen96, long_gen96 },
{ short_gen97, long_gen97 },
{ short_gen98, long_gen98 },
{ short_gen99, long_gen99 },
{ short_gen100, long_gen100 },
{ short_gen101, long_gen101 },
{ short_gen102, long_gen102 },
{ short_gen103, long_gen103 },
{ short_gen104, long_gen104 },
{ short_gen105, long_gen105 },
{ short_gen106, long_gen106 },
{ short_gen107, long_gen107 },
{ short_gen108, long_gen108 },
{ short_gen109, long_gen109 },
{ short_gen110, long_gen110 },
{ short_gen111, long_gen111 },
{ short_gen112, long_gen112 },
{ short_gen113, long_gen113 },
{ short_gen114, long_gen114 },
{ short_gen115, long_gen115 },
{ short_gen116, long_gen116 },
{ short_gen117, long_gen117 },
{ short_gen118, long_gen118 },
{ short_gen119, long_gen119 },
{ short_gen120, long_gen120 },
{ short_gen121, long_gen121 },
{ short_gen122, long_gen122 },
{ short_gen123, long_gen123 },
{ short_gen124, long_gen124 },
{ short_gen125, long_gen125 },
{ short_gen126, long_gen126 },
{ short_gen127, long_gen127 },
{ short_gen128, long_gen128 },
{ short_gen129, long_gen129 },
{ short_gen130, long_gen130 },
{ short_gen131, long_gen131 },
{ short_gen132, long_gen132 },
{ short_gen133, long_gen133 },
{ short_gen134, long_gen134 },
{ short_gen135, long_gen135 },
{ short_gen136, long_gen136 },
{ short_gen137, long_gen137 },
{ short_gen138, long_gen138 },
{ short_gen139, long_gen139 },
{ short_gen140, long_gen140 },
{ short_gen141, long_gen141 },
{ short_gen142, long_gen142 },
{ short_gen143, long_gen143 },
{ short_gen144, long_gen144 },
{ short_gen145, long_gen145 },
{ short_gen146, long_gen146 },
{ short_gen147, long_gen147 },
{ short_gen148, long_gen148 },
{ short_gen149, long_gen149 },
{ short_gen150, long_gen150 },
{ short_gen151, long_gen151 },
{ short_gen152, long_gen152 },
{ short_gen153, long_gen153 },
{ short_gen154, long_gen154 },
{ short_gen155, long_gen155 },
{ short_gen156, long_gen156 },
{ short_gen157, long_gen157 },
{ short_gen158, long_gen158 },
{ short_gen159, long_gen159 },
{ short_gen160, long_gen160 },
{ short_gen161, long_gen161 },
{ short_gen162, long_gen162 },
{ short_gen163, long_gen163 },
{ short_gen164, long_gen164 },
{ short_gen165, long_gen165 },
{ short_gen166, long_gen166 },
{ short_gen167, long_gen167 },
{ short_gen168, long_gen168 },
{ short_gen169, long_gen169 },
{ short_gen170, long_gen170 },
{ short_gen171, long_gen171 },
{ short_gen172, long_gen172 },
{ short_gen173, long_gen173 },
{ short_gen174, long_gen174 },
{ short_gen175, long_gen175 },
{ short_gen176, long_gen176 },
{ short_gen177, long_gen177 },
{ short_gen178, long_gen178 },
{ short_gen179, long_gen179 },
{ short_gen180, long_gen180 },
{ short_gen181, long_gen181 },
{ short_gen182, long_gen182 },
{ short_gen183, long_gen183 },
{ short_gen184, long_gen184 },
{ short_gen185, long_gen185 },
{ short_gen186, long_gen186 },
{ short_gen187, long_gen187 },
{ short_gen188, long_gen188 },
{ short_gen189, long_gen189 },
{ short_gen190, long_gen190 },
{ short_gen191, long_gen191 },
{ short_gen192, long_gen192 },
{ short_gen193, long_gen193 },
{ short_gen194, long_gen194 },
{ short_gen195, long_gen195 },
{ short_gen196, long_gen196 },
{ short_gen197, long_gen197 },
{ short_gen198, long_gen198 },
{ short_gen199, long_gen199 },
{ short_gen200, long_gen200 },
{ short_gen201, long_gen201 },
{ short_gen202, long_gen202 },
{ short_gen203, long_gen203 },
{ short_gen204, long_gen204 },
{ short_gen205, long_gen205 },
{ short_gen206, long_gen206 },
{ short_gen207, long_gen207 },
{ short_gen208, long_gen208 },
{ short_gen209, long_gen209 },
{ short_gen210, long_gen210 },
{ short_gen211, long_gen211 },
{ short_gen212, long_gen212 },
{ short_gen213, long_gen213 },
{ short_gen214, long_gen214 },
{ short_gen215, long_gen215 },
{ short_gen216, long_gen216 },
{ short_gen217, long_gen217 },
{ short_gen218, long_gen218 },
{ short_gen219, long_gen219 },
{ short_gen220, long_gen220 },
{ short_gen221, long_gen221 },
{ short_gen222, long_gen222 },
{ short_gen223, long_gen223 },
{ short_gen224, long_gen224 },
{ short_gen225, long_gen225 },
{ short_gen226, long_gen226 },
{ short_gen227, long_gen227 },
{ short_gen228, long_gen228 },
{ short_gen229, long_gen229 },
{ short_gen230, long_gen230 },
{ short_gen231, long_gen231 },
{ short_gen232, long_gen232 },
{ short_gen233, long_gen233 },
{ short_gen234, long_gen234 },
{ short_gen235, long_gen235 },
{ short_gen236, long_gen236 },
{ short_gen237, long_gen237 },
{ short_gen238, long_gen238 },
{ short_gen239, long_gen239 },
{ short_gen240, long_gen240 },
{ short_gen241, long_gen241 },
{ short_gen242, long_gen242 },
{ short_gen243, long_gen243 },
{ short_gen244, long_gen244 },
{ short_gen245, long_gen245 },
{ short_gen246, long_gen246 },
{ short_gen247, long_gen247 },
{ short_gen248, long_gen248 },
{ short_gen249, long_gen249 },
{ short_gen250, long_gen250 },
{ short_gen251, long_gen251 },
{ short_gen252, long_gen252 },
{ short_gen253, long_gen253 },
{ short_gen254, long_gen254 },
{ short_gen255, long_gen255 },
{ short_gen256, long_gen256 },
{ short_gen257, long_gen257 },
{ short_gen258, long_gen258 },
{ short_gen259, long_gen259 },
{ short_gen260, long_gen260 },
{ short_gen261, long_gen261 },
{ short_gen262, long_gen262 },
{ short_gen263, long_gen263 },
{ short_gen264, long_gen264 },
{ short_gen265, long_gen265 },
{ short_gen266, long_gen266 },
{ short_gen267, long_gen267 },
{ short_gen268, long_gen268 },
{ short_gen269, long_gen269 },
{ short_gen270, long_gen270 },
{ short_gen271, long_gen271 },
{ short_gen272, long_gen272 },
{ short_gen273, long_gen273 },
{ short_gen274, long_gen274 },
{ short_gen275, long_gen275 },
{ short_gen276, long_gen276 },
{ short_gen277, long_gen277 },
{ short_gen278, long_gen278 },
{ short_gen279, long_gen279 },
{ short_gen280, long_gen280 },
{ short_gen281, long_gen281 },
{ short_gen282, long_gen282 },
{ short_gen283, long_gen283 },
{ short_gen284, long_gen284 },
{ short_gen285, long_gen285 },
{ short_gen286, long_gen286 },
{ short_gen287, long_gen287 },
{ short_gen288, long_gen288 },
{ short_gen289, long_gen289 },
{ short_gen290, long_gen290 },
{ short_gen291, long_gen291 },
{ short_gen292, long_gen292 },
{ short_gen293, long_gen293 },
{ short_gen294, long_gen294 },
{ short_gen295, long_gen295 },
{ short_gen296, long_gen296 },
{ short_gen297, long_gen297 },
{ short_gen298, long_gen298 },
{ short_gen299, long_gen299 },
{ short_gen300, long_gen300 },
{ short_gen301, long_gen301 },
{ short_gen302, long_gen302 },
{ short_gen303, long_gen303 },
{ short_gen304, long_gen304 },
{ short_gen305, long_gen305 },
{ short_gen306, long_gen306 },
{ short_gen307, long_gen307 },
{ short_gen308, long_gen308 },
{ short_gen309, long_gen309 },
{ short_gen310, long_gen310 },
{ short_gen311, long_gen311 },
{ short_gen312, long_gen312 },
{ short_gen313, long_gen313 },
{ short_gen314, long_gen314 },
{ short_gen315, long_gen315 },
{ short_gen316, long_gen316 },
{ short_gen317, long_gen317 },
{ short_gen318, long_gen318 },
{ short_gen319, long_gen319 },
{ short_gen320, long_gen320 },
{ short_gen321, long_gen321 },
{ short_gen322, long_gen322 },
{ short_gen323, long_gen323 },
{ short_gen324, long_gen324 },
{ short_gen325, long_gen325 },
{ short_gen326, long_gen326 },
{ short_gen327, long_gen327 },
{ short_gen328, long_gen328 },
{ short_gen329, long_gen329 },
{ short_gen330, long_gen330 },
{ short_gen331, long_gen331 },
{ short_gen332, long_gen332 },
{ short_gen333, long_gen333 },
{ short_gen334, long_gen334 },
{ short_gen335, long_gen335 },
{ short_gen336, long_gen336 },
{ short_gen337, long_gen337 },
{ short_gen338, long_gen338 },
{ short_gen339, long_gen339 },
{ short_gen340, long_gen340 },
{ short_gen341, long_gen341 },
{ short_gen342, long_gen342 },
{ short_gen343, long_gen343 },
{ short_gen344, long_gen344 },
{ short_gen345, long_gen345 },
{ short_gen346, long_gen346 },
{ short_gen347, long_gen347 },
{ short_gen348, long_gen348 },
{ short_gen349, long_gen349 },
{ short_gen350, long_gen350 },
{ short_gen351, long_gen351 },
{ short_gen352, long_gen352 },
{ short_gen353, long_gen353 },
{ short_gen354, long_gen354 },
{ short_gen355, long_gen355 },
{ short_gen356, long_gen356 },
{ short_gen357, long_gen357 },
{ short_gen358, long_gen358 },
{ short_gen359, long_gen359 },
{ short_gen360, long_gen360 },
{ short_gen361, long_gen361 },
{ short_gen362, long_gen362 },
{ short_gen363, long_gen363 },
{ short_gen364, long_gen364 },
{ short_gen365, long_gen365 },
{ short_gen366, long_gen366 },
{ short_gen367, long_gen367 },
{ short_gen368, long_gen368 },
{ short_gen369, long_gen369 },
{ short_gen370, long_gen370 },
{ short_gen371, long_gen371 },
{ short_gen372, long_gen372 },
{ short_gen373, long_gen373 },
{ short_gen374, long_gen374 },
{ short_gen375, long_gen375 },
{ short_gen376, long_gen376 },
{ short_gen377, long_gen377 },
{ short_gen378, long_gen378 },
{ short_gen379, long_gen379 },
{ short_gen380, long_gen380 },
{ short_gen381, long_gen381 },
{ short_gen382, long_gen382 },
{ short_gen383, long_gen383 },
{ short_gen384, long_gen384 },
{ short_gen385, long_gen385 },
{ short_gen386, long_gen386 },
{ short_gen387, long_gen387 },
{ short_gen388, long_gen388 },
{ short_gen389, long_gen389 },
{ short_gen390, long_gen390 },
{ short_gen391, long_gen391 },
{ short_gen392, long_gen392 },
{ short_gen393, long_gen393 },
{ short_gen394, long_gen394 },
{ short_gen395, long_gen395 },
{ short_gen396, long_gen396 },
{ short_gen397, long_gen397 },
{ short_gen398, long_gen398 },
{ short_gen399, long_gen399 },
{ short_gen400, long_gen400 },
{ short_gen401, long_gen401 },
{ short_gen402, long_gen402 },
{ short_gen403, long_gen403 },
{ short_gen404, long_gen404 },
{ short_gen405, long_gen405 },
{ short_gen406, long_gen406 },
{ short_gen407, long_gen407 },
{ short_gen408, long_gen408 },
{ short_gen409, long_gen409 },
{ short_gen410, long_gen410 },
{ short_gen411, long_gen411 },
{ short_gen412, long_gen412 },
{ short_gen413, long_gen413 },
{ short_gen414, long_gen414 },
{ short_gen415, long_gen415 },
{ short_gen416, long_gen416 },
{ short_gen417, long_gen417 },
{ short_gen418, long_gen418 },
{ short_gen419, long_gen419 },
{ short_gen420, long_gen420 },
{ short_gen421, long_gen421 },
{ short_gen422, long_gen422 },
{ short_gen423, long_gen423 },
{ short_gen424, long_gen424 },
{ short_gen425, long_gen425 },
{ short_gen426, long_gen426 },
{ short_gen427, long_gen427 },
{ short_gen428, long_gen428 },
{ short_gen429, long_gen429 },
{ short_gen430, long_gen430 },
{ short_gen431, long_gen431 },
{ short_gen432, long_gen432 },
{ short_gen433, long_gen433 },
{ short_gen434, long_gen434 },
{ short_gen435, long_gen435 },
{ short_gen436, long_gen436 },
{ short_gen437, long_gen437 },
{ short_gen438, long_gen438 },
{ short_gen439, long_gen439 },
{ short_gen440, long_gen440 },
{ short_gen441, long_gen441 },
{ short_gen442, long_gen442 },
{ short_gen443, long_gen443 },
{ short_gen444, long_gen444 },
{ short_gen445, long_gen445 },
{ short_gen446, long_gen446 },
{ short_gen447, long_gen447 },
{ short_gen448, long_gen448 },
{ short_gen449, long_gen449 },
{ short_gen450, long_gen450 },
{ short_gen451, long_gen451 },
{ short_gen452, long_gen452 },
{ short_gen453, long_gen453 },
{ short_gen454, long_gen454 },
{ short_gen455, long_gen455 },
{ short_gen456, long_gen456 },
{ short_gen457, long_gen457 },
{ short_gen458, long_gen458 },
{ short_gen459, long_gen459 },
{ short_gen460, long_gen460 },
{ short_gen461, long_gen461 },
{ short_gen462, long_gen462 },
{ short_gen463, long_gen463 },
{ short_gen464, long_gen464 },
{ short_gen465, long_gen465 },
{ short_gen466, long_gen466 },
{ short_gen467, long_gen467 },
{ short_gen468, long_gen468 },
{ short_gen469, long_gen469 },
{ short_gen470, long_gen470 },
{ short_gen471, long_gen471 },
{ short_gen472, long_gen472 },
{ short_gen473, long_gen473 },
{ short_gen474, long_gen474 },
{ short_gen475, long_gen475 },
{ short_gen476, long_gen476 },
{ short_gen477, long_gen477 },
{ short_gen478, long_gen478 },
{ short_gen479, long_gen479 },
{ short_gen480, long_gen480 },
{ short_gen481, long_gen481 },
{ short_gen482, long_gen482 },
{ short_gen483, long_gen483 },
{ short_gen484, long_gen484 },
{ short_gen485, long_gen485 },
{ short_gen486, long_gen486 },
{ short_gen487, long_gen487 },
{ short_gen488, long_gen488 },
{ short_gen489, long_gen489 },
{ short_gen490, long_gen490 },
{ short_gen491, long_gen491 },
{ short_gen492, long_gen492 },
{ short_gen493, long_gen493 },
{ short_gen494, long_gen494 },
{ short_gen495, long_gen495 },
{ short_gen496, long_gen496 },
{ short_gen497, long_gen497 },
{ short_gen498, long_gen498 },
{ short_gen499, long_gen499 },
{ short_gen500, long_gen500 },
{ short_gen501, long_gen501 },
{ short_gen502, long_gen502 },
{ short_gen503, long_gen503 },
{ short_gen504, long_gen504 },
{ short_gen505, long_gen505 },
{ short_gen506, long_gen506 },
{ short_gen507, long_gen507 },
{ short_gen508, long_gen508 },
{ short_gen509, long_gen509 },
{ short_gen510, long_gen510 },
{ short_gen511, long_gen511 },
{ short_gen512, long_gen512 },
{ short_gen513, long_gen513 },
{ short_gen514, long_gen514 },
{ short_gen515, long_gen515 },
{ short_gen516, long_gen516 },
{ short_gen517, long_gen517 },
{ short_gen518, long_gen518 },
{ short_gen519, long_gen519 },
{ short_gen520, long_gen520 },
{ short_gen521, long_gen521 },
{ short_gen522, long_gen522 },
{ short_gen523, long_gen523 },
{ short_gen524, long_gen524 },
{ short_gen525, long_gen525 },
{ short_gen526, long_gen526 },
{ short_gen527, long_gen527 },
{ short_gen528, long_gen528 },
{ short_gen529, long_gen529 },
{ short_gen530, long_gen530 },
{ short_gen531, long_gen531 },
{ short_gen532, long_gen532 },
{ short_gen533, long_gen533 },
{ short_gen534, long_gen534 },
{ short_gen535, long_gen535 },
{ short_gen536, long_gen536 },
{ short_gen537, long_gen537 },
{ short_gen538, long_gen538 },
{ short_gen539, long_gen539 },
{ short_gen540, long_gen540 },
{ short_gen541, long_gen541 },
{ short_gen542, long_gen542 },
{ short_gen543, long_gen543 },
{ short_gen544, long_gen544 },
{ short_gen545, long_gen545 },
{ short_gen546, long_gen546 },
{ short_gen547, long_gen547 },
{ short_gen548, long_gen548 }
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
static const char short_spc21[] = "**ca %d";
static const char long_spc21[]  = "invalid completion action (%d)";
static const char short_spc22[] = "**ca_guids %s";
static const char long_spc22[]  = "unable to get the infiniband guids - %s";
static const char short_spc23[] = "**cartcoordinvalid %d %d %d";
static const char long_spc23[]  = " Cartesian coordinate for the %d coordinate is %d but must be between 0 and %d";
static const char short_spc24[] = "**cartdim %d %d";
static const char long_spc24[]  = "Size of the communicator (%d) is smaller than the size of the Cartesian topology (%d)";
static const char short_spc25[] = "**ch3|badca %d";
static const char long_spc25[]  = "specified completion action in not known (%d)";
static const char short_spc26[] = "**ch3|badmsgtype %d";
static const char long_spc26[]  = "request contained an invalid message type (%d)";
static const char short_spc27[] = "**ch3|badreqtype %d";
static const char long_spc27[]  = "request contained an invalid request type (%d)";
static const char short_spc28[] = "**ch3|loadrecviov %s";
static const char long_spc28[]  = "failure occurred while loading the receive I/O vector (%s)";
static const char short_spc29[] = "**ch3|postrecv %s";
static const char long_spc29[]  = "failure occurred while posting a receive for message data (%s)";
static const char short_spc30[] = "**ch3|recvdata %s";
static const char long_spc30[]  = "failure occurred while attempting to receive message data (%s)";
static const char short_spc31[] = "**ch3|sock|badbuscard %s";
static const char long_spc31[]  = "[ch3:sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc32[] = "**ch3|sock|badpacket %d";
static const char long_spc32[]  = "[ch3:sock] received packet of unknown type (%d)";
static const char short_spc33[] = "**ch3|sock|connfailed %d %d";
static const char long_spc33[]  = "[ch3:sock] failed to connnect to remote process %d:%d";
static const char short_spc34[] = "**ch3|sock|connrefused %s %d %s";
static const char long_spc34[]  = "[ch3:sock] failed to connect to process %s:%d (%s)";
static const char short_spc35[] = "**ch3|sock|failure %d";
static const char long_spc35[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc36[] = "**ch3|sock|hostlookup %s %d %s";
static const char long_spc36[]  = "[ch3:sock] failed to obtain host information for process %s:%d (%s)";
static const char short_spc37[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc37[]  = "PMI_Finalize failed, error %d";
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
static const char short_spc43[] = "**dims %d";
static const char long_spc43[]  = "Invalid dimension argument (value is %d)";
static const char short_spc44[] = "**dimsmany %d %d";
static const char long_spc44[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc45[] = "**duphandle %d";
static const char long_spc45[]  = "unable to duplicate a handle (errno %d)";
static const char short_spc46[] = "**dupprocesses %d";
static const char long_spc46[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes; both contain process %d";
static const char short_spc47[] = "**edgeoutrange %d %d %d";
static const char long_spc47[]  = "Edge index edges[%d] is %d but must be nonnegative and less than %d";
static const char short_spc48[] = "**fail %d";
static const char long_spc48[]  = "failure (errno %d)";
static const char short_spc49[] = "**fileaccess %s";
static const char long_spc49[]  = "Access denied to file %s";
static const char short_spc50[] = "**filenamedir %s";
static const char long_spc50[]  = "Invalid or missing directory %s";
static const char short_spc51[] = "**filenamelong %s %d";
static const char long_spc51[]  = "Pathname %s too long (%d characters)";
static const char short_spc52[] = "**filenoexist %s";
static const char long_spc52[]  = "File %s does not exist";
static const char short_spc53[] = "**filerdonly %s";
static const char long_spc53[]  = "Read-only file or filesystem name %s";
static const char short_spc54[] = "**ftok %s %d %d";
static const char long_spc54[]  = "failed to create a sysv key from the file '%s' and id %d, error %d";
static const char short_spc55[] = "**ftruncate %s %d %d";
static const char long_spc55[]  = "unable to resize the shared memory object %s to size %d (errno %d)";
static const char short_spc56[] = "**gethostbyname %d";
static const char long_spc56[]  = "gethostbyname failed (errno %d)";
static const char short_spc57[] = "**getinfo %d";
static const char long_spc57[]  = "getaddrinfo failed (errno %d)";
static const char short_spc58[] = "**groupnotincomm %d";
static const char long_spc58[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc59[] = "**hostlookup %d %d %s";
static const char long_spc59[]  = "Host lookup failed for process group %d, rank %d, business card <%s>";
static const char short_spc60[] = "**ibu_op %d";
static const char long_spc60[]  = "invalid infiniband operation (%d)";
static const char short_spc61[] = "**ibu_wait %d";
static const char long_spc61[]  = "ibu_wait failed (errno %d)";
static const char short_spc62[] = "**indexneg %d %d";
static const char long_spc62[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc63[] = "**indexnonmonotone %d %d %d";
static const char long_spc63[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc64[] = "**infonkey %d %d";
static const char long_spc64[]  = "Requested key %d but this MPI_Info only has %d keys";
static const char short_spc65[] = "**infonokey %s";
static const char long_spc65[]  = "MPI_Info key %s is not defined ";
static const char short_spc66[] = "**intern %s";
static const char long_spc66[]  = "Internal MPI error!  %s";
static const char short_spc67[] = "**invalid_handle %d";
static const char long_spc67[]  = "invalid handle (%d)";
static const char short_spc68[] = "**invalid_listener %p";
static const char long_spc68[]  = "invalid listener (%p)";
static const char short_spc69[] = "**invalid_refcount %d";
static const char long_spc69[]  = "invalid reference count (%d)";
static const char short_spc70[] = "**io %s";
static const char long_spc70[]  = "Other I/O error %s";
static const char short_spc71[] = "**iocp %d";
static const char long_spc71[]  = "unable to create an I/O completion port (errno %d)";
static const char short_spc72[] = "**iov_offset %d %d";
static const char long_spc72[]  = "invalid iov offset (%d > %d)";
static const char short_spc73[] = "**listen %d";
static const char long_spc73[]  = "listen failed (errno %d)";
static const char short_spc74[] = "**mmap %d";
static const char long_spc74[]  = " unable to map memory, mmap failed (errno %d)";
static const char short_spc75[] = "**mpi_accumulate %p %d %D %d %d %d %D %O %W";
static const char long_spc75[]  = "MPI_Accumulate(%p, %d, %D, %d, %d, %d, %D, %O, %W) failed";
static const char short_spc76[] = "**mpi_add_error_string %d %s";
static const char long_spc76[]  = "MPI_Add_error_string(%d, %s) failed";
static const char short_spc77[] = "**mpi_allgather %p %d %D %p %d %D %C";
static const char long_spc77[]  = "MPI_Allgather(%p, %d, %D, %p, %d, %D, %C) failed";
static const char short_spc78[] = "**mpi_allgatherv %p %d %D %p %p %p %D %C";
static const char long_spc78[]  = "MPI_Allgatherv(%p, %d, %D, %p, %p, %p, %D, %C) failed";
static const char short_spc79[] = "**mpi_allreduce %p %p %d %D %O %C";
static const char long_spc79[]  = "MPI_Allreduce(%p, %p, %d, %D, %O, %C) failed";
static const char short_spc80[] = "**mpi_alltoall %p %d %D %p %d %D %C";
static const char long_spc80[]  = "MPI_Alltoall(%p, %d, %D, %p, %d, %D, %C) failed";
static const char short_spc81[] = "**mpi_alltoallv %p %p %p %D %p %p %p %D %C";
static const char long_spc81[]  = "MPI_Alltoallv(%p, %p, %p, %D, %p, %p, %p, %D, %C) failed";
static const char short_spc82[] = "**mpi_alltoallw %p %p %p %p %p %p %p %p %C";
static const char long_spc82[]  = "MPI_Alltoallw(%p, %p, %p, %p, %p, %p, %p, %p, %C) failed";
static const char short_spc83[] = "**mpi_attr_delete %C %d";
static const char long_spc83[]  = "MPI_Attr_delete(%C, %d) failed";
static const char short_spc84[] = "**mpi_attr_get %C %d %p %p";
static const char long_spc84[]  = "MPI_Atr_get(%C, %d, %p, %p) failed";
static const char short_spc85[] = "**mpi_attr_put %C %d %p";
static const char long_spc85[]  = "MPI_Attr_put(%C, %d, %p) failed";
static const char short_spc86[] = "**mpi_barrier %C";
static const char long_spc86[]  = "MPI_Barrier(%C) failed";
static const char short_spc87[] = "**mpi_bcast %p %d %D %d %C";
static const char long_spc87[]  = "MPI_Bcast(%p, %d, %D, %d, %C) failed";
static const char short_spc88[] = "**mpi_bsend %p %d %D %d %d %C";
static const char long_spc88[]  = "MPI_Bsend(%p, %d, %D, %d, %d, %C) failed";
static const char short_spc89[] = "**mpi_bsend_init %p %d %D %d %d %C %p";
static const char long_spc89[]  = "MPI_Bsend_init(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc90[] = "**mpi_buffer_attach %p %d";
static const char long_spc90[]  = "MPI_Buffer_attach(%p, %d) failed";
static const char short_spc91[] = "**mpi_buffer_detach %p %p";
static const char long_spc91[]  = "MPI_Buffer_detach(%p, %p) failed";
static const char short_spc92[] = "**mpi_cancel %p";
static const char long_spc92[]  = "MPI_Cancel(%p) failed";
static const char short_spc93[] = "**mpi_cart_create %C %d %p %p %d %p";
static const char long_spc93[]  = "MPI_Cart_create(%C, %d, %p, %p, %d, %p) failed";
static const char short_spc94[] = "**mpi_close_port %s";
static const char long_spc94[]  = "MPI_Close_port(%s) failed";
static const char short_spc95[] = "**mpi_comm_accept %s %I %d %C %p";
static const char long_spc95[]  = "MPI_Comm_accept(%s, %I, %d, %C, %p) failed";
static const char short_spc96[] = "**mpi_comm_connect %s %I %d %C %p";
static const char long_spc96[]  = "MPI_Comm_connect(%s, %I, %d, %C, %p) failed";
static const char short_spc97[] = "**mpi_comm_disconnect %p";
static const char long_spc97[]  = "MPI_Comm_disconnect(%p) failed";
static const char short_spc98[] = "**mpi_comm_dup %C %p";
static const char long_spc98[]  = "MPI_Comm_dup(%C, %p) failed";
static const char short_spc99[] = "**mpi_comm_free %p";
static const char long_spc99[]  = "MPI_Comm_free(%p) failed";
static const char short_spc100[] = "**mpi_comm_group %C %p";
static const char long_spc100[]  = "MPI_Comm_group(%C, %p) failed";
static const char short_spc101[] = "**mpi_comm_join %d %p";
static const char long_spc101[]  = "MPI_Comm_join(%d, %p) failed";
static const char short_spc102[] = "**mpi_comm_remote_group %C %p";
static const char long_spc102[]  = "MPI_Comm_remote_group(%C, %p) failed";
static const char short_spc103[] = "**mpi_comm_set_attr %C %d %p";
static const char long_spc103[]  = "MPI_Comm_set_attr(%C, %d, %p) failed";
static const char short_spc104[] = "**mpi_comm_spawn %s %p %d %I %d %C %p %p";
static const char long_spc104[]  = "MPI_Comm_spawn(%s, %p, %d, %I, %d, %C, %p, %p) failed";
static const char short_spc105[] = "**mpi_comm_spawn_multiple %d %p %p %p %p %d %C %p %p";
static const char long_spc105[]  = "MPI_Comm_spawn_multiple(%d, %p, %p, %p, %p, %d, %C, %p, %p) failed";
static const char short_spc106[] = "**mpi_comm_split %C %d %d %p";
static const char long_spc106[]  = "MPI_Comm_split(%C, %d, %d, %p) failed";
static const char short_spc107[] = "**mpi_errhandler_create %p %p";
static const char long_spc107[]  = "MPI_Errhandler_create(%p) failed";
static const char short_spc108[] = "**mpi_errhandler_get %C %p";
static const char long_spc108[]  = "MPI_Errhandler_get(%C, %p) failed";
static const char short_spc109[] = "**mpi_errhandler_set %C %E";
static const char long_spc109[]  = "MPI_Errhandler_set(%C, %E) failed";
static const char short_spc110[] = "**mpi_exscan %p %p %d %D %O %C";
static const char long_spc110[]  = "MPI_Exscan(%p, %p, %d, %D, %O, %C) failed";
static const char short_spc111[] = "**mpi_file_create_errhandler %p %p";
static const char long_spc111[]  = "MPI_File_create_errhandler(%p, %p) failed";
static const char short_spc112[] = "**mpi_gather %p %d %D %p %d %D %d %C";
static const char long_spc112[]  = "MPI_Gather(%p, %d, %D, %p, %d, %D, %d, %C) failed";
static const char short_spc113[] = "**mpi_gatherv %p %d %D %p %p %p %D %d %C";
static const char long_spc113[]  = "MPI_Gatherv failed(%p, %d, %D, %p, %p, %p, %D, %d, %C) failed";
static const char short_spc114[] = "**mpi_get %p %d %D %d %d %d %D %W";
static const char long_spc114[]  = "MPI_Get(%p, %d, %D, %d, %d, %d, %D, %W) failed";
static const char short_spc115[] = "**mpi_get_processor_name %p %p";
static const char long_spc115[]  = "MPI_Get_processor_name(%p, %p) failed";
static const char short_spc116[] = "**mpi_graph_create %C %d %p %p %d %p";
static const char long_spc116[]  = "MPI_Graph_create(%C, %d, %p, %p, %d, %p) failed";
static const char short_spc117[] = "**mpi_graph_map %C %d %p %p %p";
static const char long_spc117[]  = "MPI_Graph_map(%C, %d, %p, %p, %p) failed";
static const char short_spc118[] = "**mpi_grequest_start %p %p %p %p %p";
static const char long_spc118[]  = "MPI_Grequest_start(%p, %p, %p, %p, %p) failed";
static const char short_spc119[] = "**mpi_group_difference %G %G %p";
static const char long_spc119[]  = "MPI_Group_difference(%G, %G, %p) failed";
static const char short_spc120[] = "**mpi_group_excl %G %d %p %p";
static const char long_spc120[]  = "MPI_Group_excl(%G, %d, %p, %p) failed";
static const char short_spc121[] = "**mpi_group_free %p";
static const char long_spc121[]  = "MPI_Group_free(%p) failed";
static const char short_spc122[] = "**mpi_group_incl %G %d %p %p";
static const char long_spc122[]  = "MPI_Group_incl(%G, %d, %p, %p) failed";
static const char short_spc123[] = "**mpi_group_intersection %G %G %p";
static const char long_spc123[]  = "MPI_Group_intersection(%G, %G, %p) failed";
static const char short_spc124[] = "**mpi_group_range_excl %G %d %p %p";
static const char long_spc124[]  = "MPI_Group_range_excl(%G, %d, %p, %p) failed";
static const char short_spc125[] = "**mpi_group_range_incl %G %d %p %p";
static const char long_spc125[]  = "MPI_Group_range_incl(%G, %d, %p, %p) failed";
static const char short_spc126[] = "**mpi_group_union %G %G %p";
static const char long_spc126[]  = "MPI_Group_union(%G, %G, %p) failed";
static const char short_spc127[] = "**mpi_ibsend %p %d %D %d %d %C %p";
static const char long_spc127[]  = "MPI_Ibsend(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc128[] = "**mpi_info_create %p";
static const char long_spc128[]  = "MPI_Info_create(%p) failed";
static const char short_spc129[] = "**mpi_info_delete %I %p";
static const char long_spc129[]  = "MPI_Info_delete(%I, %p) failed";
static const char short_spc130[] = "**mpi_info_dup %I %p";
static const char long_spc130[]  = "MPI_Info_dup(%I, %p) failed";
static const char short_spc131[] = "**mpi_info_get_nthkey %I %d %p";
static const char long_spc131[]  = "MPI_Info_get_nthkey(%I, %d, %p) failed";
static const char short_spc132[] = "**mpi_info_set %I %p %p";
static const char long_spc132[]  = "MPI_Info_set(%I, %p, %p) failed";
static const char short_spc133[] = "**mpi_init %p %p";
static const char long_spc133[]  = "MPI_Init(%p, %p) failed";
static const char short_spc134[] = "**mpi_init_thread %p %p %d %p";
static const char long_spc134[]  = "MPI_Init_thread(%p, %p, %d, %p)";
static const char short_spc135[] = "**mpi_intercomm_create %C %d %C %d %d %p";
static const char long_spc135[]  = "MPI_Intercomm_create(%C, %d, %C, %d, %d, %p) failed";
static const char short_spc136[] = "**mpi_intercomm_merge %C %d %p";
static const char long_spc136[]  = "MPI_Intercomm_merge(%C, %d, %p) failed";
static const char short_spc137[] = "**mpi_iprobe %d %d %C %p %p";
static const char long_spc137[]  = "MPI_Iprobe(%d, %d, %C, %p, %p) failed";
static const char short_spc138[] = "**mpi_irecv %p %d %D %d %d %C %p";
static const char long_spc138[]  = "MPI_Irecv(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc139[] = "**mpi_irsend %p %d %D %d %d %C %p";
static const char long_spc139[]  = "MPI_Irsend(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc140[] = "**mpi_isend %p %d %D %d %d %C %p";
static const char long_spc140[]  = "MPI_Isend(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc141[] = "**mpi_issend %p %d %D %d %d %C %p";
static const char long_spc141[]  = "MPI_Issend(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc142[] = "**mpi_keyval_create %p %p %p %p";
static const char long_spc142[]  = "MPI_Keyval_create(%p, %p, %p, %p) failed";
static const char short_spc143[] = "**mpi_keyval_free %p";
static const char long_spc143[]  = "MPI_Keyval_free(%p) failed";
static const char short_spc144[] = "**mpi_lookup_name %s %I %p";
static const char long_spc144[]  = "MPI_Lookup_name(%s, %I, %p) failed";
static const char short_spc145[] = "**mpi_op_create %p %d %p";
static const char long_spc145[]  = "MPI_Op_create(%p, %d, %p) failed";
static const char short_spc146[] = "**mpi_open_port %I %s";
static const char long_spc146[]  = "MPI_Open_port(%I, %s) failed";
static const char short_spc147[] = "**mpi_pack %p %d %D %p %d %p %C";
static const char long_spc147[]  = "MPI_Pack(%p, %d, %D, %p, %d, %p, %C) failed";
static const char short_spc148[] = "**mpi_pack_external %s %p %d %D %p %d %p";
static const char long_spc148[]  = "MPI_Pack_external(%s, %p, %d, %D, %p, %d, %p) failed";
static const char short_spc149[] = "**mpi_probe %d %d %C %p";
static const char long_spc149[]  = "MPI_Probe(%d, %d, %C, %p) failed";
static const char short_spc150[] = "**mpi_publish_name %s %I %s";
static const char long_spc150[]  = "MPI_Publish_name(%s, %I, %s) failed";
static const char short_spc151[] = "**mpi_put %p %d %D %d %d %d %D %W";
static const char long_spc151[]  = "MPI_Put(%p, %d, %D, %d, %d, %d, %D, %W) failed";
static const char short_spc152[] = "**mpi_recv %p %d %D %d %d %C %p";
static const char long_spc152[]  = "MPI_Recv(%p, %d %D, %d, %C, %p) failed";
static const char short_spc153[] = "**mpi_recv_init %p %d %D %d %d %C %p";
static const char long_spc153[]  = "MPI_Recv_init(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc154[] = "**mpi_reduce %p %p %d %D %O %d %C";
static const char long_spc154[]  = "MPI_Reduce(%p, %p, %d, %D, %O, %d, %C) failed";
static const char short_spc155[] = "**mpi_reduce_scatter %p %p %p %D %O %C";
static const char long_spc155[]  = "MPI_Reduce_scatter(%p, %p, %p, %D, %O, %C) failed";
static const char short_spc156[] = "**mpi_register_datarep %s %p %p %p %p";
static const char long_spc156[]  = "MPI_Register_Datarep(%s %p %p %p %p) failed";
static const char short_spc157[] = "**mpi_request_free %p";
static const char long_spc157[]  = "MPI_Request_free(%p) failed";
static const char short_spc158[] = "**mpi_request_get_status %R %p %p";
static const char long_spc158[]  = "MPI_Request_get_status(%R, %p, %p) failed";
static const char short_spc159[] = "**mpi_rsend %p %d %D %d %d %C";
static const char long_spc159[]  = "MPI_Rsend(%p, %d, %D, %d, %d, %C) failed";
static const char short_spc160[] = "**mpi_rsend_init %p %d %D %d %d %C %p";
static const char long_spc160[]  = "MPI_Rsend_init(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc161[] = "**mpi_scan %p %p %d %D %O %C";
static const char long_spc161[]  = "MPI_Scan(%p, %p, %d, %D, %O, %C) failed";
static const char short_spc162[] = "**mpi_scatter %p %d %D %p %d %D %d %C";
static const char long_spc162[]  = "MPI_Scatter(%p, %d, %D, %p, %d, %D, %d, %C) failed";
static const char short_spc163[] = "**mpi_scatterv %p %p %p %D %p %d %D %d %C";
static const char long_spc163[]  = "MPI_Scatterv(%p, %p, %p, %D, %p, %d, %D, %d, %C) failed";
static const char short_spc164[] = "**mpi_send %p %d %D %d %d %C";
static const char long_spc164[]  = "MPI_Send(%p, %d, %D, %d, %d, %C) failed";
static const char short_spc165[] = "**mpi_send_init %p %d %D %d %d %C %p";
static const char long_spc165[]  = "MPI_Send_init(%p %d %D %d %d %C %p) failed";
static const char short_spc166[] = "**mpi_sendrecv %p %d %D %d %d %p %d %D %d %d %C %p";
static const char long_spc166[]  = "MPI_Sendrecv(%p, %d, %D, %d, %d, %p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc167[] = "**mpi_sendrecv_replace %p %d %D %d %d %d %d %C %p";
static const char long_spc167[]  = "MPI_Sendrecv_replace(%p, %d, %D, %d, %d, %d, %d, %C, %p) failed";
static const char short_spc168[] = "**mpi_ssend %p %d %D %d %d %C";
static const char long_spc168[]  = "MPI_Ssend(%p, %d, %D, %d, %d, %C) failed";
static const char short_spc169[] = "**mpi_ssend_init %p %d %D %d %d %C %p";
static const char long_spc169[]  = "MPI_Ssend_init(%p, %d, %D, %d, %d, %C, %p) failed";
static const char short_spc170[] = "**mpi_start %p";
static const char long_spc170[]  = "MPI_Start(%p) failed";
static const char short_spc171[] = "**mpi_startall %d %p";
static const char long_spc171[]  = "MPI_Startall(%d, %p) failed";
static const char short_spc172[] = "**mpi_test %p %p %p";
static const char long_spc172[]  = "MPI_Test(%p, %p, %p) failed";
static const char short_spc173[] = "**mpi_testall %d %p %p %p";
static const char long_spc173[]  = "MPI_Testall(%d, %p, %p, %p) failed";
static const char short_spc174[] = "**mpi_testany %d %p %p %p %p";
static const char long_spc174[]  = "MPI_Testany(%d, %p, %p, %p, %p) failed";
static const char short_spc175[] = "**mpi_testsome %d %p %p %p %p";
static const char long_spc175[]  = "MPI_Testsome(%d, %p, %p, %p, %p) failed";
static const char short_spc176[] = "**mpi_type_commit %p";
static const char long_spc176[]  = "MPI_Type_commit(%p) failed";
static const char short_spc177[] = "**mpi_type_contiguous %d %D %p";
static const char long_spc177[]  = "MPI_Type_contiguous(%d, %D, %p) failed";
static const char short_spc178[] = "**mpi_type_create_hindexed %d %p %p %D %p";
static const char long_spc178[]  = "MPI_Type_create_hindexed(%d, %p, %p, %D, %p) failed";
static const char short_spc179[] = "**mpi_type_create_hvector %d %d %d %D %p";
static const char long_spc179[]  = "MPI_Type_create_hvector(%d, %d, %d, %D, %p) failed";
static const char short_spc180[] = "**mpi_type_create_indexed_block %d %d %p %D %p";
static const char long_spc180[]  = "MPI_Type_create_indexed_block(%d, %d, %p, %D, %p) failed";
static const char short_spc181[] = "**mpi_type_create_keyval %p %p %p %p";
static const char long_spc181[]  = "MPI_Type_create_keyval(%p, %p, %p, %p) failed";
static const char short_spc182[] = "**mpi_type_create_resized %D %d %d %p";
static const char long_spc182[]  = "MPI_Type_create_resized(%D, %d, %d, %p) failed";
static const char short_spc183[] = "**mpi_type_create_struct %d %p %p %p %p";
static const char long_spc183[]  = "MPI_Type_create_struct(%d, %p, %p, %p, %p) failed";
static const char short_spc184[] = "**mpi_type_delete_attr %D %d";
static const char long_spc184[]  = "MPI_Type_delete_atrr(%D, %d) failed";
static const char short_spc185[] = "**mpi_type_dup %D %p";
static const char long_spc185[]  = "MPI_Type_dup(%D, %p) failed";
static const char short_spc186[] = "**mpi_type_get_contents %D %d %d %d %p %p %p";
static const char long_spc186[]  = "MPI_Type_get_contents(%D, %d, %d, %d, %p, %p, %p) failed";
static const char short_spc187[] = "**mpi_type_get_envelope %D %p %p %p %p";
static const char long_spc187[]  = "MPI_Type_get_envelope(%D, %p, %p, %p, %p) failed";
static const char short_spc188[] = "**mpi_type_hindexed %d %p %p %D %p";
static const char long_spc188[]  = "MPI_Type_hindexed(%d, %p, %p, %D, %p) failed";
static const char short_spc189[] = "**mpi_type_hvector %d %d %d %D %p";
static const char long_spc189[]  = "MPI_Type_hvector(%d, %d, %d, %D, %p) failed";
static const char short_spc190[] = "**mpi_type_indexed %d %p %p %D %p";
static const char long_spc190[]  = "MPI_Type_indexed(%d, %p, %p, %D, %p) failed";
static const char short_spc191[] = "**mpi_type_match_size %d %d %p";
static const char long_spc191[]  = "MPI_Type_match_size(%d, %d, %p) failed";
static const char short_spc192[] = "**mpi_type_set_attr %D %d %p";
static const char long_spc192[]  = "MPI_Type_set_attr(%D, %d, %p) failed";
static const char short_spc193[] = "**mpi_type_struct %d %p %p %p %p";
static const char long_spc193[]  = "MPI_Type_struct(%d, %p, %p, %p, %p) failed";
static const char short_spc194[] = "**mpi_type_vector %d %d %d %D %p";
static const char long_spc194[]  = "MPI_Type_vector(%d, %d, %d, %D, %p) failed";
static const char short_spc195[] = "**mpi_unpack_external %s %p %d %p %p %d %D";
static const char long_spc195[]  = "MPI_Unpack_external(%s, %p, %d, %p, %p, %d, %D) failed";
static const char short_spc196[] = "**mpi_unpublish_name %s %I %s";
static const char long_spc196[]  = "MPI_Unpublish_name(%s, %I, %s) failed";
static const char short_spc197[] = "**mpi_wait %p %p";
static const char long_spc197[]  = "MPI_Wait(%p, %p) failed";
static const char short_spc198[] = "**mpi_waitall %d %p %p";
static const char long_spc198[]  = "MPI_Waitall(%d, %p, %p) failed";
static const char short_spc199[] = "**mpi_waitany %d %p %p %p";
static const char long_spc199[]  = "MPI_Waitany(%d, %p, %p, %p) failed";
static const char short_spc200[] = "**mpi_waitsome %d %p %p %p %p";
static const char long_spc200[]  = "MPI_Waitsome(%d, %p, %p, %p, %p) failed";
static const char short_spc201[] = "**mpi_win_complete %W";
static const char long_spc201[]  = "MPI_Win_complete(%W) failed";
static const char short_spc202[] = "**mpi_win_create %p %d %d %I %C %p";
static const char long_spc202[]  = "MPI_Win_create(%p, %d, %d, %I, %C, %p) failed";
static const char short_spc203[] = "**mpi_win_create_errhandler %p %p";
static const char long_spc203[]  = "MPI_Win_create_errhandler(%p, %p) failed";
static const char short_spc204[] = "**mpi_win_create_keyval %p %p %p %p";
static const char long_spc204[]  = "MPI_Win_create_keyval(%p, %p, %p, %p) failed";
static const char short_spc205[] = "**mpi_win_delete_attr %W %d";
static const char long_spc205[]  = "MPI_Win_delete_attr(%W, %d) failed";
static const char short_spc206[] = "**mpi_win_fence %d %W";
static const char long_spc206[]  = "MPI_Win_fence(%d, %W) failed";
static const char short_spc207[] = "**mpi_win_free %p";
static const char long_spc207[]  = "MPI_Win_free(%p) failed";
static const char short_spc208[] = "**mpi_win_get_group %W %p";
static const char long_spc208[]  = "MPI_Win_get_group(%W, %p) failed";
static const char short_spc209[] = "**mpi_win_lock %d %d %d %W";
static const char long_spc209[]  = "MPI_Win_lock(%d, %d, %d, %W) failed";
static const char short_spc210[] = "**mpi_win_post %G %d %W";
static const char long_spc210[]  = "MPI_Win_post(%G, %d, %W) failed";
static const char short_spc211[] = "**mpi_win_set_attr %W %d %p";
static const char long_spc211[]  = "MPI_Win_set_attr(%W, %d, %p) failed";
static const char short_spc212[] = "**mpi_win_test %W %p";
static const char long_spc212[]  = "MPI_Win_test(%W, %p) failed";
static const char short_spc213[] = "**mpi_win_unlock %d %W";
static const char long_spc213[]  = "MPI_Win_unlock(%d, %W) failed";
static const char short_spc214[] = "**mpi_win_wait %W";
static const char long_spc214[]  = "MPI_Win_wait(%W) failed";
static const char short_spc215[] = "**mq_close %d";
static const char long_spc215[]  = "failed to close a posix message queue, error %d";
static const char short_spc216[] = "**mq_open %d";
static const char long_spc216[]  = "failed to open a posix message queue, error %d";
static const char short_spc217[] = "**mq_receive %d";
static const char long_spc217[]  = "failed to receive a posix message queue message, error %d";
static const char short_spc218[] = "**mq_send %d";
static const char long_spc218[]  = "failed to send a posix message queue message, error %d";
static const char short_spc219[] = "**msgctl %d";
static const char long_spc219[]  = "msgctl returned %d";
static const char short_spc220[] = "**msgget %d";
static const char long_spc220[]  = "msgget returned %d";
static const char short_spc221[] = "**msgrcv %d";
static const char long_spc221[]  = "msgrcv returned %d";
static const char short_spc222[] = "**msgsnd %d";
static const char long_spc222[]  = "msgsnd returned %d";
static const char short_spc223[] = "**namepublish %s";
static const char long_spc223[]  = "Unable to publish service name %s";
static const char short_spc224[] = "**namepubnotpub %s";
static const char long_spc224[]  = "Lookup failed for service name %s";
static const char short_spc225[] = "**nomem %d";
static const char long_spc225[]  = "Out of memory (unable to allocate %d bytes)";
static const char short_spc226[] = "**nomem %s %d";
static const char long_spc226[]  = "Out of memory (unable to allocate a '%s' of size %d)";
static const char short_spc227[] = "**nomem %s";
static const char long_spc227[]  = "Out of memory (unable to allocate a '%s')";
static const char short_spc228[] = "**notsame %s %s";
static const char long_spc228[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc229[] = "**nulledge %d %d";
static const char long_spc229[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc230[] = "**nullptr %s";
static const char long_spc230[]  = "Null pointer in parameter %s";
static const char short_spc231[] = "**nullptrtype %s";
static const char long_spc231[]  = "Null %s pointer";
static const char short_spc232[] = "**open %s %d %d";
static const char long_spc232[]  = "open(%s) failed for process %d, error %d";
static const char short_spc233[] = "**opnotpredefined %d";
static const char long_spc233[]  = "only predefined ops are valid (op = %d)";
static const char short_spc234[] = "**opundefined %s";
static const char long_spc234[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc235[] = "**opundefined_rma %d";
static const char long_spc235[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc236[] = "**pd_alloc %s";
static const char long_spc236[]  = "unable to allocate a protection domain - %s";
static const char short_spc237[] = "**pkt_ptr %p %p";
static const char long_spc237[]  = "invalid shm queue packet pointer (%p != %p)";
static const char short_spc238[] = "**pmi_barrier %d";
static const char long_spc238[]  = "PMI_Barrier returned %d";
static const char short_spc239[] = "**pmi_finalize %d";
static const char long_spc239[]  = "PMI_Finalize returned %d";
static const char short_spc240[] = "**pmi_get_id %d";
static const char long_spc240[]  = "PMI_Get_id returned %d";
static const char short_spc241[] = "**pmi_get_id_length_max %d";
static const char long_spc241[]  = "PMI_Get_id_length_max returned %d";
static const char short_spc242[] = "**pmi_get_rank %d";
static const char long_spc242[]  = "PMI_Get_rank returned %d";
static const char short_spc243[] = "**pmi_get_size %d";
static const char long_spc243[]  = "PMI_Get_size returned %d";
static const char short_spc244[] = "**pmi_init %d";
static const char long_spc244[]  = "PMI_Init returned %d";
static const char short_spc245[] = "**pmi_kvs_commit %d";
static const char long_spc245[]  = "PMI_KVS_Commit returned %d";
static const char short_spc246[] = "**pmi_kvs_create %d";
static const char long_spc246[]  = "PMI_KVS_Create returned %d";
static const char short_spc247[] = "**pmi_kvs_get %d";
static const char long_spc247[]  = "PMI_KVS_Get returned %d";
static const char short_spc248[] = "**pmi_kvs_get_key_length_max %d";
static const char long_spc248[]  = "PMI_KVS_Get_key_length_max returned %d";
static const char short_spc249[] = "**pmi_kvs_get_my_name %d";
static const char long_spc249[]  = "PMI_KVS_Get_my_name returned %d";
static const char short_spc250[] = "**pmi_kvs_get_name_length_max %d";
static const char long_spc250[]  = "PMI_KVS_Get_name_length_max returned %d";
static const char short_spc251[] = "**pmi_kvs_get_parent %d";
static const char long_spc251[]  = "unable to get the PARENT_ROOT_PORT_NAME from the keyval space (pmi_error %d)";
static const char short_spc252[] = "**pmi_kvs_get_value_length_max %d";
static const char long_spc252[]  = "PMI_KVS_Get_value_length_max returned %d";
static const char short_spc253[] = "**pmi_kvs_put %d";
static const char long_spc253[]  = "PMI_KVS_Put returned %d";
static const char short_spc254[] = "**pmi_spawn_multiple %d";
static const char long_spc254[]  = "PMI_Spawn_multiple returned %d";
static const char short_spc255[] = "**post_connect %s";
static const char long_spc255[]  = "%s failed in VC_post_connect";
static const char short_spc256[] = "**rangedup %d %d %d";
static const char long_spc256[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc257[] = "**rangeendinvalid %d %d %d";
static const char long_spc257[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc258[] = "**rangestartinvalid %d %d %d";
static const char long_spc258[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc259[] = "**rank %d %d";
static const char long_spc259[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc260[] = "**rankarray %d %d %d";
static const char long_spc260[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc261[] = "**rankdup %d %d %d";
static const char long_spc261[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc262[] = "**ranklocal %d %d";
static const char long_spc262[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc263[] = "**rankremote %d %d";
static const char long_spc263[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc264[] = "**rmasize %d";
static const char long_spc264[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc265[] = "**root %d";
static const char long_spc265[]  = "Invalid root (value given was %d)";
static const char short_spc266[] = "**rsendnomatch %d %d";
static const char long_spc266[]  = "Ready send from source %d and with tag %d had no matching receive";
static const char short_spc267[] = "**shm_op %d";
static const char long_spc267[]  = "invalid shm operation (%d)";
static const char short_spc268[] = "**shm_open %s %d";
static const char long_spc268[]  = "unable to open shared memory object %s (errno %d)";
static const char short_spc269[] = "**shm_unlink %s %d";
static const char long_spc269[]  = "failed to unlink shared memory object %s, error %d";
static const char short_spc270[] = "**shmat %d";
static const char long_spc270[]  = "shmat failed, error %d";
static const char short_spc271[] = "**shmctl %d %d";
static const char long_spc271[]  = "failed to mark the sysv segment %d for removal, error %d";
static const char short_spc272[] = "**shmget %d";
static const char long_spc272[]  = "shmget failed, error %d";
static const char short_spc273[] = "**shmhost %s %s";
static const char long_spc273[]  = "process not on the same host (%s != %s)";
static const char short_spc274[] = "**shmq_index %d %d";
static const char long_spc274[]  = "invalid shm queue index (%d > %d)";
static const char short_spc275[] = "**snprintf %d";
static const char long_spc275[]  = "snprintf returned %d";
static const char short_spc276[] = "**sock_byname %d";
static const char long_spc276[]  = "gethostbyname failed (errno %d)";
static const char short_spc277[] = "**sock_connect %d";
static const char long_spc277[]  = "connect failed (errno %d)";
static const char short_spc278[] = "**sock_create %d";
static const char long_spc278[]  = "unable to create a socket (errno %d)";
static const char short_spc279[] = "**sock_gethost %d";
static const char long_spc279[]  = "gethostname failed (errno %d)";
static const char short_spc280[] = "**socket %d";
static const char long_spc280[]  = "WSASocket failed (errno %d)";
static const char short_spc281[] = "**sock|badbuf %d %d";
static const char long_spc281[]  = "the supplied buffer contains invalid memory (set=%d,sock=%d)";
static const char short_spc282[] = "**sock|badiovn %d %d %d";
static const char long_spc282[]  = "size of iov is invalid (set=%d,sock=%d,iov_n=%d)";
static const char short_spc283[] = "**sock|badlen %d %d %d %d";
static const char long_spc283[]  = "bad length parameter(s) (set=%d,sock=%d,min=%d,max=%d)";
static const char short_spc284[] = "**sock|badport %d";
static const char long_spc284[]  = "port number is out of range (sock=%d)";
static const char short_spc285[] = "**sock|closing %d %d";
static const char long_spc285[]  = "sock is in the process of being closed locally (set=%d,sock=%d)";
static const char short_spc286[] = "**sock|connclosed %d %d";
static const char long_spc286[]  = "connection closed by peer (set=%d,sock=%d)";
static const char short_spc287[] = "**sock|connfailed %d %d";
static const char long_spc287[]  = "connection failure (set=%d,sock=%d)";
static const char short_spc288[] = "**sock|notconnected %d %d";
static const char long_spc288[]  = "sock is not connected (set=%d,sock=%d)";
static const char short_spc289[] = "**sock|osnomem %d %d";
static const char long_spc289[]  = "operating system routine failed due to lack of memory (set=%d,sock=%d)";
static const char short_spc290[] = "**sock|poll|accept %d";
static const char long_spc290[]  = "accept failed to acquire a new socket (errno=%d)";
static const char short_spc291[] = "**sock|poll|badhandle %d %d %d";
static const char long_spc291[]  = "sock contains an invalid handle (set=%d,sock=%d,fd=%d)";
static const char short_spc292[] = "**sock|poll|bind %d %d";
static const char long_spc292[]  = "unable to bind socket to port (port=%d,errno=%d)";
static const char short_spc293[] = "**sock|poll|connfailed %d %d %d";
static const char long_spc293[]  = "connection failure (set=%d,sock=%d,errno=%d)";
static const char short_spc294[] = "**sock|poll|connfailed %d";
static const char long_spc294[]  = "connection failure (errno=%d)";
static const char short_spc295[] = "**sock|poll|connrefused %d %d %s";
static const char long_spc295[]  = "connection refused (set=%d,sock=%d,host=%s)";
static const char short_spc296[] = "**sock|poll|eqfail %d %d %d";
static const char long_spc296[]  = "fatal error: failed to enqueue an event; event was lost (set=%d,sock=%d,op=%d)";
static const char short_spc297[] = "**sock|poll|eqfail %d";
static const char long_spc297[]  = "fatal error: failed to enqueue an event; event was lost (op=%d)";
static const char short_spc298[] = "**sock|poll|getport %d";
static const char long_spc298[]  = "failed to obtain port number of listener (errno=%d)";
static const char short_spc299[] = "**sock|poll|hostres %d %d %s";
static const char long_spc299[]  = "unable to resolve host name to an address (set=%d,sock=%d,host=%s)";
static const char short_spc300[] = "**sock|poll|listen %d";
static const char long_spc300[]  = "listen() failed (errno=%d)";
static const char short_spc301[] = "**sock|poll|nodelay %d";
static const char long_spc301[]  = "unable to set TCP no delay attribute on socket (errno=%d)";
static const char short_spc302[] = "**sock|poll|nonblock %d";
static const char long_spc302[]  = "unable to set socket to nonblocking (errno=%d)";
static const char short_spc303[] = "**sock|poll|oserror %d %d %d";
static const char long_spc303[]  = "unknown operating system error (set=%d,sock=%d,errno=%d)";
static const char short_spc304[] = "**sock|poll|oserror %d";
static const char long_spc304[]  = "unknown operating system error (errno=%d)";
static const char short_spc305[] = "**sock|poll|pipe %d";
static const char long_spc305[]  = "unable to allocate pipe to wakeup a blocking poll() (errno=%d)";
static const char short_spc306[] = "**sock|poll|pipenonblock %d";
static const char long_spc306[]  = "unable to set wakeup pipe to nonblocking (errno=%d)";
static const char short_spc307[] = "**sock|poll|reuseaddr %d";
static const char long_spc307[]  = "unable to set reuseaddr attribute on socket (errno=%d)";
static const char short_spc308[] = "**sock|poll|socket %d";
static const char long_spc308[]  = "unable to obtain new socket (errno=%d)";
static const char short_spc309[] = "**sock|poll|unhandledstate %d %s";
static const char long_spc309[]  = "encountered an unhandled state (%d) while processing %s";
static const char short_spc310[] = "**sock|reads %d %d";
static const char long_spc310[]  = "attempt to perform multiple simultaneous reads (set=%d,sock=%d)";
static const char short_spc311[] = "**sock|writes %d %d";
static const char long_spc311[]  = "attempt to perform multiple simultaneous writes (set=%d,sock=%d)";
static const char short_spc312[] = "**stride %d %d %d";
static const char long_spc312[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc313[] = "**tag %d";
static const char long_spc313[]  = "Invalid tag, value is %d";
static const char short_spc314[] = "**topotoolarge %d %d";
static const char long_spc314[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc315[] = "**truncate %d %d %d %d";
static const char long_spc315[]  = "Message from rank %d and tag %d truncated %d bytes received but buffer size is %d";
static const char short_spc316[] = "**truncate %d %d";
static const char long_spc316[]  = "Message truncated; %d bytes received but buffer size is %d";
static const char short_spc317[] = "**typematchsize %s %d";
static const char long_spc317[]  = "No MPI datatype available for typeclass %s and size %d";
static const char short_spc318[] = "**typenamelen %d";
static const char long_spc318[]  = " Specified datatype name is too long (%d characters)";
static const char short_spc319[] = "**vc_state %d";
static const char long_spc319[]  = "invalid vc state (%d)";
static const char short_spc320[] = "**wsasock %d";
static const char long_spc320[]  = "WSAStartup failed (errno %d)";

static const int specific_msgs_len = 321;
static msgpair specific_err_msgs[] = {
{ short_spc0, long_spc0 },
{ short_spc1, long_spc1 },
{ short_spc2, long_spc2 },
{ short_spc3, long_spc3 },
{ short_spc4, long_spc4 },
{ short_spc5, long_spc5 },
{ short_spc6, long_spc6 },
{ short_spc7, long_spc7 },
{ short_spc8, long_spc8 },
{ short_spc9, long_spc9 },
{ short_spc10, long_spc10 },
{ short_spc11, long_spc11 },
{ short_spc12, long_spc12 },
{ short_spc13, long_spc13 },
{ short_spc14, long_spc14 },
{ short_spc15, long_spc15 },
{ short_spc16, long_spc16 },
{ short_spc17, long_spc17 },
{ short_spc18, long_spc18 },
{ short_spc19, long_spc19 },
{ short_spc20, long_spc20 },
{ short_spc21, long_spc21 },
{ short_spc22, long_spc22 },
{ short_spc23, long_spc23 },
{ short_spc24, long_spc24 },
{ short_spc25, long_spc25 },
{ short_spc26, long_spc26 },
{ short_spc27, long_spc27 },
{ short_spc28, long_spc28 },
{ short_spc29, long_spc29 },
{ short_spc30, long_spc30 },
{ short_spc31, long_spc31 },
{ short_spc32, long_spc32 },
{ short_spc33, long_spc33 },
{ short_spc34, long_spc34 },
{ short_spc35, long_spc35 },
{ short_spc36, long_spc36 },
{ short_spc37, long_spc37 },
{ short_spc38, long_spc38 },
{ short_spc39, long_spc39 },
{ short_spc40, long_spc40 },
{ short_spc41, long_spc41 },
{ short_spc42, long_spc42 },
{ short_spc43, long_spc43 },
{ short_spc44, long_spc44 },
{ short_spc45, long_spc45 },
{ short_spc46, long_spc46 },
{ short_spc47, long_spc47 },
{ short_spc48, long_spc48 },
{ short_spc49, long_spc49 },
{ short_spc50, long_spc50 },
{ short_spc51, long_spc51 },
{ short_spc52, long_spc52 },
{ short_spc53, long_spc53 },
{ short_spc54, long_spc54 },
{ short_spc55, long_spc55 },
{ short_spc56, long_spc56 },
{ short_spc57, long_spc57 },
{ short_spc58, long_spc58 },
{ short_spc59, long_spc59 },
{ short_spc60, long_spc60 },
{ short_spc61, long_spc61 },
{ short_spc62, long_spc62 },
{ short_spc63, long_spc63 },
{ short_spc64, long_spc64 },
{ short_spc65, long_spc65 },
{ short_spc66, long_spc66 },
{ short_spc67, long_spc67 },
{ short_spc68, long_spc68 },
{ short_spc69, long_spc69 },
{ short_spc70, long_spc70 },
{ short_spc71, long_spc71 },
{ short_spc72, long_spc72 },
{ short_spc73, long_spc73 },
{ short_spc74, long_spc74 },
{ short_spc75, long_spc75 },
{ short_spc76, long_spc76 },
{ short_spc77, long_spc77 },
{ short_spc78, long_spc78 },
{ short_spc79, long_spc79 },
{ short_spc80, long_spc80 },
{ short_spc81, long_spc81 },
{ short_spc82, long_spc82 },
{ short_spc83, long_spc83 },
{ short_spc84, long_spc84 },
{ short_spc85, long_spc85 },
{ short_spc86, long_spc86 },
{ short_spc87, long_spc87 },
{ short_spc88, long_spc88 },
{ short_spc89, long_spc89 },
{ short_spc90, long_spc90 },
{ short_spc91, long_spc91 },
{ short_spc92, long_spc92 },
{ short_spc93, long_spc93 },
{ short_spc94, long_spc94 },
{ short_spc95, long_spc95 },
{ short_spc96, long_spc96 },
{ short_spc97, long_spc97 },
{ short_spc98, long_spc98 },
{ short_spc99, long_spc99 },
{ short_spc100, long_spc100 },
{ short_spc101, long_spc101 },
{ short_spc102, long_spc102 },
{ short_spc103, long_spc103 },
{ short_spc104, long_spc104 },
{ short_spc105, long_spc105 },
{ short_spc106, long_spc106 },
{ short_spc107, long_spc107 },
{ short_spc108, long_spc108 },
{ short_spc109, long_spc109 },
{ short_spc110, long_spc110 },
{ short_spc111, long_spc111 },
{ short_spc112, long_spc112 },
{ short_spc113, long_spc113 },
{ short_spc114, long_spc114 },
{ short_spc115, long_spc115 },
{ short_spc116, long_spc116 },
{ short_spc117, long_spc117 },
{ short_spc118, long_spc118 },
{ short_spc119, long_spc119 },
{ short_spc120, long_spc120 },
{ short_spc121, long_spc121 },
{ short_spc122, long_spc122 },
{ short_spc123, long_spc123 },
{ short_spc124, long_spc124 },
{ short_spc125, long_spc125 },
{ short_spc126, long_spc126 },
{ short_spc127, long_spc127 },
{ short_spc128, long_spc128 },
{ short_spc129, long_spc129 },
{ short_spc130, long_spc130 },
{ short_spc131, long_spc131 },
{ short_spc132, long_spc132 },
{ short_spc133, long_spc133 },
{ short_spc134, long_spc134 },
{ short_spc135, long_spc135 },
{ short_spc136, long_spc136 },
{ short_spc137, long_spc137 },
{ short_spc138, long_spc138 },
{ short_spc139, long_spc139 },
{ short_spc140, long_spc140 },
{ short_spc141, long_spc141 },
{ short_spc142, long_spc142 },
{ short_spc143, long_spc143 },
{ short_spc144, long_spc144 },
{ short_spc145, long_spc145 },
{ short_spc146, long_spc146 },
{ short_spc147, long_spc147 },
{ short_spc148, long_spc148 },
{ short_spc149, long_spc149 },
{ short_spc150, long_spc150 },
{ short_spc151, long_spc151 },
{ short_spc152, long_spc152 },
{ short_spc153, long_spc153 },
{ short_spc154, long_spc154 },
{ short_spc155, long_spc155 },
{ short_spc156, long_spc156 },
{ short_spc157, long_spc157 },
{ short_spc158, long_spc158 },
{ short_spc159, long_spc159 },
{ short_spc160, long_spc160 },
{ short_spc161, long_spc161 },
{ short_spc162, long_spc162 },
{ short_spc163, long_spc163 },
{ short_spc164, long_spc164 },
{ short_spc165, long_spc165 },
{ short_spc166, long_spc166 },
{ short_spc167, long_spc167 },
{ short_spc168, long_spc168 },
{ short_spc169, long_spc169 },
{ short_spc170, long_spc170 },
{ short_spc171, long_spc171 },
{ short_spc172, long_spc172 },
{ short_spc173, long_spc173 },
{ short_spc174, long_spc174 },
{ short_spc175, long_spc175 },
{ short_spc176, long_spc176 },
{ short_spc177, long_spc177 },
{ short_spc178, long_spc178 },
{ short_spc179, long_spc179 },
{ short_spc180, long_spc180 },
{ short_spc181, long_spc181 },
{ short_spc182, long_spc182 },
{ short_spc183, long_spc183 },
{ short_spc184, long_spc184 },
{ short_spc185, long_spc185 },
{ short_spc186, long_spc186 },
{ short_spc187, long_spc187 },
{ short_spc188, long_spc188 },
{ short_spc189, long_spc189 },
{ short_spc190, long_spc190 },
{ short_spc191, long_spc191 },
{ short_spc192, long_spc192 },
{ short_spc193, long_spc193 },
{ short_spc194, long_spc194 },
{ short_spc195, long_spc195 },
{ short_spc196, long_spc196 },
{ short_spc197, long_spc197 },
{ short_spc198, long_spc198 },
{ short_spc199, long_spc199 },
{ short_spc200, long_spc200 },
{ short_spc201, long_spc201 },
{ short_spc202, long_spc202 },
{ short_spc203, long_spc203 },
{ short_spc204, long_spc204 },
{ short_spc205, long_spc205 },
{ short_spc206, long_spc206 },
{ short_spc207, long_spc207 },
{ short_spc208, long_spc208 },
{ short_spc209, long_spc209 },
{ short_spc210, long_spc210 },
{ short_spc211, long_spc211 },
{ short_spc212, long_spc212 },
{ short_spc213, long_spc213 },
{ short_spc214, long_spc214 },
{ short_spc215, long_spc215 },
{ short_spc216, long_spc216 },
{ short_spc217, long_spc217 },
{ short_spc218, long_spc218 },
{ short_spc219, long_spc219 },
{ short_spc220, long_spc220 },
{ short_spc221, long_spc221 },
{ short_spc222, long_spc222 },
{ short_spc223, long_spc223 },
{ short_spc224, long_spc224 },
{ short_spc225, long_spc225 },
{ short_spc226, long_spc226 },
{ short_spc227, long_spc227 },
{ short_spc228, long_spc228 },
{ short_spc229, long_spc229 },
{ short_spc230, long_spc230 },
{ short_spc231, long_spc231 },
{ short_spc232, long_spc232 },
{ short_spc233, long_spc233 },
{ short_spc234, long_spc234 },
{ short_spc235, long_spc235 },
{ short_spc236, long_spc236 },
{ short_spc237, long_spc237 },
{ short_spc238, long_spc238 },
{ short_spc239, long_spc239 },
{ short_spc240, long_spc240 },
{ short_spc241, long_spc241 },
{ short_spc242, long_spc242 },
{ short_spc243, long_spc243 },
{ short_spc244, long_spc244 },
{ short_spc245, long_spc245 },
{ short_spc246, long_spc246 },
{ short_spc247, long_spc247 },
{ short_spc248, long_spc248 },
{ short_spc249, long_spc249 },
{ short_spc250, long_spc250 },
{ short_spc251, long_spc251 },
{ short_spc252, long_spc252 },
{ short_spc253, long_spc253 },
{ short_spc254, long_spc254 },
{ short_spc255, long_spc255 },
{ short_spc256, long_spc256 },
{ short_spc257, long_spc257 },
{ short_spc258, long_spc258 },
{ short_spc259, long_spc259 },
{ short_spc260, long_spc260 },
{ short_spc261, long_spc261 },
{ short_spc262, long_spc262 },
{ short_spc263, long_spc263 },
{ short_spc264, long_spc264 },
{ short_spc265, long_spc265 },
{ short_spc266, long_spc266 },
{ short_spc267, long_spc267 },
{ short_spc268, long_spc268 },
{ short_spc269, long_spc269 },
{ short_spc270, long_spc270 },
{ short_spc271, long_spc271 },
{ short_spc272, long_spc272 },
{ short_spc273, long_spc273 },
{ short_spc274, long_spc274 },
{ short_spc275, long_spc275 },
{ short_spc276, long_spc276 },
{ short_spc277, long_spc277 },
{ short_spc278, long_spc278 },
{ short_spc279, long_spc279 },
{ short_spc280, long_spc280 },
{ short_spc281, long_spc281 },
{ short_spc282, long_spc282 },
{ short_spc283, long_spc283 },
{ short_spc284, long_spc284 },
{ short_spc285, long_spc285 },
{ short_spc286, long_spc286 },
{ short_spc287, long_spc287 },
{ short_spc288, long_spc288 },
{ short_spc289, long_spc289 },
{ short_spc290, long_spc290 },
{ short_spc291, long_spc291 },
{ short_spc292, long_spc292 },
{ short_spc293, long_spc293 },
{ short_spc294, long_spc294 },
{ short_spc295, long_spc295 },
{ short_spc296, long_spc296 },
{ short_spc297, long_spc297 },
{ short_spc298, long_spc298 },
{ short_spc299, long_spc299 },
{ short_spc300, long_spc300 },
{ short_spc301, long_spc301 },
{ short_spc302, long_spc302 },
{ short_spc303, long_spc303 },
{ short_spc304, long_spc304 },
{ short_spc305, long_spc305 },
{ short_spc306, long_spc306 },
{ short_spc307, long_spc307 },
{ short_spc308, long_spc308 },
{ short_spc309, long_spc309 },
{ short_spc310, long_spc310 },
{ short_spc311, long_spc311 },
{ short_spc312, long_spc312 },
{ short_spc313, long_spc313 },
{ short_spc314, long_spc314 },
{ short_spc315, long_spc315 },
{ short_spc316, long_spc316 },
{ short_spc317, long_spc317 },
{ short_spc318, long_spc318 },
{ short_spc319, long_spc319 },
{ short_spc320, long_spc320 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
532,42,104,112,533,95,445,461,150,401,
536,109,10,542,538,406,184,183,182,454,
124,125,131,103,106,129,130,123,161,162,
168,167,190,381,9,397,135,134,430,137,
138,464,526,107,136,545,27,222,216,457,
460,459,458,20};
#endif
