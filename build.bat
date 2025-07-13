@echo off
setlocal

:: Konfiguration: STATIC oder SHARED
set BUILD_SHARED_LIBS=OFF

:: Pfad zum Build-Verzeichnis
set BUILD_DIR=build

echo.
echo [INFO] Erstelle Build-Verzeichnis: %BUILD_DIR%
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo.
echo [INFO] Starte CMake-Konfiguration (BUILD_SHARED_LIBS=%BUILD_SHARED_LIBS%)
cmake .. -DBUILD_SHARED_LIBS=%BUILD_SHARED_LIBS%

if errorlevel 1 (
    echo [ERROR] CMake-Konfiguration fehlgeschlagen!
    exit /b 1
)

echo.
echo [INFO] Baue Projekt im Release-Modus ...
cmake --build . --config Release

if errorlevel 1 (
    echo [ERROR] Build fehlgeschlagen!
    exit /b 1
)

echo.
echo [INFO] Build erfolgreich abgeschlossen!
endlocal
