#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path
from popen2 import Popen3
from pwd    import getpwuid
from urllib import quote
import xml.dom.minidom

global totalProcs, nextRange, argvCopy, configLines, configIdx, setenvall
global gEnv, gHost, gWDIR, gPath, gNProcs

def mpiexec():
    global totalProcs, nextRange, argvCopy, configLines, configIdx, setenvall
    global gEnv, gHost, gWDIR, gPath, gNProcs
    totalProcs = 0
    nextRange  = 0
    configLines = []
    configIdx = 0
    xmlForArgsets = []
    defaultArgs = []
    setenvall = 0
    gEnv = {}
    gHost = ''  # default
    gWDIR  = path.abspath(getcwd())   # default
    gPath = environ['PATH']   # default ; avoid name conflict with python path module
    gNProcs = 1  # default

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
                    (var,val) = defaultArgs[gargIdx+1].split('=')
                    gEnv[var] = val
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
        xmlDOC = xml.dom.minidom.Document()
        xmlPMR = xmlDOC.createElement('PMRequests')
        xmlDOC.appendChild(xmlPMR)
        xmlCPG = xmlDOC.createElement('create-process-group')
        xmlPMR.appendChild(xmlCPG)
        argset = get_next_argset()
        while argset:
	    handle_argset(argset,xmlDOC,xmlCPG)
            argset = get_next_argset()
        xmlCPG.setAttribute('totalprocs', str(totalProcs) )  # after handling argsets
        xmlFilename = '/tmp/%s_tempxml_%d' % (getpwuid(getuid())[0],getpid())
        xmlFile = open(xmlFilename,'w')
        print >>xmlFile, xmlDOC.toprettyxml(indent='   ')
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

def handle_argset(argset,xmlDOC,xmlCPG):
    global totalProcs, nextRange, setenvall
    global gEnv, gHost, gWDIR, gPath, gNProcs
    host   = gHost
    wdir   = gWDIR
    wpath  = gPath
    nProcs = gNProcs
    cmdAndArgs = []
    argidx = 0    # can not use range here
    while argidx < len(argset):
        if argset[argidx][0] != '-':
            break
        if argset[argidx] == '-n' or argset[argidx] == '-np':
            nProcs = int(argset[argidx+1])
        elif argset[argidx] == '-host':
            host = argset[argidx+1]
        elif argset[argidx] == '-wdir':
            wdir = argset[argidx+1]
        elif argset[argidx] == '-path':
            wpath = argset[argidx+1]
        else:
            print 'unsupported or unrecognized option: %s' % argset[argidx]
            usage()
        argidx += 2  # currently, all args have subargs
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

    if host:
        xmlHOST = xmlDOC.createElement('host')
        xmlCPG.appendChild(xmlHOST)
        xmlHOST.setAttribute('name',host)
        xmlHOST.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 

    xmlPATH = xmlDOC.createElement('path')
    xmlCPG.appendChild(xmlPATH)
    xmlPATH.setAttribute('name',wpath)
    xmlPATH.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 

    xmlCWD = xmlDOC.createElement('cwd')
    xmlCPG.appendChild(xmlCWD)
    xmlCWD.setAttribute('name',wdir)
    xmlCWD.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 

    xmlEXEC = xmlDOC.createElement('exec')
    xmlCPG.appendChild(xmlEXEC)
    xmlEXEC.setAttribute('name',cmdAndArgs[0])
    xmlEXEC.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 

    xmlARGS = xmlDOC.createElement('args')
    xmlCPG.appendChild(xmlARGS)
    xmlARGS.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 
    for arg in cmdAndArgs[1:]:
        xmlARG = xmlDOC.createElement('arg')
        xmlARGS.appendChild(xmlARG)
        xmlARG.setAttribute('value', '%s' % (quote(arg)))

    xmlENVVARS = xmlDOC.createElement('envvars')
    xmlCPG.appendChild(xmlENVVARS)
    xmlENVVARS.setAttribute('range','%d-%d' % (thisRange[0],thisRange[1]) ) 
    if setenvall:
        for envvar in environ:
            xmlENVVAR = xmlDOC.createElement('envvar')
            xmlENVVARS.appendChild(xmlENVVAR)
            xmlENVVAR.setAttribute('key', '%s' % (envvar))
            xmlENVVAR.setAttribute('value', '%s' % (environ[envvar]))
    for envvar in gEnv.keys():
        xmlENVVAR = xmlDOC.createElement('envvar')
        xmlENVVARS.appendChild(xmlENVVAR)
        xmlENVVAR.setAttribute('key', '%s' % (envvar))
        xmlENVVAR.setAttribute('value', '%s' % (gEnv[envvar]))

def usage():
    print ''
    print 'mpiexec [ -h   or  -help   or  --help ]'
    print 'mpiexec -file filename  # where filename contains pre-built xml for mpdrun'
    print 'mpiexec -configfile filename  # where filename contains cmd-line arg-sets'
    print 'mpiexec -default defaultArgs -n <n> -host <h> -wdir <w> -path <p> cmd args : more_arg_sets : ...'
    print '    defaultArgs are passed to all processes unless overridden'
    print 'sample execution:'
    print '    mpiexec.py  -default -n 2 -wdir /bin -env RMB3=e3 : pwd : -wdir /tmp pwd : printenv'
    print ''
    exit(-1)

if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass
