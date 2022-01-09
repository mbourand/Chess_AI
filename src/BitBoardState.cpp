#include "BitBoardState.h"

BitBoardState::BitBoardState(sf::RenderWindow& window, BitBoard& bitboard, Computer& computer) : window(window), bitboard(bitboard), heldSquare(255), computer(computer)
{
    piece_textures.fill(sf::Texture());
    piece_textures[PAWN - 1].loadFromFile("assets/png/wp.png");
    piece_textures[KNIGHT - 1].loadFromFile("assets/png/wn.png");
    piece_textures[BISHOP - 1].loadFromFile("assets/png/wb.png");
    piece_textures[ROOK - 1].loadFromFile("assets/png/wr.png");
    piece_textures[QUEEN - 1].loadFromFile("assets/png/wq.png");
    piece_textures[KING - 1].loadFromFile("assets/png/wk.png");
    piece_textures[PAWN + 6 - 1].loadFromFile("assets/png/bp.png");
    piece_textures[KNIGHT + 6 - 1].loadFromFile("assets/png/bn.png");
    piece_textures[BISHOP + 6 - 1].loadFromFile("assets/png/bb.png");
    piece_textures[ROOK + 6 - 1].loadFromFile("assets/png/br.png");
    piece_textures[QUEEN + 6 - 1].loadFromFile("assets/png/bq.png");
    piece_textures[KING + 6 - 1].loadFromFile("assets/png/bk.png");
}

void BitBoardState::render()
{
    sf::RectangleShape square;
    square.setSize(sf::Vector2f(window.getView().getSize().x / 8.0f, window.getView().getSize().y / 8.0f));

    bool white = true;
    for (int i = 0; i < 64; i++)
    {
        square.setFillColor(white ? sf::Color(0xF0D9B5FF) : sf::Color(0xB58863FF));
        square.setPosition(sf::Vector2f((i % 8) * square.getSize().x, i / 8 * square.getSize().y));
        window.draw(square);
        if (i % 8 != 7)
            white = !white;
    }

    sf::Sprite sprite;
    for (int i = 0; i < 64; i++)
    {
        if (bitboard.at(i) != 0 && i != heldSquare)
        {
            sprite.setTexture(piece_textures[bitboard.at(i) + (6 * ((bitboard.colorBoard(BLACK) & (1ULL << i)) != 0)) - 1]);
            sprite.setPosition(sf::Vector2f((i % 8) * square.getSize().x, i / 8 * square.getSize().y));
            sprite.setScale(square.getSize().x / sprite.getTexture()->getSize().x, square.getSize().y / sprite.getTexture()->getSize().y);
            window.draw(sprite);
        }
    }

    if (heldSquare != 255)
    {
        auto mousePos = sf::Mouse::getPosition(window);
        sprite.setTexture(piece_textures[bitboard.at(heldSquare) + (6 * ((bitboard.colorBoard(BLACK) & (1ULL << heldSquare)) != 0)) - 1]);
        sprite.setScale(square.getSize().x / sprite.getTexture()->getSize().x, square.getSize().y / sprite.getTexture()->getSize().y);
        sprite.setPosition(mousePos.x - (sprite.getTexture()->getSize().x * sprite.getScale().x / 2.0f), mousePos.y - (sprite.getTexture()->getSize().y * sprite.getScale().y / 2.0f));
        window.draw(sprite);
    }
}

void BitBoardState::update()
{
    static bool hasToRelease = false;
    auto mousePos = sf::Mouse::getPosition(window);

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && heldSquare == 255 && !hasToRelease)
    {
        uint16_t move = computer.getBestMove(bitboard);
        bitboard.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        hasToRelease = true;
        return;
    }
    else if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
        hasToRelease = false;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        if (heldSquare == 255)
        {
            int x = mousePos.x / (window.getView().getSize().x / 8.0f);
            int y = mousePos.y / (window.getView().getSize().y / 8.0f);
            int index = y * 8 + x;
            if (index >= 0 && index < 64 && bitboard.at(index) != 0)
                heldSquare = index;
        }
    }
    else if (heldSquare != 255)
    {
        int x = mousePos.x / (window.getView().getSize().x / 8.0f);
        int y = mousePos.y / (window.getView().getSize().y / 8.0f);
        int index = y * 8 + x;
        if (index >= 0 && index < 64)
        {
            auto moves = bitboard.get_moves(bitboard.player_to_move());
            uint8_t promotion_piece = ((1ULL << index) & (ROW_8 | ROW_1)) && bitboard.at(heldSquare) == PAWN ? QUEEN : 0;
            if (std::find(moves.begin(), moves.end(), (heldSquare << 6) | (index) | (promotion_piece << 12)) != moves.end())
                bitboard.movePiece(heldSquare, index, promotion_piece);
            heldSquare = 255;
        }
        else
            heldSquare = 255;
    }
}
