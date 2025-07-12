#define NOMINMAX
#include "AudioEngine.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>


std::string formatTime(double seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    char buffer[10];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", mins, secs);
    return std::string(buffer);
}

int main(int argc, char** argv) {
     SetConsoleOutputCP(CP_UTF8);    // UTF-8 für die Konsole  
    if (argc < 2) {
        std::cout << "Usage: player <file>\n";
        return 1;
    }

    std::string path = argv[1];
    AudioEngine engine;
    float vol;

    if (!engine.loadFile(path)) {
        std::cerr << "Datei konnte nicht geladen werden: " << path << "\n";
        return 1;
    }
    

    engine.setVolume(1.0f);
    engine.play();

    bool running = true;
    while (running) {

        std::cout << "Spiele: " << path << "\n[q = Quit, p = Play/Pause, s = Stop, f = +5s, b = -5s, +/- = Volume]\n";
        std::cout << "[INFO] [DEBUG] Gesamtdauer: " << formatTime(engine.getTotalTimeSeconds()) << "\n";

        char c;
        std::cin >> c;

        switch (c) {
            case 'q':
                engine.stop();
                running = false;
                break;          
            case '+':
                vol = engine.getVolume();
                vol = std::min(1.0f, vol + 0.1f);
                engine.setVolume(vol);
                std::cout << "Volume: " << vol << "\n";
                break;
            case '-':
                 vol = engine.getVolume();
                 vol = std::max(0.0f, vol - 0.1f);
                 engine.setVolume(vol);
                 std::cout << "Volume: " << vol << "\n";
                break;
            case 's':
                engine.stop();
                std::cout << "Gestoppt.\n";
                break;
            case 'p':
                engine.play();
                std::cout << "Wiedergabe gestartet.\n";
                break;
            case 'f': { // Vorspulen
                    int sekunden = 5;
                    int newFrame = engine.getCurrentFrame() + sekunden * engine.getSampleRate();
                    if (engine.seek(newFrame)) {
                        std::cout << "+5s\n";
                    } else {
                        std::cout << "Vorspulen fehlgeschlagen.\n";
                    }
                    break;
            }
            case 'b': { // Zurueckspulen
                    int sekunden = 5;
                    int newFrame = engine.getCurrentFrame() - sekunden * engine.getSampleRate();
                    if (engine.seek(std::max(0, newFrame))) {
                        std::cout << "-5s\n";
                    } else {
                        std::cout << "Ruecksprung fehlgeschlagen.\n";
                    }
                    break;
            }    
        }
    }

    return 0;
}
