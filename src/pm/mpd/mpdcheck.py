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

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = "mom"

msg_to_server = 'hello_from_client_to_server'

if len(argv) > 1  and  (argv[1] == '-h' or argv[1] == '--help'):
    print __doc__
    exit(-1)

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

if '-v' in argv:
    verbose = 1
else:
    verbose = 0

# See if we can do gethostXXX, etc. for this host
uqhn1 = gethostname()
fqhn1 = getfqdn()
print "gethostname gives ", uqhn1
print "getfqdn gives ", fqhn1
if uqhn1.startswith('localhost'):
    if verbose:
        msg = """
        **********
        The unqualified hostname seems to be localhost. This generally
        means that the machine's hostname is not set. You may change
        it by using the 'hostname' command, e.g.:
            hostname mybox1
        However, this will not remain after a reboot. To do this, you
        will need to consult the operating system's documentation. On
        Debian Linux systems, this can be done by:
            echo "mybox1" > /etc/hostname
        **********
        """
    else:
        msg = "*** the uq hostname seems to be localhost"
    print msg.strip().replace('        ','')
elif uqhn1 == '':
    if verbose:
        msg = """
        **********
        The unqualified hostname seems to be blank. This generally
        means that the machine's hostname is not set. You may change
        it by using the 'hostname' command, e.g.:
            hostname mybox1
        However, this will not remain after a reboot. To do this, you
        will need to consult the operating system's documentation. On
        Debian Linux systems, this can be done by:
            echo "mybox1" > /etc/hostname
        **********
        """
    else:
        msg = "*** the uq hostname seems to be localhost"
    print msg.replace('        ','')
if fqhn1.startswith('localhost'):
    if verbose:
        msg = """
        **********
        Your fully qualified hostname seems to be set to 'localhost'.
        This generally means that your machine's /etc/hosts file contains a line
        similar to this:
            127.0.0.1 mybox1 localhost.localdomain localhost
        You probably want to remove your hostname from this line and place it on
        a line by itself with your ipaddress, like this:
            $ipaddr mybox1
        **********
        """
    else:
        msg =  "*** the fq hostname seems to be localhost"
    print msg.rstrip().replace('        ','')
elif fqhn1 == '':
    if verbose:
        msg = """
        **********
        Your fully qualified hostname seems to be blank.
        **********
        """
    else:
        msg = "*** the fq hostname is blank"
    print msg.replace('        ','')

uipaddr1 = 0
try:
    ghbnu = gethostbyname_ex(uqhn1)
    print "gethostbyname_ex: ", ghbnu
    uipaddr1 = ghbnu[2][0]
    if uipaddr1.startswith('127'):
        if verbose:
            msg = """
            **********
            Your unqualified hostname resolves to 127.0.0.1, which is
            the IP address reserved for localhost. This likely means that
            you have a line similar to this one in your /etc/hosts file:
            127.0.0.1   $uqhn
            This should perhaps be changed to the following:
            127.0.0.1   localhost.localdomain localhost
            **********
            """
        else:
            msg = "*** first ipaddr for this host (via %s) is: %s" % (uqhn1,uipaddr1)
        print msg.replace('            ','')
    try:
        ghbau = gethostbyaddr(uipaddr1)
    except:
        print "*** gethostbyaddr failed for this hosts's IP %s" % (uipaddr1)
except:
    if verbose:
        msg = """
        **********
        The system call gethostbyname(3) failed to resolve your
        unqualified hostname, or $uqhn. This can be caused by
        missing info from your /etc/hosts file or your system not
        having correctly configured name resolvers, or by your IP 
        address not existing in resolution services.
        If you run DNS, you may wish to make sure that your
        DNS server has the correct forward A set up for yout machine's
        hostname. If you are not using DNS and are only using hosts
        files, please check that a line similar to the one below exists
        in your /etc/hosts file:
            $ipaddr $uqdn
        If you plan to use DNS but you are not sure that it is
        correctly configured, please check that the file /etc/resolv.conf
        contains entries similar to the following:
            nameserver 1.2.3.4
        where 1.2.3.4 is an actual IP of one of your nameservers.
        **********
        """
    else:
        msg = "*** gethostbyname_ex failed for this host %s" % (uqhn1)
    print msg.replace('        ','')

fipaddr1 = 0
try:
    ghbnf = gethostbyname_ex(fqhn1)
    print "gethostbyname_ex: ", ghbnf
    fipaddr1 = ghbnf[2][0]
    if fipaddr1.startswith('127'):
        msg = """
        **********
        Your fully qualified hostname resolves to 127.0.0.1, which
        is the IP address reserved for localhost. This likely means
        that you have a line similar to this one in your /etc/hosts file:
             127.0.0.1   $fqhn
        This should be perhaps changed to the following:
             127.0.0.1   localhost.localdomain localhost
        **********
        """
    try:
        ghbaf = gethostbyaddr(fipaddr1)
    except:
        print "*** gethostbyaddr failed for this hosts's IP %s" % (uipaddr1)
except:
    if verbose:
        msg = """
        **********
        The system call gethostbyname(3) failed to resolve your
        fully qualified hostname, or $fqhn. This can be caused by
        missing info from your /etc/hosts file or your system not
        having correctly configured name resolvers, or by your IP 
        address not existing in resolution services.
        If you run DNS, please check and make sure that your
        DNS server has the correct forward A record set up for yout
        machine's hostname. If you are not using DNS and are only using
        hosts files, please check that a line similar to the one below
        exists in your /etc/hosts file:
            $ipaddr $fqhn
        If you intend to use DNS but you are not sure that it is
        correctly configured, please check that the file /etc/resolv.conf
        contains entries similar to the following:
            nameserver 1.2.3.4
        where 1.2.3.4 is an actual IP of one of your nameservers.
        **********
        """
    else:
        msg = "*** gethostbyname_ex failed for this host %s" % (fqhn1)
    print msg.replace('        ','')

if uipaddr1 and fipaddr1 and uipaddr1 != fipaddr1:
    msg = """
        **********
        Your fully qualified and unqualified names do not resolve to
        the same IP. This likely means that your DNS domain name is not
        set correctly.  This might be fixed by adding a line similar
        to the following to your /etc/hosts:
             $ipaddr             $fqhn   $uqdn
        **********
        """
    print msg.replace('        ','')

# See if we can do gethostXXX, etc. for the remote host
for argidx in range(1,len(argv)):
    if argv[argidx] == '-v':    # skip verbose
        continue
    uqhn2 = argv[argidx]
    fqhn2 = getfqdn(uqhn2)
    uipaddr2 = 0
    try:
        ghbnu = gethostbyname_ex(uqhn2)
        print "gethostbyname_ex: ", ghbnu
        uipaddr2 = ghbnu[2][0]
        try:
            ghbau = gethostbyaddr(uipaddr2)
        except:
            print "*** gethostbyaddr failed for remote hosts's IP %s" % (fipaddr2)
    except:
        print "*** gethostbyname_ex failed for host %s" % (fqhn2)
    try:
        ghbnf = gethostbyname_ex(fqhn2)
        print "gethostbyname_ex: ", ghbnf
        fipaddr2 = ghbnf[2][0]
        if uipaddr2  and  fipaddr2 != uipaddr2:
            print "*** ipaddr via uqn (%s) does not match via fqn (%s)" % (uipaddr2,fipaddr2)
        try:
            ghbaf = gethostbyaddr(fipaddr2)
        except:
            print "*** gethostbyaddr failed for remote hosts's IP %s" % (fipaddr2)
    except:
        print "*** gethostbyname_ex failed for this host %s" % (fqhn2)

