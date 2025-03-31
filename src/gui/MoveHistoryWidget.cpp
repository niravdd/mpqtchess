// src/gui/MoveHistoryWidget.cpp
#include "MoveHistoryWidget.h"

MoveHistoryWidget::MoveHistoryWidget(QWidget* parent)
    : QWidget(parent)
    , layout_(new QVBoxLayout(this))
    , historyText_(new QTextEdit(this))
    , moveNumber_(1)
    , isWhiteMove_(true)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
//  historyText_ = new QTextEdit(this);
    historyText_->setReadOnly(true);
    historyText_->setFont(QFont("Courier New", 12));
    
    layout->addWidget(historyText_);
}

void MoveHistoryWidget::clear()
{
    historyText_->clear();
    moveNumber_ = 1;
    isWhiteMove_ = true;
}

void MoveHistoryWidget::addMove(const QString& move)
{
    if (isWhiteMove_) {
        historyText_->append(QString("%1. %2")
            .arg(moveNumber_, 3)
            .arg(move.leftJustified(8)));
    } else {
        QTextCursor cursor = historyText_->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        cursor.insertText(QString("%1\n").arg(move));
        moveNumber_++;
    }
    
    isWhiteMove_ = !isWhiteMove_;
    
    // Scroll to bottom
    QScrollBar* sb = historyText_->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MoveHistoryWidget::loadFromGame(const ChessGame* game)
{
    clear();
    
    const auto& moveHistory = game->getMoveHistory();
    for (const auto& moveRecord : moveHistory) {
        // Format: {moveNumber, piece, from, to, isCapture, isCheck, isCheckmate}
        QString moveNotation;
        
        // Add piece symbol (except for pawns)
        if (moveRecord.piece != PieceType::Pawn) {
            moveNotation += getPieceSymbol(moveRecord.piece);
        }
        
        // Add capture symbol
        if (moveRecord.isCapture) {
            if (moveRecord.piece == PieceType::Pawn) {
                moveNotation += moveRecord.fromSquare.at(0); // File of capturing pawn
            }
            moveNotation += "x";
        }
        
        // Add destination square
        moveNotation += moveRecord.toSquare;
        
        // Add castling notation
        if (moveRecord.piece == PieceType::King) {
            if (moveRecord.fromSquare == "e1" && moveRecord.toSquare == "g1") {
                moveNotation = "O-O";
            } else if (moveRecord.fromSquare == "e1" && moveRecord.toSquare == "c1") {
                moveNotation = "O-O-O";
            } else if (moveRecord.fromSquare == "e8" && moveRecord.toSquare == "g8") {
                moveNotation = "O-O";
            } else if (moveRecord.fromSquare == "e8" && moveRecord.toSquare == "c8") {
                moveNotation = "O-O-O";
            }
        }
        
        // Add promotion
        if (moveRecord.promotionPiece != PieceType::None) {
            moveNotation += "=";
            moveNotation += getPieceSymbol(moveRecord.promotionPiece);
        }
        
        // Add check/checkmate symbols
        if (moveRecord.isCheckmate) {
            moveNotation += "#";
        } else if (moveRecord.isCheck) {
            moveNotation += "+";
        }
        
        addMove(moveNotation);
    }
}

QString MoveHistoryWidget::getPieceSymbol(PieceType piece)
{
    switch (piece) {
        case PieceType::King:   return "K";
        case PieceType::Queen:  return "Q";
        case PieceType::Rook:   return "R";
        case PieceType::Bishop: return "B";
        case PieceType::Knight: return "N";
        case PieceType::Pawn:   return "";
        default:                return "?";
    }
}

MoveHistoryWidget::~MoveHistoryWidget()
{
    // Qt will automatically delete child widgets (historyText_)
    // since they have this widget as their parent
    
    // If you've created any objects that aren't parented to this widget
    // or aren't Qt widgets (raw pointers allocated with new), delete them here
    
    // For example, if you have any custom data structures:
    // delete customData_;
    
    // Clean up layout if it wasn't parented properly
    // Note: Usually Qt handles this automatically if layout_ was set as the widget's layout
    if (layout_ && !layout_->parent()) {
        delete layout_;
    }
    
    // Note: Usually most of this cleanup is unnecessary as Qt's parent-child
    // relationship ensures proper cleanup of child widgets
}