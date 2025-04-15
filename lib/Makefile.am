## Process this file with automake to produce Makefile.in

# Makefile for survex support files

check-local: survex.pot
	srcdir=$(srcdir) LANG=C perl $(srcdir)/checkshortcut.pl $(msg_files)
	if grep '^#: n:' $(srcdir)/survex.pot|sort|uniq -c|grep -v '^ *1 '; then\
	    echo 'Duplicate message code(s) in survex.pot'; exit 1;\
	fi
	for po in $(po_files) ; do\
	    $(MSGFMT) -c --check-accelerators -o /dev/null "$(srcdir)/$$po" || exit 1;\
	done
	if cd "$(srcdir)" && grep 'msgstr "[0-9][0-9]*"' $(po_files); then\
	    echo '.po file with numeric code'; exit 1;\
	fi

SUBDIRS = icons images

po_files = bg.po ca.po cs.po de.po de_CH.po el.po en_US.po es.po fr.po\
 hu.po id.po it.po pl.po pt.po pt_BR.po ro.po ru.po sk.po zh_CN.po

msg_files = $(po_files:.po=.msg) en.msg

EXTRA_DIST = survex.pot $(po_files)\
 po-to-msg.pl checkshortcut.pl extract-msgs.pl\
 ChineseSimplified.isl\
 ChineseTraditional.isl\
 EnglishBritish.isl\
 Greek.isl\
 Indonesian.isl\
 Romanian.isl\
 make-pixel-font preload_font.h

MAINTAINERCLEANFILES = $(msg_files) unifont.pixelfont preload_font.h

dist_pkgdata_DATA = bcra3.svx bcra5.svx $(msg_files) unifont.pixelfont

appsdir = $(datadir)/applications
dist_apps_DATA = survex-aven.desktop

gtksourceview3dir = $(datadir)/gtksourceview-3.0/language-specs
dist_gtksourceview3_DATA = survex.lang

gtksourceview4dir = $(datadir)/gtksourceview-4/language-specs
dist_gtksourceview4_DATA = survex.lang

$(msg_files): survex.pot $(po_files) po-to-msg.pl
	LANG=C perl $(srcdir)/po-to-msg.pl $(po_files)

packagesdir = $(datadir)/mime/packages
dist_packages_DATA = survex.xml

unifont.pixelfont preload_font.h: $(srcdir)/make-pixel-font
	$(srcdir)/make-pixel-font

merge-pos:
	cd '$(srcdir)' && ./merge-po $(po_files)

generate-todo:
	cd '$(srcdir)' && ./generate-po-todo $(po_files)

launchpad-upload.tar.bz2: survex.pot $(po_files)
	cd '$(srcdir)' && tar --transform='s!^!survex/!' -jcf $@ $^
	@echo 'Upload $@ to'
	@echo 'https://translations.launchpad.net/survex/trunk/+translations-upload'

launchpad-diff:
	rm -rf lib
	mkdir lib
	cd lib && tar --strip-components=1 -xf ../launchpad-export.tar.gz
	for po in $(po_files) ; do \
	    lpo="lib/survex-$$po" ;\
	    [ -r "$$lpo" ] || continue ;\
	    perl ./unlaunchpad-po "$$lpo" | ./launchpad-merge-po "$$po" - > "lib/$$po" ;\
	    if ! diff -q "$$po" "lib/$$po" ; then \
		echo "Edit changes for $$po ? (Y/n/q)" ;\
		read ;\
		case $$REPLY in \
		q*) exit 0 ;;\
		n*) ;;\
		*) vimdiff "$$po" "lib/$$po" ;;\
		esac ;\
	    fi ;\
	done

.PHONY: merge-pos
