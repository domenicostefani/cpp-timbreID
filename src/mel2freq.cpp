/*

mel2freq - Calculate the frequency in Hertz of a given mel frequency
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "mel2freq.hpp"

#include "tIDLib.hpp"
#include <string>
#include <stdexcept>

/* ------------------------ mel2freq -------------------------------- */
float Mel2freq::calculate(float mel)
{
	if(mel>=0.0 && mel<=MAXMELS)
		return tIDLib_mel2freq(mel);
	else
        throw std::domain_error("Mel frequency must be between 0 and " + std::to_string(MAXMELS) + " mels");
}
