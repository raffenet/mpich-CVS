#!/usr/bin/env python

from os     import environ, getpid, pipe, fork, fdopen, read, write, close, dup2, \
                   chdir, execvpe, kill, waitpid, _exit
from sys    import exit
from socket import gethostname, fromfd, AF_INET, SOCK_STREAM
from select import select, error
from re     import findall, sub
from signal import signal, SIGKILL, SIGUSR1, SIGTSTP, SIGCONT, SIGCHLD, SIG_DFL, SIG_IGN
from md5    import new
from mpdlib import mpd_set_my_id, mpd_print, mpd_print_tb, mpd_get_ranks_in_binary_tree, \
                   mpd_send_one_line, mpd_recv_one_line, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_inet_listen_socket, mpd_get_inet_socket_and_connect, \
                   mpd_get_my_username, mpd_raise, mpdError, mpd_version


global get_sigtype_from_mpd

def mpdman():
    global get_sigtype_from_mpd
    get_sigtype_from_mpd = 0
    signal(SIGUSR1,sigusr1_handler)
    signal(SIGCHLD,SIG_DFL)  # reset mpd's values

    myHost = environ['MPDMAN_MYHOST']
    myRank = int(environ['MPDMAN_RANK'])
    myId = myHost + '_mpdman_' + str(myRank)
    spawned = int(environ['MPDMAN_SPAWNED'])
    if spawned:
        myId = myId + '_s'
    mpd_set_my_id(myId)
    try:
        chdir(environ['MPDMAN_CWD'])
    except Exception, errmsg:
        errmsg =  '%s: invalid dir: %s' % (myId,environ['MPDMAN_CWD'])
        # print errmsg    ## may syslog it in some cases ?
    clientPgm = environ['MPDMAN_CLI_PGM']
    clientPgmArgs = environ['MPDMAN_PGM_ARGS']
    clientPgmArgs = findall('\S+',clientPgmArgs)
    mpd_print(0000, "ARGS=", clientPgmArgs )
    clientPgmEnv = environ['MPDMAN_PGM_ENVVARS']
    clientPgmEnv = findall('\S+',clientPgmEnv)
    mpd_print(0000, 'entering mpdman to exec %s' % (clientPgm) )
    jobid = environ['MPDMAN_JOBID']
    nprocs = int(environ['MPDMAN_NPROCS'])
    mpdPort = int(environ['MPDMAN_MPD_LISTEN_PORT'])
    mpdConfPasswd = environ['MPDMAN_MPD_CONF_PASSWD']
    environ['MPDMAN_MPD_CONF_PASSWD'] = ''  ## do NOT pass it on to clients
    conHost = environ['MPDMAN_CONHOST']
    conPort = int(environ['MPDMAN_CONPORT'])
    lhsHost = environ['MPDMAN_LHSHOST']
    lhsPort = int(environ['MPDMAN_LHSPORT'])
    host0 = environ['MPDMAN_HOST0']        # only used by right-most man
    port0 = int(environ['MPDMAN_PORT0'])   # only used by right-most man
    myPort = int(environ['MPDMAN_MY_LISTEN_PORT'])
    listenFD = int(environ['MPDMAN_MY_LISTEN_FD'])
    mpd_print(0000, "lhost=%s lport=%d h0=%s p0=%d" % (lhsHost,lhsPort,host0,port0) )
    listenSocket = fromfd(listenFD,AF_INET,SOCK_STREAM)
    close(listenFD)
    socketsToSelect = { listenSocket : 1 }
    lineLabels = int(environ['MPDMAN_LINE_LABELS'])
    startLineLabel = 1
    myLineLabel = str(myRank) + ': '

    # set up pmi stuff early in case I was spawned
    kvsname_template = 'kvs_' + host0 + '_' + str(port0) + '_'
    default_kvsname = kvsname_template + '0'
    default_kvsname = sub('\.','_',default_kvsname)  # chg magpie.cs to magpie_cs
    exec('%s = {}' % (default_kvsname) )
    kvs_next_id = 1
    pmiCollectiveJob = 0

    if nprocs == 1:  # one-man ring
        lhsSocket = mpd_get_inet_socket_and_connect(host0,port0)  # to myself
        (rhsSocket,rhsAddr) = listenSocket.accept()
    else:
        if myRank == 0:
            for i in range(2):    # accept lhs and rhs
                (tempSocket,tempAddr) = listenSocket.accept()
                msg = mpd_recv_one_msg(tempSocket)
                if msg['cmd'] == 'i_am_lhs':
                    (lhsSocket,lhsAddr) = (tempSocket,tempAddr)
                else:
                    (rhsSocket,rhsAddr) = (tempSocket,tempAddr)
        else:
            lhsSocket = mpd_get_inet_socket_and_connect(lhsHost,lhsPort)
            mpd_send_one_msg(lhsSocket, { 'cmd' : 'i_am_rhs' } )
            if myRank == (nprocs-1):              # right-most man
                rhsSocket = mpd_get_inet_socket_and_connect(host0,port0)
                mpd_send_one_msg(rhsSocket, { 'cmd' : 'i_am_lhs' } )
            else:
                (rhsSocket,rhsAddr) = listenSocket.accept()
                msg = mpd_recv_one_msg(rhsSocket)  # drain out the i_am_... msg
    socketsToSelect[lhsSocket] = 1
    socketsToSelect[rhsSocket] = 1

    if myRank == 0:
        conSocket = mpd_get_inet_socket_and_connect(conHost,conPort)  # for cntl msgs
        socketsToSelect[conSocket] = 1
        if spawned:
            msgToSend = { 'cmd' : 'spawned_child' }
            mpd_send_one_msg(conSocket,msgToSend)
        stdoutToConSocket = mpd_get_inet_socket_and_connect(conHost,conPort)
        if spawned:
            msgToSend = { 'cmd' : 'child_in_stdout_tree', 'from_rank' : myRank }
            mpd_send_one_msg(stdoutToConSocket,msgToSend)
        stderrToConSocket = mpd_get_inet_socket_and_connect(conHost,conPort)
        if spawned:
            msgToSend = { 'cmd' : 'child_in_stderr_tree', 'from_rank' : myRank }
            mpd_send_one_msg(stderrToConSocket,msgToSend)
    else:
        conSocket = 0

    (clientListenSocket,clientListenPort) = mpd_get_inet_listen_socket('',0)
    (pipe_read_cli_stdout,pipe_write_cli_stdout) = pipe()
    (pipe_read_cli_stderr,pipe_write_cli_stderr) = pipe()
    (pipe_cli_end,pipe_man_end) = pipe()
    clientPid = fork()
    if clientPid == 0:
        mpd_set_my_id(gethostname() + '_man_before_exec_client_' + `getpid()`)
        lhsSocket.close()
        rhsSocket.close()
        listenSocket.close()
        if conSocket:
            conSocket.close()

        # to simply print on the mpd's tty:
        #     comment out the next lines
        close(pipe_read_cli_stdout)
        dup2(pipe_write_cli_stdout,1)  # closes fd 1 (stdout) if open
        close(pipe_write_cli_stdout)
        close(pipe_read_cli_stderr)
        dup2(pipe_write_cli_stderr,2)  # closes fd 2 (stderr) if open
        close(pipe_write_cli_stderr)

        msg = read(pipe_cli_end,2)
        if msg != 'go':
            mpd_raise('%s: invalid go msg from man :%s:' % (myId,msg) )
        close(pipe_cli_end)
        (pmiSocket,pmiAddr) = clientListenSocket.accept()
        pmiFile = fdopen(pmiSocket.fileno(),'r')
	msg = mpd_recv_one_line(pmiFile)
        ## mpd_print(0000, "recvd pmi handshake=:%s:" % msg )
        if not msg  or  msg != 'cmd=pmi_handler\n':    # handshake
            mpd_raise('%d: invalid msg from handler :%s:' % (myRank,msg) )
        clientPgmArgs = [clientPgm] + clientPgmArgs
        environ['PATH'] = environ['MPDMAN_CLI_PATH']
        environ['PMI_FD'] = str(pmiSocket.fileno())
        environ['PMI_SIZE'] = str(nprocs)
        environ['PMI_RANK'] = str(myRank)
        environ['PMI_DEBUG'] = str(0)
        for envvar in clientPgmEnv:
            (envkey,envval) = envvar.split('=')
            environ[envkey] = envval
        ## mpd_print(0000, 'execing clientPgm=:%s:' % (clientPgm) )
        try:
            execvpe(clientPgm,clientPgmArgs,environ)    # client
        except Exception, errmsg:
            ## mpd_raise('execvpe failed for client %s; errmsg=:%s:' % (clientPgm,errmsg) )
            print '%s: execvpe failed for: %s' % (myId,clientPgm)
            exit(0)
        _exit(0)  # just in case (does no cleanup)
    close(pipe_write_cli_stdout)
    close(pipe_write_cli_stderr)
    clientStdoutFD = pipe_read_cli_stdout
    clientStdoutFile = fdopen(clientStdoutFD,'r')
    socketsToSelect[clientStdoutFD] = 1
    clientStderrFD = pipe_read_cli_stderr
    clientStderrFile = fdopen(clientStderrFD,'r')
    socketsToSelect[clientStderrFD] = 1
    clientListenSocket.close()
    numWithIO = 2    # stdout and stderr so far
    waitPids = [clientPid]

    # connect to the client telling it that we are providing pmi service
    pmiSocket = mpd_get_inet_socket_and_connect('localhost',clientListenPort)
    pmiFile = fdopen(pmiSocket.fileno(),'r')
    mpd_send_one_line(pmiSocket,'cmd=pmi_handler\n')  # handshake
    socketsToSelect[pmiSocket] = 1

    # begin setup of stdio tree
    (parent,lchild,rchild) = mpd_get_ranks_in_binary_tree(myRank,nprocs)
    spawnedChildSockets = []
    childrenStdoutTreeSockets = []
    childrenStderrTreeSockets = []
    if lchild >= 0:
        numWithIO += 2    # stdout and stderr from child
        msgToSend = { 'cmd' : 'info_for_parent_in_tree',
                      'to_rank' : str(lchild),
                      'parent_host' : myHost,
                      'parent_port' : myPort }
        mpd_send_one_msg(rhsSocket,msgToSend)
    if rchild >= 0:
        numWithIO += 2    # stdout and stderr from child
        msgToSend = { 'cmd' : 'info_for_parent_in_tree',
                      'to_rank' : str(rchild),
                      'parent_host' : myHost,
                      'parent_port' : myPort }
        mpd_send_one_msg(rhsSocket,msgToSend)
    if myRank == 0:
        parentStdoutSocket = stdoutToConSocket
        parentStderrSocket = stderrToConSocket
        msgToSend = { 'cmd' : 'jobgo' }
        mpd_send_one_msg(rhsSocket,msgToSend)
    else:
        parentStdoutSocket = 0
        parentStderrSocket = 0

    if environ.has_key('MPDMAN_RSHIP'):
        rship = environ['MPDMAN_RSHIP']
        # (rshipSocket,rshipPort) = mpd_get_inet_listen_socket('',0)
        rshipPid = fork()
        if rshipPid == 0:
            environ['MPDCP_MSHIP_HOST'] = environ['MPDMAN_MSHIP_HOST']
            environ['MPDCP_MSHIP_PORT'] = environ['MPDMAN_MSHIP_PORT']
            environ['MPDCP_MSHIP_NPROCS'] = str(nprocs)
            environ['MPDCP_CLI_PID'] = str(clientPid)
            try:
                execvpe(rship,[rship],environ)
            except Exception, errmsg:
                # make sure my error msgs get to console
                dup2(parentStdoutSocket.fileno(),1)  # closes fd 1 (stdout) if open
                dup2(parentStderrSocket.fileno(),2)  # closes fd 2 (stderr) if open
                mpd_raise('execvpe failed for copgm %s; errmsg=:%s:' % (rship,errmsg) )
            _exit(0);  # do NOT do cleanup
        # rshipSocket.close()
        waitPids.append(rshipPid)

    pmiBarrierInRecvd = 0
    holdingPMIBarrierLoop1 = 0
    holdingEndBarrierLoop1 = 0
    endBarrierDone = 0
    numDone = 0
    mpdSocket = 0
    while not endBarrierDone:
        if get_sigtype_from_mpd:
            mpdSocket = mpd_get_inet_socket_and_connect('localhost',mpdPort)
            msgToSend = { 'cmd' : 'manager_needs_help', 'host' : myHost, 'port' : myPort }
            mpd_send_one_msg(mpdSocket,msgToSend)
            msg = mpd_recv_one_msg(mpdSocket)
            if (not msg.has_key('cmd')) or  \
               (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
                mpd_raise('%s: failed to recv challenge from rhs; msg=:%s:' % (myId,msg) )
            response = new(''.join([mpdConfPasswd,msg['randnum']])).digest()
            msgToSend = { 'cmd' : 'challenge_response', 'response' : response,
                          'host' : myHost, 'port' : myPort }
            mpd_send_one_msg(mpdSocket,msgToSend)
            msg = mpd_recv_one_msg(mpdSocket)
            if (not msg.has_key('cmd'))  or  (msg['cmd'] != 'OK_to_send_requests'):
                mpd_raise('%s: NOT OK to send requests to mpd; msg=:%s:' % (myId,msg) )
            msgToSend = { 'cmd' : 'get_signal_to_deliver', 'pid' : `getpid()`,
                          'jobid' : jobid }
            mpd_send_one_msg(mpdSocket,msgToSend)
            msg = mpd_recv_one_msg(mpdSocket)
            if msg['sigtype'].isdigit():
                signum = int(msg['sigtype'])
            else:
                import signal as tmpimp  # just to get valid SIG's
                exec('signum = %s' % 'tmpimp.SIG' + msg['sigtype'])
            try:    
                kill(clientPid,signum)
            except Exception, errmsg:
                mpd_print(1, 'invalid signal %d' % (signum) )
            mpdSocket.close()
            mpdSocket = 0
            get_sigtype_from_mpd = 0
        try:
            (inReadySockets,None,None) = select(socketsToSelect.keys(),[],[],30)
        except error, errmsg:
            if isinstance(errmsg,Exception)  and  errmsg[0] == 4:  # interrupted system call
                continue
            else:
                mpd_raise('%s: select failed: errmsg=:%s:' % (myId,errmsg) )
        for readySocket in inReadySockets:
            if readySocket not in socketsToSelect.keys():
                continue
            if readySocket == listenSocket:
                (tempSocket,tempConnAddr) = listenSocket.accept()
                msg = mpd_recv_one_msg(tempSocket)
                if msg  and  msg.has_key('cmd'):
                    if msg['cmd'] == 'child_in_stdout_tree':
                        socketsToSelect[tempSocket] = 1
                        childrenStdoutTreeSockets.append(tempSocket)
                    elif msg['cmd'] == 'child_in_stderr_tree':
                        socketsToSelect[tempSocket] = 1
                        childrenStderrTreeSockets.append(tempSocket)
                    elif msg['cmd'] == 'spawned_child':
                        socketsToSelect[tempSocket] = 1
                        spawnedChildSockets.append(tempSocket)
                    else:
                        mpd_print(1, 'unknown msg recvd on listenSocket :%s:' % (msg) )
            elif readySocket == lhsSocket:
                msg = mpd_recv_one_msg(lhsSocket)
                if not msg:
                    mpd_print(1, 'lhs died' )
                    del socketsToSelect[lhsSocket]
                    lhsSocket.close()
                elif msg['cmd'] == 'jobgo':
                    if myRank == 0:
                        msgToSend = { 'cmd' : 'job_started', 'jobid' : jobid }
                        mpd_send_one_msg(conSocket,msgToSend)
                    else:
                        mpd_send_one_msg(rhsSocket,msg)  # forward it on
                    write(pipe_man_end,'go')
                    close(pipe_man_end)
                elif msg['cmd'] == 'info_for_parent_in_tree':
                    if int(msg['to_rank']) == myRank:
                        parentHost = msg['parent_host']
                        parentPort = msg['parent_port']
                        parentStdoutSocket = \
                            mpd_get_inet_socket_and_connect(parentHost,parentPort)
                        msgToSend = { 'cmd' : 'child_in_stdout_tree', 'from_rank' : myRank }
                        mpd_send_one_msg(parentStdoutSocket,msgToSend)
                        parentStderrSocket = \
                            mpd_get_inet_socket_and_connect(parentHost,parentPort)
                        msgToSend = { 'cmd' : 'child_in_stderr_tree', 'from_rank' : myRank }
                        mpd_send_one_msg(parentStderrSocket,msgToSend)
                    else:
                        mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'end_barrier_loop_1':
                    if myRank == 0:
                        msgToSend = { 'cmd' : 'end_barrier_loop_2' }
                        mpd_send_one_msg(rhsSocket,msgToSend)
                    else:
                        if numDone >= numWithIO:
                            mpd_send_one_msg(rhsSocket,msg)
                        else:
                            holdingEndBarrierLoop1 = 1
                elif msg['cmd'] == 'end_barrier_loop_2':
                    endBarrierDone = 1
                    if myRank != 0:
                        mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'pmi_barrier_loop_1':
                    if myRank == 0:
                        msgToSend = { 'cmd' : 'pmi_barrier_loop_2' }
                        mpd_send_one_msg(rhsSocket,msgToSend)
                        pmiMsgToSend = 'cmd=barrier_out\n'
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    else:
                        holdingPMIBarrierLoop1 = 1
                        if pmiBarrierInRecvd:
                            mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'pmi_barrier_loop_2':
                    pmiBarrierInRecvd = 0
                    holdingPMIBarrierLoop1 = 0
                    if myRank != 0:
                        pmiMsgToSend = 'cmd=barrier_out\n'
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                        mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'pmi_get':
                    if msg['from_rank'] == myRank:
                        pmiMsgToSend = 'cmd=get_result rc=-1 msg="%s"\n' % msg['key']
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    else:
                        kvsname = msg['kvsname']
                        key = msg['key']
                        cmd = 'value = ' + kvsname + '["' + key + '"]'
                        try:
                            exec(cmd)
                            gotit = 1
                        except Exception, errmsg:
                            gotit = 0
                        if gotit:
                            msgToSend = { 'cmd' : 'pmi_get_response', 'value' : value, 'to_rank' : msg['from_rank'] }
                            mpd_send_one_msg(rhsSocket,msgToSend)
                        else:
                            mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'pmi_get_response':
                    if msg['to_rank'] == myRank:
                        pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (msg['value'])
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    else:
                        mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'signal':
                    if msg['signo'] == 'SIGINT':
                        if conSocket:
                            msgToSend = { 'cmd' : 'job_terminated_early', 'jobid' : jobid, 'id' : myId }
                            mpd_send_one_msg(conSocket,msgToSend)
                            conSocket.close()
                        if rhsSocket in socketsToSelect.keys():  # still alive ?
                            mpd_send_one_msg(rhsSocket,msg)
                            rhsSocket.close()
                        kill(0,SIGKILL)  # pid 0 -> all in my process group
                        _exit(0)
                    elif msg['signo'] == 'SIGTSTP':
                        if msg['dest'] != myId:
                            mpd_send_one_msg(rhsSocket,msg)
                            kill(clientPid,SIGTSTP)
                    elif msg['signo'] == 'SIGCONT':
                        if msg['dest'] != myId:
                            mpd_send_one_msg(rhsSocket,msg)
                            kill(clientPid,SIGCONT)
                elif msg['cmd'] == 'client_exit_status':
                    if myRank == 0:
                        mpd_send_one_msg(conSocket,msg)
                    else:
                        mpd_send_one_msg(rhsSocket,msg)
                elif msg['cmd'] == 'collective_abort':
		    if msg['src'] != myId:
                        if conSocket:
                            msgToSend = { 'cmd' : 'job_terminated_early', 'jobid' : jobid, 'rank' : msg['rank'] }
                            mpd_send_one_msg(conSocket,msgToSend)
                            # conSocket.close()
                        if rhsSocket in socketsToSelect.keys():  # still alive ?
                            mpd_send_one_msg(rhsSocket,msg)
                            # rhsSocket.close()
			try:
                            kill(clientPid,SIGKILL)
			except:
			    pass    # client may already be gone
                else:
                    mpd_print(1, 'unexpected msg recvd on lhsSocket :%s:' % msg )
            elif readySocket == rhsSocket:
                msg = mpd_recv_one_msg(rhsSocket)
                mpd_print(0000, 'rhs died' )
                del socketsToSelect[rhsSocket]
                rhsSocket.close()
            elif readySocket == clientStdoutFD:
                line = read(clientStdoutFD,1024)
                # line = clientStdoutFile.readline()
                if not line:
                    del socketsToSelect[clientStdoutFD]
                    close(clientStdoutFD)
                    numDone += 1
                    if numDone >= numWithIO:
                        if parentStdoutSocket:
                            parentStdoutSocket.close()
                            parentStdoutSocket = 0
                        if parentStderrSocket:
                            parentStderrSocket.close()
                            parentStderrSocket = 0
                        if myRank == 0 or holdingEndBarrierLoop1:
                            holdingEndBarrierLoop1 = 0
                            msgToSend = {'cmd' : 'end_barrier_loop_1'}
                            mpd_send_one_msg(rhsSocket,msgToSend)
                else:
                    if parentStdoutSocket:
                        if lineLabels:
                            splitLine = line.split('\n',1024)
                            if startLineLabel:
                                line = myLineLabel
                            else:
                                line = ''
                            if splitLine[-1] == '':
                                startLineLabel = 1
                                del splitLine[-1]
                            else:
                                startLineLabel = 0
                            for s in splitLine[0:-1]:
                                line = line + s + '\n' + myLineLabel
                            line = line + splitLine[-1]
                            if startLineLabel:
                                line = line + '\n'
                        mpd_send_one_line(parentStdoutSocket,line)
                        # parentStdoutSocket.sendall('STDOUT by %d: |%s|' % (myRank,line) )
            elif readySocket == clientStderrFD:
                line = read(clientStderrFD,1024)
                # line = clientStderrFile.readline()
                if not line:
                    del socketsToSelect[clientStderrFD]
                    close(clientStderrFD)
                    numDone += 1
                    if numDone >= numWithIO:
                        if parentStdoutSocket:
                            parentStdoutSocket.close()
                            parentStdoutSocket = 0
                        if parentStderrSocket:
                            parentStderrSocket.close()
                            parentStderrSocket = 0
                        if myRank == 0 or holdingEndBarrierLoop1:
                            holdingEndBarrierLoop1 = 0
                            msgToSend = {'cmd' : 'end_barrier_loop_1'}
                            mpd_send_one_msg(rhsSocket,msgToSend)
                else:
                    if parentStderrSocket:
                        # note not handling linelabels for stderr right now
                        mpd_send_one_line(parentStderrSocket,line)
            elif readySocket in childrenStdoutTreeSockets:
                line = readySocket.recv(1024)
                if not line:
                    del socketsToSelect[readySocket]
                    readySocket.close()
                    numDone += 1
                    if numDone >= numWithIO:
                        if parentStdoutSocket:
                            parentStdoutSocket.close()
                            parentStdoutSocket = 0
                        if parentStderrSocket:
                            parentStderrSocket.close()
                            parentStderrSocket = 0
                        if myRank == 0 or holdingEndBarrierLoop1:
                            holdingEndBarrierLoop1 = 0
                            msgToSend = {'cmd' : 'end_barrier_loop_1'}
                            mpd_send_one_msg(rhsSocket,msgToSend)
                else:
                    if parentStdoutSocket:
                        mpd_send_one_line(parentStdoutSocket,line)
                        # parentStdoutSocket.sendall('FWD by %d: |%s|' % (myRank,line) )
            elif readySocket in childrenStderrTreeSockets:
                line = readySocket.recv(1024)
                if not line:
                    del socketsToSelect[readySocket]
                    readySocket.close()
                    numDone += 1
                    if numDone >= numWithIO:
                        if parentStdoutSocket:
                            parentStdoutSocket.close()
                            parentStdoutSocket = 0
                        if parentStderrSocket:
                            parentStderrSocket.close()
                            parentStderrSocket = 0
                        if myRank == 0 or holdingEndBarrierLoop1:
                            holdingEndBarrierLoop1 = 0
                            msgToSend = {'cmd' : 'end_barrier_loop_1'}
                            mpd_send_one_msg(rhsSocket,msgToSend)
                else:
                    if parentStderrSocket:
                        mpd_send_one_line(parentStderrSocket,line)
                        # parentStdoutSocket.sendall('FWD by %d: |%s|' % (myRank,line) )
            elif readySocket in spawnedChildSockets:
                msg = mpd_recv_one_msg(readySocket)
                if not msg:
                    del socketsToSelect[readySocket]
                    readySocket.close()
                elif msg['cmd'] == 'job_started'  or  msg['cmd'] == 'job_terminated':
                    pass
                elif msg['cmd'] == 'spawned_childs_kvs':
                    ## NOTE: if you spawn a non-MPI job, it may not send this msg
                    ## in which case the pgm will hang
                    exec('%s = %s' % (msg['kvsname'],msg['kvs']))  # copy remote kvs here
                    exec('default_kvs = %s' % default_kvsname)  # prepare to send my kvs
                    msgToSend = { 'cmd' : 'parents_kvs',
                                  'kvsname' : default_kvsname,
                                  'kvs' : default_kvs }
                    mpd_send_one_msg(readySocket,msgToSend)
                    pmiMsgToSend = \
                         'cmd=spawn_result status=spawn_done remote_kvsname=%s\n' % \
                             msg['kvsname']
                    mpd_send_one_line(pmiSocket,pmiMsgToSend)
                else:
                    mpd_print(1, "unrecognized msg from spawned child :%s:" % line )
            elif readySocket == pmiSocket:
	        line = mpd_recv_one_line(pmiFile)
                if not line:
                    (donePid,status) = waitpid(clientPid,0)
                    msgToSend = { 'cmd' : 'client_exit_status', 'status' : status,
                                  'id' : myId, 'rank' : myRank }
                    if myRank == 0:
                        mpd_send_one_msg(conSocket,msgToSend)
                    else:
                        mpd_send_one_msg(rhsSocket,msgToSend)
                    del socketsToSelect[pmiSocket]
                    pmiSocket.close()
                    if pmiCollectiveJob:
                        if conSocket:
                            msgToSend = { 'cmd' : 'job_terminated_early', 'jobid' : jobid, 'rank' : myRank }
                            mpd_send_one_msg(conSocket,msgToSend)
                            # conSocket.close()
                        if rhsSocket in socketsToSelect.keys():  # still alive ?
                            msgToSend = { 'cmd' : 'collective_abort', 'src' : myId, 'rank' : myRank}
                            mpd_send_one_msg(rhsSocket,msgToSend)
                            # rhsSocket.close()
			try:
                            kill(clientPid,SIGKILL)
			except:
			    pass    # client may already be gone
                else:
                    parsedMsg = parse_pmi_msg(line)
                    if parsedMsg['cmd'] == 'get_my_kvsname':
                        pmiMsgToSend = 'cmd=my_kvsname kvsname=%s\n' % (default_kvsname)
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    elif parsedMsg['cmd'] == 'get_maxes':
                        pmiMsgToSend = 'cmd=maxes kvsname_max=4096 ' + \
                                       'keylen_max=4096 vallen_max=4096\n'
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                        pmiCollectiveJob = 1    # really needs a pmi init
                    elif parsedMsg['cmd'] == 'create_kvs':
                        new_kvsname = kvsname_template + str(kvs_next_id)
                        exec('%s = {}' % (new_kvsname))
                        kvs_next_id += 1
                        pmiMsgToSend = 'cmd=newkvs kvsname=%s\n' % (new_kvsname)
                        mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    elif parsedMsg['cmd'] == 'put':
                        kvsname = parsedMsg['kvsname']
                        key = parsedMsg['key']
                        value = parsedMsg['value']
                        cmd = kvsname + '["' + key + '"] = "' + value + '"'
                        try:
                            exec(cmd)
                            pmiMsgToSend = 'cmd=put_result rc=0\n'
                            mpd_send_one_line(pmiSocket,pmiMsgToSend)
                        except Exception, errmsg:
                            pmiMsgToSend = 'cmd=put_result rc=-1 msg="%s"\n' % errmsg
                            mpd_send_one_line(pmiSocket,pmiMsgToSend)
                    elif parsedMsg['cmd'] == 'barrier_in':
                        pmiBarrierInRecvd = 1
                        if myRank == 0  or  holdingPMIBarrierLoop1:
                            msgToSend = { 'cmd' : 'pmi_barrier_loop_1' }
                            mpd_send_one_msg(rhsSocket,msgToSend)
                        if myRank == 0  and  spawned:
                            ## NOTE: a non-MPI job might not call the pmi barrier code;
                            ## this may cause the pgm to hang if it does spawns
                            exec('default_kvs = %s' % default_kvsname)
                            default_kvs['rmb_test'] = 'magpie'  # just for testing
                            msgToSend = { 'cmd' : 'spawned_childs_kvs',
                                          'kvsname' : default_kvsname,
                                          'kvs' : default_kvs }
                            mpd_send_one_msg(conSocket,msgToSend)
                            msg = mpd_recv_one_msg(conSocket)
                            exec('%s = %s' % (msg['kvsname'],msg['kvs']))  # get parentkvs
                    elif parsedMsg['cmd'] == 'get':
                        kvsname = parsedMsg['kvsname']
                        key = parsedMsg['key']
                        cmd = 'value = ' + kvsname + '["' + key + '"]'
                        try:
                            exec(cmd)
                            gotit = 1
                        except Exception, errmsg:
                            gotit = 0
                        if gotit:
                            pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (value)
                            mpd_send_one_line(pmiSocket,pmiMsgToSend)
                        else:
                            msgToSend = { 'cmd' : 'pmi_get', 'key' : key,
                                          'kvsname' : kvsname, 'from_rank' : myRank }
                            mpd_send_one_msg(rhsSocket,msgToSend)
                    elif parsedMsg['cmd'] == 'spawn':
                        mpdSocket = mpd_get_inet_socket_and_connect('localhost',mpdPort)
                        msgToSend = { 'cmd' : 'manager_needs_help',
                                      'host' : myHost,
                                      'port' : myPort }
                        mpd_send_one_msg(mpdSocket,msgToSend)
                        msg = mpd_recv_one_msg(mpdSocket)
                        if (not msg.has_key('cmd')) or  \
                           (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
                            mpd_raise('%s: failed to recv challenge from rhs; msg=:%s:' \
                                      % (myId,msg) )
                        response = new(''.join([mpdConfPasswd,msg['randnum']])).digest()
                        msgToSend = { 'cmd' : 'challenge_response',
                                      'response' : response,
                                      'host' : myHost,
                                      'port' : myPort }
                        mpd_send_one_msg(mpdSocket,msgToSend)
                        msg = mpd_recv_one_msg(mpdSocket)
                        if (not msg.has_key('cmd')) or  \
                           (msg['cmd'] != 'OK_to_send_requests'):
                            mpd_raise('%s: NOT OK to send requests to mpd; msg=:%s:' \
                                      % (myId,msg) )
                        nprocs  = int(parsedMsg['nprocs'])
                        hosts   = { (0,nprocs-1) : '_any_' }
                        execs   = { (0,nprocs-1) : parsedMsg['execname'] }
                        users   = { (0,nprocs-1) : mpd_get_my_username() }
                        cwds    = { (0,nprocs-1) : environ['MPDMAN_CWD'] }
                        paths   = { (0,nprocs-1) : '' }
                        args    = { (0,nprocs-1) : parsedMsg['arg'] } # fix later to handle
                                                                      # more than 1 arg
                        envvars = { (0,nprocs-1) : '' }
                        msgToSend = { 'cmd' : 'spawn',
                                      'conhost'  : gethostname(),
                                      'conport'  : myPort,
                                      'spawned'  : 1,
                                      'nstarted' : 0,
                                      'nprocs'   : nprocs,
                                      'hosts'    : hosts,
                                      'execs'    : execs,
                                      'users'    : users,
                                      'cwds'     : cwds,
                                      'paths'    : paths,
                                      'args'     : args,
                                      'envvars'  : envvars
                                    }
                        numWithIO += 2    # this proc may produce stdout and stderr
                        mpd_send_one_msg(mpdSocket,msgToSend)
                        # we send a result back to client after exchging kvs with spawnee
                        mpdSocket.close()
                        mpdSocket = 0
                    elif parsedMsg['cmd'] == 'finalize':
                        pmiCollectiveJob = 0
                    else:
                        mpd_print(1, "unrecognized pmi msg :%s:" % line )
            elif readySocket == conSocket:
                msg = mpd_recv_one_msg(conSocket)
                if not msg:
                    del socketsToSelect[conSocket]
                    conSocket.close()
                    conSocket = 0
                elif msg['cmd'] == 'signal':
                    if msg['signo'] == 'SIGINT':
                        mpd_send_one_msg(rhsSocket,msg)
                        rhsSocket.close()
                        kill(0,SIGKILL)  # pid 0 -> all in my process group
                        _exit(0)
                    elif msg['signo'] == 'SIGTSTP':
                        msg['dest'] = myId
                        mpd_send_one_msg(rhsSocket,msg)
                        kill(clientPid,SIGTSTP)
                    elif msg['signo'] == 'SIGCONT':
                        msg['dest'] = myId
                        mpd_send_one_msg(rhsSocket,msg)
                        kill(clientPid,SIGCONT)
                else:
                    mpd_print(1, 'unexpected msg recvd on conSocket :%s:' % msg )
            elif readySocket == mpdSocket:
                    mpd_print(1, 'unexpected msg recvd on mpdSocket :%s:' % msg )
            else:
                mpd_print(1, 'recvd msg on unknown socket :%s:' % readySocket )
    mpd_print(0000, "out of loop")
    if myRank == 0:
        if conSocket:    # may race with spawner to exit
            msgToSend = { 'cmd' : 'job_terminated',
                          'jobid' : jobid }
            mpd_send_one_msg(conSocket,msgToSend)
            conSocket.close()
    # may want to want to wait for waitPids here

def parse_pmi_msg(msg):
    parsed_msg = {}
    sm = findall(r'\S+',msg)
    for e in sm:
        se = e.split('=')
        parsed_msg[se[0]] = se[1]
    return parsed_msg

def sigusr1_handler(signum,frame):
    global get_sigtype_from_mpd
    get_sigtype_from_mpd = 1

if __name__ == '__main__':
    if not environ.has_key('MPDMAN_CLI_PGM'):    # assume invoked from keyboard
        print 'mpdman for mpd version: %s' % str(mpd_version)
        print 'mpdman does NOT run as a console program; should be execd by mpd'
        exit(-1)
    try:
        mpdman()
    except mpdError, errmsg:
        print 'mpdman failed; cause: %s' % (errmsg)  ##
        pass
