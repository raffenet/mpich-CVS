ALL: all-redirect
SHELL = /bin/sh

DOCS = mm.ps mm.pdf
MANUALSRC = mpichman.tex mpichdef.sty tpage.sty trouble/trouble.tex config.txt
SOWINGHOME = /home/gropp/sowing-proj/sowing
MPICHHOME = /home/gropp/tmp/cvstesting/mpich
#tohtml = tohtml
tohtml = /home/gropp/sowing-proj/sowing/src/tohtml/tohtml
all-redirect: ${DOCS} ${DEVDOCS} ${MPEDOCS}

# Release manuals does not include the device-only documents, as those are 
# used *only* to help the authors produce the manuals
release-manuals: ${DOCS} ${MPEDOCS}

.SUFFIXES: 
.SUFFIXES: .ps .pdf .dvi .htm .tex .dvips

# Dependencies
mpichman.dvi: ${MANUALSRC} faq/faqlist.tex
mpichman.dvips: ${MANUALSRC} faq/faqlist.tex

faq/faqlist.tex: 
	cd faq && make
tpage.sty:
	cp ${SOWINGHOME}/docs/doctext/tpage.sty .
mpichdef.sty: 
	if [ -n "${DEVICE}" ] ; then \
	    cp device/${DEVICE}/mpichdef.sty . ; \
	else \
	    cp device/mpichdef.sty . ; fi
config.txt:
	if [ -x ${MPICHHOME}/configure ] ; then \
	    (cd ${MPICHHOME} && ./configure --help > config.txt 2>&1 ) ; \
	    mv ${MPICHHOME}/config.txt . ; \
	else \
	    echo "(No configure help file)" > config.txt ; \
	fi

#
# Special versions
${DEVICE}.dvi: ${MANUALSRC} 
	rm -f mpichman.dvi mpichdef.sty ${DEVICE}.dvi devspec-${DEVICE}.dvi
	rm -f mpichman-${DEVICE}.dvi mpichman-${DEVICE}.log
	make DEVICE=${DEVICE} mpichman.dvi
	make DEVICE=${DEVICE} devspec.dvi
	mv mpichman.dvi mpichman-${DEVICE}.dvi
	mv mpichman.log mpichman-${DEVICE}.log
	mv devspec.dvi devspec-${DEVICE}.dvi
devspec-${DEVICE}.dvi: ${DEVICE}.dvi
mpichman-chp4.dvi: ${MANUALSRC} device/chp4/runsetup.tex
	make DEVICE=chp4 chp4.dvi
mpichman-chp4mpd.dvi: ${MANUALSRC} device/chp4mpd/runsetup.tex
	make DEVICE=chp4mpd chp4mpd.dvi
mpichman-chshmem.dvi: ${MANUALSRC} device/chshmem/runsetup.tex
	make DEVICE=chshmem chshmem.dvi
mpichman-globus2.dvi: ${MANUALSRC} device/globus2/runsetup.tex
	make DEVICE=globus2 globus2.dvi
devspec-chp4.dvi: ${MANUALSRC} device/chp4/runsetup.tex
	make DEVICE=chp4 devspec-chp4.dvi
devspec-chp4mpd.dvi: ${MANUALSRC} device/chp4mpd/runsetup.tex
	make DEVICE=chp4mpd devspec-chp4mpd.dvi
devspec-chshmem.dvi: ${MANUALSRC} device/chshmem/runsetup.tex
	make DEVICE=chshmem devspec-chshmem.dvi
devspec-globus2.dvi: ${MANUALSRC} device/globus2/runsetup.tex
	make DEVICE=globus2 devspec-globus2.dvi

${DEVICE}.dvips: ${MANUALSRC} 
	rm -f mpichman.dvi mpichman.dvips mpichdef.sty devspec.ps
	rm -f mpichman-${DEVICE}.log mpichman-${DEVICE}.dvips
	rm -f devspec-${DEVICE}.dvips
	make DEVICE=${DEVICE} mpichman.dvips
	make DEVICE=${DEVICE} devspec.dvips
	mv mpichman.dvips mpichman-${DEVICE}.dvips
	mv mpichman.log mpichman-${DEVICE}.log
	mv devspec.dvips devspec-${DEVICE}.dvips
devspec-${DEVICE}.dvips: ${DEVICE}.dvips
mpichman-chp4.dvips: ${MANUALSRC} device/chp4/runsetup.tex
	make DEVICE=chp4 chp4.dvips
mpichman-chp4mpd.dvips: ${MANUALSRC} device/chp4mpd/runsetup.tex
	make DEVICE=chp4mpd chp4mpd.dvips
mpichman-chshmem.dvips: ${MANUALSRC} device/chshmem/runsetup.tex
	make DEVICE=chshmem chshmem.dvips
mpichman-globus2.dvips: ${MANUALSRC} device/globus2/runsetup.tex
	make DEVICE=globus2 globus2.dvips
devspec-chp4.dvips: ${MANUALSRC} device/chp4/runsetup.tex
	make DEVICE=chp4 devspec-chp4.dvips
devspec-chp4mpd.dvips: ${MANUALSRC} device/chp4mpd/runsetup.tex
	make DEVICE=chp4mpd devspec-chp4mpd.dvips
devspec-chshmem.dvips: ${MANUALSRC} device/chshmem/runsetup.tex
	make DEVICE=chshmem devspec-chshmem.dvips
devspec-globus2.dvips: ${MANUALSRC} device/globus2/runsetup.tex
	make DEVICE=globus2 devspec-globus2.dvips

mpichman/mpichman.htm: mpichdef.sty
	${tohtml} -debugfile -default -dosnl -gaudy -basedef mpiman.def \
		-basedef mpich${DEVICE}.def mpichman.tex
	${tohtml} -default -dosnl -gaudy -basedef mpiman.def \
		-basedef mpich${DEVICE}.def mpichman.tex
mpeman/mpeman.htm: mpeman.tex
	${tohtml} -debugfile -default -dosnl -gaudy -basedef mpiman.def \
		-basedef mpich${DEVICE}.def mpeman.tex
	${tohtml} -default -dosnl -gaudy -basedef mpiman.def \
		-basedef mpich${DEVICE}.def mpeman.tex

mpichman-${DEVICE}/mpichman-${DEVICE}.htm: ${MANUALSRC} mpiman.def
	rm -f mpichdef.sty
	rm -rf mpichman
	-mv -f mpichman-${DEVICE} mpichman
	make DEVICE=${DEVICE} mpichman/mpichman.htm
	mv -f mpichman/mpichman.htm mpichman/mpichman-${DEVICE}.htm
	mv mpichman mpichman-${DEVICE}
htmlmans: ${MANUALSRC} mpiman.def
	touch htmlmans
	for dev in ${DEVICES} ; do \
	    make DEVICE=$$dev mpichman-$$dev/mpichman-$$dev.htm ; \
	done

# Generic rules
#
# The first two latex passes have the output sent to a file and the
# input from /dev/null to suppress the 

#       Donner has been upgraded...
#	@if [ `hostname` = "donner.mcs.anl.gov" ] ; then \
#	    echo "Donner may not be used because of its old latex" ; \
#	    exit 1; fi
.tex.dvi:
	-rm -f manmsg.txt
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; \
		latex $* </dev/null >manmsg.txt 2>&1 )
	-bibtex $*
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; \
		latex $* </dev/null >manmsg.txt 2>&1 )
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; latex $*)
# Special target for dvips
#       Donner has been upgraded...
# 	@if [ `hostname` = "donner.mcs.anl.gov" ] ; then \
# 	    echo "Donner may not be used because of its old latex" ; \
# 	    exit 1; fi
.tex.dvips:
	-rm -f tmp.tex tmp.dvips tmp.log manmsg.txt tmp.aux tmp.bbl tmp.blg
	sed -e '/usepackage.*hyperref/d' \
	    -e 's/class\[dvipdfm/class\[/' \
	    -e 's/%\\usepackage{url}/\\usepackage{url}/g' $*.tex > tmp.tex
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; \
		latex tmp </dev/null >manmsg.txt 2>&1 )
	-bibtex tmp
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; \
		latex tmp </dev/null >manmsg.txt 2>&1 )
	-(TEXINPUTS=${TEXINPUTS}:/home/gropp/bin/share/tex ; latex tmp)
	mv -f tmp.log $*.log 
	mv -f tmp.dvi $*.dvips
.dvips.ps:
	dvips $*.dvips
#.dvi.ps:
#	dvips $* 2>&1 | grep -v 'pdf:'

# Use this since not everyone has dvipdfm in their path
DVIPDFM = /home/gropp/mylocal/linux-2/bin/dvipdfm
.dvi.pdf:
	unset TEXMF ; if dvipdfm $* ; then : ; else ${DVIPDFM} $* ; fi

.tex.htm:
	-tohtml -default -dosnl -basedef mpiman.def $*.tex
	-tohtml -default -dosnl -basedef mpiman.def $*.tex

FTPDIR = /home/ftp/pub/mpi
WEBDIR = /mcs/www/mpi/mpich/docs

install: ${DOCS} htmlmans
	-for file in ${DOCS} ; do \
	    rm -f ${FTPDIR}/$$file ; \
	    cp $$file ${FTPDIR}/$$file ; \
	    if [ "`basename $${file} .ps`"".ps" = "$$file" ] ; then \
	        gzip -c $${file} > ${FTPDIR}/$${file}.gz ; \
	    fi ; \
	done
	-for file in ${DOCS} ; do \
	    rm -f ${WEBDIR}/$$file.new ; \
	    cp -f $$file ${WEBDIR}/$$file.new ; \
	    mv -f ${WEBDIR}/$$file.new ${WEBDIR}/$$file ; \
	done
	-for dev in ${DEVICES} ; do \
	    rm -rf ${WEBDIR}/mpichman-$$dev.new ; \
	    cp -rp mpichman-$$dev ${WEBDIR}/mpichman-$$dev.new ; \
	    rm -rf ${WEBDIR}/mpichman-$$dev.old ;\
	    mv -f ${WEBDIR}/mpichman-$$dev ${WEBDIR}/mpichman-$$dev.old ; \
	    mv -f ${WEBDIR}/mpichman-$$dev.new ${WEBDIR}/mpichman-$$dev ; \
	    rm -rf ${WEBDIR}/mpichman-$$dev.old ;\
	done
	-for dev in ${DEVICES} ; do \
	    rm -f ${FTPDIR}/mpichman-$$dev.tar.gz ; \
	    tar cf - mpichman-$$dev | gzip -c > ${FTPDIR}/mpichman-$$dev.tar.gz ;\
	done
	(cd faq && make install-web)
install-mpe: ${MPEDOCS} mpeman/mpeman.htm
	-for file in ${MPEDOCS} ; do \
	    rm -f ${FTPDIR}/$$file ; \
	    cp $$file ${FTPDIR}/$$file ; \
	    if [ "`basename $${file} .ps`"".ps" = "$$file" ] ; then \
		gzip -c $${file} > ${FTPDIR}/$${file}.gz ; \
	    fi ; \
	done
	-for file in ${MPEDOCS} ; do \
	    rm -f ${WEBDIR}/$$file.new ; \
	    cp -f $$file ${WEBDIR}/$$file.new ; \
	    mv -f ${WEBDIR}/$$file.new ${WEBDIR}/$$file ; \
	done
	rm -rf ${WEBDIR}/mpeman.new ${WEBDIR}/mpeman.old
	cp -rp mpeman ${WEBDIR}mpiman.new
	mv -f ${WEBDIR}/mpeman ${WEBDIR}/mpeman.old
	mv -f ${WEBDIR}/mpeman.new ${WEBDIR}/mpeman

clean:
	-rm -f *~ *.log *.dvi *.aux *.bbl *.blg mpichman*.pdf mpichman*.ps
	-rm -f devspec*.ps devspec*.pdf *.dvips latex.err mpichman.out *.toc
	-rm -f tmp.tex tmp.out mpichdef.sty *.hux
	-rm -f mm.ps mm.pdf manmsg.txt mm.out

maintainerclean: clean
	-rm -rf ${DOCS} htmlmans config.txt
	-(cd faq && make clean)
