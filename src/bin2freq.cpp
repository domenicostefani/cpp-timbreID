/*

bin2freq - Calculate the frequency of a bin number for a given window size and sample rate.
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "bin2freq.hpp"

#include "tIDLib.hpp"
#include <string>
#include <stdexcept>

#include <assert.h>

namespace tid /* TimbreID namespace*/
{
double Bin2freq::sampleRate = (double)tIDLib::SAMPLERATEDEFAULT;
unsigned long int Bin2freq::windowSize = tIDLib::WINDOWSIZEDEFAULT;

/* ------------------------ bin2freq -------------------------------- */
float Bin2freq::calculate(float bin)
{
    if (bin >= 0.0f && bin < (float)Bin2freq::windowSize)
        return tIDLib::bin2freq(bin, (float)Bin2freq::windowSize, Bin2freq::sampleRate);
    else
        throw std::domain_error("bin2freq: bin number must be between 0 and " + std::to_string(Bin2freq::windowSize - 1));
}

void Bin2freq::setWinSampRate(long int windowSize, double sampleRate)
{
    if (windowSize <= 0)
        throw std::invalid_argument("Window size cannot be <= 0");
    if (sampleRate <= 0)
        throw std::invalid_argument("Sample rate cannot be <= 0");
    Bin2freq::windowSize = windowSize;
    Bin2freq::sampleRate = sampleRate;
}

unsigned long int Bin2freq::getWindowSize()
{
    return Bin2freq::windowSize;
}

unsigned long int Bin2freq::getSampleRate()
{
    return Bin2freq::sampleRate;
}
} // namespace tid
