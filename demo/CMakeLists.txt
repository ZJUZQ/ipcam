add_executable(demo_camera_hc ./demo_camera_hc.cpp)
target_link_libraries(demo_camera_hc sakUtils IPC_CAMERA)

add_executable(demo_camera_hc_undistort ./demo_camera_hc_undistort.cpp)
target_link_libraries(demo_camera_hc_undistort sakUtils IPC_CAMERA ${OpenCV_LIBS})


add_executable(demo_camera_dh ./demo_camera_dh.cpp)
target_link_libraries(demo_camera_dh sakUtils IPC_CAMERA)

add_executable(demo_camera_rtsp ./demo_camera_rtsp.cpp)
target_link_libraries(demo_camera_rtsp sakUtils ${OpenCV_LIBS})