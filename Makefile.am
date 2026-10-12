## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = librawstudio plugins src po pixmaps profiles

desktopdir = $(datadir)/applications
desktop_DATA = rawstudio-legacy.desktop

appdatadir = $(datarootdir)/appdata
dist_appdata_DATA = rawstudio-legacy.appdata.xml

EXTRA_DIST = \
	autogen.sh \
	ChangeLog \
	rawstudio-legacy.desktop \
	rawstudio-legacy.appdata.xml \
	gettext.h

ChangeLog:
	git log >$@

install-data-hook:
	cd $(DESTDIR)$(desktopdir) && rm -f rawstudio.desktop && $(LN_S) rawstudio-legacy.desktop rawstudio.desktop
	cd $(DESTDIR)$(datadir) && if [ -d rawstudio-legacy ] && [ ! -e rawstudio ]; then $(LN_S) rawstudio-legacy rawstudio; fi
