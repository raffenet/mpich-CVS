ALL: mpich2.ps
SHELL = /bin/sh

.SUFFIXES: .ps .dvi .tex

.dvi.ps:
	dvips $*
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

mpich2.ps: mpich2.tex 
	latex mpich2
	latex mpich2
	dvips mpich2

# This uses the full-featured date program in Linux
install-web: mpich2.ps
	cp mpich2.ps /mcs/www-unix/mpi/mpich/adi3
	cp /mcs/www-unix/mpi/mpich/adi3/index.html .foo
	sed "s/Last updated .*$$/Last updated on `date '+%B %e, %Y'`./" \
		.foo > /mcs/www-unix/mpi/mpich/adi3/index.html
	rm -f .foo

clean:
	rm -f *.log *.aux *.dvi *.bbl mpich2.ps *~ *.toc

