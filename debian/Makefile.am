## Process this file with automake to produce Makefile.in

EXTRA_DIST = copyright copyright.in survex-aven.files doc-base \
 rules survex-aven.menu watch control changelog dhelp \
 survex-svxedit.files survex-svxedit.menu

MAINTAINERCLEANFILES = control

control: $(srcdir)/Makefile.am $(srcdir)/../configure.in $(srcdir)/../desc.txt $(srcdir)/../desc-aven.txt
	echo Source: survex1.1 > control
	echo Section: science >> control
	echo Priority: extra >> control
	echo "Maintainer: Wookey <wookey@survex.com>" >> control
	echo Standards-Version: 3.6.1 >> control
	: # Need debhelper >= 1.1.17 for DH_OPTIONS to work.
	echo 'Build-Depends: debhelper (>= 1.1.17), xlibs-dev, libwxgtk2.4-dev' >> control
	echo >> control
	echo 'Package: ${PACKAGE}' >> control
	echo Architecture: any >> control
	echo 'Depends: $${shlibs:Depends}' >> control
	echo 'Description: Cave surveying and mapping software (development branch)' >> control
	sed 's/^$$/./;s/^/ /' < $(srcdir)/../desc.txt >> control
