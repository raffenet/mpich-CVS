#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

try:
    from signal          import signal, alarm, SIG_DFL, SIG_IGN, SIGINT, SIGTSTP, \
                                SIGCONT, SIGALRM
except KeyboardInterrupt:
    exit(0)

signal(SIGINT,SIG_IGN)
signal(SIGTSTP,SIG_IGN)
signal(SIGCONT,SIG_IGN)

from sys             import argv, exit, stdin, stdout, stderr
from os              import environ, fork, execvpe, getuid, getpid, path, getcwd, \
                            close, wait, waitpid, kill, unlink, _exit,  \
			    WIFSIGNALED, WEXITSTATUS
from socket          import socket, fromfd, AF_UNIX, SOCK_STREAM, gethostname, \
                            gethostbyname_ex, gethostbyaddr
from select          import select, error
from errno           import EINTR
from exceptions      import Exception
from re              import findall
from urllib          import unquote
from mpdlib          import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                            mpd_get_inet_listen_socket, mpd_get_my_username, \
                            mpd_raise, mpdError, mpd_version, mpd_print, mpd_read_one_line
import xml.dom.minidom

class mpdrunInterrupted(Exception):
    def __init__(self,args=None):
        self.args = args

global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
       try0Locally, lineLabels, jobAlias, hostsFile, mergingOutput, gdb
global stdinGoesToWho, myExitStatus, manSocket, jobid
global outXmlDoc, outXmlEC, outXmlFile


