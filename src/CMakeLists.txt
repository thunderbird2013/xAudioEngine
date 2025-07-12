# Option für statisch vs. dynamisch
option(BUILD_SHARED_LIBS "Build shared library" ON)

file(GLOB LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/stb_vorbis.c
)

add_library(AudioEngine ${LIB_SOURCES})  # erzeugt .lib oder .dll je nach BUILD_SHARED_LIBS

target_include_directories(AudioEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_SOURCE_DIR}/thirdparty/miniaudio
)

target_compile_definitions(AudioEngine PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# Definiere Export-Symbol je nach Build-Art
if(BUILD_SHARED_LIBS)
    target_compile_definitions(AudioEngine PRIVATE AUDIOENGINE_EXPORTS)
else()
    target_compile_definitions(AudioEngine PUBLIC AUDIOENGINE_STATIC)
endif()

target_link_libraries(AudioEngine ole32)

# Installiere die erzeugte Library
install(TARGETS AudioEngine
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/dist/Release      # .dll
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/dist/Release      # .so / .dll
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/static_lib/Release # .lib
)

# Header-Dateien mitinstallieren
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_BINARY_DIR}/dist/include
        FILES_MATCHING PATTERN "*.h")
