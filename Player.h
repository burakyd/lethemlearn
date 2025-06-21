#pragma once
#include <vector>
#include <array>
#include "Settings.h"
#include <SDL.h>
#include <random>
#include <memory>
class Game;

// Helper struct for NN input and dx/dy values
struct NNInputsResult {
    std::array<float, NN_INPUTS> inputs; // Now includes size difference to nearest player as last input
    float food_dx, food_dy, hunter_dx, hunter_dy, player_dx, player_dy;
};

class Player {
public:
    Player(int width = DOT_WIDTH, int height = DOT_HEIGHT, std::array<int, 3> color = DOT_COLOR, float x = 0, float y = 0, bool alive = true);
    Player(const std::vector<std::vector<float>>& parent_genes, int width, int height, std::array<int, 3> color, float x, float y, int parent_id = -1);
    virtual void update(Game& game);
    void mutate(int nMutate = NUMBER_OF_MUTATES);
    std::array<float, NN_OUTPUTS> predict(const std::array<float, NN_INPUTS>& input);
    std::vector<std::vector<float>> genes; // Neural net weights (per layer: weights)
    std::vector<std::vector<float>> biases; // Neural net biases (per layer: biases)
    std::vector<std::vector<float>> mitosis(bool mutate = true);
    bool collide(const Player& other) const;
    virtual bool eatPlayer(Game& game, Player& other);
    virtual bool eatFood(Game& game);
    virtual void draw(SDL_Renderer* renderer);
    float x, y;
    int width, height;
    std::array<int, 3> color;
    float speed;
    int foodCount, lifeTime, killTime, foodScore, playerEaten;
    bool alive;
    int parent_id;
    std::vector<std::vector<float>> shape;
    float angle; // direction in radians
    // For NN input: last state
    float last_angle = 0.0f;
    float last_speed = 0.0f;
    float last_rel_food_angle = 0.0f;
    float last_rel_hunter_angle = 0.0f;
    float last_rel_player_angle = 0.0f;
    // For storing last dx/dy for relative angle update
    float last_nn_food_dx = 0.0f, last_nn_food_dy = 0.0f;
    float last_nn_hunter_dx = 0.0f, last_nn_hunter_dy = 0.0f;
    float last_nn_player_dx = 0.0f, last_nn_player_dy = 0.0f;
    float distance_traveled = 0.0f;
    // For input smoothing (temporal smoothing)
    float smoothed_food_dist = 0.0f;
    float smoothed_food_angle = 0.0f;
    float smoothed_player_dist = 0.0f;
    float smoothed_player_angle = 0.0f;
    float smoothed_left_wall = 0.0f;
    float smoothed_right_wall = 0.0f;
    float smoothed_top_wall = 0.0f;
    float smoothed_bottom_wall = 0.0f;
    float smoothed_speed = 0.0f;
    float smoothed_size_diff = 0.0f;
    float smoothed_own_norm_size = 0.0f;
    float smoothed_own_food_count = 0.0f;
    void initialize_weights_xavier();
    NNInputsResult get_nn_inputs(const Game& game);
    void apply_nn_output(const std::array<float, NN_OUTPUTS>& nn_output);
    float get_hunger() const;
    float get_random_input() const;
    // --- Gene Pool System ---
    struct GeneEntry {
        float fitness;
        std::vector<std::vector<float>> genes;
        std::vector<std::vector<float>> biases;
    };
    static std::vector<GeneEntry> gene_pool;
    static constexpr int GENE_POOL_SIZE = 10; // configurable pool size
    static void try_insert_gene_to_pool(float fitness, const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases);
    static void save_gene_pool(const std::string& filename = "gene_pool.txt");
    static void load_gene_pool(const std::string& filename = "gene_pool.txt");
    static GeneEntry sample_gene_from_pool();
private:
    void clamp_to_screen(const Game& game);
};

// Helper functions for gene crossover and mutation
std::vector<std::vector<float>> crossover(const std::vector<std::vector<float>>&, const std::vector<std::vector<float>>&);
void mutate_genes(std::vector<std::vector<float>>&, int nMutate = NUMBER_OF_MUTATES); 