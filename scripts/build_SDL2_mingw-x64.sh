export PREFIX=`pwd`/clib
mkdir -p cbuild && cd cbuild || exit 1
git clone --depth 1 https://github.com/libsdl-org/SDL || exit 1
cd SDL && mkdir -p build && cd build || exit 1
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
