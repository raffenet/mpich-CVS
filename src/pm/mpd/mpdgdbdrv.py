#!/usr/bin/env python

from sys    import argv, exit, stdin, stdout, stderr
from os     import kill, getpid, write
from popen2 import Popen4
from signal import signal, SIGUSR1, SIGINT, SIGKILL
from errno  import EINTR
from select import select, error
from re     import findall, sub

global appPid, gdbPID


def sig_handler(signum,frame):
    global appPid, gdbPID
    if signum == SIGINT:
        try:
            kill(appPid,SIGINT)
        except:
            pass
    elif signum == SIGUSR1:
        try:
            kill(gdbPid,SIGKILL)
        except:
            pass
        try:
            kill(appPid,SIGKILL)
        except:
            pass


signal(SIGINT,sig_handler)
signal(SIGUSR1,sig_handler)

if len(argv) > 2:
    args = ' '.join(argv[2:])
else:
    args = ''
gdb_info = Popen4('gdb -q %s %s' % (argv[1],args), 0 )
gdbPid = gdb_info.pid
# print "PID=%d GDBPID=%d" % (getpid(),gdbPid) ; stdout.flush()
gdb_sin = gdb_info.tochild
gdb_sin_fileno = gdb_sin.fileno()
gdb_sout_serr = gdb_info.fromchild
write(gdb_sin_fileno,'set prompt (gdb)\\n\n')
line = gdb_sout_serr.readline() #; print "LINE1=|%s|" % (line.rstrip()) ; stdout.flush()
write(gdb_sin_fileno,'set confirm off\n')
line = gdb_sout_serr.readline() #; print "LINE2=|%s|" % (line.rstrip()) ; stdout.flush()
write(gdb_sin_fileno,'handle SIGUSR1 nostop noprint\n')
line = gdb_sout_serr.readline() #; print "LINE3=|%s|" % (line.rstrip()) ; stdout.flush()
write(gdb_sin_fileno,'handle SIGPIPE nostop noprint\n')
line = gdb_sout_serr.readline() #; print "LINE4=|%s|" % (line.rstrip()) ; stdout.flush()
write(gdb_sin_fileno,'set confirm on\n')
line = gdb_sout_serr.readline() #; print "LINE5=|%s|" % (line.rstrip()) ; stdout.flush()

print '(gdb)\n', ; stdout.flush()    # initial prompt to user

user_fileno = stdin.fileno()
gdb_sout_serr_fileno = gdb_sout_serr.fileno()
while 1:
    try:
        (readyFDs,unused1,unused2) = select([user_fileno,gdb_sout_serr_fileno],[],[],1)
    except error, data:
        if data[0] == EINTR:    # interrupted by timeout for example
            continue
        else:
            print 'select error: %s' % strerror(data[0])
    # print "READY=", readyFDs ; stdout.flush()
    for readyFD in readyFDs:
        if readyFD == gdb_sout_serr_fileno:
            gdb_line = gdb_sout_serr.readline()
            if not gdb_line:
                print "MPIGDB ENDING" ; stdout.flush()
                exit(0)
            # print "LINE |%s|" % (gdb_line.rstrip()) ; stdout.flush()
            print gdb_line, ; stdout.flush()
        elif readyFD == user_fileno:
            user_line = stdin.readline()
            # print "USERLINE=", user_line, ; stdout.flush()
            if not line:
                print '***** problem no line from gdb'
                exit(-1)
            if user_line.startswith('r'):
                write(gdb_sin_fileno,'b 1\n')
                for i in range(2):    # drain multi-line response
                    gdb_line = gdb_sout_serr.readline()  # drain response
                write(gdb_sin_fileno,user_line)
                for i in range(6):    # drain multi-line response
                    gdb_line = gdb_sout_serr.readline()  # drain reaponse
                    # print "RESPLINE=|%s|" % (gdb_line) ; stdout.flush()

                write(gdb_sin_fileno,'info program\n')
                gdb_line = gdb_sout_serr.readline()  # get pid
                appPid = findall(r'Using .* image of child process (\d+)',gdb_line)
                appPid = int(appPid[0])
                # print "PID=%d" % (appPid) ; stdout.flush()
                for i in range(3):    # drain multi-line response
                    gdb_line = gdb_sout_serr.readline()  # drain reaponse

                write(gdb_sin_fileno,'c\n')
            else:
                write(gdb_sin_fileno,user_line)
