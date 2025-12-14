cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0072 NEW)

project(wren)

add_subdirectory(libwren)
add_subdirectory(main)

add_subdirectory(testParser)
add_subdirectory(testLexer)
add_subdirectory(testCompiler)
add_subdirectory(testPerformance)
add_subdirectory(testTable)
 

