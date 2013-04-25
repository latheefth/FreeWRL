man_MANS = \
	freewrl.1

if FW_DOCS
all-local: html/index.html
endif

headers = $(top_srcdir)/src/lib/libFreeWRL.h

latex/refman.pdf: doxyfile $(headers)
	doxygen
	$(MAKE) -C latex refman.pdf

html/index.html: doxyfile $(headers)
	doxygen

clean-local:
	-rm -rf html latex doxytags
