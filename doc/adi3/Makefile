ALL: paper.ps
SHELL = /bin/sh

.SUFFIXES: .ps .dvi .tex

adi3.dvi: adi3.tex
paper.dvi: paper.tex

.dvi.ps:
	dvips $*
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

adi3man.ps: adi3man.tex adi3.c adi3.h
	-if [ ! -d adi3man ] ; then mkdir adi3man ; fi
	-cp adi3man.tex adi3man/adi3man.tex
	(cd adi3man ; \
	doctext -latex -nolocation -quotefmt -outfile adi3func.tex ../adi3.c ;\
	doctext -latex -nolocation -quotefmt -outfile adi3data.tex ../adi3.h ;\
	latex adi3man ;\
	dvips adi3man )
	if [ -s adi3man/adi3man.ps ] ; then cp adi3man/adi3man.ps . ; fi
clean:
	rm -f *.log *.aux *.dvi adi3.ps *.bbl paper.ps
