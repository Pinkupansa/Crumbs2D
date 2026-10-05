cmake_minimum_required(VERSION 3.21)

project(Crumbs2D
    VERSION 0.1.0
    DESCRIPTION "2D physics engine"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Options
# Tests et Sandbox sont activés par défaut seulement quand Crumbs2D est
# compilé seul, pas quand il est inclus par Oven via add_subdirectory().
# ---------------------------------------------------------------------------
option(CRUMBS2D_BUILD_TESTS "Build Crumbs2D unit tests" ${PROJECT_IS_TOP_LEVEL})
option(CRUMBS2D_BUILD_SANDBOX "Build Crumbs2D visual sandbox" ${PROJECT_IS_TOP_LEVEL})
option(CRUMBS2D_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# ---------------------------------------------------------------------------
# Bibliothèque
# ---------------------------------------------------------------------------
add_subdirectory(Crumbs2D)

# ---------------------------------------------------------------------------
# Tests automatisés (dossier Tests/, ajouté seulement s'il existe)
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