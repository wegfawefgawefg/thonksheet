# blobview: the viewer. Included by the root CMakeLists.txt. SDL3 + Vulkan + Dear ImGui; its
# own small Vulkan renderer in src/view/renderer, laid out like Mans In My House's.
# Builds against blobcore when it exists (BLOB_HAS_CORE=1); otherwise only the fake blob runs.

find_package(Threads REQUIRED)
find_package(Vulkan QUIET)
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL3 QUIET IMPORTED_TARGET sdl3)
        if(SDL3_FOUND)
            add_library(SDL3::SDL3 ALIAS PkgConfig::SDL3)
        endif()
    endif()
endif()
find_program(GLSLANG glslangValidator)
if(NOT Vulkan_FOUND OR NOT SDL3_FOUND OR NOT GLSLANG)
    message(STATUS "blobview skipped: needs Vulkan, SDL3 and glslangValidator")
    return()
endif()

# Dear ImGui, vendored unmodified (third_party/imgui/VERSION). Gubsy's copy has no Vulkan
# backend, so this one (with the SDL3 and Vulkan backends) is used instead.
set(imgui_dir ${CMAKE_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
    ${imgui_dir}/imgui.cpp
    ${imgui_dir}/imgui_demo.cpp
    ${imgui_dir}/imgui_draw.cpp
    ${imgui_dir}/imgui_tables.cpp
    ${imgui_dir}/imgui_widgets.cpp
    ${imgui_dir}/backends/imgui_impl_sdl3.cpp
    ${imgui_dir}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC ${imgui_dir})
target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL3::SDL3)

file(GLOB_RECURSE view_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/view/*.cpp)
# blobrun's flags and task stepping, shared so a task runs the same in both
set(view_run_sources ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp
    ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp)
add_executable(blobview ${view_sources} ${view_run_sources})
target_link_libraries(blobview PRIVATE blobworld)
target_include_directories(blobview PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/view)
target_compile_options(blobview PRIVATE ${warnings} -O2 -march=native)
target_link_libraries(blobview PRIVATE Threads::Threads Vulkan::Vulkan SDL3::SDL3 imgui)
target_compile_definitions(blobview PRIVATE BLOBVIEW_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders")
if(TARGET blobcore)
    target_link_libraries(blobview PRIVATE blobcore)
    target_compile_definitions(blobview PRIVATE BLOB_HAS_CORE=1)
endif()

# shaders: GLSL to SPIR-V in build/shaders, next to the binary
file(GLOB view_shader_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/*.vert
     ${CMAKE_SOURCE_DIR}/shaders/*.frag ${CMAKE_SOURCE_DIR}/shaders/*.comp)
file(GLOB view_shader_includes CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/*.glsl)
set(view_shader_outputs)
foreach(source ${view_shader_sources})
    get_filename_component(name ${source} NAME)
    set(output ${CMAKE_BINARY_DIR}/shaders/${name}.spv)
    add_custom_command(OUTPUT ${output}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${GLSLANG} -V --target-env vulkan1.3 -I${CMAKE_SOURCE_DIR}/shaders -o ${output} ${source}
        DEPENDS ${source} ${view_shader_includes}
        COMMENT "glslang ${name}")
    list(APPEND view_shader_outputs ${output})
endforeach()
add_custom_target(blobview_shaders ALL DEPENDS ${view_shader_outputs})
add_dependencies(blobview blobview_shaders)
