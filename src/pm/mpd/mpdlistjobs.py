#!/usr/bin/env python

from sys    import argv, exit
from os     import environ, getuid, close
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from re     import sub
from signal import signal, SIG_DFL, SIGINT, SIGTSTP, SIGCONT
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError

def mpdlistjobs():
    mpd_set_my_id('mpdlistjobs_')
    username = mpd_get_my_username()
    uname    = ''
    jobid    = ''
    sjobid   = ''
    jobalias = ''
    if len(argv) > 1:
        aidx = 1
        while aidx < len(argv):
            if argv[aidx] == '-h'  or  argv[aidx] == '--help':
                print 'usage: mpdlistjobs [-u | --user username] [-a | --alias jobalias] ',
                print '[-j | --jobid jobid]'
                print '  (only use one of jobalias or jobid)'
                print 'lists jobs being run by an mpd ring, all by default, or filtered'
                print 'by user, mpd job id, or alias assigned when the job was submitted'
                exit(-1)
            if argv[aidx] == '-u' or argv[aidx] == '--user':
                uname = argv[aidx+1]
                aidx += 2
            elif argv[aidx] == '-j'  or  argv[aidx] == '--jobid':
                jobid = argv[aidx+1]
                aidx += 2
                sjobid = jobid.split('@')    # jobnum and originating host
            elif argv[aidx] == '-a'  or  argv[aidx] == '--alias':
                jobalias = argv[aidx+1]
                aidx += 2
            elif argv[aidx] == '-sss':
                aidx +=1
            else:
                print 'unrecognized arg: %s' % argv[aidx]
                exit(-1)
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
    msgToSend = { 'cmd' : 'mpdlistjobs' }
    mpd_send_one_msg(conSocket,msgToSend)
    msg = mpd_recv_one_msg(conSocket)
    if msg['cmd'] != 'local_mpdid':     # get full id of local mpd for filters later
        mpd_raise('did not recv local_mpdid msg from local mpd; instead, recvd: %s' % msg)
    else:
        if len(sjobid) == 1:
            sjobid.append(msg['id'])
    while 1:
        msg = mpd_recv_one_msg(conSocket)
        if not msg.has_key('cmd'):
            raise RuntimeError, 'mpdlistjobs: INVALID msg=:%s:' % (msg)
        if msg['cmd'] == 'mpdlistjobs_info':
            smjobid = msg['jobid'].split('  ')  # jobnum, mpdid, and alias (if present)
            if len(smjobid) < 3:
                smjobid.append('')
            print_based_on_uname    = 0    # default
            print_based_on_jobid    = 0    # default
            print_based_on_jobalias = 0    # default
            if not uname  or  uname == msg['username']:
                print_based_on_uname = 1
            if not jobid  and  not jobalias:
                print_based_on_jobid = 1
                print_based_on_jobalias = 1
            else:
                if sjobid  and  sjobid[0] == smjobid[0]  and  sjobid[1] == smjobid[1]:
                    print_based_on_jobid = 1
                if jobalias  and  jobalias == smjobid[2]:
                    print_based_on_jobalias = 1
            if not smjobid[2]:
                smjobid[2] = '          '  # just for printing
            if print_based_on_uname and (print_based_on_jobid or print_based_on_jobalias):
                if '-sss' in argv:
                    print "%s %s %s"%(msg['host'],msg['clipid'],msg['sid'])
                else:
                    print 'jobid    = %s@%s' % (smjobid[0],smjobid[1])
                    print 'jobalias = %s'    % (smjobid[2])
                    print 'username = %s'    % (msg['username'])
                    print 'host     = %s'    % (msg['host'])
                    print 'pid      = %s'    % (msg['clipid'])
                    print 'sid      = %s'    % (msg['sid'])
                    print 'rank     = %s'    % (msg['rank'])
                    print 'pgm      = %s'    % (msg['pgm'])
                    print
        else:
            break  # mpdlistjobs_trailer


def sigint_handler(signum,frame):
    exit(-1)

if __name__ == '__main__':
    signal(SIGINT,sigint_handler)
    try:
        mpdlistjobs()
    except mpdError, errmsg:
	print 'mpdlistjobs failed: %s' % (errmsg)
