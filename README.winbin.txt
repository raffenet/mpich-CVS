MPICH2 for Microsoft Windows


SYSTEM REQUIREMENTS:


1) MS Development Environment 2003 to compile C MPI programs.
2) Intel Fortran 8.0 to compile Fortran MPI programs.
3) Administrator privileges on your machines to install the process manager, smpd.exe.


THE INSTALLER:


You must install MPICH2 on all machines that you want to run MPI programs on.
MPICH2 does not install on your "network".  It installs on each machine individually.

The installer creates the following mpich2 directory structure on your machine:
mpich2\bin
mpich2\include
mpich2\lib

The include and lib directories contain the libraries needed to compile MPI programs.
The mpich2 dlls are copied to the Windows\system32 directory.  The bin directory contains
smpd.exe which is the MPICH2 process manager used to launch MPI programs.  mpiexec.exe is
also found in the bin directory.  It is used to start MPICH2 jobs.


MINIMAL INSTALLATION:


If you want to have worker nodes that do not have any tools on them, you can simply copy smpd.exe
to each node and execute "smpd.exe -install".  smpd.exe is the only application required on
each machine to launch MPICH2 jobs.  But MPICH2 applications require the mpich2 dlls.  This
requirement can be satisfied by copying the mpich2 dlls to the windows\system32 directory on
each node.  Then any mpich2 application can run on those systems.  This is what the installer
does.  If you don't want to copy the mpich2 dlls to each machine, then you need to place the dlls
in the same location as the executable you are going to launch.  For example, if you have a
directory called \\myserver\mysharedfolder and you have myapp.exe and mpich2.dll in that directory
then you can execute this command:  "mpiexec -n 4 \\myserver\mysharedfolder\myapp.exe"
Note: There are several mpich2 dlls and depending on your build target (Fortran, C, Debug or Release)
you will need the corresponding dll in the application directory.


COMPILING:


Compiling an MPI program:
1) Create a project or makefile for Visual Studio 2003, or Intel Fortran 8.0
2) Add mpich2\include to the include path
3) Add mpich2\lib to the library path
4) Add mpich2.lib to your Release target link command and
   mpich2d.lib to your Debug target link command.
5) Fortran applications must add mpich2f.lib and mpich2fd.lib to their link commands.
6) Compile
7) Place your application and all the dlls it depends on in a shared location or copy
   them to all the nodes.
8) Run the application using mpiexec


RUNNING MPI JOBS:


mpiexec is a command line application used to launch MPI jobs.  
Bring up a command prompt to run it.  Execute mpiexec to see the available options.

The simplest mpiexec command is like this:
mpiexec -n 3 myapp.exe


Email bugs and error reports to mpich2-maint@mcs.anl.gov.
