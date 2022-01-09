#ifndef BITBOARD_STATE_H
#define BITBOARD_STATE_H

#include "globals.h"
#include "BitBoard.h"
#include "Computer.h"

class BitBoardState
{
private:
    sf::RenderWindow& window;
    BitBoard& bitboard;
    std::array<sf::Texture, 12> piece_textures;
    uint8_t heldSquare;
    Computer& computer;

public:
    BitBoardState(sf::RenderWindow& window, BitBoard& bitboard, Computer& computer);

    void render();
    void update();
};

#endif
