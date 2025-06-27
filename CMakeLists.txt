cmake_minimum_required(VERSION 3.10)
project(AI_Simulation_CPP)
set(CMAKE_CXX_STANDARD 17)

# Simulation executable (main.exe)
set(SIM_SOURCES
    main.cpp
    GameApp.cpp
    Game.cpp
    Player.cpp
    Hunter.cpp
    Food.cpp
)
add_executable(main ${SIM_SOURCES})
find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(main ${SDL2_LIBRARIES} SDL2_ttf)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Master process executable (Master.exe)
add_executable(Master Master.cpp)
set_target_properties(Master PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")