#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpd [--host=<host> --port=<portnum>] [--noconsole]
           [--trace] [--echo] [--daemon] [--bulletproof] --ncpus=<ncpus>
           [--ifhn=<interface-hostname>] [--listenport=<listenport>]
           [--pid=<pidfilename>]

Some long parameter names may be abbreviated to their first letters by using
  only one hyphen and no equal sign:
     mpd -h donner -p 4268 -n
  is equivalent to
     mpd --host=donner --port=4268 --noconsole

--host and --port must be specified together; they tell the new mpd where
  to enter an existing ring;  if they are omitted, the new mpd forms a
  stand-alone ring that other mpds may enter later
--noconsole is useful for running 2 mpds on the same machine; only one of
  them will accept mpd commands
--trace yields lots of traces thru mpd routines; currently too verbose
--echo causes the mpd echo its listener port by which other mpds may connect
--daemon causes mpd to run backgrounded, with no controlling tty
--bulletproof says to turn bulletproofing on (experimental)
--ncpus indicates how many cpus are on the local host; used for starting processes
--ifhn specifies an alternate interface hostname for the host this mpd is running on;
  e.g. may be used to specify the alias for an interface other than default
--listenport specifies a port for this mpd to listen on; by default it will
  acquire one from the system
--pid=filename writes the mpd pid into the specified file, or 
--pid alone writes it into /var/run/mpd.pid

A file named .mpd.conf file must be present in the user's home directory
  with read and write access only for the user, and must contain at least
  a line with MPD_SECRETWORD=<secretword>

