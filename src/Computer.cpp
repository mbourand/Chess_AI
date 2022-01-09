#include "Computer.h"


/* -------------------------------------------------------------------------- */
/*                              Precomputed utils                             */
/* -------------------------------------------------------------------------- */

void precomputed_passed_pawns()
{
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t mask = 0;
        int8_t file = square % 8;
        int8_t rank = square / 8;
        while (rank < 8)
        {
            mask |= (1ULL << (rank * 8 + file));
            rank++;
        }
        mask ^= (1ULL << square);
        // printBitboard(mask)
        std::cout << mask << "ULL, ";
        if (square % 8 == 7)
            std::cout << std::endl;
    }
    std::cout << std::endl;
}

/* -------------------------------------------------------------------------- */
/*                               Computer class                               */
/* -------------------------------------------------------------------------- */

Computer::Computer()
{
    m_depth = 6;
    m_timeToPlay = 1 * 1000;
    m_killerMoves.resize(m_depth, {0, 0});
}

Computer::Computer(uint8_t depth, const std::string& openingBook)
{
    m_depth = depth;
    m_openingBook = OpeningBook(openingBook);
    m_timeToPlay = 1 * 1000;
    m_killerMoves.resize(m_depth, {0, 0});
}

Computer::Computer(const Computer& other)
{
    *this = other;
}

Computer& Computer::operator=(const Computer& other)
{
    m_depth = other.m_depth;
    m_openingBook = other.m_openingBook;
    m_transpositionTable = other.m_transpositionTable;
    m_timeToPlay = other.m_timeToPlay;
    m_killerMoves = other.m_killerMoves;
    return *this;
}

uint64_t Computer::hash(const BitBoard& board) const
{
    // Piece
    uint64_t pieces = 0;
    for (int8_t i = 0; i < 64; i++)
    {
        uint8_t piece = board.m_pieces[i];
        if (piece == 0)
            continue;
        uint8_t piece_zobrist_index = 0;
        switch (piece)
        {
        case PAWN:
            piece_zobrist_index = (board.m_bitboards[BLACK][PAWN] & (1ULL << i)) ? 0 : 1;
            break;
        case KNIGHT:
            piece_zobrist_index = (board.m_bitboards[BLACK][KNIGHT] & (1ULL << i)) ? 2 : 3;
            break;
        case BISHOP:
            piece_zobrist_index = (board.m_bitboards[BLACK][BISHOP] & (1ULL << i)) ? 4 : 5;
            break;
        case ROOK:
            piece_zobrist_index = (board.m_bitboards[BLACK][ROOK] & (1ULL << i)) ? 6 : 7;
            break;
        case QUEEN:
            piece_zobrist_index = (board.m_bitboards[BLACK][QUEEN] & (1ULL << i)) ? 8 : 9;
            break;
        case KING:
            piece_zobrist_index = (board.m_bitboards[BLACK][KING] & (1ULL << i)) ? 10 : 11;
            break;
        }

        pieces ^= ZOBRIST_KEYS[64 * piece_zobrist_index + 8 * (7 - i / 8) + (i % 8)];
    }

    // Castling
    uint64_t castling = ((board.m_castling_rights & WK) ? ZOBRIST_KEYS[768] : 0) ^ ((board.m_castling_rights & WQ) ? ZOBRIST_KEYS[769] : 0)
                        ^ ((board.m_castling_rights & BK) ? ZOBRIST_KEYS[770] : 0) ^ ((board.m_castling_rights & BQ) ? ZOBRIST_KEYS[771] : 0);

    // En passant
    uint64_t en_passant = 0;
    if (board.m_en_passant_square != 255)
    {
        uint8_t en_passant_pawn_square = board.m_en_passant_square + (board.player_to_move() == WHITE ? 8 : -8);
        bool can_en_passant = ((ROWS[en_passant_pawn_square / 8] & ((1ULL << (en_passant_pawn_square + 1)) | (1ULL << (en_passant_pawn_square - 1)))
                                    & board.m_bitboards[board.player_to_move()][PAWN]) != 0);
        en_passant = can_en_passant ? ZOBRIST_KEYS[772 + (board.m_en_passant_square % 8)] : 0;
    }

    // Side to move
    uint64_t side_to_move = (board.player_to_move() == WHITE ? ZOBRIST_KEYS[780] : 0);

    // Hash
    return (pieces ^ castling ^ en_passant ^ side_to_move);
}

