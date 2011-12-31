## Process this file with automake to produce Makefile.in

# Makefile for survex support files

check-local: survex.pot
	srcdir=$(srcdir) LANG=C perl $(srcdir)/checkshortcut.pl $(msg_files)
	if grep ^msgstr $(srcdir)/codes.po|sort|uniq -c|grep -v '^ *1 '; then\
	    echo 'Duplicate message code(s) in codes.po'; exit 1;\
	fi
	for po in $(po_files) ; do\
	    msgfmt -c --check-accelerators -o /dev/null "$(srcdir)/$$po" || exit 1;\
	done
	if cd "$(srcdir)" && grep 'msgstr "[0-9][0-9]*"' $(po_files); then\
	    echo '.po file with numeric code'; exit 1;\
	fi

SUBDIRS = icons

po_files = ca.po de.po de_CH.po de_DE.po en_US.po\
 es.po fr.po it.po pt.po pt_BR.po sk.po ro.po

msg_files = $(po_files:.po=.msg) en.msg

EXTRA_DIST = survex.pot $(po_files) codes.po\
 named-entities.txt po-to-msg.pl checkshortcut.pl extract-msgs.pl\
 English_GB.isl Romanian-7-5.1.11.isl\
 make-pixel-font\
 INSTALL.OSX

MAINTAINERCLEANFILES = $(msg_files) *.todo unifont.pixelfont

dist_pkgdata_DATA = print.ini bcra3.svx bcra5.svx $(msg_files) unifont.pixelfont

appsdir = $(datadir)/applications
dist_apps_DATA = survex-aven.desktop survex-svxedit.desktop

$(msg_files): survex.pot $(po_files) po-to-msg.pl
	LANG=C perl $(srcdir)/po-to-msg.pl $(po_files)

survex.pot: codes.po
	perl -pe 's/^msgstr ".*/msgstr ""/' codes.po > survex.pot~
	mv survex.pot~ survex.pot

mimedir = $(datadir)/mime-info
dist_mime_DATA = survex.mime
mime_DATA = survex.keys

unifont.pixelfont: $(srcdir)/make-pixel-font
	$(srcdir)/make-pixel-font
