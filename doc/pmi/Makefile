ALL: paper.ps

LATEX=latex
DVIPDF=dvipdfm

paper.dvi: paper.tex paper.bib
	-$(LATEX) paper
	-bibtex paper
	-$(LATEX) paper
	-$(LATEX) paper

paper.ps: paper.dvi
	dvips paper

paper.pdf: paper.dvi
	$(DVIPDF) paper

clean:
	rm -f *.log *.dvi paper.bbl paper.aux *~ paper.blg
distclean: clean
	rm -f paper.ps paper.pdf

