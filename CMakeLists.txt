cmake_minimum_required(VERSION 3.10)
project(tls-proxy)

file(GLOB_RECURSE sources      src/*.cpp include/*.h)
file(GLOB_RECURSE sources_test tests/src/*.cpp tests/include/*.h)

add_executable(${PROJECT_NAME} ${sources})

target_compile_options(${PROJECT_NAME} PUBLIC -std=c++14 -Wall -ggdb3 -O0)
target_include_directories(${PROJECT_NAME} PUBLIC include)

find_package(OpenSSL REQUIRED)

if(NOT OPENSSL_VERSION MATCHES "^1.1.1")
    message(FATAL_ERROR "OpenSSL version should be from 1.1.1 stream")
endif()

message("OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message("OpenSSL libraries: ${OPENSSL_LIBRARIES}")
include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
	pthread
    ${OPENSSL_LIBRARIES}
)
