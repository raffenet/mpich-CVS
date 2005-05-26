#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#


import socket
import inspect

from  sys       import  version_info, stdout, exit
from  os        import  path, environ, getuid, strerror, unlink, read, access, R_OK, X_OK
from  os        import  error as os_error, close as osclose
from  marshal   import  dumps, loads
from  pwd       import  getpwuid, getpwnam
from  grp       import  getgrall
from  types     import  TupleType
from  syslog    import  syslog, LOG_ERR, LOG_INFO
from  traceback import  extract_tb, extract_stack, format_list
from  re        import  sub, split
from  errno     import  EINTR, ECONNRESET
from  md5       import  new as md5new
from  select    import  select, error as select_error
from  time      import  sleep
from  random    import  randrange, random
from  signal    import  alarm, SIGINT, SIGALRM
from  popen2    import  Popen4

# some global vars for some utilities
global mpd_my_id, mpd_signum, mpd_my_hostname, mpd_procedures_to_trace
mpd_my_id = ''
mpd_procedures_to_trace = []
mpd_my_hostname = ''
mpd_signum = 0
# mpd_signum can be set by mpd_handle_signal to indicate which signal was recently caught;
# this can be useful below to pop out of loops that ordinarily continue after sigs
# NOTE: mpd_handle_signal must be called by the user, e.g. in his own signal handler

def mpd_set_my_id(myid=''):
    global mpd_my_id
    mpd_my_id = myid

def mpd_get_my_id():
    global mpd_my_id
    return(mpd_my_id)

def mpd_handle_signal(signum,frame):
    global mpd_signum
    mpd_signum = signum

