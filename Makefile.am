AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
EXTRA_DIST = autogen.sh web po images misc pkginfo prototype postinstall
SUBDIRS = src po images misc

sysv-pkg: all 
	pkgmk -o -d /tmp -a @host_cpu@
	touch @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@.pkg
	pkgtrans -s /tmp @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@.pkg @PACKAGE@
	rm -r /tmp/@PACKAGE@
	gzip @PACKAGE@-@PACKAGE_VERSION@-@host_cpu@.pkg

