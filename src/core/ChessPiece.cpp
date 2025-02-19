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

// [Implementation of other piece movement methods...]

bool ChessPiece::isValidPosition(const Position& pos) const
{
    return pos.row >= 0 && pos.row < 8 && pos.col >= 0 && pos.col < 8;
}