cmake_minimum_required(VERSION 3.21)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(Graphics)

set(CMAKE_BUILD_TYPE Debug)


include(cmake/CPM.cmake)

include(Libs.cmake)

find_package(fmt CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(Vulkan)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_subdirectory(root/core)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(root/app)
    message(STATUS "[${PROJECT_NAME}] Building as standalone app -> compiling src/app")
else()
    message(STATUS "[${PROJECT_NAME}] Included as library -> skipping src/app")
endif()
