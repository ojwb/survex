## Process this file with automake to produce Makefile.in

icondir = $(pkgdatadir)/icons
# aven.png doesn't need to be installed on win32, but it's only 527 bytes.
icon_DATA = elevation.png open.png crosses.png entrances.png plan.png\
	rotation.png defaults.png surface-legs.png ug-legs.png fixed-pts.png\
	exported-pts.png names.png aven-about.png\
	3d.png err.png plt.png pos.png svx.png aven.png

SUFFIXES = .ico .png

.png.ico:
	$(SHELL) $(srcdir)/png2winicon $< $@

EXTRA_DIST = $(icon_DATA) 3d.ico err.ico plt.ico pos.ico svx.ico\
 aven.ico svxedit.png svxedit.ico png2winicon
