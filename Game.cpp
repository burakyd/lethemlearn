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

void Game::newPlayer(int number, int width, int height, std::array<int, 3> color, float speed, bool random_color, bool random_size) {
    for (int i = 0; i < number; ++i) {
        if (random_color) {
            color = {rand() % 256, rand() % 256, rand() % 256};
        }
        float x = (rand() % (this->width - width)) + width / 2.0f;
        float y = (rand() % (this->height - height)) + height / 2.0f;
        // Check collision with existing players
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
        // Check collision with food
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
        if (!Player::gene_pool.empty()) {
            auto entry = Player::sample_gene_from_pool();
            players.push_back(new Player(entry.genes, width, height, color, x, y));
        } else {
            players.push_back(new Player(width, height, color, x, y));
        }
    }
}

void Game::newHunter(int number, int width, int height, std::array<int, 3> color, float speed, bool random_color, bool random_size) {
    for (int i = 0; i < number; ++i) {
        if (random_color) {
            color = {rand() % 256, rand() % 256, rand() % 256};
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
    // Remove dead players (but not hunters)
    for (auto it = players.begin(); it != players.end(); ) {
        Player* p = *it;
        bool is_hunter = false;
        for (auto* h : hunters) if (h == p) is_hunter = true;
        if (!p->alive && !is_hunter) {
            // Insert genes into pool before deleting if fitness is high enough
            if (!p->is_human) {
                // Use the same fitness function as sorting
                const float w_food = 10.0f;
                const float w_life = 1.0f;
                const float w_dist = 0.05f;
                const float w_size = 2.0f;
                const float w_explore = 3.0f;
                const float min_food = 2;
                const float min_life = 1000;
                const float early_death_time = 500;
                const float early_death_penalty = 50.0f;
                float wall_penalty = 0.0f;
                if (p->x - p->width / 2 < 20 || p->x + p->width / 2 > width - 20) wall_penalty -= 10.0f;
                if (p->y - p->height / 2 < 20 || p->y + p->height / 2 > height - 20) wall_penalty -= 10.0f;
                float exploration_bonus = w_explore * p->visited_cells.size();
                float wall_camping_penalty = WALL_PENALTY_PER_FRAME * p->time_near_wall;
                float fitness = w_food * p->foodScore + w_life * p->lifeTime + w_dist * p->distance_traveled + w_size * (p->width - DOT_WIDTH) + wall_penalty + exploration_bonus + wall_camping_penalty;
                if (p->foodScore < min_food || p->lifeTime < min_life) fitness = 0.0f;
                if (p->lifeTime < early_death_time) fitness -= early_death_penalty;
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
    // Dynamic elitism: top ELITISM_PERCENT
    std::vector<Player*> sorted_alive;
    for (auto* p : alive_bots) {
        if (!p->is_human) sorted_alive.push_back(p);
    }
    std::sort(sorted_alive.begin(), sorted_alive.end(), [this](Player* a, Player* b) {
        // Improved fitness function with exploration bonus
        const float w_food = 10.0f;
        const float w_life = 1.0f;
        const float w_dist = 0.05f;
        const float w_size = 2.0f;
        const float w_explore = 3.0f; // exploration bonus weight
        const float min_food = 5;
        const float min_life = 2000;
        const float early_death_time = 1000;
        const float early_death_penalty = 50.0f;
        float wall_penalty_a = 0.0f;
        if (a->x - a->width / 2 < 20 || a->x + a->width / 2 > width - 20) wall_penalty_a -= 10.0f;
        if (a->y - a->height / 2 < 20 || a->y + a->height / 2 > height - 20) wall_penalty_a -= 10.0f;
        float exploration_bonus_a = w_explore * a->visited_cells.size();
        float wall_camping_penalty_a = WALL_PENALTY_PER_FRAME * a->time_near_wall;
        float fitness_a = w_food * a->foodScore + w_life * a->lifeTime + w_dist * a->distance_traveled + w_size * (a->width - DOT_WIDTH) + wall_penalty_a + exploration_bonus_a + wall_camping_penalty_a;
        if (a->foodScore < min_food || a->lifeTime < min_life) fitness_a = 0.0f;
        if (a->lifeTime < early_death_time) fitness_a -= early_death_penalty;
        float wall_penalty_b = 0.0f;
        if (b->x - b->width / 2 < 20 || b->x + b->width / 2 > width - 20) wall_penalty_b -= 10.0f;
        if (b->y - b->height / 2 < 20 || b->y + b->height / 2 > height - 20) wall_penalty_b -= 10.0f;
        float exploration_bonus_b = w_explore * b->visited_cells.size();
        float wall_camping_penalty_b = WALL_PENALTY_PER_FRAME * b->time_near_wall;
        float fitness_b = w_food * b->foodScore + w_life * b->lifeTime + w_dist * b->distance_traveled + w_size * (b->width - DOT_WIDTH) + wall_penalty_b + exploration_bonus_b + wall_camping_penalty_b;
        if (b->foodScore < min_food || b->lifeTime < min_life) fitness_b = 0.0f;
        if (b->lifeTime < early_death_time) fitness_b -= early_death_penalty;
        return fitness_a > fitness_b;
    });
    int n_elites = std::max(1, int(ELITISM_PERCENT * float(MIN_BOT)));
    std::vector<Player*> elites;
    for (int i = 0; i < n_elites && i < (int)sorted_alive.size(); ++i) {
        if (sorted_alive[i]->foodScore >= MIN_FOOD_FOR_REPRO && sorted_alive[i]->lifeTime >= MIN_LIFETIME_FOR_REPRO) {
            elites.push_back(sorted_alive[i]);
        }
    }
    int clones_added = 0;
    std::vector<bool> elite_cloned(n_elites, false);
    // Prune gene pool every GENE_POOL_PRUNE_INTERVAL generations
    if (generation % GENE_POOL_PRUNE_INTERVAL == 0 && Player::gene_pool.size() > GENE_POOL_SIZE) {
        std::sort(Player::gene_pool.begin(), Player::gene_pool.end(), [](const Player::GeneEntry& a, const Player::GeneEntry& b) { return a.fitness > b.fitness; });
        Player::gene_pool.resize(GENE_POOL_SIZE);
    }
    // Fill up population
    while (alive_bots.size() < MIN_BOT) {
        // 30% chance: inject random agent for diversity
        if ((rand() % 100 < 30) || alive_bots.empty()) {
            std::array<int, 3> color = {rand() % 256, rand() % 256, rand() % 256};
            players.push_back(new Player(DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
        } else {
            // 40% chance: clone an elite
            if (!elites.empty() && (rand() % 100 < 40)) {
                int e = rand() % elites.size();
                Player* clone = new Player(elites[e]->genes, DOT_WIDTH, DOT_HEIGHT, elites[e]->color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), elites[e]->parent_id);
                clone->biases = elites[e]->biases;
                players.push_back(clone);
            } else if (!Player::gene_pool.empty()) {
                // 30% chance: crossover from gene pool using tournament selection
                int tournament_size = 5;
                std::vector<const Player::GeneEntry*> tournament;
                for (int t = 0; t < tournament_size; ++t) {
                    int idx = rand() % Player::gene_pool.size();
                    tournament.push_back(&Player::gene_pool[idx]);
                }
                // Use tournament selection
                const Player::GeneEntry* parent1 = *std::max_element(tournament.begin(), tournament.end(), [](const Player::GeneEntry* a, const Player::GeneEntry* b) { return a->fitness < b->fitness; });
                const Player::GeneEntry* parent2 = *std::max_element(tournament.begin(), tournament.end(), [parent1](const Player::GeneEntry* a, const Player::GeneEntry* b) { return (a->fitness < b->fitness) && (a != parent1); });
                std::array<int, 3> color = {rand() % 256, rand() % 256, rand() % 256};
                auto new_genes = crossover(parent1->genes, parent2->genes);

                // Adaptive mutation: higher if population is less diverse
                float mutation_rate = MUTATION_RATE;
                if (Player::gene_pool.size() < GENE_POOL_SIZE / 2) mutation_rate *= 2.0f;
                mutate_genes(new_genes, int(10 * mutation_rate));
                Player* child = new Player(new_genes, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), -1);
                child->biases = parent1->biases;
                players.push_back(child);
            } else {
                // fallback: inject random
                std::array<int, 3> color = {rand() % 256, rand() % 256, rand() % 256};
                players.push_back(new Player(DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
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