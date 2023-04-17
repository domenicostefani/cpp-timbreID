#!/usr/bin/env bash

cd Builds/LinuxMakefile
make -j`nproc` CONFIG=Release