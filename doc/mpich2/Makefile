ALL: mpich2.ps
SHELL = /bin/sh

.SUFFIXES: .ps .dvi .tex .fig .eps

.dvi.ps:
	dvips $*
#.tex.dvi:
#	-latex $*
#	-bibtex $*
#	-latex $*
#	-latex $*

# Here is a default target for subsections as separate docs
.tex.dvi:
	sed -e s/@dummy@/$*.tex/ \
	    -e s/@paragraph@/paragraph/g \
	    -e s/@newparagraph@/subsection/g \
	    -e s/@subsubsection@/subsubsection/g \
	    -e s/@newsubsubsection@/subsection/g \
	    -e s/@subsection@/subsection/g \
	    -e s/@newsubsection@/section/g \
	    -e s/@section@/section/g \
	    -e s/@newsection@/relax/g \
	template.orig > template.tex
	latex template
	-bibtex template
	latex template
	mv template.dvi $*.dvi

# We touch mpich2.ind to work around a LaTeX bug in accessing the index file
mpich2.ps: mpich2.tex errcodes.tex pt2ptdesc.tex \
	Makefile fixupindex scanpattern.eps mpidoc.sty
	touch mpich2.ind
	latex mpich2
	-bibtex mpich2
	-rm -f tmp.idx tmp.ind
	cat mpich2.idx | ./fixupindex > tmp.idx
	makeindex tmp ;\
	sed -e 's/\([A-Za-z0-9_!]*\)_MPID,/MPID_\1,/g' \
	    -e 's/\([A-Za-z0-9_!]*\)_MPID$$/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9_!]*\)_MPIi,/MPIi_\1,/g' \
	    -e 's/\([A-Za-z0-9_!]*\)_MPIi$$/MPIi_\1/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_MPI,/MPI_\1,/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_MPI$$/MPI_\1/g' \
	    -e 's/\([A-Za-z0-9_!()]*\)_PMPI,/PMPI_\1,/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' \
		tmp.ind > mpich2.ind ; \
	latex mpich2
	latex mpich2
	dvips mpich2

errcodes.dvi: errcodes.tex
	sed -e s/@dummy@/errcodes.tex/ \
	    -e s/@paragraph@/paragraph/g \
	    -e s/@newparagraph@/subsection/g \
	    -e s/@subsubsection@/subsubsection/g \
	    -e s/@newsubsubsection@/subsection/g \
	    -e s/@subsection@/subsection/g \
	    -e s/@newsubsection@/section/g \
	    -e s/@section@/section/g \
	    -e s/@newsection@/relax/g \
	template.orig > template.tex
	latex template
	mv template.dvi errcodes.dvi

pt2ptdesc.dvi: pt2ptdesc.tex Makefile template.orig
	sed -e s/@dummy@/pt2ptdesc.tex/ \
	    -e s/@paragraph@/paragraph/g \
	    -e s/@newparagraph@/paragraph/g \
	    -e s/@subsubsection@/subsubsection/g \
	    -e s/@newsubsubsection@/subsubsection/g \
	    -e s/@subsection@/subsection/g \
	    -e s/@newsubsection@/subsection/g \
	    -e s/@section@/section/g \
	    -e s/@newsection@/section/g \
	    template.orig > template.tex
	latex template
	-bibtex template
	latex template
	mv template.dvi pt2ptdesc.dvi

colldesc.dvi: colldesc.tex Makefile template.orig
	sed -e s/@dummy@/colldesc.tex/ \
	    -e s/@paragraph@/paragraph/g \
	    -e s/@newparagraph@/subsubsection/g \
	    -e s/@subsubsection@/subsubsection/g \
	    -e s/@newsubsubsection@/subsection/g \
	    -e s/@subsection@/subsection/g \
	    -e s/@newsubsection@/section/g \
	    -e s/@section@/section/g \
	    -e s/@newsection@/section/g \
	    template.orig > template.tex
	latex template
	-bibtex template
	latex template
	mv template.dvi colldesc.dvi

mmca.dvi: mmca.tex Makefile template.orig
	sed -e s/@dummy@/mmca.tex/ \
	    -e s/@paragraph@/paragraph/g \
	    -e s/@newparagraph@/paragraph/g \
	    -e s/@subsubsection@/subsubsection/g \
	    -e s/@newsubsubsection@/subsubsection/g \
	    -e s/@subsection@/subsection/g \
	    -e s/@newsubsection@/subsection/g \
	    -e s/@section@/section/g \
	    -e s/@newsection@/section/g \
	    template.orig > template.tex
	latex template
	-bibtex template
	latex template
	mv template.dvi mmca.dvi


# This uses the full-featured date program in Linux
install-web: mpich2.ps
	cp mpich2.ps /mcs/www-unix/mpi/mpich/adi3
	cp /mcs/www-unix/mpi/mpich/adi3/index.html .foo
	sed "s/Last updated .*$$/Last updated on `date '+%B %e, %Y'`./" \
		.foo > /mcs/www-unix/mpi/mpich/adi3/index.html
	rm -f .foo

clean:
	rm -f *.log *.aux *.dvi *.bbl mpich2.ps *~ *.toc errcodes.ps \
		pt2ptdesc.ps

.fig.eps:
	fig2dev -L ps -l landscape $*.fig > $*.eps
