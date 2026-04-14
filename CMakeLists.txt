cmake_minimum_required(VERSION 3.20)

project(meshviewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MESHVIEWER_USE_NFD "Enable Native File Dialog support" ON)

set(MESHVIEWER_SOURCES
    myproj/main.cpp
    myproj/myFace.cpp
    myproj/myHalfedge.cpp
    myproj/myMesh.cpp
    myproj/myPoint3D.cpp
    myproj/myVector3D.cpp
    myproj/myVertex.cpp
)

add_executable(meshviewer ${MESHVIEWER_SOURCES})

target_include_directories(meshviewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/myproj
)

find_package(OpenGL REQUIRED)
include(FetchContent)
message(STATUS "Fetching SDL2, GLEW and GLM via FetchContent")

# SDL2
set(SDL_SHARED ON CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL2)

# GLEW (CMake files are under build/cmake)
set(BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(GLEW_REGAL OFF CACHE BOOL "" FORCE)
set(GLEW_OSMESA OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glew
    URL https://downloads.sourceforge.net/glew/glew-2.3.1.tgz
    URL_HASH MD5=923ac4437518cc7cd323adde8a4091ae
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(glew)

# GLM (header-only)
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_98 OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_11 OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_14 OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_17 OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_20 OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS_CXX_23 OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${sdl2_SOURCE_DIR}/include
)
target_link_libraries(imgui PUBLIC ${MESHVIEWER_SDL2_LIB} ${MESHVIEWER_OPENGL_LIB})

# GLEW headers live in the fetched source tree.
target_include_directories(meshviewer PRIVATE
    ${glew_SOURCE_DIR}/include
)

if(TARGET OpenGL::GL)
    set(MESHVIEWER_OPENGL_LIB OpenGL::GL)
elseif(OPENGL_gl_LIBRARY)
    set(MESHVIEWER_OPENGL_LIB ${OPENGL_gl_LIBRARY})
else()
    message(FATAL_ERROR "OpenGL library target not found.")
endif()

if(TARGET SDL2::SDL2)
    set(MESHVIEWER_SDL2_LIB SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(MESHVIEWER_SDL2_LIB SDL2::SDL2-static)
elseif(TARGET SDL2)
    set(MESHVIEWER_SDL2_LIB SDL2)
elseif(TARGET SDL2-static)
    set(MESHVIEWER_SDL2_LIB SDL2-static)
elseif(SDL2_LIBRARIES)
    set(MESHVIEWER_SDL2_LIB ${SDL2_LIBRARIES})
else()
    message(FATAL_ERROR "SDL2 target not found after dependency resolution.")
endif()

if(TARGET GLEW::GLEW)
    set(MESHVIEWER_GLEW_LIB GLEW::GLEW)
elseif(TARGET GLEW::glew)
    set(MESHVIEWER_GLEW_LIB GLEW::glew)
elseif(TARGET glew)
    set(MESHVIEWER_GLEW_LIB glew)
elseif(TARGET glew_s)
    set(MESHVIEWER_GLEW_LIB glew_s)
    target_compile_definitions(meshviewer PRIVATE GLEW_STATIC)
elseif(GLEW_LIBRARIES)
    set(MESHVIEWER_GLEW_LIB ${GLEW_LIBRARIES})
else()
    message(FATAL_ERROR "GLEW target not found after dependency resolution.")
endif()

if(TARGET glm::glm)
    set(MESHVIEWER_GLM_TARGET glm::glm)
elseif(TARGET glm::glm-header-only)
    set(MESHVIEWER_GLM_TARGET glm::glm-header-only)
else()
    set(MESHVIEWER_GLM_TARGET "")
endif()

target_link_libraries(meshviewer PRIVATE
    ${MESHVIEWER_OPENGL_LIB}
    ${MESHVIEWER_SDL2_LIB}
    ${MESHVIEWER_GLEW_LIB}
    imgui
)

if(MESHVIEWER_GLM_TARGET)
    target_link_libraries(meshviewer PRIVATE ${MESHVIEWER_GLM_TARGET})
else()
    # Fallback for older/non-standard GLM CMake exports.
    target_include_directories(meshviewer PRIVATE ${glm_SOURCE_DIR})
endif()

if(MESHVIEWER_USE_NFD)
    # Fetch NFD (Native File Dialog Extended)
    FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG v1.2.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nfd)

    target_link_libraries(meshviewer PRIVATE nfd)
    target_compile_definitions(meshviewer PRIVATE MESHVIEWER_ENABLE_NFD=1)
    message(STATUS "Native File Dialog enabled")
endif()

if(MSVC)
    target_compile_definitions(meshviewer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Copy shaders and mesh files next to executable.
add_custom_command(TARGET meshviewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/myproj/shaders"
    "$<TARGET_FILE_DIR:meshviewer>/shaders"
)

file(GLOB MESHVIEWER_MESH_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/myproj/*.obj"
    "${CMAKE_CURRENT_SOURCE_DIR}/myproj/*.objtri.obj"
)
foreach(mesh_file IN LISTS MESHVIEWER_MESH_FILES)
    add_custom_command(TARGET meshviewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${mesh_file}"
        "$<TARGET_FILE_DIR:meshviewer>"
    )
endforeach()

# Copy runtime DLLs from FetchContent targets on Windows.
if(WIN32)
    if(TARGET SDL2::SDL2)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    elseif(TARGET SDL2)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL2>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    endif()

    if(TARGET glew)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:glew>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    elseif(TARGET GLEW::glew)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:GLEW::glew>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    elseif(TARGET GLEW::GLEW)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:GLEW::GLEW>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    endif()

    if(MESHVIEWER_USE_NFD AND TARGET nfd)
        add_custom_command(TARGET meshviewer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:nfd>"
            "$<TARGET_FILE_DIR:meshviewer>"
        )
    endif()
endif()
