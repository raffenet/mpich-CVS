#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys       import stdout, argv, settrace, exit
from os        import environ, getpid, fork, setpgrp, waitpid, kill, chdir, \
                      setsid, getuid, setuid, setreuid, setregid, setgroups, \
                      umask, close, access, path, stat, unlink, strerror, \
                      R_OK, X_OK, WNOHANG
from pwd       import getpwnam
from socket    import socket, AF_UNIX, SOCK_STREAM, gethostname
from errno     import EINTR
from select    import select, error
from getopt    import getopt
from types     import FunctionType
from signal    import signal, SIGCHLD, SIGKILL, SIGHUP, SIG_IGN
from atexit    import register
from time      import sleep
from random    import seed, randrange
from syslog    import syslog, openlog, closelog, LOG_DAEMON, LOG_INFO, LOG_ERR
from md5       import new
from cPickle   import dumps
from mpdlib    import mpd_print, mpd_print_tb, mpd_get_ranks_in_binary_tree, \
                      mpd_send_one_msg, mpd_recv_one_msg, \
                      mpd_get_inet_listen_socket, mpd_get_inet_socket_and_connect, \
                      mpd_set_procedures_to_trace, mpd_trace_calls, mpd_raise, mpdError, \
                      mpd_get_my_username, mpd_get_groups_for_username, \
                      mpd_set_my_id, mpd_check_python_version, mpd_version, \
                      mpd_socketpair, mpd_same_ips
from mpdman    import mpdman

class _ActiveSockInfo:
    pass

class g:    # global data items
    pass


def _mpd_init():
    global stdout
    close(0)
    g.myPid = getpid()
    (g.mySocket,g.myPort) = mpd_get_inet_listen_socket('',0)
    if g.echoPortNum:    # do this before becoming a daemon
        print g.myPort
        stdout.flush()
    g.myId = '%s_%d' % (g.myHost,g.myPort)
    mpd_set_my_id(g.myId)

    # setup syslog
    openlog("mpd",0,LOG_DAEMON)
    syslog(LOG_INFO,"mpdid=%s (port=%d)" % (g.myId,g.myPort) )

    if g.daemon:      # see if I should become a daemon with no controlling tty
        rc = fork()
        if rc != 0:   # parent exits; child in background
            exit(0)
        setsid()  # become session leader; no controlling tty
        signal(SIGHUP,SIG_IGN)  # make sure no sighup when leader ends
        ## leader exits; svr4: make sure do not get another controlling tty
        rc = fork()
        if rc != 0:
            exit(0)
        chdir("/")  # free up filesys for umount
        umask(0)
        g.logfileName = '/tmp/mpd2.logfile_' + mpd_get_my_username()
        logfile = open(g.logfileName,'w')
        stdout = logfile
        stderr = logfile
        print >>stdout, 'logfile for mpd with pid %d' % getpid()
        stdout.flush()

    mpd_print(0, 'starting ')
    g.activeSockets = {}
    _add_active_socket(g.mySocket,
                       'my (%s) listener socket' % g.myId, # name
                       '_handle_new_connection',           # handler
                       '',0)                               # host,port
    g.nextJobInt = 1
    g.activeJobs = {}
    
    seed()
    g.correctChallengeResponse = {}
    g.conListenSocket = 0
    g.conSocket       = 0
    g.allExiting      = 0
    if g.allowConsole:
        g.conListenName = '/tmp/mpd2.console_' + mpd_get_my_username()
        consoleAlreadyExists = 0
        if access(g.conListenName,R_OK):    # if console is there, see if mpd is listening
            tempSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
            try:
                tempSocket.connect(g.conListenName)
                consoleAlreadyExists = 1
            except Exception, errmsg:
                tempSocket.close()
                unlink(g.conListenName)
        if consoleAlreadyExists:
            # mpd_raise('an mpd is already running with console at %s' %  (g.conListenName) )
            print 'An mpd is already running with console at %s on %s. ' %  (g.conListenName, g.myHost) 
            print 'Start mpd with the -n option for second mpd on same host.'
            exit(0)
        g.conListenSocket = socket(AF_UNIX,SOCK_STREAM)  # UNIX
        g.conListenSocket.bind(g.conListenName)
        g.conListenSocket.listen(1)
        _add_active_socket(g.conListenSocket,
                           'my (%s) console listen socket' % g.myId,  # name
                           'handled-inline',                          # handler
                           g.conListenName,0)                         # host,port
        
    g.generation = 0  # will chg when enter the ring
    if g.entryHost:
        _enter_existing_ring()
    else:
        _create_ring_of_one_mpd()
    
    signal(SIGCHLD,sigchld_handler)


def _mpd():
    global stdout
    # Main Loop
    done = 0
    while not done:
        socketsToSelect = g.activeSockets.keys()
        try:
            (inReadySockets,None,None) = select(socketsToSelect,[],[],30)
        except error, data:
            if data[0] == EINTR:        # will come here if receive SIGCHLD, for example
                continue
            else:
                mpd_raise('select error: %s' % strerror(data[0]))
        except Exception, data:
            mpd_raise('other error after select %s :%s:' % ( data.__class__, data) )
        for readySocket in inReadySockets:
            if readySocket not in g.activeSockets.keys():  # deleted on another iteration ?
                continue
            if readySocket == g.mySocket:
                _handle_new_connection()
            elif readySocket == g.lhsSocket:
                _handle_lhs_input()
                if g.allExiting:          # got mpdallexit command
                    done = 1
                    break                # out of for loop, then out of while
            elif readySocket == g.rhsSocket:
                _handle_rhs_input()    # ignoring rc=1 which means we re-entered ring
            elif readySocket == g.conSocket:
                _handle_console_input()
            elif g.activeSockets[readySocket].name == 'rhs_being_challenged':
                _handle_rhs_challenge_response(readySocket)
            elif g.activeSockets[readySocket].name == 'lhs_being_challenged':
                _handle_lhs_challenge_response(readySocket)
            elif g.activeSockets[readySocket].name == 'man_msgs':
                _handle_man_msgs(readySocket)
            elif readySocket == g.conListenSocket:
                _handle_console_connection()
            else:
                mpd_raise('unknown ready socket %s' %  \
                          (`g.activeSockets[readySocket].name`) )

def _handle_console_connection():
    if not g.conSocket:
        (g.conSocket,newConnAddr) = g.conListenSocket.accept()
        _add_active_socket(g.conSocket,
                           'my (%s) console socket' % g.myId,  # name
                           '_handle_console_input',            # handler
                           g.conSocket,0)                      # host,port
    else:
        return  ## postpone it; hope the other one frees up soon
        ## we used to deny the second console; now just let it wait for us
        # mpd_print(1, 'rejecting console; already have one' )
        # (tempSocket,newConnAddr) = g.conListenSocket.accept()
        # msgToSend = { 'cmd' : 'already_have_a_console' }
        # mpd_send_one_msg(tempSocket,msgToSend)
        # tempSocket.close()

def _handle_console_input():
    msg = mpd_recv_one_msg(g.conSocket)
    if not msg:
        mpd_print(0000, 'console has disappeared; closing it')
        del g.activeSockets[g.conSocket]
        g.conSocket.close()
        g.conSocket = 0
        return
    if not msg.has_key('cmd'):
        mpd_print(1, 'console sent bad msg :%s:' % msg)
        mpd_send_one_msg(g.rhsSocket,'cmd','invalid_msg_received_from_you')
        del g.activeSockets[g.conSocket]
        g.conSocket.close()
        g.conSocket = 0
        return
    if msg['cmd'] == 'mpdrun':
        msg['mpdid_mpdrun_start'] = g.myId
        msg['nstarted_on_this_loop'] = 0
        msg['first_loop'] = 1
        if msg.has_key('try_0_locally'):
            _do_mpdrun(msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
        # send ack after job is going
    elif msg['cmd'] == 'get_mpd_version':
        msg = { 'cmd' : 'mpd_version_response', 'mpd_version' : mpd_version }
        mpd_send_one_msg(g.conSocket,msg)
    elif msg['cmd'] == 'mpdtrace':
        msgToSend = { 'cmd'  : 'mpdtrace_info',
                      'dest' : g.myId,
                      'id'   : g.myId,
                      'lhs'  : '%s_%d' % (g.lhsHost,g.lhsPort),
                      'rhs'  : '%s_%d' % (g.rhsHost,g.rhsPort) }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        msgToSend = { 'cmd'  : 'mpdtrace_trailer',
                      'dest' : g.myId }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        # do not send an ack to console now; will send trace info later
    elif msg['cmd'] == 'mpdallexit':
        g.allExiting = 1
        mpd_send_one_msg(g.rhsSocket, {'cmd' : 'mpdallexit', 'src' : g.myId} )
        mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdallexit_ack'} )
    elif msg['cmd'] == 'mpdringtest':
        msg['src'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # do not send an ack to console now; will send ringtest info later
    elif msg['cmd'] == 'mpdlistjobs':
        msgToSend = { 'cmd'  : 'local_mpdid', 'id' : g.myId }
        mpd_send_one_msg(g.conSocket,msgToSend)
        for jobid in g.activeJobs.keys():
            for manPid in g.activeJobs[jobid]:
                msgToSend = { 'cmd' : 'mpdlistjobs_info',
                              'dest' : g.myId,
                              'jobid' : jobid,
                              'username' : g.activeJobs[jobid][manPid]['username'],
                              'host' : g.myHost,
                              'clipid' : str(g.activeJobs[jobid][manPid]['clipid']),
                              'sid' : str(manPid),  # may chg to actual sid later
                              'pgm'  : g.activeJobs[jobid][manPid]['pgm'],
                              'rank' : g.activeJobs[jobid][manPid]['rank'] }
                mpd_send_one_msg(g.conSocket, msgToSend)
        msgToSend = { 'cmd'  : 'mpdlistjobs_trailer', 'dest' : g.myId }
        mpd_send_one_msg(g.rhsSocket,msgToSend)
        # do not send an ack to console now; will send listjobs info later
    elif msg['cmd'] == 'mpdkilljob':
        msg['src'] = g.myId
        msg['handled'] = 0
        if msg['mpdid'] == '':
            msg['mpdid'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # send ack to console after I get this msg back and do the kill myself
    elif msg['cmd'] == 'mpdsigjob':
        msg['src'] = g.myId
        msg['handled'] = 0
        if msg['mpdid'] == '':
            msg['mpdid'] = g.myId
        mpd_send_one_msg(g.rhsSocket, msg)
        # send ack to console after I get this msg back
    else:
        msgToSend = { 'cmd' : 'invalid_msg_received_from_you' }
        mpd_send_one_msg(g.conSocket,msgToSend)
        badMsg = 'invalid msg received from console: %s' % (str(msg))
        mpd_print(1, badMsg)
        syslog(LOG_ERR,badMsg)

def _handle_lhs_input():
    msg = mpd_recv_one_msg(g.lhsSocket)
    mpd_print(0000, "MPD LHS GOT MSG :%s:" % msg)
    if not msg:    # lost lhs; don't worry
        mpd_print(0, "CLOSING g.lhsSocket ", g.lhsSocket )
        del g.activeSockets[g.lhsSocket]
        g.lhsSocket.close()
        return
    if msg['cmd'] == 'mpdrun':
        if msg.has_key('mpdid_mpdrun_start')  and  msg['mpdid_mpdrun_start'] == g.myId:
            if msg['nstarted'] == msg['nprocs']:
                if g.conSocket:
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdrun_ack', } )
                return
            if not msg['first_loop']  and  msg['nstarted_on_this_loop'] == 0:
                if msg.has_key('jobid'):
                    mpd_send_one_msg(g.rhsSocket,
                                     {'cmd':'abortjob', 'src' : g.myId,
                                      'jobid' : msg['jobid']})
                if g.conSocket:
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'job_failed',
                                                   'reason' : 'some_procs_not_started',
                                                   'remaining_hosts' : msg['hosts'] } )
                return
            msg['first_loop'] = 0
            msg['nstarted_on_this_loop'] = 0
        _do_mpdrun(msg)
    elif msg['cmd'] == 'mpdtrace_info':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'mpdtrace_trailer':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            msgToSend = { 'cmd'  : 'mpdtrace_info',
                          'dest' : msg['dest'],
                          'id'   : g.myId,
                          'lhs'  : '%s_%d' % (g.lhsHost,g.lhsPort),
                          'rhs'  : '%s_%d' % (g.rhsHost,g.rhsPort) }
            mpd_send_one_msg(g.rhsSocket, msgToSend)
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdlistjobs_info':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
    elif msg['cmd'] == 'mpdlistjobs_trailer':
        if msg['dest'] == g.myId:
            mpd_send_one_msg(g.conSocket,msg)
        else:
            for jobid in g.activeJobs.keys():
                for manPid in g.activeJobs[jobid]:
                    msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                  'dest' : msg['dest'],
                                  'jobid' : jobid,
                                  'username' : g.activeJobs[jobid][manPid]['username'],
                                  'host' : g.myHost,
                                  'clipid' : str(g.activeJobs[jobid][manPid]['clipid']),
                                  'sid' : str(manPid),  # may chg to actual sid later
                                  'pgm' : g.activeJobs[jobid][manPid]['pgm'],
                                  'rank' : g.activeJobs[jobid][manPid]['rank'] }
                    mpd_send_one_msg(g.rhsSocket, msgToSend)
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdallexit':
        g.allExiting = 1
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket, msg)
    elif msg['cmd'] == 'mpdringtest':
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket, msg)
        else:
            numLoops = msg['numloops'] - 1
            if numLoops > 0:
                msg['numloops'] = numLoops
                mpd_send_one_msg(g.rhsSocket, msg)
            else:
                if g.conSocket:    # may have closed it if user did ^C at console
                    mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdringtest_done' })
    elif msg['cmd'] == 'mpdsigjob':
        if msg['handled']:    # already handled on at least one other host
            mpd_send_one_msg(g.rhsSocket,msg)
        handledHere = 0
        for jobid in g.activeJobs.keys():
            sjobid = jobid.split('  ')  # jobnum and mpdid
            if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
            or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                for manPid in g.activeJobs[jobid].keys():
                    if g.activeJobs[jobid][manPid]['username'] == msg['username']  \
                    or g.activeJobs[jobid][manPid]['username'] == 'root':
                        manSocket = g.activeJobs[jobid][manPid]['socktoman']
                        mpd_send_one_msg(manSocket, { 'cmd' : 'signal_to_handle',
                                                      'sigtype' : msg['sigtype'] } )
                        handledHere = 1
        if handledHere:
            msg['handled'] = 1
        if not msg['handled']  and  msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        if msg['src'] == g.myId:
            mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdsigjob_ack',
                                           'handled' : msg['handled'] } )
    elif msg['cmd'] == 'mpdkilljob':
        if msg['handled']:    # already handled on at least one other host
            mpd_send_one_msg(g.rhsSocket,msg)
        handledHere = 0
        for jobid in g.activeJobs.keys():
            sjobid = jobid.split('  ')  # jobnum and mpdid
            if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
            or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                for manPid in g.activeJobs[jobid].keys():
                    if g.activeJobs[jobid][manPid]['username'] == msg['username']  \
                    or g.activeJobs[jobid][manPid]['username'] == 'root':
                        kill(manPid * (-1), SIGKILL)  # neg manPid -> group
                        handledHere = 1
                # del g.activeJobs[jobid]  ## handled when child goes away
        if handledHere:
            msg['handled'] = 1
        if not msg['handled']  and  msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        if msg['src'] == g.myId:
            mpd_send_one_msg(g.conSocket, {'cmd' : 'mpdkilljob_ack',
                                           'handled' : msg['handled'] } )
    elif msg['cmd'] == 'abortjob':
        if msg['src'] != g.myId:
            mpd_send_one_msg(g.rhsSocket,msg)
        for jobid in g.activeJobs.keys():
            if jobid == msg['jobid']:
                for manPid in g.activeJobs[jobid].keys():
                    kill(manPid * (-1), SIGKILL)  # neg manPid -> group
                # del g.activeJobs[jobid]  ## handled when child goes away
    else:
        mpd_print(1, 'unrecognized cmd from lhs: %s' % (msg) )

def _do_mpdrun(msg):
    mpd_print(0000, "DO_MPDRUN MSG=:%s:" % msg)
    handled_one__any_ = 0
    while 1:
        if msg['nstarted'] >= msg['nprocs']:
            break
        if handled_one__any_:
            break
        hosts = msg['hosts']
        chked_ips = [ mpd_same_ips(g.myHost,x) for x in hosts.values() ]
        if 1 not in chked_ips  and  '_any_' not in hosts.values():
            break
        currRank = msg['nstarted']
        found = 0
        for ranks in hosts.keys():
            (lo,hi) = ranks
            if mpd_same_ips(g.myHost,hosts[ranks]) and (currRank >= lo and currRank <= hi):
                found = 1
                break
        if not found:
            for ranks in hosts.keys():
                (lo,hi) = ranks
                if hosts[ranks] == '_any_'  and  (currRank >= lo and currRank <= hi):
                    handled_one__any_ = 1
                    found = 1
                    break
        if not found:
            break
        if lo < hi:
            msg['hosts'][(lo+1,hi)] = msg['hosts'][ranks]
        del msg['hosts'][ranks]
        msg['nstarted'] += 1
        msg['nstarted_on_this_loop'] += 1
        if currRank == 0:
            manLhsHost = 'dummy_host'
            manLhsPort = 0
        else:
            manLhsHost = msg['lhshost']
            manLhsPort = msg['lhsport']
        (tempSocket,tempPort) = mpd_get_inet_listen_socket('',0)
        (toManSocket,toMpdSocket) = mpd_socketpair()
        msg['lhshost'] = g.myHost
        msg['lhsport'] = tempPort
        if currRank == 0:
            msg['host0'] = g.myHost
            msg['port0'] = tempPort
        manHost0 = msg['host0']
        manPort0 = msg['port0']
        if currRank == 0:
            jobid = str(g.nextJobInt) + '  ' + g.myId + '  ' + msg['jobalias']
            g.nextJobInt += 1
            msg['jobid'] = jobid
        else:
            jobid = msg['jobid']
        users = msg['users']
        for ranks in users.keys():
            if currRank >= lo  and  currRank <= hi:
                username = users[ranks]
                break
        execs = msg['execs']
        for ranks in execs.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgm = execs[ranks]
                break
        paths = msg['paths']
        for ranks in paths.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pathForExec = paths[ranks]
                break
        args = msg['args']
        for ranks in args.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmArgs = dumps(args[ranks])
                break
        envvars = msg['envvars']
        for ranks in envvars.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmEnvVars = dumps(envvars[ranks])
                break
        cwds = msg['cwds']
        for ranks in cwds.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                cwd = cwds[ranks]
                break
        manPid = fork()
        if manPid == 0:
            mpd_set_my_id('%s_man_%d' % (g.myHost,g.myPid) )
            g.myId = '%s_man_%d' % (g.myHost,g.myPid)
            for sock in g.activeSockets:
                sock.close()
            toManSocket.close()
            setpgrp()
            environ['MPDMAN_MYHOST'] = g.myHost
            environ['MPDMAN_JOBID'] = jobid
            environ['MPDMAN_CLI_PGM'] = pgm
            environ['MPDMAN_CLI_PATH'] = pathForExec
            environ['MPDMAN_PGM_ARGS'] = pgmArgs
            environ['MPDMAN_PGM_ENVVARS'] = pgmEnvVars
            environ['MPDMAN_CWD'] = cwd
            environ['MPDMAN_SPAWNED'] = str(msg['spawned'])
            environ['MPDMAN_NPROCS'] = str(msg['nprocs'])
            environ['MPDMAN_MPD_LISTEN_PORT'] = str(g.myPort)
            environ['MPDMAN_MPD_CONF_PASSWD'] = g.configParams['password']
            environ['MPDMAN_CONHOST'] = msg['conhost']
            environ['MPDMAN_CONPORT'] = str(msg['conport'])
            environ['MPDMAN_RANK'] = str(currRank)
            environ['MPDMAN_LHSHOST'] = manLhsHost
            environ['MPDMAN_LHSPORT'] = str(manLhsPort)
            environ['MPDMAN_HOST0'] = manHost0
            environ['MPDMAN_PORT0'] = str(manPort0)
            environ['MPDMAN_MY_LISTEN_PORT'] = str(tempPort)
            environ['MPDMAN_MY_LISTEN_FD'] = str(tempSocket.fileno())
            environ['MPDMAN_TO_MPD_FD'] = str(toMpdSocket.fileno())
            environ['MPDMAN_STDIN_GOES_TO_WHO'] = msg['stdin_goes_to_who']
            if msg.has_key('line_labels'):
                environ['MPDMAN_LINE_LABELS'] = '1'
            else:
                environ['MPDMAN_LINE_LABELS'] = '0'
            if msg.has_key('rship'):
                environ['MPDMAN_RSHIP'] = msg['rship']
                environ['MPDMAN_MSHIP_HOST'] = msg['mship_host']
                environ['MPDMAN_MSHIP_PORT'] = str(msg['mship_port'])
            if getuid() == 0:
                pwent = getpwnam(username)
                uid = pwent[2]
                gid = pwent[3]
                setgroups(mpd_get_groups_for_username(username))
                setregid(gid,gid)
                setreuid(uid,uid)
	    import atexit    # need to use full name of _exithandlers
	    atexit._exithandlers = []    # un-register handlers in atexit module
            # import profile
            # print 'profiling the manager'
            # profile.run('mpdman()')
            mpdman()
            exit(0)  # do NOT do cleanup    ## RMB TEMP DEBUG
        else:
            tempSocket.close()
            toMpdSocket.close()
            if not g.activeJobs.has_key(jobid):
                g.activeJobs[jobid] = {}
            g.activeJobs[jobid][manPid] = { 'pgm' : pgm, 'rank' : currRank,
                                            'username' : username,
                                            'clipid' : -1,    # until reported by man
                                            'socktoman' : toManSocket }
            _add_active_socket(toManSocket,'man_msgs','_handle_man_msgs',
                               'localhost',tempPort)
    mpd_print(0000, "FORWARDING MSG=:%s:" % msg)
    mpd_send_one_msg(g.rhsSocket,msg)  # forward it on around

def _handle_rhs_input():
    if g.allExiting:
        return
    msg = mpd_recv_one_msg(g.rhsSocket)
    if not msg:    # lost rhs; re-knit the ring
        del g.activeSockets[g.rhsSocket]
        g.rhsSocket.close()
        if g.activeSockets.has_key(g.lhsSocket):
            del g.activeSockets[g.lhsSocket]
            g.lhsSocket.close()
        mpd_print(0000, 're-entering the ring' )
        if g.entryHost:
            inRing = 0
            numTries = 5
            while not inRing  and  numTries > 0:
                rc = _enter_existing_ring()
                if rc < 0:    # fails if next g.generation <= current
                    sleep(2)
                else:
                    inRing = 1
                numTries -= 1
        else:
            _create_ring_of_one_mpd()
        return 1
    mpd_print(1, 'unexpected from rhs; msg=:%s:' % (msg) )
    return 0

def _handle_new_connection():
    randHiRange = 10000
    (newConnSocket,newConnAddr) = g.mySocket.accept()
    msg = mpd_recv_one_msg(newConnSocket)
    if (not msg) or \
       (not msg.has_key('cmd')) or (not msg.has_key('host')) or  \
       (not msg.has_key('port')):
        mpd_print(1, 'INVALID msg from new connection :%s: msg=:%s:' % (newConnAddr,msg) )
        newConnSocket.close()
        return
    if msg['cmd'] == 'request_to_enter_as_rhs':
        if msg['mpd_version'] != mpd_version:
            msgToSend = { 'cmd'          : 'entry_rejected_bad_mpd_version',
                          'your_version' : msg['mpd_version'],
                          'my_version'   : mpd_version }
            mpd_send_one_msg(newConnSocket,msgToSend)
            return
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        g.correctChallengeResponse[newConnSocket] = \
            new(''.join([g.configParams['password'],randNumStr])).digest()
        msgToSend = { 'cmd'        : 'challenge',
                      'randnum'    : randNumStr,
                      'g.generation' : g.generation }  # only send to rhs
        mpd_send_one_msg(newConnSocket,msgToSend)
        _add_active_socket(newConnSocket,'rhs_being_challenged',
                           '_handle_rhs_challenge_response',
                           msg['host'],msg['port'])
    elif msg['cmd'] == 'request_to_enter_as_lhs':
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        g.correctChallengeResponse[newConnSocket] = \
            new(''.join([g.configParams['password'],randNumStr])).digest()
        msgToSend = { 'cmd' : 'challenge',
                      'randnum' : randNumStr }
        mpd_send_one_msg(newConnSocket,msgToSend)
        _add_active_socket(newConnSocket,'lhs_being_challenged',
                           '_handle_lhs_challenge_response',
                           msg['host'],msg['port'])
    else:
        mpd_print(1, 'INVALID msg from new connection :%s: msg=:%s:' % (newConnAddr,msg) )
        newConnSocket.close()

def _handle_lhs_challenge_response(responseSocket):
    msg = mpd_recv_one_msg(responseSocket)
    if (not msg)   or  \
       (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
       (not msg.has_key('host'))  or  (not msg.has_key('port'))  or  \
       (msg['response'] != g.correctChallengeResponse[responseSocket]):
        mpd_print(1, 'INVALID msg for lhs response msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_response' }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    else:
        msgToSend = { 'cmd' : 'OK_to_enter_as_lhs' }
        mpd_send_one_msg(responseSocket,msgToSend)
        if g.activeSockets.has_key(g.lhsSocket):
            del g.activeSockets[g.lhsSocket]
            g.lhsSocket.close()
        g.lhsSocket = responseSocket
        g.lhsHost = msg['host']
        g.lhsPort = int(msg['port'])
        _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',g.lhsHost,g.lhsPort)

def _handle_rhs_challenge_response(responseSocket):
    msg = mpd_recv_one_msg(responseSocket)
    if (not msg)   or  \
       (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
       (not msg.has_key('host'))  or  (not msg.has_key('port'))  or  \
       (msg['response'] != g.correctChallengeResponse[responseSocket]):
        mpd_print(1, 'INVALID msg for rhs response msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_response' }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.correctChallengeResponse[responseSocket]
        del g.activeSockets[responseSocket]
        responseSocket.close()
    else:
        msgToSend = { 'cmd' : 'OK_to_enter_as_rhs',
                      'rhshost' : g.rhsHost,
                      'rhsport' : g.rhsPort }
        mpd_send_one_msg(responseSocket,msgToSend)
        del g.activeSockets[g.rhsSocket]
        g.rhsSocket.close()
        g.rhsSocket = responseSocket
        g.rhsHost = msg['host']
        g.rhsPort = int(msg['port'])
        _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',g.rhsHost,g.rhsPort)

def _handle_man_msgs(manSocket):
    msg = mpd_recv_one_msg(manSocket)
    if not msg:
        for jobid in g.activeJobs.keys():
	    deleted = 0
	    for manPid in g.activeJobs[jobid]:
		if manSocket == g.activeJobs[jobid][manPid]['socktoman']:
                    del g.activeJobs[jobid][manPid]
		    if len(g.activeJobs[jobid]) == 0:
		        del g.activeJobs[jobid]
		    deleted = 1
                    break
	    if deleted:
	        break
        del g.activeSockets[manSocket]
        manSocket.close()
        return
    if not msg.has_key('cmd'):
        mpd_print(1, 'INVALID msg for man request msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_msg' }
        mpd_send_one_msg(manSocket,msgToSend)
        del g.activeSockets[manSocket]
        manSocket.close()
        return
    if msg['cmd'] == 'client_pid':
        jobid = msg['jobid']
        manPid = msg['manpid']
        g.activeJobs[jobid][manPid]['clipid'] = msg['clipid']
    elif msg['cmd'] == 'spawn':
        msg['cmd'] = 'mpdrun'  # handle much like an mpdrun from a console
        msg['mpdid_mpdrun_start'] = g.myId
        msg['nstarted_on_this_loop'] = 0
        msg['first_loop'] = 1
        msg['jobalias'] = ''
        msg['stdin_goes_to_who'] = '0'
        if msg.has_key('try_0_locally'):
            _do_mpdrun(msg)
        else:
            mpd_send_one_msg(g.rhsSocket,msg)
        ## mpd_send_one_msg(manSocket, {'cmd' : 'mpdrun_ack', } )
    else:
        mpd_print(1, 'INVALID request from man msg=:%s:' % (msg) )
        msgToSend = { 'cmd' : 'invalid_request' }
        mpd_send_one_msg(manSocket,msgToSend)

def _add_active_socket(socket,name,handler,host,port):
    g.activeSockets[socket] = _ActiveSockInfo()
    g.activeSockets[socket].name    = name
    g.activeSockets[socket].handler = handler
    g.activeSockets[socket].rhsHost = host
    g.activeSockets[socket].rhsPort = port

def _enter_existing_ring():
    # connect to lhs
    g.lhsHost = g.entryHost
    g.lhsPort  = g.entryPort
    try:
        g.lhsSocket = mpd_get_inet_socket_and_connect(g.lhsHost,g.lhsPort)
    except Exception, errmsg:
        mpd_raise('unable to enter existing ring at: %s %d' % (g.entryHost,g.entryPort))
    _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',g.lhsHost,g.lhsPort)
    msgToSend = { 'cmd' : 'request_to_enter_as_rhs',
                  'host' : g.myHost,
                  'port' : g.myPort,
                  'mpd_version' : mpd_version }
    mpd_send_one_msg(g.lhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.lhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or  \
       (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')) or  \
       (not msg.has_key('g.generation')):
        mpd_raise('invalid challenge msg: %s' % (msg) )
    g.generationFromMsg = int(msg['g.generation'])
    if g.generationFromMsg > g.generation:
        g.generation = g.generationFromMsg
    else:
        del g.activeSockets[g.lhsSocket]
        g.lhsSocket.close()
        return -1
    response = new(''.join([g.configParams['password'],msg['randnum']])).digest()
    msgToSend = { 'cmd' : 'challenge_response',
                  'response' : response,
                  'host' : g.myHost,
                  'port' : g.myPort }
    mpd_send_one_msg(g.lhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.lhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or (msg['cmd'] != 'OK_to_enter_as_rhs'):
        mpd_raise('NOT OK to enter ring')
    if (not msg.has_key('rhshost'))  or (not msg.has_key('rhsport')):
        mpd_raise('invalid OK msg: %s' % (msg) )
    g.rhsHost = msg['rhshost']
    g.rhsPort = int(msg['rhsport'])
    # connect to rhs
    g.rhsSocket = mpd_get_inet_socket_and_connect(g.rhsHost,g.rhsPort)
    _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',g.rhsHost,g.rhsPort)
    msgToSend = { 'cmd' : 'request_to_enter_as_lhs',
                  'host' : g.myHost,
                  'port' : g.myPort }
    mpd_send_one_msg(g.rhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.rhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or  \
       (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
        mpd_raise('failed to recv challenge from rhs; msg=:%s:' % (msg) )
    response = new(''.join([g.configParams['password'],msg['randnum']])).digest()
    msgToSend = { 'cmd' : 'challenge_response',
                  'response' : response,
                  'host' : g.myHost,
                  'port' : g.myPort }
    mpd_send_one_msg(g.rhsSocket,msgToSend)
    msg = mpd_recv_one_msg(g.rhsSocket)
    if (not msg) or  \
       (not msg.has_key('cmd')) or  \
       (msg['cmd'] != 'OK_to_enter_as_lhs'):
        mpd_raise('NOT OK to enter ring; msg=:%s:' % (msg) )
    return 0

def _create_ring_of_one_mpd():
    # use a temp port and socket to avoid accidentally 
    #   accepting/handling connections by others
    (tempSocket,tempPort) = mpd_get_inet_listen_socket('',0)
    g.lhsSocket = mpd_get_inet_socket_and_connect(g.myHost,tempPort)
    g.lhsHost = g.myHost
    g.lhsPort = g.myPort
    _add_active_socket(g.lhsSocket,'lhs','_handle_lhs_input',g.lhsHost,g.lhsPort)
    (g.rhsSocket,addr) = tempSocket.accept()
    g.rhsHost = g.myHost
    g.rhsPort = g.myPort
    _add_active_socket(g.rhsSocket,'rhs','_handle_rhs_input',g.rhsHost,g.rhsPort)
    tempSocket.close()
    g.generation += 1

def _process_configfile_params():
    if getuid() == 0:    # if ROOT
        configFilename = '/etc/mpd.conf'
    else:
        configFilename = environ['HOME'] + '/.mpd.conf'
    try:
        mode = stat(configFilename)[0]
    except:
        mode = ''
    if not mode:
        # mpd_raise('%s: config file not found' % (configFilename) )
        print 'configuration file %s not found' % (configFilename)
        exit(0)
    if  (mode & 0x3f):
        # mpd_raise('%s: config file accessible by others' % (configFilename) )
        print 'configuration file %s is accessible by others' % (configFilename)
        print 'change permissions to allow read and write access only by you'
        exit(0)
    configFile = open(configFilename,'r')
    g.configParams = {}
    for line in configFile:
        line = line.rstrip()
        withoutComments = line.split('#')[0]
        splitLine = withoutComments.split('=')
        if len(splitLine) == 2:
            g.configParams[splitLine[0]] = splitLine[1]
        else:
            mpd_print(0, 'skipping config file line = :%s:' % (line) )
    if 'password' not in g.configParams.keys():
        mpd_raise('%s: configFile has no password' % (configFilename) )

def _process_cmdline_args():
    g.entryHost    = ''
    g.entryPort    = 0
    g.tracingMPD   = 0
    g.allowConsole = 1
    g.echoPortNum  = 0
    g.daemon       = 0
    g.bulletproof  = 0
    if g.configParams.has_key('idmyhost'):
        g.myHost   = g.configParams['idmyhost']
    else:
        g.myHost   = gethostname()
    try:
        (opts,args) = getopt(argv[1:],
                             'h:p:i:tnedb',
                             ['host=','port=','idmyhost=','trace','noconsole','echo',
                              'daemon','bulletproof'])
    except:
        usage()

    for opt in opts:
        if   opt[0] == '-h'  or  opt[0] == '--host':
            g.entryHost = opt[1]
        elif opt[0] == '-p'  or  opt[0] == '--port':
            g.entryPort = int(opt[1])
        elif opt[0] == '-i'  or  opt[0] == '--idmyhost':
            g.myHost = opt[1]
        elif opt[0] == '-t'  or  opt[0] == '--trace':
            g.tracingMPD = 1
        elif opt[0] == '-n'  or  opt[0] == '--noconsole':
            g.allowConsole = 0
        elif opt[0] == '-e'  or  opt[0] == '--echo':
            g.echoPortNum = 1 
        elif opt[0] == '-d'  or  opt[0] == '--daemon':
            g.daemon = 1 
        elif opt[0] == '-b'  or  opt[0] == '--bulletproof':
            g.bulletproof = 1 
        else:
            pass    ## getopt raises an exception if not recognized
    if (g.entryHost and not g.entryPort) or (not g.entryHost and g.entryPort):
        mpd_raise('host and port must be specified together')

def sigchld_handler(signum,frame):
    done = 0
    while not done:
        try:
            (pid,status) = waitpid(-1,WNOHANG)
	    if pid == 0:    # no existing child process is finished
	        done = 1
        except:    # no more child processes to be waited for
            done = 1

def usage():
    print 'mpd version %s' % str(mpd_version)
    print 'usage: %s -h -p -t -n -e -d -b -i' % argv[0]
    print '   or: %s --host --port --trace --noconsole --echo --daemon --bulletproof --idmyhost' % argv[0]
    print 'host and port must be specified together and tell where to enter a ring;'
    print '  if they are not coded, the mpd forms a stand-alone ring that other mpds'
    print '  may enter later'
    print 'trace yields lots of traces thru mpd routines; not generally useful'
    print 'noconsole is useful for running 2 mpds on the same machine; only one of'
    print '  them can have a unix socket which a console program can connect to'
    print 'echo says to echo the listener port for the mpd; useful in scripts'
    print 'daemon causes mpd to run backgrounded, with no controlling tty'
    print 'bulletproof says to turn bulletproofing on (experimental)'
    print 'idmyhost specifies an alternate hostname for the host this mpd is running on'
    print '.mpd.conf file must be present in home directory with read and write access' 
    print '  only for user, and must contain at least a line with password=<password>'

    exit(-1)

def _cleanup():
    try:
        mpd_print(0, "CLEANING UP" )
        if g.conListenSocket in g.activeSockets:    # only delete if I put it there
            unlink(g.conListenName)
        closelog()
    except:
        pass


if __name__ == '__main__':
    try:
        g.myId = gethostname() + '_no_port_yet' + '_' + `getpid()`  # chgd later
        mpd_check_python_version()
        mpd_set_my_id(g.myId)  # chgd later
        proceduresToTrace = []
        for (symbol,symtype) in globals().items():
            if type(symtype) == FunctionType:
                proceduresToTrace.append(symbol)
        mpd_set_procedures_to_trace(proceduresToTrace)
        _process_configfile_params()
        _process_cmdline_args()
        if g.tracingMPD:
            settrace(mpd_trace_calls)
        register(_cleanup)
    
        _mpd_init()

        if g.bulletproof:
            try:
                from threading import Thread
            except:
                print '*** mpd terminating'
                print '    bulletproof option must be able to import threading-Thread'
                exit(-1)
            # may use SIG_IGN on all but SIGCHLD and SIGHUP (handled above)
            while 1:
                mpdtid = Thread(target=_mpd)
                mpdtid.start()
                mpdtid.join()   # only come out if exiting or thread fails
                if g.allExiting:
                    break
                if g.conSocket:
                    if g.activeSockets[g.conSocket]:
                        msgToSend = { 'cmd' : 'restarting_mpd' }
                        mpd_send_one_msg(g.conSocket,msgToSend)
                        del g.activeSockets[g.conSocket]
                    g.conSocket.close()
                    g.conSocket = 0
        else:
            #    import profile
            #    profile.run('_mpd()')
            _mpd()
    except mpdError, errmsg:
        print '%s failed ; cause: %s' % (g.myId,errmsg)
