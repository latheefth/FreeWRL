## $Id$
##
## Makefile for LaTeX documentation.

all: dvi

.PHONY: all clean realclean spotless

DOCS = scene-backend-management.tex

AUX = ${DOCS:.tex=.aux}
LOG = ${DOCS:.tex=.log}
DVI = ${DOCS:.tex=.dvi}
PS  = ${DOCS:.tex=.ps}
PDF = ${DOCS:.tex=.pdf}

BIB = freewrl-cites.bib

dvi: ${DVI}

%.dvi: %.tex force
	latex $<

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

# empty target forces dependent targets to run their commands
force:
