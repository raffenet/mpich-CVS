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
write(gdb_sin_fileno,'echo hi1\n')
line = gdb_sout_serr.readline() #; print "LINE6=|%s|" % (line.rstrip()) ; stdout.flush()
i = 0
while i < 8  and  not line.startswith('hi1'):
    line = gdb_sout_serr.readline() #; print "LINEx=|%s|" % (line.rstrip());stdout.flush()
    i += 1
if i >= 8:
    print 'failed to drain valid input from gdb'
    exit(-1)

write(gdb_sin_fileno,'b main\n')
gdb_line = gdb_sout_serr.readline()  # drain breakpoint response
if not gdb_line.startswith('Breakpoint'):
    print 'mpdgdbdrv: expecting "Breakpoint", got :%s:' % (gdb_line)
    exit(-1)
gdb_line = gdb_sout_serr.readline()  # drain prompt
if not gdb_line.startswith('(gdb)'):
    print 'mpdgdbdrv: expecting "(gdb)", got :%s:' % (gdb_line)
    exit(-1)

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
                print 'mpdgdbdrv: problem: expected user input but got none'
                exit(-1)
            if user_line.startswith('r'):
                # we have already set breakpoint 1 in main
                write(gdb_sin_fileno,user_line)
                gdb_line = gdb_sout_serr.readline()  # drain starting msg
                if not gdb_line.startswith('Starting program'):
                    print 'mpdgdbdrv: expecting "Starting program", got :%s:' % (gdb_line)
                    exit(-1)
                while 1:    # drain to a prompt
                    gdb_line = gdb_sout_serr.readline()  # drain one line
                    if gdb_line.startswith('(gdb)'):
                        break
                write(gdb_sin_fileno,'info program\n')
                gdb_line = gdb_sout_serr.readline().lstrip()  # get pid
                if gdb_line.startswith('Using'):
                    print "LINE=", gdb_line
                    if gdb_line.find('process') >= 0:
                        appPid = findall(r'Using .* image of child process (\d+)',gdb_line)
                    elif gdb_line.find('Thread') >= 0:
                        appPid = findall(r'Using .* image of child .* \(LWP (\d+)\).',gdb_line)
                    else:
                        print 'mpdgdbdrv: expecting process or thread line, got :%s:' % (gdb_line)
                        exit(-1)
                    appPid = int(appPid[0])
                else:
                    print 'mpdgdbdrv: expecting line with "Using"; got :%s:' % (gdb_line)
                    exit(-1)
                while 1:    # drain to a prompt
                    gdb_line = gdb_sout_serr.readline()  # drain one line
                    if gdb_line.startswith('(gdb)'):
                        break
                write(gdb_sin_fileno,'c\n')
            else:
                write(gdb_sin_fileno,user_line)
