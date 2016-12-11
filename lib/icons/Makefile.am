## Process this file with automake to produce Makefile.in

xpm_files = \
	aven.xpm \
	log.xpm \
	open.xpm \
	open_pres.xpm \
	rotation.xpm \
	plan.xpm \
	elevation.xpm \
	defaults.xpm \
	names.xpm \
	crosses.xpm \
	entrances.xpm \
	fixed_pts.xpm \
	exported_pts.xpm \
	ug_legs.xpm \
	surface_legs.xpm \
	tubes.xpm \
	solid_surface.xpm \
	pres_frew.xpm \
	pres_rew.xpm \
	pres_go_back.xpm \
	pres_pause.xpm \
	pres_go.xpm \
	pres_ff.xpm \
	pres_fff.xpm \
	pres_stop.xpm \
	find.xpm \
	hideresults.xpm \
	survey_tree.xpm \
	pres_tree.xpm

# These ".rcdata" files are PNGs, but we end up with a loop of implicit rules
# if we call them ".png", and the .rc file doesn't care about the extension.
rcdata_files = $(xpm_files:.xpm=.rcdata)

aven-icons.rc: Makefile.am
	perl -e 'for (@ARGV) { /(.*)\.rcdata$$/ and $$1 ne "aven" and print "$$1 RCDATA $$_\n" }' \
	  $(rcdata_files) > aven-icons.rc

EXTRA_DIST = \
	3d.svg err.svg plt.svg pos.svg svx.svg \
	3d.png err.png plt.png pos.png svx.png

svgicondir = $(datadir)/icons/hicolor/scalable
x32icondir = $(datadir)/icons/hicolor/32x32

install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(svgicondir)/mimetypes"
	$(MKDIR_P) "$(DESTDIR)$(x32icondir)/mimetypes"
	for f in 3d err pos svx ; do \
	  $(INSTALL_DATA) "$(srcdir)/$$f.svg" "$(DESTDIR)$(svgicondir)/mimetypes/application-x-survex-$$f.svg" ; \
	  $(INSTALL_DATA) "$(srcdir)/$$f.png" "$(DESTDIR)$(x32icondir)/mimetypes/application-x-survex-$$f.png" ; \
	done
	$(INSTALL_DATA) "$(srcdir)/plt.svg" "$(DESTDIR)$(svgicondir)/mimetypes/application-x-compass-plot.svg"
	$(INSTALL_DATA) "$(srcdir)/plt.png" "$(DESTDIR)$(x32icondir)/mimetypes/application-x-compass-plot.png"

uninstall-local:
	for f in 3d err pos svx ; do \
	  rm -f $(DESTDIR)$(svgicondir)/mimetypes/application-x-survex-$$f.svg ; \
	  rm -f $(DESTDIR)$(x32icondir)/mimetypes/application-x-survex-$$f.png ; \
	done
	rm -f $(DESTDIR)$(svgicondir)/mimetypes/application-x-compass-plot.svg
	rm -f $(DESTDIR)$(x32icondir)/mimetypes/application-x-compass-plot.png

#	tubesprefs.png stationsprefs.png indicatorprefs.png\
#	windowprefs.png gridprefs.png unitsprefs.png \
#	legsprefs.png ctlprefs.png pres-restart.png pres-create.png\
#	pres-record.png pres-finish.png svxedit.png

appicondir = $(datadir)/icons/hicolor/scalable/apps
dist_appicon_DATA = aven.svg

Aven.iconset.zip: aven.svg
	$(SHELL) $(srcdir)/svg2iconsetzip aven.svg Aven.iconset.zip

SUFFIXES = .ico .png .svg .iconset.zip .xpm

.svg.iconset.zip:
	$(SHELL) $(srcdir)/svg2iconsetzip $< $@

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

.xpm.ico:
	$(SHELL) $(srcdir)/xpm2winicon $< $@

.xpm.rcdata:
	$(SHELL) $(srcdir)/xpm2png $< $@

iconsets = Aven.iconset.zip\
	3d.iconset.zip err.iconset.zip plt.iconset.zip pos.iconset.zip\
	svx.iconset.zip

MAINTAINERCLEANFILES = $(iconsets) $(rcdata_files)

BUILT_SOURCES = aven-icons.rc $(rcdata_files)

EXTRA_DIST += png2winicon png2iconsetzip svg2iconsetzip\
	aven-icons.rc\
	$(xpm_files)\
	$(rcdata_files)\
	aven.ico\
	3d.ico err.ico plt.ico pos.ico svx.ico\
	$(iconsets)
