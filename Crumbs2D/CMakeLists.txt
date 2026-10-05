# ---------------------------------------------------------------------------
# Crumbs2D : la bibliothèque de physique
# Attendu : include/crumbs2d/, src/, vendor/glm/
# ---------------------------------------------------------------------------

# glm (header-only). Si le projet parent (Oven) fournit déjà glm::glm,
# on réutilise sa cible pour éviter deux copies.
if(NOT TARGET glm::glm)
    add_library(crumbs2d_glm INTERFACE)
    target_include_directories(crumbs2d_glm SYSTEM INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
    )
    add_library(glm::glm ALIAS crumbs2d_glm)
endif()

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
target_link_libraries(crumbs2d PUBLIC glm::glm)

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