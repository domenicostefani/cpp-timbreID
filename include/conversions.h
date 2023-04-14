/**
 * Multiple conversion externals from the timbreID library
 * Probably less useful, converted just to practice porting.
 * - bark2freq
 * - freq2bark
 * - freq2mel
 * - mel2freq
 * 
 */







/*

bark2freq
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.h"
#include <string>
#include <stdexcept>

namespace tid   /* TimbreID namespace*/
{
class Bark2freq
{
public:
    static float calculate(float bark)
    {
        if(bark >= 0.0f && bark <= tIDLib::MAXBARKS)
    		return tIDLib::bark2freq(bark);
    	else
            throw std::domain_error("Bark value must be between 0.0 and " + std::to_string(tIDLib::MAXBARKS) + " Barks");
    }
};


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
class Freq2bark
{
public:
    static float calculate(float freq)
    {
        if(freq >= 0.0f && freq <= tIDLib::MAXBARKFREQ)
    		return tIDLib::freq2bark(freq);
    	else
            throw std::domain_error("Frequency must be between 0 and " + std::to_string(tIDLib::MAXBARKFREQ) + " Hz");
    }
};


/*

freq2mel - Calculate the mel frequency of a given frequency in Hertz
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

class Freq2mel
{
public:
    static float calculate(float freq)
    {
    	if(freq >= 0.0f && freq <= tIDLib::MAXMELFREQ)
    		return tIDLib::freq2mel(freq);
    	else
            throw std::domain_error("Frequency must be between 0 and " + std::to_string(tIDLib::MAXMELFREQ) + " Hz");        
    }
};





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

class Mel2freq
{
public:
    static float calculate(float mel)
    {
        if(mel >= 0.0f && mel <= tIDLib::MAXMELS)
    		return tIDLib::mel2freq(mel);
    	else
            throw std::domain_error("Mel frequency must be between 0 and " + std::to_string(tIDLib::MAXMELS) + " mels");
    }
};
} // namespace tid
