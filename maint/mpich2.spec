Summary: A Portable Implementation of MPI
Name: mpich2
Version: 0.92
Release: 1
Copyright: freely distributable
Group: Development/Libraries
Source: %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-buildroot

%define prefixdir /opt/mpich2/

#
# This spec file derived from Bill Gropp's one in mpich-1.2.5.
# Altered to build in a Rocks development environment, and to
# simplify a bit. We build for as many compilers as we have, instead
# of just one.
#
# Assumes that compilers are in your PATH.
#

%description
MPICH is a freely available, portable implementation of MPI (Message Passing
Interface), the Standard for message-passing libraries.
This version is configured to use ssh instead of rsh when using mpirun.
This is the MPD over ethernet version of MPICH. The fast, failure-resistant
MPD daemons start parallel jobs, instead of the standard SSH-based mpirun.


%prep
# The -n arg defines the name of the top level directory in the tarball.
%setup -n mpich2-%version

%build

mkdir -p %{prefixdir}

#
# Intel compilers (Gucci)
#
%ifarch ia64
#COMPILERS="ecc"
%else
#COMPILERS="icc"
%endif

#
# GNU compilers (Blue Collar, Bulletproof)
#
COMPILERS="$COMPILERS gcc"

for cc in $COMPILERS
do
	echo "Processing $cc compiler"

	# Need to use this construct to prevent
	# RPM from aborting - the return code must be zero.
	which $cc || continue
	CC=`which $cc`
	INSTALL_DIR=%{prefixdir}/$cc

	mkdir -p $RPM_BUILD_ROOT/$INSTALL_DIR

	# Case statement for compiler-specific options.
	case $cc in
		gcc)
			;;
		icc)
			;;
		ecc)
			;;
	esac

	# Trick the package into installing to the RPM ROOT.
	# This will erase any existing installation, but I assume users
	# do not save work here.
        rm -rf $INSTALL_DIR
	ln -s $RPM_BUILD_ROOT/$INSTALL_DIR $INSTALL_DIR

	# Only one copy of docs, even with multiple compilers.
	./configure --prefix=$INSTALL_DIR \
		--with-device=ch3:shm \
		--with-pm=mpd \
		--oldincludedir=$INSTALL_DIR/include \
		--infodir=%{prefixdir}/info \
		--mandir=%{prefixdir}/man \
		--enable-root

	make
	make install

done

%install


%post	-p /sbin/ldconfig

%postun -p /sbin/ldconfig


%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYRIGHT README
%{prefixdir}


%changelog
* Tue Jan 14 2003 Federico Sacerdoti <fds@sdsc.edu>
- Added compiler for loop structure, removed doc duplication.
- Package builds correctly in a user's BUILDROOT sandbox.

* Fri Feb 25 2000 Tim Powers <timp@redhat.com>
- built for Powertools 6.2

* Fri Feb 18 2000 Mike Wangsmo <wanger@redhat.com>
- moved everything to /usr/share/mpi so mpi can coexist w/ LAM

* Fri Feb 04 2000 Mike Wangsmo <wanger@redhat.com>
- pulled out upshot (I don't mpich to require X).

* Mon Jan 31 2000 Cristian Gafton <gafton@redhat.com>
- buildroot
