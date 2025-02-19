// src/gui/ChessPieceItem.h
#pragma once
#include <QSvgRenderer>
#include <QPainter>
#include <QGraphicsPixmapItem>
#include "../core/ChessPiece.h"
#include "../core/Position.h"

class ChessPieceItem : public QGraphicsPixmapItem {
public:
    ChessPieceItem(std::shared_ptr<ChessPiece> piece, QGraphicsItem* parent = nullptr);
    
    void updateSize(qreal squareSize);
    std::shared_ptr<ChessPiece> getPiece() const { return piece_; }

private:
    std::shared_ptr<ChessPiece> piece_;
    QString getResourcePath() const;
};