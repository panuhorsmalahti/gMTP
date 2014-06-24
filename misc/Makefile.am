desktopdir 	 = $(datadir)/applications
desktop_in_files = gmtp.desktop
desktop_DATA 	 = $(desktop_in_files:.desktop.in=.desktop)

gsettings_SCHEMAS = org.gnome.gmtp.gschema.xml

@GSETTINGS_RULES@

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_DATA = gmtp.schemas

EXTRA_DIST = 					\
	$(desktop_in_files)			\
	org.gnome.gmtp.gschema.xml.in		

CLEANFILES =	 			\
	$(desktop_DATA)			\
	$(gsettings_SCHEMAS)

DISTCLEANFILES =	 		\
	$(desktop_DATA)			\
	$(gsettings_SCHEMAS)

MAINTAINERCLEANFILES = $(gsettings_SCHEMAS:.xml=.valid)

register-gsettings-schemas: 
	$(INSTALL) -m 644 org.gnome.gmtp.gschema.xml $(DESTDIR)$(PREFIX)/share/glib-2.0/schemas
	glib-compile-schemas $(DESTDIR)$(PREFIX)/share/glib-2.0/schemas

register-gconf-schemas: 
	GCONF_CONFIG_SOURCE=`$(GCONFTOOL) --get-default-source` $(GCONFTOOL) --makefile-install-rule $(DESTDIR)$(PREFIX)/share/gconf/schemas/gmtp.schemas

if GCONF_SCHEMAS_INSTALL
install-local-data:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			    $(GCONFTOOL) --makefile-install-rule $(schemas_DATA)
endif
