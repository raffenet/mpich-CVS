#!/usr/bin/env python

from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path
from popen2 import Popen3
from pwd    import getpwuid
from urllib import quote
import xml.dom.minidom

global totalProcs, nextRange, argvCopy, configFile

def mpiexec():
    global totalProcs, nextRange, argvCopy, configFile
    totalProcs = 0
    nextRange  = 0
    xmlForArgsets = []

    if len(argv) < 2:
	usage()
    if argv[1] == '-file':
	delXmlFile = 0
	if len(argv) != 3:
	    usage()
        xmlFilename = argv[2]
    else:
	delXmlFile = 1
        configFile = 0
        if argv[1] == '-configfile':
	    if len(argv) != 3:
	        usage()
            configFile = open(argv[2],'r')
        else:
            argvCopy = argv[1:]
            argvCopy.append(':')   # stick an extra : on the end
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
    mpdrun = path.normpath(fullDirName + '/mpdrun.py')
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
    global argvCopy, configFile
    argset = []
    if configFile:
        line = configFile.readline().strip()
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
    global totalProcs, nextRange
    host  = ''   # default
    wdir  = path.abspath(getcwd())   # default
    wpath = environ['PATH']   # default ; avoid name conflict with python path module
    nProcs = 1  # default
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

def usage():
    print ''
    print 'mpiexec -n <n> -host <h> -wdir <w> -path <p> cmd args : more_arg_sets : ...'
    print 'mpiexec -configfile filename  # where filename contains arg_sets'
    print 'mpiexec -file filename  # where filename contains pre-built xml for mpdrun'
    exit(-1)

if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass
