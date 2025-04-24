// src/gui/ChessPieceItem.cpp
#include "ChessPieceItem.h"
#include <QtGui/QPixmap>

ChessPieceItem::ChessPieceItem(std::shared_ptr<ChessPiece> piece, QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
    , piece_(piece)
{
    setFlag(::QGraphicsItem::ItemIsMovable);
    setFlag(::QGraphicsItem::ItemSendsGeometryChanges);
    
    // Enable SVG smooth scaling
    setTransformationMode(::Qt::SmoothTransformation);
}

void ChessPieceItem::updateSize(qreal squareSize)
{
    lastSquareSize_ = squareSize;
    QString resourcePath = getResourcePath();
    QPixmap pixmap(resourcePath);
    
    // Create a pixmap of the desired size
    QPixmap scaledPixmap(squareSize, squareSize);
    scaledPixmap.fill(::Qt::transparent);  // Ensure transparent background
    
    // Scale the pixmap to the desired size
    scaledPixmap = pixmap.scaled(squareSize, squareSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Get theme scale factor
    qreal scale = ThemeManager::getInstance().getCurrentTheme().pieceScale;
    
    // Calculate scaled size and position
    qreal scaledSize = squareSize * scale;
    qreal offset = (squareSize - scaledSize) / 2;
    
    // Draw the scaled pixmap onto the new pixmap
    QPainter painter(&scaledPixmap);
    painter.drawPixmap(QRectF(offset, offset, scaledSize, scaledSize), pixmap, QRectF(0, 0, pixmap.width(), pixmap.height()));
    setPixmap(scaledPixmap);
}

QString ChessPieceItem::getResourcePath() const
{
    // Potential null/empty checks
    if (!piece_)
    {
        qWarning() << "from ChessPieceItem::getResourcePath(): Attempt to get resource path for null piece";
        return QString();
    }

    QString themeName = (currentTheme_.isEmpty() ? Settings::getInstance().getCurrentTheme().toLower() : currentTheme_);
    QString color = (piece_->getColor() == PieceColor::White) ? "white" : "black";
    QString pieceName;
    
    switch (piece_->getType()) {
        case PieceType::King:   pieceName = "king";   break;
        case PieceType::Queen:  pieceName = "queen";  break;
        case PieceType::Rook:   pieceName = "rook";   break;
        case PieceType::Bishop: pieceName = "bishop"; break;
        case PieceType::Knight: pieceName = "knight"; break;
        case PieceType::Pawn:   pieceName = "pawn";   break;
        default:                pieceName = "unknown"; break;
    }

//  return QString(":/pieces/%1/%2_%3.svg")
    return QString(":/pieces/%1/%2_%3.png")
           .arg(themeName)
           .arg(color)
           .arg(pieceName);
}

void ChessPieceItem::setTheme(const QString& themeName)
{
    if (currentTheme_ != themeName) {
        currentTheme_ = themeName.toLower();
        // Re-render with new theme if we have valid size
        if (lastSquareSize_ > 0) {
            updateSize(lastSquareSize_);
        }
    }
}