To run mpd as root, install it while root and instead of a .mpd.conf file
use mpd.conf (no leading dot) in the /etc directory.' 
"""
from  time    import  ctime
from  mpdlib  import  mpd_version
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision$"
__version__ += "  " + str(mpd_version())
__credits__ = ""


import sys    # so I can chg excepthook, stdout, etc.

from  sys         import  argv, exit, settrace
from  os          import  environ, getpid, fork, getuid, setsid, chdir, path, \
                          umask, stat, dup2, unlink, setpgrp, fdopen, \
                          setreuid, setregid, setgroups, waitpid, WNOHANG, kill
from  os          import  open  as osopen, close as osclose, \
                          O_CREAT, O_WRONLY, O_EXCL
from  socket      import  gethostname, gethostbyname_ex
from  signal      import  signal, SIGCHLD, SIGHUP, SIG_IGN, SIGKILL
from  re          import  sub
from  pwd         import  getpwnam
from  atexit      import  register
from  cPickle     import  dumps
from  types       import  ClassType
from  random      import  seed
from  time        import  sleep
from  syslog      import  openlog, syslog, closelog, LOG_DAEMON, LOG_INFO, LOG_ERR
from  md5         import  new as md5new
from  mpdlib      import  mpd_set_my_id, mpd_check_python_version, mpd_sockpair, \
                          mpd_print, mpd_get_my_username, \
                          mpd_get_groups_for_username, mpd_uncaught_except_tb, \
                          mpd_set_procedures_to_trace, mpd_trace_calls, \
                          MPDSock, MPDListenSock, MPDConsServerSock, \
                          MPDStreamHandler, MPDRing, MPDParmDB
from  mpdman      import  MPDMan


def sigchld_handler(signum,frame):
    done = 0
    while not done:
        try:
            (pid,status) = waitpid(-1,WNOHANG)
            if pid == 0:    # no existing child process is finished
                done = 1
        except:    # no more child processes to be waited for
            done = 1
            
class MPD(object):
    def __init__(self):
        self.myHost        = gethostname()
        try:
            hostinfo = gethostbyname_ex(self.myHost)
            self.myIfhn = hostinfo[2][0]    # chgd below when I get the real value
        except:
            print 'mpd failed: gethostbyname_ex failed for %s' % (self.myHost)
            exit(-1)
    def run(self):
        openlog("mpd",0,LOG_DAEMON)
        syslog(LOG_INFO,"mpd starting; no mpdid yet")
        sys.excepthook = mpd_uncaught_except_tb
        self.parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
        self.parmsToOverride = {
                                 'MPD_SECRETWORD'       :  '',
                                 'MPD_MY_IFHN'          :  self.myIfhn,
                                 'MPD_ENTRY_IFHN'       :  '',
                                 'MPD_ENTRY_PORT'       :  0,
                                 'MPD_NCPUS'            :  1,
                                 'MPD_LISTEN_PORT'      :  0,
                                 'MPD_TRACE_FLAG'       :  0,
                                 'MPD_CONSOLE_FLAG'     :  1,
                                 'MPD_ECHO_PORT_FLAG'   :  0,
                                 'MPD_DAEMON_FLAG'      :  0,
                                 'MPD_BULLETPROOF_FLAG' :  0,
                                 'MPD_PID_FILENAME'     :  '',
                               }
        for (k,v) in self.parmsToOverride.items():
            self.parmdb[('thispgm',k)] = v
        self.get_parms_from_cmdline()
        self.get_parms_from_rcfile()
        self.get_parms_from_env()
        self.myIfhn = self.parmdb['MPD_MY_IFHN']    # variable for convenience
        self.myPid = getpid()
        self.listenSock = MPDListenSock(name='ring_listen_sock',
                                        port=self.parmdb['MPD_LISTEN_PORT'])
        self.parmdb[('thispgm','MPD_LISTEN_PORT')] = self.listenSock.sock.getsockname()[1]
        self.myId = '%s_%d' % (self.myHost,self.parmdb['MPD_LISTEN_PORT'])
        mpd_set_my_id(myid=self.myId)
        self.streamHandler = MPDStreamHandler()
        self.ring = MPDRing(streamHandler=self.streamHandler,
                            secretword=self.parmdb['MPD_SECRETWORD'],
                            listenSock=self.listenSock,
                            myIfhn=self.myIfhn,
                            entryIfhn=self.parmdb['MPD_ENTRY_IFHN'],
                            entryPort=self.parmdb['MPD_ENTRY_PORT'])
        # setup tracing if requested via args
        if self.parmdb['MPD_TRACE_FLAG']:
            proceduresToTrace = []
            import inspect
            symbolsAndTypes = globals().items() + \
                              inspect.getmembers(self) + \
                              inspect.getmembers(self.ring) + \
                              inspect.getmembers(self.streamHandler)
            for (symbol,symtype) in symbolsAndTypes:
                if symbol == '__init__':  # a problem to trace
                    continue
                if inspect.isfunction(symtype)  or  inspect.ismethod(symtype):
                    # print symbol
                    proceduresToTrace.append(symbol)
            mpd_set_procedures_to_trace(proceduresToTrace)
            settrace(mpd_trace_calls)
        syslog(LOG_INFO,"mpd has mpdid=%s (port=%d)" % (self.myId,self.parmdb['MPD_LISTEN_PORT']) )
        vinfo = mpd_check_python_version()
        if vinfo:
            print "mpd: your python version must be >= 2.2 ; current version is:", vinfo
            exit(-1)
        osclose(0)
        if self.parmdb['MPD_ECHO_PORT_FLAG']:    # do this before becoming a daemon
            print self.parmdb['MPD_LISTEN_PORT']
            sys.stdout.flush()
            ##### RMB: NEXT 2 for debugging
            print >>sys.stderr, self.parmdb['MPD_LISTEN_PORT']
            sys.stderr.flush()
        self.myRealUsername = mpd_get_my_username()
        self.currRingSize = 1    # default
        self.currRingNCPUs = 1   # default
        if environ.has_key('MPD_CON_EXT'):
            self.conExt = '_'  + environ['MPD_CON_EXT']
        else:
            self.conExt = ''
        self.logFilename = '/tmp/mpd2.logfile_' + mpd_get_my_username() + self.conExt
        if self.parmdb['MPD_PID_FILENAME']:
            pidFile = open(self.parmdb['MPD_PID_FILENAME'],'w')
            print >>pidFile, "%d" % (getpid())
            pidFile.close()

        self.conListenSock = 0    # don't want one when I do cleanup for forked daemon procs
        if self.parmdb['MPD_DAEMON_FLAG']:      # see if I should become a daemon with no controlling tty
            rc = fork()
            if rc != 0:   # parent exits; child in background
                exit(0)
            setsid()  # become session leader; no controlling tty
            signal(SIGHUP,SIG_IGN)  # make sure no sighup when leader ends
            ## leader exits; svr4: make sure do not get another controlling tty
            rc = fork()
            if rc != 0:
                exit(0)
            chdir("/")  # free up filesys for umount
            umask(0)
            try:    unlink(self.logFilename)
            except: pass
            logFileFD = osopen(self.logFilename,O_CREAT|O_WRONLY|O_EXCL,0600)
            logFile = fdopen(logFileFD,'w',0)
            sys.stdout = logFile
            sys.stderr = logFile
            print >>sys.stdout, 'logfile for mpd with pid %d' % getpid()
            sys.stdout.flush()
            dup2(logFile.fileno(),sys.__stdout__.fileno())
            dup2(logFile.fileno(),sys.__stderr__.fileno())
        if self.parmdb['MPD_CONSOLE_FLAG']:
            self.conListenSock = MPDConsServerSock()
            self.streamHandler.set_handler(self.conListenSock,
                                           self.handle_console_connection)
        register(self.cleanup)
        seed()
        self.nextJobInt    = 1
        self.activeJobs    = {}
        self.conSock       = 0
        self.allExiting    = 0
        self.exiting       = 0    # for mpdexit
        rc = self.ring.enter_ring(lhsHandler=self.handle_lhs_input,
                                  rhsHandler=self.handle_rhs_input)
        if rc < 0:
            mpd_print(1,"failed to enter ring")
            exit(-1)
        self.pmi_published_names = {}
        signal(SIGCHLD,sigchld_handler)
        if not self.parmdb['MPD_BULLETPROOF_FLAG']:
            #    import profile ; profile.run('self.runmainloop()')
            self.runmainloop()
        else:
            try:
                from threading import Thread
            except:
                print '*** mpd terminating'
                print '    bulletproof option must be able to import threading-Thread'
                exit(-1)
            # may use SIG_IGN on all but SIGCHLD and SIGHUP (handled above)
            while 1:
                mpdtid = Thread(target=self.runmainloop)
                mpdtid.start()
                # signals must be handled in main thread; thus we permit timeout of join
                while mpdtid.isAlive():
                    mpdtid.join(2)   # come out sometimes and handle signals
                if self.allExiting:
                    break
                if self.conSock:
                    msgToSend = { 'cmd' : 'restarting_mpd' }
                    self.conSock,msgToSend.send_dict_msg(msgToSend)
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
    def runmainloop(self):
        # Main Loop
        while 1:
            rv = self.streamHandler.handle_active_streams(timeout=8.0)
            if rv[0] < 0:
                if type(rv[1]) == ClassType  and  rv[1] == KeyboardInterrupt: # ^C
                    exit(-1)
            if self.exiting  or  self.allExiting:
                break
    def usage(self):
        print __doc__
        print "This version of mpd is", mpd_version()
        exit(-1)
    def cleanup(self):
        try:
            mpd_print(0, "CLEANING UP" )
            syslog(LOG_INFO,"mpd ending mpdid=%s (inside cleanup)" % (self.myId) )
            if self.conListenSock:    # only del if I created
                unlink(self.conListenSock.conListenName)
            closelog()
        except:
            pass
    def get_parms_from_env(self):
        pass  # none right now
    def get_parms_from_rcfile(self):
        if getuid() == 0:    # if ROOT
            parmsRCFilename = '/etc/mpd.conf'
        else:
            parmsRCFilename = environ['HOME'] + '/.mpd.conf'
        try:
            mode = stat(parmsRCFilename)[0]
        except:
            mode = ''
        if not mode:
            print 'configuration file %s not found' % (parmsRCFilename)
            print 'A file named .mpd.conf file must be present in the user\'s home'
            print 'directory (/etc/mpd.conf if root) with read and write access'
            print 'only for the user, and must contain at least a line with:'
            print 'MPD_SECRETWORD=<secretword>'
            print 'One way to safely create this file is to do the following:'
            print '  cd $HOME'
            print '  touch .mpd.conf'
            print '  chmod 600 .mpd.conf'
            print 'and then use an editor to insert a line like'
            print '  MPD_SECRETWORD=mr45-j9z'
            print 'into the file.  (Of course use some other secret word than mr45-j9z.)' 
            exit(-1)
        if  (mode & 0x3f):
            print 'configuration file %s is accessible by others' % (parmsRCFilename)
            print 'change permissions to allow read and write access only by you'
            exit(-1)
        parmsRCFile = open(parmsRCFilename)
        for line in parmsRCFile:
            if line[0] == '#':
                continue
            line = line.rstrip()
            try:
                withoutComments = line.split('#')[0]
                splitLine = withoutComments.split('=')
            except:
                splitLine = []
            if len(splitLine) == 2:
                (k,v) = splitLine
                k = k.upper()
                if not k.startswith('MPD'):   # may also be MPDRUN_ etc.
                    k = 'MPD_' + k    # default to an mpd parm
                if k in self.parmsToOverride.keys():
                    self.parmdb[('rcfile',k)] = v
                else:
                    mpd_print(1,'invalid key in rc file; line=:%s:' % (line) )
                    exit(-1)
            else:
                mpd_print(0, 'skipping .mpd.conf file line = :%s:' % (line) )
        if not self.parmdb.has_key('MPD_SECRETWORD'):
            print 'parmsRCFile %s has no secretword' % (parmsRCFilename)
            exit(-1)
    def get_parms_from_cmdline(self):
        argidx = 1
        while argidx < len(argv):
            if argv[argidx] == '--help':
                self.usage()
                argidx += 1
            elif argv[argidx] == '-h':
                if len(argv) < 3:
                    self.usage()
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = argv[argidx+1]
                argidx += 2
            elif argv[argidx].startswith('--host'):
                entryHost = argv[argidx].split('=',1)[1]
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = entryHost
                argidx += 1
            elif argv[argidx] == '-p':
                if argidx >= (len(argv)-1):
                    print 'missing arg for -p'
                    exit(-1)
                if not argv[argidx+1].isdigit():
                    print 'invalid port %s ; must be numeric' % (argv[argidx+1])
                    exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(argv[argidx+1])
                argidx += 2
            elif argv[argidx].startswith('--port'):
                entryPort = argv[argidx].split('=',1)[1]
                if not entryPort.isdigit():
                    print 'invalid port %s ; must be numeric' % (entryPort)
                    exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(entryPort)
                argidx += 1
            elif argv[argidx].startswith('--ncpus'):
                NCPUs = argv[argidx].split('=',1)[1]
                if not NCPUs.isdigit():
                    print 'invalid ncpus %s ; must be numeric' % (NCPUs)
                    exit(-1)
                self.parmdb[('cmdline','MPD_NCPUS')] = int(NCPUs)
                argidx += 1
            elif argv[argidx].startswith('--pid'):
                splitPid = argv[argidx].split('=')
                if len(splitPid) == 1  or  not splitPid[1]:
                    pidFilename = '/var/run/mpd.pid'
                else:
                    pidFilename = splitPid[1]
                self.parmdb[('cmdline','MPD_PID_FILENAME')] = pidFilename
                argidx += 1
            elif argv[argidx].startswith('--ifhn'):
                ifhn = argv[argidx].split('=',1)[1]
                try:
                    hostinfo = gethostbyname_ex(ifhn)
                    ifhn = hostinfo[2][0]
                except:
                    print 'mpd failed: gethostbyname_ex failed for %s' % (ifhn)
                    exit(-1)
                self.parmdb[('cmdline','MPD_MY_IFHN')] = ifhn
                argidx += 1
            elif argv[argidx] == '-l':
                if argidx >= (len(argv)-1):
                    print 'missing arg for -l'
                    exit(-1)
                if not argv[argidx+1].isdigit():
                    print 'invalid listenport %s ; must be numeric' % (argv[argidx+1])
                    exit(-1)
                self.parmdb[('cmdline','MPD_LISTEN_PORT')] = int(argv[argidx+1])
                argidx += 2
            elif argv[argidx].startswith('--listenport'):
                myListenPort = argv[argidx].split('=',1)[1]
                if not myListenPort.isdigit():
                    print 'invalid listenport %s ; must be numeric' % (myListenPort)
                    exit(-1)
                self.parmdb[('cmdline','MPD_LISTEN_PORT')] = int(myListenPort)
                argidx += 1
            elif argv[argidx] == '-hp':
                if argidx >= (len(argv)-1):
                    print 'missing arg for -hp'
                    exit(-1)
                try:
                    (entryIfhn,entryPort) = argv[argidx+1].split('_')
                except:
                    print 'invalid entry host: %s' % (argv[argidx+1])
                    exit(-1)
                if not entryPort.isdigit():
                    print 'invalid port %s ; must be numeric' % (argv[argidx+1])
                    exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = entryIfhn
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(entryPort)
                argidx += 2
            elif argv[argidx] == '-t'  or  argv[argidx] == '--trace':
                self.parmdb[('cmdline','MPD_TRACE_FLAG')] = 1
                argidx += 1
            elif argv[argidx] == '-n'  or  argv[argidx] == '--noconsole':
                self.parmdb[('cmdline','MPD_CONSOLE_FLAG')] = 0
                argidx += 1
            elif argv[argidx] == '-e'  or  argv[argidx] == '--echo':
                self.parmdb[('cmdline','MPD_ECHO_PORT_FLAG')] = 1 
                argidx += 1
            elif argv[argidx] == '-d'  or  argv[argidx] == '--daemon':
                self.parmdb[('cmdline','MPD_DAEMON_FLAG')] = 1 
                argidx += 1
            elif argv[argidx] == '-b'  or  argv[argidx] == '--bulletproof':
                self.parmdb[('cmdline','MPD_BULLETPROOF_FLAG')] = 1 
                argidx += 1
            else:
                print 'unrecognized arg: %s' % (argv[argidx])
                exit(-1)
        if (self.parmdb['MPD_ENTRY_IFHN']  and  not self.parmdb['MPD_ENTRY_PORT']) \
        or (self.parmdb['MPD_ENTRY_PORT']  and  not self.parmdb['MPD_ENTRY_IFHN']):
            print 'host and port must be specified together'
            exit(-1)
    def handle_console_connection(self,sock):
        if not self.conSock:
            (self.conSock,newConnAddr) = sock.accept()
            line = self.conSock.recv_char_msg().rstrip()
            if not line:    # caller went away (perhaps another mpd seeing if I am here)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            try:
                splitLine = line.split('=',1)
            except:
                return
            if len(splitLine) < 2  or  splitLine[0] != 'realusername':
                mpd_print(0, 'console sent bad msg :%s:' % line)
                try:  # try to let console know
                    self.conSock.send_dict_msg( {'cmd':'invalid_msg_received_from_you'} )
                except:
                    pass
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            self.streamHandler.set_handler(self.conSock,self.handle_console_input)
            self.conSock.realUsername = splitLine[1]
            self.conSock.name = 'console'
        else:
            return  ## postpone it; hope the other one frees up soon
    def handle_console_input(self,sock):
        msg = self.conSock.recv_dict_msg()
        if not msg:
            mpd_print(0000, 'console has disappeared; closing it')
            self.streamHandler.del_handler(self.conSock)
            self.conSock.close()
            self.conSock = 0
            return
        if not msg.has_key('cmd'):
            mpd_print(1, 'console sent bad msg :%s:' % msg)
            try:  # try to let console know
                self.conSock.send_dict_msg({ 'cmd':'invalid_msg_received_from_you' })
            except:
                pass
            self.streamHandler.del_handler(self.conSock)
            self.conSock.close()
            self.conSock = 0
            return
        if msg['cmd'] == 'mpdrun':
            # permit anyone to run but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
                msg['users'] = { (0,msg['nprocs']-1) : self.conSock.realUsername }
            #
            msg['mpdid_mpdrun_start'] = self.myId
            msg['nstarted_on_this_loop'] = 0
            msg['first_loop'] = 1
            msg['ringsize'] = 0
            msg['ring_ncpus'] = 0
            if msg.has_key('try_1st_locally'):
                self.do_mpdrun(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
            # send ack after job is going
        elif msg['cmd'] == 'get_mpd_version':
            msgToSend = { 'cmd' : 'mpd_version_response', 'mpd_version' : mpd_version() }
            self.conSock.send_dict_msg(msgToSend)
        elif msg['cmd'] == 'mpdtrace':
            msgToSend = { 'cmd'     : 'mpdtrace_info',
                          'dest'    : self.myId,
                          'id'      : self.myId,
                          'lhsport' : '%s' % (self.ring.lhsPort),
                          'lhsifhn' : '%s' % (self.ring.lhsIfhn),
                          'rhsport' : '%s' % (self.ring.rhsPort),
                          'rhsifhn' : '%s' % (self.ring.rhsIfhn) }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msgToSend = { 'cmd'  : 'mpdtrace_trailer', 'dest' : self.myId }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send trace info later
        elif msg['cmd'] == 'mpdallexit':
            if self.conSock.realUsername != self.myRealUsername:
                msgToSend = { 'cmd':'invalid_username_to_make_this_request' }
                self.conSock.send_dict_msg(msgToSend)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            self.allExiting = 1
            self.ring.rhsSock.send_dict_msg( {'cmd' : 'mpdallexit', 'src' : self.myId} )
            self.conSock.send_dict_msg( {'cmd' : 'mpdallexit_ack'} )
        elif msg['cmd'] == 'mpdexit':
            if self.conSock.realUsername != self.myRealUsername:
                msgToSend = { 'cmd':'invalid_username_to_make_this_request' }
                self.conSock.send_dict_msg(msgToSend)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            if msg['mpdid'] == 'localmpd':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg( {'cmd' : 'mpdexit', 'src' : self.myId,
                                              'done' : 0, 'dest' : msg['mpdid']} )
        elif msg['cmd'] == 'mpdringtest':
            msg['src'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # do not send an ack to console now; will send ringtest info later
        elif msg['cmd'] == 'mpdlistjobs':
            msgToSend = { 'cmd'  : 'local_mpdid', 'id' : self.myId }
            self.conSock.send_dict_msg(msgToSend)
            for jobid in self.activeJobs.keys():
                for manPid in self.activeJobs[jobid]:
                    msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                  'dest' : self.myId,
                                  'jobid' : jobid,
                                  'username' : self.activeJobs[jobid][manPid]['username'],
                                  'host' : self.myHost,
                                  'ifhn' : self.myIfhn,
                                  'clipid' : str(self.activeJobs[jobid][manPid]['clipid']),
                                  'sid' : str(manPid),  # may chg to actual sid later
                                  'pgm'  : self.activeJobs[jobid][manPid]['pgm'],
                                  'rank' : self.activeJobs[jobid][manPid]['rank'] }
                    self.conSock.send_dict_msg(msgToSend)
            msgToSend = { 'cmd'  : 'mpdlistjobs_trailer', 'dest' : self.myId }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send listjobs info later
        elif msg['cmd'] == 'mpdkilljob':
            # permit anyone to kill but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
            msg['src'] = self.myId
            msg['handled'] = 0
            if msg['mpdid'] == '':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # send ack to console after I get this msg back and do the kill myself
        elif msg['cmd'] == 'mpdsigjob':
            # permit anyone to sig but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
            msg['src'] = self.myId
            msg['handled'] = 0
            if msg['mpdid'] == '':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # send ack to console after I get this msg back
        elif msg['cmd'] == 'verify_hosts_in_ring':
            msgToSend = { 'cmd'  : 'verify_hosts_in_ring', 'dest' : self.myId,
                          'host_list' : msg['host_list'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send trace info later
        else:
            msgToSend = { 'cmd' : 'invalid_msg_received_from_you' }
            self.conSock.send_dict_msg(msgToSend)
            badMsg = 'invalid msg received from console: %s' % (str(msg))
            mpd_print(1, badMsg)
            syslog(LOG_ERR,badMsg)
    def handle_man_input(self,sock):
        msg = sock.recv_dict_msg()
        if not msg:
            for jobid in self.activeJobs.keys():
                deleted = 0
                for manPid in self.activeJobs[jobid]:
                    if sock == self.activeJobs[jobid][manPid]['socktoman']:
                        del self.activeJobs[jobid][manPid]
                        if len(self.activeJobs[jobid]) == 0:
                            del self.activeJobs[jobid]
                        deleted = 1
                        break
                if deleted:
                    break
            self.streamHandler.del_handler(sock)
            sock.close()
            return
        if not msg.has_key('cmd'):
            mpd_print(1, 'INVALID msg for man request msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_msg' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
            return
        if msg['cmd'] == 'client_pid':
            jobid = msg['jobid']
            manPid = msg['manpid']
            self.activeJobs[jobid][manPid]['clipid'] = msg['clipid']
        elif msg['cmd'] == 'spawn':
            msg['cmd'] = 'mpdrun'  # handle much like an mpdrun from a console
            msg['mpdid_mpdrun_start'] = self.myId
            msg['nstarted_on_this_loop'] = 0
            msg['first_loop'] = 1
            msg['jobalias'] = ''
            msg['stdin_dest'] = '0'
            msg['ringsize'] = 0
            msg['ring_ncpus'] = 0
            msg['gdb'] = 0
            msg['totalview'] = 0
            self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'publish_name':
            self.pmi_published_names[msg['service']] = msg['port']
            msgToSend = { 'cmd' : 'publish_result', 'info' : 'ok' }
            sock.send_dict_msg(msgToSend)
        elif msg['cmd'] == 'lookup_name':
            if self.pmi_published_names.has_key(msg['service']):
                msgToSend = { 'cmd' : 'lookup_result', 'info' : 'ok',
                              'port' : self.pmi_published_names[msg['service']] }
                sock.send_dict_msg(msgToSend)
            else:
                msg['cmd'] = 'pmi_lookup_name'    # add pmi_
                msg['src'] = self.myId
                msg['port'] = 0    # invalid
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'unpublish_name':
            if self.pmi_published_names.has_key(msg['service']):
                del self.pmi_published_names[msg['service']]
                msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'ok' }
                sock.send_dict_msg(msgToSend)
            else:
                msg['cmd'] = 'pmi_unpublish_name'    # add pmi_
                msg['src'] = self.myId
                self.ring.rhsSocket.send_dict_msg(msg)
        else:
            mpd_print(1, 'INVALID request from man msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_request' }
            sock.send_dict_msg(msgToSend)
    def handle_lhs_input(self,sock):
        msg = self.ring.lhsSock.recv_dict_msg()
        if not msg:    # lost lhs; don't worry
            mpd_print(0, "CLOSING self.ring.lhsSock ", self.ring.lhsSock )
            self.streamHandler.del_handler(self.ring.lhsSock)
            self.ring.lhsSock.close()
            self.ring.lhsSock = 0
            return
        if msg['cmd'] == 'mpdrun':
            if  msg.has_key('mpdid_mpdrun_start')  \
            and msg['mpdid_mpdrun_start'] == self.myId:
                if msg['first_loop']:
                    self.currRingSize = msg['ringsize']
                    self.currRingNCPUs = msg['ring_ncpus']
                if msg['nstarted'] == msg['nprocs']:
                    if self.conSock:
                        msgToSend = {'cmd' : 'mpdrun_ack',
                                     'ringsize' : self.currRingSize,
                                     'ring_ncpus' : self.currRingNCPUs}
                        self.conSock.send_dict_msg(msgToSend)
                    return
                if not msg['first_loop']  and  msg['nstarted_on_this_loop'] == 0:
                    if msg.has_key('jobid'):
                        msgToSend = {'cmd' : 'abortjob', 'src' : self.myId,
                                     'jobid' : msg['jobid'],
                                     'reason' : 'some_procs_not_started'}
                        self.ring.rhsSock.send_dict_msg(msgToSend)
                    if self.conSock:
                        msgToSend = {'cmd' : 'job_failed',
                                     'reason' : 'some_procs_not_started',
                                     'remaining_hosts' : msg['hosts']}
                        self.conSock.send_dict_msg(msgToSend)
                    return
                msg['first_loop'] = 0
                msg['nstarted_on_this_loop'] = 0
            self.do_mpdrun(msg)
        elif msg['cmd'] == 'mpdtrace_info':
            if msg['dest'] == self.myId:
                self.conSock.send_dict_msg(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdtrace_trailer':
            if msg['dest'] == self.myId:
                self.conSock.send_dict_msg(msg)
            else:
                msgToSend = { 'cmd'     : 'mpdtrace_info',
                              'dest'    : msg['dest'],
                              'id'      : self.myId,
                              'lhsport' : '%s' % (self.ring.lhsPort),
                              'lhsifhn' : '%s' % (self.ring.lhsIfhn),
                              'rhsport' : '%s' % (self.ring.rhsPort),
                              'rhsifhn' : '%s' % (self.ring.rhsIfhn) }
                self.ring.rhsSock.send_dict_msg(msgToSend)
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdlistjobs_info':
            if msg['dest'] == self.myId:
                self.conSock.send_dict_msg(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdlistjobs_trailer':
            if msg['dest'] == self.myId:
                self.conSock.send_dict_msg(msg)
            else:
                for jobid in self.activeJobs.keys():
                    for manPid in self.activeJobs[jobid]:
                        msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                      'dest' : msg['dest'],
                                      'jobid' : jobid,
                                      'username' : self.activeJobs[jobid][manPid]['username'],
                                      'host' : self.myHost,
                                      'clipid' : str(self.activeJobs[jobid][manPid]['clipid']),
                                      'sid' : str(manPid),  # may chg to actual sid later
                                      'pgm' : self.activeJobs[jobid][manPid]['pgm'],
                                      'rank' : self.activeJobs[jobid][manPid]['rank'] }
                        self.ring.rhsSock.send_dict_msg(msgToSend)
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdallexit':
            self.allExiting = 1
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdexit':
            if msg['dest'] == self.myId:
                msg['done'] = 1    # do this first
            if msg['src'] == self.myId:    # may be src and dest
                if self.conSock:
                    if msg['done']:
                        self.conSock.send_dict_msg({'cmd' : 'mpdexit_ack'})
                    else:
                        self.conSock.send_dict_msg({'cmd' : 'mpdexit_failed'})
            else:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['dest'] == self.myId:
                self.exiting = 1
                self.ring.lhsSock.send_dict_msg( { 'cmd'     : 'mpdexiting',
                                                   'rhsifhn' : self.ring.rhsIfhn,
                                                   'rhsport' : self.ring.rhsPort })
        elif msg['cmd'] == 'mpdringtest':
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            else:
                numLoops = msg['numloops'] - 1
                if numLoops > 0:
                    msg['numloops'] = numLoops
                    self.ring.rhsSock.send_dict_msg(msg)
                else:
                    if self.conSock:    # may have closed it if user did ^C at console
                        self.conSock.send_dict_msg({'cmd' : 'mpdringtest_done' })
        elif msg['cmd'] == 'mpdsigjob':
            forwarded = 0
            if msg['handled']  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
                forwarded = 1
            handledHere = 0
            for jobid in self.activeJobs.keys():
                sjobid = jobid.split('  ')  # jobnum and mpdid
                if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
                or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                    for manPid in self.activeJobs[jobid].keys():
                        if self.activeJobs[jobid][manPid]['username'] == msg['username']  \
                        or msg['username'] == 'root':
                            manSock = self.activeJobs[jobid][manPid]['socktoman']
                            manSock.send_dict_msg( { 'cmd' : 'signal_to_handle',
                                                     's_or_g' : msg['s_or_g'],
                                                     'sigtype' : msg['sigtype'] } )
                            handledHere = 1
            if handledHere:
                msg['handled'] = 1
            if not forwarded  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['src'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg( {'cmd' : 'mpdsigjob_ack',
                                                 'handled' : msg['handled'] } )
        elif msg['cmd'] == 'mpdkilljob':
            forwarded = 0
            if msg['handled'] and msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
                forwarded = 1
            handledHere = 0
            for jobid in self.activeJobs.keys():
                sjobid = jobid.split('  ')  # jobnum and mpdid
                if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
                or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                    for manPid in self.activeJobs[jobid].keys():
                        if self.activeJobs[jobid][manPid]['username'] == msg['username']  \
                        or msg['username'] == 'root':
                            try:
                                pgrp = manPid * (-1)  # neg manPid -> group
                                kill(pgrp,SIGKILL)
                                cliPid = self.activeJobs[jobid][manPid]['clipid']
                                pgrp = cliPid * (-1)  # neg Pid -> group
                                kill(pgrp,SIGKILL)  # neg Pid -> group
                                handledHere = 1
                            except:
                                pass
                    # del self.activeJobs[jobid]  ## handled when child goes away
            if handledHere:
                msg['handled'] = 1
            if not forwarded  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['src'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg( {'cmd' : 'mpdkilljob_ack',
                                                 'handled' : msg['handled'] } )
        elif msg['cmd'] == 'abortjob':
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            for jobid in self.activeJobs.keys():
                if jobid == msg['jobid']:
                    for manPid in self.activeJobs[jobid].keys():
                        manSocket = self.activeJobs[jobid][manPid]['socktoman']
                        if manSocket:
                            manSocket.send_dict_msg(msg)
                            sleep(0.5)  # give man a brief chance to deal with this
                        try:
                            pgrp = manPid * (-1)  # neg manPid -> group
                            kill(pgrp,SIGKILL)
                            cliPid = self.activeJobs[jobid][manPid]['clipid']
                            pgrp = cliPid * (-1)  # neg Pid -> group
                            kill(pgrp,SIGKILL)  # neg Pid -> group
                        except:
                            pass
                    # del self.activeJobs[jobid]  ## handled when child goes away
        elif msg['cmd'] == 'pulse':
            self.ring.lhsSock.send_dict_msg({'cmd':'pulse_ack'})
        elif msg['cmd'] == 'verify_hosts_in_ring':
            while self.myIfhn in msg['host_list']  or  self.myHost in msg['host_list']:
                if self.myIfhn in msg['host_list']:
                    msg['host_list'].remove(self.myIfhn)
                elif self.myHost in msg['host_list']:
                    msg['host_list'].remove(self.myHost)
            if msg['dest'] == self.myId:
                msgToSend = { 'cmd' : 'verify_hosts_in_ring_response',
                              'host_list' : msg['host_list'] }
                self.conSock.send_dict_msg(msgToSend)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_lookup_name':
            if msg['src'] == self.myId:
                if msg.has_key('port') and msg['port'] != 0:
                    msgToSend = msg
                    msgToSend['cmd'] = 'lookup_result'
                    msgToSend['info'] = 'ok'
                else:
                    msgToSend = { 'cmd' : 'lookup_result', 'info' : 'unknown_service',
                                  'port' : 0}
                jobid = msg['jobid']
                manPid = msg['manpid']
                manSock = self.activeJobs[jobid][manPid]['socktoman']
                manSock.send_dict_msg(msgToSend)
            else:
                if self.pmi_published_names.has_key(msg['service']):
                    msg['port'] = self.pmi_published_names[msg['service']]
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_unpublish_name':
            if msg['src'] == self.myId:
                if msg.has_key('done'):
                    msgToSend = msg
                    msgToSend['cmd'] = 'unpublish_result'
                    msgToSend['info'] = 'ok'
                else:
                    msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'unknown_service' }
                jobid = msg['jobid']
                manPid = msg['manpid']
                manSock = self.activeJobs[jobid][manPid]['socktoman']
                manSock.send_dict_msg(msgToSend)
            else:
                if self.pmi_published_names.has_key(msg['service']):
                    del self.pmi_published_names[msg['service']]
                    msg['done'] = 1
                self.ring.rhsSock.send_dict_msg(msg)
        else:
            mpd_print(1, 'unrecognized cmd from lhs: %s' % (msg) )
    def handle_rhs_input(self,sock):
        if self.allExiting:
            return
        msg = sock.recv_dict_msg()
        if not msg:    # lost rhs; re-knit the ring
            if sock == self.ring.rhsSock:
                needToReenter = 1
            else:
                needToReenter = 0
            if sock == self.ring.rhsSock  and self.ring.lhsSock:
                self.streamHandler.del_handler(self.ring.lhsSock)
                self.ring.lhsSock.close()
                self.ring.lhsSock = 0
            if sock == self.ring.rhsSock  and self.ring.rhsSock:
                self.streamHandler.del_handler(self.ring.rhsSock)
                self.ring.rhsSock.close()
                self.ring.rhsSock = 0
            if needToReenter:
                mpd_print(1,'lost rhs; re-entering ring')
                rc = self.ring.reenter_ring(lhsHandler=self.handle_lhs_input,
                                            rhsHandler=self.handle_rhs_input,
                                            ntries=10)
                if rc < 0:
                    mpd_print(1,"failed to reenter ring")
                    exit(-1)
            return
        if msg['cmd'] == 'pulse_ack':
            self.pulse_ctr = 0
        elif msg['cmd'] == 'mpdexiting':    # for mpdexit
            if self.ring.rhsSock:
                self.streamHandler.del_handler(self.ring.rhsSock)
                self.ring.rhsSock.close()
                self.ring.rhsSock = 0
            # connect to new rhs
            self.ring.rhsIfhn = msg['rhsifhn']
            self.ring.rhsPort = int(msg['rhsport'])
            mpd_print(0000,"TRYING TO CONN TO %s %s" % (self.ring.rhsIfhn,self.ring.rhsPort))
            if self.ring.rhsIfhn == self.myIfhn  and  self.ring.rhsPort == self.parmdb['MPD_LISTEN_PORT']:
                rv = self.ring.connect_rhs(rhsHost=self.ring.rhsIfhn,
                                           rhsPort=self.ring.rhsPort,
                                           rhsHandler=self.handle_rhs_input,
                                           numTries=3)
                if rv[0] <=  0:  # connect did not succeed; may try again
                    mpd_print(1,"rhs connect failed")
                    exit(-1)
                return
            self.ring.rhsSock = MPDSock(name='rhs')
            self.ring.rhsSock.connect((self.ring.rhsIfhn,self.ring.rhsPort))
            if not self.ring.rhsSock:
                mpd_print(1,'handle_rhs_input failed to obtain rhs socket')
                return
            msgToSend = { 'cmd' : 'request_to_enter_as_lhs', 'host' : self.myHost,
                          'ifhn' : self.myIfhn, 'port' : self.parmdb['MPD_LISTEN_PORT'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msg = self.ring.rhsSock.recv_dict_msg()
            if (not msg) or  \
               (not msg.has_key('cmd')) or  \
               (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
                mpd_print(1, 'failed to recv challenge from rhs; msg=:%s:' % (msg) )
            response = md5new(''.join([self.parmdb['MPD_SECRETWORD'],
                                       msg['randnum']])).digest()
            msgToSend = { 'cmd' : 'challenge_response',
                          'response' : response,
                          'host' : self.myHost, 'ifhn' : self.myIfhn,
                          'port' : self.parmdb['MPD_LISTEN_PORT'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msg = self.ring.rhsSock.recv_dict_msg()
            if (not msg) or  \
               (not msg.has_key('cmd')) or  \
               (msg['cmd'] != 'OK_to_enter_as_lhs'):
                mpd_print(1, 'NOT OK to enter ring; msg=:%s:' % (msg) )
            mpd_print(0000,"GOT CONN TO %s %s" % (self.ring.rhsIfhn,self.ring.rhsPort))
        else:
            mpd_print(1, 'unexpected from rhs; msg=:%s:' % (msg) )
        return
    def do_mpdrun(self,msg):
        if msg.has_key('jobid'):
            jobid = msg['jobid']
        else:
            jobid = str(self.nextJobInt) + '  ' + self.myId + '  ' + msg['jobalias']
            self.nextJobInt += 1
            msg['jobid'] = jobid
        if msg['nstarted'] >= msg['nprocs']:
            self.ring.rhsSock.send_dict_msg(msg)  # forward it on around
            return
        hosts = msg['hosts']
        if self.myIfhn in hosts.values():
            for ranks in hosts.keys():
                if hosts[ranks] == self.myIfhn:
                    (lorank,hirank) = ranks
                    for rank in range(lorank,hirank+1):
                        self.run_one_cli(rank,msg)
                        msg['nstarted'] += 1
                        msg['nstarted_on_this_loop'] += 1
                    del msg['hosts'][ranks]
        elif '_any_from_pool_' in hosts.values():
            hostsKeys = hosts.keys()
            hostsKeys.sort()
            for ranks in hostsKeys:
                if hosts[ranks] == '_any_from_pool_':
                    (lorank,hirank) = ranks
                    hostSpecPool = msg['host_spec_pool']
                    if self.myIfhn in hostSpecPool  or  self.myHost in hostSpecPool:
                        self.run_one_cli(lorank,msg)
                        msg['nstarted'] += 1
                        msg['nstarted_on_this_loop'] += 1
                        del msg['hosts'][ranks]
                        if lorank < hirank:
                            msg['hosts'][(lorank+1,hirank)] = '_any_from_pool_'
                    break
        elif '_any_' in hosts.values():
            hostsKeys = hosts.keys()
            hostsKeys.sort()
            for ranks in hostsKeys:
                if hosts[ranks] == '_any_':
                    (lorank,hirank) = ranks
                    self.run_one_cli(lorank,msg)
                    msg['nstarted'] += 1
                    msg['nstarted_on_this_loop'] += 1
                    del msg['hosts'][ranks]
                    if lorank < hirank:
                        msg['hosts'][(lorank+1,hirank)] = '_any_'
                    break
        if msg['first_loop']:
            msg['ringsize'] += 1
            msg['ring_ncpus'] += self.parmdb['MPD_NCPUS']
        self.ring.rhsSock.send_dict_msg(msg)  # forward it on around
    def run_one_cli(self,currRank,msg):
        manListenSock = MPDListenSock('',0,name='tempsock')
        manListenPort = manListenSock.getsockname()[1]
        if not msg.has_key('entry_host'):
            manEntryIfhn = ''
            manEntryPort = 0
            manKVSTemplate = '%s_%d' % (self.myHost,manListenPort)
            manKVSTemplate = sub('\.','_',manKVSTemplate)  # chg magpie.cs to magpie_cs
            manKVSTemplate = sub('\-','_',manKVSTemplate)  # chg node-0 to node_0
            msg['kvs_template'] = manKVSTemplate
        else:
            manEntryIfhn  = msg['entry_ifhn']
            manEntryPort = msg['entry_port']
            manKVSTemplate = msg['kvs_template']
        (toManSock,toMpdSock) = mpd_sockpair()
        toManSock.name = 'to_man'
        toMpdSock.name = 'to_mpd'  ## to be used by mpdman below
        self.streamHandler.set_handler(toManSock,self.handle_man_input)
        msg['entry_host'] = self.myHost
        msg['entry_ifhn'] = self.myIfhn
        msg['entry_port'] = manListenPort
        if msg['nstarted'] == 0:
            msg['pos0_host'] = self.myHost
            msg['pos0_ifhn'] = self.myIfhn
            msg['pos0_port'] = str(manListenPort)
        users = msg['users']
        for ranks in users.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                username = users[ranks]
                try:
                    pwent = getpwnam(username)
                except:
                    mpd_print(1,'invalid username :%s: on %s' % (username,self.myHost))
                    msgToSend = {'cmd' : 'job_failed', 'reason' : 'invalid_username',
                                 'username' : username, 'host' : self.myHost }
                    self.conSock.send_dict_msg(msgToSend)
                    return
                break
        execs = msg['execs']
        for ranks in execs.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgm = execs[ranks]
                break
        paths = msg['paths']
        for ranks in paths.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pathForExec = paths[ranks]
                break
        args = msg['args']
        for ranks in args.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmArgs = dumps(args[ranks])
                break
        envvars = msg['envvars']
        for ranks in envvars.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmEnvVars = dumps(envvars[ranks])
                break
        limits = msg['limits']
        for ranks in limits.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmLimits = dumps(limits[ranks])
                break
        cwds = msg['cwds']
        for ranks in cwds.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                cwd = cwds[ranks]
                break
        umasks = msg['umasks']
        for ranks in umasks.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmUmask = umasks[ranks]
                break
        jobid = msg['jobid']
        manPid = fork()
        if manPid == 0:
            self.conListenSock = 0    # don't want to clean up console if I am manager
            self.myId = '%s_man_%d' % (self.myHost,self.myPid)
            mpd_set_my_id(self.myId)
            self.streamHandler.close_all_active_streams()
            toManSock.close()
            setpgrp()
            environ['MPDMAN_MYHOST'] = self.myHost
            environ['MPDMAN_MYIFHN'] = self.myIfhn
            environ['MPDMAN_JOBID'] = jobid
            environ['MPDMAN_CLI_PGM'] = pgm
            environ['MPDMAN_CLI_PATH'] = pathForExec
            environ['MPDMAN_PGM_ARGS'] = pgmArgs
            environ['MPDMAN_PGM_ENVVARS'] = pgmEnvVars
            environ['MPDMAN_PGM_LIMITS'] = pgmLimits
            environ['MPDMAN_CWD'] = cwd
            environ['MPDMAN_UMASK'] = pgmUmask
            environ['MPDMAN_SPAWNED'] = str(msg['spawned'])
            environ['MPDMAN_NPROCS'] = str(msg['nprocs'])
            environ['MPDMAN_MPD_LISTEN_PORT'] = str(self.parmdb['MPD_LISTEN_PORT'])
            environ['MPDMAN_MPD_CONF_SECRETWORD'] = self.parmdb['MPD_SECRETWORD']
            environ['MPDMAN_CONHOST'] = msg['conhost']
            environ['MPDMAN_CONIFHN'] = msg['conifhn']
            environ['MPDMAN_CONPORT'] = str(msg['conport'])
            environ['MPDMAN_RANK'] = str(currRank)
            environ['MPDMAN_POS_IN_RING'] = str(msg['nstarted'])
            environ['MPDMAN_MY_LISTEN_PORT'] = str(manListenPort)
            environ['MPDMAN_POS0_HOST'] = msg['pos0_host']
            environ['MPDMAN_POS0_IFHN'] = msg['pos0_ifhn']
            environ['MPDMAN_POS0_PORT'] = msg['pos0_port']
            environ['MPDMAN_LHS_IFHN']  = manEntryIfhn
            environ['MPDMAN_LHS_PORT'] = str(manEntryPort)
            environ['MPDMAN_KVS_TEMPLATE'] = manKVSTemplate
            environ['MPDMAN_MY_LISTEN_FD'] = str(manListenSock.fileno())
            environ['MPDMAN_TO_MPD_FD'] = str(toMpdSock.fileno())
            environ['MPDMAN_STDIN_DEST'] = msg['stdin_dest']
            environ['MPDMAN_TOTALVIEW'] = str(msg['totalview'])
            environ['MPDMAN_GDB'] = str(msg['gdb'])
            fullDirName = path.abspath(path.split(argv[0])[0])  # normalize
            environ['MPDMAN_FULLPATHDIR'] = fullDirName    # used to find gdbdrv
            environ['MPDMAN_SINGINIT_PID']  = str(msg['singinitpid'])
            environ['MPDMAN_SINGINIT_PORT'] = str(msg['singinitport'])
            if msg.has_key('line_labels'):
                environ['MPDMAN_LINE_LABELS'] = '1'
            else:
                environ['MPDMAN_LINE_LABELS'] = '0'
            if msg.has_key('rship'):
                environ['MPDMAN_RSHIP'] = msg['rship']
                environ['MPDMAN_MSHIP_HOST'] = msg['mship_host']
                environ['MPDMAN_MSHIP_PORT'] = str(msg['mship_port'])
            if msg.has_key('doing_bnr'):
                environ['MPDMAN_DOING_BNR'] = '1'
            else:
                environ['MPDMAN_DOING_BNR'] = '0'
            if getuid() == 0:
                uid = pwent[2]
                gid = pwent[3]
                setgroups(mpd_get_groups_for_username(username))
                setregid(gid,gid)
                setreuid(uid,uid)
            import atexit    # need to use full name of _exithandlers
            atexit._exithandlers = []    # un-register handlers in atexit module
            # import profile
            # print 'profiling the manager'
            # profile.run('mpdman()')
            mpdman = MPDMan()
            mpdman.run()
            exit(0)  # do NOT do cleanup
        else:
            manListenSock.close()
            toMpdSock.close()
            if not self.activeJobs.has_key(jobid):
                self.activeJobs[jobid] = {}
            self.activeJobs[jobid][manPid] = { 'pgm' : pgm, 'rank' : currRank,
                                               'username' : username,
                                               'clipid' : -1,    # until report by man
                                               'socktoman' : toManSock }


# code for testing

if __name__ == '__main__':
    mpd = MPD()
    mpd.run()
