#!/usr/bin/env  python

from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path
from popen2 import Popen3
from pwd    import getpwuid

global totalProcs, nextRange, argvCopy, configFile

def mpiexec():
    global totalProcs, nextRange, argvCopy, configFile
    totalProcs = 0
    nextRange  = 0
    xmlForArgsets = []
    xmlString = ""

    if len(argv) < 2:
	usage()
    if argv[1] == '-file':
	if len(argv) != 3:
	    usage()
        xmlFilename = argv[2]
	delXmlFile = 0
    else:
        configFile = 0
        if argv[1] == '-configfile':
	    if len(argv) != 3:
	        usage()
            configFile = open(argv[2],'r')
        else:
            argvCopy = argv[1:]
            argvCopy.append(':')   # stick an extra : on the end
        argset = get_next_argset()
        while argset:
	    xmlForArgset = handle_argset(argset)
	    xmlString += xmlForArgset
	    argset = get_next_argset()
    
        xmlString = "<PMRequests>\n"                             +  \
                    "    <create-process-group\n"                +  \
                    "        totalprocs='%d'\n" % (totalProcs)   +  \
                    "        >\n"                                +  \
                    xmlString                                    +  \
                    "    </create-process-group>\n"              +  \
                    "</PMRequests>\n"
        xmlFilename = '/tmp/%s_tempxml_%d' % (getpwuid(getuid())[0],getpid())
        xmlFile = open(xmlFilename,'w')
        print >>xmlFile, xmlString
        xmlFile.close()
	delXmlFile = 1
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
                argset.append(line[1:].strip())
    else:
        if argvCopy:
            colonPos = argvCopy.index(':')
	    argset = argvCopy[0:colonPos]
	    argvCopy = argvCopy[ colonPos+1 : ]
	    try:    colonPos = argvCopy.index(':')
	    except: colonPos = -1
    return argset

def handle_argset(argset):
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
        if argset[argidx] == '-n':
            nProcs = int(argset[argidx+1])
        elif argset[argidx] == '-host':
            host = argset[argidx+1]
        elif argset[argidx] == '-wdir':
            wdir = argset[argidx+1]
        elif argset[argidx] == '-path':
            wpath = argset[argidx+1]
        else:
            print 'unsupported or unrecognized option: %s' % argset[i]
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
    xmlForArgset = ""
    if host:
        xmlForArgset += "        <host name='%s' range='%d-%d'/>\n" % \
            (host,thisRange[0],thisRange[1])
    xmlForArgset += "        <path name='%s' range='%d-%d'/>\n" % \
        (wpath,thisRange[0],thisRange[1])
    xmlForArgset += "        <cwd name='%s' range='%d-%d'/>\n" % \
        (wdir,thisRange[0],thisRange[1])
    xmlForArgset += "        <exec name='%s' range='%d-%d'/>\n" % \
        (cmdAndArgs[0],thisRange[0],thisRange[1])
    xmlForArgset += "        <args range='%d-%d'>\n" % \
            (thisRange[0],thisRange[1])
    for arg in cmdAndArgs[1:]:
        xmlForArgset += "            <arg value='%s'/>\n" % (arg)
    xmlForArgset += "        </args>\n"
    # print xmlForArgset
    return xmlForArgset

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
