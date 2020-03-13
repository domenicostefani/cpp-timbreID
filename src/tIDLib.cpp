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

#include <cmath>


/* ---------------- conversion functions ---------------------- */
float tIDLib_freq2bin(float freq, float n, float sr)
{
	float bin;

	bin = freq*n/sr;

	return(bin);
}

float tIDLib_bin2freq(float bin, float n, float sr)
{
	float freq;

	freq = bin*sr/n;

	return(freq);
}

float tIDLib_bark2freq(float bark)
{
	float freq;
	t_bark2freqFormula formula; // TODO: this should be an enum

	// formula 2 was used in timbreID 0.6
	formula = bark2freqFormula0;

	switch(formula)
	{
		case bark2freqFormula0:
			freq = 600.0 * sinh(bark/6.0);
			break;
		case bark2freqFormula1:
			freq = 53548.0/(bark*bark - 52.56*bark + 690.39);
			break;
		case bark2freqFormula2:
			freq = 1960/(26.81/(bark+0.53) - 1);
			freq = (freq<0)?0:freq;
			break;
		default:
			freq = 0;
	}

	return(freq);
}

float tIDLib_freq2bark(float freq)
{
	float barkFreq;
	t_freq2barkFormula formula; // TODO: this should be an enum

	formula = freq2barkFormula0;

	switch(formula)
	{
		case freq2barkFormula0:
			barkFreq = 6.0*asinh(freq/600.0);
			break;
		case freq2barkFormula1:
			barkFreq = 13*atan(0.00076*freq) + 3.5*atan(powf((freq/7500), 2));
			break;
		case freq2barkFormula2:
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

float tIDLib_freq2mel(float freq)
{
	float mel;

	mel = 1127*log(1+(freq/700));
	mel = (mel<0)?0:mel;
	return(mel);
}

float tIDLib_mel2freq(float mel)
{
	float freq;

	freq = 700 * (exp(mel/1127) - 1);
//	freq = 700 * (exp(mel/1127.01048) - 1);
	freq = (freq<0)?0:freq;
	return(freq);
}
/* ---------------- END conversion functions ---------------------- */
