#include "Game.h"
#include "Player.h"
#include "Food.h"
#include "Hunter.h"
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <utility>
#include "Settings.h"
#include <iostream>
#include <omp.h> // Enable OpenMP parallelization
#include <iomanip>

#define MIN_FOOD_FOR_REPRO 2
#define MIN_LIFETIME_FOR_REPRO 2000

int game_time_units = 0;

// Add grid cell size for partitioning
constexpr int GRID_SIZE = GRID_CELL_SIZE;
using GridCell = std::vector<Player*>;
using FoodCell = std::vector<Food*>;

// Helper to get grid cell index
inline std::pair<int, int> get_cell(float x, float y) {
    return {static_cast<int>(x) / GRID_SIZE, static_cast<int>(y) / GRID_SIZE};
}

// Add grid structures to Game
std::unordered_map<long long, GridCell> player_grid;
std::unordered_map<long long, FoodCell> food_grid;

// Helper to get a unique key for a cell
inline long long cell_key(int cx, int cy) { return (static_cast<long long>(cx) << 32) | (cy & 0xffffffff); }

// Update grid each frame
void update_grids(Game& game) {
    player_grid.clear();
    food_grid.clear();
    for (auto* p : game.players) {
        if (!p->alive) continue;
        auto [cx, cy] = get_cell(p->x, p->y);
        player_grid[cell_key(cx, cy)].push_back(p);
    }
    for (auto* f : game.foods) {
        auto [cx, cy] = get_cell(f->x, f->y);
        food_grid[cell_key(cx, cy)].push_back(f);
    }
}

// Helper to get all players/food in neighboring cells
std::vector<Player*> get_nearby_players(float x, float y) {
    std::vector<Player*> result;
    auto [cx, cy] = get_cell(x, y);
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            long long key = cell_key(cx + dx, cy + dy);
            if (player_grid.count(key)) {
                result.insert(result.end(), player_grid[key].begin(), player_grid[key].end());
            }
        }
    }
    return result;
}
std::vector<Food*> get_nearby_food(float x, float y) {
    std::vector<Food*> result;
    auto [cx, cy] = get_cell(x, y);
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            long long key = cell_key(cx + dx, cy + dy);
            if (food_grid.count(key)) {
                result.insert(result.end(), food_grid[key].begin(), food_grid[key].end());
            }
        }
    }
    return result;
}

Game::Game(SDL_Renderer* renderer) : renderer(renderer) {
    // Initialize game state, spawn initial players/food as needed
}

void Game::update() {
    game_time_units++;
    update_grids();
    for (auto* p : players) if (p) p->update(*this);
    for (auto* h : hunters) if (h) h->update(*this);
    for (auto* f : foods) if (f) f->update(*this);
    maintain_population();
}

void Game::render() {
    // Draw all players
    for (auto* p : players) if (p) p->draw(renderer);
    // Draw all hunters
    for (auto* h : hunters) if (h) h->draw(renderer);
    // Draw all foods
    for (auto* f : foods) if (f) f->draw(renderer);
}

bool Game::inLocation(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
    // Simple AABB collision
    return !(x1 + w1 < x2 || x1 > x2 + w2 || y1 + h1 < y2 || y1 > y2 + h2);
}

void Game::newPlayer(const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases, int width, int height, SDL_Color color, float speed) {
    float x = (rand() % (this->width - width)) + width / 2.0f;
    float y = (rand() % (this->height - height)) + height / 2.0f;
    players.push_back(new Player(genes, biases, width, height, color, x, y));
}

void Game::newHunter(int number, int width, int height, SDL_Color color, float speed, bool random_color, bool random_size) {
    for (int i = 0; i < number; ++i) {
        if (random_color) {
            color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
        }
        float x = (rand() % (this->width - width)) + width / 2.0f;
        float y = (rand() % (this->height - height)) + height / 2.0f;
        bool valid = true;
        for (auto* player : players) {
            float dx = x - player->x;
            float dy = y - player->y;
            float min_dist = (width + player->width) / 2.0f;
            if (std::sqrt(dx * dx + dy * dy) < min_dist) {
                valid = false;
                break;
            }
        }
        if (!valid) { --i; continue; }
        for (auto* food : foods) {
            float dx = x - food->x;
            float dy = y - food->y;
            float min_dist = (width + food->width) / 2.0f;
            if (std::sqrt(dx * dx + dy * dy) < min_dist) {
                valid = false;
                break;
            }
        }
        if (!valid) { --i; continue; }
        if (random_size) {
            int s = RANDOM_SIZE_MIN + rand() % (RANDOM_SIZE_MAX - RANDOM_SIZE_MIN + 1);
            width = height = s;
        }
        Hunter* hunter = new Hunter(width, height, color, x, y, speed);
        hunters.push_back(hunter);
        players.push_back(hunter);
    }
}

void Game::randomFood(int num) {
    for (int i = 0; i < num; ++i) {
        int width = FOOD_WIDTH, height = FOOD_HEIGHT;
        float x = (rand() % (this->width - width)) + width / 2.0f;
        float y = (rand() % (this->height - height)) + height / 2.0f;
        bool valid = true;
        for (auto* player : players) {
            float dx = x - player->x;
            float dy = y - player->y;
            float min_dist = (width + player->width) / 2.0f;
            if (std::sqrt(dx * dx + dy * dy) < min_dist) {
                valid = false;
                break;
            }
        }
        if (!valid) { --i; continue; }
        for (auto* food : foods) {
            float dx = x - food->x;
            float dy = y - food->y;
            float min_dist = (width + food->width) / 2.0f;
            if (std::sqrt(dx * dx + dy * dy) < min_dist) {
                valid = false;
                break;
            }
        }
        if (!valid) { --i; continue; }
        foods.push_back(new Food(x, y, width, height));
    }
}

// Maintains population, gene pool, elitism, crossover and other mechanisms of Genetic Algorithm
void Game::maintain_population() {
    static int generation = 0;
    static float best_fitness = 0.0f;
    static int generations_since_improvement = 0;
    static std::vector<Player*> sorted_alive;
    static std::vector<Player*> elites;
    // fitness calculation lambda
    auto calc_fitness = [this](Player* p, float w_food, float w_life, float w_explore, float w_total_players, float min_food, float min_life, float early_death_time, float early_death_penalty) {
        float exploration_bonus = w_explore * p->visited_cells.size();
        float wall_camping_penalty = WALL_PENALTY_PER_FRAME * p->time_near_wall;
        float fitness = w_food * p->totalFoodEaten
            + w_life * p->lifeTime
            + exploration_bonus
            + w_total_players * p->totalPlayersEaten
            + wall_camping_penalty;
        if (p->totalFoodEaten < min_food || p->lifeTime < min_life) fitness = 0.0f;
        if (p->lifeTime < early_death_time) fitness -= early_death_penalty;
        return fitness;
    };
    // Remove dead players (but not hunters)
    for (auto it = players.begin(); it != players.end(); ) {
        Player* p = *it;
        bool is_hunter = false;
        for (auto* h : hunters) if (h == p) is_hunter = true;
        if (!p->alive && !is_hunter) {
            if (!p->is_human) {
                float fitness = calc_fitness(p, FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
                if (fitness >= MIN_FITNESS_FOR_GENE_POOL) {
                    Player::try_insert_gene_to_pool(fitness, p->genes, p->biases);
                }
            }
            delete p;
            it = players.erase(it);
        } else {
            ++it;
        }
    }
    // Count alive bots (not hunters)
    std::vector<Player*> alive_bots;
    for (auto* p : players) {
        bool is_hunter = false;
        for (auto* h : hunters) if (h == p) is_hunter = true;
        if (p->alive && !is_hunter) alive_bots.push_back(p);
    }
    // Only run heavy operations at intervals
    if (generation % GENE_POOL_CHECK_INTERVAL == 0) {
        // Dynamic elitism: select elites for reproduction
        sorted_alive.clear();
        for (auto* p : alive_bots) {
            if (!p->is_human) sorted_alive.push_back(p);
        }
        std::sort(sorted_alive.begin(), sorted_alive.end(), [this, &calc_fitness](Player* a, Player* b) {
            float fitness_a = calc_fitness(a, FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
            float fitness_b = calc_fitness(b, FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
            return fitness_a > fitness_b;
        });
        // Select elites for reproduction (use TOP_ALIVE_TO_INSERT as the number of elites)
        elites.clear();
        int n_elites = std::min(TOP_ALIVE_TO_INSERT, (int)sorted_alive.size());
        for (int i = 0; i < n_elites; ++i) {
            if (sorted_alive[i]->totalFoodEaten >= FITNESS_MIN_FOR_REPRO && sorted_alive[i]->lifeTime >= FITNESS_MIN_LIFETIME_FOR_REPRO) {
                elites.push_back(sorted_alive[i]);
            }
        }
        // Insert all elites into gene pool
        int inserted = 0;
        for (Player* p : elites) {
            float fitness = calc_fitness(p, FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
            if (fitness >= MIN_FITNESS_FOR_GENE_POOL) {
                Player::try_insert_gene_to_pool(fitness, p->genes, p->biases);
                ++inserted;
            }
        }
        // Optionally, insert additional top non-elite players to reach TOP_ALIVE_TO_INSERT
        for (int i = 0; i < (int)sorted_alive.size() && inserted < TOP_ALIVE_TO_INSERT; ++i) {
            Player* p = sorted_alive[i];
            if (std::find(elites.begin(), elites.end(), p) == elites.end()) {
                float fitness = calc_fitness(p, FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
                if (fitness >= MIN_FITNESS_FOR_GENE_POOL) {
                    Player::try_insert_gene_to_pool(fitness, p->genes, p->biases);
                    ++inserted;
                }
            }
        }
        // Prune gene pool
        Player::prune_gene_pool_diversity(FITNESS_DIVERSITY_PRUNE_MIN_DIST);
        // Diversity and mutation rate logic
        float current_best = 0.0f;
        float diversity_sum = 0.0f;
        float diversity_min = 1e9, diversity_max = 0.0f;
        int diversity_count = 0;
        float avg_fitness = 0.0f;
        float last_fitness = Player::get_last_inserted_fitness();
        // Calculate fitness stats for alive players using the same fitness function
        if (!sorted_alive.empty()) {
            float sum_fitness = 0.0f;
            float best_fitness_alive = 0.0f;
            for (size_t i = 0; i < sorted_alive.size(); ++i) {
                float fit = calc_fitness(sorted_alive[i], FITNESS_WEIGHT_FOOD, FITNESS_WEIGHT_LIFE, FITNESS_WEIGHT_EXPLORE, FITNESS_WEIGHT_PLAYERS, FITNESS_MIN_FOOD, FITNESS_MIN_LIFE, FITNESS_EARLY_DEATH_TIME, FITNESS_EARLY_DEATH_PENALTY);
                sum_fitness += fit;
                if (fit > best_fitness_alive) best_fitness_alive = fit;
            }
            avg_fitness = sum_fitness / sorted_alive.size();
            current_best = best_fitness_alive;
        }
        // Calculate diversity for gene pool
        for (size_t i = 0; i < Player::gene_pool.size(); ++i) {
            for (size_t j = i + 1; j < Player::gene_pool.size(); ++j) {
                float dist = Player::genetic_distance(Player::gene_pool[i], Player::gene_pool[j]);
                diversity_sum += dist;
                if (dist < diversity_min) diversity_min = dist;
                if (dist > diversity_max) diversity_max = dist;
                diversity_count++;
            }
        }
        float avg_diversity = (diversity_count > 0) ? (diversity_sum / diversity_count) : 0.0f;
        float diversity_threshold = 0.15f;
        float prev_mutation_rate = Player::adaptive_mutation_rate;
        if (current_best > best_fitness) {
            best_fitness = current_best;
            generations_since_improvement = 0;
            Player::adaptive_mutation_rate = MUTATION_RATE;
        } else {
            generations_since_improvement++;
            if (generations_since_improvement > ADAPTIVE_MUTATION_PATIENCE ) {
                if (avg_diversity < diversity_threshold || generations_since_improvement > ADAPTIVE_MUTATION_PATIENCE) {
                    Player::adaptive_mutation_rate = std::min(Player::adaptive_mutation_rate * ADAPTIVE_MUTATION_FACTOR, MAX_MUTATION_RATE);
                }
                generations_since_improvement = 0;
            }
        }
        if (Player::adaptive_mutation_rate != prev_mutation_rate) {
        }
    }
    // Fill up population
    while (alive_bots.size() < MIN_BOT) {
        // 5% chance: insert Hall of Fame agent
        // if (!Player::hall_of_fame.empty() && (rand() % 100 < 5)) {
        //     auto hof = Player::sample_hall_of_fame();
        //     SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
        //     auto [genes, biases] = random_genes_and_biases();
        //     Player* hof_agent = new Player(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), -1);
        //     players.push_back(hof_agent);
        // } else 
        if ((rand() % 100 < 30) || alive_bots.empty()) {
            SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
            auto [genes, biases] = random_genes_and_biases();
            players.push_back(new Player(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
        } else {
            // 40% chance: clone an elite
            if (!elites.empty() && (rand() % 100 < 40)) {
                int e = rand() % elites.size();
                auto [genes, biases] = random_genes_and_biases();
                SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
                Player* clone = new Player(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), elites[e]->parent_id);
                players.push_back(clone);
            } else if (!Player::gene_pool.empty()) {
                // 30% chance: crossover from gene pool using tournament selection
                int tournament_size = 5;
                std::vector<const Player::GeneEntry*> tournament;
                std::set<const Player::GeneEntry*> unique_entries;
                while ((int)tournament.size() < tournament_size && (int)unique_entries.size() < (int)Player::gene_pool.size()) {
                    int idx = rand() % Player::gene_pool.size();
                    const Player::GeneEntry* entry = &Player::gene_pool[idx];
                    if (unique_entries.insert(entry).second) {
                        tournament.push_back(entry);
                    }
                }
                if (tournament.size() < 2) {
                    // fallback: inject random
                    SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
                    auto [genes, biases] = random_genes_and_biases();
                    players.push_back(new Player(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
                } else {
                    // Use tournament selection
                    const Player::GeneEntry* parent1 = *std::max_element(tournament.begin(), tournament.end(), [](const Player::GeneEntry* a, const Player::GeneEntry* b) { return a->fitness < b->fitness; });
                    // Remove parent1 from tournament for parent2 selection
                    std::vector<const Player::GeneEntry*> tournament2;
                    for (const auto* entry : tournament) if (entry != parent1) tournament2.push_back(entry);
                    const Player::GeneEntry* parent2 = *std::max_element(tournament2.begin(), tournament2.end(), [](const Player::GeneEntry* a, const Player::GeneEntry* b) { return a->fitness < b->fitness; });
                    SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
                    auto new_genes = crossover(parent1->genes, parent2->genes);
                    auto new_biases = crossover_biases(parent1->biases, parent2->biases);
                    int nMutate = int(MUTATION_ATTEMPTS * Player::adaptive_mutation_rate);
                    mutate_genes(new_genes, nMutate);
                    mutate_biases(new_biases, nMutate);
                    Player* child = new Player(new_genes, new_biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), -1);
                    players.push_back(child);
                }
            } else {
                // fallback: inject random
                SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
                auto [genes, biases] = random_genes_and_biases();
                players.push_back(new Player(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
            }
        }
        alive_bots.push_back(players.back());
    }
    ++generation;
}

void Game::update_grids() {
    // Clear grids
    for (int x = 0; x < GRID_WIDTH; ++x)
        for (int y = 0; y < GRID_HEIGHT; ++y) {
            player_grid[x][y].clear();
            food_grid[x][y].clear();
        }
    // Assign players
    for (Player* p : players) {
        int gx = int(p->x) / CELL_SIZE;
        int gy = int(p->y) / CELL_SIZE;
        if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT)
            player_grid[gx][gy].push_back(p);
    }
    // Assign food
    for (Food* f : foods) {
        int gx = int(f->x) / CELL_SIZE;
        int gy = int(f->y) / CELL_SIZE;
        if (gx >= 0 && gx < GRID_WIDTH && gy >= 0 && gy < GRID_HEIGHT)
            food_grid[gx][gy].push_back(f);
    }
}

std::vector<Player*> Game::get_nearby_players(float x, float y) {
    std::vector<Player*> result;
    int gx = int(x) / CELL_SIZE;
    int gy = int(y) / CELL_SIZE;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = gx + dx, ny = gy + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                for (Player* p : player_grid[nx][ny]) {
                    result.push_back(p);
                }
            }
        }
    }
    return result;
}

std::vector<Food*> Game::get_nearby_food(float x, float y) {
    std::vector<Food*> result;
    int gx = int(x) / CELL_SIZE;
    int gy = int(y) / CELL_SIZE;
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = gx + dx, ny = gy + dy;
            if (nx >= 0 && nx < GRID_WIDTH && ny >= 0 && ny < GRID_HEIGHT) {
                for (Food* f : food_grid[nx][ny]) {
                    result.push_back(f);
                }
            }
        }
    }
    return result;
}