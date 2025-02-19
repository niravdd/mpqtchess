// src/gui/ChessPieceItem.cpp
#include "ChessPieceItem.h"
#include <QPixmap>

ChessPieceItem::ChessPieceItem(std::shared_ptr<ChessPiece> piece, QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
    , piece_(piece)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    
    // Enable SVG smooth scaling
    setTransformationMode(Qt::SmoothTransformation);
}

void ChessPieceItem::updateSize(qreal squareSize)
{
    QString resourcePath = getResourcePath();
    QSvgRenderer renderer(resourcePath);
    
    // Create a pixmap of the desired size
    QPixmap pixmap(squareSize, squareSize);
    pixmap.fill(Qt::transparent);  // Ensure transparent background
    
    // Render the SVG to the pixmap
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Get theme scale factor
    qreal scale = Settings::getInstance().getThemeScale(piece_->getColor());
    
    // Calculate scaled size and position
    qreal scaledSize = squareSize * scale;
    qreal offset = (squareSize - scaledSize) / 2;
    
    renderer.render(&painter, QRectF(offset, offset, scaledSize, scaledSize));
    setPixmap(pixmap);
}

QString ChessPieceItem::getResourcePath() const
{
    QString themeName = Settings::getInstance().getCurrentTheme().toLower();
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
    
    return QString(":/pieces/%1/%2_%3.svg")
           .arg(themeName)
           .arg(color)
           .arg(pieceName);
}