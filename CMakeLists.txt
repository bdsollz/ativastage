cmake_minimum_required(VERSION 3.27)

# Read version from the VERSION file (single source of truth).
file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" ATIVASTAGE_VERSION LIMIT_COUNT 1)
string(STRIP "${ATIVASTAGE_VERSION}" ATIVASTAGE_VERSION)

project(AtivaStage
    VERSION ${ATIVASTAGE_VERSION}
    DESCRIPTION "Church audiovisual projection and operation software"
    LANGUAGES CXX)

# ---- Global settings -------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(ATIVASTAGE_BUILD_TESTS "Build unit tests" ON)
option(ATIVASTAGE_BUILD_APP   "Build the QML application" OFF) # habilitado a partir da Fase 1

# ---- Warnings as a baseline ------------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- Dependencies ----------------------------------------------------------
# Qt é resolvido via CMAKE_PREFIX_PATH (definido nos presets). Só é exigido
# quando ATIVASTAGE_BUILD_APP=ON, para manter a Fase 0 leve.
if(ATIVASTAGE_BUILD_APP)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
    qt_standard_project_setup(REQUIRES 6.5)
endif()

# Catch2 vem do vcpkg (manifest mode).
if(ATIVASTAGE_BUILD_TESTS)
    enable_testing()
    find_package(Catch2 3 CONFIG QUIET)
    if(NOT Catch2_FOUND)
        message(STATUS "Catch2 não encontrado via vcpkg; testes que dependem dele serão pulados.")
    endif()
endif()

# ---- Modules ---------------------------------------------------------------
add_subdirectory(src/pal)
add_subdirectory(src/persistence)
add_subdirectory(src/audio)

if(ATIVASTAGE_BUILD_APP AND EXISTS ${CMAKE_SOURCE_DIR}/src/app/CMakeLists.txt)
    add_subdirectory(src/app)
elseif(ATIVASTAGE_BUILD_APP)
    message(STATUS "ATIVASTAGE_BUILD_APP=ON, mas src/app ainda não existe (Fase 0). "
                   "O alvo GUI chega na Fase 1; pulando por enquanto.")
endif()

# ---- Summary ---------------------------------------------------------------
message(STATUS "AtivaStage ${ATIVASTAGE_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests      : ${ATIVASTAGE_BUILD_TESTS}")
message(STATUS "  Build app  : ${ATIVASTAGE_BUILD_APP}")
