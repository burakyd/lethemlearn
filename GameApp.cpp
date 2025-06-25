#include "GameApp.h"
#include "Food.h"
#include "Player.h"
#include "Hunter.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <iomanip>
#include <sstream>

extern int game_time_units;

GameApp::GameApp() {}
GameApp::~GameApp() {}

bool GameApp::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    window = SDL_CreateWindow("C++ AI Simulation", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH + 200, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    font = TTF_OpenFont("arial.ttf", 18);
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    // Initialize simulation parameters
    g_bot_count = MIN_BOT;
    g_food_count = NUMBER_OF_FOODS;
    g_hunters_enabled = true;
    g_hunter_count = HUNTERS;
    g_player_enabled = PLAYER_ENABLED;
    pending_bot_count = g_bot_count;
    pending_food_count = g_food_count;
    pending_hunters_enabled = g_hunters_enabled;
    pending_hunter_count = g_hunter_count;
    pending_player_enabled = g_player_enabled;
    speed_steps = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, -1, -2};
    speed_index = 0;
    sim_speed = speed_steps[speed_index];
    logic_max_mode = false;
    show_menu = true;
    show_settings = false;
    sim_start_time = SDL_GetTicks();
    last_gene_pool_save = SDL_GetTicks();
    // Load gene pool
    Player::load_gene_pool("gene_pool.txt");
    // Create game
    game = new Game(renderer);
    restart_simulation();
    return true;
}

void GameApp::cleanup() {
    Player::save_gene_pool("gene_pool.txt");
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    if (game) delete game;
}

