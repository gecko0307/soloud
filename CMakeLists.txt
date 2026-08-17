cmake_minimum_required(VERSION 3.15)
project(SoLoud VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build SoLoud as a shared library" ON)
option(SOLOUD_WITH_WINMM "Enable WinMM backend" ON)
option(SOLOUD_WITH_WASAPI "Enable WASAPI backend" ON)
option(SOLOUD_WITH_XAUDIO2 "Enable XAudio2 backend" ON)
option(SOLOUD_WITH_DSOUND "Enable DirectSound backend" ON)
option(SOLOUD_WITH_COREAUDIO "Enable CoreAudio backend" ON)
option(SOLOUD_WITH_ALSA "Enable ALSA backend" ON)
option(SOLOUD_WITH_OSS "Enable OSS backend" ON)
option(SOLOUD_WITH_JACK "Enable JACK backend" OFF)
option(SOLOUD_WITH_SDL2 "Enable SDL2 backend" ON)
option(SOLOUD_WITH_SDL3 "Enable SDL3 backend" ON)
option(SOLOUD_WITH_MINIAUDIO "Enable Miniaudio backend" ON)
option(SOLOUD_WITH_OPENAL "Enable OpenAL backend" ON)
option(SOLOUD_WITH_PORTAUDIO "Enable PortAudio backend" ON)
option(SOLOUD_WITH_NULL "Enable Null backend" ON)
option(SOLOUD_WITH_NOSOUND "Enable NoSound backend" ON)

add_library(soloud)

file(GLOB_RECURSE SOLOUD_CORE_SOURCES 
    "src/core/*.cpp"
)
target_sources(soloud PRIVATE ${SOLOUD_CORE_SOURCES})

file(GLOB SOLOUD_FILTER_SOURCES 
    "src/filter/*.cpp"
    "src/filter/*.c"
)
target_sources(soloud PRIVATE ${SOLOUD_FILTER_SOURCES})

file(GLOB_RECURSE SOLOUD_AUDIO_SOURCES 
    "src/audiosource/**/*.cpp"
    "src/audiosource/**/*.c"
)
target_sources(soloud PRIVATE ${SOLOUD_AUDIO_SOURCES})

file(GLOB SOLOUD_C_API_SOURCES 
    "src/c_api/*.cpp"
    "src/c_api/*.c"
)

if(WIN32 AND BUILD_SHARED_LIBS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/c_api/soloud.def")
        list(APPEND SOLOUD_C_API_SOURCES "src/c_api/soloud.def")
    endif()
endif()

target_sources(soloud PRIVATE ${SOLOUD_C_API_SOURCES})

target_include_directories(soloud 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/c_api>
        $<INSTALL_INTERFACE:include>
    PRIVATE 
        src/core
        src/audiosource/wav
)

if(SOLOUD_WITH_WINMM)
    if(WIN32)
        target_sources(soloud PRIVATE src/backend/winmm/soloud_winmm.cpp)
        target_compile_definitions(soloud PUBLIC WITH_WINMM)
    endif()
endif()

if(SOLOUD_WITH_WASAPI)
    if(WIN32)
        target_sources(soloud PRIVATE src/backend/wasapi/soloud_wasapi.cpp)
        target_compile_definitions(soloud PUBLIC WITH_WASAPI)
        target_link_libraries(soloud PRIVATE ksuser ole32)
    endif()
endif()

if(SOLOUD_WITH_XAUDIO2)
    if(WIN32)
        target_sources(soloud PRIVATE src/backend/xaudio2/soloud_xaudio2.cpp)
        target_compile_definitions(soloud PUBLIC WITH_XAUDIO2)
    endif()
endif()

if(SOLOUD_WITH_DSOUND)
    if(WIN32)
        target_sources(soloud PRIVATE src/backend/dsound/soloud_dsound.cpp)
        target_compile_definitions(soloud PUBLIC WITH_DSOUND)
        target_link_libraries(soloud PRIVATE dsound)
    endif()
endif()

if(SOLOUD_WITH_COREAUDIO)
    if(APPLE)
        target_sources(soloud PRIVATE src/backend/coreaudio/soloud_coreaudio.cpp)
        target_compile_definitions(soloud PUBLIC WITH_COREAUDIO)
        target_link_libraries(soloud PRIVATE "-framework AudioToolbox" "-framework AudioUnit")
    endif()
endif()

if(SOLOUD_WITH_ALSA)
    if(LINUX)
        find_package(ALSA REQUIRED)
        target_sources(soloud PRIVATE src/backend/alsa/soloud_alsa.cpp)
        target_compile_definitions(soloud PUBLIC WITH_ALSA)
        target_link_libraries(soloud PRIVATE ALSA::ALSA pthread dl m)
    endif()
endif()

if(SOLOUD_WITH_OSS)
    if(LINUX)
        target_sources(soloud PRIVATE src/backend/oss/soloud_oss.cpp)
        target_compile_definitions(soloud PUBLIC WITH_OSS)
    endif()
endif()

if(SOLOUD_WITH_JACK)
    if(LINUX)
        target_sources(soloud PRIVATE src/backend/jack/soloud_jack.cpp)
        target_compile_definitions(soloud PUBLIC WITH_JACK)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(JACK REQUIRED IMPORTED_TARGET jack)
        target_link_libraries(soloud PRIVATE PkgConfig::JACK)
    endif()
endif()

if(SOLOUD_WITH_SDL2)
    target_include_directories(soloud PRIVATE src/backend/sdl/include_sdl2)
    target_sources(soloud PRIVATE 
        src/backend/sdl/soloud_sdl2.cpp
        src/backend/sdl/soloud_sdl2_dll.c
    )
    target_compile_definitions(soloud PUBLIC WITH_SDL2)
    if(LINUX)
        target_link_libraries(soloud PRIVATE dl)
    endif()
endif()

if(SOLOUD_WITH_SDL3)
    target_include_directories(soloud PRIVATE src/backend/sdl/include_sdl3)
    target_sources(soloud PRIVATE 
        src/backend/sdl/soloud_sdl3.cpp
        src/backend/sdl/soloud_sdl3_dll.c
    )
    target_compile_definitions(soloud PUBLIC WITH_SDL3)
    if(LINUX)
        target_link_libraries(soloud PRIVATE dl)
    endif()
endif()

if(SOLOUD_WITH_MINIAUDIO)
    target_sources(soloud PRIVATE src/backend/miniaudio/soloud_miniaudio.cpp)
    target_compile_definitions(soloud PUBLIC WITH_MINIAUDIO)
    target_include_directories(soloud PRIVATE src/backend/miniaudio)
endif()

if(SOLOUD_WITH_OPENAL)
    target_include_directories(soloud PRIVATE src/backend/openal/include)
    target_sources(soloud PRIVATE 
        src/backend/openal/soloud_openal.cpp
        src/backend/openal/soloud_openal_dll.c
    )
    target_compile_definitions(soloud PUBLIC WITH_OPENAL)
    if(LINUX)
        target_link_libraries(soloud PRIVATE dl)
    endif()
endif()

if(SOLOUD_WITH_PORTAUDIO)
    target_include_directories(soloud PRIVATE src/backend/portaudio/include)
    target_sources(soloud PRIVATE 
        src/backend/portaudio/soloud_portaudio.cpp
        src/backend/portaudio/soloud_portaudio_dll.c
    )
    target_compile_definitions(soloud PUBLIC WITH_PORTAUDIO)
    if(LINUX)
        target_link_libraries(soloud PRIVATE dl)
    endif()
endif()

if(SOLOUD_WITH_NULL)
    target_sources(soloud PRIVATE src/backend/null/soloud_null.cpp)
    target_compile_definitions(soloud PUBLIC WITH_NULL)
endif()

if(SOLOUD_WITH_NOSOUND)
    target_sources(soloud PRIVATE src/backend/nosound/soloud_nosound.cpp)
    target_compile_definitions(soloud PUBLIC WITH_NOSOUND)
endif()

if(WIN32)
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(soloud PRIVATE DLL_EXPORT)
    else()
        target_compile_definitions(soloud PUBLIC SOLOUD_STATIC)
    endif()
endif()

install(TARGETS soloud EXPORT SoLoudTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT SoLoudTargets FILE SoLoudConfig.cmake DESTINATION lib/cmake/SoLoud)
