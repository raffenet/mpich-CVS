#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

try:
    from signal          import signal, SIG_IGN, SIGINT
except KeyboardInterrupt:
    exit(0)

signal(SIGINT,SIG_IGN)

from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path
from popen2 import Popen3
from pwd    import getpwuid
from urllib import quote
import xml.dom.minidom

global totalProcs, nextRange, argvCopy, configLines, configIdx, setenvall, appnum, usize
global gEnv, gHost, gWDIR, gPath, gNProcs

def mpiexec():
    global totalProcs, nextRange, argvCopy, configLines, configIdx, setenvall, \
           appnum, usize
    global gEnv, gHost, gWDIR, gPath, gNProcs
    totalProcs    = 0
    nextRange     = 0
    configLines   = []
    configIdx     = 0
    xmlForArgsets = []
    defaultArgs   = []
    setenvall     = 0
    linelabels    = 0
    usize         = 1                   # default universe size for MPI
    appnum        = 0                   # appnum counter for MPI
    gEnv          = {}
    gHost         = '_any_'                  # default
    gWDIR         = path.abspath(getcwd())   # default
    gPath         = environ['PATH'] # default ; avoid name conflict with python path module
    gNProcs       = 1                   # default

    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '-help'  or  argv[1] == '--help':
	usage()
    if argv[1] == '-file':
	delXmlFile = 0
	if len(argv) != 3:
	    usage()
        xmlFilename = argv[2]
    else:
	delXmlFile = 1
        if argv[1] == '-configfile':
	    if len(argv) != 3:
	        usage()
            configFile = open(argv[2],'r')
            configLines = configFile.readlines()
            configLines = [ x.strip() for x in configLines if x[0] != '#' ]
            if configLines[0].startswith('-default'):
                shOut = Popen3("/bin/sh -c 'for a in $*; do echo _$a; done' -- %s" % \
                               configLines[0][9:])  # 9: => skip the -default
                for line in shOut.fromchild:
                    defaultArgs.append(line[1:].strip())    # 1: strips off the leading _
                configIdx = 1
        else:
            if argv[1] == '-default':
                i = 2
                while argv[i] != ':':
                    defaultArgs.append(argv[i])
                    i += 1
                i += 1  # skip the :
                argvCopy = argv[i:]
                argvCopy.append(':')   # stick an extra : on the end
            else:
                argvCopy = argv[1:]
                argvCopy.append(':')   # stick an extra : on the end
        if defaultArgs:
            gargIdx = 0
            while gargIdx < len(defaultArgs):
                if defaultArgs[gargIdx] == '-setenvall':
                    setenvall = 1
                    gargIdx += 1
                elif defaultArgs[gargIdx] == '-env':
                    var = defaultArgs[gargIdx+1]
                    val = defaultArgs[gargIdx+2]
                    gEnv[var] = val
                    gargIdx += 3
                elif defaultArgs[gargIdx] == '-l':
                    linelabels = 1
                    gargIdx += 1
                elif defaultArgs[gargIdx] == '-usize':
                    usize = defaultArgs[gargIdx+1]
                    if not usize.isdigit():
                        print 'non-numeric usize: %s' % usize
                        usage()
                    gargIdx += 2
                elif defaultArgs[gargIdx] == '-host':
                    gHost = defaultArgs[gargIdx+1]
                    gargIdx += 2
                elif defaultArgs[gargIdx] == '-wdir':
                    gWDIR = defaultArgs[gargIdx+1]
                    gargIdx += 2
                elif defaultArgs[gargIdx] == '-path':
                    gPath = defaultArgs[gargIdx+1]
                    gargIdx += 2
                elif defaultArgs[gargIdx] == '-n'  or  defaultArgs[gargIdx] == '-np':
                    gNProcs = int(defaultArgs[gargIdx+1])
                    gargIdx += 2
                else:
                    print 'unrecognized arg: %s' % (defaultArgs[gargIdx])
                    usage()
        xmlDOC = xml.dom.minidom.Document()
        xmlCPG = xmlDOC.createElement('create-process-group')
        xmlDOC.appendChild(xmlCPG)
        # xmlHOSTSPEC = xmlDOC.createElement('host-spec')    # append to CPG after proc-specs
        argset = get_next_argset()
        while argset:
            xmlPROCSPEC = xmlDOC.createElement('process-spec')
            xmlCPG.appendChild(xmlPROCSPEC)
	    # handle_argset(argset,xmlDOC,xmlPROCSPEC,xmlHOSTSPEC)
	    handle_argset(argset,xmlDOC,xmlPROCSPEC)
            argset = get_next_argset()
        # xmlCPG.appendChild(xmlHOSTSPEC)
        xmlCPG.setAttribute('totalprocs', str(totalProcs) )  # after handling argsets
        if linelabels:
            xmlCPG.setAttribute('output', 'label')
        submitter = getpwuid(getuid())[0]
        xmlCPG.setAttribute('submitter', submitter)
        xmlFilename = '/tmp/%s_tempxml_%d' % (submitter,getpid())
        xmlFile = open(xmlFilename,'w')
        print >>xmlFile, xmlDOC.toprettyxml(indent='   ')
        # print xmlDOC.toprettyxml(indent='   ')    #### RMB TEMP DEBUG
        xmlFile.close()
    fullDirName = path.abspath(path.split(argv[0])[0])  # normalize for platform also
    mpdrun = path.join(fullDirName,'mpdrun.py')
    if not access(mpdrun,X_OK):
        print 'mpiexec: cannot execute mpdrun %s' % mpdrun
        exit(0);
    if delXmlFile:
        execvpe(mpdrun,[mpdrun,'-delxmlfile',xmlFilename],environ)
    else:
        execvpe(mpdrun,[mpdrun,'-f',xmlFilename],environ)
    print 'mpiexec: exec failed for %s' % mpdrun
    exit(0);

