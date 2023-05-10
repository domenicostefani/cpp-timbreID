/*

juce_timbreID
Bridge-header between the timbreID C++ implementation and Juce
Author: Domenico Stefani, 24th March 2020 (domenico.stefani96@gmail.com)

TimbreID original library was developed by William Brent

*/
#pragma once

// #include <juce_core/juce_core.h>

#include "include/tidRTLog.hpp"
#include "include/tidTime.hpp"

#include "include/bark2freq.hpp"
#include "include/bin2freq.hpp"
#include "include/freq2bark.hpp"
#include "include/freq2bin.hpp"
#include "include/freq2mel.hpp"
#include "include/mel2freq.hpp"
#include "include/tIDLib.hpp"

#include "include/attackTime.hpp"
#include "include/bark.hpp"
#include "include/barkSpec.hpp"
#include "include/barkSpecBrightness.hpp"
#include "include/bfcc.hpp"
#include "include/cepstrum.hpp"
#include "include/mfcc.hpp"
#include "include/peakSample.hpp"
#include "include/zeroCrossing.hpp"

#include "include/knn.hpp"

#include "include/aubioOnsetWrap.hpp"

#define WINDOWED_FEATURE_EXTRACTORS

#ifdef WINDOWED_FEATURE_EXTRACTORS
#include "include/windowed_feature_extraction.h"
#else
#include "include/feature_extractors.h"
#endif
