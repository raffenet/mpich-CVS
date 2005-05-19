#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdringtest [number of loops]
Times a single message going around the ring of mpds [num] times (default once)
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""


from  socket  import  socket, fromfd, AF_UNIX, SOCK_STREAM
from  os      import  environ, getuid, close
from  sys     import  argv, exit
from  time    import  time
from  signal  import  signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConsClientSock

def mpdringtest():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help' ) :
        usage()
    if len(argv) < 2: 
	numLoops = 1
    else:
	numLoops = int(argv[1])
    signal(SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdringtest')
    conSock = MPDConsClientSock()  # looks for MPD_UNIX_SOCKET in env
    msgToSend = { 'cmd' : 'mpdringtest', 'numloops' : numLoops }
    conSock.send_dict_msg(msgToSend)
    starttime = time()
    msg = conSock.recv_dict_msg()
    etime = time() - starttime
    if not msg:
        print 'mpdringtest terminated early'
    elif msg['cmd'] != 'mpdringtest_done':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
    else:
	print 'time for %d loops =' % numLoops, etime, 'seconds' 

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    exit(-1)

def usage():
    print __doc__
    exit(-1)

if __name__ == '__main__':
    mpdringtest()
