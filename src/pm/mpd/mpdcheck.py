#!/usr/bin/env python

"""
mpdcheck

This script is a work in progress and may change frequently as we work
with users and gain additional insights into how to improve it.

This script prints useful information about the host on which it runs.
It is here to help us help users detect problems with configurations of
their computers.  For example, some computers are configured to think
of themselves simply as 'localhost' with 127.0.0.1 as the IP address.
This might present problems if a process on that computer wishes to
identify itself by host and port to a process on another computer.
The process on the other computer would try to contact 'localhost'.

If you are having problems running parallel jobs via mpd on one or more
hosts, you might try running this script once on each of those hosts.

Any output with *** at the beginning indicates a potential problem
that you may have to resolve before being able to run parallel jobs
via mpd.

There are really three modes of operation for this program:

    mpdcheck
        prints info about 'this' host
        
    mpdcheck some_remote_hostname
        prints info about 'this' host and locatability info about the
        remote one as well
        
    mpdcheck -s
        runs this program as a server on one host
    mpdcheck -c server_host server_port
        runs a client on another (or same) host; connects to the specifed
        host/port where you previously started the server
"""

from sys    import argv, exit
from socket import gethostname, getfqdn, gethostbyname_ex, gethostbyaddr, socket

msg_to_server = 'hello_from_client_to_server'

if len(argv) == 2  and  argv[1] == '-s':
    lsock = socket()
    lsock.bind((gethostname(),0)) # anonymous port
    lsock.listen(5)
    print "server listening on: %s %s" % (gethostname(),lsock.getsockname()[1])
    (tsock,taddr) = lsock.accept()
    print "server has conn on %s from %s" % (tsock,taddr)
    msg = tsock.recv(64)
    if not msg:
        print "*** server failed to recv msg from client"
    else:
        print "server successfully recvd msg from client: %s" % (msg)
    tsock.sendall('ack_from_server_to_client')
    tsock.close()
    exit(0)

if len(argv) == 4   and  argv[1] == '-c':
    sock = socket()
    sock.connect((argv[2],int(argv[3])))  # note double parens
    sock.sendall(msg_to_server)
    msg = sock.recv(64)
    if not msg:
        print "*** client failed to recv ack from server"
    else:
        print "client successfully recvd ack from server: %s" % (msg)
    sock.close()
    exit(0)

# ELSE, check out this host and optional one mentioned on cmd-line

# See if we can do gethostXXX, etc. for this host
uqhn1 = gethostname()
fqhn1 = getfqdn()
print "gethostname gives ", uqhn1
print "getfqdn gives ", fqhn1
if uqhn1.startswith('localhost'):
    print "*** your uq hostname seems to be localhost"
if fqhn1.startswith('localhost'):
    print "*** your fq hostname seems to be localhost"

try:
    ghbnu = gethostbyname_ex(uqhn1)
    print "gethostbyname_ex: ", ghbnu
    uipaddr1 = ghbnu[2][0]
    if uipaddr1.startswith('127'):
        print "*** first ipaddr for this host (via %s) is: %s" % (uqhn1,uipaddr1)
    try:
        ghbau = gethostbyaddr(uipaddr1)
    except:
        print "*** RMB gethostbyaddr failed for %s" % (uipaddr1)
except:
    print "*** gethostbyname_ex failed for this host %s" % (uqhn1)
try:
    ghbnf = gethostbyname_ex(fqhn1)
    print "gethostbyname_ex: ", ghbnf
    fipaddr1 = ghbnf[2][0]
    if fipaddr1.startswith('127'):
        print "*** first ipaddr for this host (via %s) is: %s" % (fqhn1,fipaddr1)
    if fipaddr1 != uipaddr1:
        print "*** ipaddr via uqn (%s) does not match via fqn (%s)" % (uipaddr1,fipaddr1)
    try:
        ghbaf = gethostbyaddr(fipaddr1)
    except:
        print "*** gethostbyaddr failed for %s" % (fipaddr1)
except:
    print "*** gethostbyname_ex failed for this host %s" % (fqhn1)

# See if we can do gethostXXX, etc. for the remote host
for argidx in range(1,len(argv)):
    print
    uqhn2 = argv[argidx]
    fqhn2 = getfqdn(uqhn2)
    try:
        ghbnu = gethostbyname_ex(uqhn2)
        print "gethostbyname_ex: ", ghbnu
        uipaddr2 = ghbnu[2][0]
        try:
            ghbau = gethostbyaddr(uipaddr2)
        except:
            print "*** gethostbyaddr failed for %s" % (uipaddr2)
    except:
        print "*** gethostbyname_ex failed for this host %s %s" % (uqhn2,uipaddr2)
    try:
        ghbnf = gethostbyname_ex(fqhn2)
        print "gethostbyname_ex: ", ghbnf
        fipaddr2 = ghbnf[2][0]
        if fipaddr2 != uipaddr2:
            print "*** ipaddr via uqn (%s) does not match via fqn (%s)" % (uipaddr2,fipaddr2)
        try:
            ghbaf = gethostbyaddr(fipaddr2)
        except:
            print "*** gethostbyaddr failed for %s" % (fipaddr2)
    except:
        print "*** gethostbyname_ex failed for this host %s" % (fqhn2)

