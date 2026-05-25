cmake_minimum_required(VERSION 3.10)
project(Compilador_dislexia)

set(CMAKE_CXX_STANDARD 17)

add_executable(compilador
    src/main.cpp
    src/analizador_lexico.cpp
    src/analizador_sintactico.cpp
)

