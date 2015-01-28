## Process this file with automake to produce Makefile.in

icondir = $(pkgdatadir)/icons
# aven.png doesn't need to be installed on win32, but it's only 527 bytes.
dist_icon_DATA = elevation.png open.png crosses.png entrances.png plan.png\
	rotation.png defaults.png surface-legs.png ug-legs.png fixed-pts.png\
	exported-pts.png names.png aven-about.png\
       	texture.png solid-surface.png osterei.jpg\
	tubes.png find.png hideresults.png survey-tree.png pres-tree.png \
	open-pres.png\
	pres-frew.png pres-rew.png pres-go-back.png pres-pause.png\
       	pres-go.png pres-ff.png pres-fff.png pres-stop.png\
	3d.png err.png plt.png pos.png svx.png aven.png

#	tubesprefs.png stationsprefs.png indicatorprefs.png\
#	windowprefs.png gridprefs.png unitsprefs.png \
#	legsprefs.png ctlprefs.png pres-restart.png pres-create.png\
#	pres-record.png pres-finish.png

pixmapsdir = $(datadir)/pixmaps
dist_pixmaps_DATA = aven.png aven.svg svxedit.png

Aven.iconset.zip: aven.svg
	$(SHELL) $(srcdir)/svg2iconsetzip aven.svg Aven.iconset.zip

SUFFIXES = .ico .png .svg .iconset.zip

.png.iconset.zip:
	$(SHELL) $(srcdir)/png2iconsetzip $< $@

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

iconsets = Aven.iconset.zip svxedit.iconset.zip\
	3d.iconset.zip err.iconset.zip plt.iconset.zip pos.iconset.zip\
	svx.iconset.zip

MAINTAINERCLEANFILES = $(iconsets)

EXTRA_DIST = png2winicon png2iconsetzip svg2iconsetzip\
	aven.ico svxedit.ico\
	3d.ico err.ico plt.ico pos.ico svx.ico\
	$(iconsets)
