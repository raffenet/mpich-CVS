#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

## NOTE: we do NOT allow this pgm to run via mpdroot

from os     import environ
from sys    import argv, exit
from socket import socket, AF_UNIX, SOCK_STREAM
from signal import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpdError, mpd_raise

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
        print 'cannot connect to local mpd at %s' % consoleName
        exit(-1)
    mpd_send_one_msg(conSocket, {'cmd':'mpdallexit'})
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdallexit_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            exit(-1)
        else:
            print 'mpdallexit failed: unexpected message from mpd: %s' % (msg)
            exit(-1)
    conSocket.close()


def signal_handler(signum,frame):
    if signum == SIGALRM:
        pass
    else:
        exit(-1)

def recv_one_msg_with_timeout(sock,timeout):
    oldTimeout = alarm(timeout)
    msg = mpd_recv_one_msg(sock)    # fails WITHOUT a msg if sigalrm occurs
    alarm(oldTimeout)
    return(msg)

if __name__ == '__main__':
    signal(SIGINT,signal_handler)
    signal(SIGALRM,signal_handler)
    try:
        mpdallexit()
    except mpdError, errmsg:
        print 'mpdallexit failed: %s' % (errmsg)
