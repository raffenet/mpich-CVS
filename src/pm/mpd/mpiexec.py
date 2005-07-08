#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage:
mpiexec [-h or -help or --help]    # get this message
mpiexec -file filename             # (or -f) filename contains XML job description
mpiexec [global args] [local args] executable [args]
   where global args may be
      -l                           # line labels by MPI rank
      -bnr                         # MPICH1 compatibility mode
      -machinefile                 # file mapping procs to machines
      -s <spec>                    # direct stdin to "all" or 1,2 or 2-4,6 
      -1                           # override default of trying 1st proc locally
      -ifhn                        # network interface to use locally
      -tv                          # run procs under totalview (must be installed)
      -gdb                         # run procs under gdb
      -m                           # merge output lines (default with gdb)
      -a                           # means assign this alias to the job
      -ecfn                        # output_xml_exit_codes_filename
      -g<local arg name>           # global version of local arg (below)
    and local args may be
      -n <n> or -np <n>            # number of processes to start
      -wdir <dirname>              # working directory to start in
      -umask <umask>               # umask for remote process
      -path <dirname>              # place to look for executables
      -host <hostname>             # host to start on
      -soft <spec>                 # modifier of -n value
      -arch <arch>                 # arch type to start on (not implemented)
      -envall                      # pass all env vars in current environment
      -envnone                     # pass no env vars
      -envlist <list of env var names> # pass current values of these vars
      -env <name> <value>          # pass this value of this env var
mpiexec [global args] [local args] executable args : [local args] executable...
mpiexec -gdba jobid                # gdb-attach to existing jobid
mpiexec -configfile filename       # filename contains cmd line segs as lines
  (See User Guide for more details)

Examples:
   mpiexec -l -n 10 cpi 100
   mpiexec -genv QPL_LICENSE 4705 -n 3 a.out

   mpiexec -n 1 -host foo master : -n 4 -host mysmp slave
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""

from signal import signal, alarm, SIG_DFL, SIG_IGN, SIGINT, SIGTSTP, SIGCONT, SIGALRM, SIGTTIN
signal(SIGTTIN,SIG_IGN)

import re
from  sys    import argv, exit, stdin, stdout, stderr
from  os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path, unlink, \
                    open as osopen, fdopen, O_CREAT, O_WRONLY, O_EXCL, O_RDONLY,  \
                    fork, wait, waitpid, kill, unlink, umask, system, _exit,  \
                    WIFSIGNALED, WEXITSTATUS
from  popen2 import Popen3
from  urllib import quote
from  socket import gethostname, gethostbyname_ex, gethostbyaddr
from  time   import time
from  urllib import unquote
from  mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_version, mpd_print, \
                    mpd_uncaught_except_tb, mpd_handle_signal, mpd_which, \
                    MPDListenSock, MPDStreamHandler, MPDConsClientSock, MPDParmDB

global parmdb, nextRange, appnum
global numDoneWithIO, myExitStatus, sigOccurred, outXmlDoc, outECs

recvTimeout = 20  # const

def mpiexec():
    global parmdb, nextRange, appnum
    global numDoneWithIO, myExitStatus, sigOccurred, outXmlDoc, outECs

    import sys  # for sys.excepthook on next line
    sys.excepthook = mpd_uncaught_except_tb

    myExitStatus = 0
    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '-help'  or  argv[1] == '--help':
	usage()
    myHost = gethostname()
    mpd_set_my_id(myid='mpiexec_%s' % (myHost) )
    try:
        hostinfo = gethostbyname_ex(myHost)
    except:
        print 'mpd failed: gethostbyname_ex failed for %s' % (myHost)
        exit(-1)
    myIP = hostinfo[2][0]

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPIEXEC_LINE_LABELS'         :  0,
                        'MPIEXEC_JOB_ALIAS'           :  '',
                        'MPIEXEC_USIZE'               :  0,
                        'MPIEXEC_GDB'                 :  0,
                        'MPIEXEC_IFHN'                :  '',  # use one from mpd as default
                        'MPIEXEC_MERGE_OUTPUT'        :  0,
                        'MPIEXEC_STDIN_DEST'          :  '0',
                        'MPIEXEC_MACHINEFILE'         :  '',
                        'MPIEXEC_BNR'                 :  0,
                        'MPIEXEC_TOTALVIEW'           :  0,
                        'MPIEXEC_EXITCODES_FILENAME'  :  '',
                        'MPIEXEC_TRY_1ST_LOCALLY'     :  1,
                        'MPIEXEC_TIMEOUT'             :  0,
                        'MPIEXEC_HOST_LIST'           :  [],
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb[('thispgm','mship')] = ''
    parmdb[('thispgm','rship')] = ''
    parmdb[('thispgm','userpgm')] = ''
    parmdb[('thispgm','nprocs')] = 0
    parmdb[('thispgm','ecfn_format')] = ''
    parmdb[('thispgm','gdb_attach_jobid')] = ''
    parmdb[('thispgm','singinitpid')] = 0
    parmdb[('thispgm','singinitport')] = 0
    parmdb[('thispgm','ignore_rcfile')] = 0
    parmdb[('thispgm','ignore_environ')] = 0
    parmdb[('thispgm','inXmlFilename')] = ''
    parmdb[('thispgm','print_parmdb_all')] = 0
    parmdb[('thispgm','print_parmdb_def')] = 0

    get_parms_from_env(parmsToOverride)
    get_parms_from_rcfile(parmsToOverride)

    appnum = 0
    nextRange = 0
    localArgSets = { 0 : [] }

    if argv[1] == '-gdba':
	if len(argv) != 3:
            print '-gdba arg must appear only with jobid'
	    usage()
    elif argv[1] == '-file'  or  argv[1] == '-f':
	if len(argv) != 3:
            print '-file (-f) arg must appear alone'
	    usage()
        parmdb[('cmdline','inXmlFilename')] = argv[2]
    elif argv[1] == '-p':
        parmdb[('cmdline','singinitpid')]  = argv[2]
        parmdb[('cmdline','singinitport')] = argv[3]
        parmdb[('cmdline','userpgm')] = argv[4]
        parmdb[('cmdline','nprocs')] = 1
        parmdb[('cmdline','MPIEXEC_TRY_1ST_LOCALLY')] = 1
        machineFileInfo = {}
        tempargv = [argv[0],argv[4]]
        collect_args(tempargv,localArgSets)
    else:
        if argv[1] == '-configfile':
	    if len(argv) != 3:
	        usage()
            configFileFD = osopen(argv[2],O_RDONLY)
            configFile = fdopen(configFileFD,'r',0)
            configLines = configFile.readlines()
            configLines = [ x.strip() + ' : '  for x in configLines if x[0] != '#' ]
            tempargv = []
            for line in configLines:
                shOut = Popen3("/bin/sh -c 'for a in $*; do echo _$a; done' -- %s" % (line))
                for shline in shOut.fromchild:
                    tempargv.append(shline[1:].strip())    # 1: strips off the leading _
	    tempargv = [argv[0]] + tempargv[0:-1]   # strip off the last : I added
            collect_args(tempargv,localArgSets)
        else:
            collect_args(argv,localArgSets)
        machineFileInfo = read_machinefile(parmdb['MPIEXEC_MACHINEFILE'])

    # mostly old mpdrun below here
    numDoneWithIO = 0
    outXmlDoc = ''
    outECs = ''
    outECFile = None
    sigOccurred = 0

    # set some default values for mpd; others added as discovered below
    msgToMPD = { 'cmd'            : 'mpdrun',
                 'conhost'        : myHost,
                 'spawned'        : 0,
                 'nstarted'       : 0,
                 'hosts'          : {},
                 'execs'          : {},
                 'users'          : {},
                 'cwds'           : {},
                 'umasks'         : {},
                 'paths'          : {},
                 'args'           : {},
                 'limits'         : {},
                 'envvars'        : {},
               }

    listenSock = MPDListenSock('',0,name='socket_to_listen_for_man')
    listenPort = listenSock.getsockname()[1]
    conSock = MPDConsClientSock()  # looks for MPD_UNIX_SOCKET in env
    msgToSend = { 'cmd' : 'get_mpdrun_values' }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=recvTimeout)
    if not msg:
        mpd_print(1, 'no msg recvd from mpd during version check')
        exit(-1)
    elif msg['cmd'] != 'response_get_mpdrun_values':
        mpd_print(1,'unexpected msg from mpd :%s:' % (msg) )
        exit(-1)
    if msg['mpd_version'] != mpd_version():
        mpd_print(1,'mpd version %s does not match mpiexec version %s' % \
                  (msg['mpd_version'],mpd_version()) )
        exit(-1)
    if not parmdb['MPIEXEC_IFHN']:    # if user did not specify one, use mpd's
        parmdb[('thispgm','MPIEXEC_IFHN')] = msg['mpd_ifhn']    # not really thispgm here

    if parmdb['gdb_attach_jobid']:
        get_vals_for_attach(parmdb,conSock,msgToMPD)
    elif parmdb['inXmlFilename']:    # get these after we have a conn to mpd
        get_parms_from_xml_file(conSock,msgToMPD)
    else:
        parmdb[('cmdline','nprocs')] = 0  # for incr later
        for k in localArgSets.keys():
	    handle_local_argset(localArgSets[k],machineFileInfo,msgToMPD)

    if parmdb['MPIEXEC_MERGE_OUTPUT']  and  not parmdb['MPIEXEC_LINE_LABELS']:
        parmdb[('thispgm','MPIEXEC_LINE_LABELS')] = 1   # causes line labels also

    if parmdb['print_parmdb_all']:
        parmdb.printall()
    if parmdb['print_parmdb_def']:
        parmdb.printdef()

    if parmdb['mship']:
        mshipSock = MPDListenSock('',0,name='socket_for_mship')
        mshipPort = mshipSock.getsockname()[1]
        mshipPid = fork()
        if mshipPid == 0:
            conSock.close()
            environ['MPDCP_AM_MSHIP'] = '1'
            environ['MPDCP_MSHIP_PORT'] = str(mshipPort)
            environ['MPDCP_MSHIP_FD'] = str(mshipSock.fileno())
            environ['MPDCP_MSHIP_NPROCS'] = str(parmdb['nprocs'])
            try:
                execvpe(parmdb['mship'],[parmdb['MPIEXEC_MSHIP']],environ)
            except Exception, errmsg:
                mpd_print(1,'execvpe failed for copgm %s; errmsg=:%s:' % \
                          (parmdb['MPIEXEC_MSHIP'],errmsg))
                exit(-1)
            _exit(0);  # do NOT do cleanup
        mshipSock.close()
    else:
        mshipPid = 0

    # make sure to do this after nprocs has its value
    linesPerRank = {}  # keep this a dict instead of a list
    for i in range(parmdb['nprocs']):
        linesPerRank[i] = []

    if parmdb['MPIEXEC_EXITCODES_FILENAME']:
        if parmdb['ecfn_format'] == 'xml':
            try:
                import xml.dom.minidom
            except:
                print 'you requested to save the exit codes in an xml file, but'
                print '  I was unable to import the xml.dom.minidom module'
                exit(-1)
            outXmlDoc = xml.dom.minidom.Document()
            outECs = outXmlDoc.createElement('exit-codes')
            outXmlDoc.appendChild(outECs)
        else:
            outECs = 'exit-codes\n'

    msgToMPD['nprocs'] = parmdb['nprocs']
    msgToMPD['limits'][(0,parmdb['nprocs']-1)]  = {}
    msgToMPD['conport'] = listenPort
    msgToMPD['conip'] = myIP
    msgToMPD['conifhn'] = parmdb['MPIEXEC_IFHN']
    if parmdb['MPIEXEC_JOB_ALIAS']:
        msgToMPD['jobalias'] = parmdb['MPIEXEC_JOB_ALIAS']
    else:
        msgToMPD['jobalias'] = ''
    if parmdb['MPIEXEC_TRY_1ST_LOCALLY']:
        msgToMPD['try_1st_locally'] = 1
    if parmdb['MPIEXEC_LINE_LABELS']:
        msgToMPD['line_labels'] = 1
    if parmdb['rship']:
        msgToMPD['rship'] = parmdb['rship']
        msgToMPD['mship_host'] = gethostname()
        msgToMPD['mship_port'] = mshipPort
    if parmdb['MPIEXEC_BNR']:
        msgToMPD['doing_bnr'] = 1
    if parmdb['MPIEXEC_STDIN_DEST'] == 'all':
        stdinDest = '0-%d' % (parmdb['nprocs']-1)
    else:
        stdinDest = parmdb['MPIEXEC_STDIN_DEST']
    msgToMPD['stdin_dest'] = stdinDest
    msgToMPD['gdb'] = parmdb['MPIEXEC_GDB']
    msgToMPD['totalview'] = parmdb['MPIEXEC_TOTALVIEW']
    msgToMPD['singinitpid'] = parmdb['singinitpid']
    msgToMPD['singinitport'] = parmdb['singinitport']
    msgToMPD['host_spec_pool'] = parmdb['MPIEXEC_HOST_LIST']

    # set sig handlers up right before we send mpdrun msg to mpd
    signal(SIGINT, sig_handler)
    signal(SIGTSTP,sig_handler)
    signal(SIGCONT,sig_handler)
    signal(SIGALRM,sig_handler)

    conSock.send_dict_msg(msgToMPD)
    msg = conSock.recv_dict_msg(timeout=recvTimeout)
    if not msg:
        mpd_print(1, 'no msg recvd from mpd when expecting ack of request')
        exit(-1)
    elif msg['cmd'] == 'mpdrun_ack':
        currRingSize = msg['ringsize']
        currRingNCPUs = msg['ring_ncpus']
    else:
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            exit(-1)
        elif msg['cmd'] == 'job_failed':
            if  msg['reason'] == 'some_procs_not_started':
                print 'mpiexec: unable to start all procs; may have invalid machine names'
                print '    remaining specified hosts:'
                for host in msg['remaining_hosts'].values():
                    if host != '_any_':
                        try:
                            print '        %s (%s)' % (host,gethostbyaddr(host)[0])
                        except:
                            print '        %s' % (host)
            elif  msg['reason'] == 'invalid_username':
                print 'mpiexec: invalid username %s at host %s' % \
                      (msg['username'],msg['host'])
            else:
                print 'mpiexec: job failed; reason=:%s:' % (msg['reason'])
            exit(-1)
        else:
            mpd_print(1, 'unexpected message from mpd: %s' % (msg) )
            exit(-1)
    conSock.close()
    jobTimeout = parmdb['MPIEXEC_TIMEOUT']
    if jobTimeout:
        alarm(jobTimeout)

    streamHandler = MPDStreamHandler()

    (manSock,addr) = listenSock.accept()
    if not manSock:
        mpd_print(1, 'mpiexec: failed to obtain sock from manager')
        exit(-1)
    streamHandler.set_handler(manSock,handle_man_input,args=(streamHandler,))
    streamHandler.set_handler(stdin,handle_stdin_input,args=(parmdb,streamHandler,manSock))
    # first, do handshaking with man
    msg = manSock.recv_dict_msg()
    if (not msg  or  not msg.has_key('cmd') or msg['cmd'] != 'man_checking_in'):
        mpd_print(1, 'mpiexec: from man, invalid msg=:%s:' % (msg) )
        exit(-1)
    msgToSend = { 'cmd' : 'ringsize', 'ring_ncpus' : currRingNCPUs,
                  'ringsize' : currRingSize }
    manSock.send_dict_msg(msgToSend)
    msg = manSock.recv_dict_msg()
    if (not msg  or  not msg.has_key('cmd')):
        mpd_print(1, 'mpiexec: from man, invalid msg=:%s:' % (msg) )
        exit(-1)
    if (msg['cmd'] == 'job_started'):
        jobid = msg['jobid']
        if outECs:
            if parmdb['ecfn_format'] == 'xml':
                outECs.setAttribute('jobid',jobid.strip())
            else:
                outECs += 'jobid=%s\n' % (jobid.strip())
        # print 'mpiexec: job %s started' % (jobid)
        if parmdb['MPIEXEC_TOTALVIEW']:
            if not mpd_which('totalview'):
                print 'cannot find "totalview" in your $PATH:'
                print '    ', environ['PATH']
                exit(-1)
            import mtv
            tv_cmd = 'dattach python ' + `getpid()` + '; dgo; dassign MPIR_being_debugged 1'
            system('totalview -e "%s" &' % (tv_cmd) )
            mtv.wait_for_debugger()
            mtv.allocate_proctable(parmdb['nprocs'])
            # extract procinfo (rank,hostname,exec,pid) tuples from msg
            for i in range(parmdb['nprocs']):
                tvhost = msg['procinfo'][i][0]
                tvpgm  = msg['procinfo'][i][1]
                tvpid  = msg['procinfo'][i][2]
                # print "%d %s %s %d" % (i,host,pgm,pid)
                mtv.append_proctable_entry(tvhost,tvpgm,tvpid)
            mtv.complete_spawn()
            msgToSend = { 'cmd' : 'tv_ready' }
            manSock.send_dict_msg(msgToSend)
    else:
        mpd_print(1, 'mpiexec: from man, unknown msg=:%s:' % (msg) )
        exit(-1)

    (manCliStdoutSock,addr) = listenSock.accept()
    streamHandler.set_handler(manCliStdoutSock,
                              handle_cli_stdout_input,
                              args=(parmdb,streamHandler,linesPerRank,))
    (manCliStderrSock,addr) = listenSock.accept()
    streamHandler.set_handler(manCliStderrSock,
                              handle_cli_stderr_input,
                              args=(streamHandler,))

    # Main Loop
    timeDelayForPrints = 2  # seconds
    timeForPrint = time() + timeDelayForPrints   # to get started
    numDoneWithIO = 0
    while numDoneWithIO < 3:    # man, client stdout, and client stderr
        if sigOccurred:
            handle_sig_occurred(manSock)
        rv = streamHandler.handle_active_streams(timeout=1.0)
        if rv[0] < 0:  # will handle some sigs at top of next loop
            pass       # may have to handle some err conditions here
        if parmdb['MPIEXEC_MERGE_OUTPUT']:
            if timeForPrint < time():
                print_ready_merged_lines(1,parmdb,linesPerRank)
                timeForPrint = time() + timeDelayForPrints
            else:
                print_ready_merged_lines(parmdb['nprocs'],parmdb,linesPerRank)

    if parmdb['MPIEXEC_MERGE_OUTPUT']:
        print_ready_merged_lines(1,parmdb,linesPerRank)
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)
    if parmdb['MPIEXEC_EXITCODES_FILENAME']:
        outECFile = open(parmdb['MPIEXEC_EXITCODES_FILENAME'],'w')
        if parmdb['ecfn_format'] == 'xml':
            print >>outECFile, outXmlDoc.toprettyxml(indent='   ')
        else:
            print >>outECFile, outECs,
        outECFile.close()
    return myExitStatus


def collect_args(args,localArgSets):
    validGlobalArgs = { '-l' : 0, '-usize' : 1, '-gdb' : 0, '-bnr' : 0, '-tv' : 0,
                        '-ifhn' : 1, '-machinefile' : 1, '-s' : 1, '-1' : 0,
                        '-a' : 1, '-m' : 0, '-ecfn' : 1,
                        '-gn' : 1, '-gnp' : 1, '-ghost' : 1, '-gpath' : 1, '-gwdir' : 1,
			'-gsoft' : 1, '-garch' : 1, '-gexec' : 1, '-gumask' : 1,
			'-genvall' : 0, '-genv' : 2, '-genvnone' : 0,
			'-genvlist' : 1 }
    currumask = umask(0) ; umask(currumask)  # grab it and set it back
    parmdb[('cmdline','-gn')]          = 1
    parmdb[('cmdline','-ghost')]       = '_any_'
    parmdb[('cmdline','-gpath')]       = environ['PATH']
    parmdb[('cmdline','-gwdir')]       = path.abspath(getcwd())
    parmdb[('cmdline','-gumask')]      = str(currumask)
    parmdb[('cmdline','-gsoft')]       = 0
    parmdb[('cmdline','-garch')]       = ''
    parmdb[('cmdline','-gexec')]       = ''
    parmdb[('cmdline','-genv')]        = {}
    parmdb[('cmdline','-genvlist')]    = []
    parmdb[('cmdline','-genvnone')]    = 0
    argidx = 1
    while argidx < len(args)  and  args[argidx] in validGlobalArgs.keys():
        garg = args[argidx]
        if garg == '-genv':
            parmdb['genv'][args[argidx+1]] = args[argidx+2]
            argidx += 3
        elif garg == '-gn'  or  garg == '-gnp':
            if args[argidx+1].isdigit():
                parmdb[('cmdline','-gn')] = int(args[argidx+1])
            else:
                print 'argument to %s must be numeric' % (garg)
                usage()
            argidx += 2
        elif garg == '-ghost':
            try:
                parmdb[('cmdline',garg)] = gethostbyname_ex(args[argidx+1])[2][0]
            except:
                print 'unable to do find info for host %s' % (args[argidx+1])
                exit(-1)
            argidx += 2
        elif garg == '-gpath':
            parmdb[('cmdline','-gpath')] = args[argidx+1]
            argidx += 2
        elif garg == '-gwdir':
            parmdb[('cmdline','-gwdir')] = args[argidx+1]
            argidx += 2
        elif garg == '-gumask':
            parmdb[('cmdline','-gumask')] = args[argidx+1]
            argidx += 2
        elif garg == '-gsoft':
            parmdb[('cmdline','-gsoft')] = args[argidx+1]
            argidx += 2
        elif garg == '-garch':
            parmdb[('cmdline','-garch')] = args[argidx+1]
            argidx += 2
            print '** -garch is accepted but not used'
        elif garg == '-gexec':
            parmdb[('cmdline','-gexec')] = args[argidx+1]
            argidx += 2
        elif garg == '-genv':
            parmdb[('cmdline','-genv')] = args[argidx+1]
            argidx += 2
        elif garg == '-genvlist':
            parmdb[('cmdline','-genvlist')] = args[argidx+1]
            argidx += 2
        elif garg == '-genvnone':
            parmdb[('cmdline','-genvnone')] = args[argidx+1]
            argidx += 1
        elif garg == '-l':
            parmdb[('cmdline','MPIEXEC_LINE_LABELS')] = 1
            argidx += 1
        elif garg == '-a':
            parmdb[('cmdline','MPIEXEC_JOB_ALIAS')] = args[argidx+1]
            argidx += 2
        elif garg == '-usize':
            if args[argidx+1].isdigit():
                parmdb[('cmdline','MPIEXEC_USIZE')] = int(args[argidx+1])
            else:
                print 'argument to %s must be numeric' % (garg)
                usage()
            argidx += 2
        elif garg == '-gdb':
            parmdb[('cmdline','MPIEXEC_GDB')] = 1
            argidx += 1
        elif garg == '-ifhn':
            parmdb[('cmdline','MPIEXEC_IFHN')] = args[argidx+1]
            argidx += 2
        elif garg == '-m':
            parmdb[('cmdline','MPIEXEC_MERGE_OUTPUT')] = 1
            argidx += 1
        elif garg == '-s':
            parmdb[('cmdline','MPIEXEC_STDIN_DEST')] = args[argidx+1]
            argidx += 2
        elif garg == '-machinefile':
            parmdb[('cmdline','MPIEXEC_MACHINEFILE')] = args[argidx+1]
            argidx += 2
        elif garg == '-bnr':
            parmdb[('cmdline','MPIEXEC_BNR')] = 1
            argidx += 1
        elif garg == '-tv':
            parmdb[('cmdline','MPIEXEC_TOTALVIEW')] = 1
            argidx += 1
        elif garg == '-ecfn':
            parmdb[('cmdline','MPIEXEC_EXITCODES_FILENAME')] = args[argidx+1]
            argidx += 2
        elif garg == '-1':
            parmdb[('cmdline','MPIEXEC_TRY_1ST_LOCALLY')] = 0  # reverses meaning
            argidx += 1
    if args[argidx] == ':':
        argidx += 1
    localArgsKey = 0
    # collect local arg sets but do not validate them until handled below
    while argidx < len(args):
        if args[argidx] == ':':
            localArgsKey += 1
            localArgSets[localArgsKey] = []
        else:
            localArgSets[localArgsKey].append(args[argidx])
        argidx += 1

def handle_local_argset(argset,machineFileInfo,msgToMPD):
    global parmdb, nextRange, appnum
    validLocalArgs  = { '-n' : 1, '-np' : 1, '-host' : 1, '-path' : 1, '-wdir' : 1,
                        '-soft' : 1, '-arch' : 1, '-umask' : 1,
			'-envall' : 0, '-env' : 2, '-envnone' : 0, '-envlist' : 1 }
    host   = parmdb['-ghost']
    wdir   = parmdb['-gwdir']
    umask  = parmdb['-gumask']
    wpath  = parmdb['-gpath']
    nProcs = parmdb['-gn']
    usize  = parmdb['MPIEXEC_USIZE']
    gexec  = parmdb['-gexec']
    softness =  parmdb['-gsoft']
    if parmdb['-genvnone']:
        envall = 0
    else:
        envall = 1
    if parmdb['-genvlist']:
        parmdb[('cmdline','-genvlist')] = parmdb['-genvlist'].split(',')
    localEnvlist = []
    localEnv  = {}
    
    argidx = 0
    while argidx < len(argset):
        if argset[argidx] not in validLocalArgs:
            if argset[argidx][0] == '-':
                print 'invalid "local" arg: %s' % argset[argidx]
                usage()
            break                       # since now at executable
        if parmdb['MPIEXEC_MACHINEFILE']:
            if argset[argidx] == '-host'  or  argset[argidx] == ['-ghost']:
                print '-host (or -ghost) and -machinefile are incompatible'
                exit(-1)
        if argset[argidx] == '-n' or argset[argidx] == '-np':
            if len(argset) < (argidx+2):
                print '** missing arg to -n'
                usage()
            nProcs = argset[argidx+1]
            if not nProcs.isdigit():
                print '** non-numeric arg to -n: %s' % nProcs
                usage()
            nProcs = int(nProcs)
            argidx += 2
        elif argset[argidx] == '-host':
            if len(argset) < (argidx+2):
                print '** missing arg to -host'
                usage()
            try:
                host = gethostbyname_ex(argset[argidx+1])[2][0]
            except:
                print 'unable to do find info for host %s' % (argset[argidx+1])
                exit(-1)
            argidx += 2
        elif argset[argidx] == '-path':
            if len(argset) < (argidx+2):
                print '** missing arg to -path'
                usage()
            wpath = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-wdir':
            if len(argset) < (argidx+2):
                print '** missing arg to -wdir'
                usage()
            wdir = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-umask':
            if len(argset) < (argidx+2):
                print '** missing arg to -umask'
                usage()
            umask = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-soft':
            if len(argset) < (argidx+2):
                print '** missing arg to -soft'
                usage()
            softness = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-arch':
            if len(argset) < (argidx+2):
                print '** missing arg to -arch'
                usage()
            print '** -arch is accepted but not used'
            argidx += 2
        elif argset[argidx] == '-envall':
            envall = 1
            argidx += 1
        elif argset[argidx] == '-envnone':
            envall = 0
            argidx += 1
        elif argset[argidx] == '-envlist':
            localEnvlist = argset[argidx+1].split(',')
            argidx += 2
        elif argset[argidx] == '-env':
            if len(argset) < (argidx+3):
                print '** missing arg to -env'
                usage()
            var = argset[argidx+1]
            val = argset[argidx+2]
            localEnv[var] = val
            argidx += 3
        else:
            print 'unknown "local" option: %s' % argset[argidx]
            usage()

    if softness:
        nProcs = adjust_nprocs(nProcs,softness)

    cmdAndArgs = []
    if argidx < len(argset):
        while argidx < len(argset):
            cmdAndArgs.append(argset[argidx])
            argidx += 1
    else:
        if gexec:
            cmdAndArgs = [gexec]
    if not cmdAndArgs:
        print 'no cmd specified'
        usage()

    argsetLoRange = nextRange
    argsetHiRange = nextRange + nProcs - 1
    loRange = argsetLoRange
    hiRange = argsetHiRange

    defaultHostForArgset = host
    while loRange <= argsetHiRange:
        host = defaultHostForArgset
        ifhn = ''
        if machineFileInfo:
            if len(machineFileInfo) <= hiRange:
                print 'too few entries in machinefile'
                exit(-1)
            host = machineFileInfo[loRange]['host']
            ifhn = machineFileInfo[loRange]['ifhn']
            for i in range(loRange+1,hiRange+1):
                if machineFileInfo[i]['host'] != host  or  machineFileInfo[i]['ifhn'] != ifhn:
                    hiRange = i - 1
                    break

        asRange = (loRange,hiRange)  # this argset range as a tuple

        msgToMPD['users'][asRange]  = mpd_get_my_username()
        msgToMPD['execs'][asRange]  = cmdAndArgs[0]
        msgToMPD['paths'][asRange]  = wpath
        msgToMPD['cwds'][asRange]   = wdir
        msgToMPD['umasks'][asRange] = umask
        msgToMPD['args'][asRange]   = cmdAndArgs[1:]
        if host.startswith('_any_'):
            msgToMPD['hosts'][(loRange,hiRange)] = host
        else:
            try:
                msgToMPD['hosts'][asRange] = gethostbyname_ex(host)[2][0]
            except:
                print 'unable to do find info for host %s' % (host)
                exit(-1)

        envToSend = {}
        if envall:
            for envvar in environ.keys():
                envToSend[envvar] = environ[envvar]
        for envvar in parmdb['-genvlist']:
            if not environ.has_key(envvar):
                print '%s in envlist does not exist in your env' % (envvar)
                exit(-1)
            envToSend[envvar] = environ[envvar]
        for envvar in localEnvlist:
            if not environ.has_key(envvar):
                print '%s in envlist does not exist in your env' % (envvar)
                exit(-1)
            envToSend[envvar] = environ[envvar]
        for envvar in parmdb['-genv'].keys():
            envToSend[envvar] = parmdb['-genv'][envvar]
        for envvar in localEnv.keys():
            envToSend[envvar] = localEnv[envvar]
        if usize:
            envToSend['MPI_UNIVERSE_SIZE'] = '%s' % (usize)
        envToSend['MPI_APPNUM'] = '%s' % '%s' % str(appnum)
        if ifhn:
            envToSend['MPICH_INTERFACE_HOSTNAME'] = '%s' % (ifhn)
        msgToMPD['envvars'][(loRange,hiRange)] = envToSend

        loRange = hiRange + 1
        hiRange = argsetHiRange  # again

    appnum += 1
    nextRange += nProcs
    parmdb[('cmdline','nprocs')] = parmdb['nprocs'] + nProcs
    
# Adjust nProcs (called maxprocs in the Standard) according to soft:
# Our interpretation is that we need the largest number <= nProcs that is
# consistent with the list of possible values described by soft.  I.e.
# if the user says
#
#   mpiexec -n 10 -soft 5 a.out
#
# we adjust the 10 down to 5.  This may not be what was intended in the Standard,
# but it seems to be what it says.

def adjust_nprocs(nProcs,softness):
    biglist = []
    list1 = softness.split(',')
    for triple in list1:                # triple is a or a:b or a:b:c
        thingy = triple.split(':')     
        if len(thingy) == 1:
            a = int(thingy[0])
            if a <= nProcs and a >= 0:
                biglist.append(a)
        elif len(thingy) == 2:
            a = int(thingy[0])
            b = int(thingy[1])
            for i in range(a,b+1):
                if i <= nProcs and i >= 0:
                    biglist.append(i)
        elif len(thingy) == 3:
            a = int(thingy[0])
            b = int(thingy[1])
            c = int(thingy[2])
            for i in range(a,b+1,c):
                if i <= nProcs and i >= 0:
                    biglist.append(i)                
        else:
            print 'invalid subargument to -soft: %s' % (softness)
            print 'should be a or a:b or a:b:c'
            usage()

        if len(biglist) == 0:
            print '-soft argument %s allows no valid number of processes' % (softness)
            usage()
        else:
            return max(biglist)


def read_machinefile(machineFilename):
    if not machineFilename:
        return None
    try:
        machineFile = open(machineFilename,'r')
    except:
        print '** unable to open machinefile'
        exit(-1)
    procID = 0
    machineFileInfo = {}
    for line in machineFile:
        line = line.strip()
        if not line  or  line[0] == '#':
            continue
        splitLine = re.split(r'\s+',line)
        host = splitLine[0]
        if ':' in host:
            (host,nprocs) = host.split(':',1)
            nprocs = int(nprocs)
        else:
            nprocs = 1
        kvps = {'ifhn' : ''}
        for kv in splitLine[1:]:
            (k,v) = kv.split('=',1)
            if k == 'ifhn':  # interface hostname
                kvps[k] = v
            else:  # may be other kv pairs later
                print 'unrecognized key in machinefile:', k
                exit(-1)
        for i in range(procID,procID+nprocs):
            machineFileInfo[i] = { 'host' : host, 'nprocs' : nprocs }
            machineFileInfo[i].update(kvps)
        procID += nprocs
    return machineFileInfo

def handle_man_input(sock,streamHandler):
    global numDoneWithIO, myExitStatus
    global outXmlDoc, outECs
    msg = sock.recv_dict_msg()
    if not msg:
        streamHandler.del_handler(sock)
        numDoneWithIO += 1
    elif not msg.has_key('cmd'):
        mpd_print(1,'mpiexec: from man, invalid msg=:%s:' % (msg) )
        exit(-1)
    elif msg['cmd'] == 'execution_problem':
        # print 'rank %d (%s) in job %s failed to find executable %s' % \
              # ( msg['rank'], msg['src'], msg['jobid'], msg['exec'] )
        host = msg['src'].split('_')[0]
        reason = unquote(msg['reason'])
        print 'problem with execution of %s  on  %s:  %s ' % \
              (msg['exec'],host,reason)
        # keep going until all man's finish
    elif msg['cmd'] == 'job_aborted_early':
        print 'rank %d in job %s caused collective abort of all ranks' % \
              ( msg['rank'], msg['jobid'] )
        status = msg['exit_status']
        if WIFSIGNALED(status):
            if status > myExitStatus:
                myExitStatus = status
            killed_status = status & 0x007f  # AND off core flag
            print '  exit status of rank %d: killed by signal %d ' % \
                  (msg['rank'],killed_status)
        else:
            exit_status = WEXITSTATUS(status)
            if exit_status > myExitStatus:
                myExitStatus = exit_status
            print '  exit status of rank %d: return code %d ' % \
                  (msg['rank'],exit_status)
    elif msg['cmd'] == 'job_aborted':
        print 'job aborted; reason = %s' % (msg['reason'])
    elif msg['cmd'] == 'client_exit_status':
        if outECs:
            if parmdb['ecfn_format'] == 'xml':
                outXmlProc = outXmlDoc.createElement('exit-code')
                outECs.appendChild(outXmlProc)
                outXmlProc.setAttribute('rank',str(msg['cli_rank']))
                outXmlProc.setAttribute('status',str(msg['cli_status']))
                outXmlProc.setAttribute('pid',str(msg['cli_pid']))
                outXmlProc.setAttribute('host',msg['cli_host'])  # cli_ifhn is also avail
            else:
                outECs += 'rank=%d status=%d pid=%d host=%s\n' % \
                          (msg['cli_rank'],msg['cli_status'],msg['cli_pid'],msg['cli_host'])

        # print "exit info: rank=%d  host=%s  pid=%d  status=%d" % \
              # (msg['cli_rank'],msg['cli_host'],
               # msg['cli_pid'],msg['cli_status'])
        status = msg['cli_status']
        if WIFSIGNALED(status):
            if status > myExitStatus:
                myExitStatus = status
            killed_status = status & 0x007f  # AND off core flag
            # print 'exit status of rank %d: killed by signal %d ' % \
            #        (msg['cli_rank'],killed_status)
        else:
            exit_status = WEXITSTATUS(status)
            if exit_status > myExitStatus:
                myExitStatus = exit_status
            # print 'exit status of rank %d: return code %d ' % \
            #       (msg['cli_rank'],exit_status)
    else:
        print 'unrecognized msg from manager :%s:' % msg

def handle_cli_stdout_input(sock,parmdb,streamHandler,linesPerRank):
    global numDoneWithIO
    if parmdb['MPIEXEC_MERGE_OUTPUT']:
        line = sock.recv_one_line()
        if not line:
            streamHandler.del_handler(sock)
            numDoneWithIO += 1
        else:
            if parmdb['MPIEXEC_GDB']:
                line = line.replace('(gdb)\n','(gdb) ')
            try:
                (rank,rest) = line.split(':',1)
                rank = int(rank)
                linesPerRank[rank].append(rest)
            except:
                print line
            print_ready_merged_lines(parmdb['nprocs'],parmdb,linesPerRank)
    else:
        msg = sock.recv(1024)
        if not msg:
            streamHandler.del_handler(sock)
            numDoneWithIO += 1
        else:
            stdout.write(msg)
            stdout.flush()


def handle_cli_stderr_input(sock,streamHandler):
    global numDoneWithIO
    msg = sock.recv(1024)
    if not msg:
        streamHandler.del_handler(sock)
        numDoneWithIO += 1
    else:
        stderr.write(msg)
        stderr.flush()

# NOTE: stdin is supposed to be slow, low-volume.  We read it all here (as it
# appears on the fd) and send it immediately to the receivers.  If the user 
# redirects a "large" file (perhaps as small as 5k) into us, we will send it
# all out right away.  This can cause things to hang on the remote (recvr) side.
# We do not wait to read here until the recvrs read because there may be several
# recvrs and they may read at different speeds/times.
def handle_stdin_input(stdin_stream,parmdb,streamHandler,manSock):
    try:
        line = stdin_stream.readline()
    except IOError:
        stdin.flush()  # probably does nothing
    else:
        gdbFlag = parmdb['MPIEXEC_GDB']
        if line:    # not EOF
            msgToSend = { 'cmd' : 'stdin_from_user', 'line' : line } # default
            if gdbFlag and line.startswith('z'):
                line = line.rstrip()
                if len(line) < 3:    # just a 'z'
                    line += ' 0-%d' % (parmdb['nprocs']-1)
                s1 = line[2:].rstrip().split(',')
                for s in s1:
                    s2 = s.split('-')
                    for i in s2:
                        if not i.isdigit():
                            print 'invalid arg to z :%s:' % i
                            continue
                msgToSend = { 'cmd' : 'stdin_dest', 'stdin_procs' : line[2:] }
                stdout.softspace = 0
                print '%s:  (gdb) ' % (line[2:]),
            elif gdbFlag and line.startswith('q'):
                msgToSend = { 'cmd' : 'stdin_dest',
                              'stdin_procs' : '0-%d' % (parmdb['nprocs']-1) }
                if manSock:
                    manSock.send_dict_msg(msgToSend)
                msgToSend = { 'cmd' : 'stdin_from_user','line' : 'q\n' }
            elif gdbFlag and line.startswith('^'):
                msgToSend = { 'cmd' : 'stdin_dest',
                              'stdin_procs' : '0-%d' % (parmdb['nprocs']-1) }
                if manSock:
                    manSock.send_dict_msg(msgToSend)
                msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
            if manSock:
                manSock.send_dict_msg(msgToSend)
        else:
            streamHandler.del_handler(stdin)
            stdin.close()
            if manSock:
                msgToSend = { 'cmd' : 'stdin_from_user', 'eof' : '' }
                manSock.send_dict_msg(msgToSend)

def handle_sig_occurred(manSock):
    global sigOccurred
    if sigOccurred == SIGINT:
        if manSock:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
            manSock.send_dict_msg(msgToSend)
            manSock.close()
        exit(-1)
    elif sigOccurred == SIGALRM:
        if manSock:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGKILL' }
            manSock.send_dict_msg(msgToSend)
            manSock.close()
        mpd_print(1,'job terminating due to timeout')
        exit(-1)
    elif sigOccurred == SIGTSTP:
        sigOccurred = 0  # do this before kill below
        if manSock:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGTSTP' }
            manSock.send_dict_msg(msgToSend)
        signal(SIGTSTP,SIG_DFL)      # stop myself
        kill(getpid(),SIGTSTP)
        signal(SIGTSTP,sig_handler)  # restore this handler
    elif sigOccurred == SIGCONT:
        sigOccurred = 0  # do it before handling
        if manSock:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGCONT' }
            manSock.send_dict_msg(msgToSend)

def sig_handler(signum,frame):
    global sigOccurred
    sigOccurred = signum
    mpd_handle_signal(signum,frame)

def format_sorted_ranks(ranks):
    all = []
    one = []
    prevRank = -999
    for i in range(len(ranks)):
        if i != 0  and  ranks[i] != (prevRank+1):
            all.append(one)
            one = []
        one.append(ranks[i])
        if i == (len(ranks)-1):
            all.append(one)
        prevRank = ranks[i]
    pline = ''
    for i in range(len(all)):
        if len(all[i]) > 1:
            pline += '%d-%d' % (all[i][0],all[i][-1])
        else:
            pline += '%d' % (all[i][0])
        if i != (len(all)-1):
            pline += ','
    return pline

def print_ready_merged_lines(minRanks,parmdb,linesPerRank):
    printFlag = 1  # default to get started
    while printFlag:
        printFlag = 0
        for r1 in range(parmdb['nprocs']):
            if not linesPerRank[r1]:
                continue
            sortedRanks = []
            lineToPrint = linesPerRank[r1][0]
            for r2 in range(parmdb['nprocs']):
                if linesPerRank[r2] and linesPerRank[r2][0] == lineToPrint: # myself also
                    sortedRanks.append(r2)
            if len(sortedRanks) >= minRanks:
                fsr = format_sorted_ranks(sortedRanks)
                stdout.softspace = 0
                print '%s: %s' % (fsr,lineToPrint),
                for r2 in sortedRanks:
                    linesPerRank[r2] = linesPerRank[r2][1:]
                printFlag = 1
    stdout.flush()

def get_parms_from_rcfile(parmsToOverride):
    global parmdb
    try:
        parmsRCFilename = environ['HOME'] + '/.mpd.conf'
        parmsRCFile = open(parmsRCFilename,'r')
    except:
        return  # OK; treat as empty file
    for line in parmsRCFile:
        line = line.strip()
        withoutComments = line.split('#')[0]    # will at least be ''
        splitLine = withoutComments.rstrip().split('=')
        if splitLine  and  not splitLine[0]:    # ['']
            continue
        if len(splitLine) == 2:
            (k,v) = splitLine
            origKey = k
            if not k.startswith('MPIEXEC_'):  # may need to chk for other mpd pgms also
                continue
            if k in parmsToOverride.keys():
                if v.isdigit():
                    v = int(v)
                parmdb[('rcfile',k)] = v
            else:
                mpd_print(1,'invalid key in mpd conf file; key=:%s:' % (origKey) )
                exit(-1)
        else:
            mpd_print(1, 'line in mpd conf is not key=val pair; line=:%s:' % (line) )

def get_parms_from_env(parmsToOverride):
    global parmdb
    for k in parmsToOverride.keys():
        if environ.has_key(k):
            v = environ[k]
            if v.isdigit():
                v = int(v)
            parmdb[('env',k)] = v

