#include "Logger.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <mutex>

std::mutex Logger::logMutex;

static std::ofstream logFile;

static void ensureLogFile() {
    static bool initialized = false;
    if (!initialized) {
       // std::filesystem::create_directory("log"); // Stelle sicher, dass Ordner existiert
        logFile.open("xAudioEngine.log", std::ios::app); // Anhängen
        initialized = true;
    }
}

void Logger::log(const std::string& msg, Level level) {
    std::lock_guard<std::mutex> lock(logMutex);
    ensureLogFile();
   
    std::string line = "[" + std::string(levelToString(level)) + "] " + msg; // Formatierung der Log-Nachricht

    #define LOG_TO_CONSOLE 0 // Definiere, ob die Log-Nachrichten auch auf der Konsole ausgegeben werden sollen
    #if LOG_TO_CONSOLE
    std::cout << line << std::endl; // Ausgabe auf der Konsole
    #endif

    if (logFile.is_open()) {
        logFile << line << std::endl;
    }
}

const char* Logger::levelToString(Level level) {
    switch (level) {
        case Level::Info: return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error: return "ERROR";
        default: return "UNKNOWN";
    }
}
