## Process this file with automake to produce Makefile.in

xpm_files = \
	aven.xpm \
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

ico_files = $(xpm_files:.xpm=.ico)

aven-icons.rc: Makefile.am
	perl -e 'for (@ARGV) { /(.*)\.ico$$/ and print "$$1 ICON $$_\n" }' \
	  $(ico_files) > aven-icons.rc

icondir = $(pkgdatadir)/icons
dist_icon_DATA = \
	aven-about.png\
	texture.png osterei.jpg\
	3d.png err.png plt.png pos.png svx.png

#	tubesprefs.png stationsprefs.png indicatorprefs.png\
#	windowprefs.png gridprefs.png unitsprefs.png \
#	legsprefs.png ctlprefs.png pres-restart.png pres-create.png\
#	pres-record.png pres-finish.png solid-surface.png

pixmapsdir = $(datadir)/pixmaps
dist_pixmaps_DATA = aven.svg svxedit.png

Aven.iconset.zip: aven.svg
	$(SHELL) $(srcdir)/svg2iconsetzip aven.svg Aven.iconset.zip

3d.iconset.zip: 3d.svg
	$(SHELL) $(srcdir)/svg2iconsetzip 3d.svg 3d.iconset.zip

SUFFIXES = .ico .png .svg .iconset.zip .xpm

.png.iconset.zip:
	$(SHELL) $(srcdir)/png2iconsetzip $< $@

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

.xpm.ico:
	$(SHELL) $(srcdir)/xpm2winicon $< $@

iconsets = Aven.iconset.zip svxedit.iconset.zip\
	3d.iconset.zip err.iconset.zip plt.iconset.zip pos.iconset.zip\
	svx.iconset.zip

MAINTAINERCLEANFILES = $(iconsets)

BUILT_SOURCES = aven-icons.rc $(ico_files)

EXTRA_DIST = png2winicon png2iconsetzip svg2iconsetzip\
	aven-icons.rc\
	$(xpm_files)\
	$(ico_files)\
	aven.ico svxedit.ico\
	3d.ico err.ico plt.ico pos.ico svx.ico\
	$(iconsets)
