export PREFIX=`pwd`/clib
export INSTALL_PREFIX=`pwd`/lib
export PKG_CONFIG_PATH=$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH
mkdir -p build && cd build || exit 1
cmake \
    -G "MSYS Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=$PREFIX \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
    -DENABLE_WASAPI=OFF \
    ../ || exit 1
make -j8 || exit 1
make -j8 install || exit 1
