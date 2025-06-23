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

// Food Settings
constexpr int FOOD_WIDTH = 5;
constexpr int FOOD_HEIGHT = 5;
constexpr int FOOD_APPEND = 1; // reward for eating a food
constexpr int EATEN_ADD = 0; // additional reward for eating a player

// Game Mechanics
constexpr int KILL_TIME = 500;
constexpr int NUMBER_OF_FOODS = 25;
constexpr int BOTS = 50;
constexpr int HUNTERS = 5;
constexpr int HUNTER_WIDTH = 25;
constexpr int HUNTER_HEIGHT = 25;
constexpr std::array<int, 3> HUNTER_COLOR = RED;
constexpr int MIN_BOT = 25;
constexpr bool ADD_NEW_WHEN_DIE = true;
constexpr bool KILL = true;

// Player Controls - for fun
constexpr bool PLAYER_ENABLED = true; // not truly implemented yet

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
constexpr float SCALE_FOOD_DIST = 50.0f;
constexpr float SCALE_FOOD_ANGLE = 50.0f;
constexpr float SCALE_PLAYER_DIST = 30.0f;
constexpr float SCALE_PLAYER_ANGLE = 30.0f;
constexpr float SCALE_WALL = 10.0f;
constexpr float SCALE_SPEED = 10.0f;
constexpr float SCALE_SIZE_DIFF = 30.0f;
constexpr float SCALE_OWN_FOOD = 20.0f;
constexpr float SCALE_OWN_SIZE = 20.0f;

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

// Performance Settings
constexpr bool SPATIAL_PARTITIONING = true;
constexpr int OPTIMIZATION_LEVEL = 2;
constexpr bool CACHE_NEARBY_ENTITIES = true;
