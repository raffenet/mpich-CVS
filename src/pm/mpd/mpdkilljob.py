#!/usr/bin/env python

from os     import environ, getuid, close
from sys    import argv, exit
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdkilljob():
    mpd_set_my_id('mpdkilljob_')
    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '--help':
        print 'usage: mpdkilljob  jobnum  [mpdid]  # as obtained from mpdlistjobs'
        print '   or: mpdkilljob  -a jobalias      # as obtained from mpdlistjobs'
        print '    mpdid is mpd contacted by mpdrun to start the job (defaults to here)'
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
    mpdid = ''
    if argv[1] == '-a':
        jobalias = argv[2]
        jobnum = '0'
    else:
        jobalias = ''
        jobnum = argv[1]
        if len(argv) > 2:
            mpdid = argv[2]
    mpd_send_one_msg(conSocket, {'cmd':'mpdkilljob',
                                 'jobnum' : jobnum, 'mpdid' : mpdid, 'jobalias' : jobalias,
                                 'username' : username})
    msg = mpd_recv_one_msg(conSocket)
    if not msg or msg['cmd'] != 'mpdkilljob_ack':
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
	mpdkilljob()
    except mpdError, errmsg:
	print 'mpdkilljob failed: %s' % (errmsg)
