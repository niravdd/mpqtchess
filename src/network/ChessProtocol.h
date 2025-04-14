// src/network/ChessProtocol.h
#pragma once
#include "../QtCompat.h"
#include <QtCore/QDataStream>
#include "../core/ChessGame.h"
#include "../core/Position.h"
#include "../core/ChessPiece.h"

enum class MessageType {
    CONNECT_REQUEST,
    CONNECT_RESPONSE,
    GAME_STATE,
    MOVE,
    MOVE_RESPONSE,
    DRAW_OFFER,
    DRAW_RESPONSE,
    RESIGN,
    CHAT,
    ERROR,
    GAME_END,
    KEEPALIVE,
    PLAYER_READY,
    GAME_START
};

inline QString messageTypeToString(MessageType messageType)
{
    switch (messageType)
    {
        case MessageType::CONNECT_REQUEST: return "CONNECT_REQUEST";
        case MessageType::CONNECT_RESPONSE: return "CONNECT_RESPONSE";
        case MessageType::GAME_STATE: return "GAME_STATE";
        case MessageType::MOVE: return "MOVE";
        case MessageType::MOVE_RESPONSE: return "MOVE_RESPONSE";
        case MessageType::DRAW_OFFER: return "DRAW_OFFER";
        case MessageType::DRAW_RESPONSE: return "DRAW_RESPONSE";
        case MessageType::RESIGN: return "RESIGN";
        case MessageType::CHAT: return "CHAT";
        case MessageType::ERROR: return "ERROR";
        case MessageType::GAME_END: return "GAME_END";
        case MessageType::KEEPALIVE: return "KEEPALIVE";
        case MessageType::PLAYER_READY: return "PLAYER_READY";
        case MessageType::GAME_START: return "GAME_START";
        default: return "Unknown";
     }

     return "Unknown";
}

struct MoveData {
    Position from;
    Position to;
    PieceType promotionPiece;

    QString toString() const {
        return QString("from(%1,%2) to(%3,%4) promotionPiece(%5)")
                .arg(from.row).arg(from.col)
                .arg(to.row).arg(to.col)
                .arg(pieceTypeToString(promotionPiece));
    }
};

struct NetworkMessage {
    MessageType type;
    bool success{false};
    QString data;
    QString extraData;
    MoveData moveData;
};

/*
// Serialization operators
inline QDataStream& operator<<(QDataStream& stream, const Position& pos) {
    return stream << pos.row << pos.col;
}

inline QDataStream& operator>>(QDataStream& stream, Position& pos) {
    return stream >> pos.row >> pos.col;
}
*/

inline QDataStream& operator<<(QDataStream& stream, const MoveData& moveData) {
    return stream << moveData.from << moveData.to 
                 << static_cast<qint32>(moveData.promotionPiece);
}

inline QDataStream& operator>>(QDataStream& stream, MoveData& moveData) {
    qint32 piece;
    stream >> moveData.from >> moveData.to >> piece;
    moveData.promotionPiece = static_cast<PieceType>(piece);
    return stream;
}

inline QDataStream& operator<<(QDataStream& stream, const NetworkMessage& msg) {
    return stream << static_cast<qint32>(msg.type)
                 << msg.success
                 << msg.data
                 << msg.extraData
                 << msg.moveData;
}

inline QDataStream& operator>>(QDataStream& stream, NetworkMessage& msg) {
    qint32 type;
    stream >> type >> msg.success >> msg.data >> msg.extraData >> msg.moveData;
    msg.type = static_cast<MessageType>(type);
    return stream;
}