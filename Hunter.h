#pragma once
#include "Player.h"
#include <SDL.h>
#include "Food.h"
#include <array>

class Hunter : public Player {
public:
    Hunter(int width = HUNTER_WIDTH, int height = HUNTER_HEIGHT, SDL_Color color = HUNTER_COLOR, float x = 0, float y = 0, float speed = SPEED, bool alive = true);
    void update(Game& game) override;
    void randomMove(Game& game);
    void draw(SDL_Renderer* renderer) override;
    int movetime;
    std::array<int, 4> keys;
    bool eatPlayer(Game& game, Player& other) override;
    bool eatFood(Game& game) override;
}; 