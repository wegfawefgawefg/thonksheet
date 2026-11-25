cmake_minimum_required(VERSION 3.20)
project(thonksheet CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(warnings -Wall -Wextra -Wno-missing-field-initializers)
# x86 here, Apple silicon on the borrowed Mac (stevejobs): tune for whichever we are on
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(speed -O3 -mcpu=native)
else()
    set(speed -O3 -march=native)
endif()

# Sources are picked up by folder, so adding a .cpp needs no edit here. Each target is only
# built once its folder has sources.

# core: the blob itself
file(GLOB blob_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/blob/*.cpp)
if(blob_sources)
    add_library(blobcore STATIC ${blob_sources})
    target_include_directories(blobcore PUBLIC src)
    target_compile_options(blobcore PRIVATE ${warnings} ${speed})
endif()

# blobbench: tick speed
file(GLOB bench_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/bench/*.cpp)
if(bench_sources AND TARGET blobcore)
    add_executable(blobbench ${bench_sources})
    target_compile_options(blobbench PRIVATE ${warnings} ${speed})
    target_link_libraries(blobbench PRIVATE blobcore)
endif()

# blobrun: one blob on one task, headless
file(GLOB world_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/world/*.cpp)
file(GLOB run_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/run/*.cpp)
if(world_sources)
    add_library(blobworld STATIC ${world_sources})
    target_include_directories(blobworld PUBLIC src)
    target_compile_options(blobworld PRIVATE ${warnings} ${speed})
endif()
if(run_sources AND TARGET blobworld)
    add_executable(blobrun ${run_sources})
    target_include_directories(blobrun PRIVATE src)
    target_compile_options(blobrun PRIVATE ${warnings} ${speed})
    target_link_libraries(blobrun PRIVATE blobworld)
    if(TARGET blobcore)
        target_link_libraries(blobrun PRIVATE blobcore)
        target_compile_definitions(blobrun PRIVATE BLOB_HAS_CORE=1)
    endif()
endif()

# blobview: the viewer. Its block lives in its own file so the viewer owner can edit freely.
# -DTHONK_NO_VIEW=1 leaves the viewer out (no Vulkan or SDL needed), for headless machines
if(EXISTS ${CMAKE_SOURCE_DIR}/src/view/view.cmake AND NOT THONK_NO_VIEW)
    include(${CMAKE_SOURCE_DIR}/src/view/view.cmake)
endif()

# blobprobe: small measurements of a running blob (rhythm)
file(GLOB probe_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/probe/*.cpp)
if(probe_sources AND TARGET blobcore)
    add_executable(blobprobe ${probe_sources})
    target_compile_options(blobprobe PRIVATE ${warnings} ${speed})
    target_link_libraries(blobprobe PRIVATE blobcore)
    # blobprobe linear runs real tasks through the plug (idea 58)
    if(TARGET blobworld)
        target_sources(blobprobe PRIVATE ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp
                       ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp)
        target_include_directories(blobprobe PRIVATE src)
        target_link_libraries(blobprobe PRIVATE blobworld)
    endif()
endif()

# blobtable: idea 26, table neurons, standing behind the same Blob struct (src/table/).
# blobrun drives it with --table; tablebench measures its tick speed.
file(GLOB table_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/table/*.cpp)
if(table_sources AND TARGET blobcore)
    add_library(blobtable STATIC ${table_sources})
    target_include_directories(blobtable PUBLIC src)
    target_compile_options(blobtable PRIVATE ${warnings} ${speed})
    if(TARGET blobrun)
        target_link_libraries(blobrun PRIVATE blobtable)
    endif()
    add_executable(tablebench ${CMAKE_SOURCE_DIR}/src/table/bench/tablebench.cpp)
    target_compile_options(tablebench PRIVATE ${warnings} ${speed})
    target_link_libraries(tablebench PRIVATE blobtable blobcore)
    # tableprobe: idea 57, where noise-graded credit goes (src/table/probe/)
    file(GLOB table_probe_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/table/probe/*.cpp)
    if(table_probe_sources AND TARGET blobworld)
        add_executable(tableprobe ${table_probe_sources}
                       ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp
                       ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp)
        target_compile_options(tableprobe PRIVATE ${warnings} ${speed})
        target_link_libraries(tableprobe PRIVATE blobtable blobworld blobcore)
    endif()
endif()

# blobminimal: the minimal engine (popcount neurons, one bit per source, noise-graded credit
# under a budget), and minimalprobe for its long-life, idle and speed measurements
file(GLOB minimal_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/minimal/*.cpp)
if(minimal_sources)
    add_library(blobminimal STATIC ${minimal_sources})
    target_include_directories(blobminimal PUBLIC src)
    target_compile_options(blobminimal PRIVATE ${warnings} ${speed})
    if(TARGET blobrun)
        target_link_libraries(blobrun PRIVATE blobminimal)
        target_compile_definitions(blobrun PRIVATE BLOB_HAS_MINIMAL=1)
    endif()
    file(GLOB minimal_probe_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/minimal/probe/*.cpp)
    if(minimal_probe_sources AND TARGET blobcore AND TARGET blobworld)
        add_executable(minimalprobe ${minimal_probe_sources}
                       ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp
                       ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp
                       ${CMAKE_SOURCE_DIR}/src/probe/linear_fit.cpp)
        target_compile_options(minimalprobe PRIVATE ${warnings} ${speed})
        target_link_libraries(minimalprobe PRIVATE blobminimal blobworld blobcore)
    endif()
endif()

# blobmemory: idea 123, a memory instead of a sheet (src/memory/): codes, a store found by
# content, counters per button. blobrun drives it with --engine memory.
file(GLOB memory_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/memory/*.cpp)
if(memory_sources)
    add_library(blobmemory STATIC ${memory_sources})
    target_include_directories(blobmemory PUBLIC src)
    target_compile_options(blobmemory PRIVATE ${warnings} ${speed})
    # the combination (--y.sheet) puts a minimal-engine sheet in front of the memory
    if(TARGET blobminimal)
        target_link_libraries(blobmemory PUBLIC blobminimal)
        target_compile_definitions(blobmemory PRIVATE MEMORY_HAS_SHEET=1)
    endif()
    if(TARGET blobrun)
        target_link_libraries(blobrun PRIVATE blobmemory)
        target_compile_definitions(blobrun PRIVATE BLOB_HAS_MEMORY=1)
    endif()
endif()
# memprobe: worlds the suite lacks, for the memory engine (src/memory/probe/)
file(GLOB memory_probe_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/memory/probe/*.cpp)
if(memory_probe_sources AND TARGET blobmemory AND TARGET blobworld AND TARGET blobcore)
    add_executable(memprobe ${memory_probe_sources})
    target_compile_options(memprobe PRIVATE ${warnings} ${speed})
    target_link_libraries(memprobe PRIVATE blobmemory blobworld blobcore)
endif()
# blobnn: the fixed reference networks (mlp, gru) behind the same shell, picked with
# blobrun --engine mlp|gru. floats, no fused multiply-add, so runs match across machines
file(GLOB nn_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/nn/*.cpp)
if(nn_sources AND TARGET blobworld AND TARGET blobrun)
    add_library(blobnn STATIC ${nn_sources})
    target_include_directories(blobnn PUBLIC src)
    target_compile_options(blobnn PRIVATE ${warnings} ${speed} -ffp-contract=off)
    target_link_libraries(blobnn PUBLIC blobworld)
    target_link_libraries(blobrun PRIVATE blobnn)
    target_compile_definitions(blobrun PRIVATE BLOB_HAS_NN=1)
endif()

# blobmnist: MNIST through the sheet, and the plain learners it is measured against (src/mnist/)
file(GLOB mnist_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/mnist/*.cpp)
if(mnist_sources AND TARGET blobcore AND TARGET blobworld)
    add_executable(blobmnist ${mnist_sources} ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp
                   ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp)
    target_include_directories(blobmnist PRIVATE src)
    target_compile_options(blobmnist PRIVATE ${warnings} ${speed})
    target_link_libraries(blobmnist PRIVATE blobworld blobcore)
endif()

# blobgpu: the minimal engine on a Vulkan compute device, many lives at once, headless only
# (src/gpu/, shaders/gpu/). Skipped without Vulkan or glslangValidator, or with THONK_NO_VIEW.
find_package(Vulkan QUIET)
find_program(GPU_GLSLANG glslangValidator)
file(GLOB gpu_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/gpu/*.cpp)
if(gpu_sources AND Vulkan_FOUND AND GPU_GLSLANG AND NOT THONK_NO_VIEW AND TARGET blobminimal
   AND TARGET blobworld AND TARGET blobcore)
    add_executable(blobgpu ${gpu_sources} ${CMAKE_SOURCE_DIR}/src/run/task_ticker.cpp
                   ${CMAKE_SOURCE_DIR}/src/run/run_flags.cpp)
    target_include_directories(blobgpu PRIVATE src)
    target_compile_options(blobgpu PRIVATE ${warnings} ${speed})
    find_package(Threads REQUIRED)
    target_link_libraries(blobgpu PRIVATE blobminimal blobworld blobcore Vulkan::Vulkan
                          Threads::Threads)
    target_compile_definitions(blobgpu PRIVATE BLOBGPU_SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/gpu")
    file(GLOB gpu_shader_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/gpu/*.comp)
    file(GLOB gpu_shader_includes CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/shaders/gpu/*.glsl)
    set(gpu_shader_outputs)
    foreach(source ${gpu_shader_sources})
        get_filename_component(name ${source} NAME)
        set(output ${CMAKE_BINARY_DIR}/shaders/gpu/${name}.spv)
        add_custom_command(OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders/gpu
            COMMAND ${GPU_GLSLANG} -V --target-env vulkan1.3 -I${CMAKE_SOURCE_DIR}/shaders/gpu
                    -o ${output} ${source}
            DEPENDS ${source} ${gpu_shader_includes}
            COMMENT "glslang gpu/${name}")
        list(APPEND gpu_shader_outputs ${output})
    endforeach()
    add_custom_target(blobgpu_shaders ALL DEPENDS ${gpu_shader_outputs})
    add_dependencies(blobgpu blobgpu_shaders)
else()
    message(STATUS "blobgpu skipped: needs Vulkan, glslangValidator and the minimal engine")
endif()
