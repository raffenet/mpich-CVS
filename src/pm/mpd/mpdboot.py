#!/usr/bin/env python

from os     import environ, system
from getopt import getopt
from sys    import argv, exit
from popen2 import Popen3
from socket import gethostname
from select import select, error
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError

def mpdboot():
    rshCmd    = 'ssh'
    user      = mpd_get_my_username()
    mpdCmd    = 'mpd'
    hostsFile = 'mpd.hosts'
    totalNum  = 1
    debug     = 0
    verbosity = 0
    localConsoleVal  = ''
    remoteConsoleVal = ''
    shell = 'csh'
    oneLocal = 1
    try:
	(opts, args) = getopt(argv[1:], 'hf:r:u:m:n:dsv1',
			      ['help', 'file=', 'rsh=', 'user=', 'mpd=', 'totalnum=',
			       'loccons', 'remcons', 'shell', 'verbose'])
    except:
	usage()
    else:
	for opt in opts:
	    if   opt[0] == '-h' or opt[0] == '--help':
		usage()
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

    try:
	f = open(hostsFile,'r')
        hosts  = f.readlines()
    except:
	print 'unable to open (or read) hostsfile %s' % hostsFile
	usage()

    myHost = gethostname()
    if debug:
        print 'cmd=:%s %s -e: (executed on %s)' % (mpdCmd, localConsoleVal, myHost)
    if verbosity == 1:
        print 'starting local mpd on %s' % (myHost)
    locMPD = Popen3('%s %s -e' % (mpdCmd, localConsoleVal), 1)
    numStarted = 1
    myPort = locMPD.fromchild.readline().strip()
    try:
        (readyFDs,None,None) = select([locMPD.fromchild],[],[],1)
    except error, errmsg:
	mpd_raise('mpdboot: select failed: errmsg=:%s:' % (errmsg) )
    if locMPD.fromchild in readyFDs:
	print myPort
	for line in locMPD.fromchild.readlines():
	    print line,
	for line in locMPD.childerr.readlines():
	    print line,
	exit(-1)

    if rshCmd == 'ssh':
	xOpt = '-x'
    else:
	xOpt = ''

    for host in hosts:
	if numStarted == totalNum:
	    break
	host = host.strip()
	if oneLocal and host == myHost:
            continue
	if host[0] != '#':                    # ignore comment lines
            shellRedirect = ' >& /dev/null '  # default
            if shell == 'bourne':
                shellRedirect = ' > /dev/null 2>&1 '
            # cmd = "%s %s %s -n '%s    %s -h %s -p %s </dev/null %s &'" % \
	    cmd = "%s %s %s -n '%s -d %s -h %s -p %s </dev/null %s' & " % \
                  (rshCmd, xOpt, host, mpdCmd, remoteConsoleVal,
                   myHost, myPort, shellRedirect)
            if debug:
                print 'cmd=:%s:' % (cmd)
            if verbosity == 1:
                print 'starting remote mpd on %s' % (host)
	    system(cmd)
	    numStarted += 1

    if numStarted < totalNum:
	print ("%s only contained enough hosts to start a total of %d mpd's," + \
	      " which were started") % (hostsFile, numStarted)

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
provide confirmation that the rsh's were successful.
"""
    exit(-1)
    
if __name__ == '__main__':
    try:
        mpdboot()
    except SystemExit, errmsg:
        pass
    except mpdError, errmsg:
        print 'mpdboot failed: %s' % (errmsg)
