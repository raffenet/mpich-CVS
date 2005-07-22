#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdkilljob  jobnum  [mpdid]  # as obtained from mpdlistjobs
   or: mpdkilljob  -a jobalias      # as obtained from mpdlistjobs
    mpdid is mpd where process with 'rank' 0 starts
    mpdid of form 1@linux02_32996 (may need \@ in csh)
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__credits__ = ""


from  os      import  environ, getuid, close, path
from  sys     import  argv, exit
from  socket  import  socket, fromfd, AF_UNIX, SOCK_STREAM
from  signal  import  signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB
def mpdkilljob():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '--help':
        usage()
    signal(SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdkilljob')
    mpdid = ''
    if argv[1] == '-a':
        jobalias = argv[2]
        jobnum = '0'
    else:
        jobalias = ''
        jobid = argv[1]
        sjobid = jobid.split('@')
        jobnum = sjobid[0]
        if len(sjobid) > 1:
            mpdid = sjobid[1]

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

    msgToSend = { 'cmd':'mpdkilljob', 'jobnum' : jobnum, 'mpdid' : mpdid,
                  'jobalias' : jobalias, 'username' : mpd_get_my_username() }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=5.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
        exit(-1)
    if msg['cmd'] != 'mpdkilljob_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
        exit(-1)
    if not msg['handled']:
        print 'job not found'
        exit(-1)
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    exit(-1)

def usage():
    print __doc__
    exit(-1)

if __name__ == '__main__':
    mpdkilljob()
