ALL: mpich2.ps
SHELL = /bin/sh

.SUFFIXES: .ps .dvi .tex

.dvi.ps:
	dvips $*
#.tex.dvi:
#	-latex $*
#	-bibtex $*
#	-latex $*
#	-latex $*

mpich2.ps: mpich2.tex Makefile fixupindex
	latex mpich2
	-bibtex mpich2
	-rm -f tmp.idx tmp.ind
	cat mpich2.idx | ./fixupindex > tmp.idx
	makeindex tmp ;\
	sed -e 's/\([A-Za-z0-9_!]*\)_MPID,/MPID_\1,/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_MPI,/MPI_\1,/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_MPI$$/MPI_\1/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_PMPI,/PMPI_\1,/g' \
	    -e 's/\([A-Za-z0-9_!]*\)_MPIi,/MPIi_\1,/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' \
		tmp.ind > mpich2.ind ; \
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

