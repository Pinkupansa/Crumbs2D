# ---------------------------------------------------------------------------
# Sandbox : application de debug visuel (raylib + ImGui optionnel)
# Attendu : src/, vendor/raylib/, et optionnellement vendor/imgui/ et vendor/rlImGui/
# ---------------------------------------------------------------------------

set(SANDBOX_VENDOR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

# raylib : les options doivent être définies AVANT add_subdirectory.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${SANDBOX_VENDOR}/raylib ${CMAKE_CURRENT_BINARY_DIR}/raylib EXCLUDE_FROM_ALL)

file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(Sandbox ${SANDBOX_SOURCES})

target_include_directories(Sandbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(Sandbox PRIVATE cxx_std_20)
target_link_libraries(Sandbox PRIVATE raylib Crumbs2D::crumbs2d)

# ImGui + rlImGui : activés automatiquement dès que les submodules sont présents.
if(EXISTS ${SANDBOX_VENDOR}/imgui/imgui.cpp AND EXISTS ${SANDBOX_VENDOR}/rlImGui/rlImGui.cpp)
    add_library(sandbox_imgui STATIC
        ${SANDBOX_VENDOR}/imgui/imgui.cpp
        ${SANDBOX_VENDOR}/imgui/imgui_draw.cpp
        ${SANDBOX_VENDOR}/imgui/imgui_tables.cpp
        ${SANDBOX_VENDOR}/imgui/imgui_widgets.cpp
        ${SANDBOX_VENDOR}/imgui/imgui_demo.cpp
        ${SANDBOX_VENDOR}/rlImGui/rlImGui.cpp
    )
    target_include_directories(sandbox_imgui SYSTEM PUBLIC
        ${SANDBOX_VENDOR}/imgui
        ${SANDBOX_VENDOR}/rlImGui
    )
    target_link_libraries(sandbox_imgui PUBLIC raylib)

    target_link_libraries(Sandbox PRIVATE sandbox_imgui)
    target_compile_definitions(Sandbox PRIVATE CRUMBS2D_SANDBOX_HAS_IMGUI)
    message(STATUS "Crumbs2D Sandbox: ImGui enabled")
else()
    message(STATUS "Crumbs2D Sandbox: ImGui not found, building without it")
endif()

if(MSVC)
    target_compile_options(Sandbox PRIVATE /W4 /permissive-)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)
else()
    target_compile_options(Sandbox PRIVATE -Wall -Wextra)
endif()