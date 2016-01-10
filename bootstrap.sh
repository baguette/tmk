#!/bin/sh

# TODO: Is this a bashism?
set -e

CC=${CC:-gcc}
CFLAGS=${CFLAGS:- -ansi -Wall -g -O2}

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

C_SRC="tmake.c tm_crypto.c tm_target.c tm_core_cmds.c tm_ext_cmds.c"

MAKE_C_EXT="jimtcl/jimsh jimtcl/make-c-ext.tcl tm_ext_cmds.tcl"
echo "$MAKE_C_EXT > tm_ext_cmds.c"
echo "#define JIM_EMBEDDED" > tm_ext_cmds.c
$MAKE_C_EXT >> tm_ext_cmds.c

TM_OPSYS=$(uname -s)
TM_MACHINE_ARCH=$(uname -m)

run $CC -o tmake $CFLAGS -Ijimtcl \
        -DTM_OPSYS="\"$TM_OPSYS\"" -DTM_MACHINE_ARCH="\"$TM_MACHINE_ARCH\"" \
        $C_SRC jimtcl/libjim.a -lm
