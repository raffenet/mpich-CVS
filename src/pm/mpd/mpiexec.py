#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys    import argv, exit
from os     import environ, execvpe, getpid, getuid, getcwd, access, X_OK, path, unlink, \
                   open, fdopen, O_CREAT, O_WRONLY, O_EXCL, O_RDONLY
from popen2 import Popen3
from pwd    import getpwuid
from urllib import quote
import xml.dom.minidom

global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
global validGlobalArgs, globalArgs, validLocalArgs, localArgSets

def mpiexec():
    global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets

    validGlobalArgs = { '-l' : 0, '-usize' : 1, '-gdb' : 0, '-bnr' : 0, '-tv' : 0,
                        '-gn' : 1, '-gnp' : 1, '-ghost' : 1, '-gpath' : 1, '-gwdir' : 1, '-gexec' : 1,
                        '-genv' : 2, '-genvnone' : 0, '-genvlist' : 1 }
    validLocalArgs  = { '-n' : 1, '-np' : 1, '-host' : 1, '-path' : 1, '-wdir' : 1, '-soft' : 0,
                        '-env' : 2, '-envnone' : 0, '-envlist' : 1 }

    globalArgs   = {}
    localArgSets = {}
    localArgSets[0] = []

    totalProcs    = 0
    nextRange     = 0
    configLines   = []
    configIdx     = 0
    xmlForArgsets = []
    appnum        = 0

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
            configFileFD = open(argv[2],O_RDONLY)
            configFile = fdopen(configFileFD,'r',0)
            configLines = configFile.readlines()
            configLines = [ x.strip() + ' : '  for x in configLines if x[0] != '#' ]
            tempargv = []
            for line in configLines:
                shOut = Popen3("/bin/sh -c 'for a in $*; do echo _$a; done' -- %s" % (line))
                for shline in shOut.fromchild:
                    tempargv.append(shline[1:].strip())    # 1: strips off the leading _
	    tempargv = [argv[0]] + tempargv[0:-1]   # strip off the last : I added
            collect_args(tempargv)
        else:
            collect_args(argv)

        xmlDOC = xml.dom.minidom.Document()
        xmlCPG = xmlDOC.createElement('create-process-group')
        xmlDOC.appendChild(xmlCPG)
        for k in localArgSets.keys():
            xmlPROCSPEC = xmlDOC.createElement('process-spec')
            xmlCPG.appendChild(xmlPROCSPEC)
	    handle_argset(localArgSets[k],xmlDOC,xmlPROCSPEC)
        xmlCPG.setAttribute('totalprocs', str(totalProcs) )  # after handling argsets
        if globalArgs['-l']:
            xmlCPG.setAttribute('output', 'label')
        if globalArgs['-bnr']:
            xmlCPG.setAttribute('doing_bnr', '1')
        if globalArgs['-gdb']:
            xmlCPG.setAttribute('gdb', '1')
        if globalArgs['-tv']:
            xmlCPG.setAttribute('tv', '1')
        submitter = getpwuid(getuid())[0]
        xmlCPG.setAttribute('submitter', submitter)
        xmlFilename = '/tmp/%s_tempxml_%d' % (submitter,getpid())
        try:    unlink(xmlFilename)
	except: pass
        xmlFileFD = open(xmlFilename,O_CREAT|O_WRONLY|O_EXCL,0600)
        xmlFile = fdopen(xmlFileFD,'w',0)
        print >>xmlFile, xmlDOC.toprettyxml(indent='   ')
        # print xmlDOC.toprettyxml(indent='   ')    #### TEMP DEBUG
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

def collect_args(args):
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets
    globalArgs['-l']        = 0
    globalArgs['-usize']    = 1
    globalArgs['-gdb']      = 0
    globalArgs['-bnr']      = 0
    globalArgs['-tv']       = 0
    globalArgs['-gn']       = 1
    globalArgs['-ghost']    = '_any_'
    globalArgs['-gpath']    = environ['PATH']
    globalArgs['-gwdir']    = path.abspath(getcwd())
    globalArgs['-gexec']    = ''
    globalArgs['-genv']     = {}
    globalArgs['-genvlist'] = []
    globalArgs['-genvnone'] = 0
    argidx = 1
    while argidx < len(args)  and  args[argidx] in validGlobalArgs.keys():
        garg = args[argidx]
	if garg == '-gnp':    # alias for '-gn'
	    garg = '-gn'
        if validGlobalArgs[garg] > 0:
            if garg == '-genv':
                globalArgs['-genv'][args[argidx+1]] = args[argidx+2]
                argidx += 3
            else:
		if garg == 'usize'  or  garg == '-gn':
                    globalArgs[garg] = int(args[argidx+1])
		else:
                    globalArgs[garg] = args[argidx+1]
                argidx += 2
        else:
            globalArgs[garg] = 1
            argidx += 1
    localArgsKey = 0
    while argidx < len(args):
        if args[argidx] == ':':
            localArgsKey += 1
            localArgSets[localArgsKey] = []
        else:
            localArgSets[localArgsKey].append(args[argidx])
        argidx += 1

