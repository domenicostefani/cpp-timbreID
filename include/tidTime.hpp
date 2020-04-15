#pragma once

namespace tid   /* TimbreID namespace*/
{

class Time
{
public:
    static uint32 getTimeSince(uint32 lastTime)
    {
        return (juce::Time::currentTimeMillis() - lastTime);
    }
};

} // namespace tid
