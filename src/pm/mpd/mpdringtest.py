#!/usr/bin/env python

from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from os     import environ, getuid, close
from sys    import argv, exit
from time   import time
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdringtest():
    mpd_set_my_id('mpdringtest')
    if len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help' ) :
        print 'usage: mpdringtest [number of loops]'
        exit(-1)
    if len(argv) < 2: 
	numLoops = 1
    else:
	numLoops = int(argv[1])
    username = mpd_get_my_username()
    if environ.has_key('UNIX_SOCKET'):
        conFD = int(environ['UNIX_SOCKET'])
        conSocket = fromfd(conFD,AF_UNIX,SOCK_STREAM)
        close(conFD)
    else:
        consoleName = '/tmp/mpd2.console_' + username
        conSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
        try:
            conSocket.connect(consoleName)
        except Exception, errmsg:
            mpd_raise('cannot connect to local mpd')
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
    msgToSend = { 'cmd' : 'mpdringtest', 'numloops' : numLoops }
    starttime = time()
    mpd_send_one_msg(conSocket,msgToSend)
    msg = mpd_recv_one_msg(conSocket)
    etime = time() - starttime
    if not msg or msg['cmd'] != 'mpdringtest_done':
        if msg['cmd'] == 'already_have_a_console':
            mpd_raise('mpd already has a console (e.g. for long ringtest); try later')
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    else:
	print 'time for %d loops =' % numLoops, etime, 'seconds' 

def sigint_handler(signum,frame):
    exit(-1)

if __name__ == '__main__':
    signal(SIGINT,sigint_handler)
    try:
        mpdringtest()
    except mpdError, errmsg:
	print 'mpdringtest failed: %s' % (errmsg)
