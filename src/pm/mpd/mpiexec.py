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

    configFile = 0
    if argv[1] == '-configfile':
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
    fullDirName = path.abspath(path.split(argv[0])[0])  # normalize for platform also
    mpdrun = path.normpath(fullDirName + '/mpdrun.py')
    if not access(mpdrun,X_OK):
        print 'mpiexec: cannot execute mpdrun %s' % mpdrun
        exit(0);
    execvpe(mpdrun,[mpdrun,'-mpiexec',xmlFilename],environ)
    print 'mpiexec: exec failed for %s' % mpdrun
    exit(0);

def get_next_argset():
    global argvCopy, configFile
    argset = []
    if configFile:
        line = configFile.readline().strip()
        if line:
            # next line: prepend an _ to avoid problems with -n to echo
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
    host = ''   # default
    nProcs = 1  # default
    pgmAndArgs = []
    i = 0    # can not use range here
    while i < len(argset):
        if argset[i][0] == '-':
            if argset[i] == '-n':
                nProcs = int(argset[i+1])
                i += 1
            elif argset[i] == '-host':
                host = argset[i+1]
                i += 1
        else:
            pgmAndArgs.append(argset[i])
        i += 1
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
        (environ['PATH'],thisRange[0],thisRange[1])
    xmlForArgset += "        <cwd name='%s' range='%d-%d'/>\n" % \
        (path.abspath(getcwd()),thisRange[0],thisRange[1])
    xmlForArgset += "        <exec name='%s' range='%d-%d'/>\n" % \
        (pgmAndArgs[0],thisRange[0],thisRange[1])
    xmlForArgset += "        <args range='%d-%d'>\n" % \
            (thisRange[0],thisRange[1])
    for arg in pgmAndArgs[1:]:
        xmlForArgset += "            <arg value='%s'/>\n" % (arg)
    xmlForArgset += "        </args>\n"
    # print xmlForArgset
    return xmlForArgset

def usage():
    print ''
    print 'mpiexec -h '
    print 'Long options:'
    print '  --help '
    print """
mpiexec does something cool
mpiexec is cool
"""

if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass
