ALL: adi3new.ps tcpadi3.ps
SHELL = /bin/sh

#
# Use doctext in /home/gropp/bin/$ARCH/doctext 
DOCTEXT = doctext
.SUFFIXES: .ps .pdf .dvi .tex .eps .fig

adi3.dvi: adi3.tex
paper.dvi: paper.tex

.dvi.ps:
	dvips $*
.dvi.pdf:
	(unset TEXMF ; dvipdfm $* )
.tex.dvi:
	-latex $*
	-bibtex $*
	-latex $*
	-latex $*

adi3man.ps: adi3man.tex adi3.c adi3.h Makefile
	-if [ ! -d adi3man ] ; then mkdir adi3man ; fi
	-rm -f adi3man/*.tex adi3man/tmp.*
	(cd adi3man ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../adi3.c ;\
	${DOCTEXT} -latex -nolocation -quotefmt ../adi3.h ;)
	${MAKE} lists
	-cp adi3man.tex adi3man/adi3man.tex
	(cd adi3man ; \
	../manextract ../adi3.[ch] > /dev/null ;\
	latex adi3man < /dev/null ;\
	bibtex adi3man ;\
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		adi3man.idx > tmp.idx ; \
	makeindex tmp ;\
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > adi3man.ind ; \
	bibtex adi3man ;\
	latex adi3man < /dev/null ;\
	dvips adi3man )
	rm -f adi3man.ps
	mv adi3man/adi3man.ps .
	mv adi3man/adi3man.dvi .

# This isn't ready yet.  We'd like to use doctext to generate the 
# HTML for the 
adi3man/adi3man.htm: adi3man.tex adi3.c adi3.h Makefile
	-rm -rf adi3man
	tohtml -default adi3man.tex
	tohtml -default adi3man.tex

#adi3new/adi3new.htm: adi3new.tex adi3.c adi3.h Makefile
#	-rm -rf adi3new
#	tohtml -default adi3new.tex
#	tohtml -default adi3new.tex

adi3new.ps: adi3new.tex adi3.c adi3.h adi-ov.eps Makefile
	touch adi3new.ind
	-if [ ! -d adi3man ] ; then mkdir adi3man ; fi
	-rm -f adi3man/*.tex adi3man/tmp.*
	(cd adi3man ; \
	${DOCTEXT} -latex -nolocation -quotefmt ../adi3.c ;\
	${DOCTEXT} -latex -nolocation -quotefmt ../adi3.h ;\
	../manextract ../adi3.[ch] > /dev/null )
	latex adi3new < /dev/null
	-bibtex adi3new
	sed -e 's/{\\tt *\\char *`\\_ *}/_/g' \
	    -e 's/MPID_\([A-Za-z0-9]*\)/\1_MPID/g' \
	    -e 's/MPIU_\([A-Za-z0-9]*\)/\1_MPIU/g' \
		adi3new.idx > tmp.idx
	-makeindex tmp
	sed -e 's/\([A-Za-z0-9]*\)_MPID/MPID_\1/g' \
	    -e 's/\([A-Za-z0-9]*\)_MPIU/MPIU_\1/g' \
	    -e 's/_/\\texttt{\\char`\\_}/g' tmp.ind > adi3new.ind 
	-bibtex adi3new 
	latex adi3new < /dev/null
	dvips adi3new

# This isn't ready yet.  We'd like to use doctext to generate the 
# HTML for the adi document
adi3new/adi3new.htm: adi3new.tex adi3.c adi3.h Makefile
	tohtml -default -dosnl adi3new.tex
	tohtml -default -dosnl adi3new.tex

core.dvi: core.tex adi3.c adi3.h
	touch core-list.ind
	-latex core
	-latex core

# This uses the full-featured date program in Linux
WEBDIR = /mcs/www-unix/mpi/mpich/adi3
install-web: adi3new.ps adi3new.pdf adi3new/adi3new.htm
	-rm -f ${WEBDIR}/adi3.[ch]
	cp adi3.[ch] ${WEBDIR}
	cp adi3new.ps ${WEBDIR}/adi3man.ps
	cp adi3new.pdf ${WEBDIR}/adi3man.pdf
	rm -rf ${WEBDIR}/adi3man
	cp -r adi3new ${WEBDIR}/adi3man
	mv ${WEBDIR}/adi3man/adi3new.htm ${WEBDIR}/adi3man/adi3man.htm
	cp ${WEBDIR}/index.html .foo
	sed "s/Last updated .*$$/Last updated on `date '+%B %e, %Y'`./" \
		.foo > ${WEBDIR}/index.html
	rm -f .foo
install-web-old: adi3man.ps adi3man.pdf
	-rm -f ${WEBDIR}/adi3.[ch]
	cp adi3.[ch] ${WEBDIR}
	cp adi3man.ps ${WEBDIR}
	cp adi3man.pdf ${WEBDIR}
	cp ${WEBDIR}/index.html .foo
	sed "s/Last updated .*$$/Last updated on `date '+%B %e, %Y'`./" \
		.foo > ${WEBDIR}/index.html
	rm -f .foo

clean:
	rm -f *.log *.aux *.dvi adi3.ps *.bbl paper.ps findmod adi3man.ps
	rm -f adi3man/*.tex

#
# The following are helpers for assembling the reference manual
findmod: adi3.c adi3.h Makefile
	rm -f .findmod
	@(cd adi3man ; \
	for file in *.tex ; do \
	if [ $$file = "adi3man.tex" ] ; then continue ; fi ; \
	awk '{if (pnext == 1) { printf("%s",$$0); pnext=2; exit;} };\
	/\{Module\}/ {pnext = 1;}; \
	END{if (pnext != 2) printf("None");}' $$file | \
	sed 's/{.*\\_}/_/g' >> ../.findmod ;\
	echo " $$file" >> ../.findmod;\
	done )
	sort .findmod > findmod
	rm -f .findmod
lists: findmod
	cd adi3man ; \
	awk 'BEGIN{module=""};{if ($$1 != module) { \
	file=$$1 "-list.tex"; module=$$1; };\
	print "\\input " $$2>>file;}' ../findmod

# Handle older fig2dev programs
.fig.eps:
	-fig2dev -L eps -l landscape $*.fig > $*.eps
	if [ ! -s $*.eps ] ; then \
		fig2dev -L ps -l landscape $*.fig > $*.eps ; fi

goals.dvi: goals.tex adi-ov.eps
