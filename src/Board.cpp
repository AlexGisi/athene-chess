//
// Created by Alex Gisi on 2/17/22.
//

#include <iostream>
#include <cctype>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <optional>
#include "Board.h"
#include "MoveGen.h"
#include "assertions.cpp"
#include "util.cpp"

Board::Board() {
    setUpEmpty();
};

void Board::setUpEmpty() {
    for(int & piece : pieces)
        piece = OFFBOARD;

    for(int i : sq64ToSq120)
        pieces[i] = EMPTY;

    for(BitBoard & bb : pawns)
        bb = BitBoard();

    kingSq[0] = NO_SQ;
    kingSq[1] = NO_SQ;

    side = WHITE;
    enPas = NO_SQ;
    fiftyMove = 0;

    ply = 0;
    hisPly = 0;

    for(int & num : pceNum)
        num = 0;

    for(int i=0; i < 2; i++) {
        bigPce[i] = 0;
        majPce[i] = 0;
        minPce[i] = 0;
        material[i] = 0;
    }

    castlePerm = 0;

    for(auto & piece : pceList) {
        for(int & p : piece)
            p = NO_SQ;
    }

    posKey = PosKey(this);
}

void Board::setUp(const std::string& fen) {
    int rank = RANK_8;
    int file = FILE_A;
    int piece = 0;
    int count = 0;
    int i = 0;
    char c = '\0';
    std::istringstream ss(fen);
    int sq64 = 0;
    int sq120 = 0;

    ss >> std::noskipws;  // By default, >> skips white space, so change that.

    setUpEmpty();
    // Piece placement.
    while ((ss >> c) && !std::isspace(c)) {
        count = 1;
        switch(c) {
            case 'p': piece = bP; break;
            case 'r': piece = bR; break;
            case 'n': piece = bN; break;
            case 'b': piece = bB; break;
            case 'k': piece = bK; break;
            case 'q': piece = bQ; break;
            case 'P': piece = wP; break;
            case 'R': piece = wR; break;
            case 'N': piece = wN; break;
            case 'B': piece = wB; break;
            case 'K': piece = wK; break;
            case 'Q': piece = wQ; break;

            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                piece = EMPTY;
                count = c - '0';
                break;

            case '/':
                rank--;
                file = FILE_A;
                continue;

            default:
                throw std::runtime_error("Erroneous character in fen");
        }

        for(i = 0; i < count; i++) {
            sq64 = rank*8 + file;
            sq120 = sq64ToSq120[sq64];
            if(piece != EMPTY)
                pieces[sq120] = piece;
            file++;
        }
    }

    ss >> c;
    assert(c == 'w' || c == 'b');
    side = (c == 'w') ? WHITE : BLACK;
    ss >> c; // Space.

    castlePerm = 0;
    while((ss >> c) && !std::isspace(c)) {
        switch (c) {
            case 'K': castlePerm |= WKCA; break;
            case 'Q': castlePerm |= WQCA; break;
            case 'k': castlePerm |= BKCA; break;
            case 'q': castlePerm |= BQCA; break;
            case '-': break;
            default:
                throw std::runtime_error("Erroneous character in castling permission string.");
        }
    }
    ss >> c; // Space.

    // En passant square.
    if(c != '-') {
        file = c - 'a';
        ss >> c;
        rank = c - '1';

        assert(file >= FILE_A && file <= FILE_H);
        assert(rank >= RANK_1 && rank <= RANK_8);

        enPas = FR2SQ(file, rank);
    } else {
        enPas = NO_SQ;
    }


    // Move/halfmove clocks.
    // TODO.

    // Other initialization.
    posKey = PosKey(this);
    updateListsMaterial();

    assert(checkBoard());
}

Board::Board(const Board &rhs) {
    setUpEmpty();

    for(int i=0; i < BRD_SQ_NUM; ++i)
        pieces[i] = rhs.pieces[i];

    for(int i=0; i < 3; ++i)
        pawns[i] = rhs.pawns[i];

    kingSq[0] = rhs.kingSq[0];
    kingSq[1] = rhs.kingSq[1];

    side = rhs.side;
    enPas = rhs.enPas;
    fiftyMove = rhs.fiftyMove;

    ply = rhs.ply;
    hisPly = rhs.hisPly;

    for(int i=0; i < 13; ++i)
        pceNum[i] = rhs.pceNum[i];

    for(int i=0; i < 2; ++i) {
        bigPce[i] = rhs.bigPce[i];
        majPce[i] = rhs.majPce[i];
        minPce[i] = rhs.minPce[i];
        material[i] = rhs.material[i];
    }

    castlePerm = rhs.castlePerm;

    for(int i=0; i < 13; ++i) {
        for(int j=0; j < 10; ++j)
            pceList[i][j] = rhs.pceList[i][j];
    }

    posKey = rhs.posKey;
    pvTable = rhs.pvTable;
};

Board &Board::operator=(const Board &rhs) {
    // This self-assignment check is considered bad design.
    if(&rhs == this)
        return *this;

    setUpEmpty();

    for(int i=0; i < BRD_SQ_NUM; ++i)
        pieces[i] = rhs.pieces[i];

    for(int i=0; i < 3; ++i)
        pawns[i] = rhs.pawns[i];

    kingSq[0] = rhs.kingSq[0];
    kingSq[1] = rhs.kingSq[1];

    side = rhs.side;
    enPas = rhs.enPas;
    fiftyMove = rhs.fiftyMove;

    ply = rhs.ply;
    hisPly = rhs.hisPly;

    for(int i=0; i < 13; ++i)
        pceNum[i] = rhs.pceNum[i];

    for(int i=0; i < 2; ++i) {
        bigPce[i] = rhs.bigPce[i];
        majPce[i] = rhs.majPce[i];
        minPce[i] = rhs.minPce[i];
        material[i] = rhs.material[i];
    }

    castlePerm = rhs.castlePerm;

    for(int i=0; i < 13; ++i) {
        for(int j=0; j < 10; ++j)
            pceList[i][j] = rhs.pceList[i][j];
    }

    posKey = rhs.posKey;
    pvTable = rhs.pvTable;

    return *this;
}

// Not robust to incorrect fen strings.
Board::Board(const std::string& fen) {
    setUpEmpty();
    setUp(fen);
}

void Board::initSq120To64() {
    int index = 0;
    int file = FILE_A;
    int rank = RANK_1;
    int sq64 = 0;

    // Initialize array values.
    for(index = 0; index < 64; ++index) {
        sq120ToSq64[index] = 65;
    }
    for(index = 0; index < 64; ++index) {
        sq64ToSq120[index] = 120;
    }

    for(rank = RANK_1; rank <= RANK_8; ++rank) {
        for(file = FILE_A; file <= FILE_H; ++file) {
            int sq120 = FR2SQ(file, rank);
            sq64ToSq120[sq64] = sq120;
            sq120ToSq64[sq120] = sq64;
            sq64++;
        }
    }
}

void Board::reset() {
    *this = Board();
}

void Board::print() const {
    std::cout << to_str() << std::endl;
}

std::string Board::to_str() const {
    std::stringstream ss;

    int sq, file, rank, piece;
    for(rank = RANK_8; rank >= RANK_1; rank--) {
        ss << rank+1 << ' ';
        for(file = FILE_A; file <= FILE_H; file++) {
            sq = FR2SQ(file, rank);
            piece = pieces[sq];
            ss << std::setw(3) << pceChar[piece];
        }
        ss << std::endl;
    }

    ss << std::endl << "  ";
    for(file = FILE_A; file <= FILE_H; file++) {
        ss << std::setw(3) << char('a' + file);
    }
    ss << std::endl;

    ss << "Side: " << sideChar[side] << std::endl;
    ss << "EnPas: " << enPas << std::endl;
    ss << "Castle: ";
    ss << ( (castlePerm & WKCA) ? 'K' : '-' );
    ss << ( (castlePerm & WQCA) ? 'Q' : '-' );
    ss << ( (castlePerm & BKCA) ? 'k' : '-' );
    ss << ( (castlePerm & BQCA) ? 'q' : '-' );
    ss << std::endl;
    ss << "PosKey: " << posKey.val << std::endl;

    return ss.str();
}

void Board::updateListsMaterial() {
    int piece, sq, col, idx;
    for(idx = 0; idx < BRD_SQ_NUM; ++idx) {
        sq = idx;
        piece = pieces[idx];
        if(piece != OFFBOARD && piece != EMPTY) {
            col = pieceCol[piece];

            if(pieceBig[piece]) bigPce[col]++;
            if(pieceMin[piece]) minPce[col]++;
            if(pieceMaj[piece]) majPce[col]++;

            material[col] += pieceVal[piece];

            pceList[piece][pceNum[piece]] = sq;
            pceNum[piece]++;

            if(piece == wK) kingSq[WHITE] = sq;
            if(piece == bK) kingSq[BLACK] = sq;

            if(piece == wP) {
                pawns[WHITE].setBit(sq120ToSq64[sq]);
                pawns[BOTH].setBit(sq120ToSq64[sq]);
            } else if(piece == bP) {
                pawns[BLACK].setBit(sq120ToSq64[sq]);
                pawns[BOTH].setBit(sq120ToSq64[sq]);
            }
        }
    }
}

void Board::initFilesRanksBrd() {
    int idx = 0;
    int file = FILE_A;
    int rank = RANK_1;
    int sq = A1;
    int sq64 = 0;

    for(idx = 0; idx < BRD_SQ_NUM; ++idx) {
        filesBrd[idx] = OFFBOARD;
        ranksBrd[idx] = OFFBOARD;
    }

    for(rank = RANK_1; rank <= RANK_8; ++rank) {
        for(file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file, rank);
            filesBrd[sq] = file;
            ranksBrd[sq] = rank;
        }
    }
}

// Asserts accuracy of board-tracking arrays. Useful after series of captures or undoes.
// Always returns true or throws an error.
bool Board::checkBoard() const {
    // Temporary arrays.
    int t_pceNum[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int t_bigPce[2] = { 0, 0 };
    int t_majPce[2] = { 0, 0 };
    int t_minPce[2] = { 0, 0 };
    int t_material[2] = { 0, 0 };

    int sq64, t_piece, t_pce_num, sq120, color, p_count;
    BitBoard t_pawns[3] = { BitBoard(), BitBoard(), BitBoard() };

    t_pawns[WHITE] = pawns[WHITE];
    t_pawns[BLACK] = pawns[BLACK];
    t_pawns[BOTH] = pawns[BOTH];

    // Check piece lists.
    for(t_piece = wP; t_piece <= bK; ++t_piece) {
        for(t_pce_num = 0; t_pce_num < pceNum[t_piece]; ++t_pce_num) {
            sq120 = pceList[t_piece][t_pce_num];
            assert(pieces[sq120] == t_piece);
        }
    }

    // Check piece count and other counters.
    for(sq64 = 0; sq64 < 64; ++sq64) {
        sq120 = sq64ToSq120[sq64];
        t_piece = pieces[sq120];
        t_pceNum[t_piece]++;
        color = pieceCol[t_piece];
        if(pieceBig[t_piece]) t_bigPce[color]++;
        if(pieceMin[t_piece]) t_minPce[color]++;
        if(pieceMaj[t_piece]) t_majPce[color]++;

        t_material[color] += pieceVal[t_piece];
    }

    for(t_piece = wP; t_piece <= bK; ++t_piece)
        assert(t_pceNum[t_piece] == pceNum[t_piece]);

    // Check bitboard counts.
    p_count = t_pawns[WHITE].countBits();
    assert(p_count == pceNum[wP]);
    p_count = t_pawns[BLACK].countBits();
    assert(p_count == pceNum[bP]);
    p_count = t_pawns[BOTH].countBits();
    assert(p_count == (pceNum[wP] + pceNum[bP]));

    // Check bitboard squares.
    while(t_pawns[WHITE]) {
        sq64 = t_pawns[WHITE].popBit();
        assert(pieces[sq64ToSq120[sq64]] == wP);
    }
    while(t_pawns[BLACK]) {
        sq64 = t_pawns[BLACK].popBit();
        assert(pieces[sq64ToSq120[sq64]] == bP);
    }
    while(t_pawns[BOTH]) {
        sq64 = t_pawns[BOTH].popBit();
        assert( (pieces[sq64ToSq120[sq64]] == wP) || (pieces[sq64ToSq120[sq64]] == bP) );
    }

    // Additional sanity checks.
    assert(t_material[WHITE]==material[WHITE] && t_material[BLACK]==material[BLACK]);
    assert(t_minPce[WHITE]==minPce[WHITE] && t_minPce[BLACK]==minPce[BLACK]);
    assert(t_majPce[WHITE]==majPce[WHITE] && t_majPce[BLACK]==majPce[BLACK]);
    assert(t_bigPce[WHITE]==bigPce[WHITE] && t_bigPce[BLACK]==bigPce[BLACK]);

    assert(side==WHITE || side==BLACK);
    assert(PosKey(this) == posKey);

    assert(enPas==NO_SQ || (ranksBrd[enPas]==RANK_6 && side==WHITE)
        || (ranksBrd[enPas]==RANK_3 && side==BLACK));

    assert(pieces[kingSq[WHITE]] == wK);
    assert(pieces[kingSq[BLACK]] == bK);

    return true;
}

// Determine if a square is attacked by a side.
bool Board::sqAttacked(const int sq, const int att_side) const {
    assert(sqOnBoard(sq));
    assert(sideValid(side));
    assert(checkBoard());

    int pce, idx, t_sq, dir = 0;

    // Pawns.
    if(att_side == WHITE) {
        if(pieces[sq-11] == wP || pieces[sq-9] == wP)
            return true;
    } else {
        if(pieces[sq+11] == bP || pieces[sq+9] == bP)
            return true;
    }

    // Knights.
    for(idx = 0; idx < 8; ++idx) {
        pce = pieces[sq+knDir[idx]];
        if(pce != OFFBOARD && pce != EMPTY && isKn(pce) && pieceCol[pce]==att_side)
            return true;
    }

    // Rooks, queens.
    for(idx = 0; idx < 4; ++idx) {
        dir = rkDir[idx];
        t_sq = sq + dir;
        pce = pieces[t_sq];
        while(pce != OFFBOARD) {
            if(pce != EMPTY) {
                if(isRQ(pce) && pieceCol[pce]==att_side)
                    return true;
                break;
            }
            t_sq += dir;
            pce = pieces[t_sq];
        }
    }

    // Bishops, queens.
    for(idx = 0; idx < 4; ++idx) {
        dir = biDir[idx];
        t_sq = sq + dir;
        pce = pieces[t_sq];
        while(pce != OFFBOARD) {
            if(pce != EMPTY) {
                if(isBQ(pce) && pieceCol[pce]==att_side)
                    return true;
                break;
            }
            t_sq += dir;
            pce = pieces[t_sq];
        }
    }

    // Kings.
    for(idx = 0; idx < 8; ++idx) {
        pce = pieces[sq+kiDir[idx]];
        if(pce != OFFBOARD && pce != EMPTY && isKi(pce) && pieceCol[pce]==att_side)
            return true;
    }

    return false;
}

// Testing function for sqAttacked.
void Board::showSqAttBySide(int att_side) const {
    int rank = 0;
    int file = 0;
    int sq = 0;

    printf("\n\nSquares attacked by:%c\n", sideChar[att_side]);
    for(rank = RANK_8; rank >= RANK_1; --rank) {
        for(file = FILE_A; file <= FILE_H; ++file) {
            sq = FR2SQ(file,rank);
            if(sqAttacked(sq, att_side)) {
                printf("X");
            } else {
                printf("-");
            }

        }
        printf("\n");
    }
    printf("\n\n");
}

std::string Board::sqToStr(const int sq) {
    std::string s1;
    std::string s2;

    s1 = (char) ('a' + (char) filesBrd[sq]);
    s2 = (char) ('1' + (char) ranksBrd[sq]);

    return s1 + s2;
}

/*
 * sq: 120-based square
 */
void Board::clearPiece(const int sq) {
    assert(sqOnBoard(sq));

    int pce = pieces[sq];
    assert(pieceValid(pce));

    int col = pieceCol[pce];
    int index = 0;
    int t_pceNum = -1;

    posKey.hashPce(pce, sq);

    pieces[sq] = EMPTY;
    material[col] -= pieceVal[pce];

    if(pieceBig[pce]) {
        bigPce[col]--;
        if(pieceMaj[pce])
            majPce[col]--;
        else
            minPce[col]--;
    } else {
        pawns[col].clearBit(sq120ToSq64[sq]);
        pawns[BOTH].clearBit(sq120ToSq64[sq]);
    }

    // Remove piece from piece list; see
    // https://www.youtube.com/watch?v=F_L2AhqB4V4&list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg&index=39
    // for detailed explanation.
    for(index = 0; index < pceNum[pce]; ++index) {
        if(pceList[pce][index] == sq) {
            t_pceNum = index;
            break;
        }
    }
    assert(t_pceNum != -1);
    pceNum[pce]--;
    pceList[pce][t_pceNum] = pceList[pce][pceNum[pce]];
}

void Board::addPiece(const int sq, const int pce) {
    assert(pieceValid(pce));
    assert(sqOnBoard(sq));

    int col = pieceCol[pce];

    posKey.hashPce(pce, sq);

    pieces[sq] = pce;

    if(pieceBig[pce]) {
        bigPce[col]++;
        if(pieceMaj[pce])
            majPce[col]++;
        else
            minPce[col]++;
    } else {
        pawns[col].setBit(sq120ToSq64[sq]);
        pawns[BOTH].setBit(sq120ToSq64[sq]);
    }
    material[col] += pieceVal[pce];
    pceList[pce][pceNum[pce]++] = sq;
}

void Board::movePiece(const int from, const int to) {
    assert(sqOnBoard(from));
    assert(sqOnBoard(to));

    int idx = 0;
    int pce = pieces[from];
    int col = pieceCol[pce];

    int t_pieceNum = false; // TODO: remove

    posKey.hashPce(pce, from);
    pieces[from] = EMPTY;

    posKey.hashPce(pce, to);
    pieces[to] = pce;

    if(!pieceBig[pce]) {
        pawns[col].clearBit(sq120ToSq64[from]);
        pawns[BOTH].clearBit(sq120ToSq64[from]);
        pawns[col].setBit(sq120ToSq64[to]);
        pawns[BOTH].setBit(sq120ToSq64[to]);
    }

    for(idx = 0; idx < pceNum[pce]; ++idx) {
        if(pceList[pce][idx] == from) {
            pceList[pce][idx] = to;
            t_pieceNum = true;  // TODO: remove debug
            break;
        }
    }
    assert(t_pieceNum);
}

bool Board::makeMove(Move& move) {
    assert(checkBoard());

    int from = move.from();
    int to = move.to();

    assert(sqOnBoard(from));
    assert(sqOnBoard(to));
    assert(sideValid(side));
    assert(pieceValid(pieces[from]));

    history[hisPly].posKey = posKey;

    if(move.ep_capture()) {
        if(side == WHITE)
            clearPiece(to-10);
        else
            clearPiece(to+10);
    } else if (move.castle()) {
        switch(to) {
            case C1:
                movePiece(A1, D1);
                break;
            case C8:
                movePiece(A8, D8);
                break;
            case G1:
                movePiece(H1, F1);
                break;
            case G8:
                movePiece(H8, F8);
                break;
            default:
                assert(false);
                break;
        }
    }

    if(enPas != NO_SQ) posKey.hashEp(enPas);
    posKey.hashCa(castlePerm);

    history[hisPly].move = move;
    history[hisPly].fiftyMove = fiftyMove;
    history[hisPly].enPas = enPas;
    history[hisPly].castlePerm = castlePerm;

    castlePerm &= castlePermArr[from];
    castlePerm &= castlePermArr[to];
    enPas = NO_SQ;

    posKey.hashCa(castlePerm);

    fiftyMove++;

    int cap = move.captured();
    if(cap != EMPTY && !move.ep_capture()) {
        assert(pieceValid(cap));
        clearPiece(to);
        fiftyMove = 0;
    }

    hisPly++;
    ply++;

    if(piecePawn[pieces[from]]) {
        fiftyMove = 0;
        if(move.pawn_start()) {
            if(side == WHITE) {
                enPas = from+10;
                assert(ranksBrd[enPas == RANK_3]);
            } else {
                enPas = from-10;
                assert(ranksBrd[enPas] == RANK_6);
            }
            posKey.hashEp(enPas);
        }
    }

    movePiece(from, to);

    // Check promotion.
    int pro = move.promoted();
    if(pro != EMPTY) {
        assert(pieceValid(pro) && !piecePawn[pro]);
        clearPiece(to);
        addPiece(to, pro);
    }

    // TODO: remove debug
    if(pieceKing[pieces[to]]) {
        kingSq[side] = to;
    }

    side ^= 1;
    posKey.hashSide();

    assert(checkBoard());

    // Ensure the move didn't leave the king in check.
    if(sqAttacked(kingSq[side^1], side)) {
        takeMove();
        return false;
    }

    return true;
}

void Board::takeMove() {
    assert(checkBoard());

    hisPly--;
    ply--;

    Move move = history[hisPly].move;
    int from = move.from();
    int to = move.to();

    assert(sqOnBoard(from));
    assert(sqOnBoard(to));

    castlePerm = history[hisPly].castlePerm;
    fiftyMove = history[hisPly].fiftyMove;
    enPas = history[hisPly].enPas;

    side ^= 1;

    if(move.ep_capture()) {
        if(side == WHITE)
            addPiece(to-10, bP);
        else
            addPiece(to+10, wP);
    } else if(move.castle()) {
        switch (to) {
            case C1:
                movePiece(D1, A1);
                break;
            case C8:
                movePiece(D8, A8);
                break;
            case G1:
                movePiece(F1, H1);
                break;
            case G8:
                movePiece(F8, H8);
                break;
            default:
                assert(false);
        }
    }

    movePiece(to, from);

    if(pieceKing[pieces[from]])
        kingSq[side] = from;

    int cap = move.captured();
    if(cap != EMPTY && !move.ep_capture()) {
        assert(pieceValid(cap));
        addPiece(to, cap);
    }

    int pro = move.promoted();
    if(pro != EMPTY) {
        assert(pieceValid(pro) && !piecePawn[pro]);
        clearPiece(from);
        addPiece(from, pieceCol[pro] == WHITE ? wP : bP);
    }

    posKey = history[hisPly].posKey;

    assert(checkBoard());
}

/*
 * Ref: https://www.chessprogramming.org/Perft#Perft_function
 */
u64 Board::perft(int depth) {
    if (depth == 0)
        return 1ULL;

    u64 nodes = 0;
    MoveGen mg(this);
    MoveList ml;
    mg.generateAllMoves(&ml);
    for(int i = 0; i < ml.count; i++) {
        if (!makeMove(ml.moves[i].move))
            continue;
        nodes += perft(depth-1);
        takeMove();
    }

    return nodes;
}

/*
 * Assert agreement of move generation with pre-specified perft figures supplied by
 * perftsuite.epd, a list of challenging move generation positions.
 * The path to perftsuite.epd specified in Defs.h.
 */
void Board::perft_suite(int depth) {
    std::ifstream infile(PERFTRESULTSPATH);
    std::string line;
    u64 correct[6];
    int line_num = 1;
    int pass_num = 0;

    std::cout << "Perft suite to depth " << depth << " (pass/fail - result - correct)" << std::endl;
    while(std::getline(infile, line)) {
        std::vector<std::string> l = parse_epd_line(line);
        std::string fen = l[0];
        for(int i=0; i < 6; i++)
            correct[i] = std::stoull(l[i+1]);

        std::cout << line_num << ": ";
        if (perft_eval_pos(depth, fen, correct))
            pass_num++;
        line_num++;
    }
    std::cout << '\n' << "Passed " << pass_num << '/' << "126" << std::endl;
}

bool Board::perft_eval_pos(int depth, const std::string& fen, const u64* correct) {
    if (depth < 1 || depth > 6) throw std::invalid_argument("Depth should be between one and six.");

    setUpEmpty();
    setUp(fen);
    u64 res = perft(depth);

    bool pass = (res == correct[depth-1]);
    if (pass)
        std::cout << "PASS - ";
    else
        std::cout << "FAIL - ";
    std::cout << res << " - " << correct[depth-1] << std::endl;

    return pass;
}

/*
 * It may seem strange to parse a string into a move in the board class, but it
 * makes identifying the move easier, and we can correctly tag it as en passant
 * and the like.
 */
Move Board::getMove(std::string str) {
    if (str[1] > '8' || str[1] < '1' || str[3] > '8' || str[3] < '1' || str[0] > 'h'
    || str[0] < 'a' || str[2] > 'h' || str[2] < 'a' || str.length() < 4 || str.length() > 6)
        throw std::invalid_argument("Bad string input");

    int from = FR2SQ(str[0] - 'a', str[1] - '1');
    int to = FR2SQ(str[2] - 'a', str[3] - '1');

    assert(sqOnBoard(from) && sqOnBoard(to));

    MoveList list;
    MoveGen mg(this);
    mg.generateAllMoves(&list);
    for (int i=0; i < list.count; i++) {
        Move m = list.moves[i].move;
        if(m.from() == from && m.to() == to) {
            int pro = m.promoted();
            if(pro != EMPTY) {
                if(isRQ(pro) && !isBQ(pro) && str[4] == 'r')
                    return m;
                else if(!isRQ(pro) && isBQ(pro) && str[4] == 'b')
                    return m;
                else if(isRQ(pro) && isBQ(pro) && str[4] == 'q')
                    return m;
                else if(isKn(pro) && str[4] == 'n')
                    return m;
                continue;
            }
            return m;
        }
    }
    throw std::invalid_argument("Bad string input");
}

int Board::getPVLine(const int depth) {
    assert(depth < MAX_DEPTH);

    std::optional<Move> mv_opt = pvTable.probe(posKey);
    int count = 0;
    while (mv_opt && count < depth) {
        assert(count < MAX_DEPTH);

        if(MoveGen(this).moveValid(mv_opt.value())) {
            makeMove(mv_opt.value());
            pvArray[count++] = mv_opt.value();
        } else
            break;

        mv_opt = pvTable.probe(posKey);
    }

    while (ply > 0)
        takeMove();

    return count;
}

int Board::sq120ToSq64[BRD_SQ_NUM];
int Board::sq64ToSq120[64];
int Board::filesBrd[BRD_SQ_NUM];
int Board::ranksBrd[BRD_SQ_NUM];
