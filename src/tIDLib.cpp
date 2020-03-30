/*

TimbreID Library - main file
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "tIDLib.hpp"

#include <JuceHeader.h>
#include <cmath>
#include <vector>
#include <cfloat>   // FLT_MAX
#include <climits>  // ULONG_MAX

namespace tIDLib
{

/* ---------------- conversion functions ---------------------- */
float freq2bin(float freq, float n, float sr)
{
	float bin;

	bin = freq*n/sr;

	return(bin);
}

float bin2freq(float bin, float n, float sr)
{
	float freq;

	freq = bin*sr/n;

	return(freq);
}

float bark2freq(float bark)
{
	float freq;
	tIDLib::t_bark2freqFormula formula; // TODO: this should be an enum

	// formula 2 was used in timbreID 0.6
	formula = tIDLib::bark2freqFormula0;

	switch(formula)
	{
		case tIDLib::bark2freqFormula0:
			freq = 600.0 * sinh(bark/6.0);
			break;
		case tIDLib::bark2freqFormula1:
			freq = 53548.0/(bark*bark - 52.56*bark + 690.39);
			break;
		case tIDLib::bark2freqFormula2:
			freq = 1960/(26.81/(bark+0.53) - 1);
			freq = (freq<0)?0:freq;
			break;
		default:
			freq = 0;
	}

	return(freq);
}

float freq2bark(float freq)
{
	float barkFreq;
	tIDLib::t_freq2barkFormula formula; // TODO: this should be an enum

	formula = tIDLib::freq2barkFormula0;

	switch(formula)
	{
		case tIDLib::freq2barkFormula0:
			barkFreq = 6.0*asinh(freq/600.0);
			break;
		case tIDLib::freq2barkFormula1:
			barkFreq = 13*atan(0.00076*freq) + 3.5*atan(powf((freq/7500), 2));
			break;
		case tIDLib::freq2barkFormula2:
			barkFreq = ((26.81*freq)/(1960+freq))-0.53;
			if(barkFreq<2)
				barkFreq += 0.15*(2-barkFreq);
			else if(barkFreq>20.1)
				barkFreq += 0.22*(barkFreq-20.1);
			break;
		default:
			barkFreq = 0;
			break;
	}

	barkFreq = (barkFreq<0)?0:barkFreq;

	return(barkFreq);
}

float freq2mel(float freq)
{
	float mel;

	mel = 1127*log(1+(freq/700));
	mel = (mel<0)?0:mel;
	return(mel);
}

float mel2freq(float mel)
{
	float freq;

	freq = 700 * (exp(mel/1127) - 1);
//	freq = 700 * (exp(mel/1127.01048) - 1);
	freq = (freq<0)?0:freq;
	return(freq);
}
/* ---------------- END conversion functions ---------------------- */

/* ---------------- utility functions ---------------------- */

signed char signum(float input)
{
	signed char sign = 0;

	if(input > 0)
		sign = 1;
	else if(input < 0)
		sign = -1;
	else
		sign = 0;

	return(sign);
}
/* ---------------- END utility functions ---------------------- */

/* ---------------- dsp utility functions ---------------------- */

void peakSample(std::vector<float> &input, unsigned long int *peakIdx, float *peakVal)
{
	*peakVal = -FLT_MAX;
	*peakIdx = ULONG_MAX;

	for(unsigned long int i = 1; i < input.size(); ++i)
	{
		if(fabs(input[i]) > *peakVal)
		{
			*peakIdx = i;
			*peakVal = fabs(input[i]);
		}
	}
}

//TODO: check why sometimes the returned index is greater than the max (Original pd module seems to do that too)
unsigned long int findAttackStartSamp(std::vector<float> &input, float sampDeltaThresh, unsigned short int numSampsThresh)
{
	unsigned long int startSamp = ULONG_MAX;

	unsigned long int i = input.size();
	while(i--)
	{
		if(fabs(input[i]) <= sampDeltaThresh)
		{
			unsigned short int sampCount = 1;
			unsigned long int j = i;

			while(j--)
			{
				if(fabs(input[j]) <= sampDeltaThresh)
				{
					sampCount++;

					if(sampCount >= numSampsThresh)
					{
						startSamp = j;
						return(startSamp);
					}
				}
				else
					break;
			}
		}
	}

	return(startSamp);
}

// this could also return the location of the zero crossing
unsigned int zeroCrossingRate(const std::vector<float>& buffer)
{
    jassert(buffer.size() > 0);

	unsigned int crossings = 0;

	for(size_t sample = 1; sample < buffer.size(); ++sample)
		crossings += abs(signum(buffer[sample]) - signum(buffer[sample-1]));

	crossings *= 0.5f;

	return crossings;
}

/* ---------------- END dsp utility functions ---------------------- */

}
