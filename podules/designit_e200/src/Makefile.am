# Makefile.am for Design IT E200 Access+ podule

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libdesignit_e200.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../designit_e200.dll
../designit_e200.dll: libdesignit_e200.la
	cp .libs/libdesignit_e200-0.dll ../designit_e200.dll
libdesignit_e200_la_LDFLAGS = -no-undefined
endif
if OS_LINUX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../designit_e200.so
../designit_e200.so: libdesignit_e200.la
	cp .libs/libdesignit_e200.so ../designit_e200.so
endif
if OS_MACOSX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../designit_e200.dylib
../designit_e200.dylib: libdesignit_e200.la
	cp .libs/libdesignit_e200.dylib ../designit_e200.dylib
endif

amrefresh:	

libdesignit_e200_la_SOURCES = designit_e200.c ../../common/net/ne2000.c
libdesignit_e200_la_SOURCES += ../../common/net/net.c ../../common/net/net_slirp.c ../../common/net/queue.c

libdesignit_e200_la_CFLAGS = -I../../../src -I../../common/net
libdesignit_e200_la_LIBADD = @LIBS@ -lslirp

if OS_WINDOWS
libdesignit_e200_la_SOURCES += ../../common/net/net_pcap.c
libdesignit_e200_la_LIBADD += -lwsock32 -liphlpapi -lkernel32 -lws2_32
endif

endif
