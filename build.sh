#!/bin/bash

set -e

echo "Usage: ./build.sh <debug|release>. Debug is default"

source setup-dirs.sh

mkdir -p $build_dir
(
    cd $build_dir && cmake -DCMAKE_BUILD_TYPE=$cfg $source_dir_from_build_dir && make -j `nproc`
)
