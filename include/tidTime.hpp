#pragma once

namespace tid   /* TimbreID namespace*/
{

class Time
{
public:
    static uint32_t getTimeSince(uint32_t lastTime)
    {
        return (tid::Time::currentTimeMillis() - lastTime);
    }
};

} // namespace tid
