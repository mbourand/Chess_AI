#include "BitBoard.h"

/* -------------------------------------------------------------------------- */
/*                               BitBoard class                               */
/* -------------------------------------------------------------------------- */

BitBoard::BitBoard() {
    rook_moves.assign(64, {});
    for (auto& elem : rook_moves)
        elem.assign(4096, 0);
    bishop_moves.assign(64, {});
    for (auto& elem : bishop_moves)
        elem.assign(512, 0);
    for (auto& elem : m_bitboards)
        elem.fill(0);
    m_pieces.fill(0);
    m_castling_rights = 0;
    generate_rook_moves();
    generate_bishop_moves();
}

BitBoard::BitBoard(const BitBoard& other)
{
    *this = other;
}

BitBoard::BitBoard(const std::string& fen)
{
    rook_moves.assign(64, {});
    for (auto& elem : rook_moves)
        elem.assign(4096, 0);
    bishop_moves.assign(64, {});
    for (auto& elem : bishop_moves)
        elem.assign(512, 0);
    for (auto& elem : m_bitboards)
        elem.fill(0);
    m_pieces.fill(0);
    generate_rook_moves();
    generate_bishop_moves();

    int8_t x = 0;
    int8_t y = 0;

    for (const char c : fen)
    {
        if (c == ' ')
            break;
        else if (c == '/')
        {
            x = 0;
            y++;
        }
        else if (c >= '1' && c <= '8')
            x += c - '0';
        else
        {
            uint8_t color = std::isupper(c) ? WHITE : BLACK;
            switch (c)
            {
            case 'p':
            case 'P':
                setPiece(color, PAWN, x + y * 8);
                break;
            case 'r':
            case 'R':
                setPiece(color, ROOK, x + y * 8);
                break;
            case 'n':
            case 'N':
                setPiece(color, KNIGHT, x + y * 8);
                break;
            case 'b':
            case 'B':
                setPiece(color, BISHOP, x + y * 8);
                break;
            case 'q':
            case 'Q':
                setPiece(color, QUEEN, x + y * 8);
                break;
            case 'k':
            case 'K':
                setPiece(color, KING, x + y * 8);
                break;
            }
            x++;
        }
    }

    int index = fen.find(' ') + 1;

    m_player_to_move = fen[index] == 'w' ? WHITE : BLACK;
    index += 2;
    m_castling_rights = 0;
    while (fen[index] != ' ')
    {
        switch (fen[index])
        {
            case 'K':
                m_castling_rights |= WK;
                break;
            case 'Q':
                m_castling_rights |= WQ;
                break;
            case 'k':
                m_castling_rights |= BK;
                break;
            case 'q':
                m_castling_rights |= BQ;
                break;
        }
        index++;
    }
    index++;
    m_en_passant_square = fen[index] == '-' ? -1 : fen[index] - 'a' + (m_player_to_move == WHITE ? 2 : 5) * 8;
}

uint8_t BitBoard::player_to_move() const
{
    return m_player_to_move;
}

uint64_t BitBoard::movePiece(int8_t from, int8_t to, uint8_t promotion_piece)
{
    uint8_t piece = at(from);
    uint8_t color = m_bitboards[WHITE][ALL] & (1ULL << from) ? WHITE : BLACK;
    uint8_t captured_pos = (piece == PAWN && to == m_en_passant_square) ? (m_en_passant_square + (color == WHITE ? 8 : -8)) : to;
    uint8_t captured = at(captured_pos);
    uint8_t captured_color = m_bitboards[WHITE][ALL] & (1ULL << captured_pos) ? WHITE : BLACK;
    uint64_t encodedMove = (to & 0b111111) | (from << 6) | (captured << 12) | (captured_color << 15)
                        | (captured_pos << 16) | (static_cast<uint64_t>(m_en_passant_square) << 22) | ((static_cast<uint64_t>(m_castling_rights)) << 30)
                        | (static_cast<uint64_t>(piece) << 34);

    if (piece == KING)
        m_castling_rights &= ~((WK | WQ) << (color * 2));
    if (piece == ROOK)
    {
        if (from == 0 || from == 56)
            m_castling_rights &= ~(WQ << (color * 2));
        else if (from == 7 || from == 63)
            m_castling_rights &= ~(WK << (color * 2));
    }

    if (piece == KING && std::abs(from - to) == 2)
    {
        int8_t y = (from / 8);
        if (to - from > 0)
        {
            // Move rook to F file
            m_bitboards[color][ALL] ^= (1ULL << (y * 8 + 7));
            m_bitboards[color][ROOK] ^= (1ULL << (y * 8 + 7));
            m_pieces[y * 8 + 7] = 0;
            setPiece(color, ROOK, (y * 8 + 5));
        }
        else
        {
            // Move rook to D file
            m_bitboards[color][ALL] ^= (1ULL << (y * 8));
            m_bitboards[color][ROOK] ^= (1ULL << (y * 8));
            m_pieces[(y * 8)] = 0;
            setPiece(color, ROOK, (y * 8 + 3));
        }
        m_castling_rights &= ~((WK | WQ) << (color * 2));
    }

    if (captured)
    {
        if (captured == ROOK)
        {
            if ((captured_pos == 0 && captured_color == BLACK) || (captured_pos == 56 && captured_color == WHITE))
                m_castling_rights &= ~(WQ << (!color * 2));
            else if ((captured_pos == 7 && captured_color == BLACK) || (captured_pos == 63 && captured_color == WHITE))
                m_castling_rights &= ~(WK << (!color * 2));
        }
        m_bitboards[captured_color][ALL]      ^= (1ULL << captured_pos);
        m_bitboards[captured_color][captured] ^= (1ULL << captured_pos);
        m_pieces[captured_pos] = 0;
    }
    m_bitboards[color][ALL]    ^= (1ULL << from);
    m_bitboards[color][piece]  ^= (1ULL << from);
    m_pieces[from] = 0;
    setPiece(color, (promotion_piece == 0 ? piece : promotion_piece), to);

    if (piece == PAWN && std::abs(to - from) == 16)
        m_en_passant_square = from + (color == WHITE ? -8 : 8);
    else
        m_en_passant_square = -1;

    m_player_to_move = !color;
    return encodedMove;
}

void BitBoard::undoMove(uint64_t move)
{
    int8_t to = move & 0b111111;
    int8_t from = (move >> 6) & 0b111111;
    uint8_t captured = (move >> 12) & 0b111;
    uint8_t captured_color = (move >> 15) & 0b1;
    uint8_t captured_pos = (move >> 16) & 0b111111;
    uint8_t piece = at(to);
    uint8_t color = colorBoard(WHITE) & (1ULL << to) ? WHITE : BLACK;
    uint8_t old_piece = (move >> 34) & 0b111;

    m_en_passant_square = (move >> 22) & 0b11111111;
    m_castling_rights = (move >> 30) & 0b1111;
    if (piece == KING && std::abs(from - to) == 2)
    {
        int8_t y = (from / 8);
        if (to - from > 0)
        {
            // Move rook to H file
            m_bitboards[color][ALL] ^= (1ULL << (y * 8 + 5));
            m_bitboards[color][ROOK] ^= (1ULL << (y * 8 + 5));
            m_pieces[y * 8 + 5] = 0;
            setPiece(color, ROOK, (y * 8 + 7));
        }
        else
        {
            // Move rook to A file
            m_bitboards[color][ALL] ^= (1ULL << (y * 8 + 3));
            m_bitboards[color][ROOK] ^= (1ULL << (y * 8 + 3));
            m_pieces[(y * 8 + 3)] = 0;
            setPiece(color, ROOK, (y * 8));
        }
    }

    m_pieces[to] = 0;
    if (captured)
    {
        m_bitboards[captured_color][ALL]      |= (1ULL << captured_pos);
        m_bitboards[captured_color][captured] |= (1ULL << captured_pos);
        m_pieces[captured_pos] = captured;
    }
    m_bitboards[color][ALL] ^= (1ULL << to);
    m_bitboards[color][piece] ^= (1ULL << to);
    setPiece(color, old_piece, from);
    m_player_to_move = !color;
}

bool BitBoard::isCorrupted() const
{
    if (m_bitboards[WHITE][ALL] & m_bitboards[BLACK][ALL])
    {
        std::cerr << "Bitboard corrupted: white and black pieces overlap" << std::endl;
        return true;
    }
    if (m_en_passant_square != 255 && !((ROW_3 | ROW_6) & (1ULL << m_en_passant_square)))
    {
        std::cerr << "Bitboard corrupted: en passant square is not on a valid row" << std::endl;
        return true;
    }
    if (countBits(m_bitboards[WHITE][ALL]) > 16 || countBits(m_bitboards[BLACK][ALL]) > 16)
    {
        std::cerr << "Bitboard corrupted: too many pieces" << std::endl;
        return true;
    }
    if (countBits(m_bitboards[WHITE][KING]) != 1 || countBits(m_bitboards[BLACK][KING]) != 1)
    {
        std::cerr << "Bitboard corrupted: too many kings" << std::endl;
        return true;
    }
    for (int color = 0; color < 2; color++)
    {
        for (uint8_t i = 1; i < 7; i++)
        {
            if (m_bitboards[color][i] & m_bitboards[!color][i])
            {
                std::cerr << "Bitboard corrupted: pieces of the same type overlap" << std::endl;
                return true;
            }
            if ((m_bitboards[color][i] & m_bitboards[color][ALL]) != m_bitboards[color][i])
            {
                std::cerr << "Bitboard corrupted: ALL board doesnt correspond to piece board" << std::endl;
                return true;
            }
        }
    }
    for (int i = 0; i < 64; i++)
    {
        if (m_pieces[i] != 0 && ((m_bitboards[WHITE][m_pieces[i]] | m_bitboards[BLACK][m_pieces[i]]) & (1ULL << i)) == 0)
        {
            std::cerr << "Bitboard corrupted: piece board doesnt correspond to bitboards" << std::endl;
            return true;
        }
    }
    return false;
}

BitBoard& BitBoard::operator=(const BitBoard& other)
{
    this->m_bitboards = other.m_bitboards;
    m_check = other.m_check;
    m_castling_rights = other.m_castling_rights;
    m_en_passant_square = other.m_en_passant_square;
    return *this;
}

uint64_t BitBoard::pieceBoard(uint8_t color, uint8_t piece) const
{
    return m_bitboards[color][piece];
}

uint64_t BitBoard::colorBoard(uint8_t color) const
{
    return m_bitboards[color][ALL];
}

uint64_t BitBoard::allPieces() const
{
    return m_bitboards[WHITE][ALL] | m_bitboards[BLACK][ALL];
}

uint8_t BitBoard::at(uint8_t bit) const
{
    return m_pieces[bit];
}

bool BitBoard::occupied(uint8_t square) const
{
    return ((m_bitboards[WHITE][ALL] | m_bitboards[BLACK][ALL]) & (1ULL << square)) != 0;
}

void BitBoard::setPiece(uint8_t color, uint8_t piece, uint8_t bit)
{
    m_bitboards[color][piece] |= 1ULL << bit;
    m_bitboards[color][ALL] |= 1ULL << bit;
    m_pieces[bit] = piece;
}

uint64_t BitBoard::get_attack_mask(uint8_t color) const
{
    uint64_t mask = 0;
    uint64_t occupancy = allPieces() & ~(m_bitboards[!color][KING]);
    uint8_t y_offset = color == WHITE ? -1 : 1;
    for (uint8_t piece_type = 1; piece_type < m_bitboards[color].size(); piece_type++)
    {
        uint64_t piece_board = m_bitboards[color][piece_type];
        while (piece_board)
        {
            uint8_t square = __builtin_ctzll(piece_board);
            piece_board &= piece_board - 1;
            switch (piece_type)
            {
                case PAWN:
                    if (!((1ULL << square) & FILE_A))
                        mask |= (1ULL << (square + y_offset * 8 - 1));
                    if (!((1ULL << square) & FILE_H))
                        mask |= (1ULL << (square + y_offset * 8 + 1));
                    break;
                case KNIGHT:
                    mask |= KNIGHT_MOVES[square];
                    break;
                case BISHOP:
                    mask |= get_bishop_moves(square, occupancy);
                    break;
                case ROOK:
                    mask |= get_rook_moves(square, occupancy);
                    break;
                case QUEEN:
                    mask |= get_bishop_moves(square, occupancy) | get_rook_moves(square, occupancy);
                    break;
                case KING:
                    mask |= KING_MOVES[square];
                    break;
            }
        }
    }
    return mask;
}

uint64_t BitBoard::create_ray(uint8_t from, uint8_t to) const
{
    uint8_t x_from = from % 8;
    uint8_t y_from = from / 8;
    uint8_t x_to = to % 8;
    uint8_t y_to = to / 8;
    uint64_t ray = 0;
    if (x_from == x_to)
    {
        for (uint8_t y = std::min(y_from, y_to); y <= std::max(y_from, y_to); y++)
            ray |= (1ULL << (y * 8 + x_from));
    }
    else if (y_from == y_to)
    {
        for (uint8_t x = std::min(x_from, x_to); x <= std::max(x_from, x_to); x++)
            ray |= (1ULL << (y_from * 8 + x));
    }
    else if (x_from == x_to)
    {
        for (uint8_t y = std::min(y_from, y_to); y <= std::max(y_from, y_to); y++)
            ray |= (1ULL << (y * 8 + x_from));
    }
    else
    {
        int8_t x_diff = x_to - x_from;
        int8_t y_diff = y_to - y_from;
        int8_t x_step = x_diff > 0 ? 1 : -1;
        int8_t y_step = y_diff > 0 ? 1 : -1;
        int8_t x = x_from;
        int8_t y = y_from;
        while (x != x_to || y != y_to)
        {
            ray |= (1ULL << (y * 8 + x));
            x += x_step;
            y += y_step;
        }
    }
    ray ^= (1ULL << from);
    ray |= (1ULL << to);
    return ray;
}

std::list<uint16_t> BitBoard::get_moves(uint8_t color) const
{
    uint64_t all_pieces = allPieces();
    std::list<uint16_t> moves;

    uint8_t king_square = __builtin_ctzll(m_bitboards[color][KING]);
    uint64_t checkers = squareAttackers(king_square, !color);
    uint8_t num_checkers = countBits(checkers);

    uint64_t attack_mask = get_attack_mask(!color);
    uint64_t check_resolve_capture_mask = 0xFFFFFFFFFFFFFFFFULL;
    uint64_t check_resolve_push_mask = 0xFFFFFFFFFFFFFFFFULL;
    check_resolve_capture_mask = checkers;

    uint64_t pinned = 0;
    uint64_t pinners = xrayRookAttacks(m_bitboards[color][ALL], king_square) & (m_bitboards[!color][ROOK] | m_bitboards[!color][QUEEN]);
    uint64_t tmpPinners = pinners;
    while (tmpPinners)
    {
        uint8_t square = __builtin_ctzll(tmpPinners);
        tmpPinners &= tmpPinners - 1;
        pinned |= create_ray(square, king_square) & m_bitboards[color][ALL];
    }
    tmpPinners = xrayBishopAttacks(m_bitboards[color][ALL], king_square) & (m_bitboards[!color][BISHOP] | m_bitboards[!color][QUEEN]);
    pinners |= tmpPinners;
    while (tmpPinners)
    {
        uint8_t square = __builtin_ctzll(tmpPinners);
        tmpPinners &= tmpPinners - 1;
        pinned |= create_ray(square, king_square) & m_bitboards[color][ALL];
    }

    if (checkers)
    {
        uint8_t checker_square = __builtin_ctzll(checkers);
        uint8_t checker_piece = at(checker_square);
        if (checker_piece == BISHOP || checker_piece == ROOK || checker_piece == QUEEN)
            check_resolve_push_mask = create_ray(king_square, checker_square) & ~(m_bitboards[color][ALL]);
        else
            check_resolve_push_mask = (m_en_passant_square == 255 ? 0 : (1ULL << m_en_passant_square));
    }

    for (size_t piece_type = (num_checkers > 1 ? KING : 1); piece_type < m_bitboards[color].size(); piece_type++)
    {
        uint64_t pieces = m_bitboards[color][piece_type];
        while (pieces)
        {
            uint8_t square = __builtin_ctzll(pieces);
            pieces &= pieces - 1;

            uint64_t moveBoard = 0;
            switch (piece_type)
            {
            case PAWN:
                moveBoard = get_pawn_moves(square);
                break;
            case KNIGHT:
                moveBoard = KNIGHT_MOVES[square] & ~(m_bitboards[color][ALL]);
                break;
            case KING:
                moveBoard = KING_MOVES[square] & ~(m_bitboards[color][ALL]);
                break;
            case BISHOP:
                moveBoard = get_bishop_moves(square, all_pieces) & ~(m_bitboards[color][ALL]);
                break;
            case ROOK:
                moveBoard = get_rook_moves(square, all_pieces) & ~(m_bitboards[color][ALL]);
                break;
            case QUEEN:
                moveBoard = (get_rook_moves(square, all_pieces) | get_bishop_moves(square, all_pieces)) & ~(m_bitboards[color][ALL]);
                break;
            }

            if (pinned & (1ULL << square) && piece_type != KING)
            {
                if (piece_type == KNIGHT)
                    moveBoard = 0;
                else
                {
                    uint64_t pinner = pinners;
                    while (pinner)
                    {
                        uint8_t pinner_square = __builtin_ctzll(pinner);
                        pinner &= pinner - 1;
                        uint64_t ray = create_ray(king_square, pinner_square);
                        if (ray & (1ULL << square))
                        {
                            moveBoard &= ray;
                            break;
                        }
                    }
                }
            }

            if (piece_type != KING)
                moveBoard &= (check_resolve_capture_mask | check_resolve_push_mask);
            else
                moveBoard &= ~(attack_mask);

            while (moveBoard)
            {
                uint8_t to = __builtin_ctzll(moveBoard);
                if (piece_type == PAWN && ((1ULL << to) & (ROW_1 | ROW_8)))
                {
                    moves.push_back(to | ((square & 0b111111) << 6) | (QUEEN << 12));
                    moves.push_back(to | ((square & 0b111111) << 6) | (KNIGHT << 12));
                    moves.push_back(to | ((square & 0b111111) << 6) | (ROOK << 12));
                    moves.push_back(to | ((square & 0b111111) << 6) | (BISHOP << 12));
                    moveBoard &= moveBoard - 1;
                    continue;
                }
                moveBoard &= moveBoard - 1;
                moves.push_back(to | ((square & 0b111111) << 6));
            }
        }
    }

    if (!checkers)
    {
        switch (color)
        {
        case WHITE:
            if ((m_castling_rights & WQ) && !(all_pieces & WQ_MASK) && !(WQ_ATTACK_MASK & attack_mask))
                moves.push_back(58 | (60 << 6));
            if ((m_castling_rights & WK) && !(all_pieces & WK_MASK) && !(WK_MASK & attack_mask))
                moves.push_back(62 | (60 << 6));
            break;
        case BLACK:
            if ((m_castling_rights & BQ) && !(all_pieces & BQ_MASK) && !(BQ_ATTACK_MASK & attack_mask))
                moves.push_back(2 | (4 << 6));
            if ((m_castling_rights & BK) && !(all_pieces & BK_MASK) && !(BK_MASK & attack_mask))
                moves.push_back(6 | (4 << 6));
            break;
        }
    }
    return moves;
}

uint64_t BitBoard::get_pawn_moves(uint8_t square) const
{
    uint64_t moves = 0;
    uint8_t rank = square / 8;
    uint8_t file = square % 8;
    uint8_t color = colorBoard(WHITE) & (1ULL << square) ? WHITE : BLACK;

    uint64_t all_pieces = allPieces();

    switch (color)
    {
    case WHITE:
        if (!(all_pieces & (1ULL << (square - 8))))
        {
            moves |= (1ULL << (square - 8));
            if (rank == 6 && !(all_pieces & (1ULL << (square - 16))))
                moves |= (1ULL << (square - 16));
        }
        if (file > 0 && (colorBoard(BLACK) & (1ULL << (square - 8 - 1)) || m_en_passant_square == square - 8 - 1))
            moves |= (1ULL << (square - 8 - 1));
        if (file < 7 && (colorBoard(BLACK) & (1ULL << (square - 8 + 1)) || m_en_passant_square == square - 8 + 1))
            moves |= (1ULL << (square - 8 + 1));
        if (moves & (1ULL << m_en_passant_square))
        {
            uint64_t blockers = all_pieces & ~((1ULL << square) | (1ULL << (m_en_passant_square + 8)));
            if ((get_rook_moves(__builtin_ctzll(m_bitboards[color][KING]), blockers) & (m_bitboards[!color][ROOK] | m_bitboards[!color][QUEEN]))
                || (get_bishop_moves(__builtin_ctzll(m_bitboards[color][KING]), blockers) & (m_bitboards[!color][BISHOP] | m_bitboards[!color][QUEEN])))
                moves ^= (1ULL << m_en_passant_square);
        }
        break;
    case BLACK:
        if (!(all_pieces & (1ULL << (square + 8))))
        {
            moves |= (1ULL << (square + 8));
            if (rank == 1 && !(all_pieces & (1ULL << (square + 16))))
                moves |= (1ULL << (square + 16));
        }
        if (file > 0 && (colorBoard(WHITE) & (1ULL << (square + 8 - 1)) || m_en_passant_square == square + 8 - 1))
            moves |= (1ULL << (square + 8 - 1));
        if (file < 7 && (colorBoard(WHITE) & (1ULL << (square + 8 + 1)) || m_en_passant_square == square + 8 + 1))
            moves |= (1ULL << (square + 8 + 1));
        if (moves & (1ULL << m_en_passant_square))
        {
            uint64_t blockers = all_pieces & ~((1ULL << square) | (1ULL << (m_en_passant_square - 8)));
            if ((get_rook_moves(__builtin_ctzll(m_bitboards[color][KING]), blockers) & (m_bitboards[!color][ROOK] | m_bitboards[!color][QUEEN]))
                || (get_bishop_moves(__builtin_ctzll(m_bitboards[color][KING]), blockers) & (m_bitboards[!color][BISHOP] | m_bitboards[!color][QUEEN])))
                moves ^= (1ULL << m_en_passant_square);
        }
        break;
    }

    return moves;
}

uint64_t BitBoard::get_bishop_moves(uint8_t square, uint64_t blockers) const
{
    blockers &= BISHOP_RELEVANT_MASKS[square];
    uint64_t key = (blockers * BISHOP_MAGICS[square]) >> (64 - BISHOP_SHIFTS[square]);
    return bishop_moves[square][key];
}

uint64_t BitBoard::get_rook_moves(uint8_t square, uint64_t blockers) const
{
    blockers &= ROOK_RELEVANT_MASKS[square];
    uint64_t key = (blockers * ROOK_MAGICS[square]) >> (64 - ROOK_SHIFTS[square]);
    return rook_moves[square][key];
}

std::ostream& operator<<(std::ostream& os, const BitBoard& board)
{
    static char pieceChars[] = {
        'P', 'N', 'B', 'R', 'Q', 'K',
        'p', 'n', 'b', 'r', 'q', 'k',
    };

    for (int8_t square = 0; square < 64; square++)
    {
        if (square % 8 == 0)
            os << "\n +---+---+---+---+---+---+---+---+\n |";
        if (board.occupied(square))
        {
            int piece = (int)board.at(square);
            uint8_t color = board.m_bitboards[WHITE][piece] & (1ULL << square) ? WHITE : BLACK;
            os << " " << pieceChars[piece - 1 + color * 6] << " |";
        }
        else
            os << " . |";
        if (square % 8 == 7)
            os << ' ' << 8 - square / 8;
    }
    os << "\n +---+---+---+---+---+---+---+---+\n   a   b   c   d   e   f   g   h" << '\n';
    os << '\n';
    os << "Castling rights: " << ((board.m_castling_rights & WK) ? "K" : "") << ((board.m_castling_rights & WQ) ? "Q" : "") << ((board.m_castling_rights & BK) ? "k" : "") << ((board.m_castling_rights & BQ) ? "q" : "") << '\n';
    if (board.m_en_passant_square != 255)
        os << "En passant square: " << std::string("abcdefgh")[board.m_en_passant_square % 8] << (8 - board.m_en_passant_square / 8) << '\n' << std::endl;
    else
        os << "En passant square: -" << '\n' << std::endl;
    return os;
}

void BitBoard::generate_rook_moves()
{
    for (int8_t square = 0; square < 64; square++)
    {
        for (size_t index = 0; index < rook_moves[square].size(); index++)
        {
            uint64_t blockers = generate_blockerboard_with_index(index, ROOK_RELEVANT_MASKS[square]);
            blockers &= ROOK_RELEVANT_MASKS[square];
            uint64_t key = (blockers * ROOK_MAGICS[square]) >> (64 - ROOK_SHIFTS[square]);

            uint64_t mask = 0;
            // Y+
            for (int8_t at = square; at < 64; at += 8)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at))
                    break;
            }
            // Y-
            for (int8_t at = square; at >= 0; at -= 8)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at))
                    break;
            }
            // X+
            for (int8_t at = square; true; at++)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & FILE_H))
                    break;
            }
            // X-
            for (int8_t at = square; true; at--)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & FILE_A))
                    break;
            }
            mask ^= (1ULL << square);
            rook_moves[square][key] = mask;
        }
    }
}

void BitBoard::generate_bishop_moves()
{
    for (int8_t square = 0; square < 64; square++)
    {
        for (size_t index = 0; index < bishop_moves[square].size(); index++)
        {
            uint64_t blockers = generate_blockerboard_with_index(index, BISHOP_RELEVANT_MASKS[square]);
            blockers &= BISHOP_RELEVANT_MASKS[square];
            uint64_t key = (blockers * BISHOP_MAGICS[square]) >> (64 - BISHOP_SHIFTS[square]);

            uint64_t mask = 0;
            // Y- X+
            for (int8_t at = square; true; at = at - 8 + 1)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & (FILE_H | ROW_1)))
                    break;
            }
            // Y+ X+
            for (int8_t at = square; true; at = at + 8 + 1)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & (FILE_H | ROW_8)))
                    break;
            }
            // Y+ X-
            for (int8_t at = square; true; at = at + 8 - 1)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & (FILE_A | ROW_8)))
                    break;
            }
            // Y- X-
            for (int8_t at = square; true; at = at - 8 - 1)
            {
                mask |= (1ULL << at);
                if (blockers & (1ULL << at) || ((1ULL << at) & (FILE_A | ROW_1)))
                    break;
            }

            mask ^= (1ULL << square);
            bishop_moves[square][key] = mask;
        }
    }
}

bool BitBoard::isSquareAttacked(uint8_t square, uint8_t attacker_color) const
{
    uint64_t all_pieces = allPieces();
    if (KNIGHT_MOVES[square] & m_bitboards[attacker_color][KNIGHT])
        return true;
    if (KING_MOVES[square] & m_bitboards[attacker_color][KING])
        return true;
    if (get_bishop_moves(square, all_pieces) & m_bitboards[attacker_color][KING])
        return true;
    if (get_rook_moves(square, all_pieces) & m_bitboards[attacker_color][KING])
        return true;
    return false;
}

uint64_t BitBoard::squareAttackers(uint8_t square, uint8_t attacker_color) const
{
    uint64_t all_pieces = allPieces();
    uint64_t pawnAttacks = 0;
    if (((1ULL << square) & FILE_A) == 0)
        pawnAttacks |= (1ULL << (square + (attacker_color == WHITE ? 8 : -8) - 1));
    if (((1ULL << square) & FILE_H) == 0)
        pawnAttacks |= (1ULL << (square + (attacker_color == WHITE ? 8 : -8) + 1));
    return  (pawnAttacks & m_bitboards[attacker_color][PAWN])
            | (KNIGHT_MOVES[square] & m_bitboards[attacker_color][KNIGHT])
            | (KING_MOVES[square] & m_bitboards[attacker_color][KING])
            | (get_bishop_moves(square, all_pieces) & (m_bitboards[attacker_color][BISHOP] | m_bitboards[attacker_color][QUEEN]))
            | (get_rook_moves(square, all_pieces) & (m_bitboards[attacker_color][ROOK] | m_bitboards[attacker_color][QUEEN]));
}

uint64_t BitBoard::xrayRookAttacks(uint64_t blockers, uint8_t square) const
{
    uint64_t all_pieces = allPieces();
    uint64_t attacks = get_rook_moves(square, all_pieces);
    blockers &= attacks;
    return attacks ^ get_rook_moves(square, all_pieces ^ blockers);
}


uint64_t BitBoard::xrayBishopAttacks(uint64_t blockers, uint8_t square) const
{
    uint64_t all_pieces = allPieces();
    uint64_t attacks = get_bishop_moves(square, all_pieces);
    blockers &= attacks;
    return attacks ^ get_bishop_moves(square, all_pieces ^ blockers);
}


/* -------------------------------------------------------------------------- */
/*                        Precomputed moves generation                        */
/* -------------------------------------------------------------------------- */

/* Prints a mask containing every possible move for a knight on a square */
void precomputed_knight_moves()
{
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t square_board = (1ULL << square);
        uint64_t mask = 0;

        if (!(square_board & (ROW_1 | FILE_H | FILE_G)))
            mask |= (1ULL << (square + 2 - 8));
        if (!(square_board & (ROW_1 | FILE_A | FILE_B)))
            mask |= (1ULL << (square - 2 - 8));
        if (!(square_board & (ROW_8 | FILE_H | FILE_G)))
            mask |= (1ULL << (square + 2 + 8));
        if (!(square_board & (ROW_8 | FILE_A | FILE_B)))
            mask |= (1ULL << (square - 2 + 8));
        if (!(square_board & (ROW_1 | ROW_2 | FILE_A)))
            mask |= (1ULL << (square - 1 - 16));
        if (!(square_board & (ROW_7 | ROW_8 | FILE_A)))
            mask |= (1ULL << (square - 1 + 16));
        if (!(square_board & (ROW_1 | ROW_2 | FILE_H)))
            mask |= (1ULL << (square + 1 - 16));
        if (!(square_board & (ROW_7 | ROW_8 | FILE_H)))
            mask |= (1ULL << (square + 1 + 16));
        std::cout << mask << "ULL, " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

/* Prints a mask containing every possible move for a king on a square */
void precomputed_king_moves()
{
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t square_board = (1ULL << square);
        uint64_t mask = 0;

        if (!(square_board & (ROW_1)))
            mask |= (1ULL << (square - 8));
        if (!(square_board & (ROW_8)))
            mask |= (1ULL << (square + 8));
        if (!(square_board & (FILE_H)))
            mask |= (1ULL << (square + 1));
        if (!(square_board & (FILE_A)))
            mask |= (1ULL << (square - 1));
        if (!(square_board & (ROW_1 | FILE_A)))
            mask |= (1ULL << (square - 1 - 8));
        if (!(square_board & (ROW_1 | FILE_H)))
            mask |= (1ULL << (square + 1 - 8));
        if (!(square_board & (ROW_8 | FILE_H)))
            mask |= (1ULL << (square + 1 + 8));
        if (!(square_board & (ROW_8 | FILE_A)))
            mask |= (1ULL << (square - 1 + 8));
        // printBitboard(mask);
        std::cout << mask << "ULL, " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

void rook_relevant_masks()
{
    uint64_t borders = (FILE_A | FILE_H | ROW_1 | ROW_8);
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t square_board = (1ULL << square);
        uint64_t mask = square_board;
        
        mask ^= ROWS[square / 8] | FILES[square % 8];
        if (!(square_board & borders))
            mask &= ~(borders & mask);
        else
        {
            uint64_t unrelevant_mask = borders & mask;
            if (square_board & ROW_1)
                unrelevant_mask &= ~ROW_1;
            if (square_board & ROW_8)
                unrelevant_mask &= ~ROW_8;
            if (square_board & FILE_A)
                unrelevant_mask &= ~FILE_A;
            if (square_board & FILE_H)
                unrelevant_mask &= ~FILE_H;
            unrelevant_mask |= (1ULL << 0) | (1ULL << 7) | (1ULL << 56) | (1ULL << 63); 
            mask &= ~unrelevant_mask;
        }

        // printBitboard(mask);
        std::cout << mask << "ULL, " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

void bishop_relevant_masks()
{
    uint64_t borders = (FILE_A | FILE_H | ROW_1 | ROW_8);
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t square_board = (1ULL << square);
        uint64_t mask = 0;
        
        // Y- X+
        for (int8_t at = square; at == square || !((1ULL << at) & borders); at = at - 8 + 1)
            mask |= (1ULL << at);
        // Y+ X+
        for (int8_t at = square; at == square || !((1ULL << at) & borders); at = at + 8 + 1)
            mask |= (1ULL << at);
        // Y+ X-
        for (int8_t at = square; at == square || !((1ULL << at) & borders); at = at + 8 - 1)
            mask |= (1ULL << at);
        // Y- X-
        for (int8_t at = square; at == square || !((1ULL << at) & borders); at = at - 8 - 1)
            mask |= (1ULL << at);

        mask ^= square_board;

        //printBitboard(mask);
        std::cout << mask << "ULL, " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

void precomputedRookShifts()
{
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t relevantBoard = ROOK_RELEVANT_MASKS[square];
        std::cout << (int)countBits(relevantBoard) << ", " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

void precomputedBishopShifts()
{
    for (int8_t square = 0; square < 64; square++)
    {
        uint64_t relevantBoard = BISHOP_RELEVANT_MASKS[square];
        std::cout << (int)countBits(relevantBoard) << ", " << (square % 8 == 7 ? "\n" : "");
    }
    std::cout << std::endl;
}

/* -------------------------------------------------------------------------- */
/*                                    Magic                                   */
/* -------------------------------------------------------------------------- */

/* Generates a unique blocker board by masking some bits in the mask passed in parameter. Each index will give a unique blocker board. */
uint64_t generate_blockerboard_with_index(int index, uint64_t blockermask)
{
    uint64_t blockerboard = blockermask;
    int8_t bitindex = 0;
    for (int8_t i = 0; i < 64; i++)
    {
        if (blockermask & (1ULL << i))
        {
            if (!(index & (1 << bitindex)))
                blockerboard &= ~(1ULL<<i);
            bitindex++;
        }
    }
    return blockerboard;
}

/* -------------------------------------------------------------------------- */
/*                                    Utils                                   */
/* -------------------------------------------------------------------------- */

/* Count the amount of set bits in the number */
uint8_t countBits(uint64_t n)
{
    uint8_t count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

/* Prints a bitboard in a human readable format */
void printBitboard(uint64_t bitboard)
{
    std::cout << "___________________";
    for (uint8_t i = 0; i < 64; i++)
    {
        if (i % 8 == 0)
            std::cout << '\n' << std::to_string(i / 8 + 1) << "| ";
        if (bitboard & (1ULL << i))
            std::cout << "1 ";
        else
            std::cout << ". ";
    }
    std::cout << '\n' << "-------------------" << '\n' <<         "   A B C D E F G H " << '\n';
    std::cout << std::endl;
}
