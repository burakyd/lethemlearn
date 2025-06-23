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
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("C++ AI Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH + 200, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    TTF_Font* font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // At startup, load the gene pool
    Player::load_gene_pool("gene_pool.txt");

    Game game(renderer);
    restart_simulation(game, renderer);
    bool quit = false;
    bool paused = true;
    std::vector<int> speed_steps = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, -1, -2}; // -1: MAX, -2: LOGIC MAX
    int speed_index = 0;
    int sim_speed = speed_steps[speed_index];
    bool logic_max_mode = false;
    SDL_Event e;
    bool show_menu = true;
    bool show_settings = false;
    Uint32 sim_start_time = SDL_GetTicks(); // Timer start
    // In the main simulation loop, periodically save the gene pool
    Uint32 last_gene_pool_save = SDL_GetTicks();
    const Uint32 GENE_POOL_SAVE_INTERVAL = 5000; // ms
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (show_menu && e.key.keysym.sym == SDLK_SPACE) {
                    show_menu = false;
                    paused = false;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    if (show_settings) {
                        // Apply pending settings and restart
                        g_bot_count = pending_bot_count;
                        g_food_count = pending_food_count;
                        g_hunters_enabled = pending_hunters_enabled;
                        g_hunter_count = pending_hunter_count;
                        g_player_enabled = pending_player_enabled;
                        restart_simulation(game, renderer);
                        show_settings = false;
                        paused = false;
                    } else {
                        paused = !paused;
                        show_menu = paused;
                        show_settings = false;
                    }
                } else if (e.key.keysym.sym == SDLK_r) {
                    restart_simulation(game, renderer);
                } else if (e.key.keysym.sym == SDLK_UP && !show_settings) {
                    speed_index = std::min((int)speed_steps.size() - 1, speed_index + 1); // Allow LOGIC MAX
                    sim_speed = speed_steps[speed_index];
                    logic_max_mode = (sim_speed == -2);
                } else if (e.key.keysym.sym == SDLK_DOWN && !show_settings) {
                    speed_index = std::max(0, speed_index - 1);
                    sim_speed = speed_steps[speed_index];
                    logic_max_mode = false;
                }
            }
            // Handle mouse events for sidebar buttons
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            hovered_button = -1;
            for (size_t i = 0; i < sidebar_buttons.size(); ++i) {
                if (mx >= sidebar_buttons[i].rect.x && mx <= sidebar_buttons[i].rect.x + sidebar_buttons[i].rect.w &&
                    my >= sidebar_buttons[i].rect.y && my <= sidebar_buttons[i].rect.y + sidebar_buttons[i].rect.h) {
                    hovered_button = (int)i;
                    break;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT && hovered_button != -1) {
                std::string action = sidebar_buttons[hovered_button].action;
                if (action == "pause") { paused = !paused; show_menu = paused; show_settings = false; }
                else if (action == "settings") {
                    if (!show_settings) {
                        pending_bot_count = g_bot_count;
                        pending_food_count = g_food_count;
                        pending_hunters_enabled = g_hunters_enabled;
                        pending_hunter_count = g_hunter_count;
                        pending_player_enabled = g_player_enabled;
                        show_settings = true;
                        paused = true;
                    } else {
                        g_bot_count = pending_bot_count;
                        g_food_count = pending_food_count;
                        g_hunters_enabled = pending_hunters_enabled;
                        g_hunter_count = pending_hunter_count;
                        g_player_enabled = pending_player_enabled;
                        restart_simulation(game, renderer);
                        show_settings = false;
                        paused = false;
                    }
                }
                else if (action == "restart") restart_simulation(game, renderer);
                else if (action == "speed_up") {
                    speed_index = std::min((int)speed_steps.size() - 1, speed_index + 1);
                    sim_speed = speed_steps[speed_index];
                    logic_max_mode = (sim_speed == -2);
                }
                else if (action == "speed_down") {
                    speed_index = std::max(0, speed_index - 1);
                    sim_speed = speed_steps[speed_index];
                    logic_max_mode = false;
                }
            }
        }
        if (sim_speed == -2) {
            // LOGIC MAX: render sidebar with LOGIC MAX and timer before freezing
            SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255); // Darker background
            SDL_RenderClear(renderer);
            SDL_Rect game_area = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
            SDL_RenderFillRect(renderer, &game_area);
            game.render();
            SDL_Rect sidebar = {SCREEN_WIDTH, 0, 200, SCREEN_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &sidebar);
            SDL_Color white = {255,255,255,255};
            SDL_Color green = {0,255,0,255};
            SDL_Color yellow = {255,255,0,255};
            SDL_Color cyan = {0,255,255,255};
            int y = 20;
            int alive_players = 0, total_food = game.foods.size(), total_hunters = game.hunters.size();
            int best_score = 0, sum_score = 0, min_score = 1e6;
            Player* best_bot = nullptr;
            int best_bot_id = -1;
            int best_bot_lifetime = 0;
            int player_index = 0;
            for (auto* p : game.players) {
                if (p->alive) {
                    ++alive_players;
                    sum_score += p->foodScore;
                    if (p->foodScore > best_score) {
                        best_score = p->foodScore;
                        best_bot = p;
                        best_bot_id = player_index;
                        best_bot_lifetime = p->lifeTime;
                    }
                    if (p->foodScore < min_score) min_score = p->foodScore;
                }
                player_index++;
            }
            float avg_score = alive_players > 0 ? float(sum_score) / alive_players : 0.0f;
            // Sidebar is now adaptive to screen size
            int sidebar_x = SCREEN_WIDTH + 20;
            renderText(renderer, font, "--- STATS ---", sidebar_x, y, green); y += 35;
            renderText(renderer, font, "Bots Alive: " + std::to_string(alive_players) + " / " + std::to_string(g_bot_count), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Hunters:   " + std::string(g_hunters_enabled ? "[X] " : "[ ] ") + std::to_string(total_hunters) + " / " + std::to_string(g_hunter_count), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Food:      " + std::to_string(total_food) + " / " + std::to_string(g_food_count), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Best Score: " + std::to_string(best_score), sidebar_x, y, yellow); y += 28;
            renderText(renderer, font, "Avg Score:  " + std::to_string(int(avg_score)), sidebar_x, y, cyan); y += 28;
            renderText(renderer, font, "Min Score:  " + std::to_string(min_score), sidebar_x, y, white); y += 28;
            if (best_bot) {
                renderText(renderer, font, "Best Bot:   #" + std::to_string(best_bot_id), sidebar_x, y, yellow); y += 22;
                SDL_Rect color_rect = {SCREEN_WIDTH + 20, y, 40, 20};
                SDL_SetRenderDrawColor(renderer, best_bot->color[0], best_bot->color[1], best_bot->color[2], 255);
                SDL_RenderFillRect(renderer, &color_rect);
                y += 22;
                renderText(renderer, font, "Lifetime:   " + std::to_string(best_bot_lifetime), sidebar_x, y, white); y += 28;
            }
            // Timer display
            Uint32 elapsed_ms = SDL_GetTicks() - sim_start_time;
            int seconds = elapsed_ms / 1000;
            int minutes = seconds / 60;
            seconds = seconds % 60;
            char timer_buf[32];
            snprintf(timer_buf, sizeof(timer_buf), "Time: %02d:%02d", minutes, seconds);
            renderText(renderer, font, timer_buf, sidebar_x, y, white); y += 28;
            // Speed display
            renderText(renderer, font, "Speed: LOGIC MAX", sidebar_x, y, white); y += 28;
            renderText(renderer, font, "game time: " + std::to_string(game_time_units/1000) + "(k)", sidebar_x, y, white); y += 28;
            SDL_RenderPresent(renderer);
            // LOGIC MAX: run logic-only updates as fast as possible, skip rendering
            logic_max_mode = true;
            SDL_Event logic_event;
            while (logic_max_mode && !quit) {
                game.update();
                game_time_units += sim_speed > 0 ? sim_speed : 1;
                while (SDL_PollEvent(&logic_event)) {
                    if (logic_event.type == SDL_QUIT) quit = true;
                    if (logic_event.type == SDL_KEYDOWN) {
                        if (logic_event.key.keysym.sym == SDLK_UP) {
                            speed_index = std::min((int)speed_steps.size() - 2, speed_index + 1); // Don't stay in LOGIC MAX
                            sim_speed = speed_steps[speed_index];
                            logic_max_mode = false;
                        } else if (logic_event.key.keysym.sym == SDLK_DOWN) {
                            speed_index = std::max(0, speed_index - 1);
                            sim_speed = speed_steps[speed_index];
                            logic_max_mode = false;
                        } else if (logic_event.key.keysym.sym == SDLK_ESCAPE) {
                            paused = !paused;
                            show_menu = paused;
                            show_settings = false;
                            logic_max_mode = false;
                        } else if (logic_event.key.keysym.sym == SDLK_q) {
                            quit = true;
                        }
                    }
                }
            }
            continue; // Skip rendering for this loop
        }
        if (!paused) {
            if (sim_speed == -1) {
                // MAX: run as many updates as possible for 10ms
                Uint32 start = SDL_GetTicks();
                int updates = 0;
                while (SDL_GetTicks() - start < 10) {
                    game.update();
                    ++updates;
                }
            } else {
                for (int i = 0; i < sim_speed; ++i) {
                    game.update();
                }
            }
        }
        // In the main simulation loop, periodically save the gene pool
        if (SDL_GetTicks() - last_gene_pool_save > GENE_POOL_SAVE_INTERVAL) {
            Player::save_gene_pool("gene_pool.txt");
            last_gene_pool_save = SDL_GetTicks();
        }
        // Rendering
        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255); // Darker background
        SDL_RenderClear(renderer);
        // Draw game area
        SDL_Rect game_area = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255); // Even darker
        SDL_RenderFillRect(renderer, &game_area);
        game.render();
        // Draw sidebar
        SDL_Rect sidebar = {SCREEN_WIDTH, 0, 200, SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &sidebar);
        // Sidebar stats
        SDL_Color white = {255,255,255,255};
        SDL_Color green = {0,255,0,255};
        SDL_Color yellow = {255,255,0,255};
        SDL_Color cyan = {0,255,255,255};
        int y = 20;
        int alive_players = 0, total_food = game.foods.size(), total_hunters = game.hunters.size();
        int best_score = 0, sum_score = 0, min_score = 1e6;
        Player* best_bot = nullptr;
        int best_bot_id = -1;
        int best_bot_lifetime = 0;
        float best_bot_size = 0.0f, avg_size = 0.0f;
        int player_index = 0;
        for (auto* p : game.players) {
            if (p->alive) {
                ++alive_players;
                sum_score += p->foodScore;
                avg_size += p->width;
                if (p->foodScore > best_score) {
                    best_score = p->foodScore;
                    best_bot = p;
                    best_bot_id = player_index;
                    best_bot_lifetime = p->lifeTime;
                    best_bot_size = p->width;
                }
                if (p->foodScore < min_score) min_score = p->foodScore;
            }
            player_index++;
        }
        avg_size = alive_players > 0 ? avg_size / alive_players : 0.0f;
        int sidebar_x = SCREEN_WIDTH + 20;
        // Setup and draw buttons
        auto setup_sidebar_buttons = [&](int sidebar_x, int y_start) {
            sidebar_buttons.clear();
            int btn_w = 48, btn_h = 32, gap = 12;
            int y = y_start;
            sidebar_buttons.push_back({{sidebar_x, y, 160, btn_h}, "Pause/Resume", "pause"}); y += btn_h + gap;
            sidebar_buttons.push_back({{sidebar_x, y, 160, btn_h}, "Settings", "settings"}); y += btn_h + gap;
            sidebar_buttons.push_back({{sidebar_x, y, 160, btn_h}, "Restart", "restart"}); y += btn_h + gap;
            // Speed row: '-' [speed] '+'
            int speed_row_y = y;
            sidebar_buttons.push_back({{sidebar_x, speed_row_y, btn_w, btn_h}, "-", "speed_down"});
            sidebar_buttons.push_back({{sidebar_x + btn_w + 8, speed_row_y, 56, btn_h}, "speed_display", "speed_display"}); // not clickable
            sidebar_buttons.push_back({{sidebar_x + btn_w + 8 + 56 + 8, speed_row_y, btn_w, btn_h}, "+", "speed_up"});
            y += btn_h + gap;
        };
        setup_sidebar_buttons(sidebar_x, y);
        for (size_t i = 0; i < sidebar_buttons.size(); ++i) {
            SDL_Rect btn = sidebar_buttons[i].rect;
            if (sidebar_buttons[i].action == "speed_display") {
                // Draw speed value centered
                std::string speed_str = (sim_speed == -2) ? "LOGIC" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
                SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
                SDL_RenderFillRect(renderer, &btn);
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                SDL_RenderDrawRect(renderer, &btn);
                int tw = 0, th = 0;
                // Center text
                renderText(renderer, font, speed_str, btn.x + 10, btn.y + 6, cyan);
            } else {
                SDL_Color btn_bg = (int)i == hovered_button ? SDL_Color{80, 120, 200, 255} : SDL_Color{60, 60, 80, 255};
                SDL_SetRenderDrawColor(renderer, btn_bg.r, btn_bg.g, btn_bg.b, btn_bg.a);
                SDL_RenderFillRect(renderer, &btn);
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                SDL_RenderDrawRect(renderer, &btn);
                renderText(renderer, font, sidebar_buttons[i].label, btn.x + 12, btn.y + 6, white);
            }
        }
        y += (int)sidebar_buttons.size() * (32 + 12) + 20;
        renderText(renderer, font, "--- STATS ---", sidebar_x, y, green); y += 35;
        renderText(renderer, font, "Bots Alive: " + std::to_string(alive_players) + " / " + std::to_string(g_bot_count), sidebar_x, y, white); y += 28;
        renderText(renderer, font, "Hunters:   " + std::string(g_hunters_enabled ? "[X] " : "[ ] ") + std::to_string(total_hunters) + " / " + std::to_string(g_hunter_count), sidebar_x, y, white); y += 28;
        renderText(renderer, font, "Food:      " + std::to_string(total_food) + " / " + std::to_string(g_food_count), sidebar_x, y, white); y += 28;
        renderText(renderer, font, "Best Score: " + std::to_string(best_score), sidebar_x, y, yellow); y += 28;
        renderText(renderer, font, "Avg Score:  " + std::to_string(int(alive_players > 0 ? float(sum_score) / alive_players : 0.0f)), sidebar_x, y, cyan); y += 28;
        renderText(renderer, font, "Min Score:  " + std::to_string(min_score), sidebar_x, y, white); y += 28;
        if (best_bot) {
            renderText(renderer, font, "Best Bot:   #" + std::to_string(best_bot_id), sidebar_x, y, yellow); y += 22;
            SDL_Rect color_rect = {SCREEN_WIDTH + 20, y, 40, 20};
            SDL_SetRenderDrawColor(renderer, best_bot->color[0], best_bot->color[1], best_bot->color[2], 255);
            SDL_RenderFillRect(renderer, &color_rect);
            y += 22;
            renderText(renderer, font, "Lifetime:   " + std::to_string(best_bot_lifetime), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Size:       " + std::to_string(int(best_bot_size)), sidebar_x, y, cyan); y += 28;
        }
        renderText(renderer, font, "Avg Size:   " + std::to_string(int(avg_size)), sidebar_x, y, cyan); y += 28;
        // Timer display
        Uint32 elapsed_ms = SDL_GetTicks() - sim_start_time;
        int seconds = elapsed_ms / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;
        char timer_buf[32];
        snprintf(timer_buf, sizeof(timer_buf), "Time: %02d:%02d", minutes, seconds);
        renderText(renderer, font, timer_buf, sidebar_x, y, white); y += 28;
        renderText(renderer, font, "game time: " + std::to_string(game_time_units/1000) + "(k)", sidebar_x, y, white); y += 28;
        std::string speed_str = (sim_speed == -2) ? "LOGIC MAX" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
        renderText(renderer, font, "Speed: " + speed_str, sidebar_x, y, white); y += 28;
        if (paused && show_settings) {
            // Enhanced settings panel
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
            SDL_Rect overlay = {SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 180, 440, 360};
            SDL_RenderFillRect(renderer, &overlay);
            renderText(renderer, font, "--- SETTINGS ---", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 160, green);
            int sy = SCREEN_HEIGHT/2 - 120;
            renderText(renderer, font, "B/N: Bots +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_bot_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "F/G: Food +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_food_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "H: Toggle Hunters", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, pending_hunters_enabled ? "[X]" : "[ ]", SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "J/K: Hunters +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_hunter_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "P: Toggle Human Player", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, pending_player_enabled ? "[X]" : "[ ]", SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            std::string speed_str = (sim_speed == -2) ? "LOGIC MAX" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
            renderText(renderer, font, "Speed (UP/DOWN):", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, speed_str, SCREEN_WIDTH/2 + 100, sy, cyan); sy += 32;
            renderText(renderer, font, "ESC: Apply & Close", SCREEN_WIDTH/2 - 80, sy + 32, cyan);
        }
        if (paused && show_menu && !show_settings) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
            SDL_Rect overlay = {SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 120, 400, 240};
            SDL_RenderFillRect(renderer, &overlay);
            renderText(renderer, font, "C++ Evolution Simulation", SCREEN_WIDTH/2 - 120, SCREEN_HEIGHT/2 - 100, green);
            renderText(renderer, font, "Press SPACE to Start", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 60, white);
            renderText(renderer, font, "ESC: Pause/Resume", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 - 30, white);
            renderText(renderer, font, "R: Restart", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 + 30, white);
            renderText(renderer, font, "UP/DOWN: Speed", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 + 60, white);
            renderText(renderer, font, "Close window to exit", SCREEN_WIDTH/2 - 90, SCREEN_HEIGHT/2 + 90, white);
        }
        SDL_RenderPresent(renderer);
    }
    // At exit, save the gene pool one last time
    Player::save_gene_pool("gene_pool.txt");
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
} 