def handle_argset(argset,xmlDOC,xmlPROCSPEC):
    global totalProcs, nextRange, argvCopy, configLines, configIdx, appnum
    global validGlobalArgs, globalArgs, validLocalArgs, localArgSets
    host   = globalArgs['-ghost']
    wdir   = globalArgs['-gwdir']
    wpath  = globalArgs['-gpath']
    nProcs = globalArgs['-gn']
    usize  = globalArgs['-usize']
    gexec  = globalArgs['-gexec']
    if globalArgs['-genvnone']:
        envall = 0
    else:
        envall = 1
    if globalArgs['-genvlist']:
        globalArgs['-genvlist'] = globalArgs['-genvlist'].split(',')
    localEnvlist = []
    localEnv  = {}

    argidx = 0
    while argidx < len(argset):
        if argset[argidx] not in validLocalArgs:
            if argset[argidx][0] == '-':
                print 'unknown option: %s' % argset[argidx]
                usage()
            break                       # since now at executable
        if argset[argidx] == '-n' or argset[argidx] == '-np':
            if len(argset) < (argidx+2):
                print '** missing arg to -n'
                usage()
            nProcs = argset[argidx+1]
            if not nProcs.isdigit():
                print '** non-numeric arg to -n: %s' % nProcs
                usage()
            nProcs = int(nProcs)
            argidx += 2
        elif argset[argidx] == '-host':
            if len(argset) < (argidx+2):
                print '** missing arg to -host'
                usage()
            host = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-path':
            if len(argset) < (argidx+2):
                print '** missing arg to -path'
                usage()
            wpath = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-wdir':
            if len(argset) < (argidx+2):
                print '** missing arg to -wdir'
                usage()
            wdir = argset[argidx+1]
            argidx += 2
        elif argset[argidx] == '-soft':
            print '** -soft is accepted but not used'
            argidx += 1
        elif argset[argidx] == '-envall':
            envall = 1
            argidx += 1
        elif argset[argidx] == '-envnone':
            envall = 0
            argidx += 1
        elif argset[argidx] == '-envlist':
            localEnvlist = argset[argidx+1].split(',')
            argidx += 2
        elif argset[argidx] == '-env':
            if len(argset) < (argidx+3):
                print '** missing arg to -env'
                usage()
            var = argset[argidx+1]
            val = argset[argidx+2]
            localEnv[var] = val
            argidx += 3
        else:
            print 'unknown option: %s' % argset[argidx]
            usage()
    cmdAndArgs = []
    if argidx < len(argset):
        while argidx < len(argset):
            cmdAndArgs.append(argset[argidx])
            argidx += 1
    else:
        if gexec:
            cmdAndArgs = [gexec]
    if not cmdAndArgs:
        print 'no cmd specified'
        usage()

    if nProcs == 1:
        thisRange = (nextRange,nextRange)
    else:
        thisRange = (nextRange,nextRange+nProcs-1)
    nextRange += nProcs
    totalProcs += nProcs

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

    envToSend = {}
    if envall:
        for envvar in environ.keys():
            envToSend[envvar] = environ[envvar]
    for envvar in globalArgs['-genvlist']:
        envToSend[envvar] = environ[envvar]
    for envvar in localEnvlist:
        envToSend[envvar] = environ[envvar]
    for envvar in globalArgs['-genv'].keys():
        envToSend[envvar] = globalArgs['-genv'][envvar]
    for envvar in localEnv.keys():
        envToSend[envvar] = localEnv[envvar]
    for envvar in envToSend.keys():
        xmlENVVAR = xmlDOC.createElement('env')
        xmlPROCSPEC.appendChild(xmlENVVAR)
        xmlENVVAR.setAttribute('name',  '%s' % (envvar))
        xmlENVVAR.setAttribute('value', '%s' % (envToSend[envvar]))
    if usize:
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
    print 'usage:'
    print 'mpiexec [-h or -help or --help]    # get this message'
    print 'mpiexec -file filename             # filename contains XML job description'
    print 'mpiexec [global args] [local args] executable [args]'
    print '   where global args may be'
    print '      -l                           # line labels by MPI rank'
    print '      -bnr                         # MPICH1 compatibility mode'
    print '      -genvall                     # pass all env vars in current environment'
    print '      -genvnone                    # pass no env vars'
    print '      -genvlist <list of env var names> # pass current values of these vars'
    print '      -genv <name> <value>         # pass this value of this env var'
    print '      -g<local arg name>           # global version of local arg (below)'
    print '    and local args may be'
    print '      -n <n> or -np <n>            # number of processes to start'
    print '      -wdir <dirname>              # working directory to start in'
    print '      -path <dirname>              # place to look for executables'
    print '      -host <hostname>             # host to start on'
    print 'mpiexec [global args] [local args] executable args : [local args] executable...'
    print 'mpiexec -configfile filename       # filename contains cmd line segs as lines'
    print '  (See User Guide for more details)'
    print ''
    print 'Examples:'
    print '   mpiexec -l -n 10 cpi 100'
    print '   mpiexec -genv QPL_LICENSE 4705 -n 3 a.out'
    print '   mpiexec -n 1 -host foo master : -n 4 -host mysmp slave'
    exit(-1)


if __name__ == '__main__':
    try:
        mpiexec()
    except SystemExit, errmsg:
        pass

