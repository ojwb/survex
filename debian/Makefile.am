## Process this file with automake to produce Makefile.in

EXTRA_DIST = copyright copyright.in survex-aven.files doc-base \
 rules survex-aven.menu watch control changelog dhelp \
 survex-svxedit.files survex-svxedit.menu

MAINTAINERCLEANFILES = control

control: $(srcdir)/Makefile.am $(srcdir)/../configure.in $(srcdir)/../desc.txt $(srcdir)/../desc-aven.txt $(srcdir)/../desc-svxedit.txt
	echo Source: survex > control
	echo Section: science >> control
	echo Priority: extra >> control
	echo "Maintainer: Wookey <wookey@survex.com>" >> control
	echo Standards-Version: 3.6.2 >> control
	: # Need debhelper >= 1.1.17 for DH_OPTIONS to work.
	echo 'Build-Depends: debhelper (>= 1.1.17), libx11-dev, libxext-dev, libxt-dev, x-dev, xlibs-dev, libwxgtk2.4-dev' >> control
	echo >> control
	echo 'Package: ${PACKAGE}' >> control
	echo Architecture: any >> control
	echo 'Depends: $${shlibs:Depends}' >> control
	echo 'Recommends: survex-aven (= ${VERSION})' >> control
	echo 'Suggests: survex-svxedit (= ${VERSION})' >> control
	echo 'Description: Cave surveying and mapping software' >> control
	sed 's/^$$/./;s/^/ /' < $(srcdir)/../desc.txt >> control
	echo ' .' >> control
	echo ' Homepage: http://www.survex.com/' >> control
	echo >> control
	echo Package: survex-aven >> control
	echo Architecture: any >> control
	echo 'Depends: survex (= ${VERSION}), $${shlibs:Depends}' >> control
	echo Description: sophisticated cave survey viewer for Survex >> control
	sed 's/^$$/./;s/^/ /' < $(srcdir)/../desc-aven.txt >> control
	echo >> control
	echo Package: survex-svxedit >> control
	echo Architecture: all >> control
	echo 'Depends: tk8.4|wish, bwidget' >> control
	echo 'Recommends: survex (= ${VERSION})' >> control
	echo Description: survey data editor for Survex >> control
	sed 's/^$$/./;s/^/ /' < $(srcdir)/../desc-svxedit.txt >> control
