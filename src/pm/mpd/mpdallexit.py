#!/usr/bin/env python

## NOTE: we do NOT allow this pgm to run via mpdroot

from os     import environ
from sys    import argv, exit
from socket import socket, AF_UNIX, SOCK_STREAM
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdallexit():
    mpd_set_my_id('mpdallexit_')
    if len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help') :
        print 'usage: mpdallexit (no args)'
        print 'causes all mpds in the ring to exit'
        exit(-1)
    consoleName = '/tmp/mpd2.console_' + mpd_get_my_username()
    conSocket = socket(AF_UNIX, SOCK_STREAM)             # note: UNIX socket
    try:
        conSocket.connect(consoleName)
    except Exception, errmsg:
        mpd_raise('cannot connect to local mpd: %s' % consoleName)
        # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
    mpd_send_one_msg(conSocket, {'cmd':'mpdallexit'})
    msg = mpd_recv_one_msg(conSocket)
    if not msg or msg['cmd'] != 'mpdallexit_ack':
        if msg['cmd'] == 'already_have_a_console':
            mpd_raise('mpd already has a console (e.g. for long ringtest); try later')
        else:
            mpd_raise('unexpected message from mpd: %s' % (msg) )
    conSocket.close()

def sigint_handler(signum,frame):
    exit(-1)

if __name__ == '__main__':
    signal(SIGINT,sigint_handler)
    try:
        mpdallexit()
    except mpdError, errmsg:
        print 'mpdallexit failed: %s' % (errmsg)
