/*

freq2bark - Calculate the Bark frequency of a given frequency in Hertz
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "freq2bark.hpp"

#include "tIDLib.hpp"
#include <string>
#include <stdexcept>

namespace tid   /* TimbreID namespace*/
{
/* ------------------------ freq2bark -------------------------------- */
float Freq2bark::calculate(float freq)
{
	if(freq >= 0.0 && freq <= tIDLib::MAXBARKFREQ)
		return tIDLib::freq2bark(freq);
	else
        throw std::domain_error("Frequency must be between 0 and " + std::to_string(tIDLib::MAXBARKFREQ) + " Hz");
}
} // namespace tid
