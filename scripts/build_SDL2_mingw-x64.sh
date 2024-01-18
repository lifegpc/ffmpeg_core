export PREFIX=`pwd`/clib
mkdir -p cbuild && cd cbuild || exit 1
curl -L "https://github.com/libsdl-org/SDL/releases/download/release-2.28.5/SDL2-2.28.5.tar.gz" -o SDL2.tar.gz || exit 1
tar -xzvf SDL2.tar.gz || exit 1
cd SDL2-* && mkdir -p build && cd build || exit 1
cmake \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_INSTALL_PREFIX=$PREFIX \
    ../ || exit 1
make -j8 || exit 1
make -j8 install || exit 1
