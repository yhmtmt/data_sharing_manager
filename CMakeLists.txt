add_library(data_sharing_manager SHARED f_data_sharing_manager.cpp ${CHANS})

target_include_directories(data_sharing_manager PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(data_sharing_manager PUBLIC ${PROJECT_SOURCE_DIR}/channels)
install(TARGETS data_sharing_manager DESTINATION lib)
