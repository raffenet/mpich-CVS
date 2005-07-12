#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdtrace [-l]
Lists the (short) hostname of each of the mpds in the ring
The -l (long) option shows full hostnames and listening ports and ifhn
"""

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""

from  sys     import  argv, exit
from  re      import  sub
from  signal  import  signal, SIGINT
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock

def mpdtrace():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(argv) > 1:
        if (argv[1] == '-h' or argv[1] == '--help') or (argv[1] != '-l'):
            usage()
    signal(SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdtrace')
    conSock = MPDConClientSock()  # looks for MPD_UNIX_SOCKET in env
    msgToSend = { 'cmd' : 'mpdtrace' }
    conSock.send_dict_msg(msgToSend)
    # Main Loop
    done = 0
    while not done:
        msg = conSock.recv_dict_msg(timeout=5.0)
        if not msg:    # also get this on ^C
            mpd_print(1, 'got eof on console')
            exit(-1)
        elif not msg.has_key('cmd'):
            print 'mpdtrace: unexpected msg from mpd=:%s:' % (msg)
            exit(-1)
        if msg['cmd'] == 'mpdtrace_info':
            if len(argv) > 1 and argv[1] == '-l':
                print '%s (%s)' % (msg['id'],msg['ifhn'])
            else:
                print sub(r'[\._].*', '', msg['id'])    # strip off domain and port
        elif msg['cmd'] == 'mpdtrace_trailer':
            done = 1
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    exit(-1)

def usage():
    print __doc__
    exit(-1)

if __name__ == '__main__':
    mpdtrace()
