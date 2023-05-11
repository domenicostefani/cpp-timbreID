/*
  ==============================================================================

    Real time logger for timbreID

    Logger that works with real time threads
    It makes use of a circular buffer to post some messages and have them
    written to file by a polling routine outside of the real time audio thread.

    Created: 19 Nov 2020
    Author:  Domenico Stefani (domenico.stefani[at]unitn.it)

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cstring>  // For strncpy
#include <memory>   // For unique_ptr
#include "choc/containers/choc_SingleReaderSingleWriterFIFO.h"  // Jules' thread safe fifo buffer implementation

namespace tid   /* TimbreID namespace*/
{

/** Logger class for real-time usage */
class RealTimeLogger
{
public:
    static const size_t LOG_MAX_ENTRIES = 2048; // Size of the fifo buffer for the log

    /** Entry in the log */
    struct LogEntry {
        static const int MESSAGE_LENGTH = 240;       // Length of messages in the fifo buffer
        int64 timeAtStart,  // Intended to log time intervals
            timeAtEnd;    // Intended to log time intervals
        char message[MESSAGE_LENGTH+1];
    };

    RealTimeLogger() { fifoBuffer.reset(LOG_MAX_ENTRIES); };
    RealTimeLogger(std::string name) { fifoBuffer.reset(LOG_MAX_ENTRIES); \
                                       this->name = name;};
    ~RealTimeLogger() { };

    bool logInfo(const char message[], int64 timeAtStart, int64 timeAtEnd);
    bool logInfo(const char message[], const char suffix[]=nullptr);

    bool logValue(const char message[], int value, const char suffix[]=nullptr);
    bool logValue(const char message[], float value, const char suffix[]=nullptr);
    bool logValue(const char message[], double value, const char suffix[]=nullptr);
    bool logValue(const char message[], long unsigned int value, const char suffix[]=nullptr);
    bool logValue(const char message[], unsigned int value, const char suffix[]=nullptr);
    bool logValue(const char message[], const char value[]);
    bool pop(LogEntry& logEntry);

    std::string getName() const { return this->name; }
private:
    choc::fifo::SingleReaderSingleWriterFIFO<RealTimeLogger::LogEntry> fifoBuffer;
    std::string name = "";
};

/** Add message to the log buffer */
inline bool RealTimeLogger::logInfo(const char message[], int64 timeAtStart, int64 timeAtEnd)
{
    LogEntry logEntry;
    logEntry.timeAtStart = timeAtStart;
    logEntry.timeAtEnd = timeAtEnd;
    strncpy(logEntry.message, message, LogEntry::MESSAGE_LENGTH);
    logEntry.message[LogEntry::MESSAGE_LENGTH] = '\0';
    return fifoBuffer.push(logEntry);
}

/** Add message to the log buffer */
inline bool RealTimeLogger::logInfo(const char message[], const char suffix[])
{
    if(suffix)
    {
        char cmessage[LogEntry::MESSAGE_LENGTH+1];
        snprintf(cmessage,sizeof(cmessage),"%s %s", message,suffix);
        return logInfo(cmessage,0,0);
    }
    else
    {
        return logInfo(message,0,0);
    }
}

/** Pop safely an element from the fifo buffer
 *  Do this from a non RT thread if you intend on writing to a file
*/
inline bool RealTimeLogger::pop(LogEntry& logEntry)
{
    return fifoBuffer.pop(logEntry);
}

inline bool RealTimeLogger::logValue(const char message[], int value, const char suffix[])
{
    char cmessage[LogEntry::MESSAGE_LENGTH+1];
    if (suffix)
        snprintf(cmessage,sizeof(cmessage),"%s %d %s", message,value,suffix);
    else
        snprintf(cmessage,sizeof(cmessage),"%s %d", message,value);
    return logInfo(cmessage);
}

inline bool RealTimeLogger::logValue(const char message[], float value, const char suffix[])
{
    char cmessage[LogEntry::MESSAGE_LENGTH+1];
    if (suffix)
        snprintf(cmessage,sizeof(cmessage),"%s %f %s", message,value,suffix);
    else
        snprintf(cmessage,sizeof(cmessage),"%s %f", message,value);
    return logInfo(cmessage);
}

inline bool RealTimeLogger::logValue(const char message[], double value, const char suffix[])
{
    char cmessage[LogEntry::MESSAGE_LENGTH+1];
    if (suffix)
        snprintf(cmessage,sizeof(cmessage),"%s %f %s", message,value,suffix);
    else
        snprintf(cmessage,sizeof(cmessage),"%s %f", message,value);
    return logInfo(cmessage);
}

inline bool RealTimeLogger::logValue(const char message[], long unsigned int value, const char suffix[])
{
    char cmessage[LogEntry::MESSAGE_LENGTH+1];
    if (suffix)
        snprintf(cmessage,sizeof(cmessage),"%s %lu %s", message,value,suffix);
    else
        snprintf(cmessage,sizeof(cmessage),"%s %lu", message,value);
    return logInfo(cmessage);
}

inline bool RealTimeLogger::logValue(const char message[], unsigned int value, const char suffix[])
{
    char cmessage[LogEntry::MESSAGE_LENGTH+1];
    if (suffix)
        snprintf(cmessage,sizeof(cmessage),"%s %u %s", message,value,suffix);
    else
        snprintf(cmessage,sizeof(cmessage),"%s %u", message,value);
    return logInfo(cmessage);
}

inline bool RealTimeLogger::logValue(const char message[], const char value[])
{
    return this->logInfo(message,value);
}

} // namespace tid
