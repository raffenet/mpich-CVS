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
static const char short_gen38[] = "**bufalias";
static const char long_gen38[]  = "Buffers must not be aliased";
static const char short_gen39[] = "**bufbsend";
static const char long_gen39[]  = "Insufficient space in Bsend buffer";
static const char short_gen40[] = "**bufexists";
static const char long_gen40[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short_gen41[] = "**buffer";
static const char long_gen41[]  = "Invalid buffer pointer";
static const char short_gen42[] = "**bufnull";
static const char long_gen42[]  = "Null buffer pointer";
static const char short_gen43[] = "**buscard";
static const char long_gen43[]  = "unable to create a business card";
static const char short_gen44[] = "**buscard_len";
static const char long_gen44[]  = "no space left in the business card to add a parameter";
static const char short_gen45[] = "**business_card";
static const char long_gen45[]  = "Invalid business card";
static const char short_gen46[] = "**ca";
static const char long_gen46[]  = "invalid completion action";
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
static const char short_gen53[] = "**ch3_send";
static const char long_gen53[]  = "send failed";
static const char short_gen54[] = "**ch3ireadaggressive";
static const char long_gen54[]  = "aggressive reading failed";
static const char short_gen55[] = "**ch3progress";
static const char long_gen55[]  = "Unable to make message passing progress";
static const char short_gen56[] = "**ch3|badca";
static const char long_gen56[]  = "specified completion action in not known";
static const char short_gen57[] = "**ch3|badmsgtype";
static const char long_gen57[]  = "request contained an invalid message type";
static const char short_gen58[] = "**ch3|badreqtype";
static const char long_gen58[]  = "request contained an invalid request type";
static const char short_gen59[] = "**ch3|canceleager";
static const char long_gen59[]  = "failure occurred while performing local cancellation of a eager message";
static const char short_gen60[] = "**ch3|cancelreq";
static const char long_gen60[]  = "failure occurred while sending remote cancellation request packet";
static const char short_gen61[] = "**ch3|cancelresp";
static const char long_gen61[]  = "failure occurred while attempting to send cancel response packet";
static const char short_gen62[] = "**ch3|cancelrndv";
static const char long_gen62[]  = "failure occurred while performing local cancellation of a rendezvous message";
static const char short_gen63[] = "**ch3|ctspkt";
static const char long_gen63[]  = "failure occurred while attempting to send CTS packet";
static const char short_gen64[] = "**ch3|eagermsg";
static const char long_gen64[]  = "failure occurred while attempting to send an eager message";
static const char short_gen65[] = "**ch3|flowcntlpkt";
static const char long_gen65[]  = "UNIMPLEMENTED: unable to handle flow control packets";
static const char short_gen66[] = "**ch3|loadrecviov";
static const char long_gen66[]  = "failure occurred while loading the receive I/O vector";
static const char short_gen67[] = "**ch3|loadsendiov";
static const char long_gen67[]  = "failure occurred while loading the send I/O vector";
static const char short_gen68[] = "**ch3|nopktcontainermem";
static const char long_gen68[]  = "failed to allocate memory for a packet reorder container";
static const char short_gen69[] = "**ch3|ooocancelreq";
static const char long_gen69[]  = "UNIMPLEMENTED: unable to process out-of-order cancellation requests";
static const char short_gen70[] = "**ch3|pktordered";
static const char long_gen70[]  = "failure occurred while processing a reordered packet";
static const char short_gen71[] = "**ch3|postrecv";
static const char long_gen71[]  = "failure occurred while posting a receive for message data";
static const char short_gen72[] = "**ch3|recvdata";
static const char long_gen72[]  = "failure occurred while attempting to receive message data";
static const char short_gen73[] = "**ch3|rmamsg";
static const char long_gen73[]  = "failure occurred while attempting to send an RMA message";
static const char short_gen74[] = "**ch3|rtspkt";
static const char long_gen74[]  = "failure occurred while attempting to send RTS packet";
static const char short_gen75[] = "**ch3|selfsenddeadlock";
static const char long_gen75[]  = "DEADLOCK: attempting to send a message to the local process without a prior matching receive";
static const char short_gen76[] = "**ch3|senddata";
static const char long_gen76[]  = "failure occurred while attempting to send message data";
static const char short_gen77[] = "**ch3|sock|addrinuse";
static const char long_gen77[]  = "[ch3:sock] tcp port already in use";
static const char short_gen78[] = "**ch3|sock|badbuscard";
static const char long_gen78[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen79[] = "**ch3|sock|badpacket";
static const char long_gen79[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen80[] = "**ch3|sock|badsock";
static const char long_gen80[]  = "[ch3:sock] internal error - bad sock";
static const char short_gen81[] = "**ch3|sock|connallocfailed";
static const char long_gen81[]  = "[ch3:sock] unable to allocate a connection structure";
static const char short_gen82[] = "**ch3|sock|connclose";
static const char long_gen82[]  = "[ch3:sock] active connection unexpectedly closed";
static const char short_gen83[] = "**ch3|sock|connfailed";
static const char long_gen83[]  = "[ch3:sock] failed to connnect to remote process";
static const char short_gen84[] = "**ch3|sock|connrefused";
static const char long_gen84[]  = "[ch3:sock] connection refused";
static const char short_gen85[] = "**ch3|sock|connterm";
static const char long_gen85[]  = "[ch3:sock] active connection unexpectedly terminated";
static const char short_gen86[] = "**ch3|sock|failure";
static const char long_gen86[]  = "[ch3:sock] unknown failure";
static const char short_gen87[] = "**ch3|sock|hostlookup";
static const char long_gen87[]  = "[ch3:sock] hostname lookup failed";
static const char short_gen88[] = "**ch3|sock|pmi_finalize";
static const char long_gen88[]  = "PMI_Finalize failed";
static const char short_gen89[] = "**ch3|sock|post_write";
static const char long_gen89[]  = "[ch3:sock] posting a write failed";
static const char short_gen90[] = "**ch3|sock|progress_finalize";
static const char long_gen90[]  = "[ch3:sock] progress_finalize failed";
static const char short_gen91[] = "**ch3|sock|strdup";
static const char long_gen91[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen92[] = "**ch3|syncack";
static const char long_gen92[]  = "failure occurred while attempting to send eager synchronization packet";
static const char short_gen93[] = "**ch3|unknownpkt";
static const char long_gen93[]  = "received unknown packet type";
static const char short_gen94[] = "**comm";
static const char long_gen94[]  = "Invalid communicator";
static const char short_gen95[] = "**commnotinter";
static const char long_gen95[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen96[] = "**commnotintra";
static const char long_gen96[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen97[] = "**commnull";
static const char long_gen97[]  = "Null communicator";
static const char short_gen98[] = "**commperm";
static const char long_gen98[]  = "Cannot free permanent communicator";
static const char short_gen99[] = "**conn_still_active";
static const char long_gen99[]  = "connection closed while still active";
static const char short_gen100[] = "**connfailed";
static const char long_gen100[]  = "Failed to connect to remote process";
static const char short_gen101[] = "**connrefused";
static const char long_gen101[]  = "Connection refused";
static const char short_gen102[] = "**conversion";
static const char long_gen102[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen103[] = "**count";
static const char long_gen103[]  = "Invalid count";
static const char short_gen104[] = "**countneg";
static const char long_gen104[]  = "Negative count";
static const char short_gen105[] = "**datarep";
static const char long_gen105[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen106[] = "**datarepunsupported";
static const char long_gen106[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen107[] = "**desc_len";
static const char long_gen107[]  = "host description buffer too small";
static const char short_gen108[] = "**dims";
static const char long_gen108[]  = "Invalid dimension argument";
static const char short_gen109[] = "**dimsmany";
static const char long_gen109[]  = "Number of dimensions is too large ";
static const char short_gen110[] = "**dimspartition";
static const char long_gen110[]  = "Cannot partition nodes as requested ";
static const char short_gen111[] = "**dtype";
static const char long_gen111[]  = "Invalid datatype";
static const char short_gen112[] = "**dtypecommit";
static const char long_gen112[]  = "Datatype has not been committed ";
static const char short_gen113[] = "**dtypemismatch";
static const char long_gen113[]  = "Receiving data with a datatype whose signature does not match that of the sending datatype.";
static const char short_gen114[] = "**dtypenull";
static const char long_gen114[]  = "Null datatype";
static const char short_gen115[] = "**dtypeperm";
static const char long_gen115[]  = "Cannot free permanent data type ";
static const char short_gen116[] = "**duphandle";
static const char long_gen116[]  = "unable to duplicate a handle";
static const char short_gen117[] = "**dupprocesses";
static const char long_gen117[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen118[] = "**edgeoutrange";
static const char long_gen118[]  = "Edge index in graph topology is out of range";
static const char short_gen119[] = "**errhandnotfile";
static const char long_gen119[]  = "Error handler is not a file error handler";
static const char short_gen120[] = "**errhandnotwin";
static const char long_gen120[]  = "Error handler is not a win error handler";
static const char short_gen121[] = "**fail";
static const char long_gen121[]  = "failure";
static const char short_gen122[] = "**file";
static const char long_gen122[]  = "Invalid MPI_File";
static const char short_gen123[] = "**fileaccess";
static const char long_gen123[]  = "Access denied to file";
static const char short_gen124[] = "**fileamode";
static const char long_gen124[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen125[] = "**fileamodeone";
static const char long_gen125[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen126[] = "**fileamoderead";
static const char long_gen126[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen127[] = "**fileamodeseq";
static const char long_gen127[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen128[] = "**fileexist";
static const char long_gen128[]  = "File exists";
static const char short_gen129[] = "**fileinuse";
static const char long_gen129[]  = "File in use by some process";
static const char short_gen130[] = "**filename";
static const char long_gen130[]  = "Invalid file name";
static const char short_gen131[] = "**filenamedir";
static const char long_gen131[]  = "Invalid or missing directory";
static const char short_gen132[] = "**filenamelong";
static const char long_gen132[]  = "Pathname too long";
static const char short_gen133[] = "**filenoexist";
static const char long_gen133[]  = "File does not exist";
static const char short_gen134[] = "**filenospace";
static const char long_gen134[]  = "Not enough space for file ";
static const char short_gen135[] = "**fileopunsupported";
static const char long_gen135[]  = "Unsupported file operation ";
static const char short_gen136[] = "**filequota";
static const char long_gen136[]  = "Quota exceeded for files";
static const char short_gen137[] = "**filerdonly";
static const char long_gen137[]  = "Read-only file or filesystem name";
static const char short_gen138[] = "**finalize_boot";
static const char long_gen138[]  = "destroying the message queue failed";
static const char short_gen139[] = "**finalize_progress";
static const char long_gen139[]  = "finalizing the progress engine failed";
static const char short_gen140[] = "**finalize_progress_finalize";
static const char long_gen140[]  = "Progress finalize failed";
static const char short_gen141[] = "**finalize_release_mem";
static const char long_gen141[]  = "Release shared memory failed";
static const char short_gen142[] = "**finish_qp";
static const char long_gen142[]  = "unable to establish a queue pair connection";
static const char short_gen143[] = "**ftok";
static const char long_gen143[]  = "failed to create a sysv key from a file name";
static const char short_gen144[] = "**ftruncate";
static const char long_gen144[]  = "unable to resize the shared memory object";
static const char short_gen145[] = "**get_guids";
static const char long_gen145[]  = "unable to get the guids for the infiniband channel adapter";
static const char short_gen146[] = "**gethostbyname";
static const char long_gen146[]  = "gethostbyname failed";
static const char short_gen147[] = "**getinfo";
static const char long_gen147[]  = "getaddrinfo failed";
static const char short_gen148[] = "**graphnnodes";
static const char long_gen148[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen149[] = "**group";
static const char long_gen149[]  = "Invalid group";
static const char short_gen150[] = "**groupnotincomm";
static const char long_gen150[]  = "Specified group is not within the communicator";
static const char short_gen151[] = "**handle_read";
static const char long_gen151[]  = "Unable to handle the read data";
static const char short_gen152[] = "**handle_sock_op";
static const char long_gen152[]  = "handle_sock_op failed";
static const char short_gen153[] = "**handle_written";
static const char long_gen153[]  = "unable to handle written data";
static const char short_gen154[] = "**hostlookup";
static const char long_gen154[]  = "Host lookup failed";
static const char short_gen155[] = "**ibu_op";
static const char long_gen155[]  = "invalid infiniband operation";
static const char short_gen156[] = "**ibu_wait";
static const char long_gen156[]  = "ibu_wait failed";
static const char short_gen157[] = "**ibwrite";
static const char long_gen157[]  = "infiniband write failed";
static const char short_gen158[] = "**indexneg";
static const char long_gen158[]  = "Index value in graph topology must be nonnegative";
static const char short_gen159[] = "**indexnonmonotone";
static const char long_gen159[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen160[] = "**info";
static const char long_gen160[]  = "Invalid MPI_Info";
static const char short_gen161[] = "**infokey";
static const char long_gen161[]  = "Invalid key for MPI_Info ";
static const char short_gen162[] = "**infokeyempty";
static const char long_gen162[]  = "Empty or blank key ";
static const char short_gen163[] = "**infokeylong";
static const char long_gen163[]  = "Key is too long";
static const char short_gen164[] = "**infokeynull";
static const char long_gen164[]  = "Null key";
static const char short_gen165[] = "**infonkey";
static const char long_gen165[]  = "Requested nth key does not exist";
static const char short_gen166[] = "**infonokey";
static const char long_gen166[]  = "MPI_Info key is not defined ";
static const char short_gen167[] = "**infoval";
static const char long_gen167[]  = "Invalid MPI_Info value ";
static const char short_gen168[] = "**infovallong";
static const char long_gen168[]  = "Value is too long ";
static const char short_gen169[] = "**infovalnull";
static const char long_gen169[]  = "Null value";
static const char short_gen170[] = "**init";
static const char long_gen170[]  = "Initialization failed";
static const char short_gen171[] = "**init_buscard";
static const char long_gen171[]  = "failed to get my business card";
static const char short_gen172[] = "**init_comm_create";
static const char long_gen172[]  = "unable to create an intercommunicator for the parent";
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
static const char short_gen224[] = "**mq_close";
static const char long_gen224[]  = "failed to close a posix message queue";
static const char short_gen225[] = "**mq_open";
static const char long_gen225[]  = "failed to open a posix message queue";
static const char short_gen226[] = "**mq_receive";
static const char long_gen226[]  = "failed to receive a posix message queue message";
static const char short_gen227[] = "**mq_send";
static const char long_gen227[]  = "failed to send a posix message queue message";
static const char short_gen228[] = "**mqp_failure";
static const char long_gen228[]  = "failed to make progress on the shared memory bootstrap message queue";
static const char short_gen229[] = "**mqshm_create";
static const char long_gen229[]  = "failed to create a shared memory message queue";
static const char short_gen230[] = "**mqshm_receive";
static const char long_gen230[]  = "failed to receive a bootstrap message";
static const char short_gen231[] = "**mqshm_send";
static const char long_gen231[]  = "failed to send a bootstrap message";
static const char short_gen232[] = "**mqshm_unlink";
static const char long_gen232[]  = "unable to unlink the shared memory message queue";
static const char short_gen233[] = "**msgctl";
static const char long_gen233[]  = "msgctl failed";
static const char short_gen234[] = "**msgget";
static const char long_gen234[]  = "msgget failed";
static const char short_gen235[] = "**msgrcv";
static const char long_gen235[]  = "msgrcv failed";
static const char short_gen236[] = "**msgsnd";
static const char long_gen236[]  = "msgsnd failed";
static const char short_gen237[] = "**multi_post_read";
static const char long_gen237[]  = "posting a read while a previously posted read is outstanding";
static const char short_gen238[] = "**multi_post_write";
static const char long_gen238[]  = "posting a write while a previously posted write is outstanding";
static const char short_gen239[] = "**namepublish";
static const char long_gen239[]  = "Unable to publish service name";
static const char short_gen240[] = "**namepubnotpub";
static const char long_gen240[]  = "Lookup failed for service name ";
static const char short_gen241[] = "**nameservice";
static const char long_gen241[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen242[] = "**needthreads";
static const char long_gen242[]  = "This function needs threads and threads have not been enabled";
static const char short_gen243[] = "**nextbootmsg";
static const char long_gen243[]  = "failed to get the next bootstrap message";
static const char short_gen244[] = "**noca";
static const char long_gen244[]  = "unable to find an active infiniband channel adapter";
static const char short_gen245[] = "**noerrclasses";
static const char long_gen245[]  = "No more user-defined error classes";
static const char short_gen246[] = "**noerrcodes";
static const char long_gen246[]  = "No more user-defined error codes";
static const char short_gen247[] = "**nomem";
static const char long_gen247[]  = "Out of memory";
static const char short_gen248[] = "**nomemreq";
static const char long_gen248[]  = "failure occurred while allocating memory for a request object";
static const char short_gen249[] = "**nonamepub";
static const char long_gen249[]  = "No name publishing service available";
static const char short_gen250[] = "**notcarttopo";
static const char long_gen250[]  = "No Cartesian topology associated with this communicator";
static const char short_gen251[] = "**notcstatignore";
static const char long_gen251[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_c2f()";
static const char short_gen252[] = "**notfstatignore";
static const char long_gen252[]  = "MPI_STATUS_IGNORE cannot be passed to MPI_Status_f2c()";
static const char short_gen253[] = "**notgenreq";
static const char long_gen253[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen254[] = "**notgraphtopo";
static const char long_gen254[]  = "No Graph topology associated with this communicator";
static const char short_gen255[] = "**notimpl";
static const char long_gen255[]  = "Function not implemented";
static const char short_gen256[] = "**notopology";
static const char long_gen256[]  = "No topology associated with this communicator";
static const char short_gen257[] = "**notsame";
static const char long_gen257[]  = "Inconsistent arguments to collective routine ";
static const char short_gen258[] = "**nulledge";
static const char long_gen258[]  = "Edge detected from a node to the same node";
static const char short_gen259[] = "**nullptr";
static const char long_gen259[]  = "Null pointer";
static const char short_gen260[] = "**nullptrtype";
static const char long_gen260[]  = "Null pointer";
static const char short_gen261[] = "**op";
static const char long_gen261[]  = "Invalid MPI_Op";
static const char short_gen262[] = "**open";
static const char long_gen262[]  = "open failed";
static const char short_gen263[] = "**opnotpredefined";
static const char long_gen263[]  = "only predefined ops are valid";
static const char short_gen264[] = "**opundefined";
static const char long_gen264[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen265[] = "**opundefined_rma";
static const char long_gen265[]  = "RMA target received unknown RMA operation";
static const char short_gen266[] = "**other";
static const char long_gen266[]  = "Other MPI error";
static const char short_gen267[] = "**pctwice";
static const char long_gen267[]  = "post connect called twice";
static const char short_gen268[] = "**pd_alloc";
static const char long_gen268[]  = "unable to allocate a protection domain";
static const char short_gen269[] = "**permattr";
static const char long_gen269[]  = "Cannot set permanent attribute";
static const char short_gen270[] = "**permop";
static const char long_gen270[]  = "Cannot free permanent MPI_Op ";
static const char short_gen271[] = "**pfinal_sockclose";
static const char long_gen271[]  = "sock_close failed";
static const char short_gen272[] = "**pkt_ptr";
static const char long_gen272[]  = "invalid shm queue packet pointer";
static const char short_gen273[] = "**pmi_barrier";
static const char long_gen273[]  = "PMI_Barrier failed";
static const char short_gen274[] = "**pmi_finalize";
static const char long_gen274[]  = "PMI_Finalize failed";
static const char short_gen275[] = "**pmi_get_rank";
static const char long_gen275[]  = "PMI_Get_rank failed";
static const char short_gen276[] = "**pmi_get_size";
static const char long_gen276[]  = "PMI_Get_size failed";
static const char short_gen277[] = "**pmi_init";
static const char long_gen277[]  = "PMI_Init failed";
static const char short_gen278[] = "**pmi_kvs_commit";
static const char long_gen278[]  = "PMI_KVS_Commit failed";
static const char short_gen279[] = "**pmi_kvs_create";
static const char long_gen279[]  = "PMI_KVS_Create failed";
static const char short_gen280[] = "**pmi_kvs_get";
static const char long_gen280[]  = "PMI_KVS_Get failed";
static const char short_gen281[] = "**pmi_kvs_get_my_name";
static const char long_gen281[]  = "PMI_KVS_Get_my_name failed";
static const char short_gen282[] = "**pmi_kvs_put";
static const char long_gen282[]  = "PMI_KVS_Put failed";
static const char short_gen283[] = "**pmi_spawn_multiple";
static const char long_gen283[]  = "PMI_Spawn_multiple failed";
static const char short_gen284[] = "**poke";
static const char long_gen284[]  = "progress_poke failed";
static const char short_gen285[] = "**port";
static const char long_gen285[]  = "Invalid port";
static const char short_gen286[] = "**post_accept";
static const char long_gen286[]  = "post accept failed";
static const char short_gen287[] = "**post_connect";
static const char long_gen287[]  = "failed to post a connection";
static const char short_gen288[] = "**post_sock_write_on_shm";
static const char long_gen288[]  = "posting a socket read on a shm connection";
static const char short_gen289[] = "**postpkt";
static const char long_gen289[]  = "Unable to post a read for the next packet header";
static const char short_gen290[] = "**process_group";
static const char long_gen290[]  = "Process group initialization failed";
static const char short_gen291[] = "**progress";
static const char long_gen291[]  = "progress engine failure";
static const char short_gen292[] = "**progress_finalize";
static const char long_gen292[]  = "finalization of the progress engine failed";
static const char short_gen293[] = "**progress_handle_sock_op";
static const char long_gen293[]  = "handle_sock_op failed";
static const char short_gen294[] = "**progress_init";
static const char long_gen294[]  = "unable to initialize the progress engine";
static const char short_gen295[] = "**progress_sock_wait";
static const char long_gen295[]  = "sock_wait failed";
static const char short_gen296[] = "**progress_test";
static const char long_gen296[]  = "progress_test engine failure";
static const char short_gen297[] = "**rangedup";
static const char long_gen297[]  = "The range array specifies duplicate entries";
static const char short_gen298[] = "**rangeendinvalid";
static const char long_gen298[]  = "Some element of a range array is either negative or too large";
static const char short_gen299[] = "**rangestartinvalid";
static const char long_gen299[]  = "Some element of a range array is either negative or too large";
static const char short_gen300[] = "**rank";
static const char long_gen300[]  = "Invalid rank";
static const char short_gen301[] = "**rankarray";
static const char long_gen301[]  = "Invalid rank in rank array";
static const char short_gen302[] = "**rankdup";
static const char long_gen302[]  = "Duplicate ranks in rank array ";
static const char short_gen303[] = "**ranklocal";
static const char long_gen303[]  = "Error specifying local_leader ";
static const char short_gen304[] = "**rankremote";
static const char long_gen304[]  = "Error specifying remote_leader ";
static const char short_gen305[] = "**rdma_finalize";
static const char long_gen305[]  = "Channel rdma finalization failed";
static const char short_gen306[] = "**rdma_init";
static const char long_gen306[]  = "Channel rdma initialization failed";
static const char short_gen307[] = "**read_progress";
static const char long_gen307[]  = "Unable to make read progress";
static const char short_gen308[] = "**request";
static const char long_gen308[]  = "Invalid MPI_Request";
static const char short_gen309[] = "**requestnotpersist";
static const char long_gen309[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen310[] = "**requestpersistactive";
static const char long_gen310[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen311[] = "**rmaconflict";
static const char long_gen311[]  = "Conflicting accesses to window ";
static const char short_gen312[] = "**rmadisp";
static const char long_gen312[]  = "Invalid displacement argument in RMA call ";
static const char short_gen313[] = "**rmasize";
static const char long_gen313[]  = "Invalid size argument in RMA call";
static const char short_gen314[] = "**rmasync";
static const char long_gen314[]  = "Wrong synchronization of RMA calls ";
static const char short_gen315[] = "**root";
static const char long_gen315[]  = "Invalid root";
static const char short_gen316[] = "**rsendnomatch";
static const char long_gen316[]  = "Ready send had no matching receive ";
static const char short_gen317[] = "**servicename";
static const char long_gen317[]  = "Attempt to lookup an unknown service name ";
static const char short_gen318[] = "**shm_op";
static const char long_gen318[]  = "invalid shm operation";
static const char short_gen319[] = "**shm_open";
static const char long_gen319[]  = "unable to open a shared memory object";
static const char short_gen320[] = "**shm_read_progress";
static const char long_gen320[]  = "shared memory read progress failed";
static const char short_gen321[] = "**shm_unlink";
static const char long_gen321[]  = "failed to unlink shared memory";
static const char short_gen322[] = "**shm_wait";
static const char long_gen322[]  = "wait function failed";
static const char short_gen323[] = "**shmat";
static const char long_gen323[]  = "shmat failed";
static const char short_gen324[] = "**shmconnect_getmem";
static const char long_gen324[]  = "failed to allocate shared memory for a write queue";
static const char short_gen325[] = "**shmctl";
static const char long_gen325[]  = "failed to mark the sysv segment for removal";
static const char short_gen326[] = "**shmget";
static const char long_gen326[]  = "shmget failed";
static const char short_gen327[] = "**shmgetmem";
static const char long_gen327[]  = "Unable to allocate shared memory";
static const char short_gen328[] = "**shmhost";
static const char long_gen328[]  = "process not on the same host";
static const char short_gen329[] = "**shmq";
static const char long_gen329[]  = "invalid shm queue pointer";
static const char short_gen330[] = "**shmq_index";
static const char long_gen330[]  = "invalid shm queue index";
static const char short_gen331[] = "**shmwrite";
static const char long_gen331[]  = "shared memory write failed";
static const char short_gen332[] = "**snprintf";
static const char long_gen332[]  = "snprintf returned an invalid number";
static const char short_gen333[] = "**sock_byname";
static const char long_gen333[]  = "gethostbyname failed";
static const char short_gen334[] = "**sock_closed";
static const char long_gen334[]  = "socket closed";
static const char short_gen335[] = "**sock_connect";
static const char long_gen335[]  = "connect failed";
static const char short_gen336[] = "**sock_create";
static const char long_gen336[]  = "unable to create a socket";
static const char short_gen337[] = "**sock_gethost";
static const char long_gen337[]  = "gethostname failed";
static const char short_gen338[] = "**sock_init";
static const char long_gen338[]  = "unable to initialize the sock library";
static const char short_gen339[] = "**sock_nop_accept";
static const char long_gen339[]  = "accept called without having received an op_accept";
static const char short_gen340[] = "**sock_post_close";
static const char long_gen340[]  = "posting a close of the socket failed";
static const char short_gen341[] = "**socket";
static const char long_gen341[]  = "WSASocket failed";
static const char short_gen342[] = "**sock|badbuf";
static const char long_gen342[]  = "the supplied buffer contains invalid memory";
static const char short_gen343[] = "**sock|badhandle";
static const char long_gen343[]  = "sock contains an invalid handle";
static const char short_gen344[] = "**sock|badhdbuf";
static const char long_gen344[]  = "a memory fault occurred while accessing the host description string";
static const char short_gen345[] = "**sock|badhdlen";
static const char long_gen345[]  = "host description string to small to store description";
static const char short_gen346[] = "**sock|badhdmax";
static const char long_gen346[]  = "the length of the host description string must be non-negative";
static const char short_gen347[] = "**sock|badiovn";
static const char long_gen347[]  = "size of iov is invalid";
static const char short_gen348[] = "**sock|badlen";
static const char long_gen348[]  = "bad length parameter(s)";
static const char short_gen349[] = "**sock|badport";
static const char long_gen349[]  = "port number is out of range";
static const char short_gen350[] = "**sock|badsock";
static const char long_gen350[]  = "supplied sock is corrupt";
static const char short_gen351[] = "**sock|closed";
static const char long_gen351[]  = "sock has been closed locally";
static const char short_gen352[] = "**sock|closing";
static const char long_gen352[]  = "sock is in the process of being closed locally";
static const char short_gen353[] = "**sock|connclosed";
static const char long_gen353[]  = "connection closed by peer";
static const char short_gen354[] = "**sock|connfailed";
static const char long_gen354[]  = "connection failure";
static const char short_gen355[] = "**sock|connrefused";
static const char long_gen355[]  = "connection refused";
static const char short_gen356[] = "**sock|getport";
static const char long_gen356[]  = "failed to obtain port number of the listener";
static const char short_gen357[] = "**sock|hostres";
static const char long_gen357[]  = "unable to resolve host name to an address";
static const char short_gen358[] = "**sock|nosock";
static const char long_gen358[]  = "no new sock was available to accept";
static const char short_gen359[] = "**sock|notconnected";
static const char long_gen359[]  = "sock is not connected";
static const char short_gen360[] = "**sock|oserror";
static const char long_gen360[]  = "unknown operating system error";
static const char short_gen361[] = "**sock|osnomem";
static const char long_gen361[]  = "operating system routine failed due to lack of memory";
static const char short_gen362[] = "**sock|poll|accept";
static const char long_gen362[]  = "accept failed to acquire a new socket";
static const char short_gen363[] = "**sock|poll|bind";
static const char long_gen363[]  = "unable to bind socket to port";
static const char short_gen364[] = "**sock|poll|eqfail";
static const char long_gen364[]  = "fatal error: failed to enqueue an event; event was lost";
static const char short_gen365[] = "**sock|poll|eqmalloc";
static const char long_gen365[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen366[] = "**sock|poll|listen";
static const char long_gen366[]  = "listen() failed";
static const char short_gen367[] = "**sock|poll|nodelay";
static const char long_gen367[]  = "unable to set TCP no delay attribute on socket";
static const char short_gen368[] = "**sock|poll|nonblock";
static const char long_gen368[]  = "unable to set socket to nonblocking";
static const char short_gen369[] = "**sock|poll|pipe";
static const char long_gen369[]  = "unable to allocate pipe to wakeup a blocking poll()";
static const char short_gen370[] = "**sock|poll|pipenonblock";
static const char long_gen370[]  = "unable to set wakeup pipe to nonblocking";
static const char short_gen371[] = "**sock|poll|reuseaddr";
static const char long_gen371[]  = "unable to set reuseaddr attribute on socket";
static const char short_gen372[] = "**sock|poll|socket";
static const char long_gen372[]  = "unable to obtain new socket";
static const char short_gen373[] = "**sock|poll|unhandledstate";
static const char long_gen373[]  = "encountered an unhandled";
static const char short_gen374[] = "**sock|reads";
static const char long_gen374[]  = "attempt to perform multiple simultaneous reads";
static const char short_gen375[] = "**sock|setalloc";
static const char long_gen375[]  = "unable to allocate a new sock set object";
static const char short_gen376[] = "**sock|sockalloc";
static const char long_gen376[]  = "unable to allocate a new sock object";
static const char short_gen377[] = "**sock|uninit";
static const char long_gen377[]  = "Sock library has not been initialized";
static const char short_gen378[] = "**sock|writes";
static const char long_gen378[]  = "attempt to perform multiple simultaneous writes";
static const char short_gen379[] = "**spawn";
static const char long_gen379[]  = "Error in spawn call";
static const char short_gen380[] = "**ssmwrite";
static const char long_gen380[]  = "sock/shared memory write failed";
static const char short_gen381[] = "**ssmwritev";
static const char long_gen381[]  = "sock/shared memory writev failed";
static const char short_gen382[] = "**stride";
static const char long_gen382[]  = "Range does not terminate";
static const char short_gen383[] = "**stridezero";
static const char long_gen383[]  = "Zero stride is invalid";
static const char short_gen384[] = "**strncpy";
static const char long_gen384[]  = "insufficient buffer length to complete strncpy";
static const char short_gen385[] = "**success";
static const char long_gen385[]  = "No MPI error";
static const char short_gen386[] = "**tag";
static const char long_gen386[]  = "Invalid tag";
static const char short_gen387[] = "**test_sock_wait";
static const char long_gen387[]  = "sock_wait failed";
static const char short_gen388[] = "**toomanycomm";
static const char long_gen388[]  = "Too many communicators";
static const char short_gen389[] = "**topology";
static const char long_gen389[]  = "Invalid topology";
static const char short_gen390[] = "**topotoolarge";
static const char long_gen390[]  = "Topology size is greater than communicator size";
static const char short_gen391[] = "**truncate";
static const char long_gen391[]  = "Message truncated";
static const char short_gen392[] = "**typenamelen";
static const char long_gen392[]  = " Specified datatype name is too long";
static const char short_gen393[] = "**unknown";
static const char long_gen393[]  = "Unknown error.  Please file a bug report.";
static const char short_gen394[] = "**unsupporteddatarep";
static const char long_gen394[]  = "Only native data representation currently supported";
static const char short_gen395[] = "**vc_state";
static const char long_gen395[]  = "invalid vc state";
static const char short_gen396[] = "**win";
static const char long_gen396[]  = "Invalid MPI_Win";
static const char short_gen397[] = "**winwait";
static const char long_gen397[]  = "WaitForSingleObject failed";
static const char short_gen398[] = "**write_progress";
static const char long_gen398[]  = "Write progress failed";
static const char short_gen399[] = "**wsasock";
static const char long_gen399[]  = "WSAStartup failed";

static const int generic_msgs_len = 400;
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
{ short_gen399, long_gen399 }
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
static const char short_spc34[] = "**ch3|sock|connrefused %d %d %s";
static const char long_spc34[]  = "[ch3:sock] failed to connect to process %d:%d (%s)";
static const char short_spc35[] = "**ch3|sock|failure %d";
static const char long_spc35[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc36[] = "**ch3|sock|hostlookup %d %d %s";
static const char long_spc36[]  = "[ch3:sock] failed to obtain host information for process %d:%d (%s)";
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
static const char short_spc66[] = "**init_comm_create %d";
static const char long_spc66[]  = "unable to create an intercommunicator for the parent (error %d)";
static const char short_spc67[] = "**intern %s";
static const char long_spc67[]  = "Internal MPI error!  %s";
static const char short_spc68[] = "**invalid_handle %d";
static const char long_spc68[]  = "invalid handle (%d)";
static const char short_spc69[] = "**invalid_listener %p";
static const char long_spc69[]  = "invalid listener (%p)";
static const char short_spc70[] = "**invalid_refcount %d";
static const char long_spc70[]  = "invalid reference count (%d)";
static const char short_spc71[] = "**io %s";
static const char long_spc71[]  = "Other I/O error %s";
static const char short_spc72[] = "**iocp %d";
static const char long_spc72[]  = "unable to create an I/O completion port (errno %d)";
static const char short_spc73[] = "**iov_offset %d %d";
static const char long_spc73[]  = "invalid iov offset (%d > %d)";
static const char short_spc74[] = "**listen %d";
static const char long_spc74[]  = "listen failed (errno %d)";
static const char short_spc75[] = "**mmap %d";
static const char long_spc75[]  = " unable to map memory, mmap failed (errno %d)";
static const char short_spc76[] = "**mq_close %d";
static const char long_spc76[]  = "failed to close a posix message queue, error %d";
static const char short_spc77[] = "**mq_open %d";
static const char long_spc77[]  = "failed to open a posix message queue, error %d";
static const char short_spc78[] = "**mq_receive %d";
static const char long_spc78[]  = "failed to receive a posix message queue message, error %d";
static const char short_spc79[] = "**mq_send %d";
static const char long_spc79[]  = "failed to send a posix message queue message, error %d";
static const char short_spc80[] = "**msgctl %d";
static const char long_spc80[]  = "msgctl returned %d";
static const char short_spc81[] = "**msgget %d";
static const char long_spc81[]  = "msgget returned %d";
static const char short_spc82[] = "**msgrcv %d";
static const char long_spc82[]  = "msgrcv returned %d";
static const char short_spc83[] = "**msgsnd %d";
static const char long_spc83[]  = "msgsnd returned %d";
static const char short_spc84[] = "**namepublish %s";
static const char long_spc84[]  = "Unable to publish service name %s";
static const char short_spc85[] = "**namepubnotpub %s";
static const char long_spc85[]  = "Lookup failed for service name %s";
static const char short_spc86[] = "**nomem %s %d";
static const char long_spc86[]  = "Out of memory (unable to allocate a '%s' of size %d)";
static const char short_spc87[] = "**nomem %s";
static const char long_spc87[]  = "Out of memory (unable to allocate a '%s')";
static const char short_spc88[] = "**notsame %s %s";
static const char long_spc88[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc89[] = "**nulledge %d %d";
static const char long_spc89[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc90[] = "**nullptr %s";
static const char long_spc90[]  = "Null pointer in parameter %s";
static const char short_spc91[] = "**nullptrtype %s";
static const char long_spc91[]  = "Null %s pointer";
static const char short_spc92[] = "**open %s %d %d";
static const char long_spc92[]  = "open(%s) failed for process %d, error %d";
static const char short_spc93[] = "**opnotpredefined %d";
static const char long_spc93[]  = "only predefined ops are valid (op = %d)";
static const char short_spc94[] = "**opundefined %s";
static const char long_spc94[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc95[] = "**opundefined_rma %d";
static const char long_spc95[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc96[] = "**pd_alloc %s";
static const char long_spc96[]  = "unable to allocate a protection domain - %s";
static const char short_spc97[] = "**pkt_ptr %p %p";
static const char long_spc97[]  = "invalid shm queue packet pointer (%p != %p)";
static const char short_spc98[] = "**pmi_barrier %d";
static const char long_spc98[]  = "PMI_Barrier returned %d";
static const char short_spc99[] = "**pmi_finalize %d";
static const char long_spc99[]  = "PMI_Finalize returned %d";
static const char short_spc100[] = "**pmi_get_rank %d";
static const char long_spc100[]  = "PMI_Get_rank returned %d";
static const char short_spc101[] = "**pmi_get_size %d";
static const char long_spc101[]  = "PMI_Get_size returned %d";
static const char short_spc102[] = "**pmi_init %d";
static const char long_spc102[]  = "PMI_Init returned %d";
static const char short_spc103[] = "**pmi_kvs_commit %d";
static const char long_spc103[]  = "PMI_KVS_Commit returned %d";
static const char short_spc104[] = "**pmi_kvs_create %d";
static const char long_spc104[]  = "PMI_KVS_Create returned %d";
static const char short_spc105[] = "**pmi_kvs_get %d";
static const char long_spc105[]  = "PMI_KVS_Get returned %d";
static const char short_spc106[] = "**pmi_kvs_get_my_name %d";
static const char long_spc106[]  = "PMI_KVS_Get_my_name returned %d";
static const char short_spc107[] = "**pmi_kvs_get_parent %d";
static const char long_spc107[]  = "unable to get the PARENT_ROOT_PORT_NAME from the keyval space (pmi_error %d)";
static const char short_spc108[] = "**pmi_kvs_put %d";
static const char long_spc108[]  = "PMI_KVS_Put returned %d";
static const char short_spc109[] = "**pmi_spawn_multiple %d";
static const char long_spc109[]  = "PMI_Spawn_multiple returned %d";
static const char short_spc110[] = "**post_connect %s";
static const char long_spc110[]  = "%s failed in VC_post_connect";
static const char short_spc111[] = "**rangedup %d %d %d";
static const char long_spc111[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc112[] = "**rangeendinvalid %d %d %d";
static const char long_spc112[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc113[] = "**rangestartinvalid %d %d %d";
static const char long_spc113[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc114[] = "**rank %d %d";
static const char long_spc114[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc115[] = "**rankarray %d %d %d";
static const char long_spc115[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc116[] = "**rankdup %d %d %d";
static const char long_spc116[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc117[] = "**ranklocal %d %d";
static const char long_spc117[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc118[] = "**rankremote %d %d";
static const char long_spc118[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc119[] = "**rmasize %d";
static const char long_spc119[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc120[] = "**root %d";
static const char long_spc120[]  = "Invalid root (value given was %d)";
static const char short_spc121[] = "**rsendnomatch %d %d";
static const char long_spc121[]  = "Ready send from source %d and with tag %d had no matching receive";
static const char short_spc122[] = "**shm_op %d";
static const char long_spc122[]  = "invalid shm operation (%d)";
static const char short_spc123[] = "**shm_open %s %d";
static const char long_spc123[]  = "unable to open shared memory object %s (errno %d)";
static const char short_spc124[] = "**shm_unlink %s %d";
static const char long_spc124[]  = "failed to unlink shared memory object %s, error %d";
static const char short_spc125[] = "**shmat %d";
static const char long_spc125[]  = "shmat failed, error %d";
static const char short_spc126[] = "**shmctl %d %d";
static const char long_spc126[]  = "failed to mark the sysv segment %d for removal, error %d";
static const char short_spc127[] = "**shmget %d";
static const char long_spc127[]  = "shmget failed, error %d";
static const char short_spc128[] = "**shmhost %s %s";
static const char long_spc128[]  = "process not on the same host (%s != %s)";
static const char short_spc129[] = "**shmq_index %d %d";
static const char long_spc129[]  = "invalid shm queue index (%d > %d)";
static const char short_spc130[] = "**snprintf %d";
static const char long_spc130[]  = "snprintf returned %d";
static const char short_spc131[] = "**sock_byname %d";
static const char long_spc131[]  = "gethostbyname failed (errno %d)";
static const char short_spc132[] = "**sock_connect %d";
static const char long_spc132[]  = "connect failed (errno %d)";
static const char short_spc133[] = "**sock_create %d";
static const char long_spc133[]  = "unable to create a socket (errno %d)";
static const char short_spc134[] = "**sock_gethost %d";
static const char long_spc134[]  = "gethostname failed (errno %d)";
static const char short_spc135[] = "**socket %d";
static const char long_spc135[]  = "WSASocket failed (errno %d)";
static const char short_spc136[] = "**sock|badbuf %d %d";
static const char long_spc136[]  = "the supplied buffer contains invalid memory (set=%d,sock=%d)";
static const char short_spc137[] = "**sock|badiovn %d %d %d";
static const char long_spc137[]  = "size of iov is invalid (set=%d,sock=%d,iov_n=%d)";
static const char short_spc138[] = "**sock|badlen %d %d %d %d";
static const char long_spc138[]  = "bad length parameter(s) (set=%d,sock=%d,min=%d,max=%d)";
static const char short_spc139[] = "**sock|badport %d";
static const char long_spc139[]  = "port number is out of range (sock=%d)";
static const char short_spc140[] = "**sock|closing %d %d";
static const char long_spc140[]  = "sock is in the process of being closed locally (set=%d,sock=%d)";
static const char short_spc141[] = "**sock|connclosed %d %d";
static const char long_spc141[]  = "connection closed by peer (set=%d,sock=%d)";
static const char short_spc142[] = "**sock|connfailed %d %d";
static const char long_spc142[]  = "connection failure (set=%d,sock=%d)";
static const char short_spc143[] = "**sock|notconnected %d %d";
static const char long_spc143[]  = "sock is not connected (set=%d,sock=%d)";
static const char short_spc144[] = "**sock|osnomem %d %d";
static const char long_spc144[]  = "operating system routine failed due to lack of memory (set=%d,sock=%d)";
static const char short_spc145[] = "**sock|poll|accept %d";
static const char long_spc145[]  = "accept failed to acquire a new socket (errno=%d)";
static const char short_spc146[] = "**sock|poll|badhandle %d %d %d";
static const char long_spc146[]  = "sock contains an invalid handle (set=%d,sock=%d,fd=%d)";
static const char short_spc147[] = "**sock|poll|bind %d %d";
static const char long_spc147[]  = "unable to bind socket to port (port=%d,errno=%d)";
static const char short_spc148[] = "**sock|poll|connfailed %d %d %d";
static const char long_spc148[]  = "connection failure (set=%d,sock=%d,errno=%d)";
static const char short_spc149[] = "**sock|poll|connfailed %d";
static const char long_spc149[]  = "connection failure (errno=%d)";
static const char short_spc150[] = "**sock|poll|connrefused %d %d %s";
static const char long_spc150[]  = "connection refused (set=%d,sock=%d,host=%s)";
static const char short_spc151[] = "**sock|poll|eqfail %d %d %d";
static const char long_spc151[]  = "fatal error: failed to enqueue an event; event was lost (set=%d,sock=%d,op=%d)";
static const char short_spc152[] = "**sock|poll|eqfail %d";
static const char long_spc152[]  = "fatal error: failed to enqueue an event; event was lost (op=%d)";
static const char short_spc153[] = "**sock|poll|getport %d";
static const char long_spc153[]  = "failed to obtain port number of listener (errno=%d)";
static const char short_spc154[] = "**sock|poll|hostres %d %d %s";
static const char long_spc154[]  = "unable to resolve host name to an address (set=%d,sock=%d,host=%s)";
static const char short_spc155[] = "**sock|poll|listen %d";
static const char long_spc155[]  = "listen() failed (errno=%d)";
static const char short_spc156[] = "**sock|poll|nodelay %d";
static const char long_spc156[]  = "unable to set TCP no delay attribute on socket (errno=%d)";
static const char short_spc157[] = "**sock|poll|nonblock %d";
static const char long_spc157[]  = "unable to set socket to nonblocking (errno=%d)";
static const char short_spc158[] = "**sock|poll|oserror %d %d %d";
static const char long_spc158[]  = "unknown operating system error (set=%d,sock=%d,errno=%d)";
static const char short_spc159[] = "**sock|poll|oserror %d";
static const char long_spc159[]  = "unknown operating system error (errno=%d)";
static const char short_spc160[] = "**sock|poll|pipe %d";
static const char long_spc160[]  = "unable to allocate pipe to wakeup a blocking poll() (errno=%d)";
static const char short_spc161[] = "**sock|poll|pipenonblock %d";
static const char long_spc161[]  = "unable to set wakeup pipe to nonblocking (errno=%d)";
static const char short_spc162[] = "**sock|poll|reuseaddr %d";
static const char long_spc162[]  = "unable to set reuseaddr attribute on socket (errno=%d)";
static const char short_spc163[] = "**sock|poll|socket %d";
static const char long_spc163[]  = "unable to obtain new socket (errno=%d)";
static const char short_spc164[] = "**sock|poll|unhandledstate %d %s";
static const char long_spc164[]  = "encountered an unhandled state (%d) while processing %s";
static const char short_spc165[] = "**sock|reads %d %d";
static const char long_spc165[]  = "attempt to perform multiple simultaneous reads (set=%d,sock=%d)";
static const char short_spc166[] = "**sock|writes %d %d";
static const char long_spc166[]  = "attempt to perform multiple simultaneous writes (set=%d,sock=%d)";
static const char short_spc167[] = "**stride %d %d %d";
static const char long_spc167[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc168[] = "**tag %d";
static const char long_spc168[]  = "Invalid tag, value is %d";
static const char short_spc169[] = "**topotoolarge %d %d";
static const char long_spc169[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc170[] = "**truncate %d %d %d %d";
static const char long_spc170[]  = "Message from rank %d and tag %d truncated %d bytes received but buffer size is %d";
static const char short_spc171[] = "**truncate %d %d";
static const char long_spc171[]  = "Message truncated; %d bytes received but buffer size is %d";
static const char short_spc172[] = "**typenamelen %d";
static const char long_spc172[]  = " Specified datatype name is too long (%d characters)";
static const char short_spc173[] = "**vc_state %d";
static const char long_spc173[]  = "invalid vc state (%d)";
static const char short_spc174[] = "**wsasock %d";
static const char long_spc174[]  = "WSAStartup failed (errno %d)";

static const int specific_msgs_len = 175;
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
{ short_spc174, long_spc174 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
385,41,103,111,386,94,300,315,149,261,
389,108,10,393,391,266,184,183,182,308,
123,124,130,102,105,128,129,122,160,161,
167,166,190,241,9,257,134,133,285,136,
137,317,379,106,135,396,27,222,216,311,
314,313,312,20};
#endif
