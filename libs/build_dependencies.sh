#!/usr/bin/env bash

cd aubio
./scripts/get_waf.sh
./waf  --verbose configure --disable-docs -o $PWD/build-amd64
./waf build
mkdir -p $PWD/build-amd64/src/PLEASE_DONT_USE_SHARED
mv $PWD/build-amd64/src/lib*.so* $PWD/build-amd64/src/PLEASE_DONT_USE_SHARED/




# Download and install from https://github.com/elk-audio/elkpi-sdk
# source /opt/elk/1.0/environment-setup-aarch64-elk-linux     # Release 0.9.0
source /opt/elk/0.11.0/environment-setup-cortexa72-elk-linux  # Release 0.11.0
cd aubio
./scripts/get_waf.sh
./waf   --verbose configure --disable-docs -o $PWD/build-aarch64\
        --enable-fftw3f\
        --disable-fftw3\
        --disable-complex\
        --disable-jack\
        --disable-sndfile\
        --disable-avcodec\
        --disable-vorbis\
        --disable-flac\
        --disable-samplerate\
        --disable-rubberband\
        --disable-accelerate\
        --disable-apple-audio\
        --disable-blas\
        --disable-atlas\
        --disable-wavread\
        --disable-wavwrite\
        --disable-docs\
        --disable-tests\
        --disable-examples
./waf build
mkdir -p $PWD/build-aarch64/src/PLEASE_DONT_USE_SHARED
mv $PWD/build-aarch64/src/lib*.so* $PWD/build-aarch64/src/PLEASE_DONT_USE_SHARED/