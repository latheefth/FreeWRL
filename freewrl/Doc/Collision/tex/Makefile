objects = algo.pdf crc_stage.pdf

all : algo.pdf crc_stage.pdf

clean :
	rm -f algo.aux algo.log algo.pdf algo.tex~ algo.toc crc_stage.aux crc_stage.log crc_stage.pdf crc_stage.tex~ crc_stage.toc

$(objects): %.pdf: %.tex
	pdflatex $<
#	xpdf $@
