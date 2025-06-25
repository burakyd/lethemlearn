# C++ Evolutionary AI Simulation

## Table of Contents
- [Overview](#overview)
- [How It Works](#how-it-works)
- [Core Components](#core-components)
  - [Neural Network Architecture](#neural-network-architecture)
  - [Genetic Algorithm](#genetic-algorithm)
  - [Agent (Player) Behavior](#agent-player-behavior)
  - [Hunter (Predator) Behavior](#hunter-predator-behavior)
  - [Food Entity](#food-entity)
- [Simulation Controls & UI](#simulation-controls--ui)
- [Configuration & Extensibility](#configuration--extensibility)
- [Project Structure](#project-structure)
- [Build & Run Instructions](#build--run-instructions)
- [Troubleshooting & Tips](#troubleshooting--tips)
- [License & Contact](#license--contact)

---

## Overview
This project is a C++17+ agent-based AI simulation, originally ported from a Python/Pygame prototype, now significantly enhanced and architected for performance and extensibility. It demonstrates emergent intelligent behavior through **neuroevolution**: a population of agents ("Players") controlled by neural networks, evolved using genetic algorithms to maximize survival and food collection in a dynamic 2D world. The simulation is visualized in real-time using SDL2.

Key features include:
- **Neuroevolution**: Agents learn to survive, collect food, and avoid predators via evolving neural networks.
- **Genetic Algorithm**: Implements mutation, crossover (uniform, single-point, arithmetic), elitism, and gene pool management.
- **Real-time Visualization**: Interactive controls, statistics, and settings menu.
- **Persistence**: Save/load best-performing agent genes for future runs.
- **Extensibility**: Modular codebase for experimenting with AI, evolution, and agent-based systems.

---

## How It Works
- **Agents (Players)** move, eat food, avoid hunters, and reproduce via mitosis.
- Each agent's neural network receives sensory inputs (distances, angles, wall proximity, etc.) and outputs movement direction and speed.
- **Genetic algorithm** operations (mutation, crossover, selection) are applied to evolve better-performing agents over generations.
- **Fitness** is based on food collected, lifetime, distance traveled, exploration, and penalties for wall-camping or early death.
- The best agent's genes are periodically saved to `gene_pool.txt`.
- **Hunters** act as predators, targeting and eliminating weaker agents, increasing evolutionary pressure.

---

## Core Components

### Neural Network Architecture
- Each agent is controlled by a feedforward neural network with the following structure (configurable in `Settings.h`):
  - **Inputs (12):**
    - Distance and angle to nearest food
    - Distance and angle to nearest player
    - Wall proximity (left, right, top, bottom)
    - Own size (normalized)
    - Own food count (normalized)
    - Speed
    - Size difference to nearest player
  - **Hidden Layers:** 3 layers, each with 12 neurons (default)
  - **Outputs (2):**
    - Desired movement angle (absolute, mapped from [-1,1] to [0,2π])
    - Speed (scaled to [0, MAX_SPEED])
- Weights and biases are evolved, not learned via backpropagation.
- Activation functions: Leaky ReLU (hidden), tanh/sigmoid (output)
- Xavier/Glorot initialization for weights
- Temporal smoothing is applied to inputs for more natural behavior.

### Genetic Algorithm
- **Population Management:**
  - Maintains a pool of agents (bots), with a minimum and maximum size.
  - Dead agents are removed; new agents are created via cloning, crossover, or random initialization.
- **Selection & Elitism:**
  - Top-performing agents (elites) are cloned directly.
  - Tournament selection and random injection maintain diversity.
- **Crossover:**
  - Uniform, single-point, and arithmetic crossover methods are used to combine genes.
- **Mutation:**
  - Small random changes to weights/biases; occasional large mutations and rare full randomization.
- **Gene Pool:**
  - Best genes are stored in a pool, periodically pruned and saved to disk.
  - New agents can be initialized from saved genes for continuity.
- **Fitness Function:**
  - Weighted sum of food collected, lifetime, distance traveled, size, exploration, and penalties for wall-camping or early death.

### Agent (Player) Behavior
- **Senses:**
  - Detects food, other players, hunters, and walls.
- **Actions:**
  - Moves, eats food, can eat smaller players, grows with food, shrinks with hunger.
  - Reproduces via mitosis (chance-based split, creating a mutated offspring).
- **Exploration:**
  - Tracks visited cells for exploration bonus.
- **Survival:**
  - Must avoid hunters and larger players, seek food, and avoid wall-camping.

### Hunter (Predator) Behavior
- **Role:**
  - Hunters are auto-controlled predators that target and eliminate weaker agents, increasing evolutionary pressure.
- **Behavior:**
  - Seek out the nearest player they can eat (must be larger than the target).
  - Add noise to movement for unpredictability.
  - Do not grow or mutate; act as a constant threat.
  - If all players are targeted, fallback to nearest edible player.
- **Implementation:**
  - Inherits from Player, but overrides update logic and cannot eat food.

### Food Entity
- **Role:**
  - Static objects that agents must collect to survive and grow.
- **Spawning:**
  - Randomly placed, avoiding overlap with players and other food.

---

## Simulation Controls & UI
- **SPACE**: Start simulation
- **ESC**: Pause/Resume simulation or exit settings
- **S**: Open settings menu
- **R**: Restart simulation
- **UP/DOWN**: Adjust simulation speed
- **B/N**: Increase/decrease bot count
- **F/G**: Increase/decrease food count
- **H**: Toggle hunters
- **J/K**: Increase/decrease hunter count
- **P**: Toggle human player
- **Close window**: Exit
- **Settings Menu:**
  - Adjust population, food, hunter settings, and simulation speed interactively.

---

## Configuration & Extensibility
- All major parameters are defined in `Settings.h`, including:
  - **Display:** Screen size, frame rate
  - **Player:** Size, speed, growth curve, color
  - **Food:** Size, reward, spawn count
  - **Hunters:** Count, size, speed, color
  - **Neural Network:** Layer sizes, input/output count, scaling factors, smoothing
  - **Genetic Algorithm:** Mutation rates, elitism, gene pool size, fitness thresholds
  - **Game Mechanics:** Hunger, wall penalties, mitosis chance
- **Spatial Partitioning:**
  - Grid-based partitioning for efficient collision and neighbor queries (scales to 100+ agents)
- **Persistence:**
  - Gene pool is saved/loaded from `gene_pool.txt` for continuity and experimentation.
- **Extending the Simulation:**
  - Add new agent types, sensors, or actions by extending `Player` or `Hunter` classes.
  - Modify fitness function or neural network structure in `Settings.h` and `Player.cpp`.

---

## Project Structure
- `main.cpp`         : Entry point, main game loop, SDL2 setup
- `Game.h/cpp`       : Game management, world state, population control, genetic algorithm
- `Player.h/cpp`     : Player/agent logic, neural network, genetic operations, gene pool
- `Hunter.h/cpp`     : Hunter (predator) logic, overrides for predatory behavior
- `Food.h/cpp`       : Food entity
- `Settings.h`       : All configuration and constants
- `GameApp.h/cpp`    : SDL2 application, UI, settings menu, rendering
- `assets/`          : (If needed) Images, fonts, etc.

---

## Build & Run Instructions
### Prerequisites
- **SDL2** and **SDL2_ttf** libraries must be installed on your system.
  - See [SDL2 documentation](https://wiki.libsdl.org/Installation) and [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf).
- **CMake** (recommended) or a C++17-compatible compiler (e.g., g++, clang++)

### Building with CMake (Recommended)
```sh
cd AIM_cpp
mkdir build
cd build
cmake ..
cmake --build .
```
This will produce an executable (e.g., `AI_Simulation_CPP`).

### Building with g++ (Manual)
```sh
g++ -std=c++17 main.cpp Game.cpp Player.cpp Food.cpp Hunter.cpp GameApp.cpp -lSDL2 -lSDL2_ttf -o simulation
```

### Running the Simulation
- Run the executable from the `build` directory (CMake) or from the source directory (g++):
  ```sh
  ./AI_Simulation_CPP
  # or
  ./simulation
  ```
- Make sure the font file (e.g., `arial.ttf`) is available in the working directory for SDL2_ttf.

---

## Troubleshooting & Tips
- For best performance, avoid running in debug mode or with excessive graphics output.
- Adjust simulation parameters in `Settings.h` for experimentation.
- If the simulation runs slowly, reduce the number of agents or food.
- The project is designed for extensibility—try modifying the neural network, fitness function, or agent behaviors!
- If you encounter issues with SDL2 or SDL2_ttf, ensure they are correctly installed and available in your system's library path.

---

## License & Contact
This project is open for educational and research purposes. For questions, suggestions, or contributions, please refer to the code comments or contact the project maintainer.

---

Enjoy experimenting with evolutionary AI in C++! 