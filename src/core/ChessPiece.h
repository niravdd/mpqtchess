// src/core/ChessPiece.h
#pragma once
#include "Position.h"
#include <vector>

enum class PieceType {
    None,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

inline QString pieceTypeToString(PieceType pieceType) {
    switch (pieceType) {
        case PieceType::None: return "None";
        case PieceType::Pawn: return "Pawn";
        case PieceType::Knight: return "Knight";
        case PieceType::Bishop: return "Bishop";
        case PieceType::Rook: return "Rook";
        case PieceType::Queen: return "Queen";
        case PieceType::King: return "King";
    }
    return "Unknown";
}

enum class PieceColor {
    White,
    Black,
    None
};

class ChessPiece {
public:
    ChessPiece(PieceType type, PieceColor color);
    virtual ~ChessPiece() = default;
    
    PieceType getType() const { return type_; }
    PieceColor getColor() const { return color_; }
    std::vector<Position> getPossibleMoves(const Position& pos) const;
    bool hasMoved() const { return hasMoved_; }
    void setMoved() { hasMoved_ = true; }
    
    QString getImagePath() const;

protected:
    void getPawnMoves(const Position& pos, std::vector<Position>& moves) const;
    void getKnightMoves(const Position& pos, std::vector<Position>& moves) const;
    void getBishopMoves(const Position& pos, std::vector<Position>& moves) const;
    void getRookMoves(const Position& pos, std::vector<Position>& moves) const;
    void getQueenMoves(const Position& pos, std::vector<Position>& moves) const;
    void getKingMoves(const Position& pos, std::vector<Position>& moves) const;
    
private:
    PieceType type_;
    PieceColor color_;
    bool hasMoved_;
};