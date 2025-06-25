#pragma once
#include <vector>
#include <array>
#include "Settings.h"
#include <SDL.h>
#include <random>
#include <memory>
#include <set>
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
    int time_near_wall = 0; // Counts frames spent near wall/corner
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
    bool is_human = false;
    void clamp_to_screen(const Game& game);
    void update_size_from_food();
    void decrease_size_step();

    // Lookup table optimization
    static constexpr int MAX_FOOD = 2000;
    static int food_to_size[MAX_FOOD + 1];
    static int size_to_food[MAX_PLAYER_SIZE + 1];
    static void init_lookup_tables();
    static bool lookup_tables_initialized;

    std::set<std::pair<int, int>> visited_cells;
    void update_exploration_cell(int cell_size, int world_width, int world_height);

    // Hall of Fame for all-time best genes
    static std::vector<GeneEntry> hall_of_fame;
    static constexpr int HALL_OF_FAME_SIZE = 10;
    static void update_hall_of_fame(float fitness, const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases);
    static GeneEntry sample_hall_of_fame();
    static void save_hall_of_fame(const std::string& filename = "hall_of_fame.txt");
    static void load_hall_of_fame(const std::string& filename = "hall_of_fame.txt");

    // Diversity-based gene pool pruning
    static float genetic_distance(const GeneEntry& a, const GeneEntry& b);
    static void prune_gene_pool_diversity(int max_size, float min_distance = 0.2f);
    // Adaptive mutation rate
    static float adaptive_mutation_rate;
};

// Helper functions for gene crossover and mutation
std::vector<std::vector<float>> crossover(const std::vector<std::vector<float>>&, const std::vector<std::vector<float>>&);
void mutate_genes(std::vector<std::vector<float>>&, int nMutate = NUMBER_OF_MUTATES);
// Bias crossover and mutation
std::vector<std::vector<float>> crossover_biases(const std::vector<std::vector<float>>&, const std::vector<std::vector<float>>&);
void mutate_biases(std::vector<std::vector<float>>&, int nMutate = NUMBER_OF_MUTATES);

class HumanPlayer : public Player {
public:
    HumanPlayer(int width = DOT_WIDTH, int height = DOT_HEIGHT, std::array<int, 3> color = DOT_COLOR, float x = 0, float y = 0, bool alive = true);
    void update(Game& game) override;
}; 