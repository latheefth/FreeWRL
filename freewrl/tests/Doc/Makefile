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

dvi: ${DVI}

%.dvi: %.tex force
	latex $<

display: dvi
	xdvi vts_local.dvi

ps: ${PS}

%.ps: %.dvi force
	dvips -o $@ $<

pdf: ${PDF}

%.pdf: %.ps force
	ps2pdf $< $@

clean:
	-rm ${LOG} ${PS} ${PDF}

realclean: clean
	-rm ${DVI} ${AUX} ${TOC}

spotless: realclean
	-rm -Rf ${bak_dir}

bak_dir:
	-mkdir -p ${bak_dir}

# empty target forces dependent targets to run their commands
force:
