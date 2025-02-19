// src/core/ChessBoard.cpp
#include "ChessBoard.h"
#include <stdexcept>

ChessBoard::ChessBoard()
    : whiteKingPos_(7, 4)
    , blackKingPos_(0, 4)
{
    initializeBoard();
}

void ChessBoard::initializeBoard()
{
    // Clear the board
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            board_[row][col] = nullptr;
        }
    }

    // Set up black pieces
    board_[0][0] = std::make_shared<ChessPiece>(PieceType::ROOK, PieceColor::Black);
    board_[0][1] = std::make_shared<ChessPiece>(PieceType::KNIGHT, PieceColor::Black);
    board_[0][2] = std::make_shared<ChessPiece>(PieceType::BISHOP, PieceColor::Black);
    board_[0][3] = std::make_shared<ChessPiece>(PieceType::QUEEN, PieceColor::Black);
    board_[0][4] = std::make_shared<ChessPiece>(PieceType::KING, PieceColor::Black);
    board_[0][5] = std::make_shared<ChessPiece>(PieceType::BISHOP, PieceColor::Black);
    board_[0][6] = std::make_shared<ChessPiece>(PieceType::KNIGHT, PieceColor::Black);
    board_[0][7] = std::make_shared<ChessPiece>(PieceType::ROOK, PieceColor::Black);

    // Set up black pawns
    for (int col = 0; col < 8; col++) {
        board_[1][col] = std::make_shared<ChessPiece>(PieceType::PAWN, PieceColor::Black);
    }

    // Set up white pieces
    board_[7][0] = std::make_shared<ChessPiece>(PieceType::ROOK, PieceColor::White);
    board_[7][1] = std::make_shared<ChessPiece>(PieceType::KNIGHT, PieceColor::White);
    board_[7][2] = std::make_shared<ChessPiece>(PieceType::BISHOP, PieceColor::White);
    board_[7][3] = std::make_shared<ChessPiece>(PieceType::QUEEN, PieceColor::White);
    board_[7][4] = std::make_shared<ChessPiece>(PieceType::KING, PieceColor::White);
    board_[7][5] = std::make_shared<ChessPiece>(PieceType::BISHOP, PieceColor::White);
    board_[7][6] = std::make_shared<ChessPiece>(PieceType::KNIGHT, PieceColor::White);
    board_[7][7] = std::make_shared<ChessPiece>(PieceType::ROOK, PieceColor::White);

    // Set up white pawns
    for (int col = 0; col < 8; col++) {
        board_[6][col] = std::make_shared<ChessPiece>(PieceType::PAWN, PieceColor::White);
    }
}

std::shared_ptr<ChessPiece> ChessBoard::getPieceAt(const Position& pos) const
{
    if (!isValidPosition(pos)) {
        return nullptr;
    }
    return board_[pos.row][pos.col];
}

bool ChessBoard::movePiece(const Position& from, const Position& to, PieceType promotionPiece)
{
    if (!isValidPosition(from) || !isValidPosition(to)) {
        return false;
    }

    auto piece = board_[from.row][from.col];
    if (!piece) {
        return false;
    }

    // Handle pawn promotion
    if (promotionPiece != PieceType::None &&
        piece->getType() == PieceType::PAWN &&
        (to.row == 0 || to.row == 7)) {
        board_[to.row][to.col] = std::make_shared<ChessPiece>(promotionPiece, piece->getColor());
    } else {
        board_[to.row][to.col] = piece;
    }

    board_[from.row][from.col] = nullptr;
    piece->setMoved();

    // Update king position if king was moved
    if (piece->getType() == PieceType::KING) {
        if (piece->getColor() == PieceColor::White) {
            whiteKingPos_ = to;
        } else {
            blackKingPos_ = to;
        }
    }

    return true;
}

bool ChessBoard::removePiece(const Position& pos)
{
    if (!isValidPosition(pos)) {
        return false;
    }

    board_[pos.row][pos.col] = nullptr;
    return true;
}

std::vector<Position> ChessBoard::getPossibleMoves(const Position& pos) const
{
    std::vector<Position> moves;
    auto piece = getPieceAt(pos);
    
    if (!piece) {
        return moves;
    }

    switch (piece->getType()) {
        case PieceType::PAWN:
            getPawnMoves(pos, piece->getColor(), moves);
            break;
        case PieceType::KNIGHT:
            getKnightMoves(pos, piece->getColor(), moves);
            break;
        case PieceType::BISHOP:
            getBishopMoves(pos, piece->getColor(), moves);
            break;
        case PieceType::ROOK:
            getRookMoves(pos, piece->getColor(), moves);
            break;
        case PieceType::QUEEN:
            getQueenMoves(pos, piece->getColor(), moves);
            break;
        case PieceType::KING:
            getKingMoves(pos, piece->getColor(), moves);
            break;
        default:
            break;
    }

    return moves;
}

void ChessBoard::getPawnMoves(const Position& pos, PieceColor color, 
                            std::vector<Position>& moves) const
{
    int direction = (color == PieceColor::White) ? -1 : 1;
    int startRow = (color == PieceColor::White) ? 6 : 1;

    // Forward move
    Position forward(pos.row + direction, pos.col);
    if (isValidPosition(forward) && !getPieceAt(forward)) {
        moves.push_back(forward);

        // Double move from starting position
        if (pos.row == startRow) {
            Position doubleForward(pos.row + 2 * direction, pos.col);
            if (!getPieceAt(doubleForward)) {
                moves.push_back(doubleForward);
            }
        }
    }

    // Captures
    for (int colOffset : {-1, 1}) {
        Position capture(pos.row + direction, pos.col + colOffset);
        if (isValidPosition(capture)) {
            auto targetPiece = getPieceAt(capture);
            if (targetPiece && targetPiece->getColor() != color) {
                moves.push_back(capture);
            }
        }
    }
}

void ChessBoard::getKnightMoves(const Position& pos, PieceColor color,
                               std::vector<Position>& moves) const
{
    const int knightMoves[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2},
        {1,-2}, {1,2}, {2,-1}, {2,1}
    };

    for (const auto& move : knightMoves) {
        Position newPos(pos.row + move[0], pos.col + move[1]);
        if (isValidPosition(newPos)) {
            auto targetPiece = getPieceAt(newPos);
            if (!targetPiece || targetPiece->getColor() != color) {
                moves.push_back(newPos);
            }
        }
    }
}

void ChessBoard::getBishopMoves(const Position& pos, PieceColor color,
                               std::vector<Position>& moves) const
{
    const int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    getSlidingMoves(pos, color, directions, 4, moves);
}

void ChessBoard::getRookMoves(const Position& pos, PieceColor color,
                             std::vector<Position>& moves) const
{
    const int directions[4][2] = {{0,1}, {0,-1}, {1,0}, {-1,0}};
    getSlidingMoves(pos, color, directions, 4, moves);
}

void ChessBoard::getQueenMoves(const Position& pos, PieceColor color,
                              std::vector<Position>& moves) const
{
    getBishopMoves(pos, color, moves);
    getRookMoves(pos, color, moves);
}

void ChessBoard::getKingMoves(const Position& pos, PieceColor color,
                             std::vector<Position>& moves) const
{
    const int directions[8][2] = {
        {-1,-1}, {-1,0}, {-1,1},
        {0,-1},          {0,1},
        {1,-1},  {1,0},  {1,1}
    };

    for (const auto& dir : directions) {
        Position newPos(pos.row + dir[0], pos.col + dir[1]);
        if (isValidPosition(newPos)) {
            auto targetPiece = getPieceAt(newPos);
            if (!targetPiece || targetPiece->getColor() != color) {
                moves.push_back(newPos);
            }
        }
    }
}

void ChessBoard::getSlidingMoves(const Position& pos, PieceColor color,
                                const int directions[][2], int numDirections,
                                std::vector<Position>& moves) const
{
    for (int i = 0; i < numDirections; i++) {
        int row = pos.row + directions[i][0];
        int col = pos.col + directions[i][1];
        
        while (isValidPosition(Position(row, col))) {
            auto piece = getPieceAt(Position(row, col));
            if (!piece) {
                moves.push_back(Position(row, col));
            } else {
                if (piece->getColor() != color) {
                    moves.push_back(Position(row, col));
                }
                break;
            }
            row += directions[i][0];
            col += directions[i][1];
        }
    }
}

bool ChessBoard::canCastleKingside(PieceColor color) const
{
    int row = (color == PieceColor::White) ? 7 : 0;
    auto king = getPieceAt(Position(row, 4));
    auto rook = getPieceAt(Position(row, 7));
    
    return king && !king->hasMoved() &&
           rook && !rook->hasMoved() &&
           !getPieceAt(Position(row, 5)) &&
           !getPieceAt(Position(row, 6));
}

bool ChessBoard::canCastleQueenside(PieceColor color) const
{
    int row = (color == PieceColor::White) ? 7 : 0;
    auto king = getPieceAt(Position(row, 4));
    auto rook = getPieceAt(Position(row, 0));
    
    return king && !king->hasMoved() &&
           rook && !rook->hasMoved() &&
           !getPieceAt(Position(row, 1)) &&
           !getPieceAt(Position(row, 2)) &&
           !getPieceAt(Position(row, 3));
}

bool ChessBoard::isValidPosition(const Position& pos) const
{
    return pos.row >= 0 && pos.row < 8 && pos.col >= 0 && pos.col < 8;
}