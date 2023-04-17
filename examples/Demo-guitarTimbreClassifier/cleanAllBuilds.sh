#!/usr/bin/env bash

set -e

rm -r bin/*

for build_folder in ./Builds/LinuxMakefile-*; do
    cd $build_folder
    make clean
    cd  ../..
done;