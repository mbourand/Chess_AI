#include "BitBoard.h"
#include "Computer.h"

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

void evaluationTest()
{
    BitBoard bitboard("rnbqkbnr/pppppppp/8/8/8/N7/PPPPPPPP/R1BQKBNR b KQkq - 0 1");
    Computer computer;

    std::cout << "Evaluation: " << computer.evaluate(bitboard) << std::endl;
}

void computer_play()
{
    srand(time(NULL));
    BitBoard bitboard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    Computer computer(7, "./komodo.bin");

    while (true)
    {
        uint16_t move = computer.getBestMove(bitboard);
        std::cout << "Best move: " << move << std::endl;
        bitboard.movePiece(move >> 6, move & 0b111111, move >> 12);
        std::cout << bitboard;
        std::string line;
        std::getline(std::cin, line);
        int from_x = line[0] - 'a';
        int from_y = 8 - (line[1] - '0');
        int to_x = line[2] - 'a';
        int to_y = 8 - (line[3] - '0');
        int promotion = 0;
        if (line.size() == 5)
        {
            switch (line[4])
            {
                case 'n':
                    promotion = 2;
                    break;
                case 'b':
                    promotion = 3;
                    break;
                case 'r':
                    promotion = 4;
                    break;
                case 'q':
                    promotion = 5;
                    break;
            }
        }
        bitboard.movePiece(from_y * 8 + from_x, to_y * 8 + to_x, promotion);
        std::cout << bitboard;
    }
}

void hashTest()
{
    std::map<std::string, std::string> tests = {
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "463b96181691fc9c" },
        { "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", "823c9b50fd114196" },
        { "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2", "756b94461c50fb0" },
        { "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2", "662fafb965db29d4" },
        { "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3", "22a48b5a8e47ff78" },
        { "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3", "652a607ca3f242c1" },
        { "rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4", "fdd303c946bdd9" },
        { "rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3", "3c8123ea7b067637" },
        { "rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4", "5c3f9b829b279560" },
    };

    Computer computer;
    int ok = 0;
    for (auto& test : tests)
    {
        BitBoard board(test.first);
        std::cout << "Testing FEN: " << test.first;
        std::stringstream ss;
        ss << std::hex << computer.hash(board);
        std::string hashed = ss.str();
        if (hashed == test.second)
        {
            std::cout << ": OK" << std::endl;
            ok++;
        }
        else
            std::cout << ": KO \n  Got: " << std::hex << hashed << "\n  Expected: " << test.second << std::dec << std::endl;
    }

    std::cout << "OK: " << ok << "/" << tests.size() << std::endl;
}

int main()
{
    computer_play();
    return 0;
}
