#!/bin/sh

set -e

CC=${CC:-gcc}
CFLAGS=${CFLAGS:- -ansi -Wall -Wextra -g -O2}

AR=${AR:-ar}
RANLIB=${RANLIB:-ranlib}

JIM_CFLAGS="-D_GNU_SOURCE -Wall -I. -g -O2 -fno-unwind-tables -fno-asynchronous-unwind-tables"

run() {
	echo "$@"
	$@
}

rootname() {
	echo "$1" |sed 's/\(.*\)\.[^\.]*$/\1/'
}

JIM_CSRC="_load-static-exts.c jim-subcmd.c jim-interactive.c jim-format.c
         jim.c utf8.c jimregexp.c jim-aio.c jim-array.c jim-clock.c
         jim-exec.c jim-file.c jim-namespace.c jim-pack.c jim-package.c
         jim-posix.c jim-readdir.c jim-regexp.c jim-signal.c
         jim-tclprefix.c _binary.c _glob.c _nshelper.c _oo.c _stdlib.c
         _tclcompat.c _tree.c"

JIM_TCLSRC="binary.tcl glob.tcl nshelper.tcl oo.tcl stdlib.tcl
            tclcompat.tcl tree.tcl initjimsh.tcl"

JIM_STATIC_EXTS="aio array clock exec file namespace pack
                 package posix readdir regexp signal tclprefix
                 binary glob nshelper oo stdlib tclcompat tree"

# Build the bootstrap JimTcl interpreter
(cd jimtcl/autosetup;
 $CC -o ../jimsh0 jimsh0.c 2>/dev/null)

# Build the real JimTcl interpreter and library
(cd jimtcl;
 echo '#define JIM_EMBEDDED' > _load-static-exts.c;
 echo "./jimsh0 make-load-static-exts.tcl $JIM_STATIC_EXTS >> _load-static-exts.c";
 ./jimsh0 make-load-static-exts.tcl $JIM_STATIC_EXTS >> _load-static-exts.c

 for tclfile in $JIM_TCLSRC; do
   echo '#define JIM_EMBEDDED' > _$(rootname $tclfile).c;
   echo "./jimsh0 make-c-ext.tcl $tclfile >> _$(rootname $tclfile).c"
   ./jimsh0 make-c-ext.tcl $tclfile >> _$(rootname $tclfile).c;
 done;

 JIM_OBJS=""
 for cfile in $JIM_CSRC; do
   JIM_OBJS="$JIM_OBJS $(rootname $cfile).o";
   run $CC $JIM_CFLAGS -o $(rootname $cfile).o -c $cfile;
 done;

 run $AR cr libjim.a $JIM_OBJS;
 run $RANLIB libjim.a
)

# Build SQLite
run $CC -o sqlite3.o -c jimtcl/sqlite3/sqlite3.c \
    -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_THREADSAFE=0 \
    -DSQLITE_DEFAULT_FILE_FORMAT=4 -DSQLITE_ENABLE_STAT3 \
    -DSQLITE_ENABLE_LOCKING_STYLE=0 -DSQLITE_OMIT_INCRBLOB

# Build TMk
C_SRC="tmake.c tm_crypto.c tm_target.c tm_update.c tm_core_cmds.c tm_ext_cmds.c"

MAKE_C_EXT="jimtcl/jimsh jimtcl/make-c-ext.tcl tm_ext_cmds.tcl"
echo "$MAKE_C_EXT > tm_ext_cmds.c"
echo "#define JIM_EMBEDDED" > tm_ext_cmds.c
$MAKE_C_EXT >> tm_ext_cmds.c

TM_OPSYS=$(uname -s)
TM_MACHINE_ARCH=$(uname -m)

run $CC -o tmk $CFLAGS -Ijimtcl -Ijimtcl/sqlite3 \
        -DTM_OPSYS="\"$TM_OPSYS\"" -DTM_MACHINE_ARCH="\"$TM_MACHINE_ARCH\"" \
        $C_SRC jimtcl/libjim.a sqlite3.o -lm
