#  Output Directory

MAN_DIR  = ./man/man3
HTML_DIR = ./html

#  Input Directory

SRC_DIR  = ../src

.SUFFIXES:	.c .txt

#  Name of the sources
API_SRC = clog.c clog_merge.c clog_util.c mpe_log.c \
          mpe_graphics.c xmouse.c \
          decomp.c examine.c getgrank.c mpe_counter.c mpe_io.c \
          mpe_seq.c privtags.c
INC_TXT = 

#  The following statements require GNU make.
REAL_SRC = $(API_SRC:%.c=$(SRC_DIR)/%.c)
REAL_TXT = $(INC_TXT:%.txt=$(SRC_DIR)/%.txt)

#  The utilities property
DOCTEXT = doctext
DOC_OPT = -heading MPE

all:	man html

man:	FORCE 
	if [ ! -d $(MAN_DIR) ] ; then \
		mkdir -p $(MAN_DIR) ; fi
	$(DOCTEXT) -mpath $(MAN_DIR) $(DOC_OPT) $(REAL_TXT) $(REAL_SRC)
FORCE:

html:	FORCE
	if [ ! -d $(HTML_DIR) ] ; then \
		mkdir -p $(HTML_DIR) ; fi
	$(DOCTEXT) -html -mpath $(HTML_DIR) $(DOC_OPT) \
	           -index $(HTML_DIR)/libmpe.cit -indexdir $(HTML_DIR) \
	           $(REAL_TXT) $(REAL_SRC)
FORCE:

clean:
	rm -f $(MAN_DIR)/*.3 $(HTML_DIR)/*.html $(HTML_DIR)/*.cit
