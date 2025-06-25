#include "Player.h"
#include "Game.h"
#include <random>
#include <cmath>
#include <algorithm>
#include "Food.h"
#include "Hunter.h"
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include "Settings.h"
#include <vector>
#include <SDL.h>
extern int game_time_units;
class Food;
class Player;

namespace {
    float leaky_relu(float x) { return x > 0.0f ? x : 0.01f * x; }
    float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
    int toWASD(float v) { return v > 0.5f ? 1 : 0; }
}

Player::Player(int width, int height, std::array<int, 3> color, float x, float y, bool alive)
    : width(width), height(height), color(color), x(x), y(y), alive(alive), foodCount(0), lifeTime(0), killTime(0), foodScore(0), playerEaten(0), parent_id(-1)
{
    std::vector<int> layer_sizes = {NN_INPUTS, NN_H1, NN_H2, NN_H3, NN_OUTPUTS};
    genes.clear();
    biases.clear();
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        int in = layer_sizes[i], out = layer_sizes[i+1];
        std::vector<float> layer(in * out);
        std::vector<float> bias(out);
        float a = std::sqrt(6.0f / (in + out));
        std::generate(layer.begin(), layer.end(), [&]() { return ((float)rand() / RAND_MAX * 2 - 1) * a; });
        std::fill(bias.begin(), bias.end(), 0.0f);
        genes.push_back(layer);
        biases.push_back(bias);
    }
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    angle = angle_dist(gen);
    speed = MAX_SPEED;
}

Player::Player(const std::vector<std::vector<float>>& parent_genes, int width, int height, std::array<int, 3> color, float x, float y, int parent_id)
    : width(width), height(height), color(color), x(x), y(y), alive(true), foodCount(0), lifeTime(0), killTime(0), foodScore(0), playerEaten(0), parent_id(parent_id)
{
    genes = parent_genes;
    biases.resize(genes.size());
    for (size_t l = 0; l < biases.size(); ++l) {
        biases[l].resize(genes[l].size() / (l == genes.size() - 1 ? NN_H3 : (l == 0 ? NN_INPUTS : (l == 1 ? NN_H1 : NN_H2))));
        std::fill(biases[l].begin(), biases[l].end(), 0.0f);
    }
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * M_PI);
    angle = angle_dist(gen);
    speed = MAX_SPEED;
}

void Player::initialize_weights_xavier() {
    // Re-initialize genes with Xavier/Glorot uniform
    std::vector<int> layer_sizes = {NN_INPUTS, NN_H1, NN_H2, NN_H3, NN_OUTPUTS};
    genes.clear();
    biases.clear();
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        int in = layer_sizes[i], out = layer_sizes[i+1];
        std::vector<float> layer(in * out);
        std::vector<float> bias(out);
        float a = std::sqrt(6.0f / (in + out));
        std::generate(layer.begin(), layer.end(), [&]() { return ((float)rand() / RAND_MAX * 2 - 1) * a; });
        std::fill(bias.begin(), bias.end(), 0.0f);
        genes.push_back(layer);
        biases.push_back(bias);
    }
}

std::array<float, NN_OUTPUTS> Player::predict(const std::array<float, NN_INPUTS>& input) {
    std::vector<float> output(input.begin(), input.end());
    std::vector<int> layer_sizes = {NN_INPUTS, NN_H1, NN_H2, NN_H3, NN_OUTPUTS};
    for (size_t l = 0; l < genes.size(); ++l) {
        std::vector<float> next(layer_sizes[l+1], 0.0f);
        for (int j = 0; j < layer_sizes[l+1]; ++j) {
            for (int i = 0; i < layer_sizes[l]; ++i) {
                next[j] += output[i] * genes[l][i * layer_sizes[l+1] + j];
            }
            next[j] += biases[l][j];
            if (l < genes.size() - 1) next[j] = leaky_relu(next[j]);
            else {
                // Last layer: tanh for angle, sigmoid for speed
                if (j == 0) next[j] = std::tanh(next[j]); // angle
                else next[j] = sigmoid(next[j]); // speed
            }
        }
        output = next;
    }
    // Output[0]: desired angle in [-1,1] -> [0, 2pi] (absolute)
    // Output[1]: speed in [0, MAX_SPEED]
    std::array<float, NN_OUTPUTS> result{};
    // Add small random noise to angle for sensitivity
    float angle_noise = (((float)rand() / RAND_MAX) - 0.5f) * 0.2f; // noise in [-0.1, 0.1] radians
    result[0] = (output[0] + 1.0f) * M_PI + angle_noise; // [-1,1] -> [0,2pi] + noise
    result[1] = output[1] * MAX_SPEED;
    return result;
}

void Player::mutate(int nMutate) {
    for (int m = 0; m < nMutate; ++m) {
        int l = rand() % genes.size();
        int idx = rand() % genes[l].size();
        genes[l][idx] += ((float)rand() / RAND_MAX * 2 - 1) * 0.1f;
        // Mutate bias as well
        int bidx = rand() % biases[l].size();
        biases[l][bidx] += ((float)rand() / RAND_MAX * 2 - 1) * 0.1f;
        // Occasionally reset a weight or bias
        if (rand() % 20 == 0) {
            genes[l][idx] = ((float)rand() / RAND_MAX * 2 - 1) * 0.5f;
            biases[l][bidx] = ((float)rand() / RAND_MAX * 2 - 1) * 0.5f;
        }
    }
}

std::vector<std::vector<float>> Player::mitosis(bool mutate) {
    std::vector<std::vector<float>> new_genes = genes;
    if (mutate) {
        for (auto& layer : new_genes) {
            for (auto& w : layer) {
                w += ((float)rand() / RAND_MAX * 2 - 1) * 0.05f;
            }
        }
        for (auto& bias : biases) {
            for (auto& b : bias) {
                b += ((float)rand() / RAND_MAX * 2 - 1) * 0.05f;
            }
        }
    }
    return new_genes;
}

bool Player::collide(const Player& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    float r1 = (width + height) / 4.0f;
    float r2 = (other.width + other.height) / 4.0f;
    float threshold = 0.9f * (r1 + r2);
    float dist2 = dx * dx + dy * dy;
    return dist2 < threshold * threshold;
}

// Static variable definitions
int Player::food_to_size[MAX_FOOD + 1];
int Player::size_to_food[MAX_PLAYER_SIZE + 1];
bool Player::lookup_tables_initialized = false;

void Player::init_lookup_tables() {
    // Fill food_to_size
    for (int f = 0; f <= MAX_FOOD; ++f) {
        food_to_size[f] = DOT_WIDTH + int(FOOD_APPEND * std::pow(float(f), PLAYER_GROWTH_EXPONENT));
    }
    // Fill size_to_food
    for (int s = DOT_WIDTH; s <= MAX_PLAYER_SIZE; ++s) {
        // Find the smallest foodCount that gives at least this size
        for (int f = 0; f <= MAX_FOOD; ++f) {
            if (food_to_size[f] >= s) {
                size_to_food[s] = f;
                break;
            }
        }
    }
    lookup_tables_initialized = true;
}

void Player::update_size_from_food() {
    if (!lookup_tables_initialized) init_lookup_tables();
    int fc = std::max(0, std::min(foodCount, MAX_FOOD));
    width = food_to_size[fc];
    height = food_to_size[fc];
    if (width < DOT_WIDTH) width = DOT_WIDTH;
    if (height < DOT_HEIGHT) height = DOT_HEIGHT;
    if (width > MAX_PLAYER_SIZE) width = MAX_PLAYER_SIZE;
    if (height > MAX_PLAYER_SIZE) height = MAX_PLAYER_SIZE;
}

void Player::decrease_size_step() {
    if (!lookup_tables_initialized) init_lookup_tables();
    int current_width = width;
    int new_width = std::max(DOT_WIDTH, current_width - 1);
    int new_food = size_to_food[new_width];
    foodCount = new_food;
    update_size_from_food();
}

bool Player::eatPlayer(Game& game, Player& other) {
    if (!other.alive || &other == this) return false;
    if (collide(other) && height > other.height * 1.2f) {
        playerEaten++;
        killTime = 0;
        if (other.foodCount == 0) foodCount += EATEN_ADD;
        else foodCount += other.foodCount * EATEN_FACTOR + EATEN_ADD;
        update_size_from_food();
        other.alive = false;
        return true;
    }
    return false;
}

bool Player::eatFood(Game& game) {
    auto foods = game.get_nearby_food(x, y);
    for (auto* food : foods) {
        float dx = x - food->x;
        float dy = y - food->y;
        float r1 = (width + height) / 4.0f;
        float r2 = (food->width + food->height) / 4.0f;
        float threshold = 0.9f * (r1 + r2);
        float dist2 = dx * dx + dy * dy;
        if (dist2 < threshold * threshold) {
            foodCount++;
            foodScore++;
            killTime = 0;
            update_size_from_food();
            auto it = std::find(game.foods.begin(), game.foods.end(), food);
            if (it != game.foods.end()) {
                delete food;
                game.foods.erase(it);
                game.randomFood(1);
            }
            return true;
        }
    }
    return false;
}

NNInputsResult Player::get_nn_inputs(const Game& game) {
    // 0-1: Distance and direction to nearest food
    float min_food_dist = 1e6f, food_dx = 0, food_dy = 0;
    for (auto* food : game.foods) {
        float dx = food->x - x;
        float dy = food->y - y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist < min_food_dist) {
            min_food_dist = dist;
            food_dx = dx;
            food_dy = dy;
        }
    }
    float diag = std::sqrt(game.width*game.width + game.height*game.height);
    float food_dist_scaled = min_food_dist / diag; // [0, 1]
    food_dist_scaled = food_dist_scaled * 2.0f - 1.0f; // [-1, 1]
    float to_food_angle = std::atan2(food_dy, food_dx);
    float rel_food_angle = to_food_angle - angle;
    while (rel_food_angle < -M_PI) rel_food_angle += 2*M_PI;
    while (rel_food_angle > M_PI) rel_food_angle -= 2*M_PI;
    float rel_food_angle_scaled = rel_food_angle / M_PI; // [-1, 1]

    // 2: Distance and relative angle to nearest player (not self)
    float min_player_dist = 1e6f, player_dx = 0, player_dy = 0;
    int nearest_player_width = DOT_WIDTH;
    for (auto* p : game.players) {
        if (p == this || !p->alive) continue;
        float dx = p->x - x;
        float dy = p->y - y;
        float center_dist = std::sqrt(dx*dx + dy*dy);
        float r_self = (width + height) / 4.0f;
        float r_other = (p->width + p->height) / 4.0f;
        float edge_dist = center_dist - r_self - r_other;
        if (edge_dist < min_player_dist) {
            min_player_dist = edge_dist;
            player_dx = dx;
            player_dy = dy;
            nearest_player_width = p->width;
        }
    }
    float player_dist_scaled = min_player_dist / diag; // [0, 1]
    player_dist_scaled = player_dist_scaled * 2.0f - 1.0f; // [-1, 1]
    float to_player_angle = std::atan2(player_dy, player_dx);
    float rel_player_angle = to_player_angle - angle;
    while (rel_player_angle < -M_PI) rel_player_angle += 2*M_PI;
    while (rel_player_angle > M_PI) rel_player_angle -= 2*M_PI;
    float rel_player_angle_scaled = rel_player_angle / M_PI; // [-1, 1]

    // 8: Own size (normalized to default size)
    float size_norm = float(width) / float(DOT_WIDTH); // 1.0 = default size
    size_norm = std::max(-1.0f, std::min(1.0f, (size_norm - 1.0f) / (float(MAX_PLAYER_SIZE)/float(DOT_WIDTH) - 1.0f) * 2.0f - 1.0f));
    // 9: Own food count (normalized, clipped after 50)
    float food_count_norm = std::min(1.0f, float(foodCount) / 50.0f);
    food_count_norm = food_count_norm * 2.0f - 1.0f;
    // 10: Own normalized size (relative to max size)
    float own_norm_size = float(width) / float(MAX_PLAYER_SIZE); // [0,1]
    own_norm_size = own_norm_size * 2.0f - 1.0f;
    // 11: Own food count (again, for new input)
    float own_food_count = food_count_norm;
    // 10-13: Wall distances (left, right, top, bottom, normalized)
    float left_wall = float(x) / game.width;
    left_wall = left_wall * 2.0f - 1.0f;
    float right_wall = float(game.width - (x + width)) / game.width;
    right_wall = right_wall * 2.0f - 1.0f;
    float top_wall = float(y) / game.height;
    top_wall = top_wall * 2.0f - 1.0f;
    float bottom_wall = float(game.height - (y + height)) / game.height;
    bottom_wall = bottom_wall * 2.0f - 1.0f;

    float speed_scaled = speed / MAX_SPEED;
    speed_scaled = speed_scaled * 2.0f - 1.0f;
    // New input: size difference to nearest player (normalized)
    float size_diff = float(width - nearest_player_width) / float(DOT_WIDTH); // positive: bigger, negative: smaller
    size_diff = std::max(-1.0f, std::min(1.0f, size_diff));

    // Multiply all inputs by scale factor after normalization/scaling
    food_dist_scaled *= SCALE_FOOD_DIST;
    rel_food_angle_scaled *= SCALE_FOOD_ANGLE;
    player_dist_scaled *= SCALE_PLAYER_DIST;
    rel_player_angle_scaled *= SCALE_PLAYER_ANGLE;
    left_wall *= SCALE_WALL;
    right_wall *= SCALE_WALL;
    top_wall *= SCALE_WALL;
    bottom_wall *= SCALE_WALL;
    speed_scaled *= SCALE_SPEED;
    size_diff *= SCALE_SIZE_DIFF;
    own_norm_size *= SCALE_OWN_SIZE;
    own_food_count *= SCALE_OWN_FOOD;

    // Temporal smoothing (low-pass filter)
    // Used to decrease the effect of rapid changes in inputs to the outputs
    // Can be adjusted by alpha
    float alpha = NN_INPUT_SMOOTHING_ALPHA;
    this->smoothed_food_dist = alpha * food_dist_scaled + (1 - alpha) * this->smoothed_food_dist;
    this->smoothed_food_angle = alpha * rel_food_angle_scaled + (1 - alpha) * this->smoothed_food_angle;
    this->smoothed_player_dist = alpha * player_dist_scaled + (1 - alpha) * this->smoothed_player_dist;
    this->smoothed_player_angle = alpha * rel_player_angle_scaled + (1 - alpha) * this->smoothed_player_angle;
    this->smoothed_left_wall = alpha * left_wall + (1 - alpha) * this->smoothed_left_wall;
    this->smoothed_right_wall = alpha * right_wall + (1 - alpha) * this->smoothed_right_wall;
    this->smoothed_top_wall = alpha * top_wall + (1 - alpha) * this->smoothed_top_wall;
    this->smoothed_bottom_wall = alpha * bottom_wall + (1 - alpha) * this->smoothed_bottom_wall;
    this->smoothed_speed = alpha * speed_scaled + (1 - alpha) * this->smoothed_speed;
    this->smoothed_size_diff = alpha * size_diff + (1 - alpha) * this->smoothed_size_diff;
    // Add smoothing for new inputs
    this->smoothed_own_norm_size = alpha * own_norm_size + (1 - alpha) * this->smoothed_own_norm_size;
    this->smoothed_own_food_count = alpha * own_food_count + (1 - alpha) * this->smoothed_own_food_count;
    std::array<float, NN_INPUTS> inputs = {
        this->smoothed_food_dist, this->smoothed_food_angle,
        this->smoothed_player_dist, this->smoothed_player_angle,
        this->smoothed_left_wall, this->smoothed_right_wall, this->smoothed_top_wall, this->smoothed_bottom_wall,
        this->smoothed_speed,
        this->smoothed_size_diff,
        this->smoothed_own_norm_size,
        this->smoothed_own_food_count
    };
    NNInputsResult result;
    result.inputs = inputs;
    result.food_dx = food_dx;
    result.food_dy = food_dy;
    result.player_dx = player_dx;
    result.player_dy = player_dy;
    return result;
}

void Player::apply_nn_output(const std::array<float, NN_OUTPUTS>& nn_output) {
    // Absolute angle output: blend toward desired angle with max turn rate
    float desired_angle = nn_output[0]; // [0, 2pi]
    float angle_diff = desired_angle - angle;
    while (angle_diff < -M_PI) angle_diff += 2*M_PI;
    while (angle_diff > M_PI) angle_diff -= 2*M_PI;
    float max_turn = 1.0f; // radians per frame
    if (fabs(angle_diff) > max_turn)
        angle_diff = (angle_diff > 0 ? 1 : -1) * max_turn;
    angle += angle_diff;
    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;
    // Slow down with size growth
    float size_factor = std::pow(float(DOT_WIDTH) / float(width), PLAYER_SIZE_SPEED_EXPONENT);
    float min_speed_factor = PLAYER_MIN_SPEED_FACTOR; // Minimum speed factor, now tunable
    size_factor = std::max(size_factor, min_speed_factor);
    float effective_max_speed = MAX_SPEED * size_factor;
    speed = std::max(0.0f, std::min(nn_output[1], effective_max_speed));
}

void Player::clamp_to_screen(const Game& game) {
    if (x < width / 2.0f) x = width / 2.0f;
    if (y < height / 2.0f) y = height / 2.0f;
    if (x > game.width - width / 2.0f) x = game.width - width / 2.0f;
    if (y > game.height - height / 2.0f) y = game.height - height / 2.0f;
}

void Player::update_exploration_cell(int cell_size, int world_width, int world_height) {
    int cx = std::clamp(int(x) / cell_size, 0, world_width / cell_size - 1);
    int cy = std::clamp(int(y) / cell_size, 0, world_height / cell_size - 1);
    visited_cells.insert({cx, cy});
}

void Player::update(Game& game) {
    lifeTime++;
    killTime++;
    update_exploration_cell(Game::CELL_SIZE, game.width, game.height);
    if (killTime >= KILL_TIME) {
        killTime = 0;
        if (foodCount > 0) {
            // Improved, gentler hunger curve
            float base = HUNGER_BASE;
            float scale = HUNGER_SCALE;
            float exponent = HUNGER_EXPONENT;
            int food_loss = std::clamp(int(std::ceil(base + scale * std::pow(width, exponent))), HUNGER_MIN, HUNGER_MAX);
            for (int i = 0; i < food_loss && foodCount > 0; ++i) {
                decrease_size_step();
            }
        } else if (KILL) {
            alive = false;
        }
    }
    if (!alive) return;
    if (MITOSIS > 0 && foodCount >= 2 && rand() % MITOSIS == 0) {
        int child_food = foodCount / 2;
        std::vector<std::vector<float>> child_genes1 = mitosis(true);
        std::vector<std::vector<float>> child_genes2 = mitosis(true);
        Player* child1 = new Player(child_genes1, DOT_WIDTH + child_food * FOOD_APPEND, DOT_HEIGHT + child_food * FOOD_APPEND, color, x, y, parent_id);
        Player* child2 = new Player(child_genes2, DOT_WIDTH + child_food * FOOD_APPEND, DOT_HEIGHT + child_food * FOOD_APPEND, color, x, y, parent_id);
        child1->foodCount = child_food;
        child2->foodCount = child_food;
        child1->update_size_from_food();
        child2->update_size_from_food();
        game.players.push_back(child1);
        game.players.push_back(child2);
        alive = false;
        return;
    }
    NNInputsResult nn_result = get_nn_inputs(game);
    auto nn_output = predict(nn_result.inputs);
    apply_nn_output(nn_output);
    float old_x = x;
    float old_y = y;
    x += std::cos(angle) * speed;
    y += std::sin(angle) * speed;
    distance_traveled += std::sqrt((x - old_x) * (x - old_x) + (y - old_y) * (y - old_y));
    clamp_to_screen(game);
    eatFood(game);
    auto nearby_players = game.get_nearby_players(x, y);
    for (auto* other : nearby_players) {
        if (other->alive && other != this) {
            eatPlayer(game, *other);
        }
    }
    last_angle = angle;
    last_speed = speed;
    float to_food_angle = std::atan2(last_nn_food_dy, last_nn_food_dx);
    float rel_food_angle = to_food_angle - angle;
    while (rel_food_angle < -M_PI) rel_food_angle += 2*M_PI;
    while (rel_food_angle > M_PI) rel_food_angle -= 2*M_PI;
    last_rel_food_angle = rel_food_angle;
    float to_player_angle = std::atan2(last_nn_player_dy, last_nn_player_dx);
    float rel_player_angle = to_player_angle - angle;
    while (rel_player_angle < -M_PI) rel_player_angle += 2*M_PI;
    while (rel_player_angle > M_PI) rel_player_angle -= 2*M_PI;
    last_rel_player_angle = rel_player_angle;
    // Track frames spent near wall/corner (size-aware)
    if (x - width / 2 < 20 || x + width / 2 > game.width - 20 ||
        y - height / 2 < 20 || y + height / 2 > game.height - 20) {
        time_near_wall++;
    }
}

void Player::draw(SDL_Renderer* renderer) {
    SDL_Rect rect = {static_cast<int>(x - width / 2.0f), static_cast<int>(y - height / 2.0f), width, height};
    SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
    SDL_RenderFillRect(renderer, &rect);
    // Draw direction arrow (half as long, with arrowhead)
    float cx = x;
    float cy = y;
    float len = (10.0f + 10.0f * (speed / MAX_SPEED)) * 0.5f;
    float ex = cx + std::cos(angle) * len;
    float ey = cy + std::sin(angle) * len;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawLine(renderer, static_cast<int>(cx), static_cast<int>(cy), static_cast<int>(ex), static_cast<int>(ey));
    // Arrowhead
    float head_len = len * 0.5f;
    float head_angle = 0.5f; // radians, ~28 degrees
    float left_x = ex - std::cos(angle - head_angle) * head_len;
    float left_y = ey - std::sin(angle - head_angle) * head_len;
    float right_x = ex - std::cos(angle + head_angle) * head_len;
    float right_y = ey - std::sin(angle + head_angle) * head_len;
    SDL_RenderDrawLine(renderer, static_cast<int>(ex), static_cast<int>(ey), static_cast<int>(left_x), static_cast<int>(left_y));
    SDL_RenderDrawLine(renderer, static_cast<int>(ex), static_cast<int>(ey), static_cast<int>(right_x), static_cast<int>(right_y));
}

// Improved crossover: uniform, single-point, and arithmetic crossover for more diversity
std::vector<std::vector<float>> crossover(const std::vector<std::vector<float>>& g1, const std::vector<std::vector<float>>& g2) {
    std::vector<std::vector<float>> result = g1;
    for (size_t l = 0; l < g1.size(); ++l) {
        int size = g1[l].size();
        int method = rand() % 3; // 0: uniform, 1: single-point, 2: arithmetic
        if (method == 0) { // Uniform crossover
            for (int i = 0; i < size; ++i) {
                result[l][i] = (rand() % 2 == 0) ? g1[l][i] : g2[l][i];
            }
        } else if (method == 1) { // Single-point crossover
            int point = rand() % size;
            for (int i = 0; i < size; ++i) {
                result[l][i] = (i < point) ? g1[l][i] : g2[l][i];
            }
        } else { // Arithmetic crossover
            float alpha = ((float)rand() / RAND_MAX);
            for (int i = 0; i < size; ++i) {
                result[l][i] = alpha * g1[l][i] + (1.0f - alpha) * g2[l][i];
            }
        }
    }
    return result;
}

// Improved mutation: larger, rarer mutations and occasional full randomization
void mutate_genes(std::vector<std::vector<float>>& genes, int nMutate) {
    for (int m = 0; m < nMutate; ++m) {
        int l = rand() % genes.size();
        int idx = rand() % genes[l].size();
        float noise = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;
        // 5% chance for a large mutation
        if (rand() % 20 == 0) noise *= 5.0f;
        genes[l][idx] += noise;
        // 1% chance for full randomization
        if (rand() % 100 == 0) genes[l][idx] = ((float)rand() / RAND_MAX * 2 - 1) * 0.5f;
    }
}

float Player::get_hunger() const {
    return std::min(1.0f, float(killTime) / float(KILL_TIME));
}

float Player::get_random_input() const {
    static thread_local std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    static thread_local std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    return dist(rng);
}

// --- Gene Pool System ---
std::vector<Player::GeneEntry> Player::gene_pool;

void Player::try_insert_gene_to_pool(float fitness, const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases) {
    // Prevent human players from being added to the gene pool
    // This function is static, so we can't check an instance, but the call site is now protected.
    // Defensive: if genes or biases are empty, do nothing (could indicate human or error)
    if (genes.empty() || biases.empty()) return;
    // Insert if pool not full or fitness is better than the worst
    if (gene_pool.size() < GENE_POOL_SIZE) {
        gene_pool.push_back({fitness, genes, biases});
        // Update Hall of Fame
        update_hall_of_fame(fitness, genes, biases);
        // Compute average fitness
        float avg_fitness = 0.0f;
        for (const auto& entry : gene_pool) avg_fitness += entry.fitness;
        avg_fitness /= gene_pool.size();
        std::cout << "Inserted fitness: " << fitness << ", game time(k): " << (int)(game_time_units/1000)
                  << ", avg fitness: " << avg_fitness << std::endl;
    } else {
        auto min_it = std::min_element(gene_pool.begin(), gene_pool.end(), [](const GeneEntry& a, const GeneEntry& b) { return a.fitness < b.fitness; });
        if (fitness > min_it->fitness) {
            *min_it = {fitness, genes, biases};
            // Update Hall of Fame
            update_hall_of_fame(fitness, genes, biases);
            // Compute average fitness
            float avg_fitness = 0.0f;
            for (const auto& entry : gene_pool) avg_fitness += entry.fitness;
            avg_fitness /= gene_pool.size();
            std::cout << "Replaced worst with fitness: " << fitness << ", game time(k): " << (int)(game_time_units/1000)
                      << ", avg fitness: " << avg_fitness << std::endl;
        } else {
            return;
        }
    }
    // Keep pool sorted descending
    std::sort(gene_pool.begin(), gene_pool.end(), [](const GeneEntry& a, const GeneEntry& b) { return a.fitness > b.fitness; });
}

void Player::save_gene_pool(const std::string& filename) {
    std::ofstream ofs(filename);
    ofs << "GENE_POOL\n";
    for (const auto& entry : gene_pool) {
        ofs << "FITNESS " << entry.fitness << "\n";
        ofs << "GENES\n";
        for (const auto& layer : entry.genes) {
            for (float w : layer) ofs << w << ' ';
            ofs << '\n';
        }
        ofs << "BIASES\n";
        for (const auto& bias : entry.biases) {
            for (float b : bias) ofs << b << ' ';
            ofs << '\n';
        }
        ofs << "END\n";
    }
}

void Player::load_gene_pool(const std::string& filename) {
    gene_pool.clear();
    std::ifstream ifs(filename);
    if (!ifs) return;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line == "FITNESS " || line.rfind("FITNESS ", 0) == 0) {
            float fitness = std::stof(line.substr(8));
            std::getline(ifs, line); // GENES
            std::vector<std::vector<float>> genes;
            for (int l = 0; l < 4; ++l) {
                std::getline(ifs, line);
                std::istringstream iss(line);
                std::vector<float> layer;
                float w;
                while (iss >> w) layer.push_back(w);
                genes.push_back(layer);
            }
            std::getline(ifs, line); // BIASES
            std::vector<std::vector<float>> biases;
            for (int l = 0; l < 4; ++l) {
                std::getline(ifs, line);
                std::istringstream iss(line);
                std::vector<float> bias;
                float b;
                while (iss >> b) bias.push_back(b);
                biases.push_back(bias);
            }
            std::getline(ifs, line); // END
            gene_pool.push_back({fitness, genes, biases});
        }
    }
    // Keep pool sorted descending
    std::sort(gene_pool.begin(), gene_pool.end(), [](const GeneEntry& a, const GeneEntry& b) { return a.fitness > b.fitness; });
}

Player::GeneEntry Player::sample_gene_from_pool() {
    if (gene_pool.empty()) throw std::runtime_error("Gene pool is empty");
    int idx = rand() % gene_pool.size();
    return gene_pool[idx];
}

HumanPlayer::HumanPlayer(int width, int height, std::array<int, 3> color, float x, float y, bool alive)
    : Player(width, height, color, x, y, alive)
{
    is_human = true;
}

void HumanPlayer::update(Game& game) {
    lifeTime++;
    killTime++;
    update_exploration_cell(Game::CELL_SIZE, game.width, game.height);
    if (killTime >= KILL_TIME) {
        killTime = 0;
        if (foodCount > 0) {
            int food_loss = std::ceil(1 + 0.05 * std::sqrt(width));
            for (int i = 0; i < food_loss && foodCount > 0; ++i) {
                decrease_size_step();
            }
        } else if (KILL) {
            alive = false;
        }
    }
    if (!alive) return;
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    float dx = mouse_x - x;
    float dy = mouse_y - y;
    float dist = std::sqrt(dx * dx + dy * dy);
    // decrease size - curved
    float size_factor = std::pow(float(DOT_WIDTH) / float(width), PLAYER_SIZE_SPEED_EXPONENT);
    size_factor = std::max(size_factor, PLAYER_MIN_SPEED_FACTOR);
    float effective_max_speed = MAX_SPEED * size_factor;
    float move_speed = effective_max_speed * (1.0f - std::exp(-dist / 50.0f));
    float move_x = 0.0f, move_y = 0.0f;
    if (dist > 1.0f) { // Only move if not very close
        move_x = (dx / dist) * move_speed;
        move_y = (dy / dist) * move_speed;
        // Clamp movement if closer than move_speed
        if (dist < move_speed) {
            move_x = dx;
            move_y = dy;
        }
        x += move_x;
        y += move_y;
        angle = std::atan2(move_y, move_x);
        speed = move_speed;
    } else {
        speed = 0.0f;
    }
    clamp_to_screen(game);
    eatFood(game);
    auto nearby_players = game.get_nearby_players(x, y);
    for (auto* other : nearby_players) {
        if (other->alive && other != this) {
            eatPlayer(game, *other);
        }
    }
    last_angle = angle;
    last_speed = speed;
}

// --- Crossover for biases ---
std::vector<std::vector<float>> crossover_biases(const std::vector<std::vector<float>>& b1, const std::vector<std::vector<float>>& b2) {
    std::vector<std::vector<float>> result = b1;
    for (size_t l = 0; l < b1.size(); ++l) {
        int size = b1[l].size();
        int method = rand() % 3; // 0: uniform, 1: single-point, 2: arithmetic
        if (method == 0) { // Uniform crossover
            for (int i = 0; i < size; ++i) {
                result[l][i] = (rand() % 2 == 0) ? b1[l][i] : b2[l][i];
            }
        } else if (method == 1) { // Single-point crossover
            int point = rand() % size;
            for (int i = 0; i < size; ++i) {
                result[l][i] = (i < point) ? b1[l][i] : b2[l][i];
            }
        } else { // Arithmetic crossover
            float alpha = ((float)rand() / RAND_MAX);
            for (int i = 0; i < size; ++i) {
                result[l][i] = alpha * b1[l][i] + (1.0f - alpha) * b2[l][i];
            }
        }
    }
    return result;
}

void mutate_biases(std::vector<std::vector<float>>& biases, int nMutate) {
    for (int m = 0; m < nMutate; ++m) {
        int l = rand() % biases.size();
        int idx = rand() % biases[l].size();
        float noise = ((float)rand() / RAND_MAX) * 0.2f - 0.1f;
        if (rand() % 20 == 0) noise *= 5.0f;
        biases[l][idx] += noise;
        if (rand() % 100 == 0) biases[l][idx] = ((float)rand() / RAND_MAX * 2 - 1) * 0.5f;
    }
}

// Hall of Fame for all-time best genes
std::vector<Player::GeneEntry> Player::hall_of_fame;

void Player::update_hall_of_fame(float fitness, const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases) {
    // Insert if not full, or replace worst if better
    if (hall_of_fame.size() < HALL_OF_FAME_SIZE) {
        hall_of_fame.push_back({fitness, genes, biases});
    } else {
        auto min_it = std::min_element(hall_of_fame.begin(), hall_of_fame.end(), [](const GeneEntry& a, const GeneEntry& b) { return a.fitness < b.fitness; });
        if (fitness > min_it->fitness) {
            *min_it = {fitness, genes, biases};
        } else {
            return;
        }
    }
    std::sort(hall_of_fame.begin(), hall_of_fame.end(), [](const GeneEntry& a, const GeneEntry& b) { return a.fitness > b.fitness; });
}

Player::GeneEntry Player::sample_hall_of_fame() {
    if (hall_of_fame.empty()) throw std::runtime_error("Hall of Fame is empty");
    int idx = rand() % hall_of_fame.size();
    return hall_of_fame[idx];
} 