## Process this file with automake to produce Makefile.in

EXTRA_DIST = copyright copyright.in survex-aven.files doc-base \
 rules survex-aven.menu watch control changelog dhelp \
 survex-svxedit.files survex-svxedit.menu

MAINTAINERCLEANFILES = control

control: $(srcdir)/Makefile.am $(srcdir)/../configure.in $(srcdir)/../desc.txt $(srcdir)/../desc-aven.txt
	echo Source: survex > control
	echo Section: math >> control
	echo Priority: optional >> control
	echo "Maintainer: Wookey <wookey@survex.com>" >> control
	echo Standards-Version: 3.5.6.0 >> control
	: # Need debhelper >= 1.1.17 for DH_OPTIONS to work.
	echo 'Build-Depends: debhelper (>= 1.1.17), libwxgtk2.4-dev' >> control
	echo >> control
	echo Package: ${PACKAGE} >> control
	echo Architecture: any >> control
	echo 'Depends: $${shlibs:Depends}' >> control
	echo 'Recommends: survex-aven (= ${VERSION})' >> control
	echo 'Suggests: survex-svxedit (= ${VERSION})' >> control
	echo Description: Cave surveying and mapping software >> control
	sed 's/^/ /' < $(srcdir)/../desc.txt >> control
	echo >> control
	echo Package: survex-aven >> control
	echo Architecture: any >> control
	echo 'Depends: survex (= ${VERSION}), $${shlibs:Depends}' >> control
	echo Description: Sophisticated cave survey viewer for Survex >> control
	sed 's/^/ /' < $(srcdir)/../desc-aven.txt >> control
	echo >> control
	echo Package: survex-svxedit >> control
	echo Architecture: all >> control
	echo 'Depends: tk8.4|wish, bwidget' >> control
	echo 'Recommends: survex (= ${VERSION})' >> control
	echo Description: Survey data editor for Survex >> control
	sed 's/^/ /' < $(srcdir)/../desc-svxedit.txt >> control
