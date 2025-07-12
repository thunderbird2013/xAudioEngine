#pragma once
#include <string>
#include <mutex>

class Logger {
public:
    enum class Level {
        Info,
        Warning,
        Error
    };

    static void log(const std::string& msg, Level level = Level::Info);

private:
    static const char* levelToString(Level level);
    static std::mutex logMutex;
};
