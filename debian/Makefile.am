## Process this file with automake to produce Makefile.in

EXTRA_DIST = changelog compat control copyright copyright.in dhelp doc-base\
 rules watch survex.install survex.menu

MAINTAINERCLEANFILES = control

control: $(srcdir)/Makefile.am $(srcdir)/../configure.in $(srcdir)/../desc.txt $(srcdir)/../desc-aven.txt
	echo Source: survex > control
	echo Section: science >> control
	echo Priority: extra >> control
	echo "Maintainer: Wookey <wookey@survex.com>" >> control
	echo Standards-Version: 3.6.2 >> control
	: # Need debhelper >= 4 for dh compat level 4; >= 4.2.13 for misc:Depends
	: # Don't move to compat level 5 yet - debian stable only has 4.2.32 currently.
	echo 'Build-Depends: debhelper (>= 4.2.13), libx11-dev, libxt-dev, libwxgtk2.4-dev' >> control
	echo >> control
	echo 'Package: ${PACKAGE}' >> control
	echo Architecture: any >> control
	echo 'Depends: $${misc:Depends}, $${shlibs:Depends}' >> control
	echo 'Description: Cave surveying and mapping software (development branch)' >> control
	sed 's/^$$/./;s/^/ /' < $(srcdir)/../desc.txt >> control
	echo ' .' >> control
	echo ' Homepage: http://www.survex.com/' >> control
