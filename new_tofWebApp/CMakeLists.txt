cmake_minimum_required(VERSION 2.8)
project(aditof-http-server)

find_package(Libwebsockets REQUIRED)

add_executable(${PROJECT_NAME} test-server.cpp ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(${PROJECT_NAME} PRIVATE aditof ${Protobuf_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE ${Protobuf_INCLUDE_DIRS} ${GENERATED_PROTO_FILES_DIR} ${LIBWEBSOCKETS_INCLUDE_DIRS})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11)
