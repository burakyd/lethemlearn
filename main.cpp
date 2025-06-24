#include <SDL.h>
#include <SDL_ttf.h>
#include "Game.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "Player.h"
#include "Hunter.h"
#include "Food.h"
#include <fstream>
#include <map>
#include "Settings.h"
#include "GameApp.h"

// Helper to render text
void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Simulation parameters (global for menu adjustment)
int g_bot_count = MIN_BOT;
int g_food_count = NUMBER_OF_FOODS;
bool g_hunters_enabled = true;
int g_hunter_count = HUNTERS;
bool g_player_enabled = PLAYER_ENABLED;

// Add pending settings variables at the top (after global settings):
int pending_bot_count = g_bot_count;
int pending_food_count = g_food_count;
bool pending_hunters_enabled = g_hunters_enabled;
int pending_hunter_count = g_hunter_count;
bool pending_player_enabled = g_player_enabled;

// Add button definitions after global variables:
struct SidebarButton {
    SDL_Rect rect;
    std::string label;
    std::string action;
};

std::vector<SidebarButton> sidebar_buttons;
int hovered_button = -1;

// Helper to check if file exists
bool file_exists(const std::string& filename) {
    std::ifstream f(filename);
    return f.good();
}

// In main.cpp, declare a global variable for game time units:
extern int game_time_units;

void restart_simulation(Game& game, SDL_Renderer* renderer, const std::vector<std::vector<std::vector<float>>>* loaded_genes = nullptr, const std::vector<std::vector<float>>* best_gene = nullptr) {
    // Delete all players (which includes hunters)
    for (auto* p : game.players) delete p;
    for (auto* f : game.foods) delete f;
    game.players.clear();
    game.hunters.clear();
    game.foods.clear();
    // Ensure at least MIN_BOT bots
    g_bot_count = std::max(g_bot_count, MIN_BOT);
    int bots_to_spawn = g_bot_count;
    if (g_player_enabled) {
        // Add human player at center
        game.players.push_back(new HumanPlayer(DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
        bots_to_spawn -= 1;
    }
    if (loaded_genes && !loaded_genes->empty()) {
        int used = 0;
        for (const auto& genes : *loaded_genes) {
            if (bots_to_spawn <= 0) break;
            game.players.push_back(new Player(genes, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
            used++;
            bots_to_spawn--;
        }
        if (used < bots_to_spawn)
            game.newPlayer(bots_to_spawn - used, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SPEED, true, true);
    } else if (best_gene && !best_gene->empty()) {
        for (int i = 0; i < bots_to_spawn; ++i) {
            game.players.push_back(new Player(*best_gene, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
        }
    } else {
        game.newPlayer(bots_to_spawn, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SPEED, true, true);
    }
    if (g_hunters_enabled) {
        game.newHunter(g_hunter_count, HUNTER_WIDTH, HUNTER_HEIGHT, HUNTER_COLOR, SPEED, false, false);
    }
    game.randomFood(g_food_count);
}

int main(int argc, char* argv[]) {
    GameApp app;
    if (!app.init()) return 1;
    app.run();
    app.cleanup();
    return 0;
} 