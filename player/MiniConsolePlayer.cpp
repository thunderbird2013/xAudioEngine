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
#include <filesystem>

// Globale Variablen
AudioEngine engine;
int selectedDevice = -1;
std::string path;
std::atomic<float> g_bpm{-1.0f};  // -1.0f = noch nicht berechnet

namespace fs = std::filesystem;

// Hilfsfunktion: Alle Audio-Dateien in einem Verzeichnis finden
std::vector<std::string> findAudioFiles(const std::string& directory) {
    std::vector<std::string> files;
    const std::vector<std::string> supportedExtensions = { ".mp3", ".flac", ".ogg", ".wav" };

    if (!fs::exists(directory)) {
        std::cerr << "[FEHLER] Pfad existiert NICHT: " << directory << "\n";
        return files;
    }

    if (!fs::is_directory(directory)) {
        std::cerr << "[FEHLER] Pfad ist KEIN Verzeichnis: " << directory << "\n";
        return files;
    }

    for (const auto& entry : fs::directory_iterator(directory)) {
        try {
            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            std::string fullPath = entry.path().string();            

            if (entry.is_regular_file() &&
                std::find(supportedExtensions.begin(), supportedExtensions.end(), ext) != supportedExtensions.end()) {

                // Testweise Pfad-Objekt erzeugen (wirkt wie Validierungs-Check)
                std::filesystem::path dummyCheck(fullPath);
                (void)dummyCheck; // suppress unused warning

                files.push_back(fullPath);
            }

        } catch (const std::exception& e) {
            std::cerr << "[WARN] Datei übersprungen (Pfadproblem): "
                      << entry.path() << "\n  Grund: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[WARN] Datei übersprungen (unbekannter Fehler): "
                      << entry.path() << "\n";
        }
    }

    std::sort(files.begin(), files.end());
    //std::cout << "[DEBUG] " << files.size() << " Audiodateien gefunden.\n";
    return files;
}




// 256-Farb-Codes (ungefährer Verlauf)
int yellowStart = 226; // hellgelb
int yellowEnd   = 220; // dunkleres orange

int redStart    = 203; // hellrot
int redEnd      = 160; // dunkelrot

// Helper: Interpolation zwischen zwei Farbwerten
int interpolateColor(int from, int to, float t) {
    return static_cast<int>(from + (to - from) * t);
}

// Clamp-Funktion
float clamp01(float x) {
    return std::max(0.0f, std::min(1.0f, x));
}


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
    std::cout << "  -listdevice          Alle verfügbaren Ausgabegeräte anzeigen\n";
    std::cout << "  -device <index>      Ausgabegerät auswählen (Index aus -listdevice)\n";
    std::cout << "  -playlist <folder>   Alle unterstützten Audio-Dateien im angegebenen Ordner abspielen\n";
    std::cout << "Examples:\n";
    std::cout << "  miniplayer -listdevice\n";
    std::cout << "  miniplayer -device 2 song.mp3\n";
    std::cout << "  miniplayer -playlist Music\n";
    std::cout << "  miniplayer -device 1 -playlist D:\\\\MeineMusik\n\n";
}

