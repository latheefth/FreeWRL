## $Id$
##
## Makefile for LaTeX documentation.

all: dvi

.PHONY: all clean realclean spotless

day=`date +%d-%m-%Y`
bak_dir=./bak

DOCS = vts_local.tex

DVI = ${DOCS:.tex=.dvi}
AUX = ${DOCS:.tex=.aux}
TOC = ${DOCS:.tex=.toc}
LOG = ${DOCS:.tex=.log}
PS  = ${DOCS:.tex=.ps}
PDF = ${DOCS:.tex=.pdf}

# Target clean_dvi is necessary in case latex needs to be rerun to properly
# set cross-references, tables, toc, Bibtex and so on.
#
# Just don't remove the *.aux, *.toc files etc. if latex is being rerun!!!
dvi: clean_dvi ${DVI}

%.dvi: %.tex 
	latex $<

old_dvi: vts_local.tex
	latex vts_local.tex

display: dvi
	xdvi vts_local.dvi

ps: ${PS}

%.ps: %.dvi
	dvips -o $@ $<

pdf: ${PDF}

%.pdf: %.ps
	ps2pdf $< $@

clean_dvi:
	-rm ${DVI}

clean:
	-rm ${LOG} ${PS} ${PDF}

realclean: clean
	-rm ${DVI} ${AUX} ${TOC}

spotless: realclean
	-rm -Rf ${bak_dir}

bak_dir:
	-mkdir -p ${bak_dir}