def get_next_argset():
    global argvCopy, configLines, configIdx
    argset = []
    if len(configLines):
        if configIdx < len(configLines):
            line = configLines[configIdx]
            configIdx += 1
            if line:
                # next line: prepend an _ to avoid problems with -n as arg to echo
                shOut = Popen3("/bin/sh -c 'for a in $*; do echo _$a; done' -- %s" % line)
                for line in shOut.fromchild:
                    argset.append(line[1:].strip())    # 1: strips off the leading _
    else:
        if argvCopy:
            colonPos = argvCopy.index(':')
	    argset = argvCopy[0:colonPos]
	    argvCopy = argvCopy[ colonPos+1 : ]
	    try:    colonPos = argvCopy.index(':')
	    except: colonPos = -1
    return argset

# def handle_argset(argset,xmlDOC,xmlPROCSPEC,xmlHOSTSPEC):
def handle_argset(argset,xmlDOC,xmlPROCSPEC):
    global totalProcs, nextRange, setenvall, appnum, usize
    global gEnv, gHost, gWDIR, gPath, gNProcs
    host   = gHost
    wdir   = gWDIR
    wpath  = gPath
    nProcs = gNProcs
    lEnv   = {}
    cmdAndArgs = []
    argidx = 0    # can not use range here
    while argidx < len(argset):
        if argset[argidx][0] != '-':    
            break                       # since now at executable
        if argset[argidx] == '-n' or argset[argidx] == '-np':
            nProcs = int(argset[argidx+1])
            argidx += 2
        elif argset[argidx] == '-host':
            host = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-wdir':
            wdir = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-path':
            wpath = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-env':
            var = argset[argidx+1]
            val = argset[argidx+2]
            lEnv[var] = val
            argidx += 3
        else:
            print 'unsupported or unrecognized option: %s' % argset[argidx]
            usage()
    while argidx < len(argset):
        cmdAndArgs.append(argset[argidx])
        argidx += 1
    if not cmdAndArgs:
        print 'no cmd specified'
        usage()

    if nProcs == 1:
        thisRange = (nextRange,nextRange)
    else:
        thisRange = (nextRange,nextRange+nProcs-1)
    nextRange += nProcs
    totalProcs += nProcs

    # if host:
        # xmlHOSTNAME = xmlDOC.createTextNode(host)
        # xmlHOSTSPEC.appendChild(xmlHOSTNAME)

    xmlPROCSPEC.setAttribute('user',getpwuid(getuid())[0])
    xmlPROCSPEC.setAttribute('exec',cmdAndArgs[0])
    xmlPROCSPEC.setAttribute('path',wpath)
    xmlPROCSPEC.setAttribute('cwd',wdir)
    xmlPROCSPEC.setAttribute('host',host)
    xmlPROCSPEC.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]))

    for i in xrange(1,len(cmdAndArgs[1:])+1):
        arg = cmdAndArgs[i]
        xmlARG = xmlDOC.createElement('arg')
        xmlPROCSPEC.appendChild(xmlARG)
        xmlARG.setAttribute('idx', '%d' % (i) )
        xmlARG.setAttribute('value', '%s' % (quote(arg)))

    if setenvall:
        for envvar in environ.keys():
            xmlENVVAR = xmlDOC.createElement('env')
            xmlPROCSPEC.appendChild(xmlENVVAR)
            xmlENVVAR.setAttribute('name',  '%s' % (envvar))
            xmlENVVAR.setAttribute('value', '%s' % (environ[envvar]))
    for envvar in gEnv.keys():
        xmlENVVAR = xmlDOC.createElement('env')
        xmlPROCSPEC.appendChild(xmlENVVAR)
        xmlENVVAR.setAttribute('name',  '%s' % (envvar))
        xmlENVVAR.setAttribute('value', '%s' % (gEnv[envvar]))
    for envvar in lEnv.keys():
        xmlENVVAR = xmlDOC.createElement('env')
        xmlPROCSPEC.appendChild(xmlENVVAR)
        xmlENVVAR.setAttribute('name',  '%s' % (envvar))
        xmlENVVAR.setAttribute('value', '%s' % (lEnv[envvar]))
    xmlENVVAR = xmlDOC.createElement('env')
    xmlPROCSPEC.appendChild(xmlENVVAR)
    xmlENVVAR.setAttribute('name', 'MPI_UNIVERSE_SIZE')
    xmlENVVAR.setAttribute('value', '%s' % (usize))
    xmlENVVAR = xmlDOC.createElement('env')
    xmlPROCSPEC.appendChild(xmlENVVAR)
    xmlENVVAR.setAttribute('name', 'MPI_APPNUM')
    xmlENVVAR.setAttribute('value', '%s' % str(appnum))

    appnum += 1


def usage():
    print ''
    print 'mpiexec [ -h   or  -help   or  --help ]'
    print 'mpiexec -file filename  # where filename contains xml for job description'
    print 'mpiexec -configfile filename  # where filename contains cmd-line arg-sets'
    print 'mpiexec [ -default defaultArgs : ] argset : more_arg_sets : ...'
    print '    where each argset contains some of:'
    print '        -n <n> -host <h> -wdir <w> -path <p> cmd args '
    print '    note: cmd must be specfied for each argset; it can not be a default arg'
    print '    other default arguments can be -l (line labels on stdout, stderr) and'
    print '    -setenvall (pass entire environment of mpiexec to all processes),'
    print '    -env KEY1 VALUE1 -env KEY2 VALUE2 ...'
    print '    defaultArgs are passed to all processes unless overridden'
    print 'sample executions:'
    print '    mpiexec -n 1 pwd : -wdir /tmp pwd : printenv'
    print '    mpiexec -default -n 2 -wdir /bin -env RMB3=e3 : pwd : printenv'
    print ''
    exit(-1)

if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass
