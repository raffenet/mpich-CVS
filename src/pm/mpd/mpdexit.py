#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdexit mpdid
    mpdid may be obtained via mpdtrace -l (or may be "localmpd")
Causes a single mpd to exit (and thus exit the ring).
Note that this may cause other mpds to become 'isolated' if they
entered the ring through the exiting one.
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""


from sys    import argv, exit
from os     import environ, getuid, close, path
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from re     import sub
from signal import alarm, signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, \
                      MPDConClientSock, MPDParmDB

def mpdexit():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if (len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help')) or \
       (len(argv) < 2):
	print __doc__
        exit(-1)
    signal(SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdexit')

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPD_USE_ROOT_MPD'            :  0,
                        'MPD_SECRETWORD'              :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb.get_parms_from_env(parmsToOverride)
    parmdb.get_parms_from_rcfile(parmsToOverride)
    if getuid() == 0  or  parmdb['MPD_USE_ROOT_MPD']:
        fullDirName = path.abspath(path.split(argv[0])[0])  # normalize
        mpdroot = fullDirName + '/mpdroot'
        conSock = MPDConClientSock(mpdroot=mpdroot,secretword=parmdb['MPD_SECRETWORD'])
    else:
        conSock = MPDConClientSock(secretword=parmdb['MPD_SECRETWORD'])

    msgToSend = { 'cmd' : 'mpdexit', 'mpdid' : argv[1] }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=5.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
        exit(-1)
    elif msg['cmd'] == 'already_have_a_console':
        mpd_print(1,'mpd already has a console (e.g. for long ringtest); try later')
        exit(-1)
    if not msg.has_key('cmd'):
        mpd_print(1,'mpdexit: INVALID msg=:%s:' % (msg))
        exit(-1)
    if msg['cmd'] != 'mpdexit_ack':
        mpd_print(1,'mpdexit failed; may have wrong mpdid')
        exit(-1)

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    exit(-1)

def usage():
    print __doc__
    exit(-1)

if __name__ == '__main__':
    mpdexit()
