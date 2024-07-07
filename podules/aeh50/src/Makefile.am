# Makefile.am for Acorn AEH50 Ethernet II podule

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libaeh50.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh50.dll
../aeh50.dll: libaeh50.la
	cp .libs/libaeh50-0.dll ../aeh50.dll
libaeh50_la_LDFLAGS = -no-undefined
endif
if OS_LINUX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh50.so
../aeh50.so: libaeh50.la
	cp .libs/libaeh50.so ../aeh50.so
endif
if OS_MACOSX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh50.dylib
../aeh50.dylib: libaeh50.la
	cp .libs/libaeh50.dylib ../aeh50.dylib
endif

amrefresh:	

libaeh50_la_SOURCES = aeh50.c ../../common/net/ne2000.c
libaeh50_la_SOURCES += ../../common/net/net.c ../../common/net/net_slirp.c ../../common/net/queue.c

libaeh50_la_CFLAGS = -I../../../src -I../../common/net
libaeh50_la_LIBADD = @LIBS@ -lslirp

if OS_WINDOWS
libaeh50_la_SOURCES += ../../common/net/net_pcap.c
libaeh50_la_LIBADD += -lwsock32 -liphlpapi -lkernel32 -lws2_32
endif

endif
