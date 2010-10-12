bin_PROGRAMS=xenstore_watch
xenstore_watch_SOURCES=xenstore_watch.c xenstore_watch.1 xmalloc.c system.h aclocal.m4 getopt.c getopt1.c getopt.h

#  uncomment the following if xenstore_watch requires the math library
#xenstore_watch_LDADD=-lm

EXTRA_DIST=xenstore_watch.lsm.in xenstore_watch.spec.in xenstore_watch.texinfo

#  if you write a self-test script named `chk', uncomment the
#  following and add `chk' to the EXTRA_DIST list
#TESTS=chk

#  build and install the .info pages
info_TEXINFOS = xenstore_watch.texinfo
xenstore_watch_TEXINFOS = gpl.texinfo

#  install the man pages
man_MANS=xenstore_watch.1
