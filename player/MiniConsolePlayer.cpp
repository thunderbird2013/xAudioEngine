#define NOMINMAX
#include "AudioEngine.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
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

    if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
        if (!engine.loadURL(path)) {
            std::cerr << "URL konnte nicht geladen werden: " << path << "\n";
            return 1;
        }
    } else {
        if (!engine.loadFile(path)) {
            std::cerr << "Datei konnte nicht geladen werden: " << path << "\n";
            return 1;
        }
    }
    

    engine.setVolume(0.9f);
    engine.play();

    bool running = true;

while (running) 
{
    // Nur eine Zeile für die Position verwenden
    std::cout << "\rPos: " << formatTime(engine.getCurrentTimeSeconds()) 
              << " / " << formatTime(engine.getTotalTimeSeconds()) 
              << "   (↑↓ Vol, ←→ Seek, p Play, s Stop, q Quit)   " << std::flush;

    if (!engine.isPlaying()) {
        std::cout << "\nWiedergabe beendet.\n";
     return 0;
    }
      // Tasteneingabe prüfen
    if (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            int special = _getch();
            switch (special) {
                case 72: { // ↑ Volume +
                    float vol = engine.getVolume();
                    vol = std::min(1.0f, vol + 0.1f);
                    engine.setVolume(vol);
                    break;
                }
                case 80: { // ↓ Volume -
                    float vol = engine.getVolume();
                    vol = std::max(0.0f, vol - 0.1f);
                    engine.setVolume(vol);
                    break;
                }
                case 77: { // → Seek +
                    int newFrame = engine.getCurrentFrame() + 5 * engine.getSampleRate();
                    engine.seek(newFrame);
                    break;
                }
                case 75: { // ← Seek -
                    int newFrame = std::max(0, engine.getCurrentFrame() - 5 * engine.getSampleRate());
                    engine.seek(newFrame);
                    break;
                }
            }
        } else {
            switch (ch) {
                case 'q': running = false; engine.stop(); break;
                case 's': engine.stop(); break;
                case 'p': engine.play(); break;
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}
    return 0;
}
