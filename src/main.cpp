#include "BitBoard.h"

int64_t perft_count(BitBoard& bitBoard, int depth)
{
    auto moves = bitBoard.get_moves(bitBoard.player_to_move());
    int64_t count = 0;
    for (auto move : moves)
    {
        uint64_t encodedMove = bitBoard.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        if (depth == 1)
            count++;
        else
        {
            int ret = perft_count(bitBoard, depth - 1);
            if (ret < 0)
                return -1;
            count += ret;
        }
        bitBoard.undoMove(encodedMove);
    }
    return count;
}

int64_t perft(BitBoard& bitBoard, int depth)
{
    int total = 0;
    auto moves = bitBoard.get_moves(bitBoard.player_to_move());
    for (auto move : moves)
    {
        uint64_t encodedMove = bitBoard.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        uint8_t to = move & 0b111111;
        uint8_t from = (move >> 6) & 0b111111;
        uint8_t promotion_piece = move >> 12;
        char fromFile = (from % 8) + 'a';
        char fromRank = (8 - from / 8) + '0';
        char toFile = (to % 8) + 'a';
        char toRank = (8 - to / 8) + '0';
        std::cout << fromFile << fromRank << toFile << toRank << (promotion_piece ? std::string("--nbrq")[promotion_piece] : '\0') << ": ";
        if (depth > 1)
        {
            int64_t count = perft_count(bitBoard, depth - 1);
            if (count < 0)
                return 0;
            total += count;
            std::cout << count << std::endl;
        }
        else
        {
            total++;
            std::cout << 1 << std::endl;
        }
        bitBoard.undoMove(encodedMove);
    }
    return total;
}

void print_all_moves_possible(BitBoard& bitBoard)
{
    auto moves = bitBoard.get_moves(bitBoard.player_to_move());
    for (auto move : moves)
    {
        std::cout << move << std::endl;
        uint64_t encodedMove = bitBoard.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        std::cout << bitBoard;
        std::cout << "==============================================================" << "==============================================================" << std::endl;
        bitBoard.undoMove(encodedMove);
    }
}

int main() {
    BitBoard bitBoard("8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1");

    std::cout << bitBoard;

    int ret = perft(bitBoard, 4);
    std::cout << "\nNodes searched: " << ret << std::endl;

    std::cout << bitBoard;

    return 0;
}
