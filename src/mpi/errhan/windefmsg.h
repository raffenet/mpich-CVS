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
static const char short0[] = "**allocmem";
static const char long0[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short1[] = "**arg";
static const char long1[]  = "Invalid argument";
static const char short2[] = "**argarrayneg";
static const char long2[]  = "Negative value in array ";
static const char short3[] = "**argneg";
static const char long3[]  = "Invalid argument; must be non-negative";
static const char short4[] = "**argrange";
static const char long4[]  = "Argument is not within valid range";
static const char short5[] = "**assert";
static const char long5[]  = "Invalid assert argument";
static const char short6[] = "**attrsentinal";
static const char long6[]  = "Internal fields in an attribute have been overwritten;possible errors in using the attribute value in user code.";
static const char short7[] = "**base";
static const char long7[]  = "Invalid base address";
static const char short8[] = "**bsendbufsmall";
static const char long8[]  = "Buffer size is smaller than MPI_BSEND_OVERHEAD";
static const char short9[] = "**bsendnobuf";
static const char long9[]  = "No buffer to detach. ";
static const char short10[] = "**bufbsend";
static const char long10[]  = "Insufficient space in Bsend buffer";
static const char short11[] = "**bufexists";
static const char long11[]  = "Buffer already attached with MPI_BUFFER_ATTACH.";
static const char short12[] = "**buffer";
static const char long12[]  = "Invalid buffer pointer";
static const char short13[] = "**cancelunknown";
static const char long13[]  = "Attempt to cancel an unknown type of request";
static const char short14[] = "**cartcoordinvalid";
static const char long14[]  = "Cartesian coordinate is invalid (not in range)";
static const char short15[] = "**cartdim";
static const char long15[]  = "Size of Cartesian grid is larger than the size of the communicator";
static const char short16[] = "**cartshiftzero";
static const char long16[]  = "Displacement must be non-zero";
static const char short17[] = "**ch3|mm|CreateFileMapping";
static const char long17[]  = "CreateFileMapping failed";
static const char short18[] = "**ch3|mm|MPIDI_CH3I_SHM_Attach_to_mem";
static const char long18[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed";
static const char short19[] = "**ch3|mm|MPIU_Strdup";
static const char long19[]  = "MPIU_Strdup failed";
static const char short20[] = "**ch3|mm|MapViewOfFileEx";
static const char long20[]  = "MapViewOfFileEx failed";
static const char short21[] = "**ch3|mm|OpenProcess";
static const char long21[]  = "OpenProcess failed";
static const char short22[] = "**ch3|mm|badpacket";
static const char long22[]  = "Received a packet of unknown type";
static const char short23[] = "**ch3|mm|badsock";
static const char long23[]  = "internal error - bad sock";
static const char short24[] = "**ch3|mm|business_card";
static const char long24[]  = "Invalid business card";
static const char short25[] = "**ch3|mm|connallocfailed";
static const char long25[]  = "Connection failed";
static const char short26[] = "**ch3|mm|connclose";
static const char long26[]  = "active connection unexpectedly closed";
static const char short27[] = "**ch3|mm|connfailed";
static const char long27[]  = "Failed to connect to remote process";
static const char short28[] = "**ch3|mm|connrefused";
static const char long28[]  = "Connection refused";
static const char short29[] = "**ch3|mm|connterm";
static const char long29[]  = "active connection unexpectedly terminated";
static const char short30[] = "**ch3|mm|failure";
static const char long30[]  = "unknown failure";
static const char short31[] = "**ch3|mm|hostlookup";
static const char long31[]  = "Host lookup failed";
static const char short32[] = "**ch3|mm|open";
static const char long32[]  = "open failed";
static const char short33[] = "**ch3|mm|pmi_finalize";
static const char long33[]  = "PMI_Finalize failed";
static const char short34[] = "**ch3|mm|shmat";
static const char long34[]  = "shmat failed";
static const char short35[] = "**ch3|mm|shmget";
static const char long35[]  = "shmget failed";
static const char short36[] = "**ch3|shm|CreateFileMapping";
static const char long36[]  = "CreateFileMapping failed";
static const char short37[] = "**ch3|shm|GetMemTwice";
static const char long37[]  = "Global shared memory initializer called more than once";
static const char short38[] = "**ch3|shm|OpenProcess";
static const char long38[]  = "OpenProcess failed";
static const char short39[] = "**ch3|shm|open";
static const char long39[]  = "open failed";
static const char short40[] = "**ch3|shm|pmi_finalize";
static const char long40[]  = "PMI_Finalize failed";
static const char short41[] = "**ch3|shm|shmat";
static const char long41[]  = "shmat failed";
static const char short42[] = "**ch3|shm|shmget";
static const char long42[]  = "shmget failed";
static const char short43[] = "**ch3|sock|badbuscard";
static const char long43[]  = "[ch3_sock] GetHostAndPort - Invalid business card";
static const char short44[] = "**ch3|sock|badpacket";
static const char long44[]  = "[ch3_sock] received packet of unknow type";
static const char short45[] = "**ch3|sock|badsock";
static const char long45[]  = "[ch3_sock] internal error - bad sock";
static const char short46[] = "**ch3|sock|connallocfailed";
static const char long46[]  = "[ch3_sock] unable to allocate a connection structure";
static const char short47[] = "**ch3|sock|connclose";
static const char long47[]  = "[ch3_sock] active connection unexpectedly closed";
static const char short48[] = "**ch3|sock|connfailed";
static const char long48[]  = "[ch3_sock] failed to connnect to remote process";
static const char short49[] = "**ch3|sock|connrefused";
static const char long49[]  = "[ch3_sock] connection refused";
static const char short50[] = "**ch3|sock|connterm";
static const char long50[]  = "[ch3_sock] active connection unexpectedly terminated";
static const char short51[] = "**ch3|sock|eqmalloc";
static const char long51[]  = "MPIU_Malloc failed to allocate memory for an event queue structure";
static const char short52[] = "**ch3|sock|failure";
static const char long52[]  = "[ch3_sock] unknown failure";
static const char short53[] = "**ch3|sock|hostlookup";
static const char long53[]  = "[ch3_sock] hostname lookup failed";
static const char short54[] = "**ch3|sock|pmi_finalize";
static const char long54[]  = "PMI_Finalize failed";
static const char short55[] = "**ch3|sock|strdup";
static const char long55[]  = "[ch3_sock] MPIU_Strdup failed";
static const char short56[] = "**comm";
static const char long56[]  = "Invalid communicator";
static const char short57[] = "**commnotinter";
static const char long57[]  = "An intercommunicator is required but an intracommunicatorwas provided.";
static const char short58[] = "**commperm";
static const char long58[]  = "Cannot free permanent communicator";
static const char short59[] = "**conversion";
static const char long59[]  = "An error occurred in a user-defined data conversion function";
static const char short60[] = "**count";
static const char long60[]  = "Invalid count";
static const char short61[] = "**datarep";
static const char long61[]  = "The requested datarep name has already been specified toMPI_REGISTER_DATAREP";
static const char short62[] = "**datarepunsupported";
static const char long62[]  = "Unsupported datarep passed to MPI_File_set_view ";
static const char short63[] = "**dims";
static const char long63[]  = "Invalid dimension argument";
static const char short64[] = "**dimsmany";
static const char long64[]  = "Number of dimensions is too large ";
static const char short65[] = "**dimspartition";
static const char long65[]  = "Cannot partition nodes as requested ";
static const char short66[] = "**dtype";
static const char long66[]  = "Invalid datatype";
static const char short67[] = "**dtypecommit";
static const char long67[]  = "Datatype has not been committed ";
static const char short68[] = "**dtypeperm";
static const char long68[]  = "Cannot free permanent data type ";
static const char short69[] = "**dupprocesses";
static const char long69[]  = "Local and remote groups in MPI_Intercomm_create must notcontain the same processes";
static const char short70[] = "**edgeoutrange";
static const char long70[]  = "Edge index in graph topology is out of range";
static const char short71[] = "**file";
static const char long71[]  = "Invalid MPI_File";
static const char short72[] = "**fileaccess";
static const char long72[]  = "Access denied to file";
static const char short73[] = "**fileamode";
static const char long73[]  = "Invalid amode value in MPI_File_open ";
static const char short74[] = "**fileexist";
static const char long74[]  = "File exists";
static const char short75[] = "**fileinuse";
static const char long75[]  = "File in use by some process";
static const char short76[] = "**filename";
static const char long76[]  = "Invalid file name";
static const char short77[] = "**filenoexist";
static const char long77[]  = "File does not exist";
static const char short78[] = "**filenospace";
static const char long78[]  = "Not enough space for file ";
static const char short79[] = "**fileopunsupported";
static const char long79[]  = "Unsupported file operation ";
static const char short80[] = "**filequota";
static const char long80[]  = "Quota exceeded for files";
static const char short81[] = "**filerdonly";
static const char long81[]  = "Read-only file or filesystem name";
static const char short82[] = "**graphnnodes";
static const char long82[]  = "Number of graph nodes exceeds size of communicator.";
static const char short83[] = "**group";
static const char long83[]  = "Invalid group";
static const char short84[] = "**groupnotincomm";
static const char long84[]  = "Specified group is not within the communicator";
static const char short85[] = "**indexneg";
static const char long85[]  = "Index value in graph topology must be nonnegative";
static const char short86[] = "**indexnonmonotone";
static const char long86[]  = "Index values in graph topology must be monotone nondecreasing";
static const char short87[] = "**info";
static const char long87[]  = "Invalid MPI_Info";
static const char short88[] = "**infokey";
static const char long88[]  = "Invalid key for MPI_Info ";
static const char short89[] = "**infokeyempty";
static const char long89[]  = "Empty or blank key ";
static const char short90[] = "**infokeylong";
static const char long90[]  = "Key is too long";
static const char short91[] = "**infokeynull";
static const char long91[]  = "Null key";
static const char short92[] = "**infonkey";
static const char long92[]  = "Requested nth key does not exist";
static const char short93[] = "**infonokey";
static const char long93[]  = "MPI_Info key is not defined ";
static const char short94[] = "**infoval";
static const char long94[]  = "Invalid MPI_Info value ";
static const char short95[] = "**infovallong";
static const char long95[]  = "Value is too long ";
static const char short96[] = "**infovalnull";
static const char long96[]  = "Null value";
static const char short97[] = "**initialized";
static const char long97[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short98[] = "**inittwice";
static const char long98[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short99[] = "**inpending";
static const char long99[]  = "Pending request (no error)";
static const char short100[] = "**instatus";
static const char long100[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short101[] = "**intercommcoll";
static const char long101[]  = "Intercommunicator collective operations have not been implemented";
static const char short102[] = "**intern";
static const char long102[]  = "Internal MPI error!";
static const char short103[] = "**io";
static const char long103[]  = "Other I/O error ";
static const char short104[] = "**keyval";
static const char long104[]  = "Invalid keyval";
static const char short105[] = "**keyvalinvalid";
static const char long105[]  = "Attribute key was MPI_KEYVAL_INVALID";
static const char short106[] = "**keyvalnotcomm";
static const char long106[]  = "Keyval was not defined for communicators";
static const char short107[] = "**keyvalnotdatatype";
static const char long107[]  = "Keyval was not defined for datatype";
static const char short108[] = "**locktype";
static const char long108[]  = "Invalid locktype";
static const char short109[] = "**namepublish";
static const char long109[]  = "Unable to publish service name";
static const char short110[] = "**namepubnotpub";
static const char long110[]  = "Lookup failed for service name ";
static const char short111[] = "**nameservice";
static const char long111[]  = "Invalid service name (see MPI_Publish_name)";
static const char short112[] = "**noerrclasses";
static const char long112[]  = "No more user-defined error classes";
static const char short113[] = "**noerrcodes";
static const char long113[]  = "No more user-defined error codes";
static const char short114[] = "**nomem";
static const char long114[]  = "Out of memory";
static const char short115[] = "**nonamepub";
static const char long115[]  = "No name publishing service available";
static const char short116[] = "**notcarttopo";
static const char long116[]  = "No Cartesian topology associated with this communicator";
static const char short117[] = "**notgenreq";
static const char long117[]  = "Attempt to complete a request with MPI_GREQUEST_COMPLETE thatwas not started with MPI_GREQUEST_START";
static const char short118[] = "**notgraphtopo";
static const char long118[]  = "No Graph topology associated with this communicator";
static const char short119[] = "**notimpl";
static const char long119[]  = "Function not implemented";
static const char short120[] = "**notopology";
static const char long120[]  = "No topology associated with this communicator";
static const char short121[] = "**notsame";
static const char long121[]  = "Inconsistent arguments to collective routine ";
static const char short122[] = "**nulledge";
static const char long122[]  = "Edge detected from a node to the same node";
static const char short123[] = "**nullptr";
static const char long123[]  = "Null pointer";
static const char short124[] = "**op";
static const char long124[]  = "Invalid MPI_Op";
static const char short125[] = "**opundefined";
static const char long125[]  = "only predefined ops valid";
static const char short126[] = "**opundefined_rma";
static const char long126[]  = "RMA target received unknown RMA operation";
static const char short127[] = "**other";
static const char long127[]  = "Other MPI error";
static const char short128[] = "**permattr";
static const char long128[]  = "Cannot set permanent attribute";
static const char short129[] = "**permop";
static const char long129[]  = "Cannot free permanent MPI_Op ";
static const char short130[] = "**pmi_finalize";
static const char long130[]  = "PMI_Finalize failed";
static const char short131[] = "**port";
static const char long131[]  = "Invalid port";
static const char short132[] = "**rangedup";
static const char long132[]  = "The range array specifies duplicate entries";
static const char short133[] = "**rangeendinvalid";
static const char long133[]  = "Some element of a range array is either negative or too large";
static const char short134[] = "**rangestartinvalid";
static const char long134[]  = "Some element of a range array is either negative or too large";
static const char short135[] = "**rank";
static const char long135[]  = "Invalid rank";
static const char short136[] = "**rankarray";
static const char long136[]  = "Invalid rank in rank array";
static const char short137[] = "**rankdup";
static const char long137[]  = "Duplicate ranks in rank array ";
static const char short138[] = "**ranklocal";
static const char long138[]  = "Error specifying local_leader ";
static const char short139[] = "**rankremote";
static const char long139[]  = "Error specifying remote_leader ";
static const char short140[] = "**request";
static const char long140[]  = "Invalid MPI_Request";
static const char short141[] = "**requestpersistactive";
static const char long141[]  = "Persistent request passed to MPI_Start or MPI_Startall is already active.";
static const char short142[] = "**rmaconflict";
static const char long142[]  = "Conflicting accesses to window ";
static const char short143[] = "**rmadisp";
static const char long143[]  = "Invalid displacement argument in RMA call ";
static const char short144[] = "**rmasize";
static const char long144[]  = "Invalid size argument in RMA call";
static const char short145[] = "**rmasync";
static const char long145[]  = "Wrong synchronization of RMA calls ";
static const char short146[] = "**root";
static const char long146[]  = "Invalid root";
static const char short147[] = "**servicename";
static const char long147[]  = "Attempt to lookup an unknown service name ";
static const char short148[] = "**spawn";
static const char long148[]  = "Error in spawn call";
static const char short149[] = "**stride";
static const char long149[]  = "Range does not terminate";
static const char short150[] = "**stridezero";
static const char long150[]  = "Zero stride is invalid";
static const char short151[] = "**success";
static const char long151[]  = "No MPI error";
static const char short152[] = "**tag";
static const char long152[]  = "Invalid tag";
static const char short153[] = "**toomanycomm";
static const char long153[]  = "Too many communicators";
static const char short154[] = "**topology";
static const char long154[]  = "Invalid topology";
static const char short155[] = "**topotoolarge";
static const char long155[]  = "Topology size is greater than communicator size";
static const char short156[] = "**truncate";
static const char long156[]  = "Message truncated";
static const char short157[] = "**typenamelen";
static const char long157[]  = " Specified datatype name is too long";
static const char short158[] = "**unknown";
static const char long158[]  = "Unknown error.  Please file a bug report.";
static const char short159[] = "**win";
static const char long159[]  = "Invalid MPI_Win";
static const int generic_msgs_len = 160;
static msgpair generic_err_msgs[] = {
{ short0, long0 },
{ short1, long1 },
{ short2, long2 },
{ short3, long3 },
{ short4, long4 },
{ short5, long5 },
{ short6, long6 },
{ short7, long7 },
{ short8, long8 },
{ short9, long9 },
{ short10, long10 },
{ short11, long11 },
{ short12, long12 },
{ short13, long13 },
{ short14, long14 },
{ short15, long15 },
{ short16, long16 },
{ short17, long17 },
{ short18, long18 },
{ short19, long19 },
{ short20, long20 },
{ short21, long21 },
{ short22, long22 },
{ short23, long23 },
{ short24, long24 },
{ short25, long25 },
{ short26, long26 },
{ short27, long27 },
{ short28, long28 },
{ short29, long29 },
{ short30, long30 },
{ short31, long31 },
{ short32, long32 },
{ short33, long33 },
{ short34, long34 },
{ short35, long35 },
{ short36, long36 },
{ short37, long37 },
{ short38, long38 },
{ short39, long39 },
{ short40, long40 },
{ short41, long41 },
{ short42, long42 },
{ short43, long43 },
{ short44, long44 },
{ short45, long45 },
{ short46, long46 },
{ short47, long47 },
{ short48, long48 },
{ short49, long49 },
{ short50, long50 },
{ short51, long51 },
{ short52, long52 },
{ short53, long53 },
{ short54, long54 },
{ short55, long55 },
{ short56, long56 },
{ short57, long57 },
{ short58, long58 },
{ short59, long59 },
{ short60, long60 },
{ short61, long61 },
{ short62, long62 },
{ short63, long63 },
{ short64, long64 },
{ short65, long65 },
{ short66, long66 },
{ short67, long67 },
{ short68, long68 },
{ short69, long69 },
{ short70, long70 },
{ short71, long71 },
{ short72, long72 },
{ short73, long73 },
{ short74, long74 },
{ short75, long75 },
{ short76, long76 },
{ short77, long77 },
{ short78, long78 },
{ short79, long79 },
{ short80, long80 },
{ short81, long81 },
{ short82, long82 },
{ short83, long83 },
{ short84, long84 },
{ short85, long85 },
{ short86, long86 },
{ short87, long87 },
{ short88, long88 },
{ short89, long89 },
{ short90, long90 },
{ short91, long91 },
{ short92, long92 },
{ short93, long93 },
{ short94, long94 },
{ short95, long95 },
{ short96, long96 },
{ short97, long97 },
{ short98, long98 },
{ short99, long99 },
{ short100, long100 },
{ short101, long101 },
{ short102, long102 },
{ short103, long103 },
{ short104, long104 },
{ short105, long105 },
{ short106, long106 },
{ short107, long107 },
{ short108, long108 },
{ short109, long109 },
{ short110, long110 },
{ short111, long111 },
{ short112, long112 },
{ short113, long113 },
{ short114, long114 },
{ short115, long115 },
{ short116, long116 },
{ short117, long117 },
{ short118, long118 },
{ short119, long119 },
{ short120, long120 },
{ short121, long121 },
{ short122, long122 },
{ short123, long123 },
{ short124, long124 },
{ short125, long125 },
{ short126, long126 },
{ short127, long127 },
{ short128, long128 },
{ short129, long129 },
{ short130, long130 },
{ short131, long131 },
{ short132, long132 },
{ short133, long133 },
{ short134, long134 },
{ short135, long135 },
{ short136, long136 },
{ short137, long137 },
{ short138, long138 },
{ short139, long139 },
{ short140, long140 },
{ short141, long141 },
{ short142, long142 },
{ short143, long143 },
{ short144, long144 },
{ short145, long145 },
{ short146, long146 },
{ short147, long147 },
{ short148, long148 },
{ short149, long149 },
{ short150, long150 },
{ short151, long151 },
{ short152, long152 },
{ short153, long153 },
{ short154, long154 },
{ short155, long155 },
{ short156, long156 },
{ short157, long157 },
{ short158, long158 },
{ short159, long159 },
};
#endif
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_GENERIC
static const char short_spc2[] = "**argarrayneg %s %d %d";
static const char long_spc2[]  = "Negative value in array %s[%d] (value is %d)";
static const char short_spc3[] = "**argneg %s %d";
static const char long_spc3[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc4[] = "**argrange %s %d %d";
static const char long_spc4[]  = "Argument %s has value %d but must be within [0,%d]";
static const char short_spc8[] = "**bsendbufsmall %d %d";
static const char long_spc8[]  = "Buffer size of %d is smaller than MPI_BSEND_OVERHEAD (%d)";
static const char short_spc10[] = "**bufbsend %d %d";
static const char long_spc10[]  = "Insufficient space in Bsend buffer; requested %d; totalbuffer size is %d";
static const char short_spc14[] = "**cartcoordinvalid %d %d %d";
static const char long_spc14[]  = " Cartesian coordinate for the %d coordinateis %d but must be between 0 and %d";
static const char short_spc15[] = "**cartdim %d %d";
static const char long_spc15[]  = "Size of the communicator (%d) is smaller than the size of theCartesian topology (%d)";
static const char short_spc17[] = "**ch3|mm|CreateFileMapping %d";
static const char long_spc17[]  = "CreateFileMapping failed, error %d";
static const char short_spc18[] = "**ch3|mm|MPIDI_CH3I_SHM_Attach_to_mem %d";
static const char long_spc18[]  = "MPIDI_CH3I_SHM_Attach_to_mem failed, error %d";
static const char short_spc20[] = "**ch3|mm|MapViewOfFileEx %d";
static const char long_spc20[]  = "MapViewOfFileEx failed, error %d";
static const char short_spc21[] = "**ch3|mm|OpenProcess %d %d";
static const char long_spc21[]  = "OpenProcess failed for process %d, error %d";
static const char short_spc22[] = "**ch3|mm|badpacket %d";
static const char long_spc22[]  = "Received a packet of unknown type (%d)";
static const char short_spc24[] = "**ch3|mm|business_card %s";
static const char long_spc24[]  = "Invalid business card (%s)";
static const char short_spc27[] = "**ch3|mm|connfailed %d %d";
static const char long_spc27[]  = "Failed to connect to remote process %d-%d";
static const char short_spc30[] = "**ch3|mm|failure %d";
static const char long_spc30[]  = "unknown failure, error %d";
static const char short_spc32[] = "**ch3|mm|open %s %d %d";
static const char long_spc32[]  = "open(%s) failed for process %d, error %d";
static const char short_spc33[] = "**ch3|mm|pmi_finalize %d";
static const char long_spc33[]  = "PMI_Finalize failed, error %d";
static const char short_spc34[] = "**ch3|mm|shmat %d";
static const char long_spc34[]  = "shmat failed, error %d";
static const char short_spc35[] = "**ch3|mm|shmget %d";
static const char long_spc35[]  = "shmget failed, error %d";
static const char short_spc36[] = "**ch3|shm|CreateFileMapping %d";
static const char long_spc36[]  = "CreateFileMapping failed, error %d";
static const char short_spc38[] = "**ch3|shm|OpenProcess %d %d";
static const char long_spc38[]  = "OpenProcess failed for process %d, error %d";
static const char short_spc39[] = "**ch3|shm|open %s %d %d";
static const char long_spc39[]  = "open(%s) failed for process %d, error %d";
static const char short_spc40[] = "**ch3|shm|pmi_finalize %d";
static const char long_spc40[]  = "PMI_Finalize failed, error %d";
static const char short_spc41[] = "**ch3|shm|shmat %d";
static const char long_spc41[]  = "shmat failed, error %d";
static const char short_spc42[] = "**ch3|shm|shmget %d";
static const char long_spc42[]  = "shmget failed, error %d";
static const char short_spc43[] = "**ch3|sock|badbuscard %s";
static const char long_spc43[]  = "[ch3_sock] GetHostAndPort - Invalid business card (%s)";
static const char short_spc44[] = "**ch3|sock|badpacket %d";
static const char long_spc44[]  = "[ch3_sock] received packet of unknown type (%d)";
static const char short_spc48[] = "**ch3|sock|connfailed %d %d";
static const char long_spc48[]  = "[ch3_sock] failed to connnect to remote process %d-%d";
static const char short_spc49[] = "**ch3|sock|connrefused %d %d %s";
static const char long_spc49[]  = "[ch3_sock] failed to connect to process %d-%d (%s)";
static const char short_spc52[] = "**ch3|sock|failure %d";
static const char long_spc52[]  = "[ch3_sock] unknown failure, sock_errno=%d";
static const char short_spc53[] = "**ch3|sock|hostlookup %d %d %s";
static const char long_spc53[]  = "[ch3_sock] failed to obtain host information for process %d-%d (%s)";
static const char short_spc54[] = "**ch3|sock|pmi_finalize %d";
static const char long_spc54[]  = "PMI_Finalize failed, error %d";
static const char short_spc58[] = "**commperm %s";
static const char long_spc58[]  = "Cannot free permanent communicator %s";
static const char short_spc63[] = "**dims %d";
static const char long_spc63[]  = "Invalid dimension argument (value is %d)";
static const char short_spc64[] = "**dimsmany %d %d";
static const char long_spc64[]  = "Number of dimensions %d is too large (maximum is %d)";
static const char short_spc69[] = "**dupprocesses %d";
static const char long_spc69[]  = "Local and remote groups in MPI_Intercomm_create must notcontain the same processes; both contain process %d";
static const char short_spc70[] = "**edgeoutrange %d %d %d";
static const char long_spc70[]  = "Edge index edges[%d] is %d but must be nonnegativeand less than %d";
static const char short_spc84[] = "**groupnotincomm %d";
static const char long_spc84[]  = "Rank %d of the specified group is not a member of this communicator";
static const char short_spc85[] = "**indexneg %d %d";
static const char long_spc85[]  = "Index value for index[%d] is %d but must be nonnegative";
static const char short_spc86[] = "**indexnonmonotone %d %d %d";
static const char long_spc86[]  = "Index values in graph topology must be monotone nondecreasing but index[%d] is %d but the next index value is %d";
static const char short_spc92[] = "**infonkey %d %d";
static const char long_spc92[]  = "Requested %dth key but this MPI_Info only has %d keys";
static const char short_spc93[] = "**infonokey %s";
static const char long_spc93[]  = "MPI_Info key %s is not defined ";
static const char short_spc101[] = "**intercommcoll %s";
static const char long_spc101[]  = "Intercommunicator collective operation for %s has not been implemented";
static const char short_spc102[] = "**intern %s";
static const char long_spc102[]  = "Internal MPI error!  %s";
static const char short_spc109[] = "**namepublish %s";
static const char long_spc109[]  = "Unable to publish service name %s";
static const char short_spc110[] = "**namepubnotpub %s";
static const char long_spc110[]  = "Lookup failed for service name %s";
static const char short_spc121[] = "**notsame %s %s";
static const char long_spc121[]  = "Inconsistent arguments %s to collective routine %s";
static const char short_spc122[] = "**nulledge %d %d";
static const char long_spc122[]  = "Edge for node %d (entry edges[%d]) is to itself";
static const char short_spc123[] = "**nullptr %s";
static const char long_spc123[]  = "Null pointer in parameter %s";
static const char short_spc125[] = "**opundefined %d";
static const char long_spc125[]  = "only predefined ops valid (op = %d)";
static const char short_spc126[] = "**opundefined_rma %d";
static const char long_spc126[]  = "RMA target received unknown RMA operation type %d";
static const char short_spc130[] = "**pmi_finalize %d";
static const char long_spc130[]  = "PMI_Finalize failed, error %d";
static const char short_spc132[] = "**rangedup %d %d %d";
static const char long_spc132[]  = "The range array specifies duplicate entries; process %d specified in range array %d was previously specified in range array %d";
static const char short_spc133[] = "**rangeendinvalid %d %d %d";
static const char long_spc133[]  = "The %dth element of a range array ends at %d but must be nonnegative and less than %d";
static const char short_spc134[] = "**rangestartinvalid %d %d %d";
static const char long_spc134[]  = "The %dth element of a range array starts at %d but must be nonnegative and less than %d";
static const char short_spc135[] = "**rank %d %d";
static const char long_spc135[]  = "Invalid rank has value %d but must be nonnegative and less than %d";
static const char short_spc136[] = "**rankarray %d %d %d";
static const char long_spc136[]  = "Invalid rank in rank array[%d], value is %d but must be in 0 to %d.";
static const char short_spc137[] = "**rankdup %d %d %d";
static const char long_spc137[]  = "Duplicate ranks in rank array at index %d, has value %d which isalso the value at index %d";
static const char short_spc138[] = "**ranklocal %d %d";
static const char long_spc138[]  = "Error specifying local_leader; rank given was %d but mustbe in the range 0 to %d";
static const char short_spc139[] = "**rankremote %d %d";
static const char long_spc139[]  = "Error specifying remote_leader; rank given was %d but mustbe in the range 0 to %d";
static const char short_spc144[] = "**rmasize %d";
static const char long_spc144[]  = "Invalid size argument in RMA call (value is %d)";
static const char short_spc149[] = "**stride %d %d %d";
static const char long_spc149[]  = "Range (start = %d, end = %d, stride = %d) does not terminate";
static const char short_spc155[] = "**topotoolarge %d %d";
static const char long_spc155[]  = "Topology size %d is larger than communicator size (%d)";
static const char short_spc157[] = "**typenamelen %d";
static const char long_spc157[]  = " Specified datatype name is too long (%d characters)";
/* This array parallels the generic_err_msgs array, with
 null entries where there is no special instance-specific message */
static msgpair specific_err_msgs[] = {
{ 0, 0 },
{ 0, 0 },
{ short_spc2, long_spc2 },
{ short_spc3, long_spc3 },
{ short_spc4, long_spc4 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc8, long_spc8 },
{ 0, 0 },
{ short_spc10, long_spc10 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc14, long_spc14 },
{ short_spc15, long_spc15 },
{ 0, 0 },
{ short_spc17, long_spc17 },
{ short_spc18, long_spc18 },
{ 0, 0 },
{ short_spc20, long_spc20 },
{ short_spc21, long_spc21 },
{ short_spc22, long_spc22 },
{ 0, 0 },
{ short_spc24, long_spc24 },
{ 0, 0 },
{ 0, 0 },
{ short_spc27, long_spc27 },
{ 0, 0 },
{ 0, 0 },
{ short_spc30, long_spc30 },
{ 0, 0 },
{ short_spc32, long_spc32 },
{ short_spc33, long_spc33 },
{ short_spc34, long_spc34 },
{ short_spc35, long_spc35 },
{ short_spc36, long_spc36 },
{ 0, 0 },
{ short_spc38, long_spc38 },
{ short_spc39, long_spc39 },
{ short_spc40, long_spc40 },
{ short_spc41, long_spc41 },
{ short_spc42, long_spc42 },
{ short_spc43, long_spc43 },
{ short_spc44, long_spc44 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc48, long_spc48 },
{ short_spc49, long_spc49 },
{ 0, 0 },
{ 0, 0 },
{ short_spc52, long_spc52 },
{ short_spc53, long_spc53 },
{ short_spc54, long_spc54 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc58, long_spc58 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc63, long_spc63 },
{ short_spc64, long_spc64 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc69, long_spc69 },
{ short_spc70, long_spc70 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc84, long_spc84 },
{ short_spc85, long_spc85 },
{ short_spc86, long_spc86 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc92, long_spc92 },
{ short_spc93, long_spc93 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc101, long_spc101 },
{ short_spc102, long_spc102 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc109, long_spc109 },
{ short_spc110, long_spc110 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc121, long_spc121 },
{ short_spc122, long_spc122 },
{ short_spc123, long_spc123 },
{ 0, 0 },
{ short_spc125, long_spc125 },
{ short_spc126, long_spc126 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc130, long_spc130 },
{ 0, 0 },
{ short_spc132, long_spc132 },
{ short_spc133, long_spc133 },
{ short_spc134, long_spc134 },
{ short_spc135, long_spc135 },
{ short_spc136, long_spc136 },
{ short_spc137, long_spc137 },
{ short_spc138, long_spc138 },
{ short_spc139, long_spc139 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc144, long_spc144 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc149, long_spc149 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc155, long_spc155 },
{ 0, 0 },
{ short_spc157, long_spc157 },
{ 0, 0 },
{ 0, 0 },
};
#endif
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
151,12,60,66,152,56,135,146,83,124,
154,63,1,158,156,127,102,100,99,140,
72,73,76,59,61,74,75,71,87,88,
94,93,103,111,0,121,78,77,131,80,
81,147,148,62,79,159,7,108,104,142,
145,144,143,5,};
#endif
