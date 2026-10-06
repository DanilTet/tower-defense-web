cmake_minimum_required(VERSION 3.10)
project(freetype_lib)

add_library(freetype SHARED IMPORTED)

set_target_properties(freetype PROPERTIES
    # Путь к .dll (нужна для работы игры)
    IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype.dll"
    
    # Путь к .lib (нужна линкеру при сборке)
    IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype.lib"
    IMPORTED_IMPLIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetyped.lib"
    IMPORTED_IMPLIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/lib/freetype.lib"
    
    # Путь к папке include, чтобы компилятор видел заголовочные файлы
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)