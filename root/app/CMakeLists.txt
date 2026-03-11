# root/app/CMakeLists.txt
file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_executable(app ${app_sources})

target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../core/include"
)

target_link_libraries(app
    PRIVATE
        ${PROJECT_NAME}
        fmt
)

# Platform definitions - must be here where app is defined
if(UNIX AND NOT APPLE)
    target_compile_definitions(app PRIVATE NOVA_LINUX)
    target_link_libraries(app PRIVATE dl pthread)
endif()

if(WIN32)
    target_compile_definitions(app PRIVATE NOVA_WINDOWS)
endif()

set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_custom_target(run
    COMMAND app
    DEPENDS app
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    COMMENT "Running the application..."
)

add_custom_target(windows
    COMMAND ${CMAKE_COMMAND}
        -B ${CMAKE_SOURCE_DIR}/build-windows
        -S ${CMAKE_SOURCE_DIR}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/mingw-w64.cmake
        -DVCPKG_TARGET_TRIPLET=x64-mingw-static
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_SOURCE_DIR}/build-windows
    COMMENT "Cross-compiling for Windows..."
)