#pragma once
#include <vector>
#include "Settings.h"
#include <SDL.h>
#include <array>
class Player;
class Food;
class Hunter;

class Game {
public:
    Game(SDL_Renderer* renderer);
    void update();
    void render();
    void handleEvents();
    void reset();
    void newPlayer(const std::vector<std::vector<float>>& genes, const std::vector<std::vector<float>>& biases, int width = DOT_WIDTH, int height = DOT_HEIGHT, SDL_Color color = DOT_COLOR, float speed = SPEED);
    void newHunter(int number = 1, int width = HUNTER_WIDTH, int height = HUNTER_HEIGHT, SDL_Color color = HUNTER_COLOR, float speed = SPEED, bool random_color = true, bool random_size = false);
    void randomFood(int num = 1);
    void maintain_population();

    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;
    std::vector<Player*> players;
    std::vector<Hunter*> hunters;
    std::vector<Food*> foods;
    SDL_Renderer* renderer;
    bool inLocation(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    // Add more as needed

    // --- Spatial Partitioning ---
    static constexpr int CELL_SIZE = GRID_CELL_SIZE;
    static constexpr int GRID_WIDTH = (SCREEN_WIDTH + CELL_SIZE - 1) / CELL_SIZE;
    static constexpr int GRID_HEIGHT = (SCREEN_HEIGHT + CELL_SIZE - 1) / CELL_SIZE;
    std::vector<Player*> player_grid[GRID_WIDTH][GRID_HEIGHT];
    std::vector<Food*> food_grid[GRID_WIDTH][GRID_HEIGHT];
    void update_grids();
    std::vector<Player*> get_nearby_players(float x, float y);
    std::vector<Food*> get_nearby_food(float x, float y);
}; 