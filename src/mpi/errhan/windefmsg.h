/* This file automatically created by extracterrmsgs */
typedef struct msgpair {
        const char *short_name, *long_name; } msgpair;
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_CLASS
/* The names are in sorted order, allowing the use of a simple
  linear search or bisection algorithm to find the message corresponding to
  a particular message */
static const char short0[] = "**allocmem";
static const char long0[]  = "Unable to allocate memory for MPI_Alloc_mem";
static const char short1[] = "**arg";
static const char long1[]  = "Invalid argument";
static const char short2[] = "**argneg";
static const char long2[]  = "Invalid argument; must be non-negative";
static const char short3[] = "**assert";
static const char long3[]  = "Invalid assert argument";
static const char short4[] = "**base";
static const char long4[]  = "Invalid base address";
static const char short5[] = "**bufalias";
static const char long5[]  = "Buffers must not be aliased";
static const char short6[] = "**buffer";
static const char long6[]  = "Invalid buffer pointer";
static const char short7[] = "**comm";
static const char long7[]  = "Invalid communicator";
static const char short8[] = "**conversion";
static const char long8[]  = "An error occurred in a user-defined data conversion function";
static const char short9[] = "**count";
static const char long9[]  = "Invalid count";
static const char short10[] = "**countneg";
static const char long10[]  = "Negative count";
static const char short11[] = "**datarep";
static const char long11[]  = "The requested datarep name has already been specified to MPI_REGISTER_DATAREP (optional arg: name (string))";
static const char short12[] = "**datarepunsupported";
static const char long12[]  = "Unsupported datarep %s passed to MPI_File_set_view";
static const char short13[] = "**dims";
static const char long13[]  = "Invalid dimension argument";
static const char short14[] = "**dtype";
static const char long14[]  = "Invalid datatype";
static const char short15[] = "**file";
static const char long15[]  = "Invalid MPI_File";
static const char short16[] = "**fileaccess";
static const char long16[]  = "Access denied to file";
static const char short17[] = "**fileamode";
static const char long17[]  = "Invalid amode value of $d in MPI_File_open ";
static const char short18[] = "**fileexist";
static const char long18[]  = "File exists";
static const char short19[] = "**fileinuse";
static const char long19[]  = "File in use by some process";
static const char short20[] = "**filename";
static const char long20[]  = "Invalid file name";
static const char short21[] = "**filenoexist";
static const char long21[]  = "File does not exist";
static const char short22[] = "**filenospace";
static const char long22[]  = "Not enough space for file %s; %d needed but only %d available";
static const char short23[] = "**fileopunsupported";
static const char long23[]  = "Unsupported file operation ";
static const char short24[] = "**filequota";
static const char long24[]  = "Quota exceeded for files";
static const char short25[] = "**filerdonly";
static const char long25[]  = "Read-only file or filesystem name %s";
static const char short26[] = "**group";
static const char long26[]  = "Invalid group";
static const char short27[] = "**info";
static const char long27[]  = "Invalid MPI_Info";
static const char short28[] = "**infokey";
static const char long28[]  = "Invalid key for MPI_Info ";
static const char short29[] = "**infokeyempty";
static const char long29[]  = "Empty or blank key ";
static const char short30[] = "**infokeylong";
static const char long30[]  = "Key %s is too long (length is %d but maximum allowed is %d)";
static const char short31[] = "**infokeynull";
static const char long31[]  = "Null key";
static const char short32[] = "**infonkey";
static const char long32[]  = "Requested nth key does not exist";
static const char short33[] = "**infonokey";
static const char long33[]  = "MPI_Info key is not defined ";
static const char short34[] = "**infoval";
static const char long34[]  = "Invalid MPI_Info value ";
static const char short35[] = "**infovallong";
static const char long35[]  = "Value %s is too long (length is %d but maximum length is %d)";
static const char short36[] = "**infovalnull";
static const char long36[]  = "Null value";
static const char short37[] = "**initialized";
static const char long37[]  = "MPI not initialized. Call MPI_Init or MPI_Init_thread first";
static const char short38[] = "**inittwice";
static const char long38[]  = "Cannot call MPI_INIT or MPI_INIT_THREAD more than once";
static const char short39[] = "**inpending";
static const char long39[]  = "Pending request (no error)";
static const char short40[] = "**instatus";
static const char long40[]  = "See the MPI_ERROR field in MPI_Status for the error code";
static const char short41[] = "**intern";
static const char long41[]  = "Internal MPI error!";
static const char short42[] = "**io";
static const char long42[]  = "Other I/O error ";
static const char short43[] = "**keyval";
static const char long43[]  = "Invalid keyval";
static const char short44[] = "**locktype";
static const char long44[]  = "Invalid locktype";
static const char short45[] = "**nameservice";
static const char long45[]  = "Invalid service name (see MPI_Publish_name)";
static const char short46[] = "**nomem";
static const char long46[]  = "Out of memory";
static const char short47[] = "**notimpl";
static const char long47[]  = "Function not implemented";
static const char short48[] = "**notsame";
static const char long48[]  = "Inconsistent arguments to collective routine ";
static const char short49[] = "**nullptr";
static const char long49[]  = "Null pointer";
static const char short50[] = "**op";
static const char long50[]  = "Invalid MPI_Op";
static const char short51[] = "**other";
static const char long51[]  = "Other MPI error";
static const char short52[] = "**port";
static const char long52[]  = "Invalid port";
static const char short53[] = "**rank";
static const char long53[]  = "Invalid rank";
static const char short54[] = "**request";
static const char long54[]  = "Invalid MPI_Request";
static const char short55[] = "**rmaconflict";
static const char long55[]  = "Conflicting accesses to window ";
static const char short56[] = "**rmadisp";
static const char long56[]  = "Invalid displacement argument in RMA call ";
static const char short57[] = "**rmasize";
static const char long57[]  = "Invalid size argument in RMA call";
static const char short58[] = "**rmasync";
static const char long58[]  = "Wrong synchronization of RMA calls ";
static const char short59[] = "**root";
static const char long59[]  = "Invalid root";
static const char short60[] = "**servicename";
static const char long60[]  = "Attempt to lookup an unknown service name ";
static const char short61[] = "**spawn";
static const char long61[]  = "Error in spawn call";
static const char short62[] = "**success";
static const char long62[]  = "No MPI error";
static const char short63[] = "**tag";
static const char long63[]  = "Invalid tag";
static const char short64[] = "**topology";
static const char long64[]  = "Invalid topology";
static const char short65[] = "**truncate";
static const char long65[]  = "Message truncated";
static const char short66[] = "**typenamelen";
static const char long66[]  = " Specified datatype name is too long";
static const char short67[] = "**unknown";
static const char long67[]  = "Unknown error.  Please file a bug report.";
static const char short68[] = "**win";
static const char long68[]  = "Invalid MPI_Win";
static const char short69[] = "No more user-defined error classes";
static const char long69[]  = "";
static const char short70[] = "No more user-defined error codes";
static const char long70[]  = "";
static const int generic_msgs_len = 71;
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
};
#endif
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_GENERIC
static const char short_spc2[] = "**argneg %s %d";
static const char long_spc2[]  = "Invalid value for %s, must be non-negative but is %d";
static const char short_spc10[] = "**countneg %d";
static const char long_spc10[]  = "Negative count, value is %d";
static const char short_spc32[] = "**infonkey %d %d";
static const char long_spc32[]  = "Requested %dth key but this MPI_Info only has %d keys";
static const char short_spc33[] = "**infonokey %s";
static const char long_spc33[]  = "MPI_Info key %s is not defined ";
static const char short_spc49[] = "**nullptr %s";
static const char long_spc49[]  = "Null pointer in parameter %s";
static const char short_spc53[] = "**rank %d";
static const char long_spc53[]  = "Invalid rank (value is %d)";
static const char short_spc63[] = "**tag %d";
static const char long_spc63[]  = "Invalid tag (value is %d)";
static const char short_spc66[] = "**typenamelen %d";
static const char long_spc66[]  = " Specified datatype name is too long (%d characters)";
static msgpair specific_err_msgs[] = {
{ 0, 0 },
{ 0, 0 },
{ short_spc2, long_spc2 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc10, long_spc10 },
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
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc32, long_spc32 },
{ short_spc33, long_spc33 },
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
{ 0, 0 },
{ 0, 0 },
{ short_spc49, long_spc49 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc53, long_spc53 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ short_spc63, long_spc63 },
{ 0, 0 },
{ 0, 0 },
{ short_spc66, long_spc66 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
{ 0, 0 },
};
#endif
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG_NONE
static int class_to_index[] = {
62,6,9,14,63,7,53,59,26,50,
64,13,1,67,65,51,41,40,39,54,
16,17,20,8,11,18,19,15,27,28,
34,33,42,45,0,48,22,21,52,24,
25,60,61,12,23,68,4,44,43,55,
58,57,56,3,};
#endif
