ALL: smpd_pmi.pdf smpd_pmi.ps
SHELL = /bin/sh

DOCTEXT = doctext
LATEX=latex
DVIPDF=dvipdfm
.SUFFIXES: .ps .pdf .dvi .tex .eps .fig

smpd_pmi.dvi: smpd_pmi.tex
	-$(LATEX) smpd_pmi
	-bibtex smpd_pmi
	-$(LATEX) smpd_pmi
	-$(LATEX) smpd_pmi

smpd_pmi.ps: smpd_pmi.dvi
	dvips smpd_pmi

smpd_pmi.pdf: smpd_pmi.dvi
	$(DVIPDF) smpd_pmi

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $* )
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

clean:
	rm -f *.log *.aux *.dvi smpd_pmi.ps smpd_pmi.pdf *.bbl
	rm -f *.ilg *.ind *.idx *.blg *.toc

distclean: clean
	rm -f smpd_pmi.ps smpd_pmi.pdf

# Handle older fig2dev programs
.fig.eps:
	-fig2dev -L eps -l landscape $*.fig > $*.eps
	if [ ! -s $*.eps ] ; then \
		fig2dev -L ps -l landscape $*.fig > $*.eps ; fi

