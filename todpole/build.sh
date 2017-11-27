#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-release}

mkdir -p $BUILD_DIR/$BUILD_TYPE-muduo-todpole \
  && cd $BUILD_DIR/$BUILD_TYPE-muduo-todpole \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           $SOURCE_DIR \
  && make $*

