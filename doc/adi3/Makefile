ALL: paper.ps

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

clean:
	rm -f *.log *.aux *.dvi adi3.ps *.bbl paper.ps
