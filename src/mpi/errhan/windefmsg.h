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
static const char short_gen69[] = "**fileexist";
static const char long_gen69[]  = "File exists";
static const char short_gen70[] = "**fileinuse";
static const char long_gen70[]  = "File in use by some process";
static const char short_gen71[] = "**filename";
static const char long_gen71[]  = "Invalid file name";
static const char short_gen72[] = "**filenoexist";
static const char long_gen72[]  = "File does not exist";
static const char short_gen73[] = "**filenospace";
static const char long_gen73[]  = "Not enough space for file ";
static const char short_gen74[] = "**fileopunsupported";
static const char long_gen74[]  = "Unsupported file operation ";
static const char short_gen75[] = "**filequota";
static const char long_gen75[]  = "Quota exceeded for files";
static const char short_gen76[] = "**filerdonly";
static const char long_gen76[]  = "Read-only file or filesystem name";
static const char short_gen77[] = "**graphnnodes";
static const char long_gen77[]  = "Number of graph nodes exceeds size of communicator.";
static const char short_gen78[] = "**group";
static const char long_gen78[]  = "Invalid group";
static const char short_gen79[] = "**groupnotincomm";
static const char long_gen79[]  = "Specified group is not within the communicator";
static const char short_gen80[] = "**hostlookup";
static const char long_gen80[]  = "Host lookup failed";
static const char short_gen81[] = "**indexneg";
static const char long_gen81[]  = "Index value in graph topology must be nonnegative";
static const char short_gen82[] = "**indexnonmonotone";
static const char long_gen82[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short_gen83[] = "**info";
static const char long_gen83[]  = "Invalid MPI_Info";
static const char short_gen84[] = "**infokey";
static const char long_gen84[]  = "Invalid key for MPI_Info ";
static const char short_gen85[] = "**infokeyempty";
static const char long_gen85[]  = "Empty or blank key ";
static const char short_gen86[] = "**infokeylong";
static const char long_gen86[]  = "Key is too long";
static const char short_gen87[] = "**infokeynull";
static const char long_gen87[]  = "Null key";
static const char short_gen88[] = "**infonkey";
static const char long_gen88[]  = "Requested nth key does not exist";
static const char short_gen89[] = "**infonokey";
static const char long_gen89[]  = "MPI_Info key is not defined ";
static const char short_gen90[] = "**infoval";
static const char long_gen90[]  = "Invalid MPI_Info value ";
static const char short_gen91[] = "**infovallong";
static const char long_gen91[]  = "Value is too long ";
static const char short_gen92[] = "**infovalnull";
static const char long_gen92[]  = "Null value";
static const char short_gen93[] = "**initialized";
static const char long_gen93[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short_gen94[] = "**inittwice";
static const char long_gen94[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short_gen95[] = "**inpending";
static const char long_gen95[]  = "Pending request (no error)";
static const char short_gen96[] = "**instatus";
static const char long_gen96[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short_gen97[] = "**intercommcoll";
static const char long_gen97[]  = "Intercommunicator collective operations have not been implemented";
static const char short_gen98[] = "**intern";
static const char long_gen98[]  = "Internal MPI error!";
static const char short_gen99[] = "**io";
static const char long_gen99[]  = "Other I/O error ";
static const char short_gen100[] = "**keyval";
static const char long_gen100[]  = "Invalid keyval";
static const char short_gen101[] = "**keyvalinvalid";
static const char long_gen101[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short_gen102[] = "**keyvalnotcomm";
static const char long_gen102[]  = "Keyval was not defined for communicators";
static const char short_gen103[] = "**keyvalnotdatatype";
static const char long_gen103[]  = "Keyval was not defined for datatype";
static const char short_gen104[] = "**locktype";
static const char long_gen104[]  = "Invalid locktype";
static const char short_gen105[] = "**namepublish";
static const char long_gen105[]  = "Unable to publish service name";
static const char short_gen106[] = "**namepubnotpub";
static const char long_gen106[]  = "Lookup failed for service name ";
static const char short_gen107[] = "**nameservice";
static const char long_gen107[]  = "Invalid service name (see MPI_Publish_name)";
static const char short_gen108[] = "**noerrclasses";
static const char long_gen108[]  = "No more user-defined error classes";
static const char short_gen109[] = "**noerrcodes";
static const char long_gen109[]  = "No more user-defined error codes";
static const char short_gen110[] = "**nomem";
static const char long_gen110[]  = "Out of memory";
static const char short_gen111[] = "**nonamepub";
static const char long_gen111[]  = "No name publishing service available";
static const char short_gen112[] = "**notcarttopo";
static const char long_gen112[]  = "No Cartesian topology associated with this communicator";
static const char short_gen113[] = "**notgenreq";
static const char long_gen113[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE that was not started with MPI_GREQUEST_START";
static const char short_gen114[] = "**notgraphtopo";
static const char long_gen114[]  = "No Graph topology associated with this communicator";
static const char short_gen115[] = "**notimpl";
static const char long_gen115[]  = "Function not implemented";
static const char short_gen116[] = "**notopology";
static const char long_gen116[]  = "No topology associated with this communicator";
static const char short_gen117[] = "**notsame";
static const char long_gen117[]  = "Inconsistent arguments to collective routine ";
static const char short_gen118[] = "**nulledge";
static const char long_gen118[]  = "Edge detected from a node to the same node";
static const char short_gen119[] = "**nullptr";
static const char long_gen119[]  = "Null pointer";
static const char short_gen120[] = "**nullptrtype";
static const char long_gen120[]  = "Null pointer";
static const char short_gen121[] = "**op";
static const char long_gen121[]  = "Invalid MPI_Op";
static const char short_gen122[] = "**open";
static const char long_gen122[]  = "open failed";
static const char short_gen123[] = "**opnotpredefined";
static const char long_gen123[]  = "only predefined ops are valid";
static const char short_gen124[] = "**opundefined";
static const char long_gen124[]  = "MPI_Op operation not defined for this datatype ";
static const char short_gen125[] = "**opundefined_rma";
static const char long_gen125[]  = "RMA target received unknown RMA operation";
static const char short_gen126[] = "**other";
static const char long_gen126[]  = "Other MPI error";
static const char short_gen127[] = "**permattr";
static const char long_gen127[]  = "Cannot set permanent attribute";
static const char short_gen128[] = "**permop";
static const char long_gen128[]  = "Cannot free permanent MPI_Op ";
static const char short_gen129[] = "**pmi_finalize";
static const char long_gen129[]  = "PMI_Finalize failed";
static const char short_gen130[] = "**port";
static const char long_gen130[]  = "Invalid port";
static const char short_gen131[] = "**rangedup";
static const char long_gen131[]  = "The range array specifies duplicate entries";
static const char short_gen132[] = "**rangeendinvalid";
static const char long_gen132[]  = "Some element of a range array is either negative or too large";
static const char short_gen133[] = "**rangestartinvalid";
static const char long_gen133[]  = "Some element of a range array is either negative or too large";
static const char short_gen134[] = "**rank";
static const char long_gen134[]  = "Invalid rank";
static const char short_gen135[] = "**rankarray";
static const char long_gen135[]  = "Invalid rank in rank array";
static const char short_gen136[] = "**rankdup";
static const char long_gen136[]  = "Duplicate ranks in rank array ";
static const char short_gen137[] = "**ranklocal";
static const char long_gen137[]  = "Error specifying local_leader ";
static const char short_gen138[] = "**rankremote";
static const char long_gen138[]  = "Error specifying remote_leader ";
static const char short_gen139[] = "**request";
static const char long_gen139[]  = "Invalid MPI_Request";
static const char short_gen140[] = "**requestnotpersist";
static const char long_gen140[]  = "Request is not persistent in MPI_Start or MPI_Startall.";
static const char short_gen141[] = "**requestpersistactive";
static const char long_gen141[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short_gen142[] = "**rmaconflict";
static const char long_gen142[]  = "Conflicting accesses to window ";
static const char short_gen143[] = "**rmadisp";
static const char long_gen143[]  = "Invalid displacement argument in RMA call ";
static const char short_gen144[] = "**rmasize";
static const char long_gen144[]  = "Invalid size argument in RMA call";
static const char short_gen145[] = "**rmasync";
static const char long_gen145[]  = "Wrong synchronization of RMA calls ";
static const char short_gen146[] = "**root";
static const char long_gen146[]  = "Invalid root";
static const char short_gen147[] = "**servicename";
static const char long_gen147[]  = "Attempt to lookup an unknown service name ";
static const char short_gen148[] = "**shmat";
static const char long_gen148[]  = "shmat failed";
static const char short_gen149[] = "**shmget";
static const char long_gen149[]  = "shmget failed";
static const char short_gen150[] = "**sock|poll|eqmalloc";
static const char long_gen150[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short_gen151[] = "**spawn";
static const char long_gen151[]  = "Error in spawn call";
static const char short_gen152[] = "**stride";
static const char long_gen152[]  = "Range does not terminate";
static const char short_gen153[] = "**stridezero";
static const char long_gen153[]  = "Zero stride is invalid";
static const char short_gen154[] = "**success";
static const char long_gen154[]  = "No MPI error";
static const char short_gen155[] = "**tag";
static const char long_gen155[]  = "Invalid tag";
static const char short_gen156[] = "**toomanycomm";
static const char long_gen156[]  = "Too many communicators";
static const char short_gen157[] = "**topology";
static const char long_gen157[]  = "Invalid topology";
static const char short_gen158[] = "**topotoolarge";
static const char long_gen158[]  = "Topology size is greater than communicator size";
static const char short_gen159[] = "**truncate";
static const char long_gen159[]  = "Message truncated";
static const char short_gen160[] = "**typenamelen";
static const char long_gen160[]  = " Specified datatype name is too long";
static const char short_gen161[] = "**unknown";
static const char long_gen161[]  = "Unknown error.  Please file a bug report.";
static const char short_gen162[] = "**win";
static const char long_gen162[]  = "Invalid MPI_Win";

static const int generic_msgs_len = 163;
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
{ short_gen162, long_gen162 }
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
static const char short_spc39[] = "**namepublish %s";
static const char long_spc39[]  = "Unable to publish service name %s";
static const char short_spc40[] = "**namepubnotpub %s";
static const char long_spc40[]  = "Lookup failed for service name %s";
static const char short_spc41[] = "**notsame %s %s";
static const char long_spc41[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc42[] = "**nulledge %d %d";
static const char long_spc42[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc43[] = "**nullptr %s";
static const char long_spc43[]  = "Null pointer in parameter %s";
static const char short_spc44[] = "**nullptrtype %s";
static const char long_spc44[]  = "Null %s pointer";
static const char short_spc45[] = "**open %s %d %d";
static const char long_spc45[]  = "open(%s) failed for process %d, error %d";
static const char short_spc46[] = "**opnotpredefined %d";
static const char long_spc46[]  = "only predefined ops are valid (op = %d)";
static const char short_spc47[] = "**opundefined %s";
static const char long_spc47[]  = "MPI_Op %s operation not defined for this datatype ";
static const char short_spc48[] = "**opundefined_rma %d";
static const char long_spc48[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc49[] = "**pmi_finalize %d";
static const char long_spc49[]  = "PMI_Finalize failed, error %d";
static const char short_spc50[] = "**rangedup %d %d %d";
static const char long_spc50[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc51[] = "**rangeendinvalid %d %d %d";
static const char long_spc51[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc52[] = "**rangestartinvalid %d %d %d";
static const char long_spc52[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc53[] = "**rank %d %d";
static const char long_spc53[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc54[] = "**rankarray %d %d %d";
static const char long_spc54[]  = "Invalid rank in rank array at index %d; value is %d but must be in the range 0 to %d";
static const char short_spc55[] = "**rankdup %d %d %d";
static const char long_spc55[]  = "Duplicate ranks in rank array at index %d, has value %d which is also the value at index %d";
static const char short_spc56[] = "**ranklocal %d %d";
static const char long_spc56[]  = "Error specifying local_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc57[] = "**rankremote %d %d";
static const char long_spc57[]  = "Error specifying remote_leader; rank given was %d but must be in the range 0 to %d";
static const char short_spc58[] = "**rmasize %d";
static const char long_spc58[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc59[] = "**root %d";
static const char long_spc59[]  = "Invalid root (value given was %d)";
static const char short_spc60[] = "**shmat %d";
static const char long_spc60[]  = "shmat failed, error %d";
static const char short_spc61[] = "**shmget %d";
static const char long_spc61[]  = "shmget failed, error %d";
static const char short_spc62[] = "**stride %d %d %d";
static const char long_spc62[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc63[] = "**tag %d";
static const char long_spc63[]  = "Invalid tag, value is %d";
static const char short_spc64[] = "**topotoolarge %d %d";
static const char long_spc64[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc65[] = "**typenamelen %d";
static const char long_spc65[]  = " Specified datatype name is too long (%d characters)";

static const int specific_msgs_len = 66;
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
{ short_spc65, long_spc65 }
};
#endif

#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
154,22,52,59,155,41,134,146,78,121,
157,56,7,161,159,126,98,96,95,139,
67,68,71,51,54,69,70,66,83,84,
90,89,99,107,6,117,73,72,130,75,
76,147,151,55,74,162,16,104,100,142,
145,144,143,12};
#endif
