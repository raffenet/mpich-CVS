#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
This program has largely been replaced by mpdcheck.  It can sometimes
still be useful to print the contents of some system config files, e.g. 
/etc/hosts.

To run:
   testconfig

This script is a work in progress and may change frequently as we work
with users and gain additional insights into how to improve it.

This script prints quite a bit of useful information about the host on
which it runs, including some info about its ability to discover other
computers via local files or dns.  It is here to help us help users
detect problems with configurations of their computers.  For example,
some computers are configured to think of themselves simply as
'localhost' with 127.0.0.1 as the IP address.  This might present
problems if a process on that computer wishes to identify itself
by host and port to a process on another computer.  The process on the
other computer would try to contact 'localhost'.

If you are having problems getting 2 computers to talk to each other,
you might change the array named 'hostnames' below to contain those 2
computers and then run the script twice, once on each computer.
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""


from sys    import argv, exit
from os     import system, uname
from socket import gethostname, gethostbyname_ex, socket
from popen2 import popen4

# provide 'something' if a user does pydoc on this module or asks for help
if argv[0].find('pydoc') >= 0  or (len(argv) == 2 and (argv[1] == '-h'  or  argv[1] == '--help')):
    print __doc__
    exit(0)

hostnames = [
              'torvalds.cs.mtsu.edu',
              'ccn55.mcs.anl.gov',
              'ccn55-66.mcs.anl.gov',
              'ccn55-67.mcs.anl.gov',
              'ccn55-68.mcs.anl.gov',
            ]

myhostname = gethostname()
print "----- checking info for ", myhostname
print "--- uname: "
try:
    print "   ", uname()
except:
    print "    uname failed for %s" % (myhostname)
print "--- hostbyname info: "
try:
    print "   ", gethostbyname_ex(myhostname)
except:
    print "    gethostbyname failed for my host %s" % (myhostname)
print "--- remote hosts info"
for hostname in hostnames:
    print "- hostbyname info for %s: " % hostname
    try:
        print "    ", gethostbyname_ex(hostname)
    except:
        print "    gethostbyname_ex failed for %s" % (hostname)

try:
    s = socket()
    s.bind((gethostname(),0))
    s.listen(5)
    port = s.getsockname()[1]
    t1 = socket()
    t1.connect((gethostname(),port))
    (s1,s1addr) = s.accept()
    print "--- sockets info"
    # print "    s1addr      =", s1addr    ## same as s1.peername
    print "    s1.peername =", s1.getpeername()
    print "    t1.peername =", t1.getpeername()
except:
    print "** failed to print sockets info"

print
print "--- try to print /etc/hosts"
try:
    f = open('/etc/hosts')
    for line in f:
        print "   ", line,
except:
    print "    ** failed to open /etc/hosts"
print "--- try to print /etc/resolv.conf"
try:
    f = open('/etc/resolv.conf')
    for line in f:
        print "   ", line,
except:
    print "    ** failed to open /etc/resolv.conf"
print "--- try to print /etc/nsswitch.conf"
try:
    f = open('/etc/nsswitch.conf')
    for line in f:
        print "   ", line,
except:
    print "    ** failed to open /etc/nsswitch.conf"
print "--- try to run /sbin/ifconfig -a"
try:
    (sout_serr,sin) = popen4('/sbin/ifconfig -a')
    for line in sout_serr:
        print "   ", line,
except:
    print "    ** failed to run /sbin/ifconfig -a"

print "----- done checking info for ", gethostname()
