
#include "tidLog.hpp"

namespace tid
{

std::string Log::path = "/";
const std::string Log::LOG_FILENAME = "timbreID";
std::unique_ptr<FileLogger> Log::jLogger;

const std::string LOG_EXTENSION = ".log";


void Log::setLogPath(std::string mpath)
{
    juce::String jpath(mpath);
    jpath = juce::File::addTrailingSeparator(jpath);
    Log::path = jpath.toStdString();

    Log::jLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(Log::path, Log::LOG_FILENAME, LOG_EXTENSION, "TimbreID - log"));
    Log::jLogger->logMessage("Starting log at: " + Log::path);
}

std::string Log::getInfoLogPath()
{
    return (Log::path + Log::LOG_FILENAME + LOG_EXTENSION);
}

std::string Log::getErrorLogPath()
{
    return (Log::path + Log::LOG_FILENAME + LOG_EXTENSION);
}

void Log::logInfo(std::string module, std::string text)
{
    Log::jLogger->logMessage(module + ": " + text);
}

void Log::logError(std::string module, std::string text)
{
    Log::jLogger->logMessage("ERROR! " + module + ": " + text);
}


}