int main(int argc, char** argv) {

    SetConsoleOutputCP(CP_UTF8);  // UTF-8-Konsole

        // --- Argumente parsen ---
        std::string playlistDir;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-listdevice") {
                auto devices = engine.getAvailableOutputDevices();
                for (size_t j = 0; j < devices.size(); ++j) {
                    std::cout << "[" << j << "] " << devices[j] << "\n";
                }
                return 0;
            } else if (arg == "-device" && i + 1 < argc) {
                selectedDevice = std::stoi(argv[++i]);
            } else if (arg == "-file" && i + 1 < argc) {
                path = argv[++i];
            } else if (arg == "-playlist" && i + 1 < argc) {
                playlistDir = argv[++i];
            } else if (arg[0] != '-') {
                path = arg;
            }
        }

        // --- Playlist zuerst laden, falls angegeben ---
        std::vector<std::string> playlist;
        int trackIndex = 0;

        if (!playlistDir.empty()) {
            std::cout << "[DEBUG] Playlist-Ordner: " << playlistDir << "\n";
            playlist = findAudioFiles(playlistDir);

            if (playlist.empty()) {
                std::cerr << "Keine Audiodateien im angegebenen Ordner gefunden.\n";
                return 1;
            }

            path = playlist[trackIndex]; // Setze den ersten Track als Pfad          
        }

        // --- Wenn nichts geladen wurde, Hilfe anzeigen ---
        if (path.empty()) {
            printUsage();
            return 1;
        }


        // --- Laden der Datei/URL ---
        bool loaded = false;

        if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
            loaded = engine.loadURL(path);
        } else {
            std::cout << "[DEBUG] Lade lokale Datei: " << path << "\n";
            loaded = engine.loadFile(path);
        }

        if (!loaded) {
            std::cerr << "Konnte Datei/URL nicht laden: " << path << "\n";
            return 1;
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


    engine.setVolume(0.9f); // Lautstärke auf 90% setzen
    engine.play(); // Wiedergabe starten

    analyzeBPMInBackground(engine); // BPM-Analyse im Hintergrund starten

    // --- Steuerung wie gehabt ---
    bool running = true;
    while (running) {
           
        float bpm = g_bpm.load();

        int current = engine.getCurrentTimeSeconds();
        int total   = engine.getTotalTimeSeconds();
        int remaining = total - current;

        // Verhältnis berechnen
        float progress = clamp01(static_cast<float>(current) / total);
        float fadeOut  = clamp01(1.0f - static_cast<float>(remaining) / 30.0f); // 0–1 ab letzten 30s

        // Farbe berechnen
        int currentColorIndex = interpolateColor(yellowStart, yellowEnd, progress);     // von hellgelb → orange
        int totalColorIndex   = interpolateColor(redStart,    redEnd,   fadeOut);       // von hellrot → dunkelrot

        std::string currentColor = "\033[38;5;" + std::to_string(currentColorIndex) + "m";
        std::string totalColor   = "\033[38;5;" + std::to_string(totalColorIndex)   + "m";

         // Cursor 4 Zeilen hoch und Zeile löschen
        std::cout << "\033[?25l"; // Verstecke den Cursor
        std::cout << "\033[H";  // Cursor Home: Zeile 1, Spalte 1
        std::cout << "\033[J";   // ← restlichen Bildschirm ab hier löschen (optional)

        // Kopfzeile        
        std::cout << "------------------------------xAudioEngine----Player----------------------------------\n";
        std::cout << "Copyright (c) 2025 by Matthias Stoltze\n\n\n";
        std::cout << "Wiedergabe: " << (engine.isPlaying() ? "Aktiv" : "Gestoppt") << "\n";
        std::cout << "Aktuelle Lautstärke: " << std::fixed << std::setprecision(1) << engine.getVolume() * 100.0f << "%\n\n";
        std::cout << "--------------------------------------------------------------------------------------\n";
        std::cout << "ID3-Tags\n";
        // Titelinfos
        std::cout << "Title:     " << engine.getTitle()  << "\n"
                  << "Album:     " << engine.getAlbum()  << "\n";
        std::cout << "Interpret: " << engine.getArtist() << "\n";
        std::cout << "--------------------------------------------------------------------------------------\n";
        // Dateiname
        std::cout << "[Track " << (trackIndex + 1) << " of " << playlist.size() << "] - " << path << "\n";
        //std::cout << "Datei:     \"" << path << "\"\n";

        // BPM
        std::cout << "BPM:       [";
        if (bpm > 0.0f) {
            std::cout << std::fixed << std::setprecision(1) << bpm;
        } else {
            std::cout << "...";
        }
        std::cout << "]\n";

        // Trennlinie + Zeit
        std::cout << "--------------------------------------------------------------------------------------\n";
        std::cout << "Time:       "
          << currentColor << formatTime(current) << "\033[0m"
          << " / "
          << totalColor   << formatTime(total)   << "\033[0m"
          << "\n";
        std::cout << "--------------------------------------------------------------------------------------\n";

        // Steuerung
        std::cout << "(\u2191\u2193 Vol, \u2190\u2192 Seek, Pg\u2191/\u2193 Track, p Play, s Stop, q Quit)\n";    
        std::cout << std::flush;

       static bool userStopped = false;  // Zustand merken

       if (!engine.isPlaying()) {

            if (!userStopped) {
                // Wenn Playlist vorhanden → zum nächsten springen
                if (!playlist.empty() && trackIndex + 1 < playlist.size()) {
                    ++trackIndex;
                    path = playlist[trackIndex];
                    engine.loadFile(path);
                    engine.play();
                    analyzeBPMInBackground(engine);
                    clearConsole();
                    continue; // → weiter in der Schleife
                } else {
                    // Ende der Playlist erreicht
                    std::cout << "\nPlaylist vollständig abgespielt.\n";
                    break;
                }
            }
            // Wenn gestoppt, tue nichts → bleibt im gestoppten Zustand
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
                    case 73: // Page Up → vorheriger Track
                    if (!playlist.empty()) {
                        trackIndex = (trackIndex - 1 + playlist.size()) % playlist.size();
                        path = playlist[trackIndex];
                        engine.stop();
                        engine.loadFile(path);
                        engine.play();
                        analyzeBPMInBackground(engine);
                        clearConsole();
                    }
                    break;

                    case 81: // Page Down → nächster Track
                        if (!playlist.empty()) {
                            trackIndex = (trackIndex + 1) % playlist.size();
                            path = playlist[trackIndex];
                            engine.stop();
                            engine.loadFile(path);
                            engine.play();
                            analyzeBPMInBackground(engine);
                            clearConsole();
                        }
                    break;                    
                }
            } else {
                switch (ch) {
                    case 'q': running = false; engine.stop(); break;
                    case 's': engine.stop(); userStopped = true; break;
                    case 'p': engine.play(); userStopped = false; break;                                       
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    clearConsole();
    std::cout << "\033[?25h";
    std::cout << "\033[J";
    return 0;
}
