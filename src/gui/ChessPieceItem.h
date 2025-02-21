// src/gui/ChessPieceItem.h
#pragma once
#include <QtSvg/QSvgRenderer>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsPixmapItem>
#include "../core/ChessPiece.h"
#include "../core/Position.h"
#include "../util/Settings.h"

class ChessPieceItem : public QGraphicsPixmapItem {
public:
    ChessPieceItem(std::shared_ptr<ChessPiece> piece, QGraphicsItem* parent = nullptr);
    
    void updateSize(qreal squareSize);
    std::shared_ptr<ChessPiece> getPiece() const { return piece_; }

private:
    std::shared_ptr<ChessPiece> piece_;
    QString getResourcePath() const;
};