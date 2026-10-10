# ---------------------------------------------------------------------------
# Crumbs2D : tests automatisés (doctest)
# Attendu : src/, vendor/doctest/ (submodule)
# ---------------------------------------------------------------------------

file(GLOB_RECURSE CRUMBS2D_TEST_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Crumbs2DTests ${CRUMBS2D_TEST_SOURCES})

# SYSTEM : pas de warnings remontés depuis le header de doctest
target_include_directories(Crumbs2DTests SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest
)

target_compile_features(Crumbs2DTests PRIVATE cxx_std_20)
target_link_libraries(Crumbs2DTests PRIVATE Crumbs2D::crumbs2d)

if(MSVC)
    target_compile_options(Crumbs2DTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(Crumbs2DTests PRIVATE -Wall -Wextra)
endif()

# Enregistre l'exécutable auprès de CTest (lancé avec `ctest` depuis build/)
add_test(NAME Crumbs2DTests COMMAND Crumbs2DTests)