project(vm)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

file(GLOB SOURCES "src/*.cpp")

add_executable(vm ${SOURCES})



if(CMAKE_BUILD_TYPE MATCHES Debug)
     target_compile_options(vm PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak)  
     target_link_options(vm PRIVATE -fsanitize=address -fsanitize=undefined -fsanitize=leak ) 
endif()
