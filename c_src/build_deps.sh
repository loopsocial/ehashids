#!/usr/bin/env bash

HASHIDS_VSN="v1.2.1"

# /bin/sh on Solaris is not a POSIX compatible shell, but /usr/bin/ksh is.
if [ `uname -s` = 'SunOS' -a "${POSIX_SHELL}" != "true" ]; then
    POSIX_SHELL="true"
    export POSIX_SHELL
    exec /usr/bin/ksh $0 $@
fi
unset POSIX_SHELL # clear it so if we invoke other scripts, they run as ksh as well

set -e

if [ `basename $PWD` != "c_src" ]; then
    # originally "pushd c_src" of bash
    # but no need to use directory stack push here
    cd c_src
fi

BASEDIR="$PWD/hashids.c"

# detecting gmake and if exists use it
# if not use make
# (code from github.com/tuncer/re2/c_src/build_deps.sh
which gmake 1>/dev/null 2>/dev/null && MAKE=gmake
MAKE=${MAKE:-make}

# Changed "make" to $MAKE

case "$1" in
    place-deps)
	#mkdir -p ../priv
	#cp hashids.c/src/.libs/libhashids.so* ../priv
	;;
    rm-deps)
        rm -rf hashids.c
        ;;

    clean)
        rm -rf hashids.c
        if [ -d hashids.c ]; then
            (cd hashids.c && $MAKE clean)
        fi
        ;;

    test)
        export CFLAGS="$CFLAGS -fPIC -I $BASEDIR/include"
        export CXXFLAGS="$CXXFLAGS -fPIC -I $BASEDIR/include"
        export LDFLAGS="$LDFLAGS -L$BASEDIR/lib"
        export LD_LIBRARY_PATH="$BASEDIR/lib:$LD_LIBRARY_PATH"

        (cd hashids.c && $MAKE check)

        ;;

    get-deps)
        if [ ! -d hashids.c ]; then
            git clone https://github.com/tzvetkoff/hashids.c.git
            (cd hashids.c && git checkout -b build $HASHIDS_VSN)
        fi
        ;;

    *)
        export MACOSX_DEPLOYMENT_TARGET=10.8

        export CFLAGS="$CFLAGS -fPIC -I $BASEDIR/include"
        export CXXFLAGS="$CXXFLAGS -fPIC -I $BASEDIR/include"
        export LDFLAGS="$LDFLAGS -L$BASEDIR/lib"
        export LD_LIBRARY_PATH="$BASEDIR/lib:$LD_LIBRARY_PATH"
        export HASHIDS_VSN="$HASHIDS_VSN"

        if [ ! -d hashids.c ]; then
            git clone https://github.com/tzvetkoff/hashids.c.git
            (cd hashids.c && git checkout -b build $HASHIDS_VSN)
        fi

        if [[ "$OSTYPE" == "darwin"* ]]; then
            path_to_libtool=$(which libtool)
            if [ -z ${path_to_libtool} ] ; then
              echo "libtool is MISSING please install it with: brew install libtool"
            fi
        fi

        if [ ! -f hashids.c/config.h ]; then
          (cd hashids.c && ./bootstrap && ./configure --enable-static=yes && $MAKE -j9 all)
        else
	  (cd hashids.c && $MAKE -j9 all)
	fi
        ;;
esac

