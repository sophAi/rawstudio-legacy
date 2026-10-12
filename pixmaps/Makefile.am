icondir = $(datadir)/icons
icon_DATA = rawstudio.png

rawstudiodir = $(datadir)/pixmaps/rawstudio
rawstudio_DATA = 	overlay_deleted.png \
		overlay_priority1.png \
		overlay_priority2.png \
		overlay_priority3.png \
		overlay_exported.png \
		transform_flip.png \
		transform_mirror.png \
		transform_90.png \
		transform_180.png \
		transform_270.png \
		camera-photo.png \
		cursor-color-picker.png \
		cursor-crop.png \
		cursor-rotate.png \
		tool-color-picker.png \
		tool-crop.png \
		tool-rotate.png


EXTRA_DIST = $(icon_DATA) $(rawstudio_DATA) artwork.license

install-data-hook:
	cd $(DESTDIR)$(icondir) && rm -f rawstudio-legacy.png && $(LN_S) rawstudio.png rawstudio-legacy.png
	cd $(DESTDIR)$(datadir)/pixmaps && rm -rf rawstudio-legacy && $(LN_S) rawstudio rawstudio-legacy
