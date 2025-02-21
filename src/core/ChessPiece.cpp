// src/core/ChessPiece.cpp
#include "ChessPiece.h"

ChessPiece::ChessPiece(PieceType type, PieceColor color)
    : type_(type)
    , color_(color)
    , hasMoved_(false)
{
}

std::vector<Position> ChessPiece::getPossibleMoves(const Position& pos) const
{
    std::vector<Position> moves;
    
    switch (type_) {
        case PieceType::Pawn:
            getPawnMoves(pos, moves);
            break;
        case PieceType::Knight:
            getKnightMoves(pos, moves);
            break;
        case PieceType::Bishop:
            getBishopMoves(pos, moves);
            break;
        case PieceType::Rook:
            getRookMoves(pos, moves);
            break;
        case PieceType::Queen:
            getQueenMoves(pos, moves);
            break;
        case PieceType::King:
            getKingMoves(pos, moves);
            break;
        default:
            break;
    }
    
    return moves;
}

/*
void ChessPiece::getPawnMoves(const Position& pos, std::vector<Position>& moves) const
{
    int direction = (color_ == PieceColor::White) ? -1 : 1;
    
    // Forward move
    Position forward(pos.row + direction, pos.col);
    if (isValidPosition(forward))
        moves.push_back(forward);
    
    // Initial double move
    if (!hasMoved_) {
        Position doubleForward(pos.row + 2 * direction, pos.col);
        if (isValidPosition(doubleForward))
            moves.push_back(doubleForward);
    }
    
    // Captures
    Position captureLeft(pos.row + direction, pos.col - 1);
    Position captureRight(pos.row + direction, pos.col + 1);
    
    if (isValidPosition(captureLeft))
        moves.push_back(captureLeft);
    if (isValidPosition(captureRight))
        moves.push_back(captureRight);
}
*/

void ChessPiece::getPawnMoves(const Position& pos, std::vector<Position>& moves) const {
    // Direction depends on pawn color: white moves up (-1), black moves down (+1)
    int direction = (color_ == PieceColor::White) ? -1 : 1;
    
    // Forward move
    int newRow = pos.row + direction;
    if (newRow >= 0 && newRow < 8) {
        moves.push_back(Position(newRow, pos.col));
        
        // Initial two-square move if pawn is in starting position
        if ((color_ == PieceColor::White && pos.row == 6) || (color_ == PieceColor::Black && pos.row == 1)) {
            newRow += direction;
            moves.push_back(Position(newRow, pos.col));
        }
    }

    // Diagonal capture moves
    if (pos.col > 0) {  // Left diagonal
        moves.push_back(Position(pos.row + direction, pos.col - 1));
    }
    if (pos.col < 7) {  // Right diagonal
        moves.push_back(Position(pos.row + direction, pos.col + 1));
    }
}

void ChessPiece::getKnightMoves(const Position& pos, std::vector<Position>& moves) const {
    // Knight moves in L-shape: 2 squares in one direction and 1 square perpendicular
    const int knightMoves[8][2] = {
        {-2, -1}, {-2, 1},  // Up 2, left/right 1
        {2, -1},  {2, 1},   // Down 2, left/right 1
        {-1, -2}, {1, -2},  // Left 2, up/down 1
        {-1, 2},  {1, 2}    // Right 2, up/down 1
    };

    for (const auto& move : knightMoves) {
        Position newPos(pos.row + move[0], pos.col + move[1]);
        if (newPos.row >= 0 && newPos.row < 8 && newPos.col >= 0 && newPos.col < 8) {
            moves.push_back(newPos);
        }
    }
}

void ChessPiece::getBishopMoves(const Position& pos, std::vector<Position>& moves) const {
    // Bishop moves diagonally in all four directions
    const int bishopDirections[4][2] = {
        {-1, -1},  // up-left
        {-1, 1},   // up-right
        {1, -1},   // down-left
        {1, 1}     // down-right
    };

    for (const auto& dir : bishopDirections) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        // Continue in each diagonal direction until edge of board
        while (row >= 0 && row < 8 && col >= 0 && col < 8) {
            moves.push_back(Position(row, col));
            row += dir[0];
            col += dir[1];
        }
    }
}

void ChessPiece::getRookMoves(const Position& pos, std::vector<Position>& moves) const {
    // Rook moves horizontally and vertically
    const int rookDirections[4][2] = {
        {-1, 0},  // up
        {1, 0},   // down
        {0, -1},  // left
        {0, 1}    // right
    };

    for (const auto& dir : rookDirections) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        // Continue in each direction until edge of board
        while (row >= 0 && row < 8 && col >= 0 && col < 8) {
            moves.push_back(Position(row, col));
            row += dir[0];
            col += dir[1];
        }
    }
}

void ChessPiece::getQueenMoves(const Position& pos, std::vector<Position>& moves) const {
    // Queen moves both like a rook and bishop combined
    const int queenDirections[8][2] = {
        {-1, 0},   // up
        {1, 0},    // down
        {0, -1},   // left
        {0, 1},    // right
        {-1, -1},  // up-left
        {-1, 1},   // up-right
        {1, -1},   // down-left
        {1, 1}     // down-right
    };

    for (const auto& dir : queenDirections) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        // Continue in each direction until edge of board
        while (row >= 0 && row < 8 && col >= 0 && col < 8) {
            moves.push_back(Position(row, col));
            row += dir[0];
            col += dir[1];
        }
    }
}

void ChessPiece::getKingMoves(const Position& pos, std::vector<Position>& moves) const {
    // King moves one square in any direction
    const int kingDirections[8][2] = {
        {-1, 0},   // up
        {1, 0},    // down
        {0, -1},   // left
        {0, 1},    // right
        {-1, -1},  // up-left
        {-1, 1},   // up-right
        {1, -1},   // down-left
        {1, 1}     // down-right
    };

    for (const auto& dir : kingDirections) {
        int row = pos.row + dir[0];
        int col = pos.col + dir[1];
        
        // Check if the new position is within board boundaries
        if (row >= 0 && row < 8 && col >= 0 && col < 8) {
            moves.push_back(Position(row, col));
        }
    }
}