#!/usr/bin/env  python

"""
To run:
   testconfig.py

This script is a work in progress and may change frequently as we work
with users and gain additional insights into how to improve it.

This script prints quite a bit of useful information about the host on
which it runs, incuding some info about its ability to discover other
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

from sys    import argv
from os     import system, uname
from socket import gethostname, gethostbyname_ex, socket
from popen2 import popen4

hostnames = [
              'torvalds.cs.mtsu.edu',
              'ccn55.mcs.anl.gov',
              'ccn55-66.mcs.anl.gov',
              'ccn55-67.mcs.anl.gov',
              'ccn55-68.mcs.anl.gov',
            ]

print "----- checking info for ", gethostname()
print "--- uname: "
print "   ", uname()
print "--- hostbyname info: "
print "   ", gethostbyname_ex(gethostname())
print "--- remote hosts info"
for hostname in hostnames:
    print "- hostbyname info for %s: " % hostname
    try:
        print "    ", gethostbyname_ex(hostname)
    except:
        print "could not get the scoop "

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
