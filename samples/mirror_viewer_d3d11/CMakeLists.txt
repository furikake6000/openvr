set(TARGET_NAME mirror_viewer_d3d11)

if(NOT WIN32)
  return()
endif()

add_executable(${TARGET_NAME} WIN32
  mirror_viewer_d3d11.cpp
)

target_link_libraries(${TARGET_NAME}
  ${OPENVR_LIBRARIES}
  d3d11
  dxgi
)

setTargetOutputDirectory(${TARGET_NAME})
