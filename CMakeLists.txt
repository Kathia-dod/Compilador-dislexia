cmake_minimum_required(VERSION 3.10)
project(Compilador_dislexia)

set(CMAKE_CXX_STANDARD 17)

# Ubuntu/Debian: sudo apt install nlohmann-json3-dev
find_package(nlohmann_json REQUIRED)

add_executable(compilador
    src/main.cpp
    src/analizador_lexico.cpp
    src/analizador_sintactico.cpp   
    src/analizador_semantico.cpp    
)

target_include_directories(compilador PRIVATE src)
target_link_libraries(compilador PRIVATE nlohmann_json::nlohmann_json)
