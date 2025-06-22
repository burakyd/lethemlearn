#include "Game.h"
#include "Player.h"
#include "Food.h"
#include "Hunter.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <ctime>

#define MIN_FOOD_FOR_REPRO 2
#define MIN_LIFETIME_FOR_REPRO 2000

int game_time_units = 0;

Game::Game(SDL_Renderer* renderer) : renderer(renderer) {
    // Initialize game state, spawn initial players/food as needed
}

void Game::update() {
    game_time_units++;
    // Update all players, hunters, and food
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

void Game::handleEvents() {
    // TODO: Implement event handling if needed
}

void Game::reset() {
    // TODO: Reset game state
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

void Game::maintain_population() {
    // Remove dead players (but not hunters)
    for (auto it = players.begin(); it != players.end(); ) {
        Player* p = *it;
        bool is_hunter = false;
        for (auto* h : hunters) if (h == p) is_hunter = true;
        if (!p->alive && !is_hunter) {
            // Insert genes into pool before deleting
            float fit = float(p->foodScore) * float(p->lifeTime) + p->distance_traveled;
            Player::try_insert_gene_to_pool(fit, p->genes, p->biases);
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
    // Elitism: keep best agent unchanged, but only if it is truly best
    const int N_ELITES = 3;
    std::vector<Player*> sorted_alive = alive_bots;
    std::sort(sorted_alive.begin(), sorted_alive.end(), [](Player* a, Player* b) {
        // Wall penalty
        float wall_penalty_a = 0.0f;
        if (a->x < 20 || a->x > a->width - 20) wall_penalty_a -= 10.0f;
        if (a->y < 20 || a->y > a->height - 20) wall_penalty_a -= 10.0f;
        // Reward for distance traveled
        float fitness_a = float(a->foodScore) * float(a->lifeTime) + wall_penalty_a + a->distance_traveled;

        float wall_penalty_b = 0.0f;
        if (b->x < 20 || b->x > b->width - 20) wall_penalty_b -= 10.0f;
        if (b->y < 20 || b->y > b->height - 20) wall_penalty_b -= 10.0f;
        float fitness_b = float(b->foodScore) * float(b->lifeTime) + wall_penalty_b + b->distance_traveled;

        return fitness_a > fitness_b;
    });
    std::vector<Player*> elites;
    for (int i = 0; i < N_ELITES && i < (int)sorted_alive.size(); ++i) {
        if (sorted_alive[i]->foodScore >= MIN_FOOD_FOR_REPRO && sorted_alive[i]->lifeTime >= MIN_LIFETIME_FOR_REPRO) {
            elites.push_back(sorted_alive[i]);
        }
    }
    int clones_added = 0;
    std::vector<bool> elite_cloned(N_ELITES, false);
    while (alive_bots.size() < MIN_BOT) {
        // Increase random agent injection probability
        if ((rand() % 100 < 40) || alive_bots.empty()) {
            std::array<int, 3> color = {rand() % 256, rand() % 256, rand() % 256};
            players.push_back(new Player(DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
        } else {
            // Clone each elite at most once
            bool cloned = false;
            for (int e = 0; e < (int)elites.size(); ++e) {
                if (!elite_cloned[e] && elites[e]->foodScore >= MIN_FOOD_FOR_REPRO && elites[e]->lifeTime >= MIN_LIFETIME_FOR_REPRO) {
                    Player* clone = new Player(elites[e]->genes, DOT_WIDTH, DOT_HEIGHT, elites[e]->color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), elites[e]->parent_id);
                    clone->biases = elites[e]->biases;
                    players.push_back(clone);
                    elite_cloned[e] = true;
                    cloned = true;
                    break;
                }
            }
            if (!cloned) {
                int top_n = 8; // Increase pool for more diversity
                std::vector<Player*> sorted_alive2;
                for (auto* p : alive_bots) {
                    if (p->foodScore >= MIN_FOOD_FOR_REPRO && p->lifeTime >= MIN_LIFETIME_FOR_REPRO) {
                        sorted_alive2.push_back(p);
                    }
                }
                if (sorted_alive2.empty()) {
                    // fallback: inject random
                    std::array<int, 3> color = {rand() % 256, rand() % 256, rand() % 256};
                    players.push_back(new Player(DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height)));
                } else {
                    std::sort(sorted_alive2.begin(), sorted_alive2.end(), [](Player* a, Player* b) {
                        return (float(a->foodScore) * float(a->lifeTime)) > (float(b->foodScore) * float(b->lifeTime));
                    });
                    Player* parent = sorted_alive2[rand() % std::min(top_n, (int)sorted_alive2.size())];
                    Player* parent2 = sorted_alive2[rand() % std::min(top_n, (int)sorted_alive2.size())];
                    int parent_id = std::distance(players.begin(), std::find(players.begin(), players.end(), parent));
                    std::array<int, 3> color = parent->color;
                    auto new_genes = crossover(parent->genes, parent2->genes);
                    mutate_genes(new_genes, 10);
                    Player* child = new Player(new_genes, DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % width), static_cast<float>(rand() % height), parent_id);
                    child->biases = parent->biases;
                    players.push_back(child);
                }
            }
        }
        alive_bots.push_back(players.back());
    }
} 