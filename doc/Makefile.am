## Process this file with automake to produce Makefile.in

# Makefile for survex documentation

pkgdocdir = $(prefix)/doc/@PACKAGE@

# FIXME: problems with svxdocs directory - may be an automake bug
#EXTRA_DIST = svxdocs/*.html svxdocs.sgml svxdocs.rtf svxdocs.txt custom.dsl
EXTRA_DIST = svxdocs svxdocs.sgml svxdocs.rtf svxdocs.txt custom.dsl

# Can't produce PS at present as my machine doesn't have
# a suitable build of TeX
# And I need to sort out an "all in one" HTML version

# svxdocs.html svxdocs.ps

MAINTAINERCLEANFILES = svxdocs svxdocs.rtf svxdocs.txt

pkgdoc_DATA = svxdocs svxdocs.rtf svxdocs.txt

svxdocs: svxdocs.sgml custom.dsl
	sgmltools --backend=html --dsssl-spec=custom.dsl#html svxdocs.sgml

#svxdocs.html: svxdocs.sgml custom.dsl
#	sgmltools --backend=html --dsssl-spec=custom.dsl#onepage svxdocs.sgml

svxdocs.rtf: svxdocs.sgml
	sgmltools --backend=rtf svxdocs.sgml

svxdocs.txt: svxdocs.sgml
	sgmltools --backend=txt svxdocs.sgml

#svxdocs.ps: svxdocs.sgml
#	sgmltools --backend=ps svxdocs.sgml
