cmake_minimum_required(VERSION 3.21)

project(Crumbs2D
    VERSION 0.1.0
    DESCRIPTION "2D physics engine"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Type de build
# Debug          : -O0 -g, assertions actives
# Release        : -O3, assertions désactivées (NDEBUG)
# RelWithDebInfo : -O2 -g, assertions désactivées (pour profiler)
# Par défaut Debug, uniquement si Crumbs2D est le projet principal :
# quand Oven l'inclut, c'est Oven qui décide.
# ---------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo)
endif()

if(PROJECT_IS_TOP_LEVEL)
    # compile_commands.json pour l'IntelliSense de VS Code
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

message(STATUS "Crumbs2D build type: ${CMAKE_BUILD_TYPE}")

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(CRUMBS2D_BUILD_TESTS "Build Crumbs2D unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CRUMBS2D_BUILD_SANDBOX "Build Crumbs2D visual sandbox" ${PROJECT_IS_TOP_LEVEL})
option(CRUMBS2D_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(CRUMBS2D_NATIVE_ARCH "Optimize for the CPU of this machine (non portable binary)" OFF)

# ---------------------------------------------------------------------------
# Bibliothèque
# ---------------------------------------------------------------------------
add_subdirectory(Crumbs2D)

# ---------------------------------------------------------------------------
# Tests automatisés
# ---------------------------------------------------------------------------
if(CRUMBS2D_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(Tests)
endif()

# ---------------------------------------------------------------------------
# Sandbox
# ---------------------------------------------------------------------------
if(CRUMBS2D_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()