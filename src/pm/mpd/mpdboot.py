#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from os     import environ, system, path
from getopt import getopt
from sys    import argv, exit
from popen2 import Popen3
from socket import gethostname
from select import select, error
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError, mpd_same_ips

def mpdboot():
    myOwnDir  = path.abspath(path.split(argv[0])[0])
    rshCmd    = 'ssh'
    user      = mpd_get_my_username()
    mpdCmd    = path.join(myOwnDir,'mpd')
    hostsFile = 'mpd.hosts'
    totalNum  = 1
    debug     = 0
    verbosity = 0
    localConsoleVal  = ''
    remoteConsoleVal = ''
    oneLocal = 1
    entryHost = ''
    entryPort = ''
    numBoots = 1    # including this one
    mpdHostsFromHere  = []
    bootHostsFromHere = []
    try:
        shell = path.split(environ['SHELL'])[-1]
    except:
        shell = 'csh'

    try:
        (opts, args) = getopt(argv[1:], 'hf:r:u:m:n:dsv1e:z:',
                              ['help', 'file=', 'rsh=', 'user=', 'mpd=', 'totalnum=',
                               'entry=',
                               'loccons', 'remcons', 'shell', 'verbose'])
    except:
        usage()
    else:
        for opt in opts:
            if   opt[0] == '-h' or opt[0] == '--help':
                usage()
            elif opt[0] == '-e' or opt[0] == '--entry':
                (entryHost,entryPort) = opt[1].split(':')
                entryHost = '-h ' + entryHost
                entryPort = '-p ' + entryPort
            elif opt[0] == '-z':
                if opt[1].isdigit():
                    numBoots = int(opt[1])
                else:
                    numBoots = 0
		    mpdHostsFromHere = opt[1].split(',')
            elif opt[0] == '-r' or opt[0] == '--rsh':
                rshCmd = opt[1]
            elif opt[0] == '-u' or opt[0] == '--user':
                user   = opt[1]
            elif opt[0] == '-m' or opt[0] == '--mpd':
                mpdCmd = opt[1]
            elif opt[0] == '-f' or opt[0] == '--file':
                hostsFile = opt[1]
            elif opt[0] == '-n' or opt[0] == '--totalnum':
                totalNum = int(opt[1])
            elif opt[0] == '-d' or opt[0] == '--debug':
                debug = 1
            elif opt[0] == '-s' or opt[0] == '--shell':
                shell = 'bourne'
            elif opt[0] == '-v' or opt[0] == '--verbose':
                verbosity = 1
            elif opt[0] == '-1':
                oneLocal = 0
            elif opt[0] == '--loccons':
                localConsoleVal  = '-n'
            elif opt[0] == '--remcons':
                remoteConsoleVal = '-n'
    if args:
        print 'unrecognized arguments:', ' '.join(args)
        usage()

    myHost = gethostname()
    if numBoots:
        try:
            f = open(hostsFile,'r')
            hosts  = f.readlines()
        except:
            print 'unable to open (or read) hostsfile %s' % hostsFile
            usage()
        hosts = [ x.strip() for x in hosts if x[0] != '#' ]
        hosts = [ x  for x in hosts if x != '' ]    # delete empty lines
        if oneLocal:
            hosts = [ x for x in hosts if not mpd_same_ips(x,myHost) ]
            # hosts = [ x for x in hosts if x != myHost ]
        if len(hosts) < (totalNum-1):    # one is local
            print 'there are not enough hosts specified on which to start all processes'
            exit(-1)
        bootHostsFromHere = hosts[0:numBoots-1]
	del hosts[0:numBoots-1]
	numMPDsPerBoot = (totalNum - numBoots) / numBoots

    if debug:
        print 'cmd=:%s %s %s %s -e: (executed on %s)' % (mpdCmd, localConsoleVal, entryHost, entryPort, myHost)
    if verbosity == 1:
        print 'starting local mpd on %s' % (myHost)
    locMPD = Popen3('%s %s %s %s -d -e' % (mpdCmd, localConsoleVal, entryHost, entryPort), 1)
    numStarted = 1
    myPort = locMPD.fromchild.readline().strip()
    try:
        (readyFDs,unused1,unused2) = select([locMPD.fromchild],[],[],1)
    except error, errmsg:
        mpd_raise('mpdboot: select failed: errmsg=:%s:' % (errmsg) )
    if locMPD.fromchild in readyFDs:
        ## print 'local mpd port: %s' % (myPort)
        for line in locMPD.fromchild.readlines():
            print line,
        for line in locMPD.childerr.readlines():
            print line,
        ## exit(-1)

    if rshCmd == 'ssh':
        xOpt = '-x'
    else:
        xOpt = ''

    redirect = {
        'sh'     :  ' > /dev/null 2>&1 ',
        'ksh'    :  ' > /dev/null 2>&1 ',
        'csh'    :  ' >& /dev/null ',
        'tcsh'   :  ' >& /dev/null ',
        'bash'   :  ' > /dev/null 2>&1 ',
        'bourne' :  ' > /dev/null 2>&1 '
        }       
    shellRedirect = redirect.get(shell, ' >& /dev/null ')

    mpdbootPathName = path.join(myOwnDir,'mpdboot.py')

    for host in bootHostsFromHere:
	## may be short for last one
	hostSet = ','.join(hosts[0:numMPDsPerBoot])
	del hosts[0:numMPDsPerBoot]
	if not hostSet:
	    hostSet = 0
        cmd = "%s %s %s -n '%s -r %s -m %s -n %d -e %s:%s -z %s </dev/null %s' & " % \
              (rshCmd, xOpt, host,
	       mpdbootPathName,
	       rshCmd,
	       mpdCmd,
	       numMPDsPerBoot+1,
               myHost, myPort, hostSet, shellRedirect)
        if debug:
            print 'cmd=:%s:' % (cmd)
        if verbosity == 1:
            print 'starting remote mpd on %s' % (host)
        status = system(cmd)
        assert status is 0, '%s bombed with status %d' % (cmd,status)
        numStarted = numStarted + numMPDsPerBoot + 1 

    if numBoots  and  numStarted < totalNum:
        numLeftToStart = totalNum - numStarted
        mpdHostsFromHere = hosts[0:numLeftToStart]
        del hosts[0:numLeftToStart]

    for host in mpdHostsFromHere:
        if numStarted >= totalNum:
            break
        # cmd = "%s %s %s -n '%s    %s -h %s -p %s </dev/null %s &'" % \
        cmd = "%s %s %s -n '%s -d %s -h %s -p %s </dev/null %s' & " % \
              (rshCmd, xOpt, host, mpdCmd, remoteConsoleVal,
               myHost, myPort, shellRedirect)
        if debug:
            print 'cmd=:%s:' % (cmd)
        if verbosity == 1:
            print 'starting remote mpd on %s' % (host)
        status = system(cmd)
        assert status is 0, '%s bombed with status %d' % (cmd,status)
        numStarted += 1


def usage():
    print ''
    print 'mpdboot [-h] [-f <hostsfile>] [-r <rshcmd>] [-u <user>] [-m <mpdcmd>] [-n n_to_start] -s -v -1'
    print 'Long options:'
    print '  --help --file=<hostsfile> --rsh=<rshcmd> --user=<user> --mpd=<mpdcmd> --totalnum=<n_to_start> --loccons --remcons --shell --verbose'
    print """
mpdboot starts one mpd locally and (n_to_start - 1) others as computed
from the -n (--totalnum) option; at least the one local mpd will be
started by default; the machines to use are specified by the --file
option (default is mpd.hosts).  You may find it useful to specify the
full pathname of mpd on remote hosts (-r) if it is not in your path on
the remote machines.  The --loccons and --remcons options indicate that
you do NOT want a console available on local and remote mpds,
respectively.  The -s option allows you to indicate that Bourne shell is
your default shell for rsh.  The -1 option indicates that you want to
start mpd's on all the nodes in the file as well as one on the local
machine even if the local machine occurs there and thus the result would
be two mpd's on the local machine.  Verbose mode (-v or --verbose)
causes the rsh attempts to be printed as they occur.  It does not
provide confirmation that the rsh's were successful.  The -z N option permits
you to parallelize the startup of mpds by starting N copies of mpdboot itself.
"""
    exit(-1)
    
if __name__ == '__main__':
    try:
        mpdboot()
    except SystemExit, errmsg:
        pass
    except mpdError, errmsg:
        print 'mpdboot failed: %s' % (errmsg)
