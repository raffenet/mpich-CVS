ALL: ch3.ps
SHELL = /bin/sh

DOCTEXT = doctext
.SUFFIXES: .ps .pdf .dvi .tex .eps .fig

ch3.dvi: ch3.tex

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $* )
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

CH3_FILES = ch3.tex ../../src/mpid/ch3/include/mpidpost.h Makefile

ch3.ps: $(CH3_FILES)
	touch ch3.ind
	-if [ ! -d ch3man ] ; then mkdir ch3man ; fi
	-rm -f ch3man/*.tex ch3man/tmp.*
	(cd ch3man ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../../../src/mpid/ch3/include/mpidpost.h ;\
	../manextract ../../../src/mpid/ch3/include/mpidpost.h > /dev/null )
	latex ch3 < /dev/null
	-bibtex ch3
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		ch3.idx > tmp.idx
	-makeindex tmp
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > ch3.ind 
	-bibtex ch3
	latex ch3 < /dev/null
	dvips ch3

ch3.pdf: $(CH3_FILES)
	touch ch3.ind
	-if [ ! -d ch3man ] ; then mkdir ch3man ; fi
	-rm -f ch3man/*.tex ch3man/tmp.*
	(cd ch3man ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../../../src/mpid/ch3/include/mpidpost.h ;\
	../manextract ../../src/mpid/ch3/include/mpidpost.h > /dev/null )
	rm -f ch3pdf.tex
	sed -e 's/documentclass{article}/documentclass[dvipdfm]{article}/' \
	    -e 's/%hyperref .*/\\usepackage[dvipdfm]{hyperref}/' \
	    -e '/usepackage{.*url}/d' \
		ch3.tex > ch3pdf.tex
	latex ch3pdf < /dev/null
	-bibtex ch3pdf
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		ch3pdf.idx > tmp.idx
	-makeindex tmp
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > ch3pdf.ind 
	-bibtex ch3pdf 
	latex ch3pdf < /dev/null
	dvipdfm ch3pdf
	rm -f ch3.pdf
	mv -f ch3pdf.pdf ch3.pdf

clean:
	rm -f *.log *.aux *.dvi ch3.ps *.bbl
	rm -f *.ilg *.ind *.idx *.blg *.toc
	rm -f ch3man/*.tex

install-devdocs: ch3.pdf
	if [ ! -d "$(DEV_INSTALL_PREFIX)" ] ; then \
	    echo "You must set DEV_INSTALL_PREFIX first" ; \
	    exit 1 ; fi
	cp -f ch3.pdf $(DEV_INSTALL_PREFIX)

# Handle older fig2dev programs
.fig.eps:
	-fig2dev -L eps -l landscape $*.fig > $*.eps
	if [ ! -s $*.eps ] ; then \
		fig2dev -L ps -l landscape $*.fig > $*.eps ; fi
