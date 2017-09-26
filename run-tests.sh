#!/bin/bash

source setup-dirs.sh
(
    cd $build_dir && make run-unit-tests
)
