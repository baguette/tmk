#!/bin/sh

# TODO: Is this a bashism?
set -e

CC=${CC:-clang}
CFLAGS=${CFLAGS:- -ansi -Wall -Wextra -g -O2}

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
 make;

 cd sqlite3;
 gcc -o sqlite3.o -c sqlite3.c \
     -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 \
     -DSQLITE_DEFAULT_FILE_FORMAT=4 -DSQLITE_ENABLE_STAT3 \
     -DSQLITE_ENABLE_LOCKING_STYLE=0 -DSQLITE_OMIT_INCRBLOB)

C_SRC="tmake.c tm_crypto.c tm_target.c tm_core_cmds.c tm_ext_cmds.c"

MAKE_C_EXT="jimtcl/jimsh jimtcl/make-c-ext.tcl tm_ext_cmds.tcl"
echo "$MAKE_C_EXT > tm_ext_cmds.c"
echo "#define JIM_EMBEDDED" > tm_ext_cmds.c
$MAKE_C_EXT >> tm_ext_cmds.c

TM_OPSYS=$(uname -s)
TM_MACHINE_ARCH=$(uname -m)

run $CC -o tmake $CFLAGS -Ijimtcl -Ijimtcl/sqlite3 \
        -DTM_OPSYS="\"$TM_OPSYS\"" -DTM_MACHINE_ARCH="\"$TM_MACHINE_ARCH\"" \
        $C_SRC jimtcl/libjim.a jimtcl/sqlite3/sqlite3.o -lm
