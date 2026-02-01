cmake_minimum_required(VERSION 3.16)

# OpenVR headers (relative to driver directory -> emucontroller -> openvr)
set(OPENVR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# Driver DLL
add_library(driver_emucontroller SHARED
    src/hmd_driver_factory.cpp
    src/device_provider.cpp
    src/controller_device_driver.cpp
    src/ipc_server.cpp
    src/driverlog.cpp
)

target_include_directories(driver_emucontroller PRIVATE
    ${OPENVR_ROOT}/headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
    src
)

target_compile_definitions(driver_emucontroller PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# Link Windows libraries
target_link_libraries(driver_emucontroller PRIVATE
    ws2_32
)

# Output to driver directory structure (keeps Release/Debug subdirectory)
set_target_properties(driver_emucontroller PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/drivers/emucontroller/bin/win64"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/drivers/emucontroller/bin/win64"
)

# Copy DLL to SteamVR expected location and resources after build
add_custom_command(TARGET driver_emucontroller POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:driver_emucontroller>"
        "${CMAKE_BINARY_DIR}/drivers/emucontroller/bin/win64/driver_emucontroller.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/emucontroller/driver.vrdrivermanifest"
        "${CMAKE_BINARY_DIR}/drivers/emucontroller/driver.vrdrivermanifest"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/emucontroller/resources"
        "${CMAKE_BINARY_DIR}/drivers/emucontroller/resources"
)
