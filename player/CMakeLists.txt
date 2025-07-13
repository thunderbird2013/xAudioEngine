# Mini-Test-Player für Konsole
add_executable(xAudioEnginePlayer MiniConsolePlayer.cpp)

target_include_directories(xAudioEnginePlayer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(xAudioEngine PRIVATE -Wno-unused-variable -Wno-unused-private-field)
endif()

# Nur in Release optimieren
target_compile_options(xAudioEnginePlayer PRIVATE
    $<$<CONFIG:Release>:
        $<IF:$<CXX_COMPILER_ID:MSVC>,/O2 /DNDEBUG,-O3 -s -DNDEBUG>
    >
)


target_link_libraries(xAudioEnginePlayer PRIVATE xAudioEngine)

install(TARGETS xAudioEnginePlayer
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/dist/Release
)

