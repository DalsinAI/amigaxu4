#!/bin/sh
# Builds the three small libraries xu4 needs and the stove lacks, static,
# for AmigaOS 3.x on a 68040 + FPU: zlib 1.3.1, libpng 1.6.44 and libxml2
# 2.9.14 (only its tree, parser, XPath, XInclude and DTD validation).
#
#   amiga/deps/build-deps.sh STOVE OUT [CACHE]
#
# The source tarballs are fetched once into CACHE (default amiga/deps/cache,
# not kept in git) and checked against the SHA-256 sums below. OUT gets
# include/ and lib/.
# The AmiCradle Team. GPL 2 or later, like xu4.
set -e

STOVE=$1
OUT=$2
HERE=$(cd "$(dirname "$0")" && pwd)
CACHE=${3:-$HERE/cache}
[ -n "$STOVE" ] && [ -n "$OUT" ] || { echo "usage: $0 STOVE OUT [CACHE]"; exit 1; }
mkdir -p "$CACHE" "$OUT/include" "$OUT/lib" "$OUT/src"
OUT=$(cd "$OUT" && pwd)

CC="$STOVE/bin/m68k-amigaos-gcc"
AR="$STOVE/bin/m68k-amigaos-ar"
RANLIB="$STOVE/bin/m68k-amigaos-ranlib"
CFLAGS="-noixemul -m68040 -m68881 -O2 -fomit-frame-pointer -fno-tree-loop-distribute-patterns"

fetch() { # url file sha256
	if [ ! -f "$CACHE/$2" ]; then
		curl -fsSL --max-time 300 -o "$CACHE/$2.part" "$1"
		mv "$CACHE/$2.part" "$CACHE/$2"
	fi
	echo "$3  $CACHE/$2" | sha256sum -c - >/dev/null || { echo "$2: wrong SHA-256"; exit 1; }
}

fetch https://zlib.net/fossils/zlib-1.3.1.tar.gz zlib-1.3.1.tar.gz \
	9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
fetch https://download.sourceforge.net/libpng/libpng-1.6.44.tar.xz libpng-1.6.44.tar.xz \
	60c4da1d5b7f0aa8d158da48e8f8afa9773c1c8baa5d21974df61f1886b8ce8e
fetch https://download.gnome.org/sources/libxml2/2.9/libxml2-2.9.14.tar.xz libxml2-2.9.14.tar.xz \
	60d74a257d1ccec0475e749cba2f21559e48139efba6ff28224357c7c798dfee

cd "$OUT/src"

# zlib: its sources as they are.
if [ ! -f "$OUT/lib/libz.a" ]; then
	rm -rf zlib-1.3.1 && tar xzf "$CACHE/zlib-1.3.1.tar.gz"
	cd zlib-1.3.1
	for f in adler32 crc32 deflate infback inffast inflate inftrees trees zutil \
	         compress uncompr gzclose gzlib gzread gzwrite; do
		$CC $CFLAGS -DHAVE_UNISTD_H -c -o $f.o $f.c
	done
	$AR rc "$OUT/lib/libz.a" *.o && $RANLIB "$OUT/lib/libz.a"
	cp zlib.h zconf.h "$OUT/include/"
	cd ..
fi

# libpng: its prebuilt configuration; no assembler parts on a 68k.
if [ ! -f "$OUT/lib/libpng.a" ]; then
	rm -rf libpng-1.6.44 && tar xJf "$CACHE/libpng-1.6.44.tar.xz"
	cd libpng-1.6.44
	cp scripts/pnglibconf.h.prebuilt pnglibconf.h
	for f in png pngerror pngget pngmem pngpread pngread pngrio pngrtran pngrutil \
	         pngset pngtrans pngwio pngwrite pngwtran pngwutil; do
		$CC $CFLAGS -I"$OUT/include" -c -o $f.o $f.c
	done
	$AR rc "$OUT/lib/libpng.a" *.o && $RANLIB "$OUT/lib/libpng.a"
	cp png.h pngconf.h pnglibconf.h "$OUT/include/"
	cd ..
fi

# libxml2: configure for the stove, everything off that xu4 doesn't use.
if [ ! -f "$OUT/lib/libxml2.a" ]; then
	rm -rf libxml2-2.9.14 && tar xJf "$CACHE/libxml2-2.9.14.tar.xz"
	cd libxml2-2.9.14
	CC="$CC" AR="$AR" RANLIB="$RANLIB" CFLAGS="$CFLAGS" \
	./configure --host=m68k-amigaos --prefix="$OUT" --enable-static --disable-shared \
		--without-python --without-threads --without-thread-alloc --without-http \
		--without-ftp --without-iconv --without-icu --without-zlib --without-lzma \
		--without-modules --without-catalog --without-docbook --without-html \
		--without-c14n --without-schematron --without-schemas --without-regexps \
		--without-writer --without-reader --without-pattern --without-push --without-legacy \
		--without-debug --without-mem-debug \
		--without-run-debug --without-readline --without-history \
		--with-tree --with-xpath --with-xptr --with-xinclude --with-valid \
		--with-output >configure.log 2>&1 || { tail -30 configure.log; exit 1; }
	make -j4 libxml2.la >make.log 2>&1 || { tail -40 make.log; exit 1; }
	cp .libs/libxml2.a "$OUT/lib/"
	mkdir -p "$OUT/include/libxml"
	cp include/libxml/*.h "$OUT/include/libxml/"
	cd ..
fi
echo "deps in $OUT"
