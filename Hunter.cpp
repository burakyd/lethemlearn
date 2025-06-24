#include "Hunter.h"
#include "Game.h"
#include <random>
#include <algorithm>
#include "Food.h"

constexpr float HUNTER_SPEED = 0.2f;

Hunter::Hunter(int width, int height, std::array<int, 3> color, float x, float y, float speed, bool alive)
    : Player(width, height, color, x, y, alive), movetime(0), keys{0,0,0,0} {
    this->speed = HUNTER_SPEED;
}

void Hunter::update(Game& game) {
    // Find which players are already targeted by other hunters
    std::vector<Player*> already_targeted;
    for (auto* h : game.hunters) {
        if (h == this) continue;
        // Find the closest player for this hunter (if any)
        Player* h_target = nullptr;
        float h_min_dist = 1e9f;
        for (auto* p : game.players) {
            if (p == h || !p->alive) continue;
            if (dynamic_cast<Hunter*>(p) != nullptr) continue;
            float dx = p->x - h->x;
            float dy = p->y - h->y;
            float dist = dx*dx + dy*dy;
            if (dist < h_min_dist) {
                h_min_dist = dist;
                h_target = p;
            }
        }
        if (h_target) already_targeted.push_back(h_target);
    }
    // Now pick the nearest player not already targeted
    Player* target = nullptr;
    float min_dist = 1e9f;
    for (auto* p : game.players) {
        if (p == this || !p->alive) continue;
        if (dynamic_cast<Hunter*>(p) != nullptr) continue;
        if (std::find(already_targeted.begin(), already_targeted.end(), p) != already_targeted.end()) continue;
        // Only target players the hunter can eat - so they won't aimlessly chase a bigger player
        if (height <= p->height * 1.2f) continue;
        float dx = p->x - x;
        float dy = p->y - y;
        float dist = dx*dx + dy*dy;
        if (dist < min_dist) {
            min_dist = dist;
            target = p;
        }
    }
    // If all players are already targeted, fallback to nearest the hunter can eat
    if (!target) {
        for (auto* p : game.players) {
            if (p == this || !p->alive) continue;
            if (dynamic_cast<Hunter*>(p) != nullptr) continue;
            // Only target players the hunter can eat
            if (height <= p->height * 1.2f) continue;
            float dx = p->x - x;
            float dy = p->y - y;
            float dist = dx*dx + dy*dy;
            if (dist < min_dist) {
                min_dist = dist;
                target = p;
            }
        }
    }
    if (target) {
        float dx = target->x - x;
        float dy = target->y - y;
        float d = std::sqrt(dx*dx + dy*dy);
        if (d > 1e-3f) {
            // Add a bit of noise to the direction
            float angle = std::atan2(dy, dx);
            float noise = (((float)rand() / RAND_MAX) - 0.5f) * 0.4f; // noise in [-0.2, 0.2] radians
            angle += noise;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;
            x += vx;
            y += vy;
        }
    }
    // Clamp to screen using Player's method
    clamp_to_screen(game);
    // Eating logic
    eatFood(game);
    for (auto* other : game.players) {
        if (other->alive && other != this) {
            eatPlayer(game, *other);
        }
    }
}

void Hunter::draw(SDL_Renderer* renderer) {
    SDL_Rect rect = {static_cast<int>(x - width / 2.0f), static_cast<int>(y - height / 2.0f), width, height};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    SDL_RenderFillRect(renderer, &rect);
}

bool Hunter::eatPlayer(Game& game, Player& other) {
    if (!other.alive || &other == this) return false;
    if (collide(other) && height > other.height * 1.2f) {
        playerEaten++;
        killTime = 0;
        other.alive = false;
        // Do NOT increase size or foodCount
        // Replenish population if needed
        if (std::count_if(game.players.begin(), game.players.end(), [](Player* p){ return p->alive; }) <= MIN_BOT) {
            game.newPlayer(1, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SPEED, true, true);
        }
        return true;
    }
    return false;
}

bool Hunter::eatFood(Game& game) {
    return false;
} 