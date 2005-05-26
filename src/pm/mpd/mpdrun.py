#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdrun [args] pgm_to_execute [pgm_args]
   where args may be: -a alias -np nprocs -cpm master_copgm -cpr remote_copgm -l -m -1 -s
       (nprocs must be a positive integer)
       (-h or --help  provides this msg; should be used alone)
       (-l means attach line labels identifying which client prints each line)
       (-m means merge identical outputs into a single line;
           implies that program produces whole lines;
           implies -l)
       (-1 means do NOT start the first process locally)
       (-a means assign this alias to the job)
       (-s means send stdin to all processes; not just first)
       (-gdb means run all procs under gdb; implies -m and -l and initially -s )
       (-gdba <jobid>  means gdb-attach to existing jobid)
       (-r output_xml_exit_codes_filename)
       (-ifhn is the interface hostname to use on the local machine)
       (-mpdiconf  says to ignore parms coded in $HOME/.mpd.conf)
       (-mpdienv says to ignore parms coded in the environment)
       (-mpdppdball print all of parm data base after fully populated from all sources)
       (-mpdppdbdef print just the default values from parm data base)
or:    mpdrun -f input_xml_filename 
   where filename contains all the arguments in xml format
"""


from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""

from signal  import signal, alarm, SIG_DFL, SIG_IGN, SIGINT, SIGTSTP, \
                    SIGCONT, SIGALRM, SIGTTIN

signal(SIGTTIN,SIG_IGN)

from  sys         import argv, stdin, stdout, stderr, exit
from  os          import environ, fork, execvpe, getuid, getpid, path, getcwd, \
                         wait, waitpid, kill, unlink, umask, system, _exit,  \
                         WIFSIGNALED, WEXITSTATUS
from  pwd         import getpwnam
from  socket      import gethostname, gethostbyname_ex, gethostbyaddr
from  time        import time
from  re          import findall, split
from  urllib      import unquote
from  mpdlib      import mpd_set_my_id, mpd_get_my_username, mpd_version, mpd_print, \
                         mpd_uncaught_except_tb, mpd_handle_signal, mpd_which, \
                         MPDListenSock, MPDStreamHandler, MPDConsClientSock, MPDParmDB
import xml.dom.minidom

global numDoneWithIO, myExitStatus, sigOccurred
global outXmlDoc, outXmlECs

recvTimeout = 20  # const

def mpdrun():
    global numDoneWithIO, myExitStatus, sigOccurred
    global outXmlDoc, outXmlECs
    import sys    # to get access to excepthook in next line

    sys.excepthook = mpd_uncaught_except_tb

    myHost = gethostname()
    try:
        hostinfo = gethostbyname_ex(myHost)
    except:
        print 'mpd failed: gethostbyname_ex failed for %s' % (myHost)
        exit(-1)
    myIP = hostinfo[2][0]

    mpd_set_my_id(myid='mpdrun_%s' % (myHost) )
    
    numDoneWithIO = 0
    myExitStatus = 0
    outXmlDoc = ''
    outXmlECs = ''
    outXmlECFile = None
    sigOccurred = 0

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPDRUN_IFHN'               :  myIP,
                        'MPDRUN_LABEL_LINES'        :  0,
                        'MPDRUN_MERGE_OUTPUT'       :  0,
                        'MPDRUN_TOTALVIEW'          :  0,
                        'MPDRUN_MSHIP'              :  '',
                        'MPDRUN_RSHIP'              :  '',
                        'MPDRUN_JOB_ALIAS'          :  '',
                        'MPDRUN_TRY_1ST_LOCALLY'    :  1,
                        'MPDRUN_STDIN_DEST'         :  '0',
                        'MPDRUN_HOST_LIST'          :  [],
                        'MPDRUN_GDB_FLAG'           :  0,
                        'MPDRUN_GDB_ATTACH_JOBID'   :  '',
                        'MPDRUN_SINGINIT_PID'       :  0,
                        'MPDRUN_SINGINIT_PORT'      :  0,
                        'MPDRUN_MPICH1_COMPAT'      :  0,
                        'MPDRUN_TIMEOUT'            :  0,
                        'MPDRUN_EXITCODES_FILENAME' :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    # other parms used in this pgm
    parmdb[('thispgm','userpgm')] = ''
    parmdb[('thispgm','nprocs')] = 0
    parmdb[('thispgm','ignore_rcfile')] = 0
    parmdb[('thispgm','ignore_environ')] = 0
    parmdb[('thispgm','delXMLFile')] = 0
    parmdb[('thispgm','inXmlFilename')] = ''
    parmdb[('thispgm','print_parmdb_all')] = 0
    parmdb[('thispgm','print_parmdb_def')] = 0

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

    get_special_parms_from_cmdline(parmdb)

    listenSock = MPDListenSock('',0,name='socket_to_listen_for_man')
    listenPort = listenSock.getsockname()[1]
    conSock = MPDConsClientSock()  # looks for MPD_UNIX_SOCKET in env
    msgToSend = { 'cmd' : 'get_mpd_version' }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=recvTimeout)
    if not msg:
        mpd_print(1, 'no msg recvd from mpd during version check')
        exit(-1)
    elif msg['cmd'] != 'mpd_version_response':
        mpd_print(1,'unexpected msg from mpd :%s:' % (msg) )
        exit(-1)
    if msg['mpd_version'] != mpd_version():
        mpd_print(1,'mpd version %s does not match mpdrun version %s' % \
                  (msg['mpd_version'],mpd_version()) )
        exit(-1)

    if not parmdb['ignore_rcfile']:
        get_parms_from_rcfile(parmdb,parmsToOverride)
    if not parmdb['ignore_environ']:
        get_parms_from_env(parmdb,parmsToOverride)
    if parmdb['MPDRUN_GDB_ATTACH_JOBID']:
        get_vals_for_attach(parmdb,conSock,msgToMPD)
    elif parmdb['inXmlFilename']:    # get these after we have a conn to mpd
        get_parms_from_xml_file(parmdb,conSock,msgToMPD)
    else:
        get_parms_from_cmdline(parmdb,msgToMPD)
    if parmdb['print_parmdb_all']:
        parmdb.printall()
    if parmdb['print_parmdb_def']:
        parmdb.printdef()
    nprocs = parmdb['nprocs']

    if parmdb['MPDRUN_MSHIP']:
        mshipSock = MPDListenSock('',0,name='socket_for_mship')
        mshipPort = mshipSock.getsockname()[1]
        mshipPid = fork()
        if mshipPid == 0:
            conSock.close()
            environ['MPDCP_AM_MSHIP'] = '1'
            environ['MPDCP_MSHIP_PORT'] = str(mshipPort)
            environ['MPDCP_MSHIP_FD'] = str(mshipSock.fileno())
            environ['MPDCP_MSHIP_NPROCS'] = str(nprocs)
            try:
                execvpe(parmdb['mship'],[parmdb['MPDRUN_MSHIP']],environ)
            except Exception, errmsg:
                mpd_print(1,'execvpe failed for copgm %s; errmsg=:%s:' % \
                          (parmdb['MPDRUN_MSHIP'],errmsg))
                exit(-1)
            _exit(0);  # do NOT do cleanup
        mshipSock.close()
    else:
        mshipPid = 0

    # make sure to do this after nprocs has its value
    linesPerRank = {}  # keep this a dict instead of a list
    for i in range(nprocs):
        linesPerRank[i] = []

    if parmdb['MPDRUN_EXITCODES_FILENAME']:
        outXmlECFile = open(parmdb['MPDRUN_EXITCODES_FILENAME'],'w')
        outXmlDoc = xml.dom.minidom.Document()
        outXmlECs = outXmlDoc.createElement('exit-codes')
        outXmlDoc.appendChild(outXmlECs)

    msgToMPD['nprocs'] = parmdb['nprocs']
    msgToMPD['conport'] = listenPort
    msgToMPD['conip'] = myIP
    msgToMPD['conifhn'] = parmdb['MPDRUN_IFHN']
    if parmdb['MPDRUN_JOB_ALIAS']:
        msgToMPD['jobalias'] = parmdb['MPDRUN_JOB_ALIAS']
    else:
        msgToMPD['jobalias'] = ''
    if parmdb['MPDRUN_TRY_1ST_LOCALLY']:
        msgToMPD['try_1st_locally'] = 1
    if parmdb['MPDRUN_LABEL_LINES']:
        msgToMPD['line_labels'] = 1
    if parmdb['MPDRUN_RSHIP']:
        msgToMPD['rship'] = parmdb['MPDRUN_RSHIP']
        msgToMPD['mship_host'] = gethostname()
        msgToMPD['mship_port'] = mshipPort
    if parmdb['MPDRUN_MPICH1_COMPAT']:
        msgToMPD['doing_bnr'] = 1
    if parmdb['MPDRUN_STDIN_DEST'] == 'all':
        stdinDest = '0-%d' % (nprocs-1)
    else:
        stdinDest = parmdb['MPDRUN_STDIN_DEST']
    msgToMPD['stdin_dest'] = stdinDest
    msgToMPD['gdb'] = parmdb['MPDRUN_GDB_FLAG']
    msgToMPD['totalview'] = parmdb['MPDRUN_TOTALVIEW']
    msgToMPD['singinitpid'] = parmdb['MPDRUN_SINGINIT_PID']
    msgToMPD['singinitport'] = parmdb['MPDRUN_SINGINIT_PORT']
    msgToMPD['host_spec_pool'] = parmdb['MPDRUN_HOST_LIST']

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
                print 'mpdrun: unable to start all procs; may have invalid machine names'
                print '    remaining specified hosts:'
                for host in msg['remaining_hosts'].values():
                    if host != '_any_':
                        try:
                            print '        %s (%s)' % (host,gethostbyaddr(host)[0])
                        except:
                            print '        %s' % (host)
            elif  msg['reason'] == 'invalid_username':
                print 'mpdrun: invalid username %s at host %s' % \
                      (msg['username'],msg['host'])
            else:
                print 'mpdrun: job failed; reason=:%s:' % (msg['reason'])
            exit(-1)
        else:
            mpd_print(1, 'unexpected message from mpd: %s' % (msg) )
            exit(-1)
    conSock.close()
    jobTimeout = parmdb['MPDRUN_TIMEOUT']
    if jobTimeout:
        alarm(jobTimeout)

    streamHandler = MPDStreamHandler()

    (manSock,addr) = listenSock.accept()
    if not manSock:
        mpd_print(1, 'mpdrun: failed to obtain sock from manager')
        exit(-1)
    streamHandler.set_handler(manSock,handle_man_input,args=(streamHandler,))
    streamHandler.set_handler(stdin,handle_stdin_input,args=(parmdb,streamHandler,manSock))
    # first, do handshaking with man
    msg = manSock.recv_dict_msg()
    if (not msg  or  not msg.has_key('cmd') or msg['cmd'] != 'man_checking_in'):
        mpd_print(1, 'mpdrun: from man, invalid msg=:%s:' % (msg) )
        exit(-1)
    msgToSend = { 'cmd' : 'ringsize', 'ring_ncpus' : currRingNCPUs,
                  'ringsize' : currRingSize }
    manSock.send_dict_msg(msgToSend)
    msg = manSock.recv_dict_msg()
    if (not msg  or  not msg.has_key('cmd')):
        mpd_print(1, 'mpdrun: from man, invalid msg=:%s:' % (msg) )
        exit(-1)
    if (msg['cmd'] == 'job_started'):
        jobid = msg['jobid']
        if outXmlECs:
            outXmlECs.setAttribute('jobid',jobid.strip())
        # print 'mpdrun: job %s started' % (jobid)
        if parmdb['MPDRUN_TOTALVIEW']:
            if not mpd_which('totalview'):
                print 'cannot find "totalview" in your $PATH:'
                print '    ', environ['PATH']
                exit(-1)
            import mtv
            tv_cmd = 'dattach python ' + `getpid()` + '; dgo; dassign MPIR_being_debugged 1'
            system('totalview -e "%s" &' % (tv_cmd) )
            mtv.wait_for_debugger()
            mtv.allocate_proctable(nprocs)
            # extract procinfo (rank,hostname,exec,pid) tuples from msg
            for i in range(nprocs):
                tvhost = msg['procinfo'][i][0]
                tvpgm  = msg['procinfo'][i][1]
                tvpid  = msg['procinfo'][i][2]
                # print "%d %s %s %d" % (i,host,pgm,pid)
                mtv.append_proctable_entry(tvhost,tvpgm,tvpid)
            mtv.complete_spawn()
            msgToSend = { 'cmd' : 'tv_ready' }
            manSock.send_dict_msg(msgToSend)
    else:
        mpd_print(1, 'mpdrun: from man, unknown msg=:%s:' % (msg) )
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
        if parmdb['MPDRUN_MERGE_OUTPUT']:
            if timeForPrint < time():
                print_ready_merged_lines(1,parmdb,linesPerRank)
                timeForPrint = time() + timeDelayForPrints
            else:
                print_ready_merged_lines(nprocs,parmdb,linesPerRank)

    if parmdb['MPDRUN_MERGE_OUTPUT']:
        print_ready_merged_lines(1,parmdb,linesPerRank)
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)
    if outXmlECFile:
        print >>outXmlECFile, outXmlDoc.toprettyxml(indent='   ')
        outXmlECFile.close()
    return myExitStatus

def handle_man_input(sock,streamHandler):
    global numDoneWithIO, myExitStatus, sigOccurred
    global outXmlDoc, outXmlECs
    msg = sock.recv_dict_msg()
    if not msg:
        streamHandler.del_handler(sock)
        numDoneWithIO += 1
    elif not msg.has_key('cmd'):
        mpd_print(1,'mpdrun: from man, invalid msg=:%s:' % (msg) )
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
        if outXmlDoc:
            outXmlProc = outXmlDoc.createElement('exit-code')
            outXmlECs.appendChild(outXmlProc)
            outXmlProc.setAttribute('rank',str(msg['cli_rank']))
            outXmlProc.setAttribute('status',str(msg['cli_status']))
            outXmlProc.setAttribute('pid',str(msg['cli_pid']))
            outXmlProc.setAttribute('host',msg['cli_host'])  # cli_ifhn is also available
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
    global numDoneWithIO, myExitStatus, sigOccurred
    if parmdb['MPDRUN_MERGE_OUTPUT']:
        line = sock.recv_one_line()
        if not line:
            streamHandler.del_handler(sock)
            numDoneWithIO += 1
        else:
            if parmdb['MPDRUN_GDB_FLAG']:
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
    global numDoneWithIO, myExitStatus, sigOccurred
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
        gdbFlag = parmdb['MPDRUN_GDB_FLAG']
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

def get_parms_from_rcfile(parmdb,parmsToOverride):
    try:
        parmsRCFilename = environ['HOME'] + '/.mpd.conf'
        parmsRCFile = open(parmsRCFilename,'r')
    except:
        print 'unable to open rc file:', parmsRCFilename
        exit(-1)
    for line in parmsRCFile:
        if line[0] == '#':
            continue
        line = line.rstrip()
        try:
            withoutComments = line.split('#')[0]
            splitLine = withoutComments.split('=')
        except:
            splitLine = []
        if len(splitLine) == 2:
            (k,v) = splitLine
            k = k.upper()
            if not k.startswith('MPDRUN_'):
                continue
        else:
            continue    # just skip it as unrecognized (perhaps a blank line)
        if k in parmsToOverride.keys():
            parmdb[('rcfile',k)] = v
        else:
            print 'mpdrun: invalid key in rc file; line=', line,

def get_parms_from_env(parmdb,parmsToOverride):
    if environ.has_key('MPIEXEC_MPICH1_COMPAT'):       # special case for mpiexec
        parmdb[('env','MPDRUN_MPICH1_COMPAT')] = 1
    if environ.has_key('MPIEXEC_TIMEOUT'):             # special case for mpiexec
        parmdb[('env','MPDRUN_TIMEOUT')] = int(environ['MPIEXEC_TIMEOUT'])
    for envvar in environ.keys():
        if envvar in parmsToOverride.keys():
            parmdb[('env',envvar)] = environ[envvar]

def get_special_parms_from_cmdline(parmdb):
    if argv[1] == '-h'  or  argv[1] == '--help':
        usage()
    if len(argv) == 3  and  argv[1] == '-delxmlfile':  # special case for mpiexec
        parmdb[('cmdline','delXMLFile')] = 1
        parmdb[('cmdline','inXmlFilename')] = argv[2]
        return
    if len(argv) == 3  and  argv[1] == '-f':
        parmdb[('cmdline','inXmlFilename')] = argv[2]
        return
    if argv[1] == '-gdba':
        if len(argv) != 3:
            print '-gdba (and its jobid) must be the only cmd-line args'
            usage()
        parmdb[('cmdline','MPDRUN_GDB_FLAG')] = 1
        parmdb[('cmdline','MPDRUN_MERGE_OUTPUT')] = 1   # implied
        parmdb[('cmdline','MPDRUN_LABEL_LINES')]  = 1   # implied
        parmdb[('cmdline','MPDRUN_STDIN_DEST')] = 'all'
        parmdb[('cmdline','MPDRUN_GDB_ATTACH_JOBID')] = argv[2]
        return
    if argv[1] == '-p':
        parmdb[('cmdline','MPDRUN_SINGINIT_PID')]  = argv[2]
        parmdb[('cmdline','MPDRUN_SINGINIT_PORT')] = argv[3]
        parmdb[('cmdline','userpgm')] = argv[4]
        parmdb[('cmdline','nprocs')] = 1
        parmdb[('cmdline','MPDRUN_TRY_1ST_LOCALLY')] = 1
        return
    argidx = 1
    while argidx < len(argv):
        if argv[argidx] == '-mpdiconf':
            parmdb[('cmdline','ignore_rcfile')] = 1
        elif argv[argidx] == '-mpdienv':
            parmdb[('cmdline','ignore_environ')] = 1
        elif argv[argidx] == '-mpdppdball':
            parmdb[('cmdline','print_parmdb_all')] = 1
        elif argv[argidx] == '-mpdppdbdef':
            parmdb[('cmdline','print_parmdb_def')] = 1
        argidx += 1

def get_parms_from_cmdline(parmdb,msgToMPD):
    argidx = 1
    userpgm = parmdb['userpgm']
    while userpgm == '':
        if argidx >= len(argv):
            usage()
        if argv[argidx][0] == '-':
            if argv[argidx] == '-np' or argv[argidx] == '-n':
                if not argv[argidx+1].isdigit():
                    print 'non-numeric arg to -n or -np'
                    usage()
                else:
                    parmdb[('cmdline','nprocs')] = int(argv[argidx+1])
                    if parmdb['nprocs'] < 1:
                        usage()
                    else:
                        argidx += 2
            elif argv[argidx] == '-a':
                parmdb[('cmdline','MPDRUN_JOB_ALIAS')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx] == '-r':
                parmdb[('cmdline','MPDRUN_EXITCODES_FILENAME')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx] == '-ifhn':
                parmdb[('cmdline','MPDRUN_IFHN')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx] == '-cpm':
                parmdb[('cmdline','MPDRUN_MSHIP')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx] == '-bnr':
                parmdb[('cmdline','MPDRUN_MPICH1_COMPAT')] = 1
                argidx += 1
            elif argv[argidx] == '-cpr':
                parmdb[('cmdline','MPDRUN_RSHIP')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx] == '-l':
                parmdb[('cmdline','MPDRUN_LABEL_LINES')] = 1
                argidx += 1
            elif argv[argidx] == '-m':
                parmdb[('cmdline','MPDRUN_MERGE_OUTPUT')] = 1
                parmdb[('cmdline','MPDRUN_LABEL_LINES')] = 1  # implied
                argidx += 1
            elif argv[argidx] == '-gdb':
                parmdb[('cmdline','MPDRUN_GDB_FLAG')]     = 1
                parmdb[('cmdline','MPDRUN_MERGE_OUTPUT')] = 1   # implied
                parmdb[('cmdline','MPDRUN_LABEL_LINES')]  = 1   # implied
                parmdb[('cmdline','MPDRUN_STDIN_DEST')]   = 'all'
                argidx += 1
            elif argv[argidx] == '-1' or argv[argidx] == '-nolocal':
                parmdb[('cmdline','MPDRUN_TRY_1ST_LOCALLY')] = 0
                argidx += 1
            elif argv[argidx] == '-s':
                parmdb[('cmdline','MPDRUN_STDIN_DEST')] = 'all'
                argidx += 1
            elif argv[argidx] == '-tv':
                parmdb[('cmdline','MPDRUN_TOTALVIEW')] = 1
                argidx += 1
            elif argv[argidx] == '-delxmlfile':
                argidx += 2    # already handled as special above
            elif argv[argidx] == '-f':
                argidx += 2    # already handled as special above
            elif argv[argidx] == '-gdba':
                argidx += 2    # already handled as special above
            elif argv[argidx] == '-p':
                argidx += 3    # already handled as special above
            elif argv[argidx] == '-mpdiconf':
                argidx += 1    # already handled as special above
            elif argv[argidx] == '-mpdienv':
                argidx += 1    # already handled as special above
            elif argv[argidx] == '-mpdppdball':
                argidx += 1    # already handled as special above
            elif argv[argidx] == '-mpdppdbdef':
                argidx += 1    # already handled as special above
            else:
                usage()
        else:
            userpgm = argv[argidx]
            parmdb[('cmdline','userpgm')] = userpgm
            argidx += 1
    pgmArgs = []
    while argidx < len(argv):
        pgmArgs.append(argv[argidx])
        argidx += 1
    if not parmdb['nprocs']:
        print 'you have to indicate how many processes to start'
        usage()
    nprocs = parmdb['nprocs']
    currumask = umask(0) ; umask(currumask)  # grab it and set it back
    msgToMPD['execs']   = { (0,nprocs-1) : parmdb['userpgm'] }
    msgToMPD['users']   = { (0,nprocs-1) : mpd_get_my_username() }
    msgToMPD['cwds']    = { (0,nprocs-1) : path.abspath(getcwd()) }
    msgToMPD['umasks']  = { (0,nprocs-1) : str(currumask) }
    msgToMPD['paths']   = { (0,nprocs-1) : environ['PATH'] }
    msgToMPD['args']    = { (0,nprocs-1) : pgmArgs }
    msgToMPD['limits']  = { (0,nprocs-1) : {} }
    cli_environ = {} 
    cli_environ.update(environ)
    msgToMPD['envvars'] = { (0,nprocs-1) : cli_environ }
    if parmdb['MPDRUN_HOST_LIST']:
        msgToMPD['hosts'][(0,nprocs-1)] = '_any_from_pool_'
    else:
        msgToMPD['hosts'][(0,nprocs-1)] = '_any_'

def get_parms_from_xml_file(parmdb,conSock,msgToMPD):
    known_rlimit_types = ['core','cpu','fsize','data','stack','rss',
                          'nproc','nofile','ofile','memlock','as','vmem']
    try:
        inXmlFilename = parmdb['inXmlFilename'] 
        parmsXMLFile = open(inXmlFilename,'r')
    except:
        print 'could not open job xml specification file %s' % (inXmlFilename)
        exit(-1)
    fileContents = parmsXMLFile.read()
    if parmdb['delXMLFile']:
        unlink(inXmlFilename)
    parsedXML = xml.dom.minidom.parseString(fileContents)
    if parsedXML.documentElement.tagName != 'create-process-group':
        print 'expecting create-process-group; got unrecognized doctype: %s' % \
              (parsedXML.documentElement.tagName)
        exit(-1)
    cpg = parsedXML.getElementsByTagName('create-process-group')[0]
    if cpg.hasAttribute('totalprocs'):
        parmdb[('xml','nprocs')] = int(cpg.getAttribute('totalprocs'))
    else:
        print '** totalprocs not specified in %s' % outXmlECFilename
        exit(-1)
    if cpg.hasAttribute('try_1st_locally'):
        parmdb[('xml','MPDRUN_TRY_1ST_LOCALLY')] = int(cpg.getAttribute('try_1st_locally'))
    if cpg.hasAttribute('output')  and  cpg.getAttribute('output') == 'label':
        parmdb[('xml','MPDRUN_LABEL_LINES')] = 1
    if cpg.hasAttribute('pgid'):    # our jobalias
        parmdb[('xml','MPDRUN_JOB_ALIAS')] = cpg.getAttribute('pgid')
    if cpg.hasAttribute('stdin_dest'):
        parmdb[('xml','MPDRUN_STDIN_DEST')] = cpg.getAttribute('stdin_dest')
    if cpg.hasAttribute('doing_bnr'):
        parmdb[('xml','MPDRUN_MPICH1_COMPAT')] = int(cpg.getAttribute('doing_bnr'))
    if cpg.hasAttribute('ifhn'):
        parmdb[('xml','MPDRUN_IFHN')] = cpg.getAttribute('ifhn')
    if cpg.hasAttribute('exit_codes_filename'):
        parmdb[('xml','MPDRUN_EXITCODES_FILENAME')] = cpg.getAttribute('exitcodes_filename')
    if cpg.hasAttribute('gdb'):
        gdbFlag = int(cpg.getAttribute('gdb'))
        if gdbFlag:
            parmdb[('xml','MPDRUN_GDB_FLAG')]     = 1
            parmdb[('xml','MPDRUN_MERGE_OUTPUT')] = 1   # implied
            parmdb[('xml','MPDRUN_LABEL_LINES')]  = 1   # implied
            parmdb[('xml','MPDRUN_STDIN_DEST')]   = 'all'
    if cpg.hasAttribute('tv'):
        parmdb[('xml','MPDRUN_TOTALVIEW')] = int(cpg.getAttribute('tv'))
    hostSpec = cpg.getElementsByTagName('host-spec')
    if hostSpec:
        hostList = []
        for node in hostSpec[0].childNodes:
            node = node.data.strip()
            hostnames = findall(r'\S+',node)
            for hostname in hostnames:
                if hostname:    # some may be the empty string
                    try:
                        ipaddr = gethostbyname_ex(hostname)[2][0]
                    except:
                        print 'unable to determine IP info for host %s' % (hostname)
                        exit(-1)
                    hostList.append(ipaddr)
        parmdb[('xml','MPDRUN_HOST_LIST')] = hostList
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
                print 'mpdrun: invalid type in limit: %s' % (typ)
                exit(-1)
        msgToMPD['limits'][(loRange,hiRange)] = limitDict
        envVals = {}
        envVarList = p.getElementsByTagName('env')
        for envVarElem in envVarList:
            envkey = envVarElem.getAttribute('name')
            envval = envVarElem.getAttribute('value')
            envVals[envkey] = envval
        msgToMPD['envvars'][(loRange,hiRange)] = envVals
    for i in range(len(covered)):
        if not covered[i]:
            nprocs = parmdb['nprocs']
            print '*** exiting; %d procs are requested, but proc %d is not described' % \
                  (nprocs,i)
            exit(-1)
        
def get_vals_for_attach(parmdb,conSock,msgToMPD):
    sjobid = parmdb['MPDRUN_GDB_ATTACH_JOBID'].split('@')    # jobnum and originating host
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
    print 'mpdrun for mpd version: %s' % str(mpd_version())
    print __doc__
    exit(-1)


if __name__ == '__main__':
    exitStatus = mpdrun()
    exit(exitStatus)
