#!/usr/bin/env python

from sys             import argv, exit, stdin, stdout, stderr
from os              import environ, fork, execvpe, getuid, getpid, path, getcwd, \
                            close, wait, waitpid, kill, _exit,  \
			    WIFSIGNALED, WEXITSTATUS
from socket          import socket, fromfd, AF_UNIX, SOCK_STREAM, gethostname
from select          import select
from signal          import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from exceptions      import Exception
from xml.dom.minidom import parseString
from re              import findall
from mpdlib          import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                            mpd_get_inet_listen_socket, mpd_get_my_username, \
                            mpd_raise, mpdError, mpd_version

class mpdrunInterrupted(Exception):
    def __init__(self,args=None):
        self.args = args

global nprocs, pgm, pgmArgs, mship, rship, argsFilename, try0Locally, lineLabels
global manSocket, timeout, sigExitDueToTimeout, stdinGoesToWho


def mpdrun():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, try0Locally, lineLabels, jobalias
    global manSocket, timeout, sigExitDueToTimeout, stdinGoesToWho

    mpd_set_my_id('mpdrun_' + `getpid()`)
    pgm = ''
    mship = ''
    rship = ''
    nprocs = 0
    jobalias = ''
    argsFilename = ''
    try0Locally = 1
    lineLabels = 0
    stdinGoesToWho = 0
    process_cmdline_args()
    sigExitDueToTimeout = 1
    (listenSocket,listenPort) = mpd_get_inet_listen_socket('',0)
    cwd = path.abspath(getcwd())
    username = mpd_get_my_username()
    if environ.has_key('MPDRUN_TIMEOUT'):
        timeout = int(environ['MPDRUN_TIMEOUT'])
    elif environ.has_key('MPIEXEC_TIMEOUT'):
        timeout = int(environ['MPIEXEC_TIMEOUT'])
    else:
        timeout = 0
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

    if argsFilename:
        argsFile = open(argsFilename,'r')
        args = argsFile.read()
        parsedArgs = parseString(args)
        if parsedArgs.doctype.name != 'PMRequests':
            print 'expecting PMRequests; got unrecognized doctype %s' 
            exit(-1)
        createReq = parsedArgs.getElementsByTagName('create-process-group')[0]
        if createReq.hasAttribute('totalprocs'):
            nprocs = int(createReq.getAttribute('totalprocs'))
        else:
            print '** totalprocs not specified in %s' % argsFilename
            exit(-1)
        if createReq.hasAttribute('line_labels'):
	    lineLabels = 1
        if createReq.hasAttribute('jobalias'):
            jobalias = createReq.getAttribute('jobalias')
        if createReq.hasAttribute('stdin_goes_to_all'):
            stdinGoesToWho = int(createReq.getAttribute('stdin_goes_to_all'))

	hosts  = extract_from_xml(createReq,'host','name','_any_')
	execs  = extract_from_xml(createReq,'exec','name','')
	paths  = extract_from_xml(createReq,'path','name','')
	users  = extract_from_xml(createReq,'user','name',username)
	cwds   = extract_from_xml(createReq,'cwd','name',cwd)

	# handle cmd-line args
        covered = [0] * nprocs
        args = {}
        if createReq.hasAttribute('args'):
            defaultArgs = createReq.getAttribute('args')
	else:
            defaultArgs = ''
	defaultArgs = findall(r'\S+',defaultArgs)
        argsElements = createReq.getElementsByTagName('args')
        for elem in argsElements:
            ranks = elem.getAttribute('range').split('-')
            if len(ranks) == 1:
                ranks = (ranks[0],ranks[0])
            ranks = tuple(map(int,ranks))
	    for i in range(ranks[0],ranks[1]+1):
	        if i >= nprocs:
		    print '*** exiting; rank %d is greater than nprocs for args' % i
		    exit(-1)
	        if covered[i]:
		    print '*** exiting; rank %d is multiply covered for args' % (i)
		    exit(-1)
	        covered[i] = 1
            argVals = []
            argList = elem.getElementsByTagName('arg')
            for argElem in argList:
                arg = argElem.getAttribute('value')
                argVals.append(arg)
            args[ranks] = argVals
        i = 0
        while i < len(covered):
	    if not covered[i]:
	        s = i
	        while i < len(covered)  and  not covered[i]:
		    i += 1
	        args[(s,i-1)] = defaultArgs
	    else:
	        i += 1

	# handle env vars
        covered = [0] * nprocs
        envvars = {}
        if createReq.hasAttribute('envvars'):
            defaultEnvVars = createReq.getAttribute('envvars')
	else:
            defaultEnvVars = ''
        envVarsElements = createReq.getElementsByTagName('envvars')
        for elem in envVarsElements:
            ranks = elem.getAttribute('range').split('-')
            if len(ranks) == 1:
                ranks = (ranks[0],ranks[0])
            ranks = tuple(map(int,ranks))
	    for i in range(ranks[0],ranks[1]+1):
	        if i >= nprocs:
		    print '*** exiting; rank %d is greater than nprocs for envvars' % i
		    exit(-1)
	        if covered[i]:
		    print '*** exiting; rank %d is multiply covered for envvars' % (i)
		    exit(-1)
	        covered[i] = 1
            evnVarStr = ''
            evnVarList = elem.getElementsByTagName('envvar')
            for evnVarElem in evnVarList:
                envkey = evnVarElem.getAttribute('key')
                envval = evnVarElem.getAttribute('value')
                evnVarStr = evnVarStr + ' ' + envkey + '=' + envval
            envvars[ranks] = evnVarStr
        i = 0
        while i < len(covered):
	    if not covered[i]:
	        s = i
	        while i < len(covered)  and  not covered[i]:
		    i += 1
	        envvars[(s,i-1)] = defaultEnvVars
	    else:
	        i += 1

        if createReq.getElementsByTagName('dont_try_0_locally'):
	    try0Locally = 0
    else:
        if not nprocs:
	    print 'you have to indicate how many processes to start'
	    usage()
        hosts   = { (0,nprocs-1) : '_any_' }
        execs   = { (0,nprocs-1) : pgm }
        users   = { (0,nprocs-1) : username }
        cwds    = { (0,nprocs-1) : cwd }
        paths   = { (0,nprocs-1) : environ['PATH'] }
        args    = { (0,nprocs-1) : pgmArgs }
        envvars = { (0,nprocs-1) : '' }

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
                  'jobalias' : jobalias,
                  'users'    : users,
                  'cwds'     : cwds,
                  'paths'    : paths,
                  'args'     : args,
                  'envvars'  : envvars
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
            mpd_raise('mpd already has a console (e.g. for long ringtest); try later')
        elif msg['cmd'] == 'job_failed'  and  msg['reason'] == 'some_procs_not_started':
            mpd_raise('unable to start all procs; may have invalid machine names')
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    conSocket.close()
    if timeout:
        signal(SIGALRM,sig_handler)
	alarm(timeout)

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
    done = 0
    while done < 3:    # man, client stdout, and client stderr
        try:
            (readySockets,None,None) = select(socketsToSelect.keys(),[],[],30)
            for readySocket in readySockets:
                if readySocket == manSocket:
                    msg = mpd_recv_one_msg(manSocket)
                    if not msg:
                        # mpd_raise('mpdrun: empty msg from man; it must have terminated early')
                        print 'mpdrun: empty msg from man; it must have terminated early'
                        del socketsToSelect[readySocket]
                        readySocket.close()
                        done += 1
		    elif not msg.has_key('cmd'):
                        mpd_raise('mpdrun: from man, invalid msg=:%s:' % (msg) )
                    elif msg['cmd'] == 'job_terminated_early':
                        print 'rank %d in job %s terminated without calling MPI_Finalize' %  ( msg['rank'], msg['jobid'] )
                        # print 'mpdrun: job %s terminated early at rank %d' % (msg['jobid'], msg['rank'])
                        # del socketsToSelect[readySocket]
                        # readySocket.close()
                        # done += 1
                    elif (msg['cmd'] == 'job_terminated'):
                        del socketsToSelect[readySocket]
                        readySocket.close()
                        done += 1
                    elif (msg['cmd'] == 'client_exit_status'):
			status = msg['status']
			if WIFSIGNALED(status):
			    killed_status = status & 0x007f  # AND off core flag
		            # print 'exit status of rank %d: killed by signal %d ' % (msg['rank'],killed_status)
			else:
			    exit_status = WEXITSTATUS(status)
		            # print 'exit status of rank %d: return code %d ' % (msg['rank'],exit_status)
		    else:
		        print 'unrecognized msg from manager :%s:' % msg
                elif readySocket == manCliStdoutSocket:
                    msg = manCliStdoutSocket.recv(1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        readySocket.close()
                        done += 1
                    else:
                        print msg,
                        # print 'MS: %s' % (msg.strip())
                        stdout.flush()
                elif readySocket == manCliStderrSocket:
                    msg = manCliStderrSocket.recv(1024)
                    if not msg:
                        del socketsToSelect[readySocket]
                        readySocket.close()
                        done += 1
                    else:
                        print >>stderr, msg,
                        # print >>stderr, 'MS: %s' % (msg.strip())
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
	            manSocket.close()
	        exit(-1)
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
	    elif sigExitDueToTimeout:
	        exit(-1)
            else:
                mpd_raise('mpdrun: select failed: errmsg=:%s:' % (errmsg) )
    if mshipPid:
        (donePid,status) = wait()    # waitpid(mshipPid,0)

def sig_handler(signum,frame):
    # for some reason, I (rmb) was unable to handle TSTP and CONT in the same way
    global manSocket, timeout, sigExitDueToTimeout
    if signum == SIGINT:
        raise mpdrunInterrupted, 'SIGINT'
    elif signum == SIGTSTP:
        raise mpdrunInterrupted, 'SIGTSTP'
    elif signum == SIGCONT:
	if manSocket:
	    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGCONT' }
	    mpd_send_one_msg(manSocket,msgToSend)
    elif signum == SIGALRM:
	if manSocket:
	    msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGINT' }
	    mpd_send_one_msg(manSocket,msgToSend)
	    manSocket.close()
	print 'mpdrun terminating due to timeout %d seconds' % timeout
	sigExitDueToTimeout = 1
	exit(-1)

def process_cmdline_args():
    global nprocs, pgm, pgmArgs, mship, rship, argsFilename, try0Locally, \
           lineLabels, jobalias, stdinGoesToWho

    if len(argv) < 3:
        usage()
    if argv[1] == '-f':
        argsFilename = argv[2]   # initialized to '' in main
	argidx = 3
    else:
        argidx = 1
    if not argsFilename:
        while pgm == '':
	    if argidx >= len(argv):
	        usage()
            if argv[argidx][0] == '-':
	        if argsFilename:
	            print 'Cannot use other args with -f'
		    usage()
                if argv[argidx] == '-np' or argv[argidx] == '-n':
                    nprocs = int(argv[argidx+1])
                    argidx += 2
                elif argv[argidx] == '-f':
                    argsFilename = argv[argidx+1]
                    argidx += 2
		    np_or_filename += 1
                elif argv[argidx] == '-a':
                    jobalias = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-cpm':
                    mship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-cpr':
                    rship = argv[argidx+1]
                    argidx += 2
                elif argv[argidx] == '-l':
                    lineLabels = 1
                    argidx += 1
                elif argv[argidx] == '-1':
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

def extract_from_xml(createReq,attr,name,defaultVal):
    global nprocs
    if createReq.hasAttribute(attr):
	defaultVal = createReq.getAttribute(attr)
    attrList = createReq.getElementsByTagName(attr)
    covered = [0] * nprocs
    attrs = {}
    for a in attrList:
	ranks = a.getAttribute('range').split('-')
	if len(ranks) == 1:
	    ranks = (ranks[0],ranks[0])
	ranks = tuple(map(int,ranks))
	for i in range(ranks[0],ranks[1]+1):
	    if i >= nprocs:
		print '*** exiting; rank %d is greater than nprocs' % i
		exit(-1)
	    if covered[i]:
		print '*** exiting; rank %d is multiply covered for %s' % (i,attr)
		exit(-1)
	    covered[i] = 1
	attrs[ranks] = a.getAttribute(name)
    i = 0
    while i < len(covered):
	if not covered[i]:
	    s = i
	    while i < len(covered)  and  not covered[i]:
		i += 1
	    attrs[(s,i-1)] = defaultVal
	else:
	    i += 1
    return attrs
                    
def usage():
    print 'mpdrun for mpd version: %s' % str(mpd_version)
    print 'usage: mpdrun [args] pgm_to_execute [pgm_args]'
    print '   where args may be: -a alias -np nprocs -cpm master_copgm -cpr remote_copgm -l -1 -s'
    print '       (-l means attach line labels identifying which client prints each line)'
    print '       (-1 means do NOT start the first process locally)'
    print '       (-a means assign this alias to the job)'
    print '       (-s means send stdin to all processes; not just first)'
    print 'or:    mpdrun -f filename'
    print '   where filename contains all the arguments in xml format'
    exit(-1)


if __name__ == '__main__':

    global manSocket

    manSocket = 0    # set when we get conn'd to a manager
    signal(SIGINT,sig_handler)
    signal(SIGTSTP,sig_handler)
    signal(SIGCONT,sig_handler)

    try:
        mpdrun()
    except mpdError, errmsg:
	print 'mpdrun failed: %s' % (errmsg)
    except SystemExit, errmsg:
        pass
