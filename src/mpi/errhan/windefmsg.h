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
static const char short_gen1[] = "**GetMemTwice";
static const char long_gen1[]  = "Global shared memory initializer called more than once";
static const char short_gen2[] = "**MPIDI_CH3I_SHM_Attach_to_mem";
static const char long_gen2[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed";
static const char short_gen3[] = "**MPIU_Strdup";
static const char long_gen3[]  = "MPIU_Strdup failed";
static const char short_gen4[] = "**MapViewOfFileEx";
static const char long_gen4[]  = "MapViewOfFileEx failed";
static const char short_gen5[] = "**OpenProcess";
static const char long_gen5[]  = "OpenProcess failed";
static const char short_gen6[] = "**allocmem";
static const char long_gen6[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short_gen7[] = "**arg";
static const char long_gen7[]  = "Invalid argument";
static const char short_gen8[] = "**argarrayneg";
static const char long_gen8[]  = "Negative value in array ";
static const char short_gen9[] = "**argneg";
static const char long_gen9[]  = "Invalid argument; must be non-negative";
static const char short_gen10[] = "**argnonpos";
static const char long_gen10[]  = "Invalid argument; must be positive";
static const char short_gen11[] = "**argrange";
static const char long_gen11[]  = "Argument is not within valid range";
static const char short_gen12[] = "**assert";
static const char long_gen12[]  = "Invalid assert argument";
static const char short_gen13[] = "**attrsentinal";
static const char long_gen13[]  = "Internal fields in an attribute have been overwritten; possible errors in using the attribute value in user code.";
static const char short_gen14[] = "**badpacket";
static const char long_gen14[]  = "Received a packet of unknown type";
static const char short_gen15[] = "**badsock";
static const char long_gen15[]  = "internal error - bad sock";
static const char short_gen16[] = "**base";
static const char long_gen16[]  = "Invalid base address";
static const char short_gen17[] = "**bsendbufsmall";
static const char long_gen17[]  = "Buffer size is smaller than MPI_BSEND_OVERHEAD";
static const char short_gen18[] = "**bsendnobuf";
static const char long_gen18[]  = "No buffer to detach. ";
static const char short_gen19[] = "**bufalias";
static const char long_gen19[]  = "Buffers must not be aliased";
static const char short_gen20[] = "**bufbsend";
static const char long_gen20[]  = "Insufficient space in Bsend buffer";
static const char short_gen21[] = "**bufexists";
static const char long_gen21[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short_gen22[] = "**buffer";
static const char long_gen22[]  = "Invalid buffer pointer";
static const char short_gen23[] = "**bufnull";
static const char long_gen23[]  = "Null buffer pointer";
static const char short_gen24[] = "**business_card";
static const char long_gen24[]  = "Invalid business card";
static const char short_gen25[] = "**cancelunknown";
static const char long_gen25[]  = "Attempt to cancel an unknown type of request";
static const char short_gen26[] = "**cartcoordinvalid";
static const char long_gen26[]  = "Cartesian coordinate is invalid (not in range)";
static const char short_gen27[] = "**cartdim";
static const char long_gen27[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short_gen28[] = "**cartshiftzero";
static const char long_gen28[]  = "Displacement must be non-zero";
static const char short_gen29[] = "**ch3|sock|badbuscard";
static const char long_gen29[]  = "[ch3:sock] GetHostAndPort - Invalid business card";
static const char short_gen30[] = "**ch3|sock|badpacket";
static const char long_gen30[]  = "[ch3:sock] received packet of unknow type";
static const char short_gen31[] = "**ch3|sock|badsock";
static const char long_gen31[]  = "[ch3:sock] internal error - bad sock";
static const char short_gen32[] = "**ch3|sock|connallocfailed";
static const char long_gen32[]  = "[ch3:sock] unable to allocate a connection structure";
static const char short_gen33[] = "**ch3|sock|connclose";
static const char long_gen33[]  = "[ch3:sock] active connection unexpectedly closed";
static const char short_gen34[] = "**ch3|sock|connfailed";
static const char long_gen34[]  = "[ch3:sock] failed to connnect to remote process";
static const char short_gen35[] = "**ch3|sock|connrefused";
static const char long_gen35[]  = "[ch3:sock] connection refused";
static const char short_gen36[] = "**ch3|sock|connterm";
static const char long_gen36[]  = "[ch3:sock] active connection unexpectedly terminated";
static const char short_gen37[] = "**ch3|sock|failure";
static const char long_gen37[]  = "[ch3:sock] unknown failure";
static const char short_gen38[] = "**ch3|sock|hostlookup";
static const char long_gen38[]  = "[ch3:sock] hostname lookup failed";
static const char short_gen39[] = "**ch3|sock|pmi_finalize";
static const char long_gen39[]  = "PMI_Finalize failed";
static const char short_gen40[] = "**ch3|sock|strdup";
static const char long_gen40[]  = "[ch3:sock] MPIU_Strdup failed";
static const char short_gen41[] = "**comm";
static const char long_gen41[]  = "Invalid communicator";
static const char short_gen42[] = "**commnotinter";
static const char long_gen42[]  = "An intercommunicator is required but an intracommunicator was provided.";
static const char short_gen43[] = "**commnotintra";
static const char long_gen43[]  = "An intracommunicator is required but an intercommunicator was provided.";
static const char short_gen44[] = "**commnull";
static const char long_gen44[]  = "Null communicator";
static const char short_gen45[] = "**commperm";
static const char long_gen45[]  = "Cannot free permanent communicator";
static const char short_gen46[] = "**connallocfailed";
static const char long_gen46[]  = "Connection failed";
static const char short_gen47[] = "**connclose";
static const char long_gen47[]  = "active connection unexpectedly closed";
static const char short_gen48[] = "**connfailed";
static const char long_gen48[]  = "Failed to connect to remote process";
static const char short_gen49[] = "**connrefused";
static const char long_gen49[]  = "Connection refused";
static const char short_gen50[] = "**connterm";
static const char long_gen50[]  = "active connection unexpectedly terminated";
static const char short_gen51[] = "**conversion";
static const char long_gen51[]  = "An error occurred in a user-defined data conversion function";
static const char short_gen52[] = "**count";
static const char long_gen52[]  = "Invalid count";
static const char short_gen53[] = "**countneg";
static const char long_gen53[]  = "Negative count";
static const char short_gen54[] = "**datarep";
static const char long_gen54[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP";
static const char short_gen55[] = "**datarepunsupported";
static const char long_gen55[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short_gen56[] = "**dims";
static const char long_gen56[]  = "Invalid dimension argument";
static const char short_gen57[] = "**dimsmany";
static const char long_gen57[]  = "Number of dimensions is too large ";
static const char short_gen58[] = "**dimspartition";
static const char long_gen58[]  = "Cannot partition nodes as requested ";
static const char short_gen59[] = "**dtype";
static const char long_gen59[]  = "Invalid datatype";
static const char short_gen60[] = "**dtypecommit";
static const char long_gen60[]  = "Datatype has not been committed ";
static const char short_gen61[] = "**dtypenull";
static const char long_gen61[]  = "Null datatype";
static const char short_gen62[] = "**dtypeperm";
static const char long_gen62[]  = "Cannot free permanent data type ";
static const char short_gen63[] = "**dupprocesses";
static const char long_gen63[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes";
static const char short_gen64[] = "**edgeoutrange";
static const char long_gen64[]  = "Edge index in graph topology is out of range";
static const char short_gen65[] = "**failure";
static const char long_gen65[]  = "unknown failure";
static const char short_gen66[] = "**file";
static const char long_gen66[]  = "Invalid MPI_File";
static const char short_gen67[] = "**fileaccess";
static const char long_gen67[]  = "Access denied to file";
static const char short_gen68[] = "**fileamode";
static const char long_gen68[]  = "Invalid amode value in MPI_File_open ";
static const char short_gen69[] = "**fileamodeone";
static const char long_gen69[]  = "Exactly one of MPI_MODE_RDONLY, MPI_MODE_WRONLY, or MPI_MODE_RDWR must be specified";
static const char short_gen70[] = "**fileamoderead";
static const char long_gen70[]  = "Cannot use MPI_MODE_CREATE or MPI_MODE_EXCL with MPI_MODE_RDONLY ";
static const char short_gen71[] = "**fileamodeseq";
static const char long_gen71[]  = "Cannot specify MPI_MODE_SEQUENTIAL with MPI_MODE_RDWR";
static const char short_gen72[] = "**fileexist";
static const char long_gen72[]  = "File exists";
static const char short_gen73[] = "**fileinuse";
static const char long_gen73[]  = "File in use by some process";
static const char short_gen74[] = "**filename";
static const char long_gen74[]  = "Invalid file name";
static const char short_gen75[] = "**filenoexist";
static const char long_gen75[]  = "File does not exist";
static const char short_gen76[] = "**filenospace";
static const char long_gen76[]  = "Not enough space for file ";
static const char short_gen77[] = "**fileopunsupported";
static const char long_gen77[]  = "Unsupported file operation ";
static const char short_gen78[] = "**filequota";
static const char long_gen78[]  = "Quota exceeded for files";
static const char short_gen79[] = "**filerdonly";
static const char long_gen79[]  = "Read-only file or filesystem name";
static const char short_gen80[] = "**graphnnodes";
static const char long_gen80[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen81[] = "**group";
static const char long_gen81[]  = "Invalid group";
static const char short_gen82[] = "**groupnotincomm";
static const char long_gen82[]  = "Specified group is not within the communicator";
static const char short_gen83[] = "**hostlookup";
static const char long_gen83[]  = "Host lookup failed";
static const char short_gen84[] = "**indexneg";
static const char long_gen84[]  = "Index value in graph topology must be nonnegative";
static const char short_gen85[] = "**indexnonmonotone";
static const char long_gen85[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen86[] = "**info";
static const char long_gen86[]  = "Invalid MPI_Info";
static const char short_gen87[] = "**infokey";
static const char long_gen87[]  = "Invalid key for MPI_Info ";
static const char short_gen88[] = "**infokeyempty";
static const char long_gen88[]  = "Empty or blank key ";
static const char short_gen89[] = "**infokeylong";
static const char long_gen89[]  = "Key is too long";
static const char short_gen90[] = "**infokeynull";
static const char long_gen90[]  = "Null key";
static const char short_gen91[] = "**infonkey";
static const char long_gen91[]  = "Requested nth key does not exist";
static const char short_gen92[] = "**infonokey";
static const char long_gen92[]  = "MPI_Info key is not defined ";
static const char short_gen93[] = "**infoval";
static const char long_gen93[]  = "Invalid MPI_Info value ";
static const char short_gen94[] = "**infovallong";
static const char long_gen94[]  = "Value is too long ";
static const char short_gen95[] = "**infovalnull";
static const char long_gen95[]  = "Null value";
static const char short_gen96[] = "**initialized";
static const char long_gen96[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen97[] = "**inittwice";
static const char long_gen97[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen98[] = "**inpending";
static const char long_gen98[]  = "Pending request (no error)";
static const char short_gen99[] = "**instatus";
static const char long_gen99[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen100[] = "**intercommcoll";
static const char long_gen100[]  = "Intercommunicator collective operations have not been implemented";
static const char short_gen101[] = "**intern";
static const char long_gen101[]  = "Internal MPI error!";
static const char short_gen102[] = "**io";
static const char long_gen102[]  = "Other I/O error ";
static const char short_gen103[] = "**ioRMWrdwr";
static const char long_gen103[]  = "Must open file with MPI_MODE_RDWR for read-modify-write ";
static const char short_gen104[] = "**ioagnomatch";
static const char long_gen104[]  = "No aggregators match";
static const char short_gen105[] = "**ioamodeseq";
static const char long_gen105[]  = "Cannot use this function when the file is opened with amode MPI_MODE_SEQUENTIAL ";
static const char short_gen106[] = "**iobadcount";
static const char long_gen106[]  = "Invalid count argument";
static const char short_gen107[] = "**iobaddisp";
static const char long_gen107[]  = "Invalid displacement argument";
static const char short_gen108[] = "**iobadfh";
static const char long_gen108[]  = "Invalid file handle";
static const char short_gen109[] = "**iobadoffset";
static const char long_gen109[]  = "Invalid offset argument";
static const char short_gen110[] = "**iobadsize";
static const char long_gen110[]  = "Invalid size argument";
static const char short_gen111[] = "**iobadwhence";
static const char long_gen111[]  = "Invalid whence argument";
static const char short_gen112[] = "**iodatarepnomem";
static const char long_gen112[]  = "User must allocate memory for datarep";
static const char short_gen113[] = "**iodispifseq";
static const char long_gen113[]  = "disp must be set to MPI_DISPLACEMENT_CURRENT since file was opened with MPI_MODE_SEQUENTIAL";
static const char short_gen114[] = "**ioetype";
static const char long_gen114[]  = "Only an integral number of etypes can be accessed";
static const char short_gen115[] = "**iofiletype";
static const char long_gen115[]  = "Filetype must be constructed out of one or more etypes";
static const char short_gen116[] = "**iofstype";
static const char long_gen116[]  = "Cannot determine filesystem type";
static const char short_gen117[] = "**iofstypeunsupported";
static const char long_gen117[]  = "Specified filesystem is not available";
static const char short_gen118[] = "**ioneedrd";
static const char long_gen118[]  = "Read access is required to this file";
static const char short_gen119[] = "**ionegoffset";
static const char long_gen119[]  = "Negative offset argument";
static const char short_gen120[] = "**iopreallocrdwr";
static const char long_gen120[]  = "Must open file with MPI_MODE_RDWR to preallocate disk space";
static const char short_gen121[] = "**iosequnsupported";
static const char long_gen121[]  = "MPI_MODE_SEQUENTIAL not supported on this file system";
static const char short_gen122[] = "**iosharedunsupported";
static const char long_gen122[]  = "Shared file pointers not supported";
static const char short_gen123[] = "**iosplitcoll";
static const char long_gen123[]  = "Only one active split collective I/O operation is allowed per file handle";
static const char short_gen124[] = "**iosplitcollnone";
static const char long_gen124[]  = "No split collective I/O operation is active";
static const char short_gen125[] = "**keyval";
static const char long_gen125[]  = "Invalid keyval";
static const char short_gen126[] = "**keyvalinvalid";
static const char long_gen126[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen127[] = "**keyvalnotcomm";
static const char long_gen127[]  = "Keyval was not defined for communicators";
static const char short_gen128[] = "**keyvalnotdatatype";
static const char long_gen128[]  = "Keyval was not defined for datatype";
static const char short_gen129[] = "**locktype";
static const char long_gen129[]  = "Invalid locktype";
static const char short_gen130[] = "**namepublish";
static const char long_gen130[]  = "Unable to publish service name";
static const char short_gen131[] = "**namepubnotpub";
static const char long_gen131[]  = "Lookup failed for service name ";
static const char short_gen132[] = "**nameservice";
static const char long_gen132[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen133[] = "**noerrclasses";
static const char long_gen133[]  = "No more user-defined error classes";
static const char short_gen134[] = "**noerrcodes";
static const char long_gen134[]  = "No more user-defined error codes";
static const char short_gen135[] = "**nomem";
static const char long_gen135[]  = "Out of memory";
static const char short_gen136[] = "**nonamepub";
static const char long_gen136[]  = "No name publishing service available";
static const char short_gen137[] = "**notcarttopo";
static const char long_gen137[]  = "No Cartesian topology associated with this communicator";
static const char short_gen138[] = "**notgenreq";
static const char long_gen138[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen139[] = "**notgraphtopo";
static const char long_gen139[]  = "No Graph topology associated with this communicator";
static const char short_gen140[] = "**notimpl";
static const char long_gen140[]  = "Function not implemented";
static const char short_gen141[] = "**notopology";
static const char long_gen141[]  = "No topology associated with this communicator";
static const char short_gen142[] = "**notsame";
static const char long_gen142[]  = "Inconsistent arguments to collective routine ";
static const char short_gen143[] = "**nulledge";
static const char long_gen143[]  = "Edge detected from a node to the same node";
static const char short_gen144[] = "**nullptr";
static const char long_gen144[]  = "Null pointer";
static const char short_gen145[] = "**nullptrtype";
static const char long_gen145[]  = "Null pointer";
static const char short_gen146[] = "**op";
static const char long_gen146[]  = "Invalid MPI_Op";
static const char short_gen147[] = "**open";
static const char long_gen147[]  = "open failed";
static const char short_gen148[] = "**opnotpredefined";
static const char long_gen148[]  = "only predefined ops are valid";
static const char short_gen149[] = "**opundefined";
static const char long_gen149[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen150[] = "**opundefined_rma";
static const char long_gen150[]  = "RMA target received unknown RMA operation";
static const char short_gen151[] = "**other";
static const char long_gen151[]  = "Other MPI error";
static const char short_gen152[] = "**permattr";
static const char long_gen152[]  = "Cannot set permanent attribute";
static const char short_gen153[] = "**permop";
static const char long_gen153[]  = "Cannot free permanent MPI_Op ";
static const char short_gen154[] = "**pmi_finalize";
static const char long_gen154[]  = "PMI_Finalize failed";
static const char short_gen155[] = "**port";
static const char long_gen155[]  = "Invalid port";
static const char short_gen156[] = "**rangedup";
static const char long_gen156[]  = "The range array specifies duplicate entries";
static const char short_gen157[] = "**rangeendinvalid";
static const char long_gen157[]  = "Some element of a range array is either negative or too large";
static const char short_gen158[] = "**rangestartinvalid";
static const char long_gen158[]  = "Some element of a range array is either negative or too large";
static const char short_gen159[] = "**rank";
static const char long_gen159[]  = "Invalid rank";
static const char short_gen160[] = "**rankarray";
static const char long_gen160[]  = "Invalid rank in rank array";
static const char short_gen161[] = "**rankdup";
static const char long_gen161[]  = "Duplicate ranks in rank array ";
static const char short_gen162[] = "**ranklocal";
static const char long_gen162[]  = "Error specifying local_leader ";
static const char short_gen163[] = "**rankremote";
static const char long_gen163[]  = "Error specifying remote_leader ";
static const char short_gen164[] = "**request";
static const char long_gen164[]  = "Invalid MPI_Request";
static const char short_gen165[] = "**requestnotpersist";
static const char long_gen165[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen166[] = "**requestpersistactive";
static const char long_gen166[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen167[] = "**rmaconflict";
static const char long_gen167[]  = "Conflicting accesses to window ";
static const char short_gen168[] = "**rmadisp";
static const char long_gen168[]  = "Invalid displacement argument in RMA call ";
static const char short_gen169[] = "**rmasize";
static const char long_gen169[]  = "Invalid size argument in RMA call";
static const char short_gen170[] = "**rmasync";
static const char long_gen170[]  = "Wrong synchronization of RMA calls ";
static const char short_gen171[] = "**root";
static const char long_gen171[]  = "Invalid root";
static const char short_gen172[] = "**servicename";
static const char long_gen172[]  = "Attempt to lookup an unknown service name ";
static const char short_gen173[] = "**shmat";
static const char long_gen173[]  = "shmat failed";
static const char short_gen174[] = "**shmget";
static const char long_gen174[]  = "shmget failed";
static const char short_gen175[] = "**sock|poll|eqmalloc";
static const char long_gen175[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen176[] = "**spawn";
static const char long_gen176[]  = "Error in spawn call";
static const char short_gen177[] = "**stride";
static const char long_gen177[]  = "Range does not terminate";
static const char short_gen178[] = "**stridezero";
static const char long_gen178[]  = "Zero stride is invalid";
static const char short_gen179[] = "**success";
static const char long_gen179[]  = "No MPI error";
static const char short_gen180[] = "**tag";
static const char long_gen180[]  = "Invalid tag";
static const char short_gen181[] = "**toomanycomm";
static const char long_gen181[]  = "Too many communicators";
static const char short_gen182[] = "**topology";
static const char long_gen182[]  = "Invalid topology";
static const char short_gen183[] = "**topotoolarge";
static const char long_gen183[]  = "Topology size is greater than communicator size";
static const char short_gen184[] = "**truncate";
static const char long_gen184[]  = "Message truncated";
static const char short_gen185[] = "**typenamelen";
static const char long_gen185[]  = " Specified datatype name is too long";
static const char short_gen186[] = "**unknown";
static const char long_gen186[]  = "Unknown error.  Please file a bug report.";
static const char short_gen187[] = "**unsupporteddatarep";
static const char long_gen187[]  = "Only native data representation currently supported";
static const char short_gen188[] = "**win";
static const char long_gen188[]  = "Invalid MPI_Win";

static const int generic_msgs_len = 189;
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
{ short_gen188, long_gen188 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_GENERIC
static const char short_spc0[] = "**CreateFileMapping %d";
static const char long_spc0[]  = "CreateFileMapping failed, error %d";
static const char short_spc1[] = "**MPIDI_CH3I_SHM_Attach_to_mem %d";
static const char long_spc1[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed, error %d";
static const char short_spc2[] = "**MapViewOfFileEx %d";
static const char long_spc2[]  = "MapViewOfFileEx failed, error %d";
static const char short_spc3[] = "**OpenProcess %d %d";
static const char long_spc3[]  = "OpenProcess failed for process %d, error %d";
static const char short_spc4[] = "**arg %s";
static const char long_spc4[]  = "Invalid argument %s";
static const char short_spc5[] = "**argarrayneg %s %d %d";
static const char long_spc5[]  = "Negative value in array %s[%d] (value is %d)";
static const char short_spc6[] = "**argneg %s %d";
static const char long_spc6[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc7[] = "**argnonpos %s %d";
static const char long_spc7[]  = "Invalid value for %s; must be positive but is %d";
static const char short_spc8[] = "**argrange %s %d %d";
static const char long_spc8[]  = "Argument %s has value %d but must be within [0,%d]";
static const char short_spc9[] = "**badpacket %d";
static const char long_spc9[]  = "Received a packet of unknown type (%d)";
static const char short_spc10[] = "**bsendbufsmall %d %d";
static const char long_spc10[]  = "Buffer size of %d is smaller than MPI_BSEND_OVERHEAD (%d)";
static const char short_spc11[] = "**bufbsend %d %d";
static const char long_spc11[]  = "Insufficient space in Bsend buffer; requested %d; total buffer size is %d";
static const char short_spc12[] = "**business_card %s";
static const char long_spc12[]  = "Invalid business card (%s)";
static const char short_spc13[] = "**cartcoordinvalid %d %d %d";
static const char long_spc13[]  = " Cartesian coordinate for the %d coordinate is %d but must be between 0 and %d";
static const char short_spc14[] = "**cartdim %d %d";
static const char long_spc14[]  = "Size of the communicator (%d) is smaller than the size of the Cartesian topology (%d)";
static const char short_spc15[] = "**ch3|sock|badbuscard %s";
static const char long_spc15[]  = "[ch3:sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc16[] = "**ch3|sock|badpacket %d";
static const char long_spc16[]  = "[ch3:sock] received packet of unknown type (%d)";
static const char short_spc17[] = "**ch3|sock|connfailed %d %d";
static const char long_spc17[]  = "[ch3:sock] failed to connnect to remote process %d:%d";
static const char short_spc18[] = "**ch3|sock|connrefused %d %d %s";
static const char long_spc18[]  = "[ch3:sock] failed to connect to process %d:%d (%s)";
static const char short_spc19[] = "**ch3|sock|failure %d";
static const char long_spc19[]  = "[ch3:sock] unknown failure, sock_errno=%d";
static const char short_spc20[] = "**ch3|sock|hostlookup %d %d %s";
static const char long_spc20[]  = "[ch3:sock] failed to obtain host information for process %d:%d (%s)";
static const char short_spc21[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc21[]  = "PMI_Finalize failed, error %d";
static const char short_spc22[] = "**commperm %s";
static const char long_spc22[]  = "Cannot free permanent communicator %s";
static const char short_spc23[] = "**connfailed %d %d";
static const char long_spc23[]  = "Failed to connect to remote process %d-%d";
static const char short_spc24[] = "**connrefused %d %d %s";
static const char long_spc24[]  = "Connection refused for process group %d, rank %d, business card <%s>";
static const char short_spc25[] = "**countneg %d";
static const char long_spc25[]  = "Negative count, value is %d";
static const char short_spc26[] = "**dims %d";
static const char long_spc26[]  = "Invalid dimension argument (value is %d)";
static const char short_spc27[] = "**dimsmany %d %d";
static const char long_spc27[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc28[] = "**dupprocesses %d";
static const char long_spc28[]  = "Local and remote groups in MPI_Intercomm_create must not contain the same processes; both contain process %d";
static const char short_spc29[] = "**edgeoutrange %d %d %d";
static const char long_spc29[]  = "Edge index edges[%d] is %d but must be nonnegative and less than %d";
static const char short_spc30[] = "**failure %d";
static const char long_spc30[]  = "unknown failure, error %d";
static const char short_spc31[] = "**groupnotincomm %d";
static const char long_spc31[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc32[] = "**hostlookup %d %d %s";
static const char long_spc32[]  = "Host lookup failed for process group %d, rank %d, business card <%s>";
static const char short_spc33[] = "**indexneg %d %d";
static const char long_spc33[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc34[] = "**indexnonmonotone %d %d %d";
static const char long_spc34[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc35[] = "**infonkey %d %d";
static const char long_spc35[]  = "Requested key %d but this MPI_Info only has %d keys";
static const char short_spc36[] = "**infonokey %s";
static const char long_spc36[]  = "MPI_Info key %s is not defined ";
static const char short_spc37[] = "**intercommcoll %s";
static const char long_spc37[]  = "Intercommunicator collective operation for %s has not been implemented";
static const char short_spc38[] = "**intern %s";
static const char long_spc38[]  = "Internal MPI error!  %s";
static const char short_spc39[] = "**io %s";
static const char long_spc39[]  = "Other I/O error %s";
static const char short_spc40[] = "**namepublish %s";
static const char long_spc40[]  = "Unable to publish service name %s";
static const char short_spc41[] = "**namepubnotpub %s";
static const char long_spc41[]  = "Lookup failed for service name %s";
static const char short_spc42[] = "**notsame %s %s";
static const char long_spc42[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc43[] = "**nulledge %d %d";
static const char long_spc43[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc44[] = "**nullptr %s";
static const char long_spc44[]  = "Null pointer in parameter %s";
static const char short_spc45[] = "**nullptrtype %s";
static const char long_spc45[]  = "Null %s pointer";
static const char short_spc46[] = "**open %s %d %d";
static const char long_spc46[]  = "open(%s) failed for process %d, error %d";
static const char short_spc47[] = "**opnotpredefined %d";
static const char long_spc47[]  = "only predefined ops are valid (op = %d)";
static const char short_spc48[] = "**opundefined %s";
static const char long_spc48[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc49[] = "**opundefined_rma %d";
static const char long_spc49[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc50[] = "**pmi_finalize %d";
static const char long_spc50[]  = "PMI_Finalize failed, error %d";
static const char short_spc51[] = "**rangedup %d %d %d";
static const char long_spc51[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc52[] = "**rangeendinvalid %d %d %d";
static const char long_spc52[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc53[] = "**rangestartinvalid %d %d %d";
static const char long_spc53[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc54[] = "**rank %d %d";
static const char long_spc54[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc55[] = "**rankarray %d %d %d";
static const char long_spc55[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc56[] = "**rankdup %d %d %d";
static const char long_spc56[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc57[] = "**ranklocal %d %d";
static const char long_spc57[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc58[] = "**rankremote %d %d";
static const char long_spc58[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc59[] = "**rmasize %d";
static const char long_spc59[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc60[] = "**root %d";
static const char long_spc60[]  = "Invalid root (value given was %d)";
static const char short_spc61[] = "**shmat %d";
static const char long_spc61[]  = "shmat failed, error %d";
static const char short_spc62[] = "**shmget %d";
static const char long_spc62[]  = "shmget failed, error %d";
static const char short_spc63[] = "**stride %d %d %d";
static const char long_spc63[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc64[] = "**tag %d";
static const char long_spc64[]  = "Invalid tag, value is %d";
static const char short_spc65[] = "**topotoolarge %d %d";
static const char long_spc65[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc66[] = "**typenamelen %d";
static const char long_spc66[]  = " Specified datatype name is too long (%d characters)";

static const int specific_msgs_len = 67;
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
{ short_spc66, long_spc66 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
179,22,52,59,180,41,159,171,81,146,
182,56,7,186,184,151,101,99,98,164,
67,68,74,51,54,72,73,66,86,87,
93,92,102,132,6,142,76,75,155,78,
79,172,176,55,77,188,16,129,125,167,
170,169,168,12};
#endif