def mpdrun():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, hostsFile, mergingOutput, gdb
    global stdinGoesToWho, myExitStatus, manSocket, jobid
    global outXmlDoc, outXmlEC, outXmlFile

    mpd_set_my_id('mpdrun_' + `getpid()`)
    pgm = ''
    mship = ''
    rship = ''
    nprocs = 0
    jobAlias = ''
    argsFilename = ''
    outExitCodesFilename = ''
    outXmlFile = ''
    outXmlDoc = ''
    outXmlEC = ''
    delArgsFile = 0
    try0Locally = 1
    lineLabels = 0
    stdinGoesToWho = '0'
    mergingOutput = 0
    linesToRanks = {}
    linesOrdered = []
    ranksCached = {}
    gdb = 0
    process_cmdline_args()
    (listenSocket,listenPort) = mpd_get_inet_listen_socket('',0)
    cwd = path.abspath(getcwd())
    username = mpd_get_my_username()
    signal(SIGALRM,sig_handler)
    if environ.has_key('MPDRUN_TIMEOUT'):
        jobTimeout = int(environ['MPDRUN_TIMEOUT'])
    elif environ.has_key('MPIEXEC_TIMEOUT'):
        jobTimeout = int(environ['MPIEXEC_TIMEOUT'])
    else:
        jobTimeout = 0
    if environ.has_key('UNIX_SOCKET'):
        conFD = int(environ['UNIX_SOCKET'])
        conSocket = fromfd(conFD,AF_UNIX,SOCK_STREAM)
        close(conFD)
    else:
        consoleName = '/tmp/mpd2.console_' + username
        conSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
        try:
            conSocket.connect(consoleName)
        except Exception, errmsg:
            print 'cannot connect to local mpd; possible causes:'
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
            exit(-1)
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
	msgToSend = { 'cmd' : 'get_mpd_version' }
	mpd_send_one_msg(conSocket,msgToSend)
	msg = recv_one_msg_with_timeout(conSocket,5)
	if not msg:
	    mpd_raise('no msg recvd from mpd during version check')
	elif msg['cmd'] != 'mpd_version_response':
	    mpd_raise('unexpected msg from mpd :%s:' % (msg) )
	if msg['mpd_version'] != mpd_version:
	    mpd_raise('mpd version %s does not match mine %s' % (msg['mpd_version'],mpd_version) )

    hostList = []
    if argsFilename:
        try:
            argsFile = open(argsFilename,'r')
        except:
            print 'could not open job specification file %s' % (argsFilename)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        args = argsFile.read()
	if delArgsFile:
	    unlink(argsFilename)
        try: 
            from xml.dom.minidom import parseString   #import only if needed
        except:
            print 'need xml parser like xml.dom.minidom'
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        parsedArgs = parseString(args)
        if parsedArgs.documentElement.tagName != 'create-process-group':
            print 'expecting create-process-group; got unrecognized doctype: %s' % \
                  (parsedArgs.documentElement.tagName)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        createReq = parsedArgs.getElementsByTagName('create-process-group')[0]
        if createReq.hasAttribute('totalprocs'):
            nprocs = int(createReq.getAttribute('totalprocs'))
        else:
            print '** totalprocs not specified in %s' % argsFilename
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        if createReq.hasAttribute('dont_try_0_locally'):
	    try0Locally = 0
        if createReq.hasAttribute('output')  and  \
           createReq.getAttribute('output') == 'label':
	    lineLabels = 1
        if createReq.hasAttribute('pgid'):    # our jobalias
            jobAlias = createReq.getAttribute('pgid')
        if createReq.hasAttribute('stdin_goes_to_who'):
            stdinGoesToWho = createReq.getAttribute('stdin_goes_to_who')

        nextHost = 0
        hostSpec = createReq.getElementsByTagName('host-spec')
        if hostSpec:
            for node in hostSpec[0].childNodes:
                node = node.data.strip()
                hostnames = findall(r'\S+',node)
                for hostname in hostnames:
                    if hostname:    # some may be the empty string
                        try:
                            ipaddr = gethostbyname_ex(hostname)[2][0]
                        except:
                            print 'unable to determine IP info for host %s' % (hostname)
                            myExitStatus = -1  # used in main
                            exit(myExitStatus) # really forces jump back into main
                        if ipaddr.startswith('127.0.0'):
                            hostList.append(gethostname())
                        else:
                            hostList.append(ipaddr)
        if hostSpec and hostSpec[0].hasAttribute('check'):
            hostSpecMode = hostSpec[0].getAttribute('check')
            if hostSpecMode == 'yes':
                msgToSend = { 'cmd' : 'verify_hosts_in_ring', 'host_list' : hostList }
                mpd_send_one_msg(conSocket,msgToSend)
                msg = recv_one_msg_with_timeout(conSocket,5)
                if not msg:
                    mpd_raise('no msg recvd from mpd mpd during chk hosts up')
                elif msg['cmd'] != 'verify_hosts_in_ring_response':
                    mpd_raise('unexpected msg from mpd :%s:' % (msg) )
                if msg['host_list']:
                    print 'These hosts are not in the mpd ring:'
                    for host in  msg['host_list']:
                        if host[0].isdigit():
                            print '    %s (%s)' % (gethostbyaddr(host)[0],host)  # ip addr
                        else:
                            print '    %s' % (host)
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main

        execs   = {}
        users   = {}
        cwds    = {}
        paths   = {}
        args    = {}
        envvars = {}
        limits  = {}
        hosts   = {}

        covered = [0] * nprocs 
        procSpec = createReq.getElementsByTagName('process-spec')
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
                (loRange,hiRange) = (0,nprocs-1)
            for i in xrange(loRange,hiRange+1):
                if i >= nprocs:
                    print '*** exiting; rank %d is greater than nprocs for args'
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
                if covered[i]:
                    print '*** exiting; rank %d is doubly used in proc specs'
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
                covered[i] = 1
            if p.hasAttribute('exec'):
                execs[(loRange,hiRange)] = p.getAttribute('exec')
            else:
                print '*** exiting; range %d-%d has no exec' % (loRange,hiRange)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
            if p.hasAttribute('user'):
                tempuser = p.getAttribute('user')
                if tempuser == username  or  getuid() == 0:
                    users[(loRange,hiRange)] = p.getAttribute('user')
                else:
                    print tempuser, 'username does not match yours and you are not root'
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
            else:
                users[(loRange,hiRange)] = username
            if p.hasAttribute('cwd'):
                cwds[(loRange,hiRange)] = p.getAttribute('cwd')
            else:
                cwds[(loRange,hiRange)] = cwd
            if p.hasAttribute('path'):
                paths[(loRange,hiRange)] = p.getAttribute('path')
            else:
                paths[(loRange,hiRange)] = environ['PATH']
            if p.hasAttribute('host'):
                host = p.getAttribute('host')
                if host.startswith('_any_'):
                    hosts[(loRange,hiRange)] = host
                else:
                    hosts[(loRange,hiRange)] = gethostbyname_ex(host)[2][0]
            else:
                if hostList:
                    hosts[(loRange,hiRange)] = '_any_from_pool_'
                else:
                    hosts[(loRange,hiRange)] = '_any_'

            argDict = {}
            argList = p.getElementsByTagName('arg')
            for argElem in argList:
                argDict[int(argElem.getAttribute('idx'))] = argElem.getAttribute('value')
            argVals = [0] * len(argList)
            for i in argDict.keys():
                argVals[i-1] = unquote(argDict[i])
            args[(loRange,hiRange)] = argVals

            limitDict = {}
            limitList = p.getElementsByTagName('limit')
            for limitElem in limitList:
                type = limitElem.getAttribute('type')
                if valid_limit_type(type):
                    limitDict[type] = limitElem.getAttribute('value')
                else:
                    print 'mpdrun: invalid type in limit: %s' % (type)
                    myExitStatus = -1  # used in main
                    exit(myExitStatus) # really forces jump back into main
            limits[(loRange,hiRange)] = limitDict

            envVals = {}
            envVarList = p.getElementsByTagName('env')
            for envVarElem in envVarList:
                envkey = envVarElem.getAttribute('name')
                envval = envVarElem.getAttribute('value')
                envVals[envkey] = envval
            envvars[(loRange,hiRange)] = envVals

        ## exit(-1)    #####  RMB         

    else:
        if not nprocs:
	    print 'you have to indicate how many processes to start'
	    usage()
        execs   = { (0,nprocs-1) : pgm }
        users   = { (0,nprocs-1) : username }
        cwds    = { (0,nprocs-1) : cwd }
        paths   = { (0,nprocs-1) : environ['PATH'] }
        args    = { (0,nprocs-1) : pgmArgs }
        limits  = { (0,nprocs-1) : {} }
        cli_environ = {} ; cli_environ.update(environ)
        envvars = { (0,nprocs-1) : cli_environ }
        if hostsFile:
            hosts = {}
            hostNames = hostsFile.readlines()
            hostNames = [ x.strip() for x in hostNames if x[0] != '#' ]
            hostIdx = 0
            for i in range(nprocs):
                hosts[(i,i)] = hostNames[hostIdx]
                hostIdx += 1
                if hostIdx >= len(hostNames):
                    hostIdx = 0
        else:
            hosts   = { (0,nprocs-1) : '_any_' }

    if mship:
        (mshipSocket,mshipPort) = mpd_get_inet_listen_socket('',0)
        mshipPid = fork()
        if mshipPid == 0:
	    conSocket.close()
            environ['MPDCP_AM_MSHIP'] = '1'
            environ['MPDCP_MSHIP_PORT'] = str(mshipPort)
            environ['MPDCP_MSHIP_FD'] = str(mshipSocket.fileno())
            environ['MPDCP_MSHIP_NPROCS'] = str(nprocs)
            try:
                execvpe(mship,[mship],environ)
            except Exception, errmsg:
                mpd_raise('execvpe failed for copgm %s; errmsg=:%s:' % (mship,errmsg) )
            _exit(0);  # do NOT do cleanup
        mshipSocket.close()
    else:
        mshipPid = 0

    msgToSend = { 'cmd' : 'mpdrun',
                  'conhost'  : gethostname(),
                  'conport'  : listenPort,
                  'spawned'  : 0,
		  'nstarted' : 0,
                  'nprocs'   : nprocs,
		  'hosts'    : hosts,
                  'execs'    : execs,
                  'jobalias' : jobAlias,
                  'users'    : users,
                  'cwds'     : cwds,
                  'paths'    : paths,
                  'args'     : args,
                  'envvars'  : envvars,
                  'limits'   : limits,
                  'host_spec_pool' : hostList
		}
    if try0Locally:
        msgToSend['try_0_locally'] = 1
    if lineLabels:
        msgToSend['line_labels'] = 1
    if rship:
        msgToSend['rship'] = rship
        msgToSend['mship_host'] = gethostname()
        msgToSend['mship_port'] = mshipPort
    msgToSend['stdin_goes_to_who'] = stdinGoesToWho
    mpd_send_one_msg(conSocket,msgToSend)
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd when expecting ack of request')
    elif msg['cmd'] == 'mpdrun_ack':
        currRingSize = msg['ringsize']
    else:
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        elif msg['cmd'] == 'job_failed'  and  msg['reason'] == 'some_procs_not_started':
            print 'mpdrun: unable to start all procs; may have invalid machine names'
            print '    remaining specified hosts:'
            for host in msg['remaining_hosts'].values():
		if host != '_any_':
                    print '        %s' % (host)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    conSocket.close()
    if jobTimeout:
	alarm(jobTimeout)

    (manSocket,addr) = listenSocket.accept()
    msg = mpd_recv_one_msg(manSocket)
    if (not msg  or  not msg.has_key('cmd') or msg['cmd'] != 'man_checking_in'):
        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
    msgToSend = { 'cmd' : 'ringsize', 'ringsize' : currRingSize }
    mpd_send_one_msg(manSocket,msgToSend)
    msg = mpd_recv_one_msg(manSocket)
    if (not msg  or  not msg.has_key('cmd')):
        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
    if (msg['cmd'] == 'job_started'):
        jobid = msg['jobid']
        if outXmlEC:
            outXmlEC.setAttribute('jobid',jobid.strip())
        # print 'mpdrun: job %s started' % (jobid)
    else:
	mpd_raise('mpdrun: from man, unknown msg=:%s:' % (msg) )

    (manCliStdoutSocket,addr) = listenSocket.accept()
    (manCliStderrSocket,addr) = listenSocket.accept()
    socketsToSelect = { manSocket : 1, manCliStdoutSocket : 1, manCliStderrSocket : 1,
                        stdin : 1 }
    signal(SIGINT,sig_handler)
    signal(SIGTSTP,sig_handler)
    signal(SIGCONT,sig_handler)
    if gdb:
        msgToSend = { 'cmd' : 'stdin_from_user' }
        msgToSend['line'] = 'set prompt\n'
        mpd_send_one_msg(manSocket,msgToSend)
        msgToSend['line'] = 'set confirm off\n'
        mpd_send_one_msg(manSocket,msgToSend)
        msgToSend['line'] = 'handle SIGUSR1 nostop noprint\n'
        mpd_send_one_msg(manSocket,msgToSend)
        msgToSend['line'] = 'handle SIGPIPE nostop noprint\n'
        mpd_send_one_msg(manSocket,msgToSend)
        msgToSend['line'] = 'set confirm on\n'
        mpd_send_one_msg(manSocket,msgToSend)
        msgToSend['line'] = 'set prompt (gdb)\\n\n'
        mpd_send_one_msg(manSocket,msgToSend)
        # msgToSend['line'] = 'quit\n'
        # mpd_send_one_msg(manSocket,msgToSend)

    firstGDB = 1
    done = 0
    while done < 3:    # man, client stdout, and client stderr
        try:
            try:
                (readySockets,unused1,unused2) = select(socketsToSelect.keys(),[],[],1)
            except error, data:
                if data[0] == EINTR:    # interrupted by timeout for example
                    continue
                else:
                    mpd_raise('select error: %s' % strerror(data[0]))
            if mergingOutput  and  len(readySockets) == 0:    # timed out
                for line in linesOrdered:
                    linesToRanks[line].sort()
                    fsr = format_sorted_ranks(linesToRanks[line])
                    print '%s: %s' % (fsr,line),
                stdout.flush()
                linesToRanks = {}
                linesOrdered = []
                ranksCached = {}
            for readySocket in readySockets:
                if readySocket == manSocket:
                    msg = mpd_recv_one_msg(manSocket)
                    if not msg:
                        del socketsToSelect[manSocket]
                        # manSocket.close()
                        tempManSocket = manSocket    # keep a ref to it
                        manSocket = 0
                        done += 1
		    elif not msg.has_key('cmd'):
                        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
                    elif msg['cmd'] == 'invalid_executable':
                        # print 'rank %d (%s) in job %s failed to find executable %s' % \
                              # ( msg['rank'], msg['src'], msg['jobid'], msg['exec'] )
                        host = msg['src'].split('_')[0]
                        reason = unquote(msg['reason'])
                        print 'problem with executable  %s  on  %s:  %s ' % \
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
                            outXmlEC.appendChild(outXmlProc)
                            outXmlProc.setAttribute('rank',str(msg['cli_rank']))
                            outXmlProc.setAttribute('status',str(msg['cli_status']))
                            outXmlProc.setAttribute('pid',str(msg['cli_pid']))
                            outXmlProc.setAttribute('host',msg['cli_host'])
                        # print "exit info: rank=%d  host=%s  pid=%d  status=%d" % \
                              # (msg['cli_rank'],msg['cli_host'],
                               # msg['cli_pid'],msg['cli_status'])
			status = msg['cli_status']
			if WIFSIGNALED(status):
			    if status > myExitStatus:
			        myExitStatus = status
			    killed_status = status & 0x007f  # AND off core flag
		            # # print 'exit status of rank %d: killed by signal %d ' % (msg['cli_rank'],killed_status)
			else:
			    exit_status = WEXITSTATUS(status)
			    if exit_status > myExitStatus:
			        myExitStatus = exit_status
		            # # print 'exit status of rank %d: return code %d ' % (msg['cli_rank'],exit_status)
		    else:
		        print 'unrecognized msg from manager :%s:' % msg
                elif readySocket == manCliStdoutSocket:
                    if mergingOutput:
                        if gdb and firstGDB:
                            firstGDB = 0
                            for i in range(nprocs):
                                line = mpd_read_one_line(manCliStdoutSocket.fileno())
                                # print "^%s$" % line
                            print '(gdb) '
                            continue
                        line = mpd_read_one_line(manCliStdoutSocket.fileno())
                        if not line:
                            del socketsToSelect[readySocket]
                            done += 1
                        else:
                            (rank,rest) = line.split(':',1)
                            rank = int(rank)
                            linesToRanks.setdefault(rest,[])
                            linesToRanks[rest].append(rank)
                            ranksCached.setdefault(rank,0)
                            ranksCached[rank] += 1
                            if rest not in linesOrdered:
                                linesOrdered.append(rest)
                            if len(ranksCached.keys()) >= nprocs:
                                # count # of procs that have same # of lines as prev proc
                                n = 0
                                x = ranksCached.values()
                                for i in range(len(x)):
                                    if x[i] == x[i-1]:    # -1 is valid subscript
                                        n += 1
                                # same number of lines for all procs ?
                                if n == len(ranksCached.keys()):
                                    for line in linesOrdered:
                                        linesToRanks[line].sort()
                                        fsr = format_sorted_ranks(linesToRanks[line])
                                        print '%s: %s' % (fsr,line),
                                    stdout.flush()
                                    linesToRanks = {}
                                    linesOrdered = []
                                    ranksCached = {}
                    else:
                        msg = manCliStdoutSocket.recv(1024)
                        if not msg:
                            del socketsToSelect[readySocket]
                            # readySocket.close()
                            done += 1
                        else:
                            stdout.write(msg)
                            stdout.flush()
                elif readySocket == manCliStderrSocket:
                    msg = manCliStderrSocket.recv(1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        # readySocket.close()
                        done += 1
                    else:
                        # print >>stderr, msg,
                        # print >>stderr, 'MS: %s' % (msg.strip())
                        stderr.write(msg)
                        stderr.flush()
                elif readySocket == stdin:
                    line = stdin.readline()
                    if line:    # not EOF
                        msgToSend = { 'cmd' : 'stdin_from_user', 'line' : line } # default
                        if gdb and line.startswith('z '):
                            s1 = line[2:].rstrip().split(',')
                            for s in s1:
                                s2 = s.split('-')
                                for i in s2:
                                    if not i.isdigit():
                                        print 'invalid arg to z :%s:' % i
					continue
                            msgToSend = { 'cmd' : 'stdin_goes_to_who',
                                          'stdin_procs' : line.rstrip()[2:] }
                        if manSocket:
                            mpd_send_one_msg(manSocket,msgToSend)
                    else:
                        del socketsToSelect[stdin]
                        stdin.close()
                else:
                    mpd_raise('unrecognized ready socket :%s:' % (readySocket) )
        except mpdError, errmsg:
            print 'mpdrun failed: %s' % (errmsg)
            myExitStatus = -1  # used in main
            exit(myExitStatus) # really forces jump back into main
        except mpdrunInterrupted, errmsg:
	    if errmsg.args == 'SIGINT':
	        if manSocket:
	            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
	            mpd_send_one_msg(manSocket,msgToSend)
                    # next code because no longer exiting
	            ### del socketsToSelect[manSocket]
	            ### # manSocket.close()
                    ### tempManSocket = manSocket
	            ### manSocket = 0
                    ### done += 1
	        # exit(-1)
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main
	    elif errmsg.args == 'SIGTSTP':
	        if manSocket:
	            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGTSTP' }
	            mpd_send_one_msg(manSocket,msgToSend)
	        signal(SIGTSTP,SIG_DFL)      # stop myself
	        kill(getpid(),SIGTSTP)
	        signal(SIGTSTP,sig_handler)  # restore this handler
	    elif errmsg.args == 'SIGALRM':
                mpd_print(1, 'mpdrun terminating due to timeout %d seconds' % \
                          (jobTimeout))
                if manSocket:
                    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
                    mpd_send_one_msg(manSocket,msgToSend)
                    manSocket.close()
                myExitStatus = -1  # used in main
                exit(myExitStatus) # really forces jump back into main

    if mergingOutput:
        for line in linesOrdered:
            linesToRanks[line].sort()
            fsr = format_sorted_ranks(linesToRanks[line])
            print '%s: %s' % (fsr,line),
        stdout.flush()
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)
    if outXmlFile:
        print >>outXmlFile, outXmlDoc.toprettyxml(indent='   ')
        outXmlFile.close()

def sig_handler(signum,frame):
    # for some reason, I (rmb) was unable to handle TSTP and CONT in the same way
    global manSocket
    if signum == SIGINT:
        raise mpdrunInterrupted, 'SIGINT'
    elif signum == SIGTSTP:
        raise mpdrunInterrupted, 'SIGTSTP'
    elif signum == SIGCONT:
	if manSocket:
	    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGCONT' }
	    mpd_send_one_msg(manSocket,msgToSend)
    elif signum == SIGALRM:
        raise mpdrunInterrupted, 'SIGALRM'

def recv_one_msg_with_timeout(sock,timeout):
    oldTimeout = alarm(timeout)
    msg = mpd_recv_one_msg(sock)    # fails WITHOUT a msg if sigalrm occurs
    alarm(oldTimeout)
    return(msg)

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

def process_cmdline_args():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, try0Locally, \
           lineLabels, jobAlias, stdinGoesToWho, hostsFile, jobid, mergingOutput, gdb
    global outXmlDoc, outXmlEC, outXmlFile

    hostsFile = ''
    if len(argv) < 3:
        usage()
    argidx = 1
    if argv[1] == '-delxmlfile':  # special case for mpiexec
	delArgsFile = 1
        argsFilename = argv[2]   # initialized to '' in main
	argidx = 3
    elif argv[1] == '-f':
        argsFilename = argv[2]   # initialized to '' in main
        argidx += 2
	if len(argv) > 3:
            if len(argv) > 5  or  argv[3] != '-r':
                print '-r is the only arg that can be used with -f'
                usage()
            else:
                outExitCodesFilename = argv[4]   # initialized to '' in main
                outXmlFile = open(outExitCodesFilename,'w')
                outXmlDoc = xml.dom.minidom.Document()
                outXmlEC = outXmlDoc.createElement('exit-codes')
                outXmlDoc.appendChild(outXmlEC)
                argidx += 2
    if not argsFilename:
        while pgm == '':
	    if argidx >= len(argv):
	        usage()
            if argv[argidx][0] == '-':
                if argv[argidx] == '-np' or argv[argidx] == '-n':
                    if not argv[argidx+1].isdigit():
	                print 'non-numeric arg to -n or -np'
                        usage()
                    else:
                        nprocs = int(argv[argidx+1])
                        if nprocs < 1:
                            usage()
                        else:
                            argidx += 2
                elif argv[argidx] == '-f':
	            print '-f must be first and only -r can appear with it'
		    usage()
                elif argv[argidx] == '-r':
                    outExitCodesFilename = argv[argidx+1]   # initialized to '' in main
                    outXmlFile = open(outExitCodesFilename,'w')
                    outXmlDoc = xml.dom.minidom.Document()
                    outXmlEC = outXmlDoc.createElement('exit-codes')
                    outXmlDoc.appendChild(outXmlEC)
                    argidx += 2
                elif argv[argidx] == '-a':
                    jobAlias = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-hf':
                    hostsFilename = argv[argidx+1]
                    argidx += 2
                    try:
                        hostsFile = open(hostsFilename,'r')
                    except:
                        print 'unable to open hosts file: %s' % (hostsFilename)
                        myExitStatus = -1  # used in main
                        exit(myExitStatus) # really forces jump back into main
                elif argv[argidx] == '-cpm':
                    mship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-cpr':
                    rship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-l':
                    lineLabels = 1
                    argidx += 1
                elif argv[argidx] == '-m':
                    mergingOutput = 1
                    lineLabels = 1  # implied
                    argidx += 1
                elif argv[argidx] == '-g':
                    gdb = 1
                    mergingOutput = 1   # implied
                    lineLabels = 1      # implied
                    stdinGoesToWho = 'all' # implied  (all processes at first)
                    argidx += 1
                elif argv[argidx] == '-1' or argv[argidx] == '-nolocal':
                    try0Locally = 0
                    argidx += 1
                elif argv[argidx] == '-s':
                    stdinGoesToWho = 'all'   # -1 -> all processes
                    argidx += 1
                else:
                    usage()
            else:
                pgm = argv[argidx]
                argidx += 1
    if stdinGoesToWho == 'all':
        stdinGoesToWho = '0-%d' % (nprocs-1)
    pgmArgs = []
    while argidx < len(argv):
        pgmArgs.append(argv[argidx])
        argidx += 1

def valid_limit_type(type):
    if type == 'core'   or  type == 'cpu'    or  type == 'fsize'  or  type == 'data'   \
    or type == 'stack'  or  type == 'rss'    or  type == 'nproc'  or  type == 'nofile' \
    or type == 'ofile'  or  type == 'memloc' or  type == 'as':
        return 1
    return 0

def usage():
    global myExitStatus
    print 'mpdrun for mpd version: %s' % str(mpd_version)
    print 'usage: mpdrun [args] pgm_to_execute [pgm_args]'
    print '   where args may be: -a alias -np nprocs -hf hostsfile -cpm master_copgm -cpr remote_copgm -l -m -1 -s'
    print '       (nprocs must be a positive integer)'
    print '       (-hf is a hostsfile containing names of nodes on which to run)'
    print '       (-l means attach line labels identifying which client prints each line)'
    print '       (-m means merge identical outputs into a single line;'
    print '           implies that program produces whole lines;'
    print '           implies -l)'
    print '       (-1 means do NOT start the first process locally)'
    print '       (-a means assign this alias to the job)'
    print '       (-s means send stdin to all processes; not just first)'
    print '       (-g means assume user will be running gdb and send some initial setup;'
    print '           implies -m and -l and initially -s );'
    print 'or:    mpdrun -f input_xml_filename [-r output_xml_exit_codes_filename]'
    print '   where filename contains all the arguments in xml format'
    myExitStatus = -1  # used in main
    exit(myExitStatus) # really forces jump back into main


if __name__ == '__main__':

    global manSocket, myExitStatus

    manSocket = 0    # set when we get conn'd to a manager
    myExitStatus = 0

    try:
        mpdrun()
    except mpdError, errmsg:
	print 'mpdrun failed: %s' % (errmsg)
    except SystemExit, errmsg:
        pass
    exit(myExitStatus)