int Computer::evaluate(const BitBoard& board) const
{
    int score = 0;

    bool endgame = (board.m_bitboards[WHITE][QUEEN] | board.m_bitboards[BLACK][QUEEN]) == 0 
            && countBits(board.m_bitboards[WHITE][ALL] & ~(board.m_bitboards[WHITE][PAWN])) <= 2
            && countBits(board.m_bitboards[BLACK][ALL] & ~(board.m_bitboards[BLACK][PAWN])) <= 2;

    // Piece squares & Material advantage
    for (uint8_t i = 0; i < 64; i++)
    {
        if (board.m_pieces[i] == KING && endgame)
            continue;
        if (board.m_bitboards[WHITE][0] & (1ULL << i))
            score += PIECE_TABLES[board.m_pieces[i] - 1][i] + PIECE_VALUES[board.m_pieces[i]];
        else if (board.m_bitboards[BLACK][0] & (1ULL << i))
            score -= PIECE_TABLES[board.m_pieces[i] - 1][(7 - i / 8) * 8 + (i % 8)] + PIECE_VALUES[board.m_pieces[i]];
    }

    // Doubled pawns
    for (uint8_t i = 0; i < 8; i++)
        score += (std::max(0, static_cast<int8_t>((countBits(board.m_bitboards[WHITE][PAWN]) & FILES[i])) - 1) - std::max(0, static_cast<int8_t>((countBits(board.m_bitboards[BLACK][PAWN])) & FILES[i]) - 1)) * DOUBLE_PAWN_VALUE;

    // Isolated pawns
    for (uint8_t i = 1; i < 7; i++)
    {
        uint64_t adjacent_mask = FILES[i + 1] | FILES[i - 1];
        if ((board.m_bitboards[WHITE][PAWN] & FILES[i]) && !(board.m_bitboards[WHITE][PAWN] & adjacent_mask))
            score += ISOLATED_PAWN_VALUE;
        if ((board.m_bitboards[BLACK][PAWN] & FILES[i]) && !(board.m_bitboards[BLACK][PAWN] & adjacent_mask))
            score -= ISOLATED_PAWN_VALUE;
    }

    // Passed pawns
    /*uint64_t w_pawns = board.m_bitboards[WHITE][PAWN];
    while (w_pawns)
    {
        uint8_t square = __builtin_ctzll(w_pawns);
        uint8_t file = (square % 8);
        w_pawns &= ~(1ULL << square);
        uint64_t adjacent_mask = FILES[file] | (file == 0 ? 0 : FILES[file - 1]) | (file == 7 ? 0 : FILES[file + 1]);
        if (!(board.m_bitboards[BLACK][PAWN] & adjacent_mask))
            score += PASSED_PAWN_VALUE;
        if (endgame && (ROOK_BEHIND_PAWN_MASKS[square] & board.m_bitboards[WHITE][ROOK]))
            score += ROOK_BEHIND_PASSED_PAWN_VALUE;
    }*/

    return score;
}

int Computer::quiescence(BitBoard& board, int alpha, int beta, int8_t color)
{
    int stand_pat = color * evaluate(board);
    if (stand_pat >= beta)
        return beta;
    if (stand_pat >= alpha)
        alpha = stand_pat;

    uint8_t player_to_move = board.player_to_move();
    std::vector<uint16_t> moves = board.get_capture_moves(player_to_move);
    
    if (moves.size() == 0)
        return color * evaluate(board);

    std::sort(moves.begin(), moves.end(), [&board, player_to_move, this](uint16_t a, uint16_t b) {
        int16_t score = 0;
        int8_t a_to = a & 0b111111;
        int8_t a_from = (a >> 6) & 0b111111;
        int8_t b_from = (b >> 6) & 0b111111;
        int8_t b_to = b & 0b111111;

        score += (PIECE_VALUES[board.at(a_to)]) - (PIECE_VALUES[board.m_pieces[a_from]] / 10);
        score -= (PIECE_VALUES[board.at(b_to)]) - (PIECE_VALUES[board.m_pieces[b_from]] / 10);

        score += ((board.m_last_move_to == a_to) - (board.m_last_move_to == b_to)) * 1001;

        return score > 0;
    });

    int bestScore = -std::numeric_limits<int>::max();
    for (uint16_t move : moves)
    {
        uint64_t encodedMove = board.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        int moveValue = -quiescence(board, -beta, -alpha, -color);
        alpha = std::max(alpha, moveValue);
        if (moveValue > bestScore)
            bestScore = moveValue;
        if (alpha >= beta)
        {
            board.undoMove(encodedMove);
            break;
        }
        board.undoMove(encodedMove);
    }

    return alpha;
}

std::pair<int, uint16_t> Computer::negamax(BitBoard& board, uint8_t depth, int alpha, int beta, int8_t color)
{
    static uint64_t tbUsed = 0;
    static uint64_t positions = 0;

    if (depth == 0)
        return std::make_pair(quiescence(board, alpha, beta, color), 0);

    int startAlpha = alpha;

    uint64_t key = hash(board);
    auto ttMoveIt = m_transpositionTable.find(key);
    if (ttMoveIt != m_transpositionTable.end() && ttMoveIt->second.depth >= depth)
    {
        tbUsed += 1;
        if (ttMoveIt->second.type == EXACT)
            return std::make_pair(ttMoveIt->second.score, ttMoveIt->second.move);
        else if (ttMoveIt->second.type == LOWERBOUND)
            alpha = std::max(alpha, ttMoveIt->second.score);
        else if (ttMoveIt->second.type == UPPERBOUND)
            beta = std::min(beta, ttMoveIt->second.score);

        if (alpha >= beta)
            return std::make_pair(ttMoveIt->second.score, ttMoveIt->second.move);
    }

    uint8_t player_to_move = board.player_to_move();
    std::vector<uint16_t> moves = board.get_moves(player_to_move);
    
    if (moves.size() == 0)
    {
        if ((board.isSquareAttacked(__builtin_ctzll(board.m_bitboards[player_to_move][KING]), !player_to_move)))
            return std::make_pair(color * (player_to_move == WHITE ? -32000 : 32000), 0);
        else
            return std::make_pair(0, 0);
    }

    std::sort(moves.begin(), moves.end(), [&board, player_to_move, &ttMoveIt, this, key, depth](uint16_t a, uint16_t b) {
        int16_t score = 0;
        int8_t a_to = a & 0b111111;
        int8_t a_from = (a >> 6) & 0b111111;
        int8_t b_from = (b >> 6) & 0b111111;
        int8_t b_to = b & 0b111111;

        if (board.isCapture(a))
            score += (PIECE_VALUES[board.at(a_to)]) - (PIECE_VALUES[board.m_pieces[a_from]] / 10);
        if (board.isCapture(b))
            score -= (PIECE_VALUES[board.at(b_to)]) - (PIECE_VALUES[board.m_pieces[b_from]] / 10);

        uint8_t a_square = (player_to_move == WHITE ? a_to : (7 - a_to / 8) * 8 + (a_to % 8));
        uint8_t b_square = (player_to_move == WHITE ? b_to : (7 - b_to / 8) * 8 + (b_to % 8));
        score += PIECE_TABLES[board.m_pieces[a_from] - 1][a_square] - PIECE_TABLES[board.m_pieces[b_from] - 1][b_square];

        score += ((board.m_last_move_to == a_to) - (board.m_last_move_to == b_to)) * 1001;

        if (ttMoveIt != this->m_transpositionTable.end() && ttMoveIt->second.move == a)
            score += 50;
        if (ttMoveIt != this->m_transpositionTable.end() && ttMoveIt->second.move == b)
            score -= 50;

        score += ((m_killerMoves[depth - 1][0] == a || m_killerMoves[depth - 1][0] == a) - (m_killerMoves[depth - 1][0] == b || m_killerMoves[depth - 1][0] == b)) * 40;
        return score > 0;
    });

    int bestScore = -std::numeric_limits<int>::max();
    uint16_t bestMove = 0;
    for (uint16_t move : moves)
    {
        positions++;
        uint64_t encodedMove = board.movePiece((move >> 6) & 0b111111, move & 0b111111, move >> 12);
        auto moveValue = negamax(board, depth - 1, -beta, -alpha, -color);
        moveValue.first *= -1;
        if (moveValue.first > alpha)
            alpha = moveValue.first;
        if (moveValue.first > bestScore)
        {
            bestScore = moveValue.first;
            bestMove = move;
        }
        if (alpha >= beta)
        {
            if (!board.isCapture(move))
            {
                m_killerMoves[depth - 1][1] = m_killerMoves[depth - 1][0];
                m_killerMoves[depth - 1][0] = move;
            }
            board.undoMove(encodedMove);
            break;
        }
        board.undoMove(encodedMove);
    }

    TranspositionTableData ttData = { bestMove, depth, alpha, (bestScore <= startAlpha ? UPPERBOUND : (bestScore >= beta ? LOWERBOUND : EXACT)), 0 };
    if (ttMoveIt == m_transpositionTable.end() || ttMoveIt->second.depth <= depth)
        m_transpositionTable[key] = ttData;

    if (depth == m_depth)
    {
        if (positions != 0)
            std::cout << "Transposition table usage: " << (tbUsed / ((float)positions)) * 100 << "%" << std::endl;
        tbUsed = 0;
        positions = 0;
    }

    return std::make_pair(alpha, bestMove);
}

uint16_t Computer::getBestMove(BitBoard& board)
{
    uint16_t bookMove = m_openingBook.getMove(hash(board));
    if (bookMove != 0)
        return bookMove;

    m_killerMoves.resize(m_depth, {0, 0});
    uint16_t ret = negamax(board, m_depth, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), board.player_to_move() == WHITE ? 1 : -1).second;
    for (auto& entry : m_transpositionTable)
        entry.second.age += 1;
        
    for (auto entry = m_transpositionTable.begin(); entry != m_transpositionTable.end();)
        if (entry->second.age > 5)
            entry = m_transpositionTable.erase(entry);
        else
            entry++;

    m_killerMoves.clear();
    return ret;
}
