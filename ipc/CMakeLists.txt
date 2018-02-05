include_directories(./inc) 
aux_source_directory(./src src)

set(target_name "IPC_CAMERA") 

set(IPC_LIBRARY_TYPE SHARED)

add_library(${target_name} ${IPC_LIBRARY_TYPE} ${src}) 
                
target_link_libraries(${target_name} dhnetsdk dhplay 
                      HCCore PlayCtrl 
                      SuperRender hcnetsdk HCPreview
                      pthread
                      ${OpenCV_LIBS})

install (TARGETS ${target_name} DESTINATION ipc/lib)
install (DIRECTORY ./inc/ DESTINATION ipc/include)
