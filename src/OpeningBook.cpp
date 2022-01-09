#include "OpeningBook.h"
#include "BitBoard.h"

OpeningBook::OpeningBook()
{
}

OpeningBook::OpeningBook(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return;
    uint64_t key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn; // unused
    while (!file.eof())
    {
        file.read(reinterpret_cast<char*>(&key), sizeof(key));
        key = invert_bytes(key);
        file.read(reinterpret_cast<char*>(&move), sizeof(move));
        move = (move << 8) | (move >> 8);
        file.read(reinterpret_cast<char*>(&weight), sizeof(weight));
        weight = (weight << 8) | (weight >> 8);
        file.read(reinterpret_cast<char*>(&learn), sizeof(learn)); // unused, just to read garbage
        if (weight != 0 && move != 0)
        {
            uint16_t formatted_move = 0;
            formatted_move |= (7 - ((move >> 3) & 0b111)) * 8 + ((move & 0b111) % 8);
            formatted_move |= ((7 - ((move >> 9) & 0b111)) * 8 + (((move >> 6) & 0b111) % 8)) << 6;
            uint8_t promotion = (move >> 12) & 0b111;
            switch (promotion)
            {
                case 1:
                    promotion = KNIGHT;
                    break;
                case 2:
                    promotion = BISHOP;
                    break;
                case 3:
                    promotion = ROOK;
                    break;
                case 4:
                    promotion = QUEEN;
                    break;
            }
            formatted_move |= (promotion << 12);
            
            book[key].push_back({formatted_move, weight});
        }
    }
}

OpeningBook::OpeningBook(const OpeningBook& other)
{
    *this = other;
}

OpeningBook& OpeningBook::operator=(const OpeningBook& other)
{
    book = other.book;
    return *this;
}

uint16_t OpeningBook::getMove(uint64_t key) const
{
    auto it = book.find(key);
    if (it == book.end())
        return 0;
    auto& moves = it->second;
    return moves[rand() % moves.size()].move;
}
