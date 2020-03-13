/*

freq2bin - Calculate the nearest bin number of a frequency for a given window size and sample rate.
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "freq2bin.hpp"

#include "tIDLib.hpp"
#include <string>
#include <stdexcept>

unsigned long int Freq2bin::sampleRate = SAMPLERATEDEFAULT;
unsigned long int Freq2bin::windowSize = WINDOWSIZEDEFAULT;

/* ------------------------ freq2bin -------------------------------- */
float Freq2bin::calculate(float freq)
{
	if(freq>=0.0 && freq<Freq2bin::sampleRate)
		return tIDLib_freq2bin(freq, Freq2bin::windowSize, Freq2bin::sampleRate);
	else
        throw std::domain_error("freq2bin: frequency must be between 0 and " + std::to_string(Freq2bin::sampleRate));
}

void Freq2bin::setWinSampRate(long int windowSize, long int sampleRate)
{
    if(windowSize <= 0)
    {
        throw std::invalid_argument("Window size cannot be <= 0");
    }
    if(sampleRate <= 0)
    {
        throw std::invalid_argument("Sample rate cannot be <= 0");
    }
	Freq2bin::windowSize = windowSize;
	Freq2bin::sampleRate = sampleRate;
}

unsigned long int Freq2bin::getWindowSize()
{
    return Freq2bin::windowSize;
}

unsigned long int Freq2bin::getSampleRate()
{
    return Freq2bin::sampleRate;
}

