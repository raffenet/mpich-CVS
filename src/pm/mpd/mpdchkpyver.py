#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from sys     import argv, exit
from os      import environ, execvpe
from mpdlib  import mpd_check_python_version

vinfo = mpd_check_python_version()
if vinfo:
    print "mpdchkpyver: your python version must be >= 2.2 ; current version is:", vinfo
    exit(-1)
if len(argv) > 1:
    mpdpgm = argv[1] + '.py'
    # print "CHKPYVER: PGM=:%s: ARGV[1:]=:%s:" % (mpdpgm,argv[1:])
    try:
        execvpe(mpdpgm,argv[1:],environ)    # client
    except Exception, errinfo:
        print 'mpdchkpyver: failed to exec %s; info=%s' % (mpdpgm,errinfo)
        exit(-1)
