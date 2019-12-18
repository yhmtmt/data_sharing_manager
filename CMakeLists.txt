add_library(data_sharing_manager SHARED f_data_sharing_manager.cpp)

target_include_directories(data_sharing_manager PUBLIC ${PROJECT_SOURCE_DIR}/include)
install(TARGETS data_sharing_manager DESTINATION lib)
