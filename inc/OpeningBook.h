#ifndef OPENING_BOOK_H
#define OPENING_BOOK_H

#include "globals.h"

// Reads polyglot format opening books.

struct MoveData
{
    uint16_t move;
    uint16_t weight;
};

class OpeningBook
{
    public:
        std::unordered_map<uint64_t, std::vector<MoveData>> book;

    public:
        OpeningBook();
        OpeningBook(const std::string& filename);
        OpeningBook(const OpeningBook& other);
        OpeningBook& operator=(const OpeningBook& other);

        uint16_t getMove(uint64_t key) const;
};

#endif
