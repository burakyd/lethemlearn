#pragma once
#include <tuple>
#include <array>

// Color definitions (RGB)
constexpr std::array<int, 3> BLACK = {0, 0, 0};
constexpr std::array<int, 3> RED   = {255, 0, 0};
constexpr std::array<int, 3> GREEN = {0, 255, 0};
constexpr std::array<int, 3> BLUE  = {0, 0, 255};
constexpr std::array<int, 3> WHITE = {255, 255, 255};

// Display Settings
constexpr int SCREEN_WIDTH = 1024;
constexpr int SCREEN_HEIGHT = 768;
constexpr int FRAME_RATE = 60;
constexpr int SPEED = 2;

// Player Settings
constexpr int DOT_WIDTH = 10;
constexpr int DOT_HEIGHT = 10;
constexpr int RANDOM_SIZE_MIN = 5;
constexpr int RANDOM_SIZE_MAX = 15;
constexpr std::array<int, 3> DOT_COLOR = GREEN;
constexpr float MAX_SPEED = 2.0f; // max speed of the players
constexpr int MAX_PLAYER_SIZE = 10 * DOT_WIDTH; // max player size
// speed penalty over growth
constexpr float PLAYER_MIN_SPEED_FACTOR = 0.5f; // lower limit for the speed penalty
constexpr float PLAYER_SIZE_SPEED_EXPONENT = 0.7f; // Lower = less penalty, 1.0 = linear, 0.5 = sqrt

// Food Settings
constexpr int FOOD_WIDTH = 5;
constexpr int FOOD_HEIGHT = 5;
constexpr int FOOD_APPEND = 1; // reward for eating a food
constexpr int EATEN_ADD = 1; // additional reward for eating a player

// Game Mechanics
constexpr int KILL_TIME = 500;
constexpr int NUMBER_OF_FOODS = 40;
constexpr int BOTS = 50;
constexpr int HUNTERS = 5;
constexpr int HUNTER_WIDTH = 30;
constexpr int HUNTER_HEIGHT = 30;
constexpr std::array<int, 3> HUNTER_COLOR = RED;
constexpr int MIN_BOT = 30;
constexpr bool ADD_NEW_WHEN_DIE = true;
constexpr bool KILL = true;

// Player Controls - for fun
constexpr bool PLAYER_ENABLED = true;

// Evolution Settings
constexpr int MITOSIS = 15000; // 1/MITOSIS chance of mitosis
constexpr bool DO_MUTATE = true;
constexpr int NUMBER_OF_MUTATES = 3;

// Neural Network Configuration
// Hidden layers can be adjusted or another layer can be added
// Don't change the input and output layers, unless it is for a logic change.
constexpr int NN_INPUTS = 12;
constexpr int NN_H1 = 12;
constexpr int NN_H2 = 12;
constexpr int NN_H3 = 12;
constexpr int NN_OUTPUTS = 2;

constexpr std::array<std::tuple<int, int>, 4> NEURAL_NET_SHAPE = {
    std::make_tuple(NN_INPUTS, NN_H1),
    std::make_tuple(NN_H1, NN_H2),
    std::make_tuple(NN_H2, NN_H3),
    std::make_tuple(NN_H3, NN_OUTPUTS)
};

// This is done after normalization to [-1,1], so the players are more sensitive to environmental changes
// Can be individually changed to increase/decrease the effect of a feature
// Works like a Gain Layer
constexpr float SCALE_FOOD_DIST = 10.0f;
constexpr float SCALE_FOOD_ANGLE = 10.0f;
constexpr float SCALE_PLAYER_DIST = 6.0f;
constexpr float SCALE_PLAYER_ANGLE = 6.0f;
constexpr float SCALE_WALL = 1.0f;
constexpr float SCALE_SPEED = 1.0f;
constexpr float SCALE_SIZE_DIFF = 6.0f;
constexpr float SCALE_OWN_FOOD = 4.0f;
constexpr float SCALE_OWN_SIZE = 4.0f;

// Smoothing factor for temporal smoothing of NN inputs
// 0 means no change in inputs, 1 means instant change to the new value
// Helps with more natural behavior, especially for rapid changes in the inputs
constexpr float NN_INPUT_SMOOTHING_ALPHA = 0.2f;

// Gene Saving
// When a player dies, their fitness and genes are considered for the pool.
// If their fitness is higher than the lowest in the pool (or if the pool isn't full), they are added.
// The pool is periodically saved to gene_pool.txt and loaded at startup.
constexpr int SAVE_GENES = 20;
constexpr bool SUPER_PLAYER = true; // add new players from the save file, if exists

// Spatial Partitioning
// Important for scaling for more than 50-100 players
constexpr int GRID_CELL_SIZE = MAX_PLAYER_SIZE / 4;

constexpr bool CACHE_NEARBY_ENTITIES = true;

// Genetic Algorithm Settings
constexpr int GENE_POOL_SIZE = 100;
constexpr float ELITISM_PERCENT = 0.1f; // top 10% as elites
constexpr float MUTATION_RATE = 0.1f; // 10% mutation rate
constexpr float LARGE_MUTATION_RATE = 0.02f; // 2% large mutation
constexpr float MIN_FITNESS_FOR_GENE_POOL = 100.0f;
constexpr int GENE_POOL_PRUNE_INTERVAL = 10; // generations

// Hunger/food decrease parameters
constexpr float HUNGER_BASE = 0.5f;           // Minimum food loss per interval
constexpr float HUNGER_SCALE = 0.005f;        // Scale for size-based food loss
constexpr float HUNGER_EXPONENT = 1.1f;       // Exponent for sublinear growth
constexpr int HUNGER_MIN = 1;                 // Minimum food loss per interval
constexpr int HUNGER_MAX = 3;                 // Maximum food loss per interval
