# Mini-Test-Player für Konsole
add_executable(miniaudio_player MiniConsolePlayer.cpp)

target_include_directories(miniaudio_player PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Nur in Release optimieren
target_compile_options(miniaudio_player PRIVATE
    $<$<CONFIG:Release>:
        $<IF:$<CXX_COMPILER_ID:MSVC>,/O2 /DNDEBUG,-O3 -s -DNDEBUG>
    >
)


target_link_libraries(miniaudio_player PRIVATE AudioEngine)

install(TARGETS miniaudio_player
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/dist/Release
)

