cmake_minimum_required(VERSION 3.10)

project(TomateEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Solo se exporta lo marcado con FFI_EXPORT (ver tomate_api.cpp).
# Reduce tamaño y evita colisiones de símbolos.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(SOURCES
    src/Juego.cpp
    src/Jugador.cpp
    src/IA.cpp
    src/JuegoTutorial.cpp
    api/tomate_api.cpp
)

add_library(tomate_engine SHARED ${SOURCES})

target_include_directories(tomate_engine PRIVATE include)

if(MSVC)
    target_compile_options(tomate_engine PRIVATE /W3 /EHsc)
else()
    target_compile_options(tomate_engine PRIVATE -Wall -Wextra)
endif()

if(ANDROID)
    # Android 15+ / Google Play: páginas de memoria de 16 KB (solo aplica a ELF/Android).
    target_link_options(tomate_engine PRIVATE
        "-Wl,-z,max-page-size=16384"
        "-Wl,-z,common-page-size=16384"
        "-Wl,--gc-sections"
        "-Wl,--exclude-libs,ALL")
    # Para __android_log_print en tomate_api.cpp
    target_link_libraries(tomate_engine PRIVATE log)
endif()