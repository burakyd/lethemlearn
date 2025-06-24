#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "Game.h"
#include "Settings.h"

class GameApp {
public:
    GameApp();
    ~GameApp();
    bool init();
    void run();
    void cleanup();
private:
    void restart_simulation(const std::vector<std::vector<std::vector<float>>>* loaded_genes = nullptr, const std::vector<std::vector<float>>* best_gene = nullptr);
    void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color);
    struct SidebarButton {
        SDL_Rect rect;
        std::string label;
        std::string action;
    };
    std::vector<SidebarButton> sidebar_buttons;
    int hovered_button = -1;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;
    Game* game = nullptr;
    // Simulation parameters
    int g_bot_count;
    int g_food_count;
    bool g_hunters_enabled;
    int g_hunter_count;
    bool g_player_enabled;
    int pending_bot_count;
    int pending_food_count;
    bool pending_hunters_enabled;
    int pending_hunter_count;
    bool pending_player_enabled;
    // Other state
    bool quit = false;
    bool paused = true;
    std::vector<int> speed_steps;
    int speed_index = 0;
    int sim_speed = 1;
    bool logic_max_mode = false;
    bool show_menu = true;
    bool show_settings = false;
    Uint32 sim_start_time = 0;
    Uint32 last_gene_pool_save = 0;
    const Uint32 GENE_POOL_SAVE_INTERVAL = 5000;
    int game_time_units = 0;
}; 