## Process this file with automake to produce Makefile.in

EXTRA_DIST = copyright copyright.in docs survex-aven.files doc-base \
 rules survex-aven.menu watch control changelog

CLEANFILES = control docs

control: Makefile $(top_srcdir)/desc.txt $(top_srcdir)/desc-aven.txt
	echo Source: survex > control
	echo Section: math >> control
	echo Priority: optional >> control
	echo "Maintainer: Wookey <wookey@survex.com>" >> control
	echo Standards-Version: 3.1.0 >> control
	: # Need debhelper >= 1.1.17 for DH_OPTIONS to work.
	echo 'Build-Depends: debhelper (>= 1.1.17), libwxgtk2.2-dev' >> control
	echo >> control
	echo Package: ${PACKAGE} >> control
	echo Architecture: any >> control
	echo 'Depends: $${shlibs:Depends}' >> control
	echo 'Recommends: survex-aven (= ${VERSION}.@RELEASE@)' >> control
	echo Description: Cave surveying and mapping software >> control
	sed 's/^/ /' < $(top_srcdir)/desc.txt >> control
	echo >> control
	echo Package: survex-aven >> control
	echo Architecture: any >> control
	echo 'Depends: survex (= ${VERSION}.@RELEASE@), $${shlibs:Depends}' >> control
	echo Description: Sophisticated cave survey viewer for Survex >> control
	sed 's/^/ /' < $(top_srcdir)/desc-aven.txt >> control

docs: Makefile $(top_srcdir)/configure.in
	rm -f docs
	for a in `echo @EXTRA_TEXT@|grep -v COPYING|grep -v ChangeLog` ; do \
	 echo $a >> docs ; done
	for a in @HTMLFILES@ @DOCS_OTHER@ manual/ ; do \
	 echo debian/tmp/usr/doc/survex/$a >> docs ; done
