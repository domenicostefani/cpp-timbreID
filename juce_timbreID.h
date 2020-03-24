/*

juce_timbreID
Bridge-header between the timbreID C++ implementation and Juce
Author: Domenico Stefani, 24th March 2020 (domenico.stefani96@gmail.com)

TimbreID original library was developed by William Brent

*/
#pragma once

#include <juce_core/juce_core.h>

#include "include/bark2freq.hpp"
#include "include/freq2bark.hpp"
#include "include/freq2mel.hpp"
#include "include/tIDLib.hpp"
#include "include/bin2freq.hpp"
#include "include/freq2bin.hpp"
#include "include/mel2freq.hpp"

#include "include/zeroCrossing.hpp"
