// src/core/ChessPiece.h
#pragma once
#include "Position.h"
#include <QString>
#include <QPoint>

enum class PieceType {
    None,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum class PieceColor {
    White,
    Black
};

class ChessPiece {
public:
    ChessPiece(PieceType type, PieceColor color);
    
    PieceType getType() const { return type_; }
    PieceColor getColor() const { return color_; }
    bool hasMoved() const { return hasMoved_; }
    void setMoved() { hasMoved_ = true; }
    
    QString getImagePath() const;
    
private:
    PieceType type_;
    PieceColor color_;
    bool hasMoved_;
};