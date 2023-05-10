#!/usr/bin/env bash

CURDIR-$(pwd)
cd ./libs/deep-classf-runtime-wrappers/TFLiteWrapper/2.5.3/
./compile_elk_aarch64_RPI4.sh
cd $CURDIR