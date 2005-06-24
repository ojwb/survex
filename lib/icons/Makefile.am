## Process this file with automake to produce Makefile.in

icondir = $(pkgdatadir)/icons
# aven.png doesn't need to be installed on win32, but it's only 527 bytes.
icon_DATA = elevation.png open.png crosses.png entrances.png plan.png\
	rotation.png defaults.png surface-legs.png ug-legs.png fixed-pts.png\
	exported-pts.png names.png aven-about.png texture.png\
	solid-surface.png\
	tubes.png find.png hideresults.png survey-tree.png pres-tree.png \
	3d.png err.png plt.png pos.png svx.png aven.png\
	open-pres.png

#	tubesprefs.png stationsprefs.png indicatorprefs.png\
#	windowprefs.png gridprefs.png unitsprefs.png \
#	legsprefs.png ctlprefs.png

#	pres-go-back.png pres-restart.png pres-go.png pres-create.png
#	pres-record.png pres-finish.png

SUFFIXES = .ico .png

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

EXTRA_DIST = $(icon_DATA) 3d.ico err.ico plt.ico pos.ico svx.ico\
 	aven.ico svxedit.png svxedit.ico png2winicon
