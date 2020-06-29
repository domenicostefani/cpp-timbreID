#pragma once

#include <string>
#include <memory>
#include <JuceHeader.h>

namespace tid   /* TimbreID namespace*/
{

class Log {
public:
    static void setLogPath(std::string mpath);
    static std::string getInfoLogPath();
    static std::string getErrorLogPath();

    static void logInfo(std::string module, std::string logtext);
    static void logError(std::string module, std::string logtext);
private:
    static const std::string LOG_FILENAME;
    static std::string path;
    static std::unique_ptr<FileLogger> jLogger;
};

} // namespace tid
