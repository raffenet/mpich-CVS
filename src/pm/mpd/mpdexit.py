#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys    import argv, exit
from os     import environ, getuid, close
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from re     import sub
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdexit():
    mpd_set_my_id('mpdexit_')
    if (len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help')) or \
       (len(argv) < 2):
        print 'usage: mpdexit mpdid'
        print '    mpdid may be obtained via mpdtrace -l (or may be "localmpd")'
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
            print 'cannot connect to local mpd at %s' % consoleName
            exit(-1)
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
    msgToSend = { 'cmd' : 'mpdexit', 'mpdid' : argv[1] }
    mpd_send_one_msg(conSocket,msgToSend)
    msg = mpd_recv_one_msg(conSocket)
    if not msg:
        mpd_raise('mpd unexpectedly closed connection')
    elif msg['cmd'] == 'already_have_a_console':
        mpd_raise('mpd already has a console (e.g. for long ringtest); try later')
    if not msg.has_key('cmd'):
        raise RuntimeError, 'mpdexit: INVALID msg=:%s:' % (msg)
    if msg['cmd'] != 'mpdexit_ack':
        print 'mpdexit failed; may have wrong mpdid'
    print 'mpdexit done'


def sigint_handler(signum,frame):
    exit(-1)

if __name__ == '__main__':
    signal(SIGINT,sigint_handler)
    try:
        mpdexit()
    except mpdError, errmsg:
	print 'mpdexit failed: %s' % (errmsg)
