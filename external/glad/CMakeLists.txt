cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(glad)

if (EMSCRIPTEN)
    add_library(glad INTERFACE)
    target_include_directories(glad INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
else()
    add_library(glad STATIC src/glad.c)
    target_include_directories(glad 
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()