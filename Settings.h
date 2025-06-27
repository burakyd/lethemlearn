#pragma once
#include <tuple>
#include <array>
#include <SDL.h>

// Color definitions: Use SDL_Color for all color usage
constexpr SDL_Color RED   = {255, 0, 0, 255};
constexpr SDL_Color GREEN = {0, 255, 0, 255};

// Display Settings
constexpr int SCREEN_WIDTH = 1024;
constexpr int SCREEN_HEIGHT = 768;
constexpr int SPEED = 2;

// Player Settings
constexpr int DOT_WIDTH = 10;
constexpr int DOT_HEIGHT = 10;
constexpr int RANDOM_SIZE_MIN = 5;
constexpr int RANDOM_SIZE_MAX = 15;
constexpr SDL_Color DOT_COLOR = GREEN;
constexpr float MAX_SPEED = 2.0f; // max speed of the players
constexpr int MAX_PLAYER_SIZE = 10 * DOT_WIDTH; // max player size
constexpr float PLAYER_MIN_SPEED_FACTOR = 0.5f; // lower limit for the speed penalty
constexpr float PLAYER_SIZE_SPEED_EXPONENT = 0.7f; // Lower = less penalty, 1.0 = linear, 0.5 = sqrt
constexpr float PLAYER_GROWTH_EXPONENT = 0.7f; // 0.5 = sqrt, 1.0 = linear, 0.7 = between

// Food Settings
constexpr int FOOD_WIDTH = 5;
constexpr int FOOD_HEIGHT = 5;
constexpr int FOOD_APPEND = 1; // reward for eating a food
constexpr float EATEN_FACTOR = 0.2f; // fraction of the eaten player's food awarded to the eater
constexpr int EATEN_ADD = 1; // additional reward for eating a player

// Game Mechanics
constexpr int KILL_TIME = 500;
constexpr int NUMBER_OF_FOODS = 40;
constexpr int HUNTERS = 5;
constexpr int HUNTER_WIDTH = 30;
constexpr int HUNTER_HEIGHT = 30;
constexpr SDL_Color HUNTER_COLOR = RED;
constexpr int MIN_BOT = 25;
constexpr bool KILL = true;

// Player Controls
constexpr bool PLAYER_ENABLED = true;

// Evolution Settings
constexpr int MITOSIS = 0; // 1/MITOSIS chance of mitosis - 0 means no mitosis occurs

// Neural Network Configuration
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

// Scaling factors for neural network inputs
// works like a gain layer and increases the response of agents to the environment
constexpr float SCALE_FOOD_DIST = 20.0f;
constexpr float SCALE_FOOD_ANGLE = 20.0f;
constexpr float SCALE_PLAYER_DIST = 12.0f;
constexpr float SCALE_PLAYER_ANGLE = 12.0f;
constexpr float SCALE_WALL = 2.0f;
constexpr float SCALE_SPEED = 2.0f;
constexpr float SCALE_SIZE_DIFF = 12.0f;
constexpr float SCALE_OWN_FOOD = 8.0f;
constexpr float SCALE_OWN_SIZE = 8.0f;

// Smoothing factor for temporal smoothing of NN inputs
constexpr float NN_INPUT_SMOOTHING_ALPHA = 0.2f;

// Spatial Partitioning
constexpr int GRID_CELL_SIZE = MAX_PLAYER_SIZE / 4;

// Genetic Algorithm Settings
constexpr int GENE_POOL_SIZE = 50;
constexpr float ELITISM_PERCENT = 0.1f;
constexpr float MUTATION_RATE = 0.1f;
constexpr int MUTATION_ATTEMPTS = 10;
constexpr float MUTATION_MAGNITUDE = 0.1f;
constexpr float LARGE_MUTATION_PROB = 0.05f;
constexpr float LARGE_MUTATION_SCALE = 5.0f;
constexpr int ADAPTIVE_MUTATION_PATIENCE = 1; // check for improvement in GENE_POOL_CHECK_INTERVAL's
constexpr float MAX_MUTATION_RATE = 1.0f;
constexpr float ADAPTIVE_MUTATION_FACTOR = 1.5f;
constexpr int GENE_POOL_CHECK_INTERVAL = 20000;
constexpr float PRUNE_RATE = 0.1f;

// Hunger/food decrease parameters
constexpr float HUNGER_BASE = 0.5f;
constexpr float HUNGER_SCALE = 0.005f;
constexpr float HUNGER_EXPONENT = 1.1f;
constexpr int HUNGER_MIN = 1;
constexpr int HUNGER_MAX = 3;

// Wall/corner penalty per time unit spent near the wall
constexpr float WALL_PENALTY_PER_FRAME = -0.2f;


// Fitness Settings (adjustable)

// Insert top N alive players into gene pool
constexpr int TOP_ALIVE_TO_INSERT = 3;

// Weights for Fitness Calculation
constexpr float FITNESS_WEIGHT_FOOD = 10.0f;
constexpr float FITNESS_WEIGHT_LIFE = 1.0f;
constexpr float FITNESS_WEIGHT_EXPLORE = 3.0f;
constexpr float FITNESS_WEIGHT_PLAYERS = 5.0f;
constexpr float FITNESS_MIN_FOOD = 5.0f;
constexpr float FITNESS_MIN_LIFE = 2000.0f;
constexpr float FITNESS_EARLY_DEATH_TIME = 1000.0f;
constexpr float FITNESS_EARLY_DEATH_PENALTY = 50.0f;
constexpr float FITNESS_MIN_FOR_REPRO = 5.0f;
constexpr float FITNESS_MIN_LIFETIME_FOR_REPRO = 2000.0f;
constexpr float FITNESS_DIVERSITY_PRUNE_MIN_DIST = 0.2f;
constexpr float MIN_FITNESS_FOR_GENE_POOL = 100.0f;
