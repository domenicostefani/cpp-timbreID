#pragma once

#include <string>
#include <JuceHeader.h>
#include "tIDLib.hpp"

namespace tid   /* TimbreID namespace*/
{

class Log {
public:
    Log(std::string filename)
    {
        this->logFilename = filename;
        juce::String jpath(tIDLib::LOG_PATH);
        jpath = juce::File::addTrailingSeparator(jpath);
        this->path = jpath.toStdString();

        this->jLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(this->path, this->logFilename, tIDLib::LOG_EXTENSION, "TimbreID - log"));
        this->jLogger->logMessage("Starting " + filename + " log at: " + this->path);
    }

    ~Log(){}

    std::string getLogPath() const
    {
        return (this->path + this->logFilename + tIDLib::LOG_EXTENSION);
    }

    void logInfo(std::string module, std::string text) const
    {
        this->jLogger->logMessage(module + ": " + text);
    }

    void logInfo(std::string text) const
    {
        this->jLogger->logMessage(text);
    }

    void logError(std::string module, std::string text) const
    {
        this->jLogger->logMessage("ERROR! " + module + ": " + text);
    }
private:
    std::string logFilename;
    std::string path;
    std::unique_ptr<FileLogger> jLogger;
};

} // namespace tid
