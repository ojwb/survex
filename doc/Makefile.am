## Process this file with automake to produce Makefile.in

# Makefile for survex documentation

# Can't produce PS at present as my machine doesn't have
# a suitable build of TeX

pkgdocdir = $(prefix)/doc/@PACKAGE@

EXTRA_DIST = svxdocs/*.html svxdocs.sgml svxdocs svxdocs.rtf svxdocs.txt\
 custom.dsl

pkgdoc_DATA = svxdocs svxdocs.rtf svxdocs.txt # svxdocs.html # svxdocs.ps

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
