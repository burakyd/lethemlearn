#include "Food.h"
#include "Game.h"

Food::Food(float x_, float y_, int width_, int height_)
    : x(x_), y(y_), width(width_), height(height_) {}

void Food::update(Game& game) {
    // Food does not update itself in this version
}

void Food::draw(SDL_Renderer* renderer) {
    SDL_Rect rect = {static_cast<int>(x), static_cast<int>(y), width, height};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
    SDL_RenderFillRect(renderer, &rect);
} 
