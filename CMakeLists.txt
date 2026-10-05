cmake_minimum_required(VERSION 3.21)

project(Crumbs2D
    VERSION 0.1.0
    DESCRIPTION "2D physics engine"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Options
# Les tests sont activés par défaut seulement quand Crumbs2D est compilé seul,
# pas quand il est inclus par Oven via add_subdirectory().
# ---------------------------------------------------------------------------
option(CRUMBS2D_BUILD_TESTS "Build Crumbs2D tests" ${PROJECT_IS_TOP_LEVEL})
option(CRUMBS2D_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# ---------------------------------------------------------------------------
# Dépendance : glm (header-only, dans vendor/glm)
# Si le projet parent (Oven) fournit déjà glm::glm, on réutilise sa cible
# pour éviter deux copies / deux versions de glm.
# ---------------------------------------------------------------------------
if(NOT TARGET glm::glm)
    add_library(crumbs2d_glm INTERFACE)
    target_include_directories(crumbs2d_glm SYSTEM INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
    )
    add_library(glm::glm ALIAS crumbs2d_glm)
endif()

# ---------------------------------------------------------------------------
# Bibliothèque
# ---------------------------------------------------------------------------
file(GLOB_RECURSE CRUMBS2D_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(crumbs2d ${CRUMBS2D_SOURCES})
add_library(Crumbs2D::crumbs2d ALIAS crumbs2d)

target_include_directories(crumbs2d
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(crumbs2d PUBLIC cxx_std_20)

# PUBLIC car les headers publics de Crumbs2D exposent des types glm
target_link_libraries(crumbs2d PUBLIC glm::glm)

# Warnings : PRIVATE pour ne pas les imposer aux projets qui utilisent Crumbs2D
if(MSVC)
    target_compile_options(crumbs2d PRIVATE /W4 /permissive-)
    if(CRUMBS2D_WARNINGS_AS_ERRORS)
        target_compile_options(crumbs2d PRIVATE /WX)
    endif()
else()
    target_compile_options(crumbs2d PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    if(CRUMBS2D_WARNINGS_AS_ERRORS)
        target_compile_options(crumbs2d PRIVATE -Werror)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(CRUMBS2D_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()