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
                            gethostbyname_ex
from select          import select
from exceptions      import Exception
from re              import findall
from urllib          import unquote
from mpdlib          import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                            mpd_get_inet_listen_socket, mpd_get_my_username, \
                            mpd_raise, mpdError, mpd_version, mpd_print

class mpdrunInterrupted(Exception):
    def __init__(self,args=None):
        self.args = args

global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
       try0Locally, lineLabels, jobAlias, hostsFile
global timeoutVal, stdinGoesToWho, myExitStatus, manSocket


def mpdrun():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, hostsFile
    global timeoutVal, stdinGoesToWho, myExitStatus, manSocket

    mpd_set_my_id('mpdrun_' + `getpid()`)
    pgm = ''
    mship = ''
    rship = ''
    nprocs = 0
    jobAlias = ''
    argsFilename = ''
    delArgsFile = 0
    try0Locally = 1
    lineLabels = 0
    stdinGoesToWho = 0
    process_cmdline_args()
    (listenSocket,listenPort) = mpd_get_inet_listen_socket('',0)
    cwd = path.abspath(getcwd())
    username = mpd_get_my_username()
    if environ.has_key('MPDRUN_TIMEOUT'):
        timeoutVal = int(environ['MPDRUN_TIMEOUT'])
    elif environ.has_key('MPIEXEC_TIMEOUT'):
        timeoutVal = int(environ['MPIEXEC_TIMEOUT'])
    else:
        timeoutVal = 0
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
            mpd_raise('cannot connect to local mpd')
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
	msgToSend = { 'cmd' : 'get_mpd_version' }
	mpd_send_one_msg(conSocket,msgToSend)
	msg = mpd_recv_one_msg(conSocket)
	if not msg:
	    mpd_raise('mpd unexpectedly closed connection')
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
            exit(-1)
        args = argsFile.read()
	if delArgsFile:
	    unlink(argsFilename)
        try: 
            from xml.dom.minidom import parseString   #import only if needed
        except:
            print 'need xml parser like xml.dom.minidom'
            exit(-1)
        parsedArgs = parseString(args)
        if parsedArgs.documentElement.tagName != 'create-process-group':
            print 'expecting create-process-group; got unrecognized doctype: %s' % \
                  (parsedArgs.documentElement.tagName)
            exit(-1)
        createReq = parsedArgs.getElementsByTagName('create-process-group')[0]
        if createReq.hasAttribute('totalprocs'):
            nprocs = int(createReq.getAttribute('totalprocs'))
        else:
            print '** totalprocs not specified in %s' % argsFilename
            exit(-1)
        if createReq.hasAttribute('dont_try_0_locally'):
	    try0Locally = 0
        if createReq.hasAttribute('output')  and  \
           createReq.getAttribute('output') == 'label':
	    lineLabels = 1
        if createReq.hasAttribute('pgid'):    # our jobalias
            jobAlias = createReq.getAttribute('pgid')
        if createReq.hasAttribute('stdin_goes_to_all'):
            stdinGoesToWho = int(createReq.getAttribute('stdin_goes_to_all'))

        nextHost = 0
        hostSpec = createReq.getElementsByTagName('host-spec')
        if hostSpec:
            for hostname in hostSpec[0].childNodes:
                hostname = hostname.data.strip()
                if hostname:
                    try:
                        ipaddr = gethostbyname_ex(hostname)[2][0]
                    except:
                        print 'unable to determine IP info for host %s' % (hostname)
                        exit(-1)
                    if ipaddr.startswith('127.0.0'):
                        hostList.append(gethostname())
                    else:
                        hostList.append(ipaddr)

        execs   = {}
        users   = {}
        cwds    = {}
        paths   = {}
        args    = {}
        envvars = {}
        hosts   = {}

        covered = [0] * nprocs 
        procSpec = createReq.getElementsByTagName('process-spec')
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
                    exit(-1)
                if covered[i]:
                    print '*** exiting; rank %d is doubly used in proc specs'
                    exit(-1)
                covered[i] = 1
            if p.hasAttribute('exec'):
                execs[(loRange,hiRange)] = p.getAttribute('exec')
            else:
                print '*** exiting; range %d-%d has no exec' % (loRange,hiRange)
                exit(-1)
            if p.hasAttribute('user'):
                users[(loRange,hiRange)] = p.getAttribute('user')
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
                hosts[(loRange,hiRange)] = p.getAttribute('host')
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

            envVals = {}
            envVarList = p.getElementsByTagName('env')
            for envVarElem in envVarList:
                envkey = envVarElem.getAttribute('name')
                envval = envVarElem.getAttribute('value')
                envVals[envkey] = envval
            envvars[(loRange,hiRange)] = envVals

        ## exit(-1)    #####  RMB TEMP

    else:
        if not nprocs:
	    print 'you have to indicate how many processes to start'
	    usage()
        execs   = { (0,nprocs-1) : pgm }
        users   = { (0,nprocs-1) : username }
        cwds    = { (0,nprocs-1) : cwd }
        paths   = { (0,nprocs-1) : environ['PATH'] }
        args    = { (0,nprocs-1) : pgmArgs }
        envvars = { (0,nprocs-1) : {} }
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
    msgToSend['stdin_goes_to_who'] = str(stdinGoesToWho)
    mpd_send_one_msg(conSocket,msgToSend)
    msg = mpd_recv_one_msg(conSocket)
    if not msg:
        mpd_raise('mpd unexpectedly closed connection')
    elif msg['cmd'] != 'mpdrun_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            exit(-1)
        elif msg['cmd'] == 'job_failed'  and  msg['reason'] == 'some_procs_not_started':
            print 'mpdrun: unable to start all procs; may have invalid machine names'
            print '    remaining specified hosts:'
            for host in msg['remaining_hosts'].values():
		if host != '_any_':
                    print '        %s' % (host)
            exit(-1)
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    conSocket.close()
    if timeoutVal:
        signal(SIGALRM,sig_handler)
	alarm(timeoutVal)

    (manSocket,addr) = listenSocket.accept()
    msg = mpd_recv_one_msg(manSocket)
    if (not msg  or  not msg.has_key('cmd')):
        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
    if (msg['cmd'] == 'job_started'):
        # print 'mpdrun: job %s started' % (msg['jobid'])
        pass
    else:
	mpd_raise('mpdrun: from man, unknown msg=:%s:' % (msg) )

    (manCliStdoutSocket,addr) = listenSocket.accept()
    (manCliStderrSocket,addr) = listenSocket.accept()
    socketsToSelect = { manSocket : 1, manCliStdoutSocket : 1, manCliStderrSocket : 1,
                        stdin : 1 }
    signal(SIGINT,sig_handler)
    signal(SIGTSTP,sig_handler)
    signal(SIGCONT,sig_handler)
    done = 0
    while done < 3:    # man, client stdout, and client stderr
        try:
            (readySockets,unused1,unused2) = select(socketsToSelect.keys(),[],[],10)
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
                        print 'Invalid executable %s on %s' % (msg['exec'],host)
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
			status = msg['status']
			if WIFSIGNALED(status):
			    if status > myExitStatus:
			        myExitStatus = status
			    killed_status = status & 0x007f  # AND off core flag
		            # print 'exit status of rank %d: killed by signal %d ' % (msg['rank'],killed_status)
			else:
			    exit_status = WEXITSTATUS(status)
			    if exit_status > myExitStatus:
			        myExitStatus = exit_status
		            # print 'exit status of rank %d: return code %d ' % (msg['rank'],exit_status)
		    else:
		        print 'unrecognized msg from manager :%s:' % msg
                elif readySocket == manCliStdoutSocket:
                    msg = manCliStdoutSocket.recv(1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        # readySocket.close()
                        done += 1
                    else:
                        # print msg,
                        # print 'MS: %s' % (msg.strip())
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
                    lineToSend = stdin.readline()
                    if lineToSend:    # EOF
                        if manSocket:
                            msgToSend = { 'cmd' : 'stdin_from_user', 'line' : lineToSend }
                            mpd_send_one_msg(manSocket,msgToSend)
                    else:
                        del socketsToSelect[stdin]
                        stdin.close()
                else:
                    mpd_raise('unrecognized ready socket :%s:' % (readySocket) )
        except mpdError, errmsg:
            print 'mpdrun failed: %s' % (errmsg)
	    exit(-1)
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
	    elif errmsg.args == 'SIGTSTP':
	        if manSocket:
	            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGTSTP' }
	            mpd_send_one_msg(manSocket,msgToSend)
	        signal(SIGTSTP,SIG_DFL)      # stop myself
	        kill(getpid(),SIGTSTP)
	        signal(SIGTSTP,sig_handler)  # restore this handler
        except Exception, errmsg:
            if isinstance(errmsg,Exception)  and  errmsg[0] == 4:  # interrupted system call
                continue
            else:
                mpd_raise('mpdrun: select failed: errmsg=:%s:' % (errmsg) )
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)

def sig_handler(signum,frame):
    # for some reason, I (rmb) was unable to handle TSTP and CONT in the same way
    global timeoutVal, manSocket
    if signum == SIGINT:
        raise mpdrunInterrupted, 'SIGINT'
    elif signum == SIGTSTP:
        raise mpdrunInterrupted, 'SIGTSTP'
    elif signum == SIGCONT:
	if manSocket:
	    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGCONT' }
	    mpd_send_one_msg(manSocket,msgToSend)
    elif signum == SIGALRM:
        mpd_print(1, 'mpdrun telling client to terminate due to timeout %d seconds %s' % \
                  (timeoutVal,manSocket))
        if manSocket:
            msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
            mpd_send_one_msg(manSocket,msgToSend)

def process_cmdline_args():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, delArgsFile, \
           try0Locally, lineLabels, jobAlias, stdinGoesToWho, hostsFile

    hostsFile = ''
    if len(argv) < 3:
        usage()
    argidx = 1
    if argv[1] == '-delxmlfile':  # special case for mpiexec
	delArgsFile = 1
        argsFilename = argv[2]   # initialized to '' in main
	argidx = 3
    elif argv[1] == '-f':
	if len(argv) > 3:
            print 'cannot use -f with other args'
	    usage()
        argsFilename = argv[2]   # initialized to '' in main
        argidx = 3
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
	            print 'cannot use -f with other args'
		    usage()
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
                        exit(-1)
                elif argv[argidx] == '-cpm':
                    mship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-cpr':
                    rship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-l':
                    lineLabels = 1
                    argidx += 1
                elif argv[argidx] == '-1' or argv[argidx] == '-nolocal':
                    try0Locally = 0
                    argidx += 1
                elif argv[argidx] == '-s':
                    stdinGoesToWho = 1   # 1 -> all processes
                    argidx += 1
                else:
                    usage()
            else:
                pgm = argv[argidx]
                argidx += 1
    pgmArgs = []
    while argidx < len(argv):
        pgmArgs.append(argv[argidx])
        argidx += 1

def usage():
    print 'mpdrun for mpd version: %s' % str(mpd_version)
    print 'usage: mpdrun [args] pgm_to_execute [pgm_args]'
    print '   where args may be: -a alias -np nprocs -hf hostsfile -cpm master_copgm -cpr remote_copgm -l -1 -s'
    print '       (nprocs must be a positive integer)'
    print '       (-hf is a hostsfile containing names of nodes on which to run)'
    print '       (-l means attach line labels identifying which client prints each line)'
    print '       (-1 means do NOT start the first process locally)'
    print '       (-a means assign this alias to the job)'
    print '       (-s means send stdin to all processes; not just first)'
    print 'or:    mpdrun -f filename'
    print '   where filename contains all the arguments in xml format'
    exit(-1)


if __name__ == '__main__':

    global manSocket, mmyExitStatus

    manSocket = 0    # set when we get conn'd to a manager
    myExitStatus = 0

    try:
        mpdrun()
    except mpdError, errmsg:
	print 'mpdrun failed: %s' % (errmsg)
    except SystemExit, errmsg:
        pass
    exit(myExitStatus)
