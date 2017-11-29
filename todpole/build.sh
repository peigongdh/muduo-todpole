#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install-muduo-todpole}

mkdir -p $BUILD_DIR/$BUILD_TYPE-muduo-todpole \
  && cd $BUILD_DIR/$BUILD_TYPE-muduo-todpole \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           $SOURCE_DIR \
  && make $*

