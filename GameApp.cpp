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
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <vector>
#include "Game.h"
#include "Settings.h"
#include <cmath>
#include <filesystem>

extern int game_time_units;

GameApp::GameApp(bool headless, const std::string& gene_pool_file, int island_id, const std::string& migration_dir)
    : headless(headless), gene_pool_file(gene_pool_file), island_id(island_id), migration_dir(migration_dir) {}

GameApp::~GameApp() {}

bool GameApp::init() {
    // Print essential configuration values from Settings.h (ASCII formatting)
    const char* hline = "-----------------------------------------------------------------------------------------------------------------------------------------------";
    std::cout << "\n+" << hline << "+\n";
    std::cout << "|" << std::setw(80) << std::left << " Simulation Configuration " << hline + 80 << "|\n";
    auto print_section = [](const std::string& title) {
        std::cout << "|\n|  " << std::left << std::setw(30) << title << std::setw(110 - title.size()) << " " << "|\n";
    };
    auto print_kv = [](const std::string& key, auto value) {
        std::cout << "|    " << std::left << std::setw(36) << key << ": " << std::setw(12) << value << "|\n";
    };
    print_section("[Display]");
    print_kv("SCREEN_WIDTH", SCREEN_WIDTH); print_kv("SCREEN_HEIGHT", SCREEN_HEIGHT); print_kv("SPEED", SPEED);
    print_section("[Player]");
    print_kv("DOT_WIDTH", DOT_WIDTH); print_kv("DOT_HEIGHT", DOT_HEIGHT); print_kv("RANDOM_SIZE_MIN", RANDOM_SIZE_MIN); print_kv("RANDOM_SIZE_MAX", RANDOM_SIZE_MAX);
    print_kv("MAX_SPEED", MAX_SPEED); print_kv("MAX_PLAYER_SIZE", MAX_PLAYER_SIZE);
    print_kv("PLAYER_MIN_SPEED_FACTOR", PLAYER_MIN_SPEED_FACTOR); print_kv("PLAYER_SIZE_SPEED_EXPONENT", PLAYER_SIZE_SPEED_EXPONENT); print_kv("PLAYER_GROWTH_EXPONENT", PLAYER_GROWTH_EXPONENT);
    print_section("[Food]");
    print_kv("FOOD_WIDTH", FOOD_WIDTH); print_kv("FOOD_HEIGHT", FOOD_HEIGHT); print_kv("FOOD_APPEND", FOOD_APPEND);
    print_section("[Fitness]");
    print_kv("FITNESS_WEIGHT_FOOD", FITNESS_WEIGHT_FOOD); print_kv("FITNESS_WEIGHT_LIFE", FITNESS_WEIGHT_LIFE);
    print_kv("FITNESS_WEIGHT_EXPLORE", FITNESS_WEIGHT_EXPLORE); print_kv("FITNESS_WEIGHT_PLAYERS", FITNESS_WEIGHT_PLAYERS);
    print_kv("FITNESS_MIN_FOOD", FITNESS_MIN_FOOD); print_kv("FITNESS_MIN_LIFE", FITNESS_MIN_LIFE); print_kv("FITNESS_EARLY_DEATH_TIME", FITNESS_EARLY_DEATH_TIME); print_kv("FITNESS_EARLY_DEATH_PENALTY", FITNESS_EARLY_DEATH_PENALTY);
    print_kv("FITNESS_MIN_FOR_REPRO", FITNESS_MIN_FOR_REPRO); print_kv("FITNESS_MIN_LIFETIME_FOR_REPRO", FITNESS_MIN_LIFETIME_FOR_REPRO);
    print_kv("FITNESS_DIVERSITY_PRUNE_MIN_DIST", FITNESS_DIVERSITY_PRUNE_MIN_DIST); print_kv("MIN_FITNESS_FOR_GENE_POOL", MIN_FITNESS_FOR_GENE_POOL);
    std::cout << "+" << hline << "+\n\n";
    if (!headless) {
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
    Player::load_gene_pool(gene_pool_file);
    // Create game
    game = new Game(renderer);
    restart_simulation();
    return true;
}

void GameApp::cleanup() {
    Player::save_gene_pool(gene_pool_file);
    if (!headless) {
        if (font) TTF_CloseFont(font);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
    }
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
            SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
            game->players.push_back(new Player(genes, std::vector<std::vector<float>>(genes.size()), DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
            used++;
            bots_to_spawn--;
        }
        if (used < bots_to_spawn) {
            for (int i = 0; i < bots_to_spawn - used; ++i) {
                auto [genes, biases] = random_genes_and_biases();
                SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
                game->newPlayer(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, SPEED);
            }
        }
    } else if (best_gene && !best_gene->empty()) {
        for (int i = 0; i < bots_to_spawn; ++i) {
            SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
            game->players.push_back(new Player(*best_gene, std::vector<std::vector<float>>(best_gene->size()), DOT_WIDTH, DOT_HEIGHT, color, static_cast<float>(rand() % SCREEN_WIDTH), static_cast<float>(rand() % SCREEN_HEIGHT)));
        }
    } else {
        for (int i = 0; i < bots_to_spawn; ++i) {
            auto [genes, biases] = random_genes_and_biases();
            SDL_Color color = {static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), static_cast<Uint8>(rand() % 256), 255};
            game->newPlayer(genes, biases, DOT_WIDTH, DOT_HEIGHT, color, SPEED);
        }
    }
    if (g_hunters_enabled) {
        game->newHunter(g_hunter_count, HUNTER_WIDTH, HUNTER_HEIGHT, HUNTER_COLOR, SPEED, false, false);
    }
    game->randomFood(g_food_count);
    // Reset mutation rate and update display after restart
    Player::adaptive_mutation_rate = MUTATION_RATE;
    Player::set_display_mutation_rate(Player::adaptive_mutation_rate);
}

// --- Mouse interaction state for settings ---
struct SettingSlider {
    std::string label;
    int* value;
    int min, max;
    SDL_Rect bar, handle;
    bool active = false;
};
struct SettingToggle {
    std::string label;
    bool* value;
    SDL_Rect box;
    bool active = false;
};
std::vector<SettingSlider> sliders;
std::vector<SettingToggle> toggles;
int dragging_slider = -1;

// Helper for migration file naming
std::string get_migration_out_file(int island_id, const std::string& migration_dir) {
    return migration_dir + "/migrants_from_" + std::to_string(island_id) + ".dat";
}
std::string get_migration_in_file(int island_id, const std::string& migration_dir) {
    return migration_dir + "/migrants_to_" + std::to_string(island_id) + ".dat";
}

// Helper to export/import a subset of the gene pool
void export_migrants(const std::string& filename, int num = 5) {
    std::ofstream ofs(filename);
    int count = 0;
    for (const auto& entry : Player::gene_pool) {
        if (count++ >= num) break;
        ofs << entry.fitness << "\n";
        for (const auto& layer : entry.genes) {
            for (float w : layer) ofs << w << ' ';
            ofs << '\n';
        }
        for (const auto& bias : entry.biases) {
            for (float b : bias) ofs << b << ' ';
            ofs << '\n';
        }
        ofs << "END\n";
    }
}
void import_migrants(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) return;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line == "END") continue;
        float fitness = std::stof(line);
        std::vector<std::vector<float>> genes, biases;
        for (int l = 0; l < 4; ++l) {
            std::getline(ifs, line);
            std::istringstream iss(line);
            std::vector<float> layer;
            float w;
            while (iss >> w) layer.push_back(w);
            genes.push_back(layer);
        }
        for (int l = 0; l < 4; ++l) {
            std::getline(ifs, line);
            std::istringstream iss(line);
            std::vector<float> bias;
            float b;
            while (iss >> b) bias.push_back(b);
            biases.push_back(bias);
        }
        Player::try_insert_gene_to_pool(fitness, genes, biases);
    }
}

void log_fitness(int island_id, const std::string& migration_dir) {
    std::string log_file = migration_dir + "/fitness_log_island_" + std::to_string(island_id) + ".txt";
    std::ofstream ofs(log_file, std::ios::app);
    ofs << "Best: " << Player::display_best_fitness
        << ", Avg: " << Player::display_avg_fitness
        << ", Last: " << Player::display_last_fitness
        << ", PoolSize: " << Player::gene_pool.size()
        << ", Diversity: " << Player::display_avg_diversity
        << ", Time: " << SDL_GetTicks() << std::endl;
}

void GameApp::run() {
    static const int MIGRATION_INTERVAL = 40000; // generations
    static const int MIGRANT_COUNT = 5;
    int generation = 0;
    if (headless) {
        quit = false;
        sim_start_time = SDL_GetTicks();
        last_gene_pool_save = SDL_GetTicks();
        while (!quit) {
            // Check for stop signal from master
            if (!migration_dir.empty() && island_id >= 0) {
                std::string stop_file = migration_dir + "/stop_island_" + std::to_string(island_id);
                if (std::filesystem::exists(stop_file)) {
                    std::cout << "[Island " << island_id << "] Stop signal received. Exiting." << std::endl;
                    break;
                }
            }
            // Run simulation logic only
            game->update();
            ++generation;
            // Migration logic
            if (!migration_dir.empty() && island_id >= 0 && generation % MIGRATION_INTERVAL == 0) {
                // Export migrants
                std::string out_file = get_migration_out_file(island_id, migration_dir);
                export_migrants(out_file, MIGRANT_COUNT);
                // Wait for incoming migrants
                std::string in_file = get_migration_in_file(island_id, migration_dir);
                int wait_count = 0;
                while (!std::filesystem::exists(in_file) && wait_count < 10000) { // up to ~10s
                    SDL_Delay(1);
                    ++wait_count;
                }
                if (std::filesystem::exists(in_file)) {
                    import_migrants(in_file);
                    std::filesystem::remove(in_file);
                }
                // Log fitness and gene pool distribution
                log_fitness(island_id, migration_dir);
            }
            // Save gene pool periodically
            if (SDL_GetTicks() - last_gene_pool_save > GENE_POOL_SAVE_INTERVAL) {
                Player::save_gene_pool(gene_pool_file);
                last_gene_pool_save = SDL_GetTicks();
            }
        }
        return;
    }
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
                // MAX - do as much as updates while keeping the rendering up
                Uint32 start = SDL_GetTicks();
                int updates = 0;
                while (SDL_GetTicks() - start < 10) {
                    game->update();
                    ++updates;
                }
            } else {
                // <sim_speed>X - do sim_speed updates before rendering 
                for (int i = 0; i < sim_speed; ++i) {
                    game->update();
                }
            }
        }
        if (SDL_GetTicks() - last_gene_pool_save > GENE_POOL_SAVE_INTERVAL) {
            Player::save_gene_pool(gene_pool_file);
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
        renderText(renderer, font, "--- STATS ---", centered_x, y, green); y += 30;
        renderText(renderer, font, "Bots Alive: " + std::to_string(alive_players) + " / " + std::to_string(g_bot_count), sidebar_x, y, white); y += 22;
        renderText(renderer, font, "Hunters:   " + std::string(g_hunters_enabled ? "[X] " : "[ ] ") + std::to_string(total_hunters) + " / " + std::to_string(g_hunter_count), sidebar_x, y, white); y += 22;
        renderText(renderer, font, "Food:      " + std::to_string(total_food) + " / " + std::to_string(g_food_count), sidebar_x, y, white); y += 22;
        Uint32 elapsed_ms = SDL_GetTicks() - sim_start_time;
        int seconds = elapsed_ms / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;
        char timer_buf[32];
        snprintf(timer_buf, sizeof(timer_buf), "Time: %02d:%02d", minutes, seconds);
        renderText(renderer, font, timer_buf, sidebar_x, y, white); y += 22;
        renderText(renderer, font, "game time: " + std::to_string(game_time_units/1000) + "(k)", sidebar_x, y, white); y += 22;
        std::string speed_str = (sim_speed == -2) ? "LOGIC MAX" : (sim_speed == -1) ? "MAX" : (std::to_string(sim_speed) + "x");
        renderText(renderer, font, "Speed: " + speed_str, sidebar_x, y, white); y += 22;
        // Show top bots and human player stats after main stats, before fitness/diversity/mutation info
        y += 8;
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
        renderText(renderer, font, "Top Bots:", sidebar_x, y, yellow); y += 18;
        renderText(renderer, font, "  S    F   L(k)", sidebar_x + 24, y, cyan); y += 16;
        for (int i = 0; i < std::min(5, (int)bot_stats.size()); ++i) {
            y += 10;
            Player* bot = bot_stats[i].first;
            SDL_Rect color_rect = {sidebar_x, y, 14, 14};
            SDL_SetRenderDrawColor(renderer, bot->color.r, bot->color.g, bot->color.b, 255);
            SDL_RenderFillRect(renderer, &color_rect); y -= 4; // for a better look
            std::ostringstream oss;
            oss << std::setw(4) << std::setfill(' ') << bot->width << " "
                << std::setw(3) << std::setfill(' ') << bot->foodCount << " "
                << std::setw(4) << std::fixed << std::setprecision(1) << (bot->lifeTime / 1000.0f);
            renderText(renderer, font, oss.str(), sidebar_x + 24, y, white); y += 15;
        }
        if (g_player_enabled && human_player) {
            y += 8;
            renderText(renderer, font, "Human Player:", sidebar_x, y, cyan); y += 20;
            SDL_Rect color_rect = {sidebar_x, y, 18, 18};
            SDL_SetRenderDrawColor(renderer, human_player->color.r, human_player->color.g, human_player->color.b, 255);
            SDL_RenderFillRect(renderer, &color_rect);
            std::string hp_info = "S:" + std::to_string(human_player->width) + " F:" + std::to_string(human_player->foodCount);
            renderText(renderer, font, hp_info, sidebar_x + 24, y, white); y += 20;
        }
        // After stats, print fitness/diversity/mutation info
        y += 6;
        renderText(renderer, font, "FITNESS", sidebar_x, y, yellow); y += 18;
        renderText(renderer, font, "best: " + std::to_string(int(Player::display_best_fitness)), sidebar_x, y, white); y += 15;
        renderText(renderer, font, "avg:  " + std::to_string(int(Player::display_avg_fitness)), sidebar_x, y, white); y += 15;
        renderText(renderer, font, "last: " + std::to_string(int(Player::display_last_fitness)), sidebar_x, y, white); y += 15;
        float diversity = std::round(Player::display_avg_diversity * 1000.0f) / 1000.0f;
        float mutation = std::round(Player::display_mutation_rate * 10000.0f) / 10000.0f;
        std::string diversity_str = std::to_string(diversity);
        diversity_str = diversity_str.substr(0, diversity_str.find(".") + 5);
        std::string mutation_str = std::to_string(mutation);
        mutation_str = mutation_str.substr(0, mutation_str.find(".") + 3);
        renderText(renderer, font, "avg div: " + diversity_str, sidebar_x, y, white); y += 15;
        renderText(renderer, font, "mut rate: " + mutation_str, sidebar_x, y, white); y += 15;
        // --- Enhanced Settings Panel ---
        if (paused) {
            sliders.clear();
            toggles.clear();
            int sx = SCREEN_WIDTH/2 - 180, sw = 160, sh = 12, th = 32;
            int sy = SCREEN_HEIGHT/2 - 160;
            sliders.push_back({"Bots", &pending_bot_count, 1, 200, {sx + 140, sy + 10, sw, sh}, {}, false}); sy += th;
            sliders.push_back({"Food", &pending_food_count, 1, 200, {sx + 140, sy + 10, sw, sh}, {}, false}); sy += th;
            toggles.push_back({"Hunters", &pending_hunters_enabled, {sx + 140, sy + 6, 48, 20}, false}); sy += th;
            sliders.push_back({"Hunter Count", &pending_hunter_count, 0, 50, {sx + 140, sy + 10, sw, sh}, {}, false}); sy += th;
            toggles.push_back({"Human Player", &pending_player_enabled, {sx + 140, sy + 6, 48, 20}, false}); sy += th;
            // Draw sliders and toggles in the same order as above
            int draw_sy = SCREEN_HEIGHT/2 - 160;
            size_t slider_idx = 0, toggle_idx = 0;
            for (int row = 0; row < 5; ++row) {
                if (row == 0) { // Bots slider
                    auto& s = sliders[slider_idx++];
                    renderText(renderer, font, s.label, sx, draw_sy, white);
                    SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
                    SDL_RenderFillRect(renderer, &s.bar);
                    float t = float(*s.value - s.min) / float(s.max - s.min);
                    int handle_x = s.bar.x + int(t * (s.bar.w - 16));
                    s.handle = {handle_x, s.bar.y - 4, 16, s.bar.h + 8};
                    SDL_SetRenderDrawColor(renderer, s.active ? 180 : 120, s.active ? 180 : 120, 255, 255);
                    SDL_RenderFillRect(renderer, &s.handle);
                    renderText(renderer, font, std::to_string(*s.value), s.bar.x + s.bar.w + 12, draw_sy, yellow);
                } else if (row == 1) { // Food slider
                    auto& s = sliders[slider_idx++];
                    renderText(renderer, font, s.label, sx, draw_sy, white);
                    SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
                    SDL_RenderFillRect(renderer, &s.bar);
                    float t = float(*s.value - s.min) / float(s.max - s.min);
                    int handle_x = s.bar.x + int(t * (s.bar.w - 16));
                    s.handle = {handle_x, s.bar.y - 4, 16, s.bar.h + 8};
                    SDL_SetRenderDrawColor(renderer, s.active ? 180 : 120, s.active ? 180 : 120, 255, 255);
                    SDL_RenderFillRect(renderer, &s.handle);
                    renderText(renderer, font, std::to_string(*s.value), s.bar.x + s.bar.w + 12, draw_sy, yellow);
                } else if (row == 2) { // Hunters toggle
                    auto& t = toggles[toggle_idx++];
                    renderText(renderer, font, t.label, sx, draw_sy, white);
                    SDL_SetRenderDrawColor(renderer, *t.value ? 0 : 80, *t.value ? 200 : 80, *t.value ? 80 : 80, 255);
                    SDL_RenderFillRect(renderer, &t.box);
                    SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                    SDL_RenderDrawRect(renderer, &t.box);
                    if (*t.value) renderText(renderer, font, "ON", t.box.x + 8, t.box.y + 2, green);
                    else renderText(renderer, font, "OFF", t.box.x + 8, t.box.y + 2, yellow);
                } else if (row == 3) { // Hunter Count slider
                    auto& s = sliders[slider_idx++];
                    renderText(renderer, font, s.label, sx, draw_sy, white);
                    SDL_SetRenderDrawColor(renderer, 80, 80, 120, 255);
                    SDL_RenderFillRect(renderer, &s.bar);
                    float t = float(*s.value - s.min) / float(s.max - s.min);
                    int handle_x = s.bar.x + int(t * (s.bar.w - 16));
                    s.handle = {handle_x, s.bar.y - 4, 16, s.bar.h + 8};
                    SDL_SetRenderDrawColor(renderer, s.active ? 180 : 120, s.active ? 180 : 120, 255, 255);
                    SDL_RenderFillRect(renderer, &s.handle);
                    renderText(renderer, font, std::to_string(*s.value), s.bar.x + s.bar.w + 12, draw_sy, yellow);
                } else if (row == 4) { // Human Player toggle
                    auto& t = toggles[toggle_idx++];
                    renderText(renderer, font, t.label, sx, draw_sy, white);
                    SDL_SetRenderDrawColor(renderer, *t.value ? 0 : 80, *t.value ? 200 : 80, *t.value ? 80 : 80, 255);
                    SDL_RenderFillRect(renderer, &t.box);
                    SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
                    SDL_RenderDrawRect(renderer, &t.box);
                    if (*t.value) renderText(renderer, font, "ON", t.box.x + 8, t.box.y + 2, green);
                    else renderText(renderer, font, "OFF", t.box.x + 8, t.box.y + 2, yellow);
                }
                draw_sy += th;
            }
            renderText(renderer, font, "ESC: Apply & Resume", SCREEN_WIDTH/2 - 80, draw_sy + 32, cyan);
            renderText(renderer, font, "R: Restart", SCREEN_WIDTH/2 - 80, draw_sy + 64, yellow);

            // --- Mouse event handling for sliders/toggles ---
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) quit = true;
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    int mx = e.button.x, my = e.button.y;
                    for (size_t i = 0; i < sliders.size(); ++i) {
                        auto& s = sliders[i];
                        if (mx >= s.handle.x && mx <= s.handle.x + s.handle.w && my >= s.handle.y && my <= s.handle.y + s.handle.h) {
                            s.active = true;
                            dragging_slider = (int)i;
                        }
                    }
                    for (size_t i = 0; i < toggles.size(); ++i) {
                        auto& t = toggles[i];
                        if (mx >= t.box.x && mx <= t.box.x + t.box.w && my >= t.box.y && my <= t.box.y + t.box.h) {
                            *t.value = !*t.value;
                        }
                    }
                }
                if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
                    for (auto& s : sliders) s.active = false;
                    dragging_slider = -1;
                }
                if (e.type == SDL_MOUSEMOTION && dragging_slider != -1) {
                    auto& s = sliders[dragging_slider];
                    int mx = e.motion.x;
                    float t = float(mx - s.bar.x) / float(s.bar.w - 16);
                    t = std::max(0.0f, std::min(1.0f, t));
                    *s.value = s.min + int(t * (s.max - s.min));
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                    // Apply and resume
                    g_bot_count = pending_bot_count;
                    g_food_count = pending_food_count;
                    g_hunters_enabled = pending_hunters_enabled;
                    g_hunter_count = pending_hunter_count;
                    g_player_enabled = pending_player_enabled;
                    restart_simulation();
                    paused = false;
                }
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r) {
                    restart_simulation();
                }
            }
        }
        SDL_RenderPresent(renderer);
    }
} 