#!/bin/sh

# TODO: Is this a bashism?
set -e

CC=${CC:-gcc}
CFLAGS=${CFLAGS:- -g -O2}

run() {
	echo "$@"
	$@
}

(cd jimtcl;
 ./configure --disable-lineedit \
             --math \
             --disable-docs \
             --without-ext="eventloop history load syslog" \
             --with-ext="binary tclprefix" \
 ;
 make)

run $CC -o tmake $CFLAGS tmake.c jimtcl/libjim.a -lm

