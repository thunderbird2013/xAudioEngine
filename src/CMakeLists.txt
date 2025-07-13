# Option für statisch vs. dynamisch
option(BUILD_SHARED_LIBS "Build shared library" ON)

# Alle Quelldateien inkl. stb_vorbis
file(GLOB LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/stb_vorbis.c
)

# --- aubio statisch importieren ---
add_library(aubio STATIC IMPORTED)
set_target_properties(aubio PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/aubio/lib/aubio.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/aubio/include"
)

# --- curl statisch importieren ---
add_library(libcurl STATIC IMPORTED)
set_target_properties(libcurl PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/libcurl_static/lib/libcurl.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/libcurl_static/include"
)

# --- zlib statisch importieren ---
add_library(zlib STATIC IMPORTED)
set_target_properties(zlib PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/zlib/lib/zlib.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/zlib/include"
)

# --- Hauptbibliothek erzeugen ---
add_library(xAudioEngine ${LIB_SOURCES})

# --- Include-Verzeichnisse ---
target_include_directories(xAudioEngine PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_SOURCE_DIR}/thirdparty/miniaudio
    ${CMAKE_SOURCE_DIR}/thirdparty/aubio/include
)

# --- Compiler-Definitionen ---
target_compile_definitions(xAudioEngine PRIVATE
    _CRT_SECURE_NO_WARNINGS
    CURL_STATICLIB
)

# --- Clang Workaround ---
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(xAudioEngine PRIVATE -Wno-unused-variable -Wno-unused-private-field)
endif()

# --- Build-Typ spezifisch: DLL oder statisch ---
if(BUILD_SHARED_LIBS)
    target_compile_definitions(xAudioEngine PRIVATE AUDIOENGINE_EXPORTS)
else()
    target_compile_definitions(xAudioEngine PUBLIC AUDIOENGINE_STATIC)
endif()

# --- Verlinkung ---
target_link_libraries(xAudioEngine PRIVATE
    aubio
    libcurl
    ws2_32
    crypt32
    winmm
    wldap32
    advapi32
    user32
    ole32
    secur32
    zlib
)

# --- Installation der kompilierten Lib ---
install(TARGETS xAudioEngine
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/dist/Release
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/dist/Release
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/static_lib/Release
)

# --- Header mitinstallieren ---
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_BINARY_DIR}/dist/include
        FILES_MATCHING PATTERN "*.h")
