ALL: adi3.ps

.SUFFIXES: .ps .dvi .tex

adi3.dvi: adi3.tex
	-latex adi3
	-bibtex adi3
	-latex adi3
	-latex adi3


.dvi.ps:
	dvips $*

clean:
	rm -f *.log *.aux *.dvi adi3.ps *.bbl
