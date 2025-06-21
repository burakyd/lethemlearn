#pragma once
#include "Settings.h"
#include <SDL.h>
class Game;

class Food {
public:
    Food(float x, float y, int width = FOOD_WIDTH, int height = FOOD_HEIGHT);
    void update(Game& game);
    void draw(SDL_Renderer* renderer);
    float x, y;
    int width, height;
}; 