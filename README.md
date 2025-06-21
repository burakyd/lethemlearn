# C++ Evolutionary AI Simulation

## Overview
This project began as a C++ port of an agent-based AI simulation originally developed in Python/Pygame. Since the initial conversion, it has evolved into an independent and significantly enhanced C++ project, featuring new algorithms, optimizations, and architectural improvements. The simulation demonstrates emergent intelligent behavior through neuroevolution, where a population of agents ("Players") controlled by neural networks are evolved using genetic algorithms to maximize survival and food collection in a 2D world.

## Features
- **Neuroevolution**: Agents use a feedforward neural network for decision-making. Weights are evolved using genetic algorithms (mutation, crossover, selection).
- **Genetic Algorithm**: Implements mutation, crossover (uniform, single-point, arithmetic), and elitism for evolving agent behavior.
- **Fitness Evaluation**: Agents are evaluated based on food collected, lifetime, and distance traveled.
- **Real-time Simulation**: Visualized using SDL2, with interactive controls and statistics.
- **Persistence**: Best-performing agent genes can be saved/loaded for future runs.
- **Significant Enhancements**: The C++ version includes new features, optimizations, and architectural changes not present in the original Python version.

## Technologies
- **C++17 or newer**
- **SDL2** for graphics and input handling
- **SDL2_ttf** for font rendering
- **Standard C++ STL** for data structures and algorithms
- (Optional) **OpenMP** or C++ threads for parallelism

## Project Structure
- `main.cpp`         : Entry point, main game loop, SDL2 setup
- `Game.h/cpp`       : Game management, world state, population control
- `Player.h/cpp`     : Player/agent logic, neural network, genetic operations
- `Food.h/cpp`       : Food entity
- `Hunter.h/cpp`     : Hunter entity (predators)
- `Settings.h`       : Configuration and constants
- `CMakeLists.txt`   : CMake build configuration
- `assets/`          : (If needed) Images, fonts, etc.

## Build Instructions
### Prerequisites
- **SDL2** and **SDL2_ttf** libraries must be installed on your system.
  - See [SDL2 documentation](https://wiki.libsdl.org/Installation) and [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf).
- **CMake** (recommended) or a C++17-compatible compiler (e.g., g++, clang++)

### Building with CMake (Recommended)
```sh
cd cpp_port
mkdir build
cd build
cmake ..
cmake --build .
```
This will produce an executable (e.g., `AI_Simulation_CPP`).

### Building with g++ (Manual)
```sh
g++ -std=c++17 main.cpp Game.cpp Player.cpp Food.cpp Hunter.cpp -lSDL2 -lSDL2_ttf -o simulation
```

## Running the Simulation
- Run the executable from the `build` directory (CMake) or from the source directory (g++):
  ```sh
  ./AI_Simulation_CPP
  # or
  ./simulation
  ```
- Make sure the font file (e.g., `arial.ttf`) is available in the working directory for SDL2_ttf.

## Controls & Usage
- **SPACE**: Start simulation
- **ESC**: Pause/Resume simulation or exit settings
- **S**: Open settings menu
- **R**: Restart simulation
- **UP/DOWN**: Adjust simulation speed
- **Close window**: Exit

## How It Works
- Agents (Players) move, eat food, avoid hunters, and reproduce via mitosis.
- Each agent's neural network receives sensory inputs (distances, angles, wall proximity, etc.) and outputs movement direction and speed.
- Genetic algorithm operations (mutation, crossover, selection) are applied to evolve better-performing agents over generations.
- Fitness is based on food collected, lifetime, and distance traveled.
- The best agent's genes are periodically saved to `bestgenes.txt`.

## Notes
- For best performance, avoid running in debug mode or with excessive graphics output.
- You can adjust simulation parameters in `Settings.h`.
- The project is designed for extensibility and experimentation with neuroevolution and agent-based AI.

---

**For any issues or questions, please refer to the code comments or contact the project maintainer.** 