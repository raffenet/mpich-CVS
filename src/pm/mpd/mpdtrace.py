#!/usr/bin/env python

from sys    import argv, exit
from os     import environ, getuid, close
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from re     import sub
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdtrace():
    mpd_set_my_id('mpdtrace_')
    if len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help') :
        print 'usage: mpdtrace [-l]'
        print 'Lists the (short) hostname of each of the mpds in the ring'
        print 'The -l (ell) option shows full hostnames and listening ports'
        exit(-1)
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
    msgToSend = { 'cmd' : 'mpdtrace' }
    mpd_send_one_msg(conSocket,msgToSend)
    while 1:
        msg = mpd_recv_one_msg(conSocket)
        if not msg:
            mpd_raise('mpd unexpectedly closed connection')
        elif msg['cmd'] == 'already_have_a_console':
	    mpd_raise('mpd already has a console (e.g. for long ringtest); try later')
        if not msg.has_key('cmd'):
            raise RuntimeError, 'mpdtrace: INVALID msg=:%s:' % (msg)
        if msg['cmd'] == 'mpdtrace_info':
	    if len(argv) > 1 and argv[1] == '-l':
		print msg['id']
            else:
		print sub(r'[\._].*', '', msg['id'])    # strip off domain and port
            # printLine = msg['id'] + ': '
            # printLine = printLine + 'lhs=' + msg['lhs'] + ' '
            # printLine = printLine + 'rhs=' + msg['rhs'] + ' '
            # print 'mpdtrace: ', printLine
	elif msg['cmd'] == 'restarting_mpd':
	    print 'mpd failed and is restarting'
	    break
        else:
            break  # mpdtrace_trailer


def sigint_handler(signum,frame):
    exit(-1)

if __name__ == '__main__':
    signal(SIGINT,sigint_handler)
    try:
        mpdtrace()
    except mpdError, errmsg:
	print 'mpdtrace failed: %s' % (errmsg)
