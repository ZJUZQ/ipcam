if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
   cmake_minimum_required(VERSION 2.8)
   project(ipc_driver)
   #set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME})
   #set(CMAKE_INSTALL_PREFIX /home/test/3rdparty/ipc)

   #set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME})
   #set(CPACK_GENERATOR "TGZ")
   #include(CPack) 
   
   find_package(OpenCV REQUIRED)
   include_directories(${OpenCV_INCLUDE_DIRS}) 
   
   set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
   set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

   ### SAK Library
   add_subdirectory(./sak)
   include_directories(${SAK_INCLUDE_DIR})
else()
    ### Set the INCLUDE_DIR for parent scope
    set(IPC_INCLUDE_DIR 
        ${CMAKE_CURRENT_SOURCE_DIR}/ipc/inc
        PARENT_SCOPE)   
endif() 

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -msse4.1")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -msse4.1")      

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dh/inc)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/dh/lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hc/inc)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hc/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hc/lib/HCNetSDKCom)

add_subdirectory(./ipc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ipc/inc)

if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
   add_subdirectory(demo)
endif()

add_subdirectory(./3rdparty)

