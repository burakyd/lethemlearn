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
    void newPlayer(int number = 1, int width = DOT_WIDTH, int height = DOT_HEIGHT, std::array<int, 3> color = DOT_COLOR, float speed = SPEED, bool random_color = true, bool random_size = false);
    void newHunter(int number = 1, int width = HUNTER_WIDTH, int height = HUNTER_HEIGHT, std::array<int, 3> color = HUNTER_COLOR, float speed = SPEED, bool random_color = true, bool random_size = false);
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
}; 