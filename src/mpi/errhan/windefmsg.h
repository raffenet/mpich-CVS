/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * This file automatically created by extracterrmsgs
 * DO NOT EDIT
 */
typedef struct {
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
static const char short_gen8[] = "**allocmem";
static const char long_gen8[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short_gen9[] = "**arg";
static const char long_gen9[]  = "Invalid argument";
static const char short_gen10[] = "**argarrayneg";
static const char long_gen10[]  = "Negative value in array ";
static const char short_gen11[] = "**argneg";
static const char long_gen11[]  = "Invalid argument; must be non-negative";
static const char short_gen12[] = "**argnonpos";
static const char long_gen12[]  = "Invalid argument; must be positive";
static const char short_gen13[] = "**argrange";
static const char long_gen13[]  = "Argument is not within valid range";
static const char short_gen14[] = "**assert";
static const char long_gen14[]  = "Invalid assert argument";
static const char short_gen15[] = "**attach_to_mem";
static const char long_gen15[]  = "attach to shared memory segment failed";
static const char short_gen16[] = "**attrsentinal";
static const char long_gen16[]  = "Internal fields in an attribute have been overwritten; possible errors in using the attribute value in user code.";
static const char short_gen17[] = "**badpacket";
static const char long_gen17[]  = "Received a packet of unknown type";
static const char short_gen18[] = "**badsock";
static const char long_gen18[]  = "internal error - bad sock";
static const char short_gen19[] = "**base";
static const char long_gen19[]  = "Invalid base address";
static const char short_gen20[] = "**boot_attach";
static const char long_gen20[]  = "failed to attach to a bootstrap queue";
static const char short_gen21[] = "**boot_detach";
static const char long_gen21[]  = "detaching from message queue failed";
static const char short_gen22[] = "**boot_recv";
static const char long_gen22[]  = "receiving bootstrap message failed";
static const char short_gen23[] = "**boot_send";
static const char long_gen23[]  = "sending bootstrap message failed";
static const char short_gen24[] = "**boot_tostring";
static const char long_gen24[]  = "unable to get a string representation of the boostrap queue";
static const char short_gen25[] = "**bsendbufsmall";
static const char long_gen25[]  = "Buffer size is smaller than MPI_BSEND_OVERHEAD";
static const char short_gen26[] = "**bsendnobuf";
static const char long_gen26[]  = "No buffer to detach. ";
static const char short_gen27[] = "**bufalias";
static const char long_gen27[]  = "Buffers must not be aliased";
static const char short_gen28[] = "**bufbsend";
static const char long_gen28[]  = "Insufficient space in Bsend buffer";
static const char short_gen29[] = "**bufexists";
static const char long_gen29[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short_gen30[] = "**buffer";
static const char long_gen30[]  = "Invalid buffer pointer";
static const char short_gen31[] = "**bufnull";
static const char long_gen31[]  = "Null buffer pointer";
static const char short_gen32[] = "**business_card";
static const char long_gen32[]  = "Invalid business card";
static const char short_gen33[] = "**cancelunknown";
static const char long_gen33[]  = "Attempt to cancel an unknown type of request";
static const char short_gen34[] = "**cartcoordinvalid";
static const char long_gen34[]  = "Cartesian coordinate is invalid (not in range)";
static const char short_gen35[] = "**cartdim";
static const char long_gen35[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short_gen36[] = "**cartshiftzero";
static const char long_gen36[]  = "Displacement must be non-zero";
static const char short_gen37[] = "**ch3_finalize";
static const char long_gen37[]  = "Channel finalization failed";
static const char short_gen38[] = "**ch3_init";
static const char long_gen38[]  = "Channel init failed";
static const char short_gen39[] = "**ch3_send";
static const char long_gen39[]  = "send failed";
static const char short_gen40[] = "**ch3ireadaggressive";
static const char long_gen40[]  = "aggressive reading failed";
static const char short_gen41[] = "**ch3progress";
static const char long_gen41[]  = "Unable to make message passing progress";
static const char short_gen42[] = "**ch3|badca";
static const char long_gen42[]  = "specified completion action in not known";
static const char short_gen43[] = "**ch3|badmsgtype";
static const char long_gen43[]  = "request contained an invalid message type";
static const char short_gen44[] = "**ch3|badreqtype";
static const char long_gen44[]  = "request contained an invalid request type";
static const char short_gen45[] = "**ch3|canceleager";
static const char long_gen45[]  = "failure occurred while performing local cancellation of a eager message";
static const char short_gen46[] = "**ch3|cancelreq";
static const char long_gen46[]  = "failure occurred while sending remote cancellation request packet";
static const char short_gen47[] = "**ch3|cancelresp";
static const char long_gen47[]  = "failure occurred while attempting to send cancel response packet";
static const char short_gen48[] = "**ch3|cancelrndv";
static const char long_gen48[]  = "failure occurred while performing local cancellation of a rendezvous message";
static const char short_gen49[] = "**ch3|ctspkt";
static const char long_gen49[]  = "failure occurred while attempting to send CTS packet";
static const char short_gen50[] = "**ch3|eagermsg";
static const char long_gen50[]  = "failure occurred while attempting to send an eager message";
static const char short_gen51[] = "**ch3|flowcntlpkt";
static const char long_gen51[]  = "UNIMPLEMENTED: unable to handle flow control packets";
static const char short_gen52[] = "**ch3|loadrecviov";
static const char long_gen52[]  = "failure occurred while loading the receive I/O vector";
static const char short_gen53[] = "**ch3|loadsendiov";
static const char long_gen53[]  = "failure occurred while loading the send I/O vector";
static const char short_gen54[] = "**ch3|nopktcontainermem";
static const char long_gen54[]  = "failed to allocate memory for a packet reorder container";
static const char short_gen55[] = "**ch3|ooocancelreq";
static const char long_gen55[]  = "UNIMPLEMENTED: unable to process out-of-order cancellation requests";
static const char short_gen56[] = "**ch3|pktordered";
static const char long_gen56[]  = "failure occurred while processing a reordered packet";
static const char short_gen57[] = "**ch3|postrecv";
static const char long_gen57[]  = "failure occurred while posting a receive for message data";
static const char short_gen58[] = "**ch3|putpkt";
static const char long_gen58[]  = "UNIMPLEMENTED: unable to handling put packets";
static const char short_gen59[] = "**ch3|recvdata";
static const char long_gen59[]  = "failure occurred while attempting to receive message data";
static const char short_gen60[] = "**ch3|rtspkt";
static const char long_gen60[]  = "failure occurred while attempting to send RTS packet";
static const char short_gen61[] = "**ch3|selfsenddeadlock";
static const char long_gen61[]  = "DEADLOCK: attempting to send a message to the local process without a prior matching receive";
static const char short_gen62[] = "**ch3|senddata";
static const char long_gen62[]  = "failure occurred while attempting to send message data";
static const char short_gen63[] = "**ch3|sock|badbuscard";
static const char long_gen63[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen64[] = "**ch3|sock|badpacket";
static const char long_gen64[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen65[] = "**ch3|sock|badsock";
static const char long_gen65[]  = "[ch3:sock] internal error - bad sock";
static const char short_gen66[] = "**ch3|sock|connallocfailed";
static const char long_gen66[]  = "[ch3:sock] unable to allocate a connection structure";
static const char short_gen67[] = "**ch3|sock|connclose";
static const char long_gen67[]  = "[ch3:sock] active connection unexpectedly closed";
static const char short_gen68[] = "**ch3|sock|connfailed";
static const char long_gen68[]  = "[ch3:sock] failed to connnect to remote process";
static const char short_gen69[] = "**ch3|sock|connrefused";
static const char long_gen69[]  = "[ch3:sock] connection refused";
static const char short_gen70[] = "**ch3|sock|connterm";
static const char long_gen70[]  = "[ch3:sock] active connection unexpectedly terminated";
static const char short_gen71[] = "**ch3|sock|failure";
static const char long_gen71[]  = "[ch3:sock] unknown failure";
static const char short_gen72[] = "**ch3|sock|hostlookup";
static const char long_gen72[]  = "[ch3:sock] hostname lookup failed";
static const char short_gen73[] = "**ch3|sock|pmi_finalize";
static const char long_gen73[]  = "PMI_Finalize failed";
static const char short_gen74[] = "**ch3|sock|strdup";
static const char long_gen74[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen75[] = "**ch3|syncack";
static const char long_gen75[]  = "failure occurred while attempting to send eager synchronization packet";
static const char short_gen76[] = "**ch3|unknownpkt";
static const char long_gen76[]  = "received unknown packet type";
static const char short_gen77[] = "**comm";
static const char long_gen77[]  = "Invalid communicator";
static const char short_gen78[] = "**commnotinter";
static const char long_gen78[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen79[] = "**commnotintra";
static const char long_gen79[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen80[] = "**commnull";
static const char long_gen80[]  = "Null communicator";
static const char short_gen81[] = "**commperm";
static const char long_gen81[]  = "Cannot free permanent communicator";
static const char short_gen82[] = "**connclose";
static const char long_gen82[]  = "active connection unexpectedly closed";
static const char short_gen83[] = "**connfailed";
static const char long_gen83[]  = "Failed to connect to remote process";
static const char short_gen84[] = "**connrefused";
static const char long_gen84[]  = "Connection refused";
static const char short_gen85[] = "**connterm";
static const char long_gen85[]  = "active connection unexpectedly terminated";
static const char short_gen86[] = "**conversion";
static const char long_gen86[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen87[] = "**count";
static const char long_gen87[]  = "Invalid count";
static const char short_gen88[] = "**countneg";
static const char long_gen88[]  = "Negative count";
static const char short_gen89[] = "**datarep";
static const char long_gen89[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen90[] = "**datarepunsupported";
static const char long_gen90[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen91[] = "**dims";
static const char long_gen91[]  = "Invalid dimension argument";
static const char short_gen92[] = "**dimsmany";
static const char long_gen92[]  = "Number of dimensions is too large ";
static const char short_gen93[] = "**dimspartition";
static const char long_gen93[]  = "Cannot partition nodes as requested ";
static const char short_gen94[] = "**dtype";
static const char long_gen94[]  = "Invalid datatype";
static const char short_gen95[] = "**dtypecommit";
static const char long_gen95[]  = "Datatype has not been committed ";
static const char short_gen96[] = "**dtypemismatch";
static const char long_gen96[]  = "Receiving data with a datatype whose signature does not match that of the sending datatype.";
static const char short_gen97[] = "**dtypenull";
static const char long_gen97[]  = "Null datatype";
static const char short_gen98[] = "**dtypeperm";
static const char long_gen98[]  = "Cannot free permanent data type ";
static const char short_gen99[] = "**dupprocesses";
static const char long_gen99[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen100[] = "**edgeoutrange";
static const char long_gen100[]  = "Edge index in graph topology is out of range";
static const char short_gen101[] = "**failure";
static const char long_gen101[]  = "unknown failure";
static const char short_gen102[] = "**file";
static const char long_gen102[]  = "Invalid MPI_File";
static const char short_gen103[] = "**fileaccess";
static const char long_gen103[]  = "Access denied to file";
static const char short_gen104[] = "**fileamode";
static const char long_gen104[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen105[] = "**fileamodeone";
static const char long_gen105[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen106[] = "**fileamoderead";
static const char long_gen106[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen107[] = "**fileamodeseq";
static const char long_gen107[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen108[] = "**fileexist";
static const char long_gen108[]  = "File exists";
static const char short_gen109[] = "**fileinuse";
static const char long_gen109[]  = "File in use by some process";
static const char short_gen110[] = "**filename";
static const char long_gen110[]  = "Invalid file name";
static const char short_gen111[] = "**filenoexist";
static const char long_gen111[]  = "File does not exist";
static const char short_gen112[] = "**filenospace";
static const char long_gen112[]  = "Not enough space for file ";
static const char short_gen113[] = "**fileopunsupported";
static const char long_gen113[]  = "Unsupported file operation ";
static const char short_gen114[] = "**filequota";
static const char long_gen114[]  = "Quota exceeded for files";
static const char short_gen115[] = "**filerdonly";
static const char long_gen115[]  = "Read-only file or filesystem name";
static const char short_gen116[] = "**finalize_boot";
static const char long_gen116[]  = "destroying the message queue failed";
static const char short_gen117[] = "**finalize_progress";
static const char long_gen117[]  = "finalizing the progress engine failed";
static const char short_gen118[] = "**finalize_progress_finalize";
static const char long_gen118[]  = "Progress finalize failed";
static const char short_gen119[] = "**finalize_release_mem";
static const char long_gen119[]  = "Release shared memory failed";
static const char short_gen120[] = "**graphnnodes";
static const char long_gen120[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen121[] = "**group";
static const char long_gen121[]  = "Invalid group";
static const char short_gen122[] = "**groupnotincomm";
static const char long_gen122[]  = "Specified group is not within the communicator";
static const char short_gen123[] = "**handle_read";
static const char long_gen123[]  = "Unable to handle the read data";
static const char short_gen124[] = "**handle_sock_op";
static const char long_gen124[]  = "handle_sock_op failed";
static const char short_gen125[] = "**hostlookup";
static const char long_gen125[]  = "Host lookup failed";
static const char short_gen126[] = "**indexneg";
static const char long_gen126[]  = "Index value in graph topology must be nonnegative";
static const char short_gen127[] = "**indexnonmonotone";
static const char long_gen127[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen128[] = "**info";
static const char long_gen128[]  = "Invalid MPI_Info";
static const char short_gen129[] = "**infokey";
static const char long_gen129[]  = "Invalid key for MPI_Info ";
static const char short_gen130[] = "**infokeyempty";
static const char long_gen130[]  = "Empty or blank key ";
static const char short_gen131[] = "**infokeylong";
static const char long_gen131[]  = "Key is too long";
static const char short_gen132[] = "**infokeynull";
static const char long_gen132[]  = "Null key";
static const char short_gen133[] = "**infonkey";
static const char long_gen133[]  = "Requested nth key does not exist";
static const char short_gen134[] = "**infonokey";
static const char long_gen134[]  = "MPI_Info key is not defined ";
static const char short_gen135[] = "**infoval";
static const char long_gen135[]  = "Invalid MPI_Info value ";
static const char short_gen136[] = "**infovallong";
static const char long_gen136[]  = "Value is too long ";
static const char short_gen137[] = "**infovalnull";
static const char long_gen137[]  = "Null value";
static const char short_gen138[] = "**init";
static const char long_gen138[]  = "Initialization failed";
static const char short_gen139[] = "**init_buscard";
static const char long_gen139[]  = "failed to get my business card";
static const char short_gen140[] = "**init_getptr";
static const char long_gen140[]  = "failed to get the vcr";
static const char short_gen141[] = "**init_progress";
static const char long_gen141[]  = "progress_init failed";
static const char short_gen142[] = "**init_strtok_host";
static const char long_gen142[]  = "failed to copy the hostname from the business card";
static const char short_gen143[] = "**init_vcrt";
static const char long_gen143[]  = "failed to create VCRT";
static const char short_gen144[] = "**initialized";
static const char long_gen144[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen145[] = "**inittwice";
static const char long_gen145[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen146[] = "**inpending";
static const char long_gen146[]  = "Pending request (no error)";
static const char short_gen147[] = "**instatus";
static const char long_gen147[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen148[] = "**intercommcoll";
static const char long_gen148[]  = "Intercommunicator collective operations have not been implemented";
static const char short_gen149[] = "**intern";
static const char long_gen149[]  = "Internal MPI error!";
static const char short_gen150[] = "**io";
static const char long_gen150[]  = "Other I/O error ";
static const char short_gen151[] = "**ioRMWrdwr";
static const char long_gen151[]  = "Must open file with MPI_MODE_RDWR for read-modify-write ";
static const char short_gen152[] = "**ioagnomatch";
static const char long_gen152[]  = "No aggregators match";
static const char short_gen153[] = "**ioamodeseq";
static const char long_gen153[]  = "Cannot use this function when the file is opened with amode MPI_MODE_SEQUENTIAL ";
static const char short_gen154[] = "**iobadcount";
static const char long_gen154[]  = "Invalid count argument";
static const char short_gen155[] = "**iobaddisp";
static const char long_gen155[]  = "Invalid displacement argument";
static const char short_gen156[] = "**iobadfh";
static const char long_gen156[]  = "Invalid file handle";
static const char short_gen157[] = "**iobadoffset";
static const char long_gen157[]  = "Invalid offset argument";
static const char short_gen158[] = "**iobadsize";
static const char long_gen158[]  = "Invalid size argument";
static const char short_gen159[] = "**iobadwhence";
static const char long_gen159[]  = "Invalid whence argument";
static const char short_gen160[] = "**iodatarepnomem";
static const char long_gen160[]  = "User must allocate memory for datarep";
static const char short_gen161[] = "**iodispifseq";
static const char long_gen161[]  = "disp must be set to MPI_DISPLACEMENT_CURRENT since file was opened with MPI_MODE_SEQUENTIAL";
static const char short_gen162[] = "**ioetype";
static const char long_gen162[]  = "Only an integral number of etypes can be accessed";
static const char short_gen163[] = "**iofiletype";
static const char long_gen163[]  = "Filetype must be constructed out of one or more etypes";
static const char short_gen164[] = "**iofstype";
static const char long_gen164[]  = "Cannot determine filesystem type";
static const char short_gen165[] = "**iofstypeunsupported";
static const char long_gen165[]  = "Specified filesystem is not available";
static const char short_gen166[] = "**ioneedrd";
static const char long_gen166[]  = "Read access is required to this file";
static const char short_gen167[] = "**ionegoffset";
static const char long_gen167[]  = "Negative offset argument";
static const char short_gen168[] = "**iopreallocrdwr";
static const char long_gen168[]  = "Must open file with MPI_MODE_RDWR to preallocate disk space";
static const char short_gen169[] = "**iosequnsupported";
static const char long_gen169[]  = "MPI_MODE_SEQUENTIAL not supported on this file system";
static const char short_gen170[] = "**iosharedunsupported";
static const char long_gen170[]  = "Shared file pointers not supported";
static const char short_gen171[] = "**iosplitcoll";
static const char long_gen171[]  = "Only one active split collective I/O operation is allowed per file handle";
static const char short_gen172[] = "**iosplitcollnone";
static const char long_gen172[]  = "No split collective I/O operation is active";
static const char short_gen173[] = "**keyval";
static const char long_gen173[]  = "Invalid keyval";
static const char short_gen174[] = "**keyvalinvalid";
static const char long_gen174[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen175[] = "**keyvalnotcomm";
static const char long_gen175[]  = "Keyval was not defined for communicators";
static const char short_gen176[] = "**keyvalnotdatatype";
static const char long_gen176[]  = "Keyval was not defined for datatype";
static const char short_gen177[] = "**locktype";
static const char long_gen177[]  = "Invalid locktype";
static const char short_gen178[] = "**msgctl";
static const char long_gen178[]  = "msgctl failed";
static const char short_gen179[] = "**msgget";
static const char long_gen179[]  = "msgget failed";
static const char short_gen180[] = "**msgrcv";
static const char long_gen180[]  = "msgrcv failed";
static const char short_gen181[] = "**msgsnd";
static const char long_gen181[]  = "msgsnd failed";
static const char short_gen182[] = "**namepublish";
static const char long_gen182[]  = "Unable to publish service name";
static const char short_gen183[] = "**namepubnotpub";
static const char long_gen183[]  = "Lookup failed for service name ";
static const char short_gen184[] = "**nameservice";
static const char long_gen184[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen185[] = "**nextbootmsg";
static const char long_gen185[]  = "failed to get the next bootstrap message";
static const char short_gen186[] = "**noerrclasses";
static const char long_gen186[]  = "No more user-defined error classes";
static const char short_gen187[] = "**noerrcodes";
static const char long_gen187[]  = "No more user-defined error codes";
static const char short_gen188[] = "**nomem";
static const char long_gen188[]  = "Out of memory";
static const char short_gen189[] = "**nomemreq";
static const char long_gen189[]  = "failure occurred while allocating memory for a request object";
static const char short_gen190[] = "**nonamepub";
static const char long_gen190[]  = "No name publishing service available";
static const char short_gen191[] = "**notcarttopo";
static const char long_gen191[]  = "No Cartesian topology associated with this communicator";
static const char short_gen192[] = "**notgenreq";
static const char long_gen192[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen193[] = "**notgraphtopo";
static const char long_gen193[]  = "No Graph topology associated with this communicator";
static const char short_gen194[] = "**notimpl";
static const char long_gen194[]  = "Function not implemented";
static const char short_gen195[] = "**notopology";
static const char long_gen195[]  = "No topology associated with this communicator";
static const char short_gen196[] = "**notsame";
static const char long_gen196[]  = "Inconsistent arguments to collective routine ";
static const char short_gen197[] = "**nulledge";
static const char long_gen197[]  = "Edge detected from a node to the same node";
static const char short_gen198[] = "**nullptr";
static const char long_gen198[]  = "Null pointer";
static const char short_gen199[] = "**nullptrtype";
static const char long_gen199[]  = "Null pointer";
static const char short_gen200[] = "**op";
static const char long_gen200[]  = "Invalid MPI_Op";
static const char short_gen201[] = "**open";
static const char long_gen201[]  = "open failed";
static const char short_gen202[] = "**opnotpredefined";
static const char long_gen202[]  = "only predefined ops are valid";
static const char short_gen203[] = "**opundefined";
static const char long_gen203[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen204[] = "**opundefined_rma";
static const char long_gen204[]  = "RMA target received unknown RMA operation";
static const char short_gen205[] = "**other";
static const char long_gen205[]  = "Other MPI error";
static const char short_gen206[] = "**permattr";
static const char long_gen206[]  = "Cannot set permanent attribute";
static const char short_gen207[] = "**permop";
static const char long_gen207[]  = "Cannot free permanent MPI_Op ";
static const char short_gen208[] = "**pfinal_sockclose";
static const char long_gen208[]  = "sock_close failed";
static const char short_gen209[] = "**pmi_barrier";
static const char long_gen209[]  = "PMI_Barrier failed";
static const char short_gen210[] = "**pmi_finalize";
static const char long_gen210[]  = "PMI_Finalize failed";
static const char short_gen211[] = "**pmi_get_rank";
static const char long_gen211[]  = "PMI_Get_rank failed";
static const char short_gen212[] = "**pmi_get_size";
static const char long_gen212[]  = "PMI_Get_size failed";
static const char short_gen213[] = "**pmi_init";
static const char long_gen213[]  = "PMI_Init failed";
static const char short_gen214[] = "**pmi_kvs_commit";
static const char long_gen214[]  = "PMI_KVS_Commit failed";
static const char short_gen215[] = "**pmi_kvs_get";
static const char long_gen215[]  = "PMI_KVS_Get failed";
static const char short_gen216[] = "**pmi_kvs_get_my_name";
static const char long_gen216[]  = "PMI_KVS_Get_my_name failed";
static const char short_gen217[] = "**pmi_kvs_put";
static const char long_gen217[]  = "PMI_KVS_Put failed";
static const char short_gen218[] = "**poke";
static const char long_gen218[]  = "progress_poke failed";
static const char short_gen219[] = "**port";
static const char long_gen219[]  = "Invalid port";
static const char short_gen220[] = "**post_connect";
static const char long_gen220[]  = "failed to post a connection";
static const char short_gen221[] = "**postpkt";
static const char long_gen221[]  = "Unable to post a read for the next packet header";
static const char short_gen222[] = "**process_group";
static const char long_gen222[]  = "Process group initialization failed";
static const char short_gen223[] = "**progress_handle_sock_op";
static const char long_gen223[]  = "handle_sock_op failed";
static const char short_gen224[] = "**progress_sock_wait";
static const char long_gen224[]  = "sock_wait failed";
static const char short_gen225[] = "**rangedup";
static const char long_gen225[]  = "The range array specifies duplicate entries";
static const char short_gen226[] = "**rangeendinvalid";
static const char long_gen226[]  = "Some element of a range array is either negative or too large";
static const char short_gen227[] = "**rangestartinvalid";
static const char long_gen227[]  = "Some element of a range array is either negative or too large";
static const char short_gen228[] = "**rank";
static const char long_gen228[]  = "Invalid rank";
static const char short_gen229[] = "**rankarray";
static const char long_gen229[]  = "Invalid rank in rank array";
static const char short_gen230[] = "**rankdup";
static const char long_gen230[]  = "Duplicate ranks in rank array ";
static const char short_gen231[] = "**ranklocal";
static const char long_gen231[]  = "Error specifying local_leader ";
static const char short_gen232[] = "**rankremote";
static const char long_gen232[]  = "Error specifying remote_leader ";
static const char short_gen233[] = "**rdma_finalize";
static const char long_gen233[]  = "Channel rdma finalization failed";
static const char short_gen234[] = "**rdma_init";
static const char long_gen234[]  = "Channel rdma initialization failed";
static const char short_gen235[] = "**read_progress";
static const char long_gen235[]  = "Unable to make read progress";
static const char short_gen236[] = "**request";
static const char long_gen236[]  = "Invalid MPI_Request";
static const char short_gen237[] = "**requestnotpersist";
static const char long_gen237[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen238[] = "**requestpersistactive";
static const char long_gen238[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen239[] = "**rmaconflict";
static const char long_gen239[]  = "Conflicting accesses to window ";
static const char short_gen240[] = "**rmadisp";
static const char long_gen240[]  = "Invalid displacement argument in RMA call ";
static const char short_gen241[] = "**rmasize";
static const char long_gen241[]  = "Invalid size argument in RMA call";
static const char short_gen242[] = "**rmasync";
static const char long_gen242[]  = "Wrong synchronization of RMA calls ";
static const char short_gen243[] = "**root";
static const char long_gen243[]  = "Invalid root";
static const char short_gen244[] = "**rsendnomatch";
static const char long_gen244[]  = "Ready send had no matching receive ";
static const char short_gen245[] = "**servicename";
static const char long_gen245[]  = "Attempt to lookup an unknown service name ";
static const char short_gen246[] = "**shm_read_progress";
static const char long_gen246[]  = "shared memory read progress failed";
static const char short_gen247[] = "**shmat";
static const char long_gen247[]  = "shmat failed";
static const char short_gen248[] = "**shmconnect_getmem";
static const char long_gen248[]  = "failed to allocate shared memory for a write queue";
static const char short_gen249[] = "**shmget";
static const char long_gen249[]  = "shmget failed";
static const char short_gen250[] = "**shmgetmem";
static const char long_gen250[]  = "Unable to allocate shared memory";
static const char short_gen251[] = "**shmwrite";
static const char long_gen251[]  = "shared memory write failed";
static const char short_gen252[] = "**snprintf";
static const char long_gen252[]  = "snprintf returned an invalid number";
static const char short_gen253[] = "**sock|poll|eqmalloc";
static const char long_gen253[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen254[] = "**spawn";
static const char long_gen254[]  = "Error in spawn call";
static const char short_gen255[] = "**stride";
static const char long_gen255[]  = "Range does not terminate";
static const char short_gen256[] = "**stridezero";
static const char long_gen256[]  = "Zero stride is invalid";
static const char short_gen257[] = "**success";
static const char long_gen257[]  = "No MPI error";
static const char short_gen258[] = "**tag";
static const char long_gen258[]  = "Invalid tag";
static const char short_gen259[] = "**toomanycomm";
static const char long_gen259[]  = "Too many communicators";
static const char short_gen260[] = "**topology";
static const char long_gen260[]  = "Invalid topology";
static const char short_gen261[] = "**topotoolarge";
static const char long_gen261[]  = "Topology size is greater than communicator size";
static const char short_gen262[] = "**truncate";
static const char long_gen262[]  = "Message truncated";
static const char short_gen263[] = "**typenamelen";
static const char long_gen263[]  = " Specified datatype name is too long";
static const char short_gen264[] = "**unknown";
static const char long_gen264[]  = "Unknown error.  Please file a bug report.";
static const char short_gen265[] = "**unsupporteddatarep";
static const char long_gen265[]  = "Only native data representation currently supported";
static const char short_gen266[] = "**win";
static const char long_gen266[]  = "Invalid MPI_Win";
static const char short_gen267[] = "**winwait";
static const char long_gen267[]  = "WaitForSingleObject failed";
static const char short_gen268[] = "**write_progress";
static const char long_gen268[]  = "Write progress failed";

static const int generic_msgs_len = 269;
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
{ short_gen268, long_gen268 }
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
static const char short_spc8[] = "**argneg %s %d";
static const char long_spc8[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc9[] = "**argnonpos %s %d";
static const char long_spc9[]  = "Invalid value for %s; must be positive but is %d";
static const char short_spc10[] = "**argrange %s %d %d";
static const char long_spc10[]  = "Argument %s has value %d but must be within [0,%d]";
static const char short_spc11[] = "**attach_to_mem %d";
static const char long_spc11[]  = "attach to shared memory returned error %d";
static const char short_spc12[] = "**badpacket %d";
static const char long_spc12[]  = "Received a packet of unknown type (%d)";
static const char short_spc13[] = "**boot_attach %s";
static const char long_spc13[]  = "failed to attach to a bootstrap queue - %s";
static const char short_spc14[] = "**bsendbufsmall %d %d";
static const char long_spc14[]  = "Buffer size of %d is smaller than MPI_BSEND_OVERHEAD (%d)";
static const char short_spc15[] = "**bufbsend %d %d";
static const char long_spc15[]  = "Insufficient space in Bsend buffer; requested %d; total buffer size is %d";
static const char short_spc16[] = "**business_card %s";
static const char long_spc16[]  = "Invalid business card (%s)";
static const char short_spc17[] = "**cartcoordinvalid %d %d %d";
static const char long_spc17[]  = " Cartesian coordinate for the %d coordinate is %d but must be between 0 and %d";
static const char short_spc18[] = "**cartdim %d %d";
static const char long_spc18[]  = "Size of the communicator (%d) is smaller than the size of the Cartesian topology (%d)";
static const char short_spc19[] = "**ch3_finalize %d";
static const char long_spc19[]  = "Channel finalization failed (line %d)";
static const char short_spc20[] = "**ch3_init %d";
static const char long_spc20[]  = "Channel init failed (line %d)";
static const char short_spc21[] = "**ch3progress %d";
static const char long_spc21[]  = "Channel progress engine failed on line %d";
static const char short_spc22[] = "**ch3|badca %d";
static const char long_spc22[]  = "specified completion action in not known (%d)";
static const char short_spc23[] = "**ch3|badmsgtype %d";
static const char long_spc23[]  = "request contained an invalid message type (%d)";
static const char short_spc24[] = "**ch3|badreqtype %d";
static const char long_spc24[]  = "request contained an invalid request type (%d)";
static const char short_spc25[] = "**ch3|loadrecviov %s";
static const char long_spc25[]  = "failure occurred while loading the receive I/O vector (%s)";
static const char short_spc26[] = "**ch3|postrecv %s";
static const char long_spc26[]  = "failure occurred while posting a receive for message data (%s)";
static const char short_spc27[] = "**ch3|recvdata %s";
static const char long_spc27[]  = "failure occurred while attempting to receive message data (%s)";
static const char short_spc28[] = "**ch3|sock|badbuscard %s";
static const char long_spc28[]  = "[ch3:sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc29[] = "**ch3|sock|badpacket %d";
static const char long_spc29[]  = "[ch3:sock] received packet of unknown type (%d)";
static const char short_spc30[] = "**ch3|sock|connfailed %d %d";
static const char long_spc30[]  = "[ch3:sock] failed to connnect to remote process %d:%d";
static const char short_spc31[] = "**ch3|sock|connrefused %d %d %s";
static const char long_spc31[]  = "[ch3:sock] failed to connect to process %d:%d (%s)";
static const char short_spc32[] = "**ch3|sock|failure %d";
static const char long_spc32[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc33[] = "**ch3|sock|hostlookup %d %d %s";
static const char long_spc33[]  = "[ch3:sock] failed to obtain host information for process %d:%d (%s)";
static const char short_spc34[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc34[]  = "PMI_Finalize failed, error %d";
static const char short_spc35[] = "**ch3|unknownpkt %d";
static const char long_spc35[]  = "received unknown packet type (type=%d)";
static const char short_spc36[] = "**commperm %s";
static const char long_spc36[]  = "Cannot free permanent communicator %s";
static const char short_spc37[] = "**connfailed %d %d";
static const char long_spc37[]  = "Failed to connect to remote process %d-%d";
static const char short_spc38[] = "**connrefused %d %d %s";
static const char long_spc38[]  = "Connection refused for process group %d, rank %d, business card <%s>";
static const char short_spc39[] = "**countneg %d";
static const char long_spc39[]  = "Negative count, value is %d";
static const char short_spc40[] = "**dims %d";
static const char long_spc40[]  = "Invalid dimension argument (value is %d)";
static const char short_spc41[] = "**dimsmany %d %d";
static const char long_spc41[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc42[] = "**dupprocesses %d";
static const char long_spc42[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes; both contain process %d";
static const char short_spc43[] = "**edgeoutrange %d %d %d";
static const char long_spc43[]  = "Edge index edges[%d] is %d but must be nonnegative and less than %d";
static const char short_spc44[] = "**failure %d";
static const char long_spc44[]  = "unknown failure, error %d";
static const char short_spc45[] = "**groupnotincomm %d";
static const char long_spc45[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc46[] = "**handle_read %d";
static const char long_spc46[]  = "Unable to handle a packet (line %d)";
static const char short_spc47[] = "**hostlookup %d %d %s";
static const char long_spc47[]  = "Host lookup failed for process group %d, rank %d, business card <%s>";
static const char short_spc48[] = "**indexneg %d %d";
static const char long_spc48[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc49[] = "**indexnonmonotone %d %d %d";
static const char long_spc49[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc50[] = "**infonkey %d %d";
static const char long_spc50[]  = "Requested key %d but this MPI_Info only has %d keys";
static const char short_spc51[] = "**infonokey %s";
static const char long_spc51[]  = "MPI_Info key %s is not defined ";
static const char short_spc52[] = "**init %d";
static const char long_spc52[]  = "Initialization failed (line %d)";
static const char short_spc53[] = "**init_strtok_host %s";
static const char long_spc53[]  = "failed to copy the hostname from this business card: %s";
static const char short_spc54[] = "**intercommcoll %s";
static const char long_spc54[]  = "Intercommunicator collective operation for %s has not been implemented";
static const char short_spc55[] = "**intern %s";
static const char long_spc55[]  = "Internal MPI error!  %s";
static const char short_spc56[] = "**io %s";
static const char long_spc56[]  = "Other I/O error %s";
static const char short_spc57[] = "**msgctl %d";
static const char long_spc57[]  = "msgctl returned %d";
static const char short_spc58[] = "**msgget %d";
static const char long_spc58[]  = "msgget returned %d";
static const char short_spc59[] = "**msgrcv %d";
static const char long_spc59[]  = "msgrcv returned %d";
static const char short_spc60[] = "**msgsnd %d";
static const char long_spc60[]  = "msgsnd returned %d";
static const char short_spc61[] = "**namepublish %s";
static const char long_spc61[]  = "Unable to publish service name %s";
static const char short_spc62[] = "**namepubnotpub %s";
static const char long_spc62[]  = "Lookup failed for service name %s";
static const char short_spc63[] = "**nomem %s";
static const char long_spc63[]  = "Out of memory (unable to allocate a '%s')";
static const char short_spc64[] = "**notsame %s %s";
static const char long_spc64[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc65[] = "**nulledge %d %d";
static const char long_spc65[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc66[] = "**nullptr %s";
static const char long_spc66[]  = "Null pointer in parameter %s";
static const char short_spc67[] = "**nullptrtype %s";
static const char long_spc67[]  = "Null %s pointer";
static const char short_spc68[] = "**open %s %d %d";
static const char long_spc68[]  = "open(%s) failed for process %d, error %d";
static const char short_spc69[] = "**opnotpredefined %d";
static const char long_spc69[]  = "only predefined ops are valid (op = %d)";
static const char short_spc70[] = "**opundefined %s";
static const char long_spc70[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc71[] = "**opundefined_rma %d";
static const char long_spc71[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc72[] = "**pmi_barrier %d";
static const char long_spc72[]  = "PMI_Barrier returned %d";
static const char short_spc73[] = "**pmi_finalize %d";
static const char long_spc73[]  = "PMI_Finalize returned %d";
static const char short_spc74[] = "**pmi_get_rank %d";
static const char long_spc74[]  = "PMI_Get_rank returned %d";
static const char short_spc75[] = "**pmi_get_size %d";
static const char long_spc75[]  = "PMI_Get_size returned %d";
static const char short_spc76[] = "**pmi_init %d";
static const char long_spc76[]  = "PMI_Init returned %d";
static const char short_spc77[] = "**pmi_kvs_commit %d";
static const char long_spc77[]  = "PMI_KVS_Commit returned %d";
static const char short_spc78[] = "**pmi_kvs_get %d";
static const char long_spc78[]  = "PMI_KVS_Get returned %d";
static const char short_spc79[] = "**pmi_kvs_get_my_name %d";
static const char long_spc79[]  = "PMI_KVS_Get_my_name returned %d";
static const char short_spc80[] = "**pmi_kvs_put %d";
static const char long_spc80[]  = "PMI_KVS_Put returned %d";
static const char short_spc81[] = "**post_connect %s";
static const char long_spc81[]  = "%s failed in VC_post_connect";
static const char short_spc82[] = "**rangedup %d %d %d";
static const char long_spc82[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc83[] = "**rangeendinvalid %d %d %d";
static const char long_spc83[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc84[] = "**rangestartinvalid %d %d %d";
static const char long_spc84[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc85[] = "**rank %d %d";
static const char long_spc85[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc86[] = "**rankarray %d %d %d";
static const char long_spc86[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc87[] = "**rankdup %d %d %d";
static const char long_spc87[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc88[] = "**ranklocal %d %d";
static const char long_spc88[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc89[] = "**rankremote %d %d";
static const char long_spc89[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc90[] = "**rdma_init %d";
static const char long_spc90[]  = "Channel rdma initialization failed (line %d)";
static const char short_spc91[] = "**rmasize %d";
static const char long_spc91[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc92[] = "**root %d";
static const char long_spc92[]  = "Invalid root (value given was %d)";
static const char short_spc93[] = "**rsendnomatch %d %d";
static const char long_spc93[]  = "Ready send from source %d and with tag %d had no matching receive";
static const char short_spc94[] = "**shmat %d";
static const char long_spc94[]  = "shmat failed, error %d";
static const char short_spc95[] = "**shmget %d";
static const char long_spc95[]  = "shmget failed, error %d";
static const char short_spc96[] = "**snprintf %d";
static const char long_spc96[]  = "snprintf returned %d";
static const char short_spc97[] = "**stride %d %d %d";
static const char long_spc97[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc98[] = "**tag %d";
static const char long_spc98[]  = "Invalid tag, value is %d";
static const char short_spc99[] = "**topotoolarge %d %d";
static const char long_spc99[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc100[] = "**typenamelen %d";
static const char long_spc100[]  = " Specified datatype name is too long (%d characters)";
static const char short_spc101[] = "**write_progress %d";
static const char long_spc101[]  = "Write progress failed (line %d)";

static const int specific_msgs_len = 102;
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
{ short_spc101, long_spc101 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
257,30,87,94,258,77,228,243,121,200,
260,91,9,264,262,205,149,147,146,236,
103,104,110,86,89,108,109,102,128,129,
135,134,150,184,8,196,112,111,219,114,
115,245,254,90,113,266,19,177,173,239,
242,241,240,14};
#endif
