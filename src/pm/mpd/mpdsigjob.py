#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from os     import environ, getuid, close
from sys    import argv, exit
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from signal import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdsigjob():
    mpd_set_my_id('mpdsigjob_')
    if len(argv) < 3  or  argv[1] == '-h'  or  argv[1] == '--help':
        print 'usage: mpdsigjob  sigtype  jobnum  [mpdid]  # as obtained from mpdlistjobs'
        print '   or: mpdsigjob  sigtype  -a jobalias      # as obtained from mpdlistjobs'
        print '    mpdid is mpd where process 0 starts'
        print '    mpdid of form 1@linux02_32996 (may need \@ in csh)'
        print 'Delivers a Unix signal to all the application processes in the job'
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
    sigtype = argv[1]
    if sigtype.startswith('-'):
        sigtype = sigtype[1:]
    if sigtype.startswith('SIG'):
        sigtype = sigtype[3:]
    import signal as tmpsig  # just to get valid SIG's
    if sigtype.isdigit():
        if int(sigtype) > tmpsig.NSIG:
            print 'invalid signum: %s' % (sigtype)
            exit(-1)
    else:
	if not tmpsig.__dict__.has_key('SIG' + sigtype):
	    print 'invalid sig type: %s' % (sigtype)
	    exit(-1)
    mpdid = ''
    if argv[2] == '-a':
        jobalias = argv[3]
        jobnum = '0'
    else:
        jobalias = ''
        jobid = argv[2]
        sjobid = jobid.split('@')
        jobnum = sjobid[0]
        if len(sjobid) > 1:
            mpdid = sjobid[1]
    msgToSend = {'cmd' : 'mpdsigjob', 'sigtype': sigtype,
                 'jobnum' : jobnum, 'mpdid' : mpdid, 'jobalias' : jobalias,
                 'username' : username }
    mpd_send_one_msg(conSocket, msgToSend)
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdsigjob_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
        exit(-1)
    if not msg['handled']:
        print 'job not found'
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
	mpdsigjob()
    except mpdError, errmsg:
	print 'mpdsigjob failed: %s' % (errmsg)