def mpd_print(*args):
    global mpd_my_id
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    printLine = '%s (%s %d): ' % (mpd_my_id,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    print printLine
    stdout.flush()
    syslog(LOG_INFO,printLine)

def mpd_print_tb(*args):
    global mpd_my_id
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    stack = extract_stack()
    stack.reverse()
    stack = stack[1:]
    printLine = '%s (%s %d):' % (mpd_my_id,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    printLine += '\n  mpdtb:\n'
    for line in format_list(stack):
        line = sub(r'\n.*','',line)
        splitLine = split(',',line)
        splitLine[0] = sub('  File "(.*)"',lambda mo: mo.group(1),splitLine[0])
        splitLine[1] = sub(' line ','',splitLine[1])
        splitLine[2] = sub(' in ','',splitLine[2])
        printLine = printLine + '    %s,  %s,  %s\n' % tuple(splitLine)
    print printLine
    stdout.flush()
    syslog(LOG_INFO,printLine)

def mpd_uncaught_except_tb(arg1,arg2,arg3):
    global mpd_my_id
    if mpd_my_id:
        errstr = '%s: ' % (mpd_my_id)
    else:
        errstr = ''
    errstr += 'mpd_uncaught_except_tb handling:\n'
    errstr += '  %s: %s\n' % (arg1,arg2)
    tb = extract_tb(arg3)
    tb.reverse()
    for tup in tb:
        # errstr += '    file %s  line# %i  procedure %s\n        %s\n' % (tup)
        errstr += '    %s  %i  %s\n        %s\n' % (tup)
    print errstr
    syslog(LOG_ERR, errstr)

def mpd_set_procedures_to_trace(procs):
    global mpd_procedures_to_trace
    mpd_procedures_to_trace = procs

def mpd_trace_calls(frame,event,args):
    global mpd_my_id, mpd_procedures_to_trace
    if frame.f_code.co_name not in mpd_procedures_to_trace:
        return None
    args_info = apply(inspect.formatargvalues,inspect.getargvalues(frame))
    print '%s: ENTER %s in %s at line %d; ARGS=%s' % \
          (mpd_my_id,frame.f_code.co_name,frame.f_code.co_filename,frame.f_lineno,args_info)
    return mpd_trace_returns

def mpd_trace_returns(frame,event,args):
    global mpd_my_id
    if event == 'return':
        print '%s: EXIT %s at line %d ' % (mpd_my_id,frame.f_code.co_name,frame.f_lineno)
        return None
    else:
        return mpd_trace_returns

def mpd_sockpair():
    sock1 = MPDSock()
    rc = sock1.sock.bind(('',0))
    rc = sock1.sock.listen(5)
    port1 = sock1.sock.getsockname()[1]
    sock2 = MPDSock()
    rc = sock2.sock.connect(('',port1))
    (sock3,addr) = sock1.sock.accept()
    sock3 = MPDSock(sock=sock3)
    sock1.close()
    return (sock2,sock3)

def mpd_which(execName):
    for d in environ['PATH'].split(':'):
        fpn = d + '/' + execName
        if path.isdir(fpn):  # follows symlinks; dirs can have execute permission
            continue
        if access(fpn,X_OK):    # NOTE access works based on real uid (not euid)
            return fpn
    return ''

def mpd_check_python_version():
    # version_info: (major,minor,micro,releaselevel,serial)
    if (version_info[0] < 2)  or (version_info[0] == 2 and version_info[1] < 2):
        return version_info
    return 0

def mpd_version():
    return (1,0,0,'May, 2005 release')  # major, minor, micro, special

def mpd_get_my_username():
    return getpwuid(getuid())[0]    #### instead of environ['USER']

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

def mpd_same_ips(host1,host2):    # hosts may be names or IPs
    try:
        ips1 = socket.gethostbyname_ex(host1)[2]    # may fail if invalid host
        ips2 = socket.gethostbyname_ex(host2)[2]    # may fail if invalid host
    except:
        return 0
    for ip1 in ips1:
        for ip2 in ips2:
            if ip1 == ip2:
                return 1
    return 0

def mpd_read_nbytes(fd,nbytes):
    global mpd_signum
    rv = 0
    while 1:
        try:
            rv = read(fd,nbytes)
            break
        except os_error, errinfo:
            if errinfo[0] == EINTR:
                if mpd_signum == SIGINT  or  mpd_signum == SIGALRM:
                    break
                else:
                    continue
            elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                break
            else:
                mpd_print(1, 'read error: %s' % strerror(errinfo[0]))
                break
        except KeyboardInterrupt, errinfo:
            break
        except Exception, errinfo:
            mpd_print(1, 'other error after read %s :%s:' % ( errinfo.__class__, errinfo) )
            break
    return rv

def mpd_get_groups_for_username(username):
    userGroups = [getpwnam(username)[3]]  # default group for the user
    allGroups = getgrall();
    for group in allGroups:
        if username in group[3]  and  group[2] not in userGroups:
            userGroups.append(group[2])
    return userGroups


class MPDSock(object):
    def __init__(self,family=socket.AF_INET,type=socket.SOCK_STREAM,proto=0,
                 sock=None,name=''):
        if sock:
            self.sock = sock
        else:
            self.sock = socket.socket(family=family,type=type,proto=proto)
        self.name = name
        ## used this when inherited from socket.socket (only works with py 2.3+)
        ## socket.socket.__init__(self,family=family,type=type,proto=proto,_sock=sock)
    def close(self):
        self.sock.close()
    def sendall(self,data):
        self.sock.sendall(data)
    def getsockname(self):
        return self.sock.getsockname()
    def fileno(self):
        return self.sock.fileno()
    def connect(self,*args):
        self.sock.connect(*args)
    def accept(self,name='accepter'):
        global mpd_signum
        newsock = 0
        newaddr = 0
        while 1:
            try:
                mpd_signum = 0
                (newsock,newaddr) = self.sock.accept()
                break
            except socket.error, errinfo:
                if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                    if mpd_signum == SIGINT  or  mpd_signum == SIGALRM:
                        break
                    else:
                        continue
                elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                    break
                else:
                    print '%s: accept error: %s' % (mpd_my_id,strerror(errinfo[0]))
                    break
            except Exception, errinfo:
                print '%s: failure doing accept : %s : %s' % \
                      (mpd_my_id,errinfo.__class__,errinfo)
                break
        if newsock:
            newsock = MPDSock(sock=newsock,name=name)    # turn new socket into an MPDSock
        return (newsock,newaddr)
    def recv(self,nbytes):
        global mpd_signum
        data = 0
        while 1:
            try:
                mpd_signum = 0
                data = self.sock.recv(nbytes)
                break
            except socket.error, errinfo:
                if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                    if mpd_signum == SIGINT  or  mpd_signum == SIGALRM:
                        break
                    else:
                        continue
                elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                    break
                else:
                    print '%s: recv error: %s' % (mpd_my_id,strerror(errinfo[0]))
                    break
            except Exception, errinfo:
                print '%s: failure doing recv %s :%s:' % \
                      (mpd_my_id,errinfo.__class__,errinfo)
                break
        return data
    def recv_dict_msg(self,timeout=None):
        global mpd_signum
        msg = {}
        readyToRecv = 0
        if timeout:
            try:
                mpd_signum = 0
                (readyToRecv,unused1,unused2) = select([self.sock],[],[],timeout)
            except select_error, errinfo:
                if errinfo[0] == EINTR:
                    if mpd_signum == SIGINT  or  mpd_signum == SIGALRM:
                        pass   # assume timedout; returns {} below
                else:
                    print 'select error: %s' % strerror(errinfo[0])
            except KeyboardInterrupt, errinfo:
                # print 'recv_dict_msg: keyboard interrupt during select'
                return msg
            except Exception, errinfo:
                print 'recv_dict_msg: exception during select %s :%s:' % \
                      ( errinfo.__class__, errinfo)
                return msg
        else:
            readyToRecv = 1
        if readyToRecv:
            try:
	        # socket.error: (104, 'Connection reset by peer')
                pickledLen = self.sock.recv(8)
                if pickledLen:
                    pickledLen = int(pickledLen)
                    pickledMsg = ''
                    lenLeft = pickledLen
                    while lenLeft:
                        recvdMsg = self.sock.recv(lenLeft)
                        pickledMsg += recvdMsg
                        lenLeft -= len(recvdMsg)
                    msg = loads(pickledMsg)
            except socket.error, data:
                if data[0] == EINTR:
                    return msg
                else:
                    mpd_print_tb(1, 'recv_dict_msg: socket error: data=:%s:' % (data) )
            except StandardError, errmsg:    # any built-in exceptions
                mpd_print_tb(1, 'recv_dict_msg: errmsg=:%s:' % (errmsg) )
            except Exception, errmsg:
                mpd_print_tb(1, 'recv_dict_msg failed on sock %s errmsg=:%s:' % \
                             (self.name,errmsg) )
        return msg
    def recv_char_msg(self):
        return self.recv_one_line()  # use leading len later
    def recv_one_line(self):
        msg = ''
        try:
            c = self.sock.recv(1)
        except Exception, errmsg:
            c = ''
            msg = ''
            mpd_print_tb(1, 'recv_char_msg: errmsg=:%s:' % (errmsg) )
        if c:
            while c != '\n':
                msg += c
                try:
                    c = self.sock.recv(1)
                except Exception, errmsg:
                    c = ''
                    msg = ''
                    mpd_print_tb(1, 'recv_char_msg: errmsg=:%s:' % (errmsg) )
                    break
            msg += c
        return msg
    def send_dict_msg(self,msg,errprint=1):
        pickledMsg = dumps(msg) 
        try:
            self.sendall( "%08d%s" % (len(pickledMsg),pickledMsg) )
        except Exception, errmsg:
            if errprint:
                mpd_print_tb(1, 'send_dict_msg: sock=%s errmsg=:%s:' % (self.name,errmsg) )
    def send_char_msg(self,msg,errprint=1):
        try:
            self.sock.sendall(msg)
        except Exception, errmsg:
            if errprint:
                mpd_print_tb(1, 'send_char_msg: sock=%s errmsg=:%s:' % (self.name,errmsg) )

class MPDListenSock(MPDSock):
    def __init__(self,host='',port=0,filename='',listen=5,name='listener',**kargs):
        MPDSock.__init__(self,name=name,**kargs)
        self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        if filename:
            self.sock.bind(filename)
        else:
            self.sock.bind((host,port))
        self.sock.listen(listen)

class MPDStreamHandler(object):
    def __init__(self):
        self.activeStreams = {}
    def set_handler(self,stream,handler,args=()):
        self.activeStreams[stream] = (handler,args)
    def del_handler(self,stream):
        if self.activeStreams.has_key(stream):
            del self.activeStreams[stream]
    def close_all_active_streams(self):
        for stream in self.activeStreams.keys():
            del self.activeStreams[stream]
            stream.close()
    def handle_active_streams(self,streams=None,timeout=0.1):
        global mpd_signum
        while 1:
            if streams:
                streamsToSelect = streams
            else:
                streamsToSelect = self.activeStreams.keys()
            readyStreams = []
            try:
                mpd_signum = 0
                (readyStreams,unused1,unused2) = select(streamsToSelect,[],[],timeout)
                break
            except select_error, errinfo:
                if errinfo[0] == EINTR:
                    if mpd_signum == SIGINT  or  mpd_signum == SIGALRM:
                        break
                    else:
                        continue
                else:
                    print 'select error: %s' % strerror(errinfo[0])
                    return (-1,strerror(errinfo[0]))
            except KeyboardInterrupt, errinfo:
                # print 'handle_active_streams: keyboard interrupt during select'
                return (-1,errinfo.__class__,errinfo)
            except Exception, errinfo:
                print 'handle_active_streams: exception during select %s :%s:' % \
                      ( errinfo.__class__, errinfo)
                return (-1,errinfo.__class__,errinfo)
        for stream in readyStreams:
            if self.activeStreams.has_key(stream):
                (handler,args) = self.activeStreams[stream]
                handler(stream,*args)
            else:
                print '*** OOPS, unknown stream in handle_active_streams'
        return (len(readyStreams),0)  #  len >= 0

class MPDRing(object):
    def __init__(self,listenSock=None,streamHandler=None,secretword='',
                 myIfhn='',entryIfhn='',entryPort=0):
        if not streamHandler:
            mpd_print(1, "must supply handler for new conns in ring")
            exit(-1)
        if not listenSock:
            mpd_print(1, "must supply listenSock for new ring")
            exit(-1)
        if not myIfhn:
            mpd_print(1, "must supply myIfhn for new ring")
            exit(-1)
        self.secretword = secretword
        self.myIfhn     = myIfhn
        self.generation = 0
        self.listenSock = listenSock
        self.listenPort = self.listenSock.sock.getsockname()[1]
        self.streamHandler = streamHandler
        self.streamHandler.set_handler(self.listenSock,self.handle_ring_listener_connection)
        self.entryIfhn = entryIfhn
        self.entryPort = entryPort
        self.lhsIfhn = ''
        self.lhsPort = 0
        self.rhsIfhn = ''
        self.rhsPort = 0
        self.lhsSock = 0
        self.rhsSock = 0
        self.lhsHandler = None
        self.rhsHandler = None
    def create_single_mem_ring(self,ifhn='',port=0,lhsHandler=None,rhsHandler=None):
        self.lhsSock,self.rhsSock = mpd_sockpair()
        self.lhsIfhn = ifhn
        self.lhsPort = port
        self.rhsIfhn = ifhn
        self.rhsPort = port
        self.lhsHandler = lhsHandler
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
        self.rhsHandler = rhsHandler
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
    def reenter_ring(self,entryIfhn='',entryPort=0,lhsHandler='',rhsHandler='',ntries=5):
        rc = -1
        numTries = 0
        while rc < 0  and  numTries < ntries:
            numTries += 1
            rc = self.enter_ring(entryIfhn=entryIfhn,entryPort=entryPort,
                                 lhsHandler=lhsHandler,rhsHandler=rhsHandler,ntries=1)
            sleep(random())
        return rc
    def enter_ring(self,entryIfhn='',entryPort=0,lhsHandler='',rhsHandler='',ntries=1):
        if not lhsHandler  or  not rhsHandler:
            print 'missing handler for enter_ring'
            exit(-1)
        if not entryIfhn:
            entryIfhn = self.entryIfhn
        if not entryPort:
            entryPort = self.entryPort
        self.generation += 1
        if not entryIfhn:
            self.create_single_mem_ring(ifhn=self.myIfhn,
                                        port=self.listenPort,
                                        lhsHandler=lhsHandler,
                                        rhsHandler=rhsHandler)
        else:
            rv = self.connect_lhs(lhsIfhn=entryIfhn,
                                  lhsPort=entryPort,
                                  lhsHandler=lhsHandler,
                                  numTries=ntries)
            if rv[0] <= 0:  # connect failed with problem
                mpd_print(1,"lhs connect failed")
                return -1
            if rv[1]:  # rhsifhn and rhsport
                rhsIfhn = rv[1][0]
                rhsPort = rv[1][1]
            else:
                mpd_print(1,"did not recv rhs host&port from lhs")
                return -1
            rv = self.connect_rhs(rhsIfhn=rhsIfhn,
                                  rhsPort=rhsPort,
                                  rhsHandler=rhsHandler,
                                  numTries=ntries)
            if rv[0] <=  0:  # connect did not succeed; may try again
                mpd_print(1,"rhs connect failed")
                return -1
        return 0
    def connect_lhs(self,lhsIfhn='',lhsPort=0,lhsHandler=None,numTries=1):
        if not lhsHandler:
            mpd_print(1, "must supply handler for lhs in ring")
            return (-1,None)
        if not lhsIfhn:
            mpd_print(1, "must supply host for lhs in ring")
            return (-1,None)
        self.lhsIfhn = lhsIfhn
        if not lhsPort:
            mpd_print(1, "must supply port for lhs in ring")
            return (-1,None)
        self.lhsPort = lhsPort
        numConnTries = 0
        while numConnTries < numTries:
            numConnTries += 1
            self.lhsSock = MPDSock(name='lhs')
            try:
                self.lhsSock.connect((self.lhsIfhn,self.lhsPort))
            except socket.error, errinfo:
                print '%s: conn error in connect_lhs: %s' % (mpd_my_id,strerror(errinfo[0]))
                self.lhsSock.close()
                self.lhsSock = 0
                sleep(random())
                continue
            break
        if not self.lhsSock  or  numConnTries > numTries:
            mpd_print(1,'failed to connect to lhs at %s %d' % (self.lhsIfhn,self.lhsPort))
            return (0,None)
        msgToSend = { 'cmd' : 'request_to_enter_as_rhs', 'ifhn' : self.myIfhn,
                      'port' : self.listenPort,
                      'mpd_version' : mpd_version() }
        self.lhsSock.send_dict_msg(msgToSend)
        msg = self.lhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'challenge') \
        or (not msg.has_key('randnum')) \
        or (not msg.has_key('generation')):
            mpd_print(1,'invalid challenge from %s %d: %s' % \
                      (self.lhsIfhn,self.lhsPort,msg) )
            return (-1,None)
        if msg['generation'] < self.generation:
            mpd_print(1,'bad generation')
            return(-1,'bad_generation')  # RMB: try again here later
        response = md5new(''.join([self.secretword,msg['randnum']])).digest()
        msgToSend = { 'cmd' : 'challenge_response', 'response' : response,
                      'ifhn' : self.myIfhn, 'port' : self.listenPort }
        self.lhsSock.send_dict_msg(msgToSend)
        msg = self.lhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'OK_to_enter_as_rhs'):
            mpd_print(1,'NOT OK to enter ring; one likely cause: mismatched secretwords')
            return (-1,None)
        self.lhsHandler = lhsHandler
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
        if msg.has_key('rhsifhn') and msg.has_key('rhsport'):
            return (1,(msg['rhsifhn'],msg['rhsport']))
        else:
            return (1,None)
    def connect_rhs(self,rhsIfhn='',rhsPort=0,rhsHandler=None,numTries=1):
        if not rhsHandler:
            mpd_print(1, "must supply handler for rhs in ring")
            return (-1,None)
        if not rhsIfhn:
            mpd_print(1, "must supply host for rhs in ring")
            return (-1,None)
        self.rhsIfhn = rhsIfhn
        if not rhsPort:
            mpd_print(1, "must supply port for rhs in ring")
            return (-1,None)
        self.rhsPort = rhsPort
        numConnTries = 0
        while numConnTries < numTries:
            numConnTries += 1
            self.rhsSock = MPDSock(name='rhs')
            try:
                self.rhsSock.connect((self.rhsIfhn,self.rhsPort))
            except socket.error, errinfo:
                print '%s: conn error in connect_rhs: %s' % (mpd_my_id,strerror(errinfo[0]))
                self.rhsSock.close()
                self.rhsSock = 0
                sleep(random())
                continue
            break
        if not self.rhsSock or numConnTries > numTries:
            mpd_print(1,'failed to connect to rhs at %s %d' % (self.rhsIfhn,self.rhsPort))
            return (0,None)
        msgToSend = { 'cmd' : 'request_to_enter_as_lhs', 'ifhn' : self.myIfhn,
                      'port' : self.listenPort,
                      'mpd_version' : mpd_version() }
        self.rhsSock.send_dict_msg(msgToSend)
        msg = self.rhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'challenge') \
        or (not msg.has_key('randnum')) \
        or (not msg.has_key('generation')):
            mpd_print(1,'invalid challenge from %s %d: %s' % (self.rhsIfhn,rhsPort,msg) )
            return (-1,None)
        if msg['generation'] < self.generation:
            mpd_print(1,'bad generation')
            return(-1,'bad_generation')  # RMB: try again here later
        response = md5new(''.join([self.secretword,msg['randnum']])).digest()
        msgToSend = { 'cmd' : 'challenge_response', 'response' : response,
                      'ifhn' : self.myIfhn, 'port' : self.listenPort }
        self.rhsSock.send_dict_msg(msgToSend)
        msg = self.rhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'OK_to_enter_as_lhs'):
            mpd_print(1,'NOT OK to enter ring; one likely cause: mismatched secretwords')
            return (-1,None)
        self.rhsHandler = rhsHandler
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
        if msg.has_key('lhsifhn') and msg.has_key('lhsport'):
            return (1,(msg['lhsifhn'],msg['lhsport']))
        else:
            return (1,None)
    def accept_lhs(self,lhsHandler=None):
        self.lhsHandler = lhsHandler
        newsock = self.handle_ring_listener_connection(self.listenSock)
        self.handle_lhs_challenge_response(newsock)
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
    def accept_rhs(self,rhsHandler=None):
        self.rhsHandler = rhsHandler
        newsock = self.handle_ring_listener_connection(self.listenSock)
        self.handle_rhs_challenge_response(newsock)
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
    def handle_ring_listener_connection(self,sock):
        randHiRange = 10000
        (newsock,newaddr) = sock.accept()
        newsock.name = 'candidate_to_enter_ring'
        msg = newsock.recv_dict_msg()
        if (not msg) or \
           (not msg.has_key('cmd')) or (not msg.has_key('ifhn')) or  \
           (not msg.has_key('port')):
            mpd_print(1, 'INVALID msg from new connection :%s: msg=:%s:' % (newaddr,msg) )
            newsock.close()
            return None
        if msg.has_key('mpd_version'):  # ping, etc may not have one
            if msg['mpd_version'] != mpd_version():
                msgToSend = { 'cmd' : 'entry_rejected_bad_mpd_version',
                              'your_version' : msg['mpd_version'],
                              'my_version' : mpd_version() }
                newsock.send_dict_msg(msgToSend)
                newsock.close()
                return None
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        newsock.correctChallengeResponse = \
                         md5new(''.join([self.secretword,randNumStr])).digest()
        msgToSend = { 'cmd' : 'challenge', 'randnum' : randNumStr,
                      'generation' : self.generation }
        newsock.send_dict_msg(msgToSend)
        if msg['cmd'] == 'request_to_enter_as_lhs':
            self.streamHandler.set_handler(newsock,self.handle_lhs_challenge_response)
            newsock.name = 'candidate_for_lhs_challenged'
            return newsock
        elif msg['cmd'] == 'request_to_enter_as_rhs':
            self.streamHandler.set_handler(newsock,self.handle_rhs_challenge_response)
            newsock.name = 'candidate_for_rhs_challenged'
            return newsock
        elif msg['cmd'] == 'ping':
            # already sent challenge instead of ack
            newsock.close()
            return None
        else:
            mpd_print(1, 'INVALID msg from new connection :%s:  msg=:%s:' % (newaddr,msg) )
            newsock.close()
            return None
        return None
    def handle_lhs_challenge_response(self,sock):
        msg = sock.recv_dict_msg()
        if (not msg)   or  \
           (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
           (not msg.has_key('ifhn'))  or  (not msg.has_key('port'))  or  \
           (not msg['response'] == sock.correctChallengeResponse):
            mpd_print(1, 'INVALID msg for lhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        else:
            msgToSend = { 'cmd' : 'OK_to_enter_as_lhs' }
            sock.send_dict_msg(msgToSend)
            if self.lhsSock:
                self.streamHandler.del_handler(self.lhsSock)
                self.lhsSock.close()
            self.lhsSock = sock
            self.lhsIfhn = msg['ifhn']
            self.lhsPort = int(msg['port'])
            self.streamHandler.set_handler(self.lhsSock,self.lhsHandler)
            self.lhsSock.name = 'lhs'
    def handle_rhs_challenge_response(self,sock):
        msg = sock.recv_dict_msg()
        if (not msg)   or  \
           (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
           (not msg.has_key('ifhn'))  or  (not msg.has_key('port')):
            mpd_print(1, 'INVALID msg for rhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        elif msg['response'] != sock.correctChallengeResponse:
            mpd_print(1, 'INVALID response in rhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        elif msg['response'] == 'bad_generation':
            mpd_print(1, 'someone failed entering my ring gen=%d msg=%s' % \
                      (self.generation,msg) )
            self.streamHandler.del_handler(sock)
            sock.close()
        else:
            msgToSend = { 'cmd' : 'OK_to_enter_as_rhs', 'rhsifhn' : self.rhsIfhn,
                          'rhsip' : self.rhsIfhn, 'rhsport' : self.rhsPort }
            sock.send_dict_msg(msgToSend)
            if self.rhsSock:
                self.streamHandler.del_handler(self.rhsSock)
                self.rhsSock.close()
            self.rhsSock = sock
            self.rhsIfhn   = msg['ifhn']
            self.rhsPort = int(msg['port'])
            self.streamHandler.set_handler(self.rhsSock,self.rhsHandler)
            self.rhsSock.name = 'rhs'

class MPDConsServerSock(MPDListenSock):
    def __init__(self,filetemplate='/tmp/mpd2.console_',name='console_listen',**kargs):
        if environ.has_key('MPD_CON_EXT'):
            conExt = '_' + environ['MPD_CON_EXT']
        else:
            conExt = ''
        self.conListenName = filetemplate + mpd_get_my_username() + conExt
        consoleAlreadyExists = 0
        if access(self.conListenName,R_OK):    # if console is there, see if mpd is listening
            tempSock = socket.socket(socket.AF_UNIX,socket.SOCK_STREAM)  # note: UNIX sock
            try:
                tempSock.connect(self.conListenName)
                consoleAlreadyExists = 1
            except Exception, errmsg:
                tempSock.close()
                unlink(self.conListenName)
        if consoleAlreadyExists:
            print 'An mpd is already running with console at %s on %s. ' % \
                  (self.conListenName, socket.gethostname())
            print 'Start mpd with the -n option for a second mpd on same host.'
            syslog(LOG_ERR,"%s: exiting; an mpd is already using the console" % \
                   (mpd_my_id))
            exit(-1)
        MPDListenSock.__init__(self,family=socket.AF_UNIX,type=socket.SOCK_STREAM,
                               filename=self.conListenName,listen=1,name=name)

class MPDConsClientSock(MPDSock):
    def __init__(self,filetemplate='/tmp/mpd2.console_',name='console_to_mpd',**kargs):
        MPDSock.__init__(self)
        if environ.has_key('MPD_UNIX_SOCKET'):
            conFD = int(environ['MPD_UNIX_SOCKET'])
            self.sock = socket.fromfd(conFD,socket.AF_UNIX,socket.SOCK_STREAM)
            self.sock = MPDSock(sock=self.sock,name=name)
            osclose(conFD)
        else:
            self.sock = MPDSock(family=socket.AF_UNIX,type=socket.SOCK_STREAM,name=name)
            if environ.has_key('MPD_CON_EXT'):
                conExt = '_' + environ['MPD_CON_EXT']
            else:
                conExt = ''
            conName = filetemplate + mpd_get_my_username() + conExt
            oldAlarmTime = alarm(8)
            try:
                self.sock.connect(conName)
            except Exception, errmsg:
                self.sock.close()
                self.sock = 0
            alarm(oldAlarmTime)
            if self.sock:
                # this is done by mpdroot otherwise
                msgToSend = 'realusername=%s\n' % mpd_get_my_username()
                self.sock.send_char_msg(msgToSend)
        if not self.sock:
            print '%s: cannot connect to local mpd (%s); possible causes:' % \
                  (mpd_my_id,conName)
            print '  1. no mpd is running on this host'
            print '  2. an mpd is running but was started without a "console" (-n option)'
            exit(-1)

class MPDParmDB(dict):
    def __init__(self,orderedSources=[]):
        dict.__init__(self)
        self.orderedSources = orderedSources
        self.db = {}
        for src in orderedSources:  # highest to lowest
            self.db[src] = {}
    def __setitem__(self,sk_tup,val):
        if type(sk_tup) != TupleType  or  len(sk_tup) != 2:
            mpd_print(1,"must use a 2-tuple as key in a parm db; invalid: %s" % (sk_tup) )
            exit(-1)
        s,k = sk_tup
        for src in self.orderedSources:
            if src == s:
                self.db[src][k] = val
                break
        else:
            mpd_print(1,"invalid src specified for insert into parm db; src=%s" % (src) )
            exit(-1)
    def __getitem__(self,key):
        for src in self.orderedSources:
            if self.db[src].has_key(key):
                return self.db[src][key]
        raise KeyError, "key %s not found in parm db" % (key)
    def has_key(self,key):
        for src in self.orderedSources:
            if self.db[src].has_key(key):
                return 1
        return 0
    def printall(self):
        print "MPDRUN's PARMDB; values from all sources:"
        for src in self.orderedSources:
            print '  %s (source)' % (src)
            for key in self.db[src].keys():
                print '    %s = %s' % (key,self.db[src][key])
    def printdef(self):
        print "MPDRUN's PARMDB; default values only:"
        printed = {}
        for src in self.orderedSources:
            for key in self.db[src]:
                if not printed.has_key(key):
                    printed[key] = 1
                    print '  %s  %s = %s' % (src,key,self.db[src][key])

class MPDTest(object):
    def __init__(self):
        pass
    def run(self,cmd='',
            expIn = '',
            chkEC=0,expEC=0,
            chkOut=0,expOut='',ordOut=0,
            grepOut=0,
            exitOnFail=1):
        rv = {}
        if chkOut and grepOut:
            print "grepOut and chkOut are mutually exclusive"
            exit(-1)
        runner = Popen4(cmd)
        rv['pid'] = runner.pid
        if expIn:
            runner.tochild.write(expIn)
        runner.tochild.close()
        outLines = []
        for line in runner.fromchild:
            outLines.append(line[:-1])    # strip newlines
        rv['EC'] = runner.wait()
        rv['OUT'] = outLines[:]
        if chkEC  and  expEC != rv['EC']:
            print "bad exit code from test: %s" % (cmd)
            print "   expected exitcode=%d ; got %d" % (expEC,rv['EC'])
            print "output from cmd:"
            for line in outLines:
                print line
            if exitOnFail:
                exit(-1)
        if chkOut:
            orderOK = 1
            expOut = expOut.split('\n')[:-1]  # leave off trailing ''
            for line in outLines[:]:    # copy of outLines
                if line in expOut:
                    if ordOut and line != expOut[0]:
                        orderOK = 0
                        break  # count rest of outLines as bad
                    expOut.remove(line)
                    outLines.remove(line)
            if not orderOK:
                print "lines out of order in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    exit(-1)
            if expOut:
                print "some required lines not found in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    exit(-1)
            if outLines:
                print "extra lines in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    exit(-1)
        elif grepOut:
            foundCnt = 0
            for expLine in expOut:
                for outLine in outLines:
                    if outLine.find(expLine) >= 0:
                        foundCnt += 1
            if foundCnt < len(expOut):
                print "some lines not matched for test: %s" % (cmd)
                for line in outLines:
                     print line
                if exitOnFail:
                    exit(-1)
        return rv


# code for testing

def _handle_msg(sock):
    msg = sock.recv_dict_msg()
    print 'recvd msg=:%s:' % (msg)

if __name__ == '__main__':
    sh = MPDStreamHandler()
    (tsock1,tsock2) = mpd_sockpair()
    tsock1.name = 'tsock1_connected_to_tsock2'
    sh.set_handler(tsock1,_handle_msg)
    tsock2.send_dict_msg( {'msgtype' : 'hello'} )
    sh.handle_active_streams()
    # just to demo a listen sock
    lsock = MPDListenSock('',9999,name='listen_sock')
    print lsock.name, lsock.getsockname()[1]

    ### import sys
    ### sys.excepthook = mpd_uncaught_except_tb
    ### i = 1/0
