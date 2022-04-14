export PREFIX=`pwd`/clib
mkdir -p cbuild && cd cbuild || exit 1
git clone --depth 1 'https://git.ffmpeg.org/ffmpeg.git' && cd ffmpeg || exit 1
./configure \
    --enable-gpl \
    --enable-shared \
    --disable-static \
    --enable-version3 \
    --prefix=$PREFIX \
    --disable-doc \
    --disable-autodetect \
    --disable-encoders \
    --disable-filters \
    "--enable-filter=volume,atempo,equalizer,aresample" \
    --disable-muxers \
    --enable-bzlib \
    --enable-gnutls \
    --enable-libcdio \
    --enable-zlib \
    || exit 1
make -j8 || exit 1
make -j8 install || exit 1