def get_parms_from_xml_file(conSock,msgToMPD):
    global parmdb
    try:
        import xml.dom.minidom
    except:
        print 'you requested to parse an xml file, but'
        print '  I was unable to import the xml.dom.minidom module'
        exit(-1)
    known_rlimit_types = ['core','cpu','fsize','data','stack','rss',
                          'nproc','nofile','ofile','memlock','as','vmem']
    try:
        inXmlFilename = parmdb['inXmlFilename'] 
        parmsXMLFile = open(inXmlFilename,'r')
    except:
        print 'could not open job xml specification file %s' % (inXmlFilename)
        exit(-1)
    fileContents = parmsXMLFile.read()
    try:
        parsedXML = xml.dom.minidom.parseString(fileContents)
    except:
        print "mpiexec failed parsing xml file (perhaps from mpiexec); here is the content:"
        print fileContents
        exit(-1)
    if parsedXML.documentElement.tagName != 'create-process-group':
        print 'expecting create-process-group; got unrecognized doctype: %s' % \
              (parsedXML.documentElement.tagName)
        exit(-1)
    cpg = parsedXML.getElementsByTagName('create-process-group')[0]
    if cpg.hasAttribute('totalprocs'):
        parmdb[('xml','nprocs')] = int(cpg.getAttribute('totalprocs'))
    else:
        print '** totalprocs not specified in %s' % inXmlFilename
        exit(-1)
    if cpg.hasAttribute('try_1st_locally'):
        parmdb[('xml','MPIEXEC_TRY_1ST_LOCALLY')] = int(cpg.getAttribute('try_1st_locally'))
    if cpg.hasAttribute('output')  and  cpg.getAttribute('output') == 'label':
        parmdb[('xml','MPIEXEC_LINE_LABELS')] = 1
    if cpg.hasAttribute('pgid'):    # our jobalias
        parmdb[('xml','MPIEXEC_JOB_ALIAS')] = cpg.getAttribute('pgid')
    if cpg.hasAttribute('stdin_dest'):
        parmdb[('xml','MPIEXEC_STDIN_DEST')] = cpg.getAttribute('stdin_dest')
    if cpg.hasAttribute('doing_bnr'):
        parmdb[('xml','MPIEXEC_BNR')] = int(cpg.getAttribute('doing_bnr'))
    if cpg.hasAttribute('ifhn'):
        parmdb[('xml','MPIEXEC_IFHN')] = cpg.getAttribute('ifhn')
    if cpg.hasAttribute('exit_codes_filename'):
        parmdb[('xml','MPIEXEC_EXITCODES_FILENAME')] = cpg.getAttribute('exit_codes_filename')
        parmdb[('xml','ecfn_format')] = 'xml'
    if cpg.hasAttribute('gdb'):
        gdbFlag = int(cpg.getAttribute('gdb'))
        if gdbFlag:
            parmdb[('xml','MPIEXEC_GDB')]     = 1
            parmdb[('xml','MPIEXEC_MERGE_OUTPUT')] = 1   # implied
            parmdb[('xml','MPIEXEC_LINE_LABELS')]  = 1   # implied
            parmdb[('xml','MPIEXEC_STDIN_DEST')]   = 'all'
    if cpg.hasAttribute('tv'):
        parmdb[('xml','MPIEXEC_TOTALVIEW')] = int(cpg.getAttribute('tv'))
    hostSpec = cpg.getElementsByTagName('host-spec')
    if hostSpec:
        hostList = []
        for node in hostSpec[0].childNodes:
            node = node.data.strip()
            hostnames = re.findall(r'\S+',node)
            for hostname in hostnames:
                if hostname:    # some may be the empty string
                    try:
                        ipaddr = gethostbyname_ex(hostname)[2][0]
                    except:
                        print 'unable to determine IP info for host %s' % (hostname)
                        exit(-1)
                    hostList.append(ipaddr)
        parmdb[('xml','MPIEXEC_HOST_LIST')] = hostList
    if hostSpec and hostSpec[0].hasAttribute('check'):
        hostSpecMode = hostSpec[0].getAttribute('check')
        if hostSpecMode == 'yes':
            msgToSend = { 'cmd' : 'verify_hosts_in_ring', 'host_list' : hostList }
            conSock.send_dict_msg(msgToSend)
            msg = conSock.recv_dict_msg(timeout=recvTimeout)
            if not msg:
                mpd_print(1,'no msg recvd from mpd mpd during chk hosts up')
                exit(-1)
            elif msg['cmd'] != 'verify_hosts_in_ring_response':
                mpd_print(1,'unexpected msg from mpd :%s:' % (msg) )
                exit(-1)
            if msg['host_list']:
                print 'These hosts are not in the mpd ring:'
                for host in  msg['host_list']:
                    if host[0].isdigit():
                        print '    %s' % (host),
                        try:
                            print ' (%s)' % (gethostbyaddr(host)[0])
                        except:
                            print ''
                    else:
                        print '    %s' % (host)
                exit(-1)
    covered = [0] * parmdb['nprocs'] 
    procSpec = cpg.getElementsByTagName('process-spec')
    if not procSpec:
        print 'No process-spec specified'
        usage()
    for p in procSpec:
        if p.hasAttribute('range'):
            therange = p.getAttribute('range')
            splitRange = therange.split('-')
            if len(splitRange) == 1:
                loRange = int(splitRange[0])
                hiRange = loRange
            else:
                (loRange,hiRange) = (int(splitRange[0]),int(splitRange[1]))
        else:
            (loRange,hiRange) = (0,parmdb['nprocs']-1)
        for i in xrange(loRange,hiRange+1):
            nprocs = parmdb['nprocs']
            if i >= nprocs:
                print '*** exiting; rank %d is greater than nprocs' % (nprocs)
                exit(-1)
            if covered[i]:
                print '*** exiting; rank %d is doubly used in proc specs' % (nprocs)
                exit(-1)
            covered[i] = 1
        if p.hasAttribute('exec'):
            msgToMPD['execs'][(loRange,hiRange)] = p.getAttribute('exec')
        else:
            print '*** exiting; range %d-%d has no exec' % (loRange,hiRange)
            exit(-1)
        if p.hasAttribute('user'):
            tempuser = p.getAttribute('user')
            try:
                pwent = getpwnam(tempuser)
            except:
                pwent = None
            if not pwent:
                print tempuser, 'is an invalid username'
                exit(-1)
            if tempuser == mpd_get_my_username()  or  getuid() == 0:
                msgToMPD['users'][(loRange,hiRange)] = p.getAttribute('user')
            else:
                print tempuser, 'username does not match yours and you are not root'
                exit(-1)
        else:
            msgToMPD['users'][(loRange,hiRange)] = mpd_get_my_username()
        if p.hasAttribute('cwd'):
            msgToMPD['cwds'][(loRange,hiRange)] = p.getAttribute('cwd')
        else:
            msgToMPD['cwds'][(loRange,hiRange)] = path.abspath(getcwd())
        if p.hasAttribute('umask'):
            msgToMPD['umasks'][(loRange,hiRange)] = p.getAttribute('umask')
        else:
            currumask = umask(0) ; umask(currumask)
            msgToMPD['umasks'][(loRange,hiRange)] = str(currumask)
        if p.hasAttribute('path'):
            msgToMPD['paths'][(loRange,hiRange)] = p.getAttribute('path')
        else:
            msgToMPD['paths'][(loRange,hiRange)] = environ['PATH']
        if p.hasAttribute('host'):
            host = p.getAttribute('host')
            if host.startswith('_any_'):
                msgToMPD['hosts'][(loRange,hiRange)] = host
            else:
                try:
                    msgToMPD['hosts'][(loRange,hiRange)] = gethostbyname_ex(host)[2][0]
                except:
                    print 'unable to do find info for host %s' % (host)
                    exit(-1)
        else:
            if hostSpec  and  hostList:
                msgToMPD['hosts'][(loRange,hiRange)] = '_any_from_pool_'
            else:
                msgToMPD['hosts'][(loRange,hiRange)] = '_any_'
        argDict = {}
        argList = p.getElementsByTagName('arg')
        for argElem in argList:
            argDict[int(argElem.getAttribute('idx'))] = argElem.getAttribute('value')
        argVals = [0] * len(argList)
        for i in argDict.keys():
            argVals[i-1] = unquote(argDict[i])
        msgToMPD['args'][(loRange,hiRange)] = argVals
        limitDict = {}
        limitList = p.getElementsByTagName('limit')
        for limitElem in limitList:
            typ = limitElem.getAttribute('type')
            if typ in known_rlimit_types:
                limitDict[typ] = limitElem.getAttribute('value')
            else:
                print 'mpiexec: invalid type in limit: %s' % (typ)
                exit(-1)
        msgToMPD['limits'][(loRange,hiRange)] = limitDict
        envVals = {}
        envVarList = p.getElementsByTagName('env')
        for envVarElem in envVarList:
            envkey = envVarElem.getAttribute('name')
            envval = unquote(envVarElem.getAttribute('value'))
            envVals[envkey] = envval
        msgToMPD['envvars'][(loRange,hiRange)] = envVals
    for i in range(len(covered)):
        if not covered[i]:
            print '*** exiting; %d procs are requested, but proc %d is not described' % \
                  (parmdb['nprocs'],i)
            exit(-1)
        
def get_vals_for_attach(parmdb,conSock,msgToMPD):
    sjobid = parmdb['-gdba'].split('@')    # jobnum and originating host
    msgToSend = { 'cmd' : 'mpdlistjobs' }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=recvTimeout)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
        exit(-1)
    if msg['cmd'] != 'local_mpdid':     # get full id of local mpd for filters later
        mpd_print(1,'did not recv local_mpdid msg from local mpd; recvd: %s' % msg)
        exit(-1)
    else:
        if len(sjobid) == 1:
            sjobid.append(msg['id'])
    got_info = 0
    while 1:
        msg = conSock.recv_dict_msg()
        if not msg.has_key('cmd'):
            mpd_print(1,'invalid message from mpd :%s:' % (msg))
            exit(-1)
        if msg['cmd'] == 'mpdlistjobs_info':
            got_info = 1
            smjobid = msg['jobid'].split('  ')  # jobnum, mpdid, and alias (if present)
            if sjobid[0] == smjobid[0]  and  sjobid[1] == smjobid[1]:  # jobnum and mpdid
                rank = int(msg['rank'])
                msgToMPD['users'][(rank,rank)]   = msg['username']
                msgToMPD['hosts'][(rank,rank)]   = msg['ifhn']
                msgToMPD['execs'][(rank,rank)]   = msg['pgm']
                msgToMPD['cwds'][(rank,rank)]    = path.abspath(getcwd())
                msgToMPD['paths'][(rank,rank)]   = environ['PATH']
                msgToMPD['args'][(rank,rank)]    = [msg['clipid']]
                msgToMPD['envvars'][(rank,rank)] = {}
                msgToMPD['limits'][(rank,rank)]  = {}
        elif  msg['cmd'] == 'mpdlistjobs_trailer':
            if not got_info:
                print 'no info on this jobid; probably invalid'
                exit(-1)
            break
        else:
            print 'invaild msg from mpd :%s:' % (msg)
            exit(-1)
    parmdb[('thispgm','nprocs')] = len(msgToMPD['execs'].keys())  # all dicts are same len
    

def usage():
    print __doc__
    exit(-1)


if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass
    exit(myExitStatus)

