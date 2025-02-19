// src/gui/ChessBoardView.cpp
#include "ChessBoardView.h"
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>

ChessBoardView::ChessBoardView(QWidget* parent)
    : QGraphicsView(parent)
    , scene_(new QGraphicsScene(this))
    , game_(std::make_unique<ChessGame>())
    , selectedPiece_(nullptr)
{
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Create board squares and highlight items
    setupBoard();
    updateBoard();
}

void ChessBoardView::setupBoard()
{
    const int BOARD_SIZE = 8;
    qreal squareSize = 80; // Initial size, will be adjusted in resizeEvent

    const ThemeManager::ThemeConfig& theme = 
        ThemeManager::getInstance().getCurrentTheme();

    // Create squares
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            QGraphicsRectItem* square = scene_->addRect(
                col * squareSize, row * squareSize,
                squareSize, squareSize);
            
            bool isLight = (row + col) % 2 == 0;
            square->setBrush(isLight ? Qt::white : QColor(128, 128, 128));
            square->setPen(Qt::NoPen);
            
            // Create highlight overlay
            highlightItems_[row][col] = scene_->addRect(
                col * squareSize, row * squareSize,
                squareSize, squareSize);
            highlightItems_[row][col]->setBrush(Qt::transparent);
            highlightItems_[row][col]->setPen(Qt::NoPen);
            highlightItems_[row][col]->setZValue(1);
        }
    }

    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            QGraphicsRectItem* square = scene_->addRect(
                col * squareSize, row * squareSize,
                squareSize, squareSize);
            
            bool isLight = (row + col) % 2 == 0;
            square->setBrush(isLight ? theme.colors.lightSquares 
                                   : theme.colors.darkSquares);
            square->setPen(QPen(theme.colors.border));
        }
    }
}

void ChessBoardView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    
    // Calculate square size based on view size (maintaining aspect ratio)
    qreal viewSize = qMin(event->size().width(), event->size().height());
    qreal squareSize = viewSize / 8;
    
    // Update scene rect and transform
    scene_->setSceneRect(0, 0, squareSize * 8, squareSize * 8);
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    
    // Update board squares and pieces
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            QGraphicsRectItem* square = 
                static_cast<QGraphicsRectItem*>(scene_->items()[row * 8 + col]);
            square->setRect(col * squareSize, row * squareSize, 
                          squareSize, squareSize);
            
            highlightItems_[row][col]->setRect(col * squareSize, row * squareSize,
                                             squareSize, squareSize);
        }
    }
    
    // Update piece positions and sizes
    for (auto piece : scene_->items()) {
        if (ChessPieceItem* chessPiece = dynamic_cast<ChessPieceItem*>(piece)) {
            chessPiece->updateSize(squareSize);
        }
    }
}

void ChessBoardView::mousePressEvent(QMouseEvent* event)
{
    QPoint boardPos = boardPositionAt(mapToScene(event->pos()));
    if (!game_->isValidPosition(boardPos)) {
        return;
    }
    
    auto piece = game_->getPieceAt(boardPos);
    if (piece && piece->getColor() == game_->getCurrentTurn()) {
        selectedPiece_ = dynamic_cast<ChessPieceItem*>(itemAt(event->pos()));
        if (selectedPiece_) {
            dragStartPos_ = selectedPiece_->pos();
            selectedPiece_->setZValue(2); // Bring to front
            highlightLegalMoves(boardPos);
        }
    }
}

void ChessBoardView::mouseMoveEvent(QMouseEvent* event)
{
    if (selectedPiece_) {
        QPointF newPos = mapToScene(event->pos()) - 
                        QPointF(selectedPiece_->boundingRect().width() / 2,
                               selectedPiece_->boundingRect().height() / 2);
        selectedPiece_->setPos(newPos);
    }
}

void ChessBoardView::mouseReleaseEvent(QMouseEvent* event)
{
    if (selectedPiece_) {
        QPoint fromPos = boardPositionAt(dragStartPos_);
        QPoint toPos = boardPositionAt(mapToScene(event->pos()));
        
        if (game_->isValidMove(fromPos, toPos)) {
            // Attempt to make the move
            if (game_->makeMove(fromPos, toPos)) {
                updateBoard();
                emit moveCompleted(generateMoveNotation(fromPos, toPos));
                
                // Check game state
                if (game_->isCheckmate(game_->getCurrentTurn())) {
                    emit gameOver(tr("Checkmate! %1 wins!")
                        .arg(game_->getCurrentTurn() == PieceColor::White ? 
                             "Black" : "White"));
                } else if (game_->isStalemate(game_->getCurrentTurn())) {
                    emit gameOver(tr("Stalemate! Game is drawn."));
                }
            }
        } else {
            // Invalid move, return piece to original position
            selectedPiece_->setPos(dragStartPos_);
        }
        
        selectedPiece_->setZValue(1);
        selectedPiece_ = nullptr;
        clearHighlights();
    }
}

void ChessBoardView::highlightLegalMoves(const QPoint& pos)
{
    clearHighlights();
    auto legalMoves = game_->getLegalMoves(pos);
    
    const ThemeManager::ThemeConfig& theme = 
    ThemeManager::getInstance().getCurrentTheme();

    for (const auto& move : legalMoves) {
        highlightItems_[move.y()][move.x()]->setBrush(
            theme.colors.highlightMove);
    }
}

void ChessBoardView::clearHighlights()
{
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            highlightItems_[row][col]->setBrush(Qt::transparent);
        }
    }
}

QPoint ChessBoardView::boardPositionAt(const QPointF& pos) const
{
    qreal squareSize = scene_->width() / 8;
    int col = static_cast<int>(pos.x() / squareSize);
    int row = static_cast<int>(pos.y() / squareSize);
    return QPoint(col, row);
}

void ChessBoardView::updateBoard()
{
    // Clear existing pieces
    for (auto item : scene_->items()) {
        if (dynamic_cast<ChessPieceItem*>(item)) {
            scene_->removeItem(item);
            delete item;
        }
    }
    
    // Add pieces from current game state
    qreal squareSize = scene_->width() / 8;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            auto piece = game_->getPieceAt(QPoint(col, row));
            if (piece) {
                auto pieceItem = new ChessPieceItem(piece);
                pieceItem->setPos(col * squareSize, row * squareSize);
                pieceItem->updateSize(squareSize);
                scene_->addItem(pieceItem);
            }
        }
    }
}

void ChessBoardView::updateTheme()
{
    setupBoard();
    updateBoard();
}