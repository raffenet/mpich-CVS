#!/usr/bin/env python

def mpdhelp():
    print """
The following mpd commands are available.  For usage of any specific one,
invoke it with the single argument --help .  If the commands have not been
installed yet, it is necessary to invoke them with the .py suffix.

mpd           start an mpd daemon
mpdtrace      show all mpd's in ring
mpdboot       start a ring of daemons all at once
mpdringtest   test how long it takes for a message to circle the ring 
mpdallexit    take down all daemons in ring
mpdcleanup    repair local Unix socket if ring crashed badly
mpdrun        start a parallel job
mpdlistjobs   list processes of jobs (-a or --all: all jobs for all users)
mpdkilljob    kill all processes of a single job
mpdsigjob     deliver a specific signal to the applications processes of a job

Each command can be invoked with the --help argument, which prints usage
information for the command without running it.
"""

if __name__ == '__main__':
    try:
        mpdhelp()
    except mpdError, errmsg:
	print 'mpdhelp failed: %s' % (errmsg)
