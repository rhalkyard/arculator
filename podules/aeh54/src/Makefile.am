# Makefile.am for Acorn AEH54 Ethernet III podule

AUTOMAKE_OPTIONS = subdir-objects

if BUILD_PODULES

lib_LTLIBRARIES = libaeh54.la
noinst_SCRIPTS =
CLEANFILES =

if OS_WINDOWS
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh54.dll
../aeh54.dll: libaeh54.la
	cp .libs/libaeh54-0.dll ../aeh54.dll
libaeh54_la_LDFLAGS = -no-undefined
endif
if OS_LINUX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh54.so
../aeh54.so: libaeh54.la
	cp .libs/libaeh54.so ../aeh54.so
endif
if OS_MACOSX
CLEANFILES += $(noinst_SCRIPTS)
noinst_SCRIPTS += ../aeh54.dylib
../aeh54.dylib: libaeh54.la
	cp .libs/libaeh54.dylib ../aeh54.dylib
endif

amrefresh:	

libaeh54_la_SOURCES = aeh54.c seeq8005.c
libaeh54_la_SOURCES += ../../common/net/net.c ../../common/net/net_slirp.c ../../common/net/queue.c

libaeh54_la_CFLAGS = -I../../../src -I../../common/net
libaeh54_la_LIBADD = @LIBS@ -lslirp

if OS_WINDOWS
libaeh54_la_SOURCES += ../../common/net/net_pcap.c
libaeh54_la_LIBADD += -lwsock32 -liphlpapi -lkernel32 -lws2_32
endif

endif
