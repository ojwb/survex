## Process this file with automake to produce Makefile.in

icondir = $(pkgdatadir)/icons
icon_DATA = elevation.png open.png crosses.png entrances.png plan.png\
	rotation.png defaults.png surface-legs.png ug-legs.png fixed-pts.png\
	exported-pts.png names.png aven-about.png\
	3d.png err.png plt.png pos.png svx.png

SUFFIXES = .ico .png

.png.ico:
	$(SHELL) $(top_srcdir)/src/png2winicon $< $@

EXTRA_DIST = $(icon_DATA) 3d.ico err.ico plt.ico pos.ico svx.ico
