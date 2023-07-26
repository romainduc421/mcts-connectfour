cmake_minimum_required(VERSION 2.8)
project(fourInARow)

set(CMAKE_C_STANDARD 11)

add_compile_options(-O3)
add_compile_options(-Wall)
add_compile_options(-Wno-unused-result)

add_executable(jeu jeu.c)
target_link_libraries(jeu m)
