#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys        import version_info, stdout, exc_info, exit
from socket     import socket, AF_INET, SOCK_STREAM, gethostbyname_ex
from re         import sub, split
from marshal    import dumps, loads
from traceback  import extract_stack, format_list, extract_tb
from exceptions import Exception
from syslog     import syslog, LOG_INFO
from os         import getuid
from grp        import getgrall
from pwd        import getpwnam, getpwuid

global mpd_version
mpd_version = (0,2,0,'Aug 2003 release')    # major, minor, micro, special

class mpdError(Exception):
    def __init__(self,args=None):
        self.args = args

def mpd_check_python_version():
    version = version_info    # (major,minor,micro,releaselevel,serial)
    if (version[0] < 2)  or (version[0] == 2 and version[1] < 2):
        mpd_raise('python version must 2.2 or greater')

def mpd_set_my_id(Id):
    global myId
    myId = Id

def mpd_set_procedures_to_trace(procs):
    global proceduresToTrace
    proceduresToTrace = procs

def mpd_print(*args):
    global myId
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    printLine = '%s (%s %d): ' % (myId,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    print printLine
    stdout.flush()
    syslog(LOG_INFO,printLine)

def mpd_print_tb(*args):
    global myId
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    stack = extract_stack()
    stack.reverse()
    stack = stack[1:]
    printLine = '%s (%s %d): ' % (myId,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    printLine = printLine + '\n    mpdtb: '
    for line in format_list(stack):
        line = sub(r'\n.*','',line)
        splitLine = split(',',line)
        splitLine[0] = sub('  File "(.*)"',lambda mo: mo.group(1),splitLine[0])
        splitLine[1] = sub(' line ','',splitLine[1])
        splitLine[2] = sub(' in ','',splitLine[2])
        printLine = printLine + '(%s,%s,%s) ' % tuple(splitLine)
    print printLine
    stdout.flush()
    syslog(LOG_INFO,printLine)

def mpd_get_tb():
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    stack.reverse()
    stack = stack[1:]
    tb = []
    for line in format_list(stack):
        line = sub(r'\n.*','',line)
        splitLine = split(',',line)
        splitLine[0] = sub('  File "(.*)"',lambda mo: mo.group(1),splitLine[0])
        splitLine[1] = sub(' line ','',splitLine[1])
        splitLine[2] = sub(' in ','',splitLine[2])
        tb.append(tuple(splitLine))
    return tb

def mpd_raise(errmsg):
    raise_msg = errmsg + '\n    traceback: %s' % (mpd_get_tb()[1:])
    syslog(raise_msg)
    raise mpdError, raise_msg

def mpd_print_non_mpd_exception(msg):
    print msg
    print '  Exception raised:  %s' % (exc_info()[0])
    print '  meaning:  %s' % (exc_info()[1])
    print '  traceback:'
    for i in extract_tb(exc_info()[2]):
        print '   ', i

def mpd_trace_calls(frame,event,args):
    global myId, proceduresToTrace
    code = frame.f_code
    if code.co_name not in proceduresToTrace:
        return None
    n = code.co_argcount
    if code.co_flags & 4: n += 1    # variable number of positionals (using *args)
    if code.co_flags & 8: n += 1    # arbitrary keyword args (using **kwargs)
    printLine = '%s:  ENTER %s line %d: ' % (myId,code.co_name,frame.f_lineno)
    for i in range(n):
        varname = code.co_varnames[i]
        if frame.f_locals.has_key(varname):
            varvalue = frame.f_locals[varname]
        else:
            varvalue = '* undefined *'
        printLine = printLine + '\n    %s = %s ' % (varname,varvalue)
    print printLine
    return mpd_trace_returns

def mpd_trace_returns(frame,event,args):
    global myId
    if event == 'return':
        print '%s:  EXIT  %s line %d ' % (myId,frame.f_code.co_name,frame.f_lineno)
        return None
    else:
        return mpd_trace_returns

def mpd_send_one_line(sock,line):
    try:
        sock.sendall(line)
    except Exception, errmsg:
        mpd_print_tb(1, 'mpd_send_one_line: sock=%s errmsg=:%s:' % (sock,errmsg) )

# This is a copy of the above function but doesn't print an error message if the
# send fails. We use it when the destination may have vanished (mpdrun got killed)
# and we don't care.
def mpd_send_one_line_noprint(sock,line):
    try:
        sock.sendall(line)
    except Exception, errmsg:
        return 0                        # note: 0 is failure
    return 1

def mpd_recv_one_line(sock):
    msg = ''
    try:
        c = sock.recv(1)
    except Exception, errmsg:
	c = ''
	msg = ''
        mpd_print_tb(0, 'mpd_recv_one_line: errmsg=:%s:' % (errmsg) )
    if c:
	while c != '\n':
	    msg += c
	    try:
	        c = sock.recv(1)
	    except Exception, errmsg:
		c = ''
		msg = ''
		mpd_print_tb(0, 'mpd_recv_one_line: errmsg=:%s:' % (errmsg) )
		break
	msg += c
    return msg

def mpd_send_one_msg(sock,msg):
    pickledMsg = dumps(msg)
    try:
        sock.sendall('%08d%s' % (len(pickledMsg),pickledMsg) )
    except StandardError, errmsg:    # any built-in exceptions
        mpd_print_tb(1, 'mpd_send_one_msg: errmsg=:%s:' % (errmsg) )
    except Exception, errmsg:
        mpd_print_tb(1, 'mpd_send_one_msg: sock=%s errmsg=:%s:' % (sock,errmsg) )
    except:
        mpd_print_tb(1, 'mpd_send_one_msg failed on sock %s' % sock)

# This is a copy of the above function but doesn't print an error message if the
# send fails. We use it when the destination may have vanished (mpdrun got killed)
# and we don't care.
def mpd_send_one_msg_noprint(sock,msg):
    pickledMsg = dumps(msg)
    try:
        sock.sendall('%08d%s' % (len(pickledMsg),pickledMsg) )
    except StandardError, errmsg:    # any built-in exceptions
        # mpd_print_tb(1, 'mpd_send_one_msg: errmsg=:%s:' % (errmsg) )
        return 0
    except Exception, errmsg:
        # mpd_print_tb(1, 'mpd_send_one_msg: sock=%s errmsg=:%s:' % (sock,errmsg) )
        return 0
    except:
        # mpd_print_tb(1, 'mpd_send_one_msg failed on sock %s' % sock)
        return 0
    return 1

def mpd_recv_one_msg(sock):
    msg = {}
    try:
        pickledLen = sock.recv(8)  # socket.error: (104, 'Connection reset by peer')
        if pickledLen:
            pickledLen = int(pickledLen)
            pickledMsg = sock.recv(pickledLen)  # socket.error: (104, 'Connection reset by peer')
            msg = loads(pickledMsg)
    except StandardError, errmsg:    # any built-in exceptions
        mpd_print_tb(0, 'mpd_recv_one_msg: errmsg=:%s:' % (errmsg) )
    except:
        mpd_print_tb(0, 'mpd_recv_one_msg failed on sock %s' % sock)
    return msg

def mpd_get_inet_listen_socket(host,port):
    sock = socket(AF_INET,SOCK_STREAM)
    sock.bind((host,port))  # note user may specify port 0 (anonymous)
    sock.listen(5)
    actualPort = sock.getsockname()[1]
    return(sock,actualPort)

def mpd_get_inet_socket_and_connect(host,port):
    tempSocket = socket(AF_INET,SOCK_STREAM)
    try:
        tempSocket.connect((host,port))  # note double parens
    except:
        mpd_raise('connect failed to host %s  port %d' % (host,port) )
    return tempSocket

def mpd_get_ranks_in_binary_tree(myRank,nprocs):
    if myRank == 0:
        parent = -1;
    else:   
        parent = (myRank - 1) / 2; 
    lchild = (myRank * 2) + 1
    if lchild > (nprocs - 1):
        lchild = -1;
    rchild = (myRank * 2) + 2
    if rchild > (nprocs - 1):
        rchild = -1;
    return (parent,lchild,rchild)

def mpd_socketpair():
    socket1 = socket(AF_INET,SOCK_STREAM)
    socket1.bind(('localhost',0))
    socket1.listen(1)
    port1 = socket1.getsockname()[1]
    socket2 = socket(AF_INET,SOCK_STREAM)
    socket2.connect(('localhost',port1))
    (socket3,addr) = socket1.accept()
    socket1.close()
    return (socket2,socket3)

def mpd_get_my_username():
    return getpwuid(getuid())[0]    #### instead of environ['USER']

def mpd_get_groups_for_username(username):
    userGroups = [getpwnam(username)[3]]  # default group for the user
    allGroups = getgrall();
    for group in allGroups:
        if username in group[3]  and  group[2] not in userGroups:
            userGroups.append(group[2])
    return userGroups

def mpd_same_ips(host1,host2):    # hosts may be names or IPs
    try:
        ips1 = gethostbyname_ex(host1)[2]    # may fail if invalid host
        ips2 = gethostbyname_ex(host2)[2]    # may fail if invalid host
    except:
        return 0
    for ip1 in ips1:
        for ip2 in ips2:
            if ip1 == ip2:
                return 1
    return 0

if __name__ == '__main__':
    print 'mpdlib for mpd version: %s' % str(mpd_version)
    print 'mpdlib does NOT run as a standalone console program;'
    print '    it is a module (library) imported by other python programs'
    exit(-1)
