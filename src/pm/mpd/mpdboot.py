#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from os     import environ, system, path, kill, access, X_OK
from sys    import argv, exit, stdout
from popen2 import Popen4, popen2
from socket import gethostname, gethostbyname_ex
from select import select, error
from signal import SIGKILL
from re     import findall
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError, \
                   mpd_same_ips, mpd_get_ranks_in_binary_tree, mpd_print, \
                   mpd_get_inet_socket_and_connect, mpd_send_one_msg, mpd_recv_one_msg

global myHost, fullDirName, topMPDBoot, user

def mpdboot():
    global myHost, fullDirName, topMPDBoot, user
    mpd_set_my_id('mpdboot_rank_notset')
    fullDirName  = path.abspath(path.split(argv[0])[0])
    rshCmd     = 'ssh'
    user       = mpd_get_my_username()
    mpdCmd     = path.join(fullDirName,'mpd')
    mpdbootCmd = path.join(fullDirName,'mpdboot')
    hostsFile  = 'mpd.hosts'
    totalNum   = 1    # may get chgd below
    debug      = 0
    verbosity  = 0
    localConsoleArg  = ''
    remoteConsoleArg = ''
    myConsoleVal = ''
    oneMPDPerHost = 1
    entryHost = ''
    entryPort = ''
    topMPDBoot = 1
    myHost = gethostname()
    try:
        shell = path.split(environ['SHELL'])[-1]
    except:
        shell = 'csh'

    argidx = 1    # skip arg 0
    while argidx < len(argv):
        if   argv[argidx] == '-h' or argv[argidx] == '--help':
            usage()
        elif argv[argidx] == '-e' or argv[argidx] == '--entry':
            if ':' not in argv[argidx+1]:
                print 'invalid pair of entry host and entry port for -e option'
                usage()
            (entryHost,entryPort) = argv[argidx+1].split(':')
            try:
                ip = gethostbyname_ex(entryHost)[2]    # may fail if invalid host
            except:
                print 'invalid entry host ', entryHost ; stdout.flush()
                usage()
            if not entryPort.isdigit():
                print 'invalid (nonumeric) entry port ', entryPort ; stdout.flush()
                usage()
            entryHost = entryHost
            entryPort = entryPort
            argidx += 2
        elif argv[argidx] == '-zrank':
            topMPDBoot = 0
            myBootRank = int(argv[argidx+1])
            argidx += 2
        elif argv[argidx] == '-zhosts':
	    hosts = argv[argidx+1]
	    hosts = hosts.split(',')
            argidx += 2
        elif argv[argidx] == '-r' or argv[argidx] == '--rsh':
            rshCmd = argv[argidx+1]
            argidx += 2
        elif argv[argidx] == '-u' or argv[argidx] == '--user':
            user   = argv[argidx+1]
            argidx += 2
        elif argv[argidx] == '-m' or argv[argidx] == '--mpd':
            mpdCmd = argv[argidx+1]
            argidx += 2
        elif argv[argidx] == '-f' or argv[argidx] == '--file':
            hostsFile = argv[argidx+1]
            argidx += 2
        elif argv[argidx] == '-n' or argv[argidx] == '--totalnum':
            totalNum = int(argv[argidx+1])
            argidx += 2
        elif argv[argidx] == '-d' or argv[argidx] == '--debug':
            debug = 1
            argidx += 1
        elif argv[argidx] == '-s' or argv[argidx] == '--shell':
            shell = 'bourne'
            argidx += 1
        elif argv[argidx] == '-v' or argv[argidx] == '--verbose':
            verbosity = 1
            argidx += 1
        elif argv[argidx] == '-1':
            oneMPDPerHost = 0
            argidx += 1
        elif argv[argidx] == '--loccons':
            localConsoleArg  = '--loccons'
            argidx += 1
        elif argv[argidx] == '--remcons':
            remoteConsoleArg = '--remcons'
            argidx += 1
        else:
            print 'unrecognized argument:', argv[argidx]
            usage()

    if topMPDBoot:
        if totalNum > 1:
            try:
                f = open(hostsFile,'r')
                lines  = f.readlines()
            except:
                print 'unable to open (or read) hostsfile %s' % (hostsFile)
                exit(-1)
        else:
            lines = []
        hosts = [myHost]
        for host in lines:
            host = host.strip()
            if host != ''  and  host[0] != '#':
                hosts.append(host)
        if oneMPDPerHost  and  totalNum > 1:
	    oldHosts = hosts[:]
	    hosts = []
	    for x in oldHosts:
	       keep = 1
	       for y in hosts:
	           if mpd_same_ips(x,y):
		       keep = 0
		       break
	       if keep:
	           hosts.append(x)
        if len(hosts) < totalNum:    # one is local
            print 'totalNum=%d  num hosts=%d' % (totalNum,len(hosts))
            print 'there are not enough hosts on which to start all processes'
            exit(-1)
        myBootRank = 0
        if localConsoleArg:
            myConsoleVal = '-n'
    else:
        if remoteConsoleArg:
            myConsoleVal = '-n'
    anMPDALreadyHere = 0
    for i in range(myBootRank):
        if mpd_same_ips(hosts[i],myHost):    # if one before me on this host
            myConsoleVal = '-n'
	    anMPDALreadyHere = 1
	    break
    if not anMPDALreadyHere:
        try:
            system('%s/mpdallexit > /dev/null' % (fullDirName))  # stop any current mpds
        except:
            pass

    mpd_set_my_id('mpdboot_rank_%d' % (myBootRank) )
    if debug:
        mpd_print(1, 'starting')
    (parent,lchild,rchild) = mpd_get_ranks_in_binary_tree(myBootRank,totalNum)
    if debug:
        mpd_print(1, 'p=%d l=%d r=%d' % (parent,lchild,rchild) )

    if entryHost:
        cmd = '%s %s -h %s -p %s -d -e' % (mpdCmd,myConsoleVal,entryHost,entryPort)
    else:
        cmd = '%s %s -d -e' % (mpdCmd,myConsoleVal)
    if verbosity:
        mpd_print(1,'starting local mpd on %s' % (myHost) )
    if debug:
        mpd_print(1, 'cmd to run local mpd = :%s:' % (cmd) )

    if not access(mpdCmd,X_OK):
        err_exit('invalid mpd cmd')
    locMPD = Popen4(cmd, 0)
    locMPDFD = locMPD.fromchild
    locMPDPort = locMPDFD.readline().strip()
    if locMPDPort.isdigit():
	# can't do this until he's already in his ring
        locMPDSocket = mpd_get_inet_socket_and_connect(myHost,int(locMPDPort))
        if locMPDSocket:
            msgToSend = { 'cmd' : 'ping', 'host' : 'ping', 'port' : 0} # dummy host & port
            mpd_send_one_msg(locMPDSocket, { 'cmd' : 'ping', 'host' : myHost, 'port' : 0} )
            msg = mpd_recv_one_msg(locMPDSocket)    # RMB: WITH TIMEOUT ??
            if not msg  or  not msg.has_key('cmd')  or  msg['cmd'] != 'ping_ack':
                err_exit('invalid msg from mpd :%s:' % (msg) )
            locMPDSocket.close()
        else:
            err_exit('failed to connect to mpd' )
    else:
        err_exit('did not get a valid port from mpd' % (myBootRank) )

    if not entryHost:
        entryHost = myHost
        entryPort = locMPDPort

    if rshCmd == 'ssh':
        xOpt = '-x'
    else:
        xOpt = ''

    lfd = 0
    rfd = 0
    fdsToSelect = []
    if debug:
        debugArg = '-d'
    else:
        debugArg = ''
    if verbosity:
        verboseArg = '-v'
    else:
        verboseArg = ''
    if lchild >= 0:
        cmd = "%s %s %s -n '%s -r %s -m %s -n %d %s %s -e %s:%s %s -zrank %s -zhosts %s </dev/null ' " % \
              (rshCmd, xOpt, hosts[lchild], mpdbootCmd, rshCmd, mpdCmd, totalNum,
               debugArg, verboseArg, entryHost, entryPort, remoteConsoleArg, lchild,
	       ','.join(hosts) )
        if verbosity:
            mpd_print(1, 'starting remote mpd on %s' % (hosts[lchild]) )
        if debug:
            mpd_print(1, 'cmd to run lchild boot = :%s:' % (cmd) )
        lchildMPDBoot = Popen4(cmd, 0)
        lfd = lchildMPDBoot.fromchild
        fdsToSelect.append(lfd)
    if rchild >= 0:
        cmd = "%s %s %s -n '%s -r %s -m %s -n %d %s %s -e %s:%s %s -zrank %s -zhosts %s </dev/null ' " % \
              (rshCmd, xOpt, hosts[rchild], mpdbootCmd, rshCmd, mpdCmd, totalNum,
               debugArg, verboseArg, entryHost, entryPort, remoteConsoleArg, rchild,
	       ','.join(hosts) )
        if verbosity:
            mpd_print(1, 'starting remote mpd on %s' % (hosts[rchild]) )
        if debug:
            mpd_print(1, 'cmd to run rchild boot = :%s:' % (cmd) )
        rchildMPDBoot = Popen4(cmd, 0)
        rfd = rchildMPDBoot.fromchild
        fdsToSelect.append(rfd)

    while fdsToSelect:
        try:
            (readyFDs,unused1,unused2) = select(fdsToSelect,[],[],0.1)
        except error, errmsg:
            mpd_raise('mpdboot: select failed: errmsg=:%s:' % (errmsg) )
        if lfd  and  lfd in readyFDs:
            line = lfd.readline()
            if line:
                if line.find('RC=MPDBOOT_ERREXIT') >= 0:
                    err_exit('RC=MPDBOOT_ERREXIT')
                else:
                    print line, ; stdout.flush()
            else:
                lfd.close()
                fdsToSelect.remove(lfd)
        if rfd  and  rfd in readyFDs:
            line = rfd.readline()
            if line:
                if line.find('RC=MPDBOOT_ERREXIT') >= 0:
                    err_exit('RC=MPDBOOT_ERREXIT')
                else:
                    print line, ; stdout.flush()
            else:
                rfd.close()
                fdsToSelect.remove(rfd)


def err_exit(msg):
    global myHost, fullDirName, topMPDBoot, user
    mpd_print(1, 'mpd failed to start correctly on %s' % (myHost) )
    mpdPid = 0
    if msg != 'RC=MPDBOOT_ERREXIT':
        print '  reason: %s' % (msg) ; stdout.flush()
        try:
            logfile = open('/tmp/mpd2.logfile_%s' % (user),'r')
            mpd_print(1, '  contents of mpd logfile in /tmp:')
            for line in logfile:
                print '    ', line, ; stdout.flush()
	        if line.startswith('logfile for mpd with pid'):
	            mpdPid = findall(r'logfile for mpd with pid (\d+)',line)
        except:
            pass
        try:
            system('%s/mpdallexit > /dev/null' % (fullDirName))  # stop any current mpds
        except:
             pass
        try:
            kill(mpdPid,SIGKILL)
        except:
             pass
        if not topMPDBoot:
            mpd_print(1, 'RC=MPDBOOT_ERREXIT')    # printable rc
    exit(-1)


def usage():
    print 'usage:  mpdboot --totalnum=<n_to_start> [--file=<hostsfile>]  [--help] \ '
    print '                [--rsh=<rshcmd>] [--user=<user>] [--mpd=<mpdcmd>] \ '
    print '                [--loccons] [--remcons] [--shell] [--verbose] [-1]'
    print ' or, in short form, '
    print '        mpdboot -n n_to_start [-f <hostsfile>] [-h] [-r <rshcmd>] [-u <user>] \ '
    print '                [-m <mpdcmd>]  -s -v [-1]'
    print ''
    print '--totalnum specifies the total number of mpds to start; at least'
    print '  one mpd will be started locally, and others on the machines specified'
    print '  by the file argument; by default, only one mpd per host will be'
    print '  started even if the hostname occurs multiple times in the hosts file'
    print '-1 means remove the restriction of starting only one mpd per machine; '
    print '  in this case, at most the first mpd on a host will have a console'
    print '--file specifies the file of machines to start the rest of the mpds on;'
    print '  it defaults to mpd.hosts'
    print '--mpd specifies the full path name of mpd on the remote hosts if it is'
    print '  not in your path'
    print '--rsh specifies the name of the command used to start remote mpds; it'
    print '  defaults to ssh; an alternative is rsh'
    print '--shell says that the Bourne shell is your default for rsh' 
    print '--verbose shows the ssh attempts as they occur; it does not provide'
    print '  confirmation that the sshs were successful'
    print '--loccons says you do not want a console available on local mpd(s)'
    print '--remcons says you do not want consoles available on remote mpd(s)'
    stdout.flush()
    exit(-1)

    
if __name__ == '__main__':
    try:
        mpdboot()
    except SystemExit, errmsg:
        pass
    except mpdError, errmsg:
        print 'mpdboot failed: %s' % (errmsg)
