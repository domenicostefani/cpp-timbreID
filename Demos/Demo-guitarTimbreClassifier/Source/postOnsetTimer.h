#ifndef POST_ONSET_TIMER_H
#define POST_ONSET_TIMER_H

using int64 = long long;
enum TimerState {IDLE, STARTED};

/** Simple timer for the post onset delay
 * Processed block counter used as a timer to add a delay after an onset is detected.
*/
struct PostOnsetTimer
{
public:
    PostOnsetTimer() { reset(); }
    /** Reset the post onset timer*/
    void reset();
    /** Prepare the timer to start by setting it to a prepare state */
    void prepare(int sampleRate, int blockSize);
    /** Start the timer */
    float start(double delay_ms);
    /** Return true if the timer is counting */
    bool isStarted() { return state == TimerState::STARTED; }
    /** Return true if the timer is not started or in prepare state */
    bool isIdle() { return state == TimerState::IDLE; }
    /** Return true if the timer is expired, than reset it */
    bool isExpired();
    /** Update the timer at the end of a block processing routine */
    int64 updateTimer();
private:
    TimerState state;
    int64 timerCounter = 0,deadline = 0;
    int blockSize = 0;
    double sampleRate = 0.0f;
};

inline void PostOnsetTimer::reset()
{
    state = TimerState::IDLE;
    int64 timerCounter = 0,deadline = 0;
}

inline void PostOnsetTimer::prepare(int sampleRate, int blockSize)
{
    if (sampleRate <= 0)
        throw std::logic_error("Sample rate can't be zero or negative");
    if (blockSize <= 0)
        throw std::logic_error("blockSize can't be zero or negative");

    this->blockSize = blockSize;
    this->sampleRate = sampleRate;
}

inline float PostOnsetTimer::start(double delay_ms)
{
    if ((sampleRate <= 0) || (blockSize <= 0))
        throw std::logic_error("call the prepare function with positive parameters before using the timer");

    if (delay_ms <= 0)
        throw std::logic_error("Delay can't be zero or negative");
    if (state == TimerState::STARTED)
        throw std::logic_error("Cannot call start in STARTED state, call reset before");
    timerCounter = 0;
    // Compute deadline in number of blocks
    deadline = std::round((delay_ms / 1000.0) * sampleRate / blockSize);
    state = TimerState::STARTED;
    return deadline * blockSize / sampleRate *1000;
}

inline bool PostOnsetTimer::isExpired()
{
    if ((state == TimerState::STARTED) && (timerCounter >= deadline))
    {
        reset();
        return true;
    }
    return false;
}

inline int64 PostOnsetTimer::updateTimer()
{
    if ( state == TimerState::STARTED )
        return timerCounter += 1;
    return -1;
}

#endif
