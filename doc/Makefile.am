## Process this file with automake to produce Makefile.in

pkgdocdir = $(prefix)/doc/@PACKAGE@

# FIXME: need to sort out an "all in one" HTML version
doclist = svxdocs svxdocs.rtf svxdocs.txt svxdocs.ps # svxdocs.html

EXTRA_DIST = $(doclist) svxdocs.sgml custom.dsl

MAINTAINERCLEANFILES = $(doclist)

pkgdoc_DATA = $(doclist)

# Allow people to build from CVS without sgmltools v2 installed
# (but they won't be able to "make dist", which is a good thing)
warning="Can't make documentation without sgmltools v2 installed"

svxdocs: svxdocs.sgml custom.dsl
if SGMLTOOLS
	sgmltools --backend=html --dsssl-spec=custom.dsl#html svxdocs.sgml
else
	echo $(warning) 1>&2
endif

svxdocs.html: svxdocs.sgml custom.dsl
if SGMLTOOLS
	sgmltools --backend=html --dsssl-spec=custom.dsl#onepage svxdocs.sgml
else
	echo $(warning) 1>&2
endif

svxdocs.rtf: svxdocs.sgml
if SGMLTOOLS
	sgmltools --backend=rtf svxdocs.sgml
else
	echo $(warning) 1>&2
endif

svxdocs.txt: svxdocs.sgml
if SGMLTOOLS
	sgmltools --backend=txt svxdocs.sgml
else
	echo $(warning) 1>&2
endif

svxdocs.ps: svxdocs.sgml
if SGMLTOOLS
	sgmltools --backend=ps svxdocs.sgml
else
	echo $(warning) 1>&2
endif
