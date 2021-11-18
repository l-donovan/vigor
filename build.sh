#!/usr/bin/env bash

BUILD_DIR=_build
DIST_DIR=dist
VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"

RC=1

err() {
    echo "ERROR: $*" >> /dev/stderr
}

if [ ! -d $VCPKG_DIR ]; then
    err "Could not find \$VCPKG_DIR. Either update \$VCPKG_DIR or install vcpkg from https://github.com/microsoft/vcpkg"
    exit $RC
elif [ ! -f $VCPKG_DIR/vcpkg ]; then
    err "Could not find \`vcpkg\` executable inside \$VCPKG_DIR. Vcpkg may be installed incorrectly"
fi

mkdir -p $BUILD_DIR $DIST_DIR &&\
cmake -B $BUILD_DIR -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake &&\
cmake --build $BUILD_DIR &&\
cp $BUILD_DIR/src/vigor $DIST_DIR/vigor &&\
RC=0

exit $RC
