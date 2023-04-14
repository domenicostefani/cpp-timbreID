#pragma once

#include <vector>
#include <cassert>
#include <algorithm>
#include <limits>  // std::numeric_limits<unsigned long int>::max()


#include "tIDLib.h"

#include "tidRTLog.h"
#include "attackTime.h"
// #include "aubioOnsetWrap.h"
#include "conversions.h"
// #include "bark.h"    // TODO: this still depends on juce's timer
#include "barkSpecBrightness.h"
#include "barkSpec.h"
#include "bfcc.h"
#include "bin2freq.h"
#include "cepstrum.h"
// #include "choc_FIFOReadWritePosition.h"
// #include "choc_SingleReaderSingleWriterFIFO.h"
#include "freq2bin.h"
// #include "knn.h"
#include "mfcc.h"
#include "peakSample.h"
// #include "tidTime.h"    // TODO: this still depends on juce's time
#include "zeroCrossing.h"