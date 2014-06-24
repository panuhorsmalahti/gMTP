AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = gmtp
gmtp_SOURCES = \
about.c about.h \
albumart.c albumart.h \
callbacks.c callbacks.h \
dnd.c dnd.h \
formatdevice.c formatdevice.h \
interface.c interface.h \
main.c main.h \
metatag_info.c metatag_info.h \
mtp.c mtp.h \
playlist.c playlist.h \
prefs.c prefs.h \
progress.c progress.h \
properties.c properties.h \
preferences.c preferences.h 

gmtp_CFLAGS = ${GTK2_CFLAGS} ${GTK3_CFLAGS} ${LIBMTP_CFLAGS} ${FLAC_CLFAGS} ${GTHREAD_CFLAGS} ${GIO_CLFAGS} ${GCONF_CFLAGS} ${ID3TAG_CFLAGS} ${VORBISFILE_CFLAGS}
gmtp_LDADD = ${GTK2_LIBS} ${GTK3_LIBS} ${LIBMTP_LIBS} ${FLAC_LIBS} ${GTHREAD_LIBS} ${GIO_LIBS} ${GCONF_LIBS} ${ID3TAG_LIBS} ${VORBISFILE_LIBS}


