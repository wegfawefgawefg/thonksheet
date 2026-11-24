cmake_minimum_required(VERSION 3.20)
project(thonksheet CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(warnings -Wall -Wextra -Wno-missing-field-initializers)
set(speed -O3 -march=native)

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
if(EXISTS ${CMAKE_SOURCE_DIR}/src/view/view.cmake)
    include(${CMAKE_SOURCE_DIR}/src/view/view.cmake)
endif()

# blobprobe: small measurements of a running blob (rhythm)
file(GLOB probe_sources CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/probe/*.cpp)
if(probe_sources AND TARGET blobcore)
    add_executable(blobprobe ${probe_sources})
    target_compile_options(blobprobe PRIVATE ${warnings} ${speed})
    target_link_libraries(blobprobe PRIVATE blobcore)
endif()
