# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
)

# Create library
add_library(${PROJECT_NAME} STATIC ${SOURCES})
add_library(Nova::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Include directory
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${imgui_SOURCE_DIR}/"
    "${Stb_INCLUDE_DIR}/"
)

# Link dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        fmt::fmt
        Nova::Core
        Nova::NovaDesktop
        GPUOpen::VulkanMemoryAllocator
        Vulkan::Vulkan
        assimp::assimp
)