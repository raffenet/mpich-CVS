#!/usr/bin/env python

## NOTE: we do NOT allow this pgm to run via mpdroot

from sys    import argv, exit
from os     import environ, system
from getopt import getopt
from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_raise, mpdError

def mpdcleanup():
    rshCmd    = 'ssh'
    user      = mpd_get_my_username()
    cleanCmd  = '/bin/rm -f '
    hostsFile = ''
    try:
	(opts, args) = getopt(argv[1:], 'hf:r:u:c:', ['help', 'file=', 'rsh=', 'user=', 'clean='])
    except:
	mpdcleanup_usage()
        mpd_raise('invalid arg(s) specified')
    else:
	for opt in opts:
	    if opt[0] == '-r' or opt[0] == '--rsh':
		rshCmd = opt[1]
	    elif opt[0] == '-u' or opt[0] == '--user':
		user   = opt[1]
	    elif opt[0] == '-f' or opt[0] == '--file':
		hostsFile = opt[1]
	    elif opt[0] == '-h' or opt[0] == '--help':
		mpdcleanup_usage()
	    elif opt[0] == '-c' or opt[0] == '--clean':
		cleanCmd = opt[1]
    if args:
	mpdcleanup_usage()
        mpd_raise('invalid arg(s) specified: ' + ' '.join(args) )

    cleanFile = '/tmp/mpd2.console_%s' % (user)
    system( '%s %s' % (cleanCmd,cleanFile) )
    if rshCmd == 'ssh':
	xOpt = '-x'
    else:
	xOpt = ''

    if hostsFile:
        try:
	    f = open(hostsFile,'r')
        except:
	    print 'Not cleaning up on remote hosts; file %s not found' % hostsFile
	    exit(0)
        hosts  = f.readlines()
        for host in hosts:
	    host = host.strip()
	    if host[0] != '#':
	        cmd = '%s %s -n %s %s %s &' % (rshCmd, xOpt, host, cleanCmd, cleanFile)
	        # print 'cmd=:%s:' % (cmd)
	        system(cmd)

def mpdcleanup_usage():
    print 'usage: mpdcleanup', '[-f <hostsfile>] [-r <rshcmd>] [-u <user>] [-c <cleancmd>] or'
    print '   or: mpdcleanup', '[--file=<hostsfile>] [--rsh=<rshcmd>] [-user=<user>] [-clean=<cleancmd>]'
    print 'Removes the Unix socket on local (the default) and remote machines'
    print 'This is useful in case the mpd crashed badly and did not remove it, which it normally does'
    exit(0)


if __name__ == '__main__':
    try:
        mpdcleanup()
    except SystemExit, errmsg:
        pass
    except mpdError, errmsg:
	print 'mpdcleanup failed: %s' % (errmsg)