// Helper to render text
void GameApp::renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void GameApp::restart_simulation(const std::vector<std::vector<std::vector<float>>>* loaded_genes, const std::vector<std::vector<float>>* best_gene) {
    for (auto* p : game->players) delete p;
    for (auto* f : game->foods) delete f;
    game->players.clear();
    game->hunters.clear();
    game->foods.clear();
    g_bot_count = std::max(g_bot_count, MIN_BOT);
    int bots_to_spawn = g_bot_count;
    if (g_player_enabled) {
        game->players.push_back(new HumanPlayer(DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SCREEN_WIDTH/2, SCREEN_HEIGHT/2));
        bots_to_spawn -= 1;
    }
    if (loaded_genes && !loaded_genes->empty()) {
        int used = 0;
        for (const auto& genes : *loaded_genes) {
            if (bots_to_spawn <= 0) break;
            game->players.push_back(new Player(genes, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
            used++;
            bots_to_spawn--;
        }
        if (used < bots_to_spawn)
            game->newPlayer(bots_to_spawn - used, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SPEED, true, true);
    } else if (best_gene && !best_gene->empty()) {
        for (int i = 0; i < bots_to_spawn; ++i) {
            game->players.push_back(new Player(*best_gene, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
        }
    } else {
        game->newPlayer(bots_to_spawn, DOT_WIDTH, DOT_HEIGHT, DOT_COLOR, SPEED, true, true);
    }
    if (g_hunters_enabled) {
        game->newHunter(g_hunter_count, HUNTER_WIDTH, HUNTER_HEIGHT, HUNTER_COLOR, SPEED, false, false);
    }
    game->randomFood(g_food_count);
}

void GameApp::run() {
    SDL_Event e;
    quit = false;
    paused = true;
    sim_start_time = SDL_GetTicks();
    last_gene_pool_save = SDL_GetTicks();
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (paused) {
                    // check keys for paused state - settings
                    switch (e.key.keysym.sym) {
                        case SDLK_b:
                            pending_bot_count = std::max(1, pending_bot_count - 1);
                            break;
                        case SDLK_n:
                            pending_bot_count = std::min(200, pending_bot_count + 1);
                            break;
                        case SDLK_f:
                            pending_food_count = std::max(1, pending_food_count - 1);
                            break;
                        case SDLK_g:
                            pending_food_count = std::min(200, pending_food_count + 1);
                            break;
                        case SDLK_h:
                            pending_hunters_enabled = !pending_hunters_enabled;
                            break;
                        case SDLK_j:
                            pending_hunter_count = std::max(0, pending_hunter_count - 1);
                            break;
                        case SDLK_k:
                            pending_hunter_count = std::min(50, pending_hunter_count + 1);
                            break;
                        case SDLK_p:
                            pending_player_enabled = !pending_player_enabled;
                            break;
                        case SDLK_ESCAPE: {
                            // Only Restart if a setting is changed
                            bool changed = (g_bot_count != pending_bot_count ||
                                            g_food_count != pending_food_count ||
                                            g_hunters_enabled != pending_hunters_enabled ||
                                            g_hunter_count != pending_hunter_count ||
                                            g_player_enabled != pending_player_enabled);
                            if (changed) {
                                g_bot_count = pending_bot_count;
                                g_food_count = pending_food_count;
                                g_hunters_enabled = pending_hunters_enabled;
                                g_hunter_count = pending_hunter_count;
                                g_player_enabled = pending_player_enabled;
                                restart_simulation();
                            }
                            paused = false;
                            break;
                        }
                        case SDLK_r:
                            restart_simulation();
                            break;
                        case SDLK_UP:
                            speed_index = std::min((int)speed_steps.size() - 1, speed_index + 1);
                            sim_speed = speed_steps[speed_index];
                            logic_max_mode = (sim_speed == -2);
                            break;
                        case SDLK_DOWN:
                            speed_index = std::max(0, speed_index - 1);
                            sim_speed = speed_steps[speed_index];
                            logic_max_mode = false;
                            break;
                    }
                } else {
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        paused = true;
                    } else if (e.key.keysym.sym == SDLK_r) {
                        restart_simulation();
                    } else if (e.key.keysym.sym == SDLK_UP) {
                        speed_index = std::min((int)speed_steps.size() - 1, speed_index + 1);
                        sim_speed = speed_steps[speed_index];
                        logic_max_mode = (sim_speed == -2);
                    } else if (e.key.keysym.sym == SDLK_DOWN) {
                        speed_index = std::max(0, speed_index - 1);
                        sim_speed = speed_steps[speed_index];
                        logic_max_mode = false;
                    }
                }
            }
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
                if (action == "pause") { paused = !paused; }
                else if (action == "settings") {
                    paused = true;
                    pending_bot_count = g_bot_count;
                    pending_food_count = g_food_count;
                    pending_hunters_enabled = g_hunters_enabled;
                    pending_hunter_count = g_hunter_count;
                    pending_player_enabled = g_player_enabled;
                }
                else if (action == "restart") restart_simulation();
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
            SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
            SDL_RenderClear(renderer);
            SDL_Rect game_area = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
            SDL_RenderFillRect(renderer, &game_area);
            game->render();
            SDL_Rect sidebar = {SCREEN_WIDTH, 0, 200, SCREEN_HEIGHT};
            SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
            SDL_RenderFillRect(renderer, &sidebar);
            SDL_Color white = {255,255,255,255};
            SDL_Color green = {0,255,0,255};
            SDL_Color yellow = {255,255,0,255};
            SDL_Color cyan = {0,255,255,255};
            int y = 20;
            int alive_players = 0, total_food = game->foods.size(), total_hunters = game->hunters.size();
            int best_bot_id = -1;
            int best_bot_lifetime = 0;
            int player_index = 0;
            for (auto* p : game->players) {
                if (p->alive) {
                    ++alive_players;
                    best_bot_id = player_index;
                    best_bot_lifetime = p->lifeTime;
                }
                player_index++;
            }
            int sidebar_x = SCREEN_WIDTH + 20;
            renderText(renderer, font, "     --- STATS ---", sidebar_x, y, green); y += 35;
            renderText(renderer, font, "Bots Alive: " + std::to_string(alive_players) + " / " + std::to_string(g_bot_count), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Hunters:   " + std::string(g_hunters_enabled ? "[X] " : "[ ] ") + std::to_string(total_hunters) + " / " + std::to_string(g_hunter_count), sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Food:      " + std::to_string(total_food) + " / " + std::to_string(g_food_count), sidebar_x, y, white); y += 28;
            Uint32 elapsed_ms = SDL_GetTicks() - sim_start_time;
            int seconds = elapsed_ms / 1000;
            int minutes = seconds / 60;
            seconds = seconds % 60;
            char timer_buf[32];
            snprintf(timer_buf, sizeof(timer_buf), "Time: %02d:%02d", minutes, seconds);
            renderText(renderer, font, timer_buf, sidebar_x, y, white); y += 28;
            renderText(renderer, font, "Speed: LOGIC MAX", sidebar_x, y, white); y += 28;
            renderText(renderer, font, "game time: " + std::to_string(game_time_units/1000) + "(k)", sidebar_x, y, white); y += 28;
            SDL_RenderPresent(renderer);
            logic_max_mode = true;
            SDL_Event logic_event;
            while (logic_max_mode && !quit) {
                game->update();
                while (SDL_PollEvent(&logic_event)) {
                    if (logic_event.type == SDL_QUIT) quit = true;
                    if (logic_event.type == SDL_KEYDOWN) {
                        if (logic_event.key.keysym.sym == SDLK_DOWN) {
                            speed_index = std::max(0, speed_index - 1);
                            sim_speed = speed_steps[speed_index];
                            logic_max_mode = false;
                        } else if (logic_event.key.keysym.sym == SDLK_ESCAPE) {
                            paused = !paused;
                            logic_max_mode = false;
                        } else if (logic_event.key.keysym.sym == SDLK_q) {
                            quit = true;
                        }
                    }
                }
            }
            continue;
        }
        if (!paused) {
            if (sim_speed == -1) {
                Uint32 start = SDL_GetTicks();
                int updates = 0;
                while (SDL_GetTicks() - start < 10) {
                    game->update();
                    ++updates;
                }
            } else {
                for (int i = 0; i < sim_speed; ++i) {
                    game->update();
                }
            }
        }
        if (SDL_GetTicks() - last_gene_pool_save > GENE_POOL_SAVE_INTERVAL) {
            Player::save_gene_pool("gene_pool.txt");
            last_gene_pool_save = SDL_GetTicks();
        }
        SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
        SDL_RenderClear(renderer);
        SDL_Rect game_area = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
        SDL_RenderFillRect(renderer, &game_area);
        game->render();
        SDL_Rect sidebar = {SCREEN_WIDTH, 0, 200, SCREEN_HEIGHT};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &sidebar);
        SDL_Color white = {255,255,255,255};
        SDL_Color green = {0,255,0,255};
        SDL_Color yellow = {255,255,0,255};
        SDL_Color cyan = {0,255,255,255};
        int y = 20;
        int alive_players = 0, total_food = game->foods.size(), total_hunters = game->hunters.size();
        int player_index = 0;
        for (auto* p : game->players) {
            if (p->alive) {
                ++alive_players;
            }
            player_index++;
        }
        int sidebar_x = SCREEN_WIDTH + 20;
        auto setup_sidebar_buttons = [&](int sidebar_x, int y_start) {
            sidebar_buttons.clear();
            int btn_w = 48, btn_h = 32, gap = 12;
            int y = y_start;
            sidebar_buttons.push_back({{sidebar_x, y, 160, btn_h}, "Pause/Resume", "pause"}); y += btn_h + gap;
            sidebar_buttons.push_back({{sidebar_x, y, 160, btn_h}, "Restart", "restart"}); y += btn_h + gap;
            int speed_row_y = y;
            sidebar_buttons.push_back({{sidebar_x, speed_row_y, btn_w, btn_h}, "-", "speed_down"});
            sidebar_buttons.push_back({{sidebar_x + btn_w + 8, speed_row_y, 56, btn_h}, "speed_display", "speed_display"});
            sidebar_buttons.push_back({{sidebar_x + btn_w + 8 + 56 + 8, speed_row_y, btn_w, btn_h}, "+", "speed_up"});
            y += btn_h + gap;
        };
        setup_sidebar_buttons(sidebar_x, y);
        for (size_t i = 0; i < sidebar_buttons.size(); ++i) {
            SDL_Rect btn = sidebar_buttons[i].rect;
            if (sidebar_buttons[i].action == "speed_display") {
                std::string speed_str = (sim_speed == -2) ? "LOGIC" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
                SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
                SDL_RenderFillRect(renderer, &btn);
                SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                SDL_RenderDrawRect(renderer, &btn);
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
        int text_width = 0, text_height = 0;
        TTF_SizeText(font, "--- STATS ---", &text_width, &text_height);
        int sidebar_width = 200;
        int centered_x = SCREEN_WIDTH + (sidebar_width - text_width) / 2;
        renderText(renderer, font, "--- STATS ---", centered_x, y, green); y += 35;
        renderText(renderer, font, "Bots Alive: " + std::to_string(alive_players) + " / " + std::to_string(g_bot_count), sidebar_x, y, white); y += 28;
        renderText(renderer, font, "Hunters:   " + std::string(g_hunters_enabled ? "[X] " : "[ ] ") + std::to_string(total_hunters) + " / " + std::to_string(g_hunter_count), sidebar_x, y, white); y += 28;
        renderText(renderer, font, "Food:      " + std::to_string(total_food) + " / " + std::to_string(g_food_count), sidebar_x, y, white); y += 28;
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
        if (paused) {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
            SDL_Rect overlay = {SCREEN_WIDTH/2 - 220, SCREEN_HEIGHT/2 - 220, 440, 440};
            SDL_RenderFillRect(renderer, &overlay);
            int menu_text_width = 0, menu_text_height = 0;
            TTF_SizeText(font, "--- MENU & SETTINGS ---", &menu_text_width, &menu_text_height);
            int menu_centered_x = SCREEN_WIDTH/2 - 220 + (440 - menu_text_width) / 2;
            renderText(renderer, font, "--- MENU & SETTINGS ---", menu_centered_x, SCREEN_HEIGHT/2 - 200, green);
            int sy = SCREEN_HEIGHT/2 - 160;
            renderText(renderer, font, "B/N: Bots +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_bot_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "F/G: Food +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_food_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "H: Toggle Hunters", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, pending_hunters_enabled ? "[X]" : "[ ]", SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "J/K: Hunters +/-", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, std::to_string(pending_hunter_count), SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            renderText(renderer, font, "P: Toggle Human Player", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, pending_player_enabled ? "[X]" : "[ ]", SCREEN_WIDTH/2 + 100, sy, yellow); sy += 32;
            std::string speed_str = (sim_speed == -2) ? "LOGIC MAX" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
            renderText(renderer, font, "Speed (UP/DOWN):", SCREEN_WIDTH/2 - 180, sy, white); renderText(renderer, font, speed_str, SCREEN_WIDTH/2 + 100, sy, cyan); sy += 32;
            renderText(renderer, font, "ESC: Apply & Resume", SCREEN_WIDTH/2 - 80, sy + 32, cyan);
            renderText(renderer, font, "R: Restart", SCREEN_WIDTH/2 - 80, sy + 64, yellow);
        }
        // Find best 3 bots and human player
        std::vector<std::pair<Player*, int>> bot_stats; // (player, index)
        Player* human_player = nullptr;
        int idx = 0;
        for (auto* p : game->players) {
            if (p->alive) {
                if (p->is_human) {
                    human_player = p;
                } else {
                    bot_stats.emplace_back(p, idx);
                }
            }
            ++idx;
        }
        // Sort bots by foodCount descending
        std::sort(bot_stats.begin(), bot_stats.end(), [](const auto& a, const auto& b) {
            return a.first->foodCount > b.first->foodCount;
        });
        // Show best 3 bots stats with column headers and compact rows
        y += 10;
        renderText(renderer, font, "Top Bots:", sidebar_x, y, yellow); y += 18;
        // Column headers
        renderText(renderer, font, "  S    F   L(k)", sidebar_x + 24, y, cyan); y += 16;
        for (int i = 0; i < std::min(5, (int)bot_stats.size()); ++i) {
            y += 10;
            Player* bot = bot_stats[i].first;
            SDL_Rect color_rect = {sidebar_x, y, 14, 14};
            SDL_SetRenderDrawColor(renderer, bot->color[0], bot->color[1], bot->color[2], 255);
            SDL_RenderFillRect(renderer, &color_rect); y -= 4; // for a better look
            // Only show values, not labels or bot number
            std::ostringstream oss;
            oss << std::setw(4) << std::setfill(' ') << bot->width << " "
                << std::setw(3) << std::setfill(' ') << bot->foodCount << " "
                << std::setw(4) << std::fixed << std::setprecision(1) << (bot->lifeTime / 1000.0f);
            renderText(renderer, font, oss.str(), sidebar_x + 24, y, white); y += 15;
        }
        // Show human player stats
        if (g_player_enabled && human_player) {
            y += 8;
            renderText(renderer, font, "Human Player:", sidebar_x, y, cyan); y += 20;
            SDL_Rect color_rect = {sidebar_x, y, 18, 18};
            SDL_SetRenderDrawColor(renderer, human_player->color[0], human_player->color[1], human_player->color[2], 255);
            SDL_RenderFillRect(renderer, &color_rect);
            std::string hp_info = "S:" + std::to_string(human_player->width) + " F:" + std::to_string(human_player->foodCount);
            renderText(renderer, font, hp_info, sidebar_x + 24, y, white); y += 20;
        }
        SDL_RenderPresent(renderer);
    }
} 