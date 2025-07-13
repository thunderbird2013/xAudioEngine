#define NOMINMAX
#include "AudioEngine.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
#include <algorithm>
#include <iomanip>
#include <atomic>

std::atomic<float> g_bpm{-1.0f};  // -1.0f = noch nicht berechnet

std::string formatTime(double seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    char buffer[10];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", mins, secs);
    return std::string(buffer);
}

void clearConsole() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD written;
    DWORD cells;

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    cells = csbi.dwSize.X * csbi.dwSize.Y;

    FillConsoleOutputCharacter(hConsole, ' ', cells, {0, 0}, &written);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cells, {0, 0}, &written);
    SetConsoleCursorPosition(hConsole, {0, 0});
}

void analyzeBPMInBackground(AudioEngine& engine) {
    std::thread([&engine]() {
        float bpm = engine.getBPM();
        g_bpm = bpm;
    }).detach();
}

void printUsage() {
    std::cout << "\nMiniPlayer v0.1 - Simple Console xAudioEngine Player\n";
    std::cout << "Supports: .wav .mp3 .ogg .flac\n";
    std::cout << "Usage:\n";
    std::cout << "  miniplayer [options] <file or URL>\n";
    std::cout << "Options:\n";
    std::cout << "  -listdevice        Alle verfügbaren Ausgabegeräte anzeigen\n";
    std::cout << "  -device <index>    Ausgabegerät auswählen (Index aus -listdevice)\n";
    std::cout << "Examples:\n";
    std::cout << "  miniplayer -listdevice\n";
    std::cout << "  miniplayer -device 2 song.mp3\n\n";
}

int main(int argc, char** argv) {

    SetConsoleOutputCP(CP_UTF8);  // UTF-8-Konsole

    if (argc < 2) {
        printUsage();
        return 1;
    }

    AudioEngine engine;
    int selectedDevice = -1;
    std::string path;

    // --- Argumente parsen ---
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-listdevice") {
            auto devices = engine.getAvailableOutputDevices();
            for (size_t j = 0; j < devices.size(); ++j) {
                std::cout << "[" << j << "] " << devices[j] << "\n";
            }
            return 0;
        }
        else if (arg == "-device" && i + 1 < argc) {
            selectedDevice = std::stoi(argv[++i]);
        }
        else if (arg[0] != '-') {
            path = arg;
        }
    }

    if (path.empty()) {
        printUsage();
        return 1;
    }

    // --- Laden der Datei/URL ---
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

    // --- Soundkarte initialisieren ---
    bool ok = false;
    if (selectedDevice >= 0) {
        ok = engine.initAudioDeviceWithAdapter(selectedDevice);
    } else {
        ok = engine.initAudioDevice();
    }

    if (!ok) {
        std::cerr << "AudioDevice konnte nicht initialisiert werden\n";
        return 1;
    }

    //float bpm = engine.getBPM();
    //std::cout << "Erkanntes BPM: " << bpm << std::endl;

    engine.setVolume(0.9f);
    engine.play();

    analyzeBPMInBackground(engine);

    // --- Steuerung wie gehabt ---
    bool running = true;
    while (running) {
           float bpm = g_bpm.load();
            std::cout << "\rPos: " << formatTime(engine.getCurrentTimeSeconds()) 
            << " / " << formatTime(engine.getTotalTimeSeconds()) 
            << "   (↑↓ Vol, ←→ Seek, p Play, s Stop, q Quit)   ";

            if (bpm > 0.0f) {
                std::cout << "BPM: " << std::fixed << std::setprecision(1) << bpm;
            } else {
                std::cout << "BPM: ...";
            }
            std::cout << "   " << std::flush;

        if (!engine.isPlaying()) {
            std::cout << "\nWiedergabe beendet.\n";
            return 0;
        }

        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 224) {
                int special = _getch();
                switch (special) {
                    case 72: engine.setVolume(std::min(1.0f, engine.getVolume() + 0.1f)); break;
                    case 80: engine.setVolume(std::max(0.0f, engine.getVolume() - 0.1f)); break;
                    case 77: engine.seek(engine.getCurrentFrame() + 5 * engine.getSampleRate()); break;
                    case 75: engine.seek(std::max(0, engine.getCurrentFrame() - 5 * engine.getSampleRate())); break;
                    clearConsole();
                }
            } else {
                switch (ch) {
                    case 'q': running = false; engine.stop(); break;
                    case 's': engine.stop(); break;
                    case 'p': engine.play(); break;
                    clearConsole();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
