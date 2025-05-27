// MPChessClient.cpp

#include "MPChessClient.h"
#include "ui_MPChessClient.h"

#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include <QColorDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QClipboard>

// Logger implementation
Logger::Logger(QObject* parent) : QObject(parent), logLevel(LogLevel::INFO), logToFile(false) {
    // Default log level is INFO
}

Logger::~Logger() {
    if (logFile.isOpen()) {
        logFile.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() const {
    return logLevel;
}

void Logger::debug(const QString& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const QString& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const QString& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const QString& message) {
    log(LogLevel::ERROR, message);
}

void Logger::setLogToFile(bool enabled, const QString& filePath) {
    QMutexLocker locker(&mutex);
    
    if (logFile.isOpen()) {
        logFile.close();
    }
    
    logToFile = enabled;
    
    if (enabled) {
        if (filePath.isEmpty()) {
            // Use default log file path
            QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir dir(defaultPath);
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            logFilePath = defaultPath + "/chess_client.log";
        } else {
            logFilePath = filePath;
        }
        
        logFile.setFileName(logFilePath);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Failed to open log file:" << logFilePath;
            logToFile = false;
        }
    }
}

bool Logger::isLoggingToFile() const {
    return logToFile;
}

QString Logger::getLogFilePath() const {
    return logFilePath;
}

void Logger::log(LogLevel level, const QString& message) {
    if (level < logLevel) {
        return;
    }
    
    QString formattedMessage = QString("%1 [%2] %3")
        .arg(getCurrentTimestamp())
        .arg(levelToString(level))
        .arg(message);
    
    QMutexLocker locker(&mutex);
    
    // Output to console
    if (level == LogLevel::ERROR) {
        qCritical() << formattedMessage;
    } else if (level == LogLevel::WARNING) {
        qWarning() << formattedMessage;
    } else {
        qDebug() << formattedMessage;
    }
    
    // Write to log file if enabled
    if (logToFile && logFile.isOpen()) {
        QTextStream stream(&logFile);
        stream << formattedMessage << Qt::endl;
        logFile.flush();
    }
    
    // Emit signal for UI components
    emit logMessage(level, formattedMessage);
}

QString Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

QString Logger::getCurrentTimestamp() const {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

// NetworkManager implementation
NetworkManager::NetworkManager(Logger* logger, QObject* parent)
    : QObject(parent), logger(logger), socket(nullptr), pingTimer(nullptr) {
    
    // Create socket
    socket = new QTcpSocket(this);
    
    // Connect socket signals
    connect(socket, &QTcpSocket::connected, this, &NetworkManager::onConnected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &NetworkManager::onError);
    
    // Create ping timer
    pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, &NetworkManager::onPingTimer);
}

NetworkManager::~NetworkManager() {
    if (socket) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
        }
        socket->deleteLater();
    }
    
    if (pingTimer) {
        pingTimer->stop();
        pingTimer->deleteLater();
    }
}

bool NetworkManager::connectToServer(const QString& host, int port) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        logger->warning("Already connected to server");
        return true;
    }
    
    logger->info(QString("Connecting to server at %1:%2").arg(host).arg(port));
    socket->connectToHost(host, port);
    
    // Wait for connection
    if (!socket->waitForConnected(5000)) {
        logger->error(QString("Failed to connect to server: %1").arg(socket->errorString()));
        return false;
    }
    
    return true;
}

void NetworkManager::disconnectFromServer() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        logger->warning("Not connected to server");
        return;
    }
    
    logger->info("Disconnecting from server");
    socket->disconnectFromHost();
    
    if (pingTimer->isActive()) {
        pingTimer->stop();
    }
}

bool NetworkManager::isConnected() const {
    return socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::authenticate(const QString& username, const QString& password, bool isRegistration) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::AUTHENTICATION);
    message["username"] = username;
    message["password"] = password;
    message["register"] = isRegistration;
    
    sendMessage(message);
    logger->info(QString("%1 attempt for user: %2")
                .arg(isRegistration ? "Registration" : "Authentication")
                .arg(username));
}

void NetworkManager::sendMove(const QString& gameId, const ChessMove& move) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::MOVE);
    message["gameId"] = gameId;
    message["move"] = move.toAlgebraic();
    
    sendMessage(message);
    logger->info(QString("Sending move: %1 for game: %2")
                .arg(move.toAlgebraic())
                .arg(gameId));
}

void NetworkManager::requestMatchmaking(bool join, TimeControlType timeControl) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::MATCHMAKING_REQUEST);
    message["join"] = join;
    
    if (join) {
        QString timeControlStr;
        switch (timeControl) {
            case TimeControlType::RAPID:     timeControlStr = "rapid"; break;
            case TimeControlType::BLITZ:     timeControlStr = "blitz"; break;
            case TimeControlType::BULLET:    timeControlStr = "bullet"; break;
            case TimeControlType::CLASSICAL: timeControlStr = "classical"; break;
            case TimeControlType::CASUAL:    timeControlStr = "casual"; break;
        }
        message["timeControl"] = timeControlStr;
    }
    
    sendMessage(message);
    logger->info(QString("%1 matchmaking queue")
                .arg(join ? "Joining" : "Leaving"));
}

void NetworkManager::requestGameHistory() {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::GAME_HISTORY_REQUEST);
    
    sendMessage(message);
    logger->info("Requesting game history");
}

void NetworkManager::requestGameAnalysis(const QString& gameId) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::GAME_ANALYSIS_REQUEST);
    message["gameId"] = gameId;
    message["includeAnalysis"] = true;
    
    sendMessage(message);
    logger->info(QString("Requesting analysis for game: %1").arg(gameId));
}

void NetworkManager::sendResignation(const QString& gameId) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::RESIGN);
    message["gameId"] = gameId;
    
    sendMessage(message);
    logger->info(QString("Sending resignation for game: %1").arg(gameId));
}

void NetworkManager::sendDrawOffer(const QString& gameId) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::DRAW_OFFER);
    message["gameId"] = gameId;
    
    sendMessage(message);
    logger->info(QString("Sending draw offer for game: %1").arg(gameId));
}

void NetworkManager::sendDrawResponse(const QString& gameId, bool accepted) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::DRAW_RESPONSE);
    message["gameId"] = gameId;
    message["accepted"] = accepted;
    
    sendMessage(message);
    logger->info(QString("%1 draw offer for game: %2")
                .arg(accepted ? "Accepting" : "Declining")
                .arg(gameId));
}

void NetworkManager::requestLeaderboard(bool allPlayers, int count) {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::LEADERBOARD_REQUEST);
    message["all"] = allPlayers;
    message["count"] = count;
    
    sendMessage(message);
    logger->info(QString("Requesting leaderboard (%1)")
                .arg(allPlayers ? "all players" : QString("top %1").arg(count)));
}

void NetworkManager::sendPing() {
    QJsonObject message;
    message["type"] = static_cast<int>(MessageType::PING);
    
    sendMessage(message);
    logger->debug("Sending ping");
}

void NetworkManager::onConnected() {
    logger->info("Connected to server");
    
    // Start ping timer
    pingTimer->start(30000); // Send ping every 30 seconds
    
    emit connected();
}

void NetworkManager::onDisconnected() {
    logger->info("Disconnected from server");
    
    // Stop ping timer
    if (pingTimer->isActive()) {
        pingTimer->stop();
    }
    
    emit disconnected();
}

void NetworkManager::onError(QAbstractSocket::SocketError socketError) {
    QString errorMessage = socket->errorString();
    logger->error(QString("Socket error: %1").arg(errorMessage));
    
    emit connectionError(errorMessage);
}

void NetworkManager::onReadyRead() {
    // Read all available data
    buffer.append(socket->readAll());
    
    // Process complete JSON messages
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(buffer, &parseError);
    
    if (parseError.error == QJsonParseError::NoError) {
        // Successfully parsed a complete JSON message
        if (doc.isObject()) {
            processMessage(doc.object());
        } else {
            logger->warning("Received JSON is not an object");
        }
        
        // Clear the buffer
        buffer.clear();
    } else if (parseError.error != QJsonParseError::InsufficientData) {
        // If the error is not just incomplete data, log it
        logger->warning(QString("JSON parse error: %1").arg(parseError.errorString()));
        buffer.clear();
    }
    // If the error is InsufficientData, keep the buffer for more data
}

void NetworkManager::onPingTimer() {
    if (isConnected()) {
        sendPing();
    }
}

void NetworkManager::sendMessage(const QJsonObject& message) {
    if (!isConnected()) {
        logger->warning("Cannot send message: not connected to server");
        return;
    }
    
    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    socket->write(data);
    socket->flush();
}

void NetworkManager::processMessage(const QJsonObject& message) {
    if (!message.contains("type")) {
        logger->warning("Received message without type field");
        return;
    }
    
    MessageType type = static_cast<MessageType>(message["type"].toInt());
    
    switch (type) {
        case MessageType::AUTHENTICATION_RESULT:
            processAuthenticationResult(message);
            break;
            
        case MessageType::GAME_START:
            processGameStart(message);
            break;
            
        case MessageType::GAME_STATE:
            processGameState(message);
            break;
            
        case MessageType::MOVE_RESULT:
            processMoveResult(message);
            break;
            
        case MessageType::GAME_OVER:
            processGameOver(message);
            break;
            
        case MessageType::MOVE_RECOMMENDATIONS:
            processMoveRecommendations(message);
            break;
            
        case MessageType::MATCHMAKING_STATUS:
            processMatchmakingStatus(message);
            break;
            
        case MessageType::GAME_HISTORY_RESPONSE:
            processGameHistoryResponse(message);
            break;
            
        case MessageType::GAME_ANALYSIS_RESPONSE:
            processGameAnalysisResponse(message);
            break;
            
        case MessageType::LEADERBOARD_RESPONSE:
            processLeaderboardResponse(message);
            break;
            
        case MessageType::ERROR:
            processError(message);
            break;
            
        case MessageType::CHAT:
            processChat(message);
            break;
            
        case MessageType::DRAW_OFFER:
            processDrawOffer(message);
            break;
            
        case MessageType::DRAW_RESPONSE:
            processDrawResponse(message);
            break;
            
        case MessageType::PONG:
            logger->debug("Received pong");
            break;
            
        default:
            logger->warning(QString("Unknown message type: %1").arg(static_cast<int>(type)));
            break;
    }
}

void NetworkManager::processAuthenticationResult(const QJsonObject& data) {
    bool success = data["success"].toBool();
    QString message = data["message"].toString();
    
    logger->info(QString("Authentication result: %1 - %2")
                .arg(success ? "Success" : "Failure")
                .arg(message));
    
    emit authenticationResult(success, message);
}

void NetworkManager::processGameStart(const QJsonObject& data) {
    QString gameId = data["gameId"].toString();
    QString whitePlayer = data["whitePlayer"].toString();
    QString blackPlayer = data["blackPlayer"].toString();
    
    logger->info(QString("Game started: %1, White: %2, Black: %3")
                .arg(gameId)
                .arg(whitePlayer)
                .arg(blackPlayer));
    
    emit gameStarted(data);
}

void NetworkManager::processGameState(const QJsonObject& data) {
    QJsonObject gameState = data["gameState"].toObject();
    QString gameId = gameState["gameId"].toString();
    
    logger->debug(QString("Received game state update for game: %1").arg(gameId));
    
    emit gameStateUpdated(gameState);
}

void NetworkManager::processMoveResult(const QJsonObject& data) {
    bool success = data["success"].toBool();
    QString message = data["message"].toString();
    
    logger->info(QString("Move result: %1 - %2")
                .arg(success ? "Success" : "Failure")
                .arg(message));
    
    emit moveResult(success, message);
}

void NetworkManager::processGameOver(const QJsonObject& data) {
    QString result = data["result"].toString();
    QString reason = data.contains("reason") ? data["reason"].toString() : "";
    
    logger->info(QString("Game over: %1%2")
                .arg(result)
                .arg(reason.isEmpty() ? "" : QString(" (%1)").arg(reason)));
    
    emit gameOver(data);
}

void NetworkManager::processMoveRecommendations(const QJsonObject& data) {
    QJsonArray recommendations = data["recommendations"].toArray();
    
    logger->debug(QString("Received %1 move recommendations").arg(recommendations.size()));
    
    emit moveRecommendationsReceived(recommendations);
}

void NetworkManager::processMatchmakingStatus(const QJsonObject& data) {
    QString status = data["status"].toString();
    
    logger->info(QString("Matchmaking status: %1").arg(status));
    
    emit matchmakingStatus(data);
}

void NetworkManager::processGameHistoryResponse(const QJsonObject& data) {
    bool success = data["success"].toBool();
    
    if (success) {
        QJsonArray gameHistories = data["gameHistories"].toArray();
        logger->info(QString("Received game history: %1 games").arg(gameHistories.size()));
        emit gameHistoryReceived(gameHistories);
    } else {
        QString message = data["message"].toString();
        logger->warning(QString("Game history request failed: %1").arg(message));
        emit errorReceived(message);
    }
}

void NetworkManager::processGameAnalysisResponse(const QJsonObject& data) {
    bool success = data["success"].toBool();
    
    if (success) {
        QJsonObject analysis = data["analysis"].toObject();
        logger->info("Received game analysis");
        emit gameAnalysisReceived(analysis);
    } else {
        QString message = data["message"].toString();
        logger->warning(QString("Game analysis request failed: %1").arg(message));
        emit errorReceived(message);
    }
}

void NetworkManager::processLeaderboardResponse(const QJsonObject& data) {
    QJsonObject leaderboard = data["leaderboard"].toObject();
    
    logger->info("Received leaderboard data");
    
    emit leaderboardReceived(leaderboard);
}

void NetworkManager::processError(const QJsonObject& data) {
    QString message = data["message"].toString();
    
    logger->error(QString("Server error: %1").arg(message));
    
    emit errorReceived(message);
}

void NetworkManager::processChat(const QJsonObject& data) {
    QString sender = data["sender"].toString();
    QString message = data["message"].toString();
    
    logger->info(QString("Chat from %1: %2").arg(sender).arg(message));
    
    emit chatMessageReceived(sender, message);
}

void NetworkManager::processDrawOffer(const QJsonObject& data) {
    QString offeredBy = data["offeredBy"].toString();
    
    logger->info(QString("Draw offered by: %1").arg(offeredBy));
    
    emit drawOfferReceived(offeredBy);
}

void NetworkManager::processDrawResponse(const QJsonObject& data) {
    bool accepted = data["accepted"].toBool();
    
    logger->info(QString("Draw %1").arg(accepted ? "accepted" : "declined"));
    
    emit drawResponseReceived(accepted);
}

// AudioManager implementation
AudioManager::AudioManager(QObject* parent)
    : QObject(parent), soundEffectsEnabled(true), backgroundMusicEnabled(true),
      soundEffectVolume(50), backgroundMusicVolume(30) {
    
    // Initialize music player
    musicPlayer = new QMediaPlayer(this);
    musicOutput = new QAudioOutput(this);
    musicPlayer->setAudioOutput(musicOutput);
    
    // Set the source for background music
    musicPlayer->setSource(QUrl("qrc:/sounds/background_music.mp3"));
    musicPlayer->setLoops(QMediaPlayer::Infinite);
    
    // Set volumes
    musicOutput->setVolume(backgroundMusicVolume / 100.0);
    
    // Load sound effects
    loadSoundEffects();
}

AudioManager::~AudioManager() {
    if (musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
        musicPlayer->stop();
    }
    
    delete musicPlayer;
    delete musicOutput;
}

void AudioManager::playSoundEffect(SoundEffect effect) {
    if (!soundEffectsEnabled) {
        return;
    }
    
    QString path = soundEffectPaths.value(effect);
    if (path.isEmpty()) {
        return;
    }
    
    QMediaPlayer* player = new QMediaPlayer(this);
    QAudioOutput* output = new QAudioOutput(this);
    player->setAudioOutput(output);
    
    output->setVolume(soundEffectVolume / 100.0);
    player->setSource(QUrl(path));
    
    // Connect to the media status changed signal to clean up the player when done
    connect(player, &QMediaPlayer::playbackStateChanged, this, [player, output](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::StoppedState) {
            player->deleteLater();
            output->deleteLater();
        }
    });
    
    player->play();
}

void AudioManager::playBackgroundMusic(bool play) {
    if (!backgroundMusicEnabled) {
        return;
    }
    
    if (play) {
        if (musicPlayer->playbackState() != QMediaPlayer::PlayingState) {
            musicPlayer->play();
        }
    } else {
        if (musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
            musicPlayer->pause();
        }
    }
}

void AudioManager::setSoundEffectsEnabled(bool enabled) {
    soundEffectsEnabled = enabled;
}

bool AudioManager::areSoundEffectsEnabled() const {
    return soundEffectsEnabled;
}

void AudioManager::setBackgroundMusicEnabled(bool enabled) {
    backgroundMusicEnabled = enabled;
    
    if (!enabled && musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
        musicPlayer->pause();
    } else if (enabled && musicPlayer->playbackState() != QMediaPlayer::PlayingState) {
        musicPlayer->play();
    }
}

bool AudioManager::isBackgroundMusicEnabled() const {
    return backgroundMusicEnabled;
}

void AudioManager::setSoundEffectVolume(int volume) {
    soundEffectVolume = qBound(0, volume, 100);
}

int AudioManager::getSoundEffectVolume() const {
    return soundEffectVolume;
}

void AudioManager::setBackgroundMusicVolume(int volume) {
    backgroundMusicVolume = qBound(0, volume, 100);
    musicOutput->setVolume(backgroundMusicVolume / 100.0);
}

int AudioManager::getBackgroundMusicVolume() const {
    return backgroundMusicVolume;
}

void AudioManager::loadSoundEffects() {
    // Map sound effects to resource paths
    soundEffectPaths[SoundEffect::MOVE] = "qrc:/sounds/move.wav";
    soundEffectPaths[SoundEffect::CAPTURE] = "qrc:/sounds/capture.wav";
    soundEffectPaths[SoundEffect::CHECK] = "qrc:/sounds/check.wav";
    soundEffectPaths[SoundEffect::CHECKMATE] = "qrc:/sounds/checkmate.wav";
    soundEffectPaths[SoundEffect::CASTLE] = "qrc:/sounds/castle.wav";
    soundEffectPaths[SoundEffect::PROMOTION] = "qrc:/sounds/promotion.wav";
    soundEffectPaths[SoundEffect::GAME_START] = "qrc:/sounds/game_start.wav";
    soundEffectPaths[SoundEffect::GAME_END] = "qrc:/sounds/game_end.wav";
    soundEffectPaths[SoundEffect::ERROR] = "qrc:/sounds/error.wav";
    soundEffectPaths[SoundEffect::NOTIFICATION] = "qrc:/sounds/notification.wav";
}

// ThemeManager implementation
ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent), theme(Theme::LIGHT), boardTheme(BoardTheme::CLASSIC), pieceTheme(PieceTheme::CLASSIC) {
    
    // Set default custom colors
    customLightSquareColor = QColor(240, 217, 181);
    customDarkSquareColor = QColor(181, 136, 99);
    customHighlightColor = QColor(124, 192, 203, 180);
    customLastMoveHighlightColor = QColor(205, 210, 106, 180);
    customCheckHighlightColor = QColor(231, 76, 60, 180);
    
    // Load settings
    loadThemeSettings();
}

ThemeManager::~ThemeManager() {
    // Save settings
    saveThemeSettings();
}

void ThemeManager::setTheme(Theme newTheme) {
    if (theme != newTheme) {
        theme = newTheme;
        emit themeChanged();
    }
}

ThemeManager::Theme ThemeManager::getTheme() const {
    return theme;
}

void ThemeManager::setBoardTheme(BoardTheme newTheme) {
    if (boardTheme != newTheme) {
        boardTheme = newTheme;
        emit boardThemeChanged();
    }
}

ThemeManager::BoardTheme ThemeManager::getBoardTheme() const {
    return boardTheme;
}

void ThemeManager::setPieceTheme(PieceTheme newTheme) {
    if (pieceTheme != newTheme) {
        pieceTheme = newTheme;
        emit pieceThemeChanged();
    }
}

ThemeManager::PieceTheme ThemeManager::getPieceTheme() const {
    return pieceTheme;
}

QColor ThemeManager::getLightSquareColor() const {
    if (boardTheme == BoardTheme::CUSTOM) {
        return customLightSquareColor;
    }
    return getLightSquareColorForTheme(boardTheme);
}

QColor ThemeManager::getDarkSquareColor() const {
    if (boardTheme == BoardTheme::CUSTOM) {
        return customDarkSquareColor;
    }
    return getDarkSquareColorForTheme(boardTheme);
}

QColor ThemeManager::getHighlightColor() const {
    return customHighlightColor;
}

QColor ThemeManager::getLastMoveHighlightColor() const {
    return customLastMoveHighlightColor;
}

QColor ThemeManager::getCheckHighlightColor() const {
    return customCheckHighlightColor;
}

QString ThemeManager::getPieceThemePath() const {
    if (pieceTheme == PieceTheme::CUSTOM) {
        return customPieceThemePath;
    }
    return getPieceThemePathForTheme(pieceTheme);
}

void ThemeManager::setCustomLightSquareColor(const QColor& color) {
    customLightSquareColor = color;
    if (boardTheme == BoardTheme::CUSTOM) {
        emit boardThemeChanged();
    }
}

void ThemeManager::setCustomDarkSquareColor(const QColor& color) {
    customDarkSquareColor = color;
    if (boardTheme == BoardTheme::CUSTOM) {
        emit boardThemeChanged();
    }
}

void ThemeManager::setCustomHighlightColor(const QColor& color) {
    customHighlightColor = color;
    emit boardThemeChanged();
}

void ThemeManager::setCustomLastMoveHighlightColor(const QColor& color) {
    customLastMoveHighlightColor = color;
    emit boardThemeChanged();
}

void ThemeManager::setCustomCheckHighlightColor(const QColor& color) {
    customCheckHighlightColor = color;
    emit boardThemeChanged();
}

void ThemeManager::setCustomPieceThemePath(const QString& path) {
    customPieceThemePath = path;
    if (pieceTheme == PieceTheme::CUSTOM) {
        emit pieceThemeChanged();
    }
}

QColor ThemeManager::getTextColor() const {
    switch (theme) {
        case Theme::LIGHT:
            return QColor(51, 51, 51);
        case Theme::DARK:
            return QColor(240, 240, 240);
        case Theme::CUSTOM:
            return QColor(51, 51, 51); // Default to light theme text color
    }
    return QColor(51, 51, 51);
}

QColor ThemeManager::getBackgroundColor() const {
    switch (theme) {
        case Theme::LIGHT:
            return QColor(245, 245, 245);
        case Theme::DARK:
            return QColor(45, 45, 45);
        case Theme::CUSTOM:
            return QColor(245, 245, 245); // Default to light theme background
    }
    return QColor(245, 245, 245);
}

QColor ThemeManager::getPrimaryColor() const {
    switch (theme) {
        case Theme::LIGHT:
            return QColor(66, 139, 202);
        case Theme::DARK:
            return QColor(66, 139, 202);
        case Theme::CUSTOM:
            return QColor(66, 139, 202); // Default primary color
    }
    return QColor(66, 139, 202);
}

QColor ThemeManager::getSecondaryColor() const {
    switch (theme) {
        case Theme::LIGHT:
            return QColor(92, 184, 92);
        case Theme::DARK:
            return QColor(92, 184, 92);
        case Theme::CUSTOM:
            return QColor(92, 184, 92); // Default secondary color
    }
    return QColor(92, 184, 92);
}

QColor ThemeManager::getAccentColor() const {
    switch (theme) {
        case Theme::LIGHT:
            return QColor(240, 173, 78);
        case Theme::DARK:
            return QColor(240, 173, 78);
        case Theme::CUSTOM:
            return QColor(240, 173, 78); // Default accent color
    }
    return QColor(240, 173, 78);
}

QString ThemeManager::getStyleSheet() const {
    QColor textColor = getTextColor();
    QColor bgColor = getBackgroundColor();
    QColor primaryColor = getPrimaryColor();
    
    QString style = QString(
        "QWidget { "
        "    color: %1; "
        "    background-color: %2; "
        "} "
        "QPushButton { "
        "    background-color: %3; "
        "    color: white; "
        "    border: none; "
        "    padding: 5px 10px; "
        "    border-radius: 3px; "
        "} "
        "QPushButton:hover { "
        "    background-color: %4; "
        "} "
        "QPushButton:pressed { "
        "    background-color: %5; "
        "} "
        "QLineEdit, QComboBox, QSpinBox { "
        "    border: 1px solid %6; "
        "    border-radius: 3px; "
        "    padding: 3px; "
        "    background-color: %7; "
        "} "
        "QTabWidget::pane { "
        "    border: 1px solid %6; "
        "} "
        "QTabBar::tab { "
        "    background-color: %8; "
        "    color: %1; "
        "    padding: 5px 10px; "
        "    border: 1px solid %6; "
        "    border-bottom: none; "
        "    border-top-left-radius: 3px; "
        "    border-top-right-radius: 3px; "
        "} "
        "QTabBar::tab:selected { "
        "    background-color: %3; "
        "    color: white; "
        "} "
        "QTableWidget { "
        "    border: 1px solid %6; "
        "    gridline-color: %6; "
        "} "
        "QHeaderView::section { "
        "    background-color: %8; "
        "    color: %1; "
        "    padding: 5px; "
        "    border: 1px solid %6; "
        "} "
        "QScrollBar:vertical { "
        "    border: none; "
        "    background-color: %8; "
        "    width: 10px; "
        "    margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "    background-color: %9; "
        "    min-height: 20px; "
        "    border-radius: 5px; "
        "} "
        "QScrollBar:horizontal { "
        "    border: none; "
        "    background-color: %8; "
        "    height: 10px; "
        "    margin: 0px; "
        "} "
        "QScrollBar::handle:horizontal { "
        "    background-color: %9; "
        "    min-width: 20px; "
        "    border-radius: 5px; "
        "} "
    ).arg(
        textColor.name(),
        bgColor.name(),
        primaryColor.name(),
        primaryColor.lighter(110).name(),
        primaryColor.darker(110).name(),
        theme == Theme::DARK ? "#555555" : "#cccccc",
        theme == Theme::DARK ? "#333333" : "#ffffff",
        theme == Theme::DARK ? "#333333" : "#f0f0f0",
        theme == Theme::DARK ? "#666666" : "#c0c0c0"
    );
    
    return style;
}

void ThemeManager::loadThemeSettings() {
    QSettings settings;
    
    // Load theme settings
    int themeValue = settings.value("theme/mainTheme", static_cast<int>(Theme::LIGHT)).toInt();
    theme = static_cast<Theme>(themeValue);
    
    int boardThemeValue = settings.value("theme/boardTheme", static_cast<int>(BoardTheme::CLASSIC)).toInt();
    boardTheme = static_cast<BoardTheme>(boardThemeValue);
    
    int pieceThemeValue = settings.value("theme/pieceTheme", static_cast<int>(PieceTheme::CLASSIC)).toInt();
    pieceTheme = static_cast<PieceTheme>(pieceThemeValue);
    
    // Load custom colors
    if (settings.contains("theme/customLightSquare")) {
        customLightSquareColor = settings.value("theme/customLightSquare").value<QColor>();
    }
    
    if (settings.contains("theme/customDarkSquare")) {
        customDarkSquareColor = settings.value("theme/customDarkSquare").value<QColor>();
    }
    
    if (settings.contains("theme/customHighlight")) {
        customHighlightColor = settings.value("theme/customHighlight").value<QColor>();
    }
    
    if (settings.contains("theme/customLastMoveHighlight")) {
        customLastMoveHighlightColor = settings.value("theme/customLastMoveHighlight").value<QColor>();
    }
    
    if (settings.contains("theme/customCheckHighlight")) {
        customCheckHighlightColor = settings.value("theme/customCheckHighlight").value<QColor>();
    }
    
    // Load custom piece theme path
    customPieceThemePath = settings.value("theme/customPieceThemePath", "").toString();
}

void ThemeManager::saveThemeSettings() {
    QSettings settings;
    
    // Save theme settings
    settings.setValue("theme/mainTheme", static_cast<int>(theme));
    settings.setValue("theme/boardTheme", static_cast<int>(boardTheme));
    settings.setValue("theme/pieceTheme", static_cast<int>(pieceTheme));
    
    // Save custom colors
    settings.setValue("theme/customLightSquare", customLightSquareColor);
    settings.setValue("theme/customDarkSquare", customDarkSquareColor);
    settings.setValue("theme/customHighlight", customHighlightColor);
    settings.setValue("theme/customLastMoveHighlight", customLastMoveHighlightColor);
    settings.setValue("theme/customCheckHighlight", customCheckHighlightColor);
    
    // Save custom piece theme path
    settings.setValue("theme/customPieceThemePath", customPieceThemePath);
}

QColor ThemeManager::getLightSquareColorForTheme(BoardTheme theme) const {
    switch (theme) {
        case BoardTheme::CLASSIC:
            return QColor(240, 217, 181);
        case BoardTheme::WOOD:
            return QColor(222, 184, 135);
        case BoardTheme::MARBLE:
            return QColor(230, 230, 230);
        case BoardTheme::BLUE:
            return QColor(187, 222, 251);
        case BoardTheme::GREEN:
            return QColor(200, 230, 201);
        case BoardTheme::CUSTOM:
            return customLightSquareColor;
    }
    return QColor(240, 217, 181);
}

QColor ThemeManager::getDarkSquareColorForTheme(BoardTheme theme) const {
    switch (theme) {
        case BoardTheme::CLASSIC:
            return QColor(181, 136, 99);
        case BoardTheme::WOOD:
            return QColor(160, 82, 45);
        case BoardTheme::MARBLE:
            return QColor(170, 170, 170);
        case BoardTheme::BLUE:
            return QColor(63, 81, 181);
        case BoardTheme::GREEN:
            return QColor(76, 175, 80);
        case BoardTheme::CUSTOM:
            return customDarkSquareColor;
    }
    return QColor(181, 136, 99);
}

QString ThemeManager::getPieceThemePathForTheme(PieceTheme theme) const {
    switch (theme) {
        case PieceTheme::CLASSIC:
            return "classic";
        case PieceTheme::MODERN:
            return "modern";
        case PieceTheme::SIMPLE:
            return "simple";
        case PieceTheme::FANCY:
            return "fancy";
        case PieceTheme::CUSTOM:
            return customPieceThemePath;
    }
    return "classic";
}

// ChessPieceItem implementation
ChessPieceItem::ChessPieceItem(PieceType type, PieceColor color, ThemeManager* themeManager, int squareSize)
    : type(type), color(color), themeManager(themeManager), squareSize(squareSize) {
    
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemIsSelectable);
    
    // Load the SVG for this piece
    loadSvg();
    
    // Set Z value to ensure pieces are drawn above squares
    setZValue(1);
}

ChessPieceItem::~ChessPieceItem() {
}

QRectF ChessPieceItem::boundingRect() const {
    return QRectF(0, 0, squareSize, squareSize);
}

void ChessPieceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // Draw the piece
    renderer.render(painter, boundingRect());
}

PieceType ChessPieceItem::getType() const {
    return type;
}

PieceColor ChessPieceItem::getColor() const {
    return color;
}

void ChessPieceItem::setSquareSize(int size) {
    prepareGeometryChange();
    squareSize = size;
    update();
}

int ChessPieceItem::getSquareSize() const {
    return squareSize;
}

void ChessPieceItem::updateTheme() {
    loadSvg();
    update();
}

void ChessPieceItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    dragStartPosition = pos();
    QGraphicsItem::mousePressEvent(event);
}

void ChessPieceItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // Bring to front while dragging
    setZValue(2);
    QGraphicsItem::mouseMoveEvent(event);
}

void ChessPieceItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    // Reset Z value
    setZValue(1);
    QGraphicsItem::mouseReleaseEvent(event);
}

void ChessPieceItem::loadSvg() {
    // Get the SVG file name for this piece
    QString svgFileName = ChessPiece(type, color).getSvgFileName(themeManager->getPieceThemePath());
    
    // Load the SVG
    renderer.load(svgFileName);
}

// ChessBoardWidget implementation
ChessBoardWidget::ChessBoardWidget(ThemeManager* themeManager, AudioManager* audioManager, QWidget* parent)
    : QGraphicsView(parent), themeManager(themeManager), audioManager(audioManager),
      squareSize(60), flipped(false), playerColor(PieceColor::WHITE), interactive(true) {
    
    // Create scene
    scene = new QGraphicsScene(this);
    setScene(scene);
    
    // Set up view properties
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Initialize pieces array
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            pieces[r][c] = nullptr;
        }
    }
    
    // Set up the board
    setupBoard();
    
    // Connect theme change signals
    connect(themeManager, &ThemeManager::boardThemeChanged, this, &ChessBoardWidget::updateTheme);
    connect(themeManager, &ThemeManager::pieceThemeChanged, this, &ChessBoardWidget::updateTheme);
}

ChessBoardWidget::~ChessBoardWidget() {
    // Scene will delete all items
    delete scene;
}

void ChessBoardWidget::resetBoard() {
    // Clear the scene
    scene->clear();
    
    // Reset pieces array
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            pieces[r][c] = nullptr;
        }
    }
    
    // Clear highlight items
    highlightItems.clear();
    hintItems.clear();
    
    // Set up the board again
    setupBoard();
}

void ChessBoardWidget::setupInitialPosition() {
    // Set up white pieces
    setPiece(Position(0, 0), PieceType::ROOK, PieceColor::WHITE);
    setPiece(Position(0, 1), PieceType::KNIGHT, PieceColor::WHITE);
    setPiece(Position(0, 2), PieceType::BISHOP, PieceColor::WHITE);
    setPiece(Position(0, 3), PieceType::QUEEN, PieceColor::WHITE);
    setPiece(Position(0, 4), PieceType::KING, PieceColor::WHITE);
    setPiece(Position(0, 5), PieceType::BISHOP, PieceColor::WHITE);
    setPiece(Position(0, 6), PieceType::KNIGHT, PieceColor::WHITE);
    setPiece(Position(0, 7), PieceType::ROOK, PieceColor::WHITE);
    
    for (int c = 0; c < 8; ++c) {
        setPiece(Position(1, c), PieceType::PAWN, PieceColor::WHITE);
    }
    
    // Set up black pieces
    setPiece(Position(7, 0), PieceType::ROOK, PieceColor::BLACK);
    setPiece(Position(7, 1), PieceType::KNIGHT, PieceColor::BLACK);
    setPiece(Position(7, 2), PieceType::BISHOP, PieceColor::BLACK);
    setPiece(Position(7, 3), PieceType::QUEEN, PieceColor::BLACK);
    setPiece(Position(7, 4), PieceType::KING, PieceColor::BLACK);
    setPiece(Position(7, 5), PieceType::BISHOP, PieceColor::BLACK);
    setPiece(Position(7, 6), PieceType::KNIGHT, PieceColor::BLACK);
    setPiece(Position(7, 7), PieceType::ROOK, PieceColor::BLACK);
    
    for (int c = 0; c < 8; ++c) {
        setPiece(Position(6, c), PieceType::PAWN, PieceColor::BLACK);
    }
}

void ChessBoardWidget::setPiece(const Position& pos, PieceType type, PieceColor color) {
    Position boardPos = logicalToBoard(pos);
    
    // Remove existing piece if any
    removePiece(pos);
    
    // Create new piece
    ChessPieceItem* piece = new ChessPieceItem(type, color, themeManager, squareSize);
    
    // Position the piece
    piece->setPos(boardPos.col * squareSize, boardPos.row * squareSize);
    
    // Add to scene
    scene->addItem(piece);
    
    // Store in pieces array
    pieces[pos.row][pos.col] = piece;
}

void ChessBoardWidget::removePiece(const Position& pos) {
    if (pos.isValid() && pieces[pos.row][pos.col]) {
        scene->removeItem(pieces[pos.row][pos.col]);
        delete pieces[pos.row][pos.col];
        pieces[pos.row][pos.col] = nullptr;
    }
}

void ChessBoardWidget::movePiece(const Position& from, const Position& to, bool animate) {
    ChessPieceItem* piece = getPieceAt(from);
    if (!piece) {
        return;
    }
    
    // Check if this is a capture
    bool isCapture = getPieceAt(to) != nullptr;
    
    // Remove captured piece
    removePiece(to);
    
    // Update pieces array
    pieces[from.row][from.col] = nullptr;
    pieces[to.row][to.col] = piece;
    
    // Convert to board coordinates
    Position boardFrom = logicalToBoard(from);
    Position boardTo = logicalToBoard(to);
    
    QPointF startPos(boardFrom.col * squareSize, boardFrom.row * squareSize);
    QPointF endPos(boardTo.col * squareSize, boardTo.row * squareSize);
    
    if (animate) {
        // Animate the piece movement
        animatePieceMovement(piece, startPos, endPos);
        
        // Play sound effect
        if (isCapture) {
            audioManager->playSoundEffect(AudioManager::SoundEffect::CAPTURE);
        } else {
            audioManager->playSoundEffect(AudioManager::SoundEffect::MOVE);
        }
    } else {
        // Move the piece immediately
        piece->setPos(endPos);
    }
}

void ChessBoardWidget::setSquareSize(int size) {
    squareSize = size;
    updateBoardSize();
}

int ChessBoardWidget::getSquareSize() const {
    return squareSize;
}

void ChessBoardWidget::setFlipped(bool flip) {
    if (flipped != flip) {
        flipped = flip;
        updateBoardSize();
    }
}

bool ChessBoardWidget::isFlipped() const {
    return flipped;
}

void ChessBoardWidget::highlightSquare(const Position& pos, const QColor& color) {
    Position boardPos = logicalToBoard(pos);
    
    QGraphicsRectItem* highlight = new QGraphicsRectItem(
        boardPos.col * squareSize,
        boardPos.row * squareSize,
        squareSize,
        squareSize
    );
    
    highlight->setBrush(QBrush(color));
    highlight->setOpacity(0.5);
    highlight->setZValue(0.5); // Above squares but below pieces
    
    scene->addItem(highlight);
    highlightItems.append(highlight);
}

void ChessBoardWidget::clearHighlights() {
    for (QGraphicsRectItem* item : highlightItems) {
        scene->removeItem(item);
        delete item;
    }
    highlightItems.clear();
}

void ChessBoardWidget::highlightLastMove(const Position& from, const Position& to) {
    clearHighlights();
    
    highlightSquare(from, themeManager->getLastMoveHighlightColor());
    highlightSquare(to, themeManager->getLastMoveHighlightColor());
}

void ChessBoardWidget::highlightCheck(const Position& kingPos) {
    highlightSquare(kingPos, themeManager->getCheckHighlightColor());
}

void ChessBoardWidget::setPlayerColor(PieceColor color) {
    playerColor = color;
    
    // Flip board if playing as black
    setFlipped(playerColor == PieceColor::BLACK);
}

PieceColor ChessBoardWidget::getPlayerColor() const {
    return playerColor;
}

void ChessBoardWidget::setInteractive(bool interactive) {
    this->interactive = interactive;
}

bool ChessBoardWidget::isInteractive() const {
    return interactive;
}

void ChessBoardWidget::showMoveHints(const QVector<Position>& positions) {
    clearMoveHints();
    
    for (const Position& pos : positions) {
        Position boardPos = logicalToBoard(pos);
        
        // Create a hint circle
        QGraphicsEllipseItem* hint = new QGraphicsEllipseItem(
            boardPos.col * squareSize + squareSize * 0.3,
            boardPos.row * squareSize + squareSize * 0.3,
            squareSize * 0.4,
            squareSize * 0.4
        );
        
        hint->setBrush(QBrush(themeManager->getHighlightColor()));
        hint->setPen(Qt::NoPen);
        hint->setOpacity(0.6);
        hint->setZValue(0.5); // Above squares but below pieces
        
        scene->addItem(hint);
        hintItems.append(hint);
    }
}

void ChessBoardWidget::clearMoveHints() {
    for (QGraphicsEllipseItem* item : hintItems) {
        scene->removeItem(item);
        delete item;
    }
    hintItems.clear();
}

void ChessBoardWidget::setCurrentGameId(const QString& gameId) {
    currentGameId = gameId;
}

QString ChessBoardWidget::getCurrentGameId() const {
    return currentGameId;
}

ChessPieceItem* ChessBoardWidget::getPieceAt(const Position& pos) const {
    if (!pos.isValid()) {
        return nullptr;
    }
    return pieces[pos.row][pos.col];
}

Position ChessBoardWidget::getPositionAt(const QPointF& scenePos) const {
    int col = static_cast<int>(scenePos.x() / squareSize);
    int row = static_cast<int>(scenePos.y() / squareSize);
    
    Position boardPos(row, col);
    return boardToLogical(boardPos);
}

void ChessBoardWidget::updateTheme() {
    // Recreate the board with the new theme
    resetBoard();
}

void ChessBoardWidget::showPromotionDialog(const Position& from, const Position& to, PieceColor color) {
    PromotionDialog dialog(color, themeManager, this);
    
    connect(&dialog, &PromotionDialog::pieceSelected, this, &ChessBoardWidget::onPromotionSelected);
    
    if (dialog.exec() == QDialog::Accepted) {
        PieceType promotionType = dialog.getSelectedPieceType();
        
        // Create the move with promotion
        ChessMove move(from, to, promotionType);
        
        // Emit the move request
        emit moveRequested(currentGameId, move);
    }
}

void ChessBoardWidget::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    updateBoardSize();
}

void ChessBoardWidget::mousePressEvent(QMouseEvent* event) {
    if (!interactive) {
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = mapToScene(event->pos());
        Position pos = getPositionAt(scenePos);
        
        if (pos.isValid()) {
            // Check if there's a piece at this position
            ChessPieceItem* piece = getPieceAt(pos);
            
            if (piece && piece->getColor() == playerColor) {
                // Start dragging the piece
                selectedPosition = pos;
            } else {
                // Clicked on an empty square or opponent's piece
                if (selectedPosition.isValid()) {
                    // Try to move the selected piece to this square
                    handleDrop(pos);
                } else {
                    // Just select the square
                    emit squareClicked(pos);
                }
            }
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void ChessBoardWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!interactive) {
        return;
    }
    
    if (event->button() == Qt::LeftButton && selectedPosition.isValid()) {
        QPointF scenePos = mapToScene(event->pos());
        Position pos = getPositionAt(scenePos);
        
        if (pos.isValid() && pos != selectedPosition) {
            // Try to move the piece
            handleDrop(pos);
        }
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void ChessBoardWidget::dragEnterEvent(QDragEnterEvent* event) {
    event->acceptProposedAction();
}

void ChessBoardWidget::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void ChessBoardWidget::dropEvent(QDropEvent* event) {
    if (!interactive) {
        return;
    }
    
    QPointF scenePos = mapToScene(event->pos());
    Position pos = getPositionAt(scenePos);
    
    if (pos.isValid() && selectedPosition.isValid()) {
        // Try to move the piece
        handleDrop(pos);
    }
    
    event->acceptProposedAction();
}

void ChessBoardWidget::onPromotionSelected(PieceType promotionType) {
    // This is handled in showPromotionDialog
}

void ChessBoardWidget::setupBoard() {
    // Create the board squares
    createSquares();
    
    // Set the scene rect
    scene->setSceneRect(0, 0, 8 * squareSize, 8 * squareSize);
    
    // Fit the view to the scene
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void ChessBoardWidget::updateBoardSize() {
    // Update scene rect
    scene->setSceneRect(0, 0, 8 * squareSize, 8 * squareSize);
    
    // Update square positions
    createSquares();
    
    // Update piece positions and sizes
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (pieces[r][c]) {
                Position boardPos = logicalToBoard(Position(r, c));
                pieces[r][c]->setSquareSize(squareSize);
                pieces[r][c]->setPos(boardPos.col * squareSize, boardPos.row * squareSize);
            }
        }
    }
    
    // Update highlight positions
    clearHighlights();
    
    // Update hint positions
    clearMoveHints();
    
    // Fit the view to the scene
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void ChessBoardWidget::createSquares() {
    // Remove existing squares
    for (QGraphicsItem* item : scene->items()) {
        if (dynamic_cast<QGraphicsRectItem*>(item) && item->zValue() == 0) {
            scene->removeItem(item);
            delete item;
        }
    }
    
    // Create new squares
    QColor lightColor = themeManager->getLightSquareColor();
    QColor darkColor = themeManager->getDarkSquareColor();
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            QGraphicsRectItem* square = new QGraphicsRectItem(c * squareSize, r * squareSize, squareSize, squareSize);
            square->setBrush(((r + c) % 2 == 0) ? lightColor : darkColor);
            square->setPen(Qt::NoPen);
            square->setZValue(0); // Ensure squares are below pieces
            scene->addItem(square);
        }
    }
    
    // Add rank and file labels
    QFont font;
    font.setPointSize(squareSize / 5);
    
    for (int r = 0; r < 8; ++r) {
        QGraphicsTextItem* rankLabel = new QGraphicsTextItem(QString::number(flipped ? r + 1 : 8 - r));
        rankLabel->setFont(font);
        rankLabel->setDefaultTextColor((r % 2 == 0) ? darkColor : lightColor);
        rankLabel->setPos(squareSize * 0.05, r * squareSize + squareSize * 0.05);
        rankLabel->setZValue(0.1);
        scene->addItem(rankLabel);
    }
    
    for (int c = 0; c < 8; ++c) {
        QGraphicsTextItem* fileLabel = new QGraphicsTextItem(QString(QChar('a' + (flipped ? 7 - c : c))));
        fileLabel->setFont(font);
        fileLabel->setDefaultTextColor((c % 2 == 1) ? darkColor : lightColor);
        fileLabel->setPos(c * squareSize + squareSize * 0.85, squareSize * 7.8);
        fileLabel->setZValue(0.1);
        scene->addItem(fileLabel);
    }
}

Position ChessBoardWidget::boardToLogical(const Position& pos) const {
    if (flipped) {
        return Position(7 - pos.row, 7 - pos.col);
    }
    return pos;
}

Position ChessBoardWidget::logicalToBoard(const Position& pos) const {
    if (flipped) {
        return Position(7 - pos.row, 7 - pos.col);
    }
    return pos;
}

void ChessBoardWidget::startDrag(const Position& pos) {
    selectedPosition = pos;
}

void ChessBoardWidget::handleDrop(const Position& pos) {
    if (!selectedPosition.isValid()) {
        return;
    }
    
    // Check if this is a pawn promotion move
    ChessPieceItem* piece = getPieceAt(selectedPosition);
    if (piece && piece->getType() == PieceType::PAWN) {
        int promotionRank = (piece->getColor() == PieceColor::WHITE) ? 7 : 0;
        if (pos.row == promotionRank) {
            // Show promotion dialog
            showPromotionDialog(selectedPosition, pos, piece->getColor());
            selectedPosition = Position();
            return;
        }
    }
    
    // Create the move
    ChessMove move(selectedPosition, pos);
    
    // Emit the move request
    emit moveRequested(currentGameId, move);
    
    // Reset selection
    selectedPosition = Position();
}

void ChessBoardWidget::animatePieceMovement(ChessPieceItem* piece, const QPointF& startPos, const QPointF& endPos) {
    QPropertyAnimation* animation = new QPropertyAnimation(piece, "pos");
    animation->setDuration(300);
    animation->setStartValue(startPos);
    animation->setEndValue(endPos);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    
    // Connect to animation finished signal to clean up
    connect(animation, &QPropertyAnimation::finished, animation, &QPropertyAnimation::deleteLater);
    
    animation->start();
}

// MoveHistoryWidget implementation
MoveHistoryWidget::MoveHistoryWidget(QWidget* parent)
    : QWidget(parent), currentMoveIndex(-1) {
    
    setupUI();
}

MoveHistoryWidget::~MoveHistoryWidget() {
}

void MoveHistoryWidget::clear() {
    moveTable->setRowCount(0);
    currentMoveIndex = -1;
}

void MoveHistoryWidget::addMove(int moveNumber, const QString& whiteMoveNotation, const QString& blackMoveNotation) {
    // Check if this move number already exists
    for (int row = 0; row < moveTable->rowCount(); ++row) {
        if (moveTable->item(row, 0)->text().toInt() == moveNumber) {
            // Update existing row
            if (!whiteMoveNotation.isEmpty()) {
                moveTable->item(row, 1)->setText(whiteMoveNotation);
            }
            if (!blackMoveNotation.isEmpty()) {
                moveTable->item(row, 2)->setText(blackMoveNotation);
            }
            return;
        }
    }
    
    // Add new row
    int row = moveTable->rowCount();
    moveTable->insertRow(row);
    
    // Set move number
    QTableWidgetItem* numberItem = new QTableWidgetItem(QString::number(moveNumber));
    numberItem->setTextAlignment(Qt::AlignCenter);
    moveTable->setItem(row, 0, numberItem);
    
    // Set white move
    QTableWidgetItem* whiteItem = new QTableWidgetItem(whiteMoveNotation);
    whiteItem->setTextAlignment(Qt::AlignCenter);
    moveTable->setItem(row, 1, whiteItem);
    
    // Set black move
    QTableWidgetItem* blackItem = new QTableWidgetItem(blackMoveNotation);
    blackItem->setTextAlignment(Qt::AlignCenter);
    moveTable->setItem(row, 2, blackItem);
    
    // Scroll to the new row
    moveTable->scrollToBottom();
    
    // Update current move index
    if (!whiteMoveNotation.isEmpty() && blackMoveNotation.isEmpty()) {
        currentMoveIndex = moveNumber * 2 - 2;
    } else if (!blackMoveNotation.isEmpty()) {
        currentMoveIndex = moveNumber * 2 - 1;
    }
}

void MoveHistoryWidget::updateLastMove(const QString& moveNotation) {
    if (moveTable->rowCount() == 0) {
        return;
    }
    
    int lastRow = moveTable->rowCount() - 1;
    
    // Check if the last move was white or black
    if (moveTable->item(lastRow, 2)->text().isEmpty()) {
        // Last move was white, update black
        moveTable->item(lastRow, 2)->setText(moveNotation);
        currentMoveIndex = lastRow * 2 + 1;
    } else {
        // Last move was black, add new row for white
        int moveNumber = moveTable->item(lastRow, 0)->text().toInt() + 1;
        addMove(moveNumber, moveNotation, "");
        currentMoveIndex = moveNumber * 2 - 2;
    }
}

void MoveHistoryWidget::setCurrentMoveIndex(int index) {
    if (index < -1 || index >= getMoveCount()) {
        return;
    }
    
    currentMoveIndex = index;
    
    // Highlight the current move
    for (int row = 0; row < moveTable->rowCount(); ++row) {
        for (int col = 1; col <= 2; ++col) {
            QTableWidgetItem* item = moveTable->item(row, col);
            if (item) {
                int moveIndex = row * 2 + (col - 1);
                if (moveIndex == currentMoveIndex) {
                    item->setBackground(QColor(255, 255, 0, 100));
                } else {
                    item->setBackground(QColor(0, 0, 0, 0));
                }
            }
        }
    }
    
    // Emit signal
    emit moveSelected(currentMoveIndex);
}

int MoveHistoryWidget::getCurrentMoveIndex() const {
    return currentMoveIndex;
}

int MoveHistoryWidget::getMoveCount() const {
    int count = 0;
    for (int row = 0; row < moveTable->rowCount(); ++row) {
        if (!moveTable->item(row, 1)->text().isEmpty()) {
            count++;
        }
        if (!moveTable->item(row, 2)->text().isEmpty()) {
            count++;
        }
    }
    return count;
}

void MoveHistoryWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create table for move history
    moveTable = new QTableWidget(this);
    moveTable->setColumnCount(3);
    moveTable->setHorizontalHeaderLabels(QStringList() << "#" << "White" << "Black");
    moveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    moveTable->setSelectionBehavior(QAbstractItemView::SelectItems);
    moveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    moveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    moveTable->verticalHeader()->setVisible(false);
    moveTable->setAlternatingRowColors(true);
    
    layout->addWidget(moveTable);
    
    // Connect signals
    connect(moveTable, &QTableWidget::cellClicked, this, [this](int row, int column) {
        if (column >= 1 && column <= 2) {
            int moveIndex = row * 2 + (column - 1);
            setCurrentMoveIndex(moveIndex);
        }
    });
    
    setLayout(layout);
}

// CapturedPiecesWidget implementation
CapturedPiecesWidget::CapturedPiecesWidget(ThemeManager* themeManager, QWidget* parent)
    : QWidget(parent), themeManager(themeManager), materialAdvantage(0) {
    
    setupUI();
}

CapturedPiecesWidget::~CapturedPiecesWidget() {
}

void CapturedPiecesWidget::clear() {
    whiteCapturedPieces.clear();
    blackCapturedPieces.clear();
    materialAdvantage = 0;
    updateDisplay();
}

void CapturedPiecesWidget::addCapturedPiece(PieceType type, PieceColor color) {
    if (color == PieceColor::WHITE) {
        whiteCapturedPieces.append(type);
        materialAdvantage -= getPieceValue(type);
    } else {
        blackCapturedPieces.append(type);
        materialAdvantage += getPieceValue(type);
    }
    
    updateDisplay();
}

void CapturedPiecesWidget::updateTheme() {
    updateDisplay();
}

void CapturedPiecesWidget::setMaterialAdvantage(int advantage) {
    materialAdvantage = advantage;
    updateDisplay();
}

int CapturedPiecesWidget::getMaterialAdvantage() const {
    return materialAdvantage;
}

void CapturedPiecesWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create labels for captured pieces
    whiteCapturedLabel = new QLabel(this);
    blackCapturedLabel = new QLabel(this);
    materialAdvantageLabel = new QLabel(this);
    
    whiteCapturedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    blackCapturedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    materialAdvantageLabel->setAlignment(Qt::AlignCenter);
    
    QFont font = whiteCapturedLabel->font();
    font.setPointSize(font.pointSize() + 2);
    whiteCapturedLabel->setFont(font);
    blackCapturedLabel->setFont(font);
    
    font.setBold(true);
    materialAdvantageLabel->setFont(font);
    
    layout->addWidget(blackCapturedLabel);
    layout->addWidget(materialAdvantageLabel);
    layout->addWidget(whiteCapturedLabel);
    
    setLayout(layout);
    
    updateDisplay();
}

void CapturedPiecesWidget::updateDisplay() {
    // Sort captured pieces by value
    std::sort(whiteCapturedPieces.begin(), whiteCapturedPieces.end(), [this](PieceType a, PieceType b) {
        return getPieceValue(a) > getPieceValue(b);
    });
    
    std::sort(blackCapturedPieces.begin(), blackCapturedPieces.end(), [this](PieceType a, PieceType b) {
        return getPieceValue(a) > getPieceValue(b);
    });
    
    // Create strings for captured pieces
    QString whiteText;
    for (PieceType type : whiteCapturedPieces) {
        whiteText += getPieceSymbol(type, PieceColor::WHITE);
    }
    
    QString blackText;
    for (PieceType type : blackCapturedPieces) {
        blackText += getPieceSymbol(type, PieceColor::BLACK);
    }
    
    // Set label texts
    whiteCapturedLabel->setText(whiteText);
    blackCapturedLabel->setText(blackText);
    
    // Set material advantage text
    if (materialAdvantage > 0) {
        materialAdvantageLabel->setText(QString("+%1").arg(materialAdvantage));
        materialAdvantageLabel->setStyleSheet("color: green;");
    } else if (materialAdvantage < 0) {
        materialAdvantageLabel->setText(QString("%1").arg(materialAdvantage));
        materialAdvantageLabel->setStyleSheet("color: red;");
    } else {
        materialAdvantageLabel->setText("0");
        materialAdvantageLabel->setStyleSheet("");
    }
}

int CapturedPiecesWidget::getPieceValue(PieceType type) const {
    switch (type) {
        case PieceType::PAWN:   return 1;
        case PieceType::KNIGHT: return 3;
        case PieceType::BISHOP: return 3;
        case PieceType::ROOK:   return 5;
        case PieceType::QUEEN:  return 9;
        case PieceType::KING:   return 0;
        default:                return 0;
    }
}

QString CapturedPiecesWidget::getPieceSymbol(PieceType type, PieceColor color) const {
    QString symbol;
    
    switch (type) {
        case PieceType::PAWN:   symbol = "♙"; break;
        case PieceType::KNIGHT: symbol = "♘"; break;
        case PieceType::BISHOP: symbol = "♗"; break;
        case PieceType::ROOK:   symbol = "♖"; break;
        case PieceType::QUEEN:  symbol = "♕"; break;
        case PieceType::KING:   symbol = "♔"; break;
        default:                symbol = ""; break;
    }
    
    if (color == PieceColor::BLACK) {
        // Use black piece symbols
        symbol = symbol.toLower();
    }
    
    return symbol;
}

// GameTimerWidget implementation
GameTimerWidget::GameTimerWidget(QWidget* parent)
    : QWidget(parent), whiteTimeMs(0), blackTimeMs(0), activeColor(PieceColor::WHITE),
      timeControl(TimeControlType::RAPID) {
    
    setupUI();
    
    // Create timer
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GameTimerWidget::updateActiveTimer);
    timer->setInterval(100); // Update every 100ms
}

GameTimerWidget::~GameTimerWidget() {
    if (timer->isActive()) {
        timer->stop();
    }
}

void GameTimerWidget::setWhiteTime(qint64 milliseconds) {
    whiteTimeMs = milliseconds;
    whiteTimerLabel->setText(formatTime(whiteTimeMs));
    updateProgressBars();
}

void GameTimerWidget::setBlackTime(qint64 milliseconds) {
    blackTimeMs = milliseconds;
    blackTimerLabel->setText(formatTime(blackTimeMs));
    updateProgressBars();
}

void GameTimerWidget::setActiveColor(PieceColor color) {
    activeColor = color;
    
    // Update timer labels to show which one is active
    QFont whiteFont = whiteTimerLabel->font();
    QFont blackFont = blackTimerLabel->font();
    
    whiteFont.setBold(activeColor == PieceColor::WHITE);
    blackFont.setBold(activeColor == PieceColor::BLACK);
    
    whiteTimerLabel->setFont(whiteFont);
    blackTimerLabel->setFont(blackFont);
    
    // Update progress bars
    updateProgressBars();
    
    // Reset last update time
    lastUpdateTime = QDateTime::currentDateTime();
}

PieceColor GameTimerWidget::getActiveColor() const {
    return activeColor;
}

void GameTimerWidget::start() {
    if (!timer->isActive()) {
        lastUpdateTime = QDateTime::currentDateTime();
        timer->start();
    }
}

void GameTimerWidget::stop() {
    if (timer->isActive()) {
        timer->stop();
    }
}

void GameTimerWidget::reset() {
    stop();
    
    // Reset times based on time control
    qint64 initialTime = getInitialTimeForControl(timeControl);
    setWhiteTime(initialTime);
    setBlackTime(initialTime);
    
    // Reset active color
    setActiveColor(PieceColor::WHITE);
}

void GameTimerWidget::setTimeControl(TimeControlType control) {
    timeControl = control;
    reset();
}

TimeControlType GameTimerWidget::getTimeControl() const {
    return timeControl;
}

void GameTimerWidget::updateActiveTimer() {
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = lastUpdateTime.msecsTo(now);
    lastUpdateTime = now;
    
    // Decrement the active timer
    if (activeColor == PieceColor::WHITE) {
        whiteTimeMs = qMax(0LL, whiteTimeMs - elapsed);
        whiteTimerLabel->setText(formatTime(whiteTimeMs));
    } else {
        blackTimeMs = qMax(0LL, blackTimeMs - elapsed);
        blackTimerLabel->setText(formatTime(blackTimeMs));
    }
    
    // Update progress bars
    updateProgressBars();
}

void GameTimerWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create timer labels
    whiteTimerLabel = new QLabel("00:00", this);
    blackTimerLabel = new QLabel("00:00", this);
    
    whiteTimerLabel->setAlignment(Qt::AlignCenter);
    blackTimerLabel->setAlignment(Qt::AlignCenter);
    
    QFont font = whiteTimerLabel->font();
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    whiteTimerLabel->setFont(font);
    blackTimerLabel->setFont(font);
    
    // Create progress bars
    whiteProgressBar = new QProgressBar(this);
    blackProgressBar = new QProgressBar(this);
    
    whiteProgressBar->setTextVisible(false);
    blackProgressBar->setTextVisible(false);
    
    whiteProgressBar->setMinimum(0);
    whiteProgressBar->setMaximum(100);
    blackProgressBar->setMinimum(0);
    blackProgressBar->setMaximum(100);
    
    // Create layouts for each player
    QVBoxLayout* whiteLayout = new QVBoxLayout();
    whiteLayout->addWidget(whiteTimerLabel);
    whiteLayout->addWidget(whiteProgressBar);
    
    QVBoxLayout* blackLayout = new QVBoxLayout();
    blackLayout->addWidget(blackTimerLabel);
    blackLayout->addWidget(blackProgressBar);
    
    // Add to main layout
    layout->addLayout(blackLayout);
    layout->addSpacing(20);
    layout->addLayout(whiteLayout);
    
    setLayout(layout);
}

QString GameTimerWidget::formatTime(qint64 milliseconds) const {
    // For casual games (days per move)
    if (timeControl == TimeControlType::CASUAL) {
        qint64 days = milliseconds / (1000 * 60 * 60 * 24);
        qint64 hours = (milliseconds % (1000 * 60 * 60 * 24)) / (1000 * 60 * 60);
        
        if (days > 0) {
            return QString("%1d %2h").arg(days).arg(hours);
        } else {
            qint64 minutes = (milliseconds % (1000 * 60 * 60)) / (1000 * 60);
            return QString("%1h %2m").arg(hours).arg(minutes);
        }
    }
    
    // For regular time controls
    qint64 totalSeconds = milliseconds / 1000;
    qint64 minutes = totalSeconds / 60;
    qint64 seconds = totalSeconds % 60;
    
    // For times less than 10 seconds, show tenths of a second
    if (totalSeconds < 10) {
        qint64 tenths = (milliseconds % 1000) / 100;
        return QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(tenths);
    }
    
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

qint64 GameTimerWidget::getInitialTimeForControl(TimeControlType control) const {
    switch (control) {
        case TimeControlType::RAPID:
            return 10 * 60 * 1000; // 10 minutes
        case TimeControlType::BLITZ:
            return 5 * 60 * 1000;  // 5 minutes
        case TimeControlType::BULLET:
            return 1 * 60 * 1000;  // 1 minute
        case TimeControlType::CLASSICAL:
            return 90 * 60 * 1000; // 90 minutes
        case TimeControlType::CASUAL:
            return 7 * 24 * 60 * 60 * 1000; // 7 days
    }
    return 10 * 60 * 1000; // Default to 10 minutes
}

void GameTimerWidget::updateProgressBars() {
    qint64 initialTime = getInitialTimeForControl(timeControl);
    
    int whiteProgress = static_cast<int>((static_cast<double>(whiteTimeMs) / initialTime) * 100);
    int blackProgress = static_cast<int>((static_cast<double>(blackTimeMs) / initialTime) * 100);
    
    whiteProgressBar->setValue(whiteProgress);
    blackProgressBar->setValue(blackProgress);
    
    // Set colors based on remaining time
    QString whiteStyle, blackStyle;
    
    if (whiteTimeMs < 30000) { // Less than 30 seconds
        whiteStyle = "QProgressBar::chunk { background-color: red; }";
    } else if (whiteTimeMs < 60000) { // Less than 1 minute
        whiteStyle = "QProgressBar::chunk { background-color: orange; }";
    } else {
        whiteStyle = "QProgressBar::chunk { background-color: green; }";
    }
    
    if (blackTimeMs < 30000) { // Less than 30 seconds
        blackStyle = "QProgressBar::chunk { background-color: red; }";
    } else if (blackTimeMs < 60000) { // Less than 1 minute
        blackStyle = "QProgressBar::chunk { background-color: orange; }";
    } else {
        blackStyle = "QProgressBar::chunk { background-color: green; }";
    }
    
    whiteProgressBar->setStyleSheet(whiteStyle);
    blackProgressBar->setStyleSheet(blackStyle);
}

// AnalysisWidget implementation
AnalysisWidget::AnalysisWidget(QWidget* parent)
    : QWidget(parent), showEvaluation(true), showRecommendations(true) {
    
    setupUI();
}

AnalysisWidget::~AnalysisWidget() {
}

void AnalysisWidget::clear() {
    // Clear evaluation chart
    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->setTitle("Evaluation");
    chart->legend()->hide();
    evaluationChartView->setChart(chart);
    
    // Clear recommendations table
    recommendationsTable->setRowCount(0);
    
    // Clear mistakes table
    mistakesTable->setRowCount(0);
}

void AnalysisWidget::setAnalysisData(const QJsonObject& analysis) {
    // Clear previous data
    clear();
    
    // Create evaluation chart
    if (analysis.contains("moveAnalysis")) {
        createEvaluationChart(analysis["moveAnalysis"].toArray());
    }
    
    // Populate mistakes table
    if (analysis.contains("mistakes")) {
        populateMistakesTable(analysis["mistakes"].toObject());
    }
}

void AnalysisWidget::setMoveRecommendations(const QJsonArray& recommendations) {
    populateRecommendationsTable(recommendations);
}

void AnalysisWidget::setShowEvaluation(bool show) {
    showEvaluation = show;
    evaluationTab->setVisible(show);
    mistakesTab->setVisible(show);
}

bool AnalysisWidget::isShowingEvaluation() const {
    return showEvaluation;
}

void AnalysisWidget::setShowRecommendations(bool show) {
    showRecommendations = show;
    recommendationsTab->setVisible(show);
}

bool AnalysisWidget::isShowingRecommendations() const {
    return showRecommendations;
}

void AnalysisWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create tab widget
    tabWidget = new QTabWidget(this);
    
    // Create evaluation tab
    evaluationTab = new QWidget();
    QVBoxLayout* evalLayout = new QVBoxLayout(evaluationTab);
    
    evaluationChartView = new QtCharts::QChartView(new QtCharts::QChart(), evaluationTab);
    evaluationChartView->setRenderHint(QPainter::Antialiasing);
    evaluationChartView->chart()->setTitle("Evaluation");
    evaluationChartView->chart()->legend()->hide();
    
    evalLayout->addWidget(evaluationChartView);
    
    // Create recommendations tab
    recommendationsTab = new QWidget();
    QVBoxLayout* recLayout = new QVBoxLayout(recommendationsTab);
    
    recommendationsTable = new QTableWidget(recommendationsTab);
    recommendationsTable->setColumnCount(3);
    recommendationsTable->setHorizontalHeaderLabels(QStringList() << "Move" << "Evaluation" << "Description");
    recommendationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recommendationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    recommendationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    recommendationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recommendationsTable->verticalHeader()->setVisible(false);
    recommendationsTable->setAlternatingRowColors(true);
    
    recLayout->addWidget(recommendationsTable);
    
    // Create mistakes tab
    mistakesTab = new QWidget();
    QVBoxLayout* mistakesLayout = new QVBoxLayout(mistakesTab);
    
    mistakesTable = new QTableWidget(mistakesTab);
    mistakesTable->setColumnCount(4);
    mistakesTable->setHorizontalHeaderLabels(QStringList() << "Move" << "Player" << "Type" << "Evaluation");
    mistakesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mistakesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mistakesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mistakesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mistakesTable->verticalHeader()->setVisible(false);
    mistakesTable->setAlternatingRowColors(true);
    
    mistakesLayout->addWidget(mistakesTable);
    
    // Add tabs to tab widget
    tabWidget->addTab(evaluationTab, "Evaluation");
    tabWidget->addTab(recommendationsTab, "Recommendations");
    tabWidget->addTab(mistakesTab, "Mistakes");
    
    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    analyzeButton = new QPushButton("Analyze Game", this);
    stockfishButton = new QPushButton("Stockfish Analysis", this);
    
    buttonLayout->addWidget(analyzeButton);
    buttonLayout->addWidget(stockfishButton);
    
    // Add widgets to main layout
    layout->addWidget(tabWidget);
    layout->addLayout(buttonLayout);
    
    setLayout(layout);
    
    // Connect signals
    connect(recommendationsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* moveItem = recommendationsTable->item(row, 0);
        if (moveItem) {
            QString moveStr = moveItem->data(Qt::UserRole).toString();
            ChessMove move = ChessMove::fromAlgebraic(moveStr);
            emit moveSelected(move);
        }
    });
    
    connect(analyzeButton, &QPushButton::clicked, this, [this]() {
        emit requestAnalysis(false);
    });
    
    connect(stockfishButton, &QPushButton::clicked, this, [this]() {
        emit requestAnalysis(true);
    });
}

void AnalysisWidget::createEvaluationChart(const QJsonArray& moveAnalysis) {
    // Create a line series for the evaluation
    QtCharts::QLineSeries* series = new QtCharts::QLineSeries();
    series->setName("Evaluation");
    
    // Add data points
    int moveNumber = 0;
    for (const QJsonValue& value : moveAnalysis) {
        QJsonObject moveObj = value.toObject();
        double evaluation = moveObj["evaluationAfter"].toDouble();
        
        // Cap evaluation for better visualization
        evaluation = qBound(-5.0, evaluation, 5.0);
        
        series->append(moveNumber, evaluation);
        moveNumber++;
    }
    
    // Create chart
    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->addSeries(series);
    chart->setTitle("Evaluation");
    chart->legend()->hide();
    
    // Create axes
    QtCharts::QValueAxis* axisX = new QtCharts::QValueAxis();
    axisX->setTitleText("Move");
    axisX->setLabelFormat("%d");
    axisX->setTickCount(std::min(11, moveNumber + 1));
    
    QtCharts::QValueAxis* axisY = new QtCharts::QValueAxis();
    axisY->setTitleText("Evaluation (pawns)");
    axisY->setRange(-5, 5);
    axisY->setTickCount(11);
    
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    
    // Add a horizontal line at 0
    QtCharts::QLineSeries* zeroLine = new QtCharts::QLineSeries();
    zeroLine->append(0, 0);
    zeroLine->append(moveNumber > 0 ? moveNumber - 1 : 1, 0);
    zeroLine->setPen(QPen(Qt::gray, 1, Qt::DashLine));
    chart->addSeries(zeroLine);
    zeroLine->attachAxis(axisX);
    zeroLine->attachAxis(axisY);
    
    // Set the chart on the chart view
    evaluationChartView->setChart(chart);
}

void AnalysisWidget::populateRecommendationsTable(const QJsonArray& recommendations) {
    // Clear the table
    recommendationsTable->setRowCount(0);
    
    // Add recommendations
    for (int i = 0; i < recommendations.size(); ++i) {
        QJsonObject recObj = recommendations[i].toObject();
        
        QString move = recObj["move"].toString();
        double evaluation = recObj["evaluation"].toDouble();
        QString notation = recObj["standardNotation"].toString();
        
        int row = recommendationsTable->rowCount();
        recommendationsTable->insertRow(row);
        
        // Move column
        QTableWidgetItem* moveItem = new QTableWidgetItem(notation);
        moveItem->setData(Qt::UserRole, move);
        recommendationsTable->setItem(row, 0, moveItem);
        
        // Evaluation column
        QString evalText = QString("%1").arg(evaluation, 0, 'f', 2);
        QTableWidgetItem* evalItem = new QTableWidgetItem(evalText);
        evalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        recommendationsTable->setItem(row, 1, evalItem);
        
        // Description column
        QString description;
        if (i == 0) {
            description = "Best move";
        } else if (i == 1) {
            description = "Second best";
        } else {
            description = QString("Alternative %1").arg(i);
        }
        QTableWidgetItem* descItem = new QTableWidgetItem(description);
        recommendationsTable->setItem(row, 2, descItem);
    }
}

void AnalysisWidget::populateMistakesTable(const QJsonObject& mistakes) {
    // Clear the table
    mistakesTable->setRowCount(0);
    
    // Add blunders
    QJsonArray blunders = mistakes["blunders"].toArray();
    for (const QJsonValue& value : blunders) {
        QJsonObject mistakeObj = value.toObject();
        addMistakeToTable(mistakeObj, "Blunder");
    }
    
    // Add errors
    QJsonArray errors = mistakes["errors"].toArray();
    for (const QJsonValue& value : errors) {
        QJsonObject mistakeObj = value.toObject();
        addMistakeToTable(mistakeObj, "Error");
    }
    
    // Add inaccuracies
    QJsonArray inaccuracies = mistakes["inaccuracies"].toArray();
    for (const QJsonValue& value : inaccuracies) {
        QJsonObject mistakeObj = value.toObject();
        addMistakeToTable(mistakeObj, "Inaccuracy");
    }
    
    // Sort by move number
    mistakesTable->sortItems(0);
}

void AnalysisWidget::addMistakeToTable(const QJsonObject& mistakeObj, const QString& type) {
    int moveNumber = mistakeObj["moveNumber"].toInt();
    QString color = mistakeObj["color"].toString();
    QString move = mistakeObj["standardNotation"].toString();
    double evalBefore = mistakeObj["evaluationBefore"].toDouble();
    double evalAfter = mistakeObj["evaluationAfter"].toDouble();
    double evalChange = mistakeObj["evaluationChange"].toDouble();
    
    int row = mistakesTable->rowCount();
    mistakesTable->insertRow(row);
    
    // Move column
    QString moveText = QString("%1. %2").arg(moveNumber).arg(move);
    QTableWidgetItem* moveItem = new QTableWidgetItem(moveText);
    moveItem->setData(Qt::UserRole, moveNumber);
    mistakesTable->setItem(row, 0, moveItem);
    
    // Player column
    QTableWidgetItem* playerItem = new QTableWidgetItem(color);
    mistakesTable->setItem(row, 1, playerItem);
    
    // Type column
    QTableWidgetItem* typeItem = new QTableWidgetItem(type);
    mistakesTable->setItem(row, 2, typeItem);
    
    // Evaluation column
    QString evalText = QString("%1 → %2 (%3)")
        .arg(evalBefore, 0, 'f', 2)
        .arg(evalAfter, 0, 'f', 2)
        .arg(evalChange, 0, 'f', 2);
    QTableWidgetItem* evalItem = new QTableWidgetItem(evalText);
    mistakesTable->setItem(row, 3, evalItem);
    
    // Set background color based on mistake type
    QColor bgColor;
    if (type == "Blunder") {
        bgColor = QColor(255, 0, 0, 50);
    } else if (type == "Error") {
        bgColor = QColor(255, 165, 0, 50);
    } else {
        bgColor = QColor(255, 255, 0, 50);
    }
    
    for (int col = 0; col < mistakesTable->columnCount(); ++col) {
        mistakesTable->item(row, col)->setBackground(bgColor);
    }
}

// ProfileWidget implementation
ProfileWidget::ProfileWidget(QWidget* parent)
    : QWidget(parent) {
    
    setupUI();
}

ProfileWidget::~ProfileWidget() {
}

void ProfileWidget::setPlayerData(const QJsonObject& playerData) {
    // Set basic info
    QString username = playerData["username"].toString();
    int rating = playerData["rating"].toInt();
    int wins = playerData["wins"].toInt();
    int losses = playerData["losses"].toInt();
    int draws = playerData["draws"].toInt();
    int gamesPlayed = wins + losses + draws;
    double winRate = gamesPlayed > 0 ? (static_cast<double>(wins) / gamesPlayed) * 100.0 : 0.0;
    
    usernameLabel->setText(username);
    ratingLabel->setText(QString("Rating: %1").arg(rating));
    winsLabel->setText(QString("Wins: %1").arg(wins));
    lossesLabel->setText(QString("Losses: %1").arg(losses));
    drawsLabel->setText(QString("Draws: %1").arg(draws));
    winRateLabel->setText(QString("Win Rate: %1%").arg(winRate, 0, 'f', 1));
    
    // Create stats chart
    createStatsChart(wins, losses, draws);
    
    // Populate recent games table
    if (playerData.contains("gameHistory")) {
        populateRecentGamesTable(playerData["gameHistory"].toArray());
    }
}

void ProfileWidget::clear() {
    usernameLabel->setText("");
    ratingLabel->setText("Rating: 0");
    winsLabel->setText("Wins: 0");
    lossesLabel->setText("Losses: 0");
    drawsLabel->setText("Draws: 0");
    winRateLabel->setText("Win Rate: 0.0%");
    
    // Clear chart
    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->setTitle("Game Results");
    statsChartView->setChart(chart);
    
    // Clear table
    recentGamesTable->setRowCount(0);
}

void ProfileWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create player info section
    QVBoxLayout* infoLayout = new QVBoxLayout();
    
    usernameLabel = new QLabel(this);
    ratingLabel = new QLabel(this);
    winsLabel = new QLabel(this);
    lossesLabel = new QLabel(this);
    drawsLabel = new QLabel(this);
    winRateLabel = new QLabel(this);
    
    QFont titleFont = usernameLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    usernameLabel->setFont(titleFont);
    
    QFont statsFont = ratingLabel->font();
    statsFont.setPointSize(statsFont.pointSize() + 1);
    ratingLabel->setFont(statsFont);
    winsLabel->setFont(statsFont);
    lossesLabel->setFont(statsFont);
    drawsLabel->setFont(statsFont);
    winRateLabel->setFont(statsFont);
    
    infoLayout->addWidget(usernameLabel, 0, Qt::AlignCenter);
    infoLayout->addWidget(ratingLabel, 0, Qt::AlignCenter);
    
    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->addWidget(winsLabel);
    statsLayout->addWidget(lossesLabel);
    statsLayout->addWidget(drawsLabel);
    statsLayout->addWidget(winRateLabel);
    
    infoLayout->addLayout(statsLayout);
    
    // Create stats chart
    statsChartView = new QtCharts::QChartView(new QtCharts::QChart(), this);
    statsChartView->setRenderHint(QPainter::Antialiasing);
    statsChartView->chart()->setTitle("Game Results");
    
    // Create recent games table
    QLabel* recentGamesLabel = new QLabel("Recent Games", this);
    QFont recentGamesFont = recentGamesLabel->font();
    recentGamesFont.setBold(true);
    recentGamesLabel->setFont(recentGamesFont);
    
    recentGamesTable = new QTableWidget(this);
    recentGamesTable->setColumnCount(4);
    recentGamesTable->setHorizontalHeaderLabels(QStringList() << "Date" << "Opponent" << "Result" << "Rating Change");
    recentGamesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recentGamesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    recentGamesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    recentGamesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    recentGamesTable->verticalHeader()->setVisible(false);
    recentGamesTable->setAlternatingRowColors(true);
    
    // Add widgets to main layout
    layout->addLayout(infoLayout);
    layout->addWidget(statsChartView);
    layout->addWidget(recentGamesLabel);
    layout->addWidget(recentGamesTable);
    
    setLayout(layout);
    
    // Connect signals
    connect(recentGamesTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* item = recentGamesTable->item(row, 0);
        if (item) {
            QString gameId = item->data(Qt::UserRole).toString();
            emit gameSelected(gameId);
        }
    });
}

void ProfileWidget::createStatsChart(int wins, int losses, int draws) {
    // Create pie series
    QtCharts::QPieSeries* series = new QtCharts::QPieSeries();
    
    if (wins > 0) {
        QtCharts::QPieSlice* winsSlice = series->append("Wins", wins);
        winsSlice->setBrush(QColor(76, 175, 80));
        winsSlice->setLabelVisible();
    }
    
    if (losses > 0) {
        QtCharts::QPieSlice* lossesSlice = series->append("Losses", losses);
        lossesSlice->setBrush(QColor(244, 67, 54));
        lossesSlice->setLabelVisible();
    }
    
    if (draws > 0) {
        QtCharts::QPieSlice* drawsSlice = series->append("Draws", draws);
        drawsSlice->setBrush(QColor(255, 193, 7));
        drawsSlice->setLabelVisible();
    }
    
    // Create chart
    QtCharts::QChart* chart = new QtCharts::QChart();
    chart->addSeries(series);
    chart->setTitle("Game Results");
    chart->legend()->setAlignment(Qt::AlignBottom);
    
    // Set the chart on the chart view
    statsChartView->setChart(chart);
}

void ProfileWidget::populateRecentGamesTable(const QJsonArray& games) {
    // Clear the table
    recentGamesTable->setRowCount(0);
    
    // Add games
    for (const QJsonValue& value : games) {
        QJsonObject gameObj = value.toObject();
        
        QString gameId = gameObj["gameId"].toString();
        QString opponent = gameObj["opponent"].toString();
        QString result = gameObj["result"].toString();
        int ratingChange = gameObj["ratingChange"].toInt();
        QDateTime date = QDateTime::fromString(gameObj["date"].toString(), Qt::ISODate);
        
        int row = recentGamesTable->rowCount();
        recentGamesTable->insertRow(row);
        
        // Date column
        QTableWidgetItem* dateItem = new QTableWidgetItem(date.toString("yyyy-MM-dd"));
        dateItem->setData(Qt::UserRole, gameId);
        recentGamesTable->setItem(row, 0, dateItem);
        
        // Opponent column
        QTableWidgetItem* opponentItem = new QTableWidgetItem(opponent);
        recentGamesTable->setItem(row, 1, opponentItem);
        
        // Result column
        QString resultText;
        QColor resultColor;
        if (result == "win") {
            resultText = "Win";
            resultColor = QColor(76, 175, 80);
        } else if (result == "loss") {
            resultText = "Loss";
            resultColor = QColor(244, 67, 54);
        } else {
            resultText = "Draw";
            resultColor = QColor(255, 193, 7);
        }
        
        QTableWidgetItem* resultItem = new QTableWidgetItem(resultText);
        resultItem->setForeground(resultColor);
        resultItem->setTextAlignment(Qt::AlignCenter);
        recentGamesTable->setItem(row, 2, resultItem);
        
        // Rating change column
        QString ratingChangeText = (ratingChange >= 0) ? QString("+%1").arg(ratingChange) : QString::number(ratingChange);
        QTableWidgetItem* ratingChangeItem = new QTableWidgetItem(ratingChangeText);
        ratingChangeItem->setForeground(ratingChange >= 0 ? QColor(76, 175, 80) : QColor(244, 67, 54));
        ratingChangeItem->setTextAlignment(Qt::AlignCenter);
        recentGamesTable->setItem(row, 3, ratingChangeItem);
    }
}

// LeaderboardWidget implementation
LeaderboardWidget::LeaderboardWidget(QWidget* parent)
    : QWidget(parent) {
    
    setupUI();
}

LeaderboardWidget::~LeaderboardWidget() {
}

void LeaderboardWidget::setLeaderboardData(const QJsonObject& leaderboard) {
    // Get leaderboard data
    QJsonArray byRating = leaderboard["byRating"].toArray();
    QJsonArray byWins = leaderboard["byWins"].toArray();
    QJsonArray byWinPercentage = leaderboard["byWinPercentage"].toArray();
    
    // Populate tables
    populateTable(ratingTable, byRating);
    populateTable(winsTable, byWins);
    populateTable(winRateTable, byWinPercentage);
    
    // Update total players label
    int totalPlayers = leaderboard["totalPlayers"].toInt();
    totalPlayersLabel->setText(QString("Total Players: %1").arg(totalPlayers));
}

void LeaderboardWidget::clear() {
    ratingTable->setRowCount(0);
    winsTable->setRowCount(0);
    winRateTable->setRowCount(0);
    
    yourRatingRankLabel->setText("Your Rank: -");
    yourWinsRankLabel->setText("Your Rank: -");
    yourWinRateRankLabel->setText("Your Rank: -");
    
    totalPlayersLabel->setText("Total Players: 0");
}

void LeaderboardWidget::setPlayerRanks(const QJsonObject& ranks) {
    int ratingRank = ranks["byRating"].toInt();
    int winsRank = ranks["byWins"].toInt();
    int winRateRank = ranks["byWinPercentage"].toInt();
    
    yourRatingRankLabel->setText(QString("Your Rank: %1").arg(ratingRank > 0 ? QString::number(ratingRank) : "-"));
    yourWinsRankLabel->setText(QString("Your Rank: %1").arg(winsRank > 0 ? QString::number(winsRank) : "-"));
    yourWinRateRankLabel->setText(QString("Your Rank: %1").arg(winRateRank > 0 ? QString::number(winRateRank) : "-"));
}

void LeaderboardWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create tab widget
    tabWidget = new QTabWidget(this);
    
    // Create rating tab
    QWidget* ratingTab = new QWidget();
    QVBoxLayout* ratingLayout = new QVBoxLayout(ratingTab);
    
    ratingTable = new QTableWidget(ratingTab);
    ratingTable->setColumnCount(5);
    ratingTable->setHorizontalHeaderLabels(QStringList() << "Rank" << "Player" << "Rating" << "W/L/D" << "Win Rate");
    ratingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ratingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ratingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ratingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ratingTable->verticalHeader()->setVisible(false);
    ratingTable->setAlternatingRowColors(true);
    
    yourRatingRankLabel = new QLabel("Your Rank: -", ratingTab);
    yourRatingRankLabel->setAlignment(Qt::AlignCenter);
    
    ratingLayout->addWidget(ratingTable);
    ratingLayout->addWidget(yourRatingRankLabel);
    
    // Create wins tab
    QWidget* winsTab = new QWidget();
    QVBoxLayout* winsLayout = new QVBoxLayout(winsTab);
    
    winsTable = new QTableWidget(winsTab);
    winsTable->setColumnCount(5);
    winsTable->setHorizontalHeaderLabels(QStringList() << "Rank" << "Player" << "Wins" << "Rating" << "Win Rate");
    winsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    winsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    winsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    winsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    winsTable->verticalHeader()->setVisible(false);
    winsTable->setAlternatingRowColors(true);
    
    yourWinsRankLabel = new QLabel("Your Rank: -", winsTab);
    yourWinsRankLabel->setAlignment(Qt::AlignCenter);
    
    winsLayout->addWidget(winsTable);
    winsLayout->addWidget(yourWinsRankLabel);
    
    // Create win rate tab
    QWidget* winRateTab = new QWidget();
    QVBoxLayout* winRateLayout = new QVBoxLayout(winRateTab);
    
    winRateTable = new QTableWidget(winRateTab);
    winRateTable->setColumnCount(5);
    winRateTable->setHorizontalHeaderLabels(QStringList() << "Rank" << "Player" << "Win Rate" << "W/L/D" << "Rating");
    winRateTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    winRateTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    winRateTable->setSelectionMode(QAbstractItemView::SingleSelection);
    winRateTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    winRateTable->verticalHeader()->setVisible(false);
    winRateTable->setAlternatingRowColors(true);
    
    yourWinRateRankLabel = new QLabel("Your Rank: -", winRateTab);
    yourWinRateRankLabel->setAlignment(Qt::AlignCenter);
    
    winRateLayout->addWidget(winRateTable);
    winRateLayout->addWidget(yourWinRateRankLabel);
    
    // Add tabs to tab widget
    tabWidget->addTab(ratingTab, "By Rating");
    tabWidget->addTab(winsTab, "By Wins");
    tabWidget->addTab(winRateTab, "By Win Rate");
    
    // Create controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    
    showAllButton = new QPushButton("Show All Players", this);
    totalPlayersLabel = new QLabel("Total Players: 0", this);
    
    controlsLayout->addWidget(showAllButton);
    controlsLayout->addWidget(totalPlayersLabel);
    
    // Add widgets to main layout
    layout->addWidget(tabWidget);
    layout->addLayout(controlsLayout);
    
    setLayout(layout);
    
    // Connect signals
    connect(ratingTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* item = ratingTable->item(row, 1);
        if (item) {
            emit playerSelected(item->text());
        }
    });
    
    connect(winsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* item = winsTable->item(row, 1);
        if (item) {
            emit playerSelected(item->text());
        }
    });
    
    connect(winRateTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* item = winRateTable->item(row, 1);
        if (item) {
            emit playerSelected(item->text());
        }
    });
    
    connect(showAllButton, &QPushButton::clicked, this, [this]() {
        bool showAll = showAllButton->text() == "Show All Players";
        showAllButton->setText(showAll ? "Show Top 100" : "Show All Players");
        emit requestAllPlayers(showAll);
    });
}

void LeaderboardWidget::populateTable(QTableWidget* table, const QJsonArray& data) {
    // Clear the table
    table->setRowCount(0);
    
    // Add players
    for (const QJsonValue& value : data) {
        QJsonObject playerObj = value.toObject();
        
        int rank = playerObj["rank"].toInt();
        QString username = playerObj["username"].toString();
        int rating = playerObj["rating"].toInt();
        int wins = playerObj["wins"].toInt();
        int losses = playerObj["losses"].toInt();
        int draws = playerObj["draws"].toInt();
        double winPercentage = playerObj["winPercentage"].toDouble();
        
        int row = table->rowCount();
        table->insertRow(row);
        
        // Rank column
        QTableWidgetItem* rankItem = new QTableWidgetItem(QString::number(rank));
        rankItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(row, 0, rankItem);
        
        // Player column
        QTableWidgetItem* playerItem = new QTableWidgetItem(username);
        table->setItem(row, 1, playerItem);
        
        // Other columns depend on the table
        if (table == ratingTable) {
            // Rating column
            QTableWidgetItem* ratingItem = new QTableWidgetItem(QString::number(rating));
            ratingItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 2, ratingItem);
            
            // W/L/D column
            QTableWidgetItem* wldItem = new QTableWidgetItem(QString("%1/%2/%3").arg(wins).arg(losses).arg(draws));
            wldItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 3, wldItem);
            
            // Win Rate column
            QTableWidgetItem* winRateItem = new QTableWidgetItem(QString("%1%").arg(winPercentage, 0, 'f', 1));
            winRateItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 4, winRateItem);
        } else if (table == winsTable) {
            // Wins column
            QTableWidgetItem* winsItem = new QTableWidgetItem(QString::number(wins));
            winsItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 2, winsItem);
            
            // Rating column
            QTableWidgetItem* ratingItem = new QTableWidgetItem(QString::number(rating));
            ratingItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 3, ratingItem);
            
            // Win Rate column
            QTableWidgetItem* winRateItem = new QTableWidgetItem(QString("%1%").arg(winPercentage, 0, 'f', 1));
            winRateItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 4, winRateItem);
        } else if (table == winRateTable) {
            // Win Rate column
            QTableWidgetItem* winRateItem = new QTableWidgetItem(QString("%1%").arg(winPercentage, 0, 'f', 1));
            winRateItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 2, winRateItem);
            
            // W/L/D column
            QTableWidgetItem* wldItem = new QTableWidgetItem(QString("%1/%2/%3").arg(wins).arg(losses).arg(draws));
            wldItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 3, wldItem);
            
            // Rating column
            QTableWidgetItem* ratingItem = new QTableWidgetItem(QString::number(rating));
            ratingItem->setTextAlignment(Qt::AlignCenter);
            table->setItem(row, 4, ratingItem);
        }
    }
}

void LeaderboardWidget::highlightPlayer(QTableWidget* table, const QString& username) {
    for (int row = 0; row < table->rowCount(); ++row) {
        QTableWidgetItem* playerItem = table->item(row, 1);
        if (playerItem && playerItem->text() == username) {
            table->selectRow(row);
            table->scrollToItem(playerItem);
            break;
        }
    }
}

// MatchmakingWidget implementation
MatchmakingWidget::MatchmakingWidget(QWidget* parent)
    : QWidget(parent), inQueue(false) {
    
    setupUI();
    
    // Create queue timer
    queueTimer = new QTimer(this);
    connect(queueTimer, &QTimer::timeout, this, &MatchmakingWidget::updateQueueTime);
    queueTimer->setInterval(1000);
}

MatchmakingWidget::~MatchmakingWidget() {
    if (queueTimer->isActive()) {
        queueTimer->stop();
    }
}

void MatchmakingWidget::setMatchmakingStatus(const QJsonObject& status) {
    QString statusStr = status["status"].toString();
    
    if (statusStr == "queued") {
        inQueue = true;
        statusLabel->setText("Status: In Queue");
        joinQueueButton->setEnabled(false);
        leaveQueueButton->setEnabled(true);
        
        int queueSize = status["queueSize"].toInt();
        queueSizeLabel->setText(QString("Queue Size: %1").arg(queueSize));
        
        // Start queue timer if not already running
        if (!queueTimer->isActive()) {
            queueStartTime = QDateTime::currentDateTime();
            queueTimer->start();
        }
    } else if (statusStr == "left") {
        inQueue = false;
        statusLabel->setText("Status: Not in Queue");
        joinQueueButton->setEnabled(true);
        leaveQueueButton->setEnabled(false);
        queueSizeLabel->setText("Queue Size: 0");
        
        // Stop queue timer
        if (queueTimer->isActive()) {
            queueTimer->stop();
        }
        
        queueTimeLabel->setText("Time in Queue: 0:00");
        queueProgressBar->setValue(0);
    } else if (statusStr == "matched") {
        inQueue = false;
        QString opponent = status["opponent"].toString();
        statusLabel->setText(QString("Status: Matched with %1").arg(opponent));
        joinQueueButton->setEnabled(false);
        leaveQueueButton->setEnabled(false);
        
        // Stop queue timer
        if (queueTimer->isActive()) {
            queueTimer->stop();
        }
    } else if (statusStr == "matched_with_bot") {
        inQueue = false;
        QString opponent = status["opponent"].toString();
        statusLabel->setText(QString("Status: Matched with bot %1").arg(opponent));
        joinQueueButton->setEnabled(false);
        leaveQueueButton->setEnabled(false);
        
        // Stop queue timer
        if (queueTimer->isActive()) {
            queueTimer->stop();
        }
    } else if (statusStr == "already_in_game") {
        inQueue = false;
        statusLabel->setText("Status: Already in a game");
        joinQueueButton->setEnabled(false);
        leaveQueueButton->setEnabled(false);
    }
}

void MatchmakingWidget::clear() {
    inQueue = false;
    statusLabel->setText("Status: Not in Queue");
    joinQueueButton->setEnabled(true);
    leaveQueueButton->setEnabled(false);
    queueSizeLabel->setText("Queue Size: 0");
    
    // Stop queue timer
    if (queueTimer->isActive()) {
        queueTimer->stop();
    }
    
    queueTimeLabel->setText("Time in Queue: 0:00");
    queueProgressBar->setValue(0);
}

bool MatchmakingWidget::isInQueue() const {
    return inQueue;
}

void MatchmakingWidget::onJoinQueueClicked() {
    TimeControlType timeControl = getSelectedTimeControl();
    emit requestMatchmaking(true, timeControl);
}

void MatchmakingWidget::onLeaveQueueClicked() {
    emit requestMatchmaking(false);
}

void MatchmakingWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create time control selection
    QGroupBox* timeControlGroup = new QGroupBox("Time Control", this);
    QVBoxLayout* timeControlLayout = new QVBoxLayout(timeControlGroup);
    
    timeControlComboBox = new QComboBox(timeControlGroup);
    timeControlComboBox->addItem("Bullet (1 minute)", static_cast<int>(TimeControlType::BULLET));
    timeControlComboBox->addItem("Blitz (5 minutes)", static_cast<int>(TimeControlType::BLITZ));
    timeControlComboBox->addItem("Rapid (10 minutes)", static_cast<int>(TimeControlType::RAPID));
    timeControlComboBox->addItem("Classical (90 minutes)", static_cast<int>(TimeControlType::CLASSICAL));
    timeControlComboBox->addItem("Casual (7 days per move)", static_cast<int>(TimeControlType::CASUAL));
    
    timeControlLayout->addWidget(timeControlComboBox);
    
    // Create queue controls
    QGroupBox* queueGroup = new QGroupBox("Matchmaking Queue", this);
    QVBoxLayout* queueLayout = new QVBoxLayout(queueGroup);
    
    statusLabel = new QLabel("Status: Not in Queue", queueGroup);
    queueTimeLabel = new QLabel("Time in Queue: 0:00", queueGroup);
    queueSizeLabel = new QLabel("Queue Size: 0", queueGroup);
    
    queueProgressBar = new QProgressBar(queueGroup);
    queueProgressBar->setRange(0, 60);
    queueProgressBar->setValue(0);
    queueProgressBar->setFormat("Bot match in %v seconds");
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    joinQueueButton = new QPushButton("Join Queue", queueGroup);
    leaveQueueButton = new QPushButton("Leave Queue", queueGroup);
    
    leaveQueueButton->setEnabled(false);
    
    buttonLayout->addWidget(joinQueueButton);
    buttonLayout->addWidget(leaveQueueButton);
    
    queueLayout->addWidget(statusLabel);
    queueLayout->addWidget(queueTimeLabel);
    queueLayout->addWidget(queueSizeLabel);
    queueLayout->addWidget(queueProgressBar);
    queueLayout->addLayout(buttonLayout);
    
    // Add groups to main layout
    layout->addWidget(timeControlGroup);
    layout->addWidget(queueGroup);
    layout->addStretch();
    
    setLayout(layout);
    
    // Connect signals
    connect(joinQueueButton, &QPushButton::clicked, this, &MatchmakingWidget::onJoinQueueClicked);
    connect(leaveQueueButton, &QPushButton::clicked, this, &MatchmakingWidget::onLeaveQueueClicked);
}

void MatchmakingWidget::updateQueueTime() {
    QDateTime now = QDateTime::currentDateTime();
    int seconds = queueStartTime.secsTo(now);
    
    int minutes = seconds / 60;
    int remainingSeconds = seconds % 60;
    
    queueTimeLabel->setText(QString("Time in Queue: %1:%2")
                           .arg(minutes)
                           .arg(remainingSeconds, 2, 10, QChar('0')));
    
    // Update progress bar (for bot match countdown)
    int botMatchCountdown = 60 - std::min(60, seconds);
    queueProgressBar->setValue(botMatchCountdown);
}

TimeControlType MatchmakingWidget::getSelectedTimeControl() const {
    return static_cast<TimeControlType>(timeControlComboBox->currentData().toInt());
}

// GameHistoryWidget implementation
GameHistoryWidget::GameHistoryWidget(QWidget* parent)
    : QWidget(parent) {
    
    setupUI();
}

GameHistoryWidget::~GameHistoryWidget() {
}

void GameHistoryWidget::setGameHistoryData(const QJsonArray& gameHistory) {
    populateGamesTable(gameHistory);
}

void GameHistoryWidget::clear() {
    gamesTable->setRowCount(0);
}

void GameHistoryWidget::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create filter controls
    QHBoxLayout* filterLayout = new QHBoxLayout();
    
    QLabel* filterLabel = new QLabel("Filter:", this);
    filterComboBox = new QComboBox(this);
    filterComboBox->addItem("All Games");
    filterComboBox->addItem("Wins");
    filterComboBox->addItem("Losses");
    filterComboBox->addItem("Draws");
    filterComboBox->addItem("In Progress");
    
    refreshButton = new QPushButton("Refresh", this);
    
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(filterComboBox);
    filterLayout->addStretch();
    filterLayout->addWidget(refreshButton);
    
    // Create games table
    gamesTable = new QTableWidget(this);
    gamesTable->setColumnCount(5);
    gamesTable->setHorizontalHeaderLabels(QStringList() << "Date" << "White" << "Black" << "Result" << "Moves");
    gamesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    gamesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    gamesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    gamesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    gamesTable->verticalHeader()->setVisible(false);
    gamesTable->setAlternatingRowColors(true);
    
    // Add widgets to main layout
    layout->addLayout(filterLayout);
    layout->addWidget(gamesTable);
    
    setLayout(layout);
    
    // Connect signals
    connect(refreshButton, &QPushButton::clicked, this, &GameHistoryWidget::requestGameHistory);
    
    connect(gamesTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int column) {
        Q_UNUSED(column);
        
        QTableWidgetItem* item = gamesTable->item(row, 0);
        if (item) {
            QString gameId = item->data(Qt::UserRole).toString();
            emit gameSelected(gameId);
        }
    });
    
    connect(filterComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        Q_UNUSED(index);
        
        // Apply filter to the table
        QString filter = filterComboBox->currentText();
        
        for (int row = 0; row < gamesTable->rowCount(); ++row) {
            bool show = true;
            
            if (filter != "All Games") {
                QTableWidgetItem* resultItem = gamesTable->item(row, 3);
                if (resultItem) {
                    QString result = resultItem->text();
                    
                    if (filter == "Wins" && result != "Win") {
                        show = false;
                    } else if (filter == "Losses" && result != "Loss") {
                        show = false;
                    } else if (filter == "Draws" && result != "Draw") {
                        show = false;
                    } else if (filter == "In Progress" && result != "In Progress") {
                        show = false;
                    }
                }
            }
            
            gamesTable->setRowHidden(row, !show);
        }
    });
}

void GameHistoryWidget::populateGamesTable(const QJsonArray& games) {
    // Clear the table
    gamesTable->setRowCount(0);
    
    // Add games
    for (const QJsonValue& value : games) {
        QJsonObject gameObj = value.toObject();
        
        QString gameId = gameObj["gameId"].toString();
        QString whitePlayer = gameObj["whitePlayer"].toString();
        QString blackPlayer = gameObj["blackPlayer"].toString();
        QString result = gameObj["result"].toString();
        bool active = gameObj["active"].toBool();
        QDateTime startTime = QDateTime::fromString(gameObj["startTime"].toString(), Qt::ISODate);
        
        int row = gamesTable->rowCount();
        gamesTable->insertRow(row);
        
        // Date column
        QTableWidgetItem* dateItem = new QTableWidgetItem(startTime.toString("yyyy-MM-dd hh:mm"));
        dateItem->setData(Qt::UserRole, gameId);
        gamesTable->setItem(row, 0, dateItem);
        
        // White column
        QTableWidgetItem* whiteItem = new QTableWidgetItem(whitePlayer);
        gamesTable->setItem(row, 1, whiteItem);
        
        // Black column
        QTableWidgetItem* blackItem = new QTableWidgetItem(blackPlayer);
        gamesTable->setItem(row, 2, blackItem);
        
        // Result column
        QString resultText;
        QColor resultColor;
        
        if (active) {
            resultText = "In Progress";
            resultColor = QColor(66, 139, 202);
        } else if (result == "white_win") {
            resultText = whitePlayer == filterComboBox->property("username").toString() ? "Win" : "Loss";
            resultColor = resultText == "Win" ? QColor(76, 175, 80) : QColor(244, 67, 54);
        } else if (result == "black_win") {
            resultText = blackPlayer == filterComboBox->property("username").toString() ? "Win" : "Loss";
            resultColor = resultText == "Win" ? QColor(76, 175, 80) : QColor(244, 67, 54);
        } else {
            resultText = "Draw";
            resultColor = QColor(255, 193, 7);
        }
        
        QTableWidgetItem* resultItem = new QTableWidgetItem(resultText);
        resultItem->setForeground(resultColor);
        resultItem->setTextAlignment(Qt::AlignCenter);
        gamesTable->setItem(row, 3, resultItem);
        
        // Moves column
        int moves = gameObj.contains("moves") ? gameObj["moves"].toInt() : 0;
        QTableWidgetItem* movesItem = new QTableWidgetItem(QString::number(moves));
        movesItem->setTextAlignment(Qt::AlignCenter);
        gamesTable->setItem(row, 4, movesItem);
    }
    
    // Apply current filter
    int filterIndex = filterComboBox->currentIndex();
    filterComboBox->setCurrentIndex(0);  // Reset to "All Games"
    filterComboBox->setCurrentIndex(filterIndex);  // Re-apply filter
}

// PromotionDialog implementation
PromotionDialog::PromotionDialog(PieceColor color, ThemeManager* themeManager, QWidget* parent)
    : QDialog(parent), selectedType(PieceType::QUEEN), themeManager(themeManager), color(color) {
    
    setupUI();
    
    setWindowTitle("Promote Pawn");
    setModal(true);
}

PromotionDialog::~PromotionDialog() {
}

PieceType PromotionDialog::getSelectedPieceType() const {
    return selectedType;
}

void PromotionDialog::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("Choose promotion piece:", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);
    
    layout->addWidget(titleLabel);
    
    // Create piece buttons
    QHBoxLayout* piecesLayout = new QHBoxLayout();
    
    createPieceButton(PieceType::QUEEN, "Queen", piecesLayout);
    createPieceButton(PieceType::ROOK, "Rook", piecesLayout);
    createPieceButton(PieceType::BISHOP, "Bishop", piecesLayout);
    createPieceButton(PieceType::KNIGHT, "Knight", piecesLayout);
    
    layout->addLayout(piecesLayout);
    
    setLayout(layout);
}

void PromotionDialog::createPieceButton(PieceType type, const QString& label, QHBoxLayout* layout) {
    QPushButton* button = new QPushButton(this);
    button->setMinimumSize(80, 80);
    
    // Load piece SVG
    QString svgFileName = ChessPiece(type, color).getSvgFileName(themeManager->getPieceThemePath());
    QSvgRenderer renderer(svgFileName);
    
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    
    button->setIcon(QIcon(pixmap));
    button->setIconSize(QSize(64, 64));
    button->setText(label);
    button->setToolTip(label);
    
    // Use vertical text layout
    button->setStyleSheet("text-align: bottom; padding-top: 5px;");
    
    connect(button, &QPushButton::clicked, this, [this, type]() {
        selectedType = type;
        emit pieceSelected(type);
        accept();
    });
    
    layout->addWidget(button);
}

// LoginDialog implementation
LoginDialog::LoginDialog(QWidget* parent)
    : QDialog(parent), registering(false) {
    
    setupUI();
    
    setWindowTitle("Chess Client - Login");
    setModal(true);
}

LoginDialog::~LoginDialog() {
}

QString LoginDialog::getUsername() const {
    return usernameEdit->text();
}

QString LoginDialog::getPassword() const {
    return passwordEdit->text();
}

bool LoginDialog::isRegistering() const {
    return registering;
}

void LoginDialog::onLoginClicked() {
    if (usernameEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
        statusLabel->setText("Please enter username and password");
        return;
    }
    
    registering = false;
    emit loginRequested(usernameEdit->text(), passwordEdit->text(), false);
}

void LoginDialog::onRegisterClicked() {
    if (usernameEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
        statusLabel->setText("Please enter username and password");
        return;
    }
    
    registering = true;
    emit loginRequested(usernameEdit->text(), passwordEdit->text(), true);
}

void LoginDialog::onTogglePasswordVisibility() {
    if (passwordEdit->echoMode() == QLineEdit::Password) {
        passwordEdit->setEchoMode(QLineEdit::Normal);
        togglePasswordButton->setText("Hide");
    } else {
        passwordEdit->setEchoMode(QLineEdit::Password);
        togglePasswordButton->setText("Show");
    }
}

void LoginDialog::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create title label
    QLabel* titleLabel = new QLabel("Chess Client", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    
    // Create form layout
    QFormLayout* formLayout = new QFormLayout();
    
    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    
    QHBoxLayout* passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(passwordEdit);
    
    togglePasswordButton = new QPushButton("Show", this);
    togglePasswordButton->setFixedWidth(50);
    passwordLayout->addWidget(togglePasswordButton);
    
    formLayout->addRow("Username:", usernameEdit);
    formLayout->addRow("Password:", passwordLayout);
    
    // Create buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    loginButton = new QPushButton("Login", this);
    registerButton = new QPushButton("Register", this);
    
    buttonLayout->addWidget(loginButton);
    buttonLayout->addWidget(registerButton);
    
    // Create status label
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("color: red;");
    
    // Add widgets to main layout
    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addLayout(formLayout);
    layout->addSpacing(10);
    layout->addLayout(buttonLayout);
    layout->addWidget(statusLabel);
    
    setLayout(layout);
    
    // Set minimum size
    setMinimumSize(300, 200);
    
    // Connect signals
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(togglePasswordButton, &QPushButton::clicked, this, &LoginDialog::onTogglePasswordVisibility);
    
    // Connect enter key to login button
    connect(usernameEdit, &QLineEdit::returnPressed, loginButton, &QPushButton::click);
    connect(passwordEdit, &QLineEdit::returnPressed, loginButton, &QPushButton::click);
}

// SettingsDialog implementation
SettingsDialog::SettingsDialog(ThemeManager* themeManager, AudioManager* audioManager, QWidget* parent)
    : QDialog(parent), themeManager(themeManager), audioManager(audioManager) {
    
    setupUI();
    loadSettings();
    
    setWindowTitle("Settings");
    setModal(true);
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::onThemeChanged(int index) {
    ThemeManager::Theme theme = static_cast<ThemeManager::Theme>(index);
    themeManager->setTheme(theme);
}

void SettingsDialog::onBoardThemeChanged(int index) {
    ThemeManager::BoardTheme theme = static_cast<ThemeManager::BoardTheme>(index);
    themeManager->setBoardTheme(theme);
}

void SettingsDialog::onPieceThemeChanged(int index) {
    ThemeManager::PieceTheme theme = static_cast<ThemeManager::PieceTheme>(index);
    themeManager->setPieceTheme(theme);
}

void SettingsDialog::onSoundEffectsToggled(bool enabled) {
    audioManager->setSoundEffectsEnabled(enabled);
}

void SettingsDialog::onMusicToggled(bool enabled) {
    audioManager->setBackgroundMusicEnabled(enabled);
}

void SettingsDialog::onSoundVolumeChanged(int value) {
    audioManager->setSoundEffectVolume(value);
}

void SettingsDialog::onMusicVolumeChanged(int value) {
    audioManager->setBackgroundMusicVolume(value);
}

void SettingsDialog::onCustomColorsClicked() {
    // Show color dialog for light squares
    QColor lightColor = QColorDialog::getColor(themeManager->getLightSquareColor(), this, "Choose Light Square Color");
    if (lightColor.isValid()) {
        themeManager->setCustomLightSquareColor(lightColor);
    }
    
    // Show color dialog for dark squares
    QColor darkColor = QColorDialog::getColor(themeManager->getDarkSquareColor(), this, "Choose Dark Square Color");
    if (darkColor.isValid()) {
        themeManager->setCustomDarkSquareColor(darkColor);
    }
    
    // Show color dialog for highlight
    QColor highlightColor = QColorDialog::getColor(themeManager->getHighlightColor(), this, "Choose Highlight Color");
    if (highlightColor.isValid()) {
        themeManager->setCustomHighlightColor(highlightColor);
    }
    
    // Set board theme to custom
    themeManager->setBoardTheme(ThemeManager::BoardTheme::CUSTOM);
    boardThemeComboBox->setCurrentIndex(static_cast<int>(ThemeManager::BoardTheme::CUSTOM));
}

void SettingsDialog::onResetToDefaultsClicked() {
    // Reset theme settings
    themeManager->setTheme(ThemeManager::Theme::LIGHT);
    themeManager->setBoardTheme(ThemeManager::BoardTheme::CLASSIC);
    themeManager->setPieceTheme(ThemeManager::PieceTheme::CLASSIC);
    
    // Reset audio settings
    audioManager->setSoundEffectsEnabled(true);
    audioManager->setBackgroundMusicEnabled(true);
    audioManager->setSoundEffectVolume(50);
    audioManager->setBackgroundMusicVolume(30);
    
    // Update UI
    loadSettings();
}

void SettingsDialog::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QTabWidget* tabWidget = new QTabWidget(this);
    
    // Appearance tab
    QWidget* appearanceTab = new QWidget();
    QVBoxLayout* appearanceLayout = new QVBoxLayout(appearanceTab);
    
    QGroupBox* themeGroup = new QGroupBox("Theme", appearanceTab);
    QFormLayout* themeLayout = new QFormLayout(themeGroup);
    
    themeComboBox = new QComboBox(themeGroup);
    themeComboBox->addItem("Light");
    themeComboBox->addItem("Dark");
    themeComboBox->addItem("Custom");
    
    boardThemeComboBox = new QComboBox(themeGroup);
    boardThemeComboBox->addItem("Classic");
    boardThemeComboBox->addItem("Wood");
    boardThemeComboBox->addItem("Marble");
    boardThemeComboBox->addItem("Blue");
    boardThemeComboBox->addItem("Green");
    boardThemeComboBox->addItem("Custom");
    
    pieceThemeComboBox = new QComboBox(themeGroup);
    pieceThemeComboBox->addItem("Classic");
    pieceThemeComboBox->addItem("Modern");
    pieceThemeComboBox->addItem("Simple");
    pieceThemeComboBox->addItem("Fancy");
    pieceThemeComboBox->addItem("Custom");
    
    customColorsButton = new QPushButton("Custom Colors...", themeGroup);
    
    themeLayout->addRow("Application Theme:", themeComboBox);
    themeLayout->addRow("Board Theme:", boardThemeComboBox);
    themeLayout->addRow("Piece Theme:", pieceThemeComboBox);
    themeLayout->addRow("", customColorsButton);
    
    appearanceLayout->addWidget(themeGroup);
    appearanceLayout->addStretch();
    
    // Audio tab
    QWidget* audioTab = new QWidget();
    QVBoxLayout* audioLayout = new QVBoxLayout(audioTab);
    
    QGroupBox* soundGroup = new QGroupBox("Sound", audioTab);
    QVBoxLayout* soundLayout = new QVBoxLayout(soundGroup);
    
    soundEffectsCheckBox = new QCheckBox("Enable Sound Effects", soundGroup);
    musicCheckBox = new QCheckBox("Enable Background Music", soundGroup);
    
    QLabel* soundVolumeLabel = new QLabel("Sound Effects Volume:", soundGroup);
    soundVolumeSlider = new QSlider(Qt::Horizontal, soundGroup);
    soundVolumeSlider->setRange(0, 100);
    soundVolumeSlider->setTickPosition(QSlider::TicksBelow);
    soundVolumeSlider->setTickInterval(10);
    
    QLabel* musicVolumeLabel = new QLabel("Background Music Volume:", soundGroup);
    musicVolumeSlider = new QSlider(Qt::Horizontal, soundGroup);
    musicVolumeSlider->setRange(0, 100);
    musicVolumeSlider->setTickPosition(QSlider::TicksBelow);
    musicVolumeSlider->setTickInterval(10);
    
    soundLayout->addWidget(soundEffectsCheckBox);
    soundLayout->addWidget(soundVolumeLabel);
    soundLayout->addWidget(soundVolumeSlider);
    soundLayout->addWidget(musicCheckBox);
    soundLayout->addWidget(musicVolumeLabel);
    soundLayout->addWidget(musicVolumeSlider);
    
    audioLayout->addWidget(soundGroup);
    audioLayout->addStretch();
    
    // Add tabs
    tabWidget->addTab(appearanceTab, "Appearance");
    tabWidget->addTab(audioTab, "Audio");
    
    // Reset button
    resetButton = new QPushButton("Reset to Defaults", this);
    
    // Add to main layout
    layout->addWidget(tabWidget);
    layout->addWidget(resetButton);
    
    // Add standard buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttonBox);
    
    setLayout(layout);
    
    // Connect signals
    connect(themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onThemeChanged);
    connect(boardThemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onBoardThemeChanged);
    connect(pieceThemeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onPieceThemeChanged);
    
    connect(soundEffectsCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onSoundEffectsToggled);
    connect(musicCheckBox, &QCheckBox::toggled, this, &SettingsDialog::onMusicToggled);
    connect(soundVolumeSlider, &QSlider::valueChanged, this, &SettingsDialog::onSoundVolumeChanged);
    connect(musicVolumeSlider, &QSlider::valueChanged, this, &SettingsDialog::onMusicVolumeChanged);
    
    connect(customColorsButton, &QPushButton::clicked, this, &SettingsDialog::onCustomColorsClicked);
    connect(resetButton, &QPushButton::clicked, this, &SettingsDialog::onResetToDefaultsClicked);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    
    connect(this, &QDialog::accepted, this, &SettingsDialog::saveSettings);
}

void SettingsDialog::loadSettings() {
    // Load theme settings
    themeComboBox->setCurrentIndex(static_cast<int>(themeManager->getTheme()));
    boardThemeComboBox->setCurrentIndex(static_cast<int>(themeManager->getBoardTheme()));
    pieceThemeComboBox->setCurrentIndex(static_cast<int>(themeManager->getPieceTheme()));
    
    // Load audio settings
    soundEffectsCheckBox->setChecked(audioManager->areSoundEffectsEnabled());
    musicCheckBox->setChecked(audioManager->isBackgroundMusicEnabled());
    soundVolumeSlider->setValue(audioManager->getSoundEffectVolume());
    musicVolumeSlider->setValue(audioManager->getBackgroundMusicVolume());
}

void SettingsDialog::saveSettings() {
    // Theme settings are saved immediately when changed
    
    // Audio settings
    audioManager->setSoundEffectsEnabled(soundEffectsCheckBox->isChecked());
    audioManager->setBackgroundMusicEnabled(musicCheckBox->isChecked());
    audioManager->setSoundEffectVolume(soundVolumeSlider->value());
    audioManager->setBackgroundMusicVolume(musicVolumeSlider->value());
    
    emit settingsChanged();
}

// GameManager implementation
GameManager::GameManager(NetworkManager* networkManager, Logger* logger, QObject* parent)
    : QObject(parent), networkManager(networkManager), logger(logger),
      playerColor(PieceColor::WHITE), gameActive(false) {
}

GameManager::~GameManager() {
}

void GameManager::startNewGame(const QJsonObject& gameData) {
    // Extract game information
    currentGameId = gameData["gameId"].toString();
    QString whitePlayer = gameData["whitePlayer"].toString();
    QString blackPlayer = gameData["blackPlayer"].toString();
    QString yourColor = gameData["yourColor"].toString();
    
    // Set player color
    playerColor = (yourColor == "white") ? PieceColor::WHITE : PieceColor::BLACK;
    
    // Set game as active
    gameActive = true;
    
    // Clear move history
    moveHistory.clear();
    
    // Clear move recommendations
    moveRecommendations = QJsonArray();
    
    logger->info(QString("Starting new game: %1, You are playing as %2")
                .arg(currentGameId)
                .arg(yourColor));
    
    // Emit signal
    emit gameStarted(gameData);
}

void GameManager::updateGameState(const QJsonObject& gameState) {
    // Update current game state
    currentGameState = gameState;
    
    // Parse move history
    if (gameState.contains("moveHistory")) {
        parseMoveHistory(gameState["moveHistory"].toArray());
    }
    
    // Emit signal
    emit gameStateUpdated(gameState);
    emit moveHistoryUpdated(moveHistory);
}

void GameManager::endGame(const QJsonObject& gameOverData) {
    // Set game as inactive
    gameActive = false;
    
    // Emit signal
    emit gameEnded(gameOverData);
}

QString GameManager::getCurrentGameId() const {
    return currentGameId;
}

PieceColor GameManager::getPlayerColor() const {
    return playerColor;
}

bool GameManager::isGameActive() const {
    return gameActive;
}

void GameManager::makeMove(const ChessMove& move) {
    if (!gameActive) {
        logger->warning("Attempted to make a move in an inactive game");
        return;
    }
    
    // Send move to server
    networkManager->sendMove(currentGameId, move);
}

void GameManager::offerDraw() {
    if (!gameActive) {
        logger->warning("Attempted to offer draw in an inactive game");
        return;
    }
    
    // Send draw offer to server
    networkManager->sendDrawOffer(currentGameId);
}

void GameManager::respondToDraw(bool accept) {
    if (!gameActive) {
        logger->warning("Attempted to respond to draw in an inactive game");
        return;
    }
    
    // Send draw response to server
    networkManager->sendDrawResponse(currentGameId, accept);
}

void GameManager::resign() {
    if (!gameActive) {
        logger->warning("Attempted to resign an inactive game");
        return;
    }
    
    // Send resignation to server
    networkManager->sendResignation(currentGameId);
}

const QJsonObject& GameManager::getCurrentGameState() const {
    return currentGameState;
}

QVector<ChessMove> GameManager::getMoveHistory() const {
    return moveHistory;
}

void GameManager::setMoveRecommendations(const QJsonArray& recommendations) {
    moveRecommendations = recommendations;
    emit moveRecommendationsUpdated(recommendations);
}

QJsonArray GameManager::getMoveRecommendations() const {
    return moveRecommendations;
}

void GameManager::parseMoveHistory(const QJsonArray& moveHistoryArray) {
    moveHistory.clear();
    
    for (const QJsonValue& value : moveHistoryArray) {
        QJsonObject moveObj = value.toObject();
        
        QString from = moveObj["from"].toString();
        QString to = moveObj["to"].toString();
        
        // Create move
        ChessMove move = ChessMove::fromAlgebraic(from + to);
        
        // Check for promotion
        if (moveObj.contains("promotion")) {
            QString promotion = moveObj["promotion"].toString();
            PieceType promotionType = PieceType::QUEEN;
            
            if (promotion == "queen") {
                promotionType = PieceType::QUEEN;
            } else if (promotion == "rook") {
                promotionType = PieceType::ROOK;
            } else if (promotion == "bishop") {
                promotionType = PieceType::BISHOP;
            } else if (promotion == "knight") {
                promotionType = PieceType::KNIGHT;
            }
            
            move.setPromotionType(promotionType);
        }
        
        moveHistory.append(move);
    }
}

// MPChessClient implementation
MPChessClient::MPChessClient(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MPChessClient), replayMode(false), currentReplayIndex(-1) {
    
    // Initialize core components
    logger = new Logger(this);
    logger->setLogLevel(Logger::LogLevel::INFO);
    logger->setLogToFile(true);
    
    networkManager = new NetworkManager(logger, this);
    themeManager = new ThemeManager(this);
    audioManager = new AudioManager(this);
    gameManager = new GameManager(networkManager, logger, this);
    
    // Set up UI
    setupUI();
    
    // Connect network signals
    connect(networkManager, &NetworkManager::connected, this, &MPChessClient::onConnected);
    connect(networkManager, &NetworkManager::disconnected, this, &MPChessClient::onDisconnected);
    connect(networkManager, &NetworkManager::connectionError, this, &MPChessClient::onConnectionError);
    connect(networkManager, &NetworkManager::authenticationResult, this, &MPChessClient::onAuthenticationResult);
    connect(networkManager, &NetworkManager::gameStarted, this, &MPChessClient::onGameStarted);
    connect(networkManager, &NetworkManager::gameStateUpdated, this, &MPChessClient::onGameStateUpdated);
    connect(networkManager, &NetworkManager::gameOver, this, &MPChessClient::onGameOver);
    connect(networkManager, &NetworkManager::moveResult, this, &MPChessClient::onMoveResult);
    connect(networkManager, &NetworkManager::moveRecommendationsReceived, this, &MPChessClient::onMoveRecommendationsReceived);
    connect(networkManager, &NetworkManager::matchmakingStatus, this, &MPChessClient::onMatchmakingStatusReceived);
    connect(networkManager, &NetworkManager::gameHistoryReceived, this, &MPChessClient::onGameHistoryReceived);
    connect(networkManager, &NetworkManager::gameAnalysisReceived, this, &MPChessClient::onGameAnalysisReceived);
    connect(networkManager, &NetworkManager::drawOfferReceived, this, &MPChessClient::onDrawOfferReceived);
    connect(networkManager, &NetworkManager::drawResponseReceived, this, &MPChessClient::onDrawResponseReceived);
    connect(networkManager, &NetworkManager::leaderboardReceived, this, &MPChessClient::onLeaderboardReceived);
    
    // Connect game manager signals
    connect(gameManager, &GameManager::gameStarted, this, &MPChessClient::onGameStarted);
    connect(gameManager, &GameManager::gameStateUpdated, this, &MPChessClient::onGameStateUpdated);
    connect(gameManager, &GameManager::gameEnded, this, &MPChessClient::onGameOver);
    connect(gameManager, &GameManager::moveRecommendationsUpdated, analysisWidget, &AnalysisWidget::setMoveRecommendations);
    
    // Load settings
    loadSettings();
    
    // Update theme
    updateTheme();
    
    // Show login dialog
    QTimer::singleShot(100, this, &MPChessClient::showLoginDialog);
    
    logger->info("MPChessClient initialized");
}

MPChessClient::~MPChessClient() {
    // Save settings
    saveSettings();
    
    // Disconnect from server
    disconnectFromServer();
    
    delete ui;
}

bool MPChessClient::connectToServer(const QString& host, int port) {
    return networkManager->connectToServer(host, port);
}

void MPChessClient::disconnectFromServer() {
    if (networkManager->isConnected()) {
        networkManager->disconnectFromServer();
    }
}

void MPChessClient::closeEvent(QCloseEvent* event) {
    // Ask for confirmation if in an active game
    if (gameManager->isGameActive()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Exit Confirmation",
            "You are in an active game. Are you sure you want to exit?",
            QMessageBox::Yes | QMessageBox::No
        );
        
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    
    // Disconnect from server
    disconnectFromServer();
    
    // Accept the close event
    event->accept();
}

void MPChessClient::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    
    // Resize the board to fit the view
    if (boardWidget) {
        int minDimension = std::min(boardWidget->width(), boardWidget->height());
        int squareSize = minDimension / 8;
        boardWidget->setSquareSize(squareSize);
    }
}

void MPChessClient::onConnected() {
    connectionStatusLabel->setText("Connected");
    connectionStatusLabel->setStyleSheet("color: green;");
    
    // Show login dialog
    showLoginDialog();
}

void MPChessClient::onDisconnected() {
    connectionStatusLabel->setText("Disconnected");
    connectionStatusLabel->setStyleSheet("color: red;");
    
    // Show message
    showMessage("Disconnected from server", true);
    
    // Show login dialog
    showLoginDialog();
}

void MPChessClient::onConnectionError(const QString& errorMessage) {
    connectionStatusLabel->setText("Connection Error");
    connectionStatusLabel->setStyleSheet("color: red;");
    
    // Show error message
    showMessage("Connection error: " + errorMessage, true);
    
    // Play error sound
    audioManager->playSoundEffect(AudioManager::SoundEffect::ERROR);
}

void MPChessClient::onAuthenticationResult(bool success, const QString& message) {
    if (success) {
        // Hide login dialog
        if (loginDialog && loginDialog->isVisible()) {
            loginDialog->accept();
        }
        
        // Show success message
        showMessage("Authentication successful");
        
        // Request leaderboard
        networkManager->requestLeaderboard();
        
        // Request game history
        networkManager->requestGameHistory();
        
        // Play notification sound
        audioManager->playSoundEffect(AudioManager::SoundEffect::NOTIFICATION);
    } else {
        // Show error message
        showMessage("Authentication failed: " + message, true);
        
        // Play error sound
        audioManager->playSoundEffect(AudioManager::SoundEffect::ERROR);
    }
}

void MPChessClient::onGameStarted(const QJsonObject& gameData) {
    // Play game start sound
    audioManager->playSoundEffect(AudioManager::SoundEffect::GAME_START);
    
    // Switch to game tab
    mainStack->setCurrentIndex(1);
    
    // Reset the board
    boardWidget->resetBoard();
    
    // Set player color
    PieceColor playerColor = gameManager->getPlayerColor();
    boardWidget->setPlayerColor(playerColor);
    
    // Set game ID
    boardWidget->setCurrentGameId(gameManager->getCurrentGameId());
    
    // Update game status
    gameStatusLabel->setText("Game in progress");
    
    // Show message
    QString opponent = (playerColor == PieceColor::WHITE) ? 
        gameData["blackPlayer"].toString() : gameData["whitePlayer"].toString();
    showMessage("Game started against " + opponent);
    
    // Enable board interaction
    boardWidget->setInteractive(true);
}

void MPChessClient::onGameStateUpdated(const QJsonObject& gameState) {
    // Update the board
    updateBoardFromGameState(gameState);
    
    // Update captured pieces
    updateCapturedPieces(gameState);
    
    // Update move history
    updateMoveHistory(gameState);
    
    // Update timers
    updateTimers(gameState);
    
    // Check if in check
    bool isCheck = gameState["isCheck"].toBool();
    if (isCheck) {
        // Get king position
        PieceColor currentTurn = gameState["currentTurn"].toString() == "white" ? 
            PieceColor::WHITE : PieceColor::BLACK;
        
        // Highlight the king
        // Note: In a real implementation, we would need to find the king's position
        // For now, we'll assume it's provided in the game state
        
        // Play check sound
        audioManager->playSoundEffect(AudioManager::SoundEffect::CHECK);
    }
}

void MPChessClient::onGameOver(const QJsonObject& gameOverData) {
    // Get result
    QString result = gameOverData["result"].toString();
    QString reason = gameOverData.contains("reason") ? gameOverData["reason"].toString() : "";
    
    // Update game status
    QString statusText;
    if (result == "white_win") {
        statusText = "White wins";
    } else if (result == "black_win") {
        statusText = "Black wins";
    } else if (result == "draw") {
        statusText = "Draw";
    } else {
        statusText = "Game over";
    }
    
    if (!reason.isEmpty()) {
        statusText += " (" + reason + ")";
    }
    
    gameStatusLabel->setText(statusText);
    
    // Show message
    showMessage("Game over: " + statusText);
    
    // Play game end sound
    audioManager->playSoundEffect(AudioManager::SoundEffect::GAME_END);
    
    // Disable board interaction
    boardWidget->setInteractive(false);
    
    // Request game analysis
    networkManager->requestGameAnalysis(gameManager->getCurrentGameId());
}

void MPChessClient::onMoveResult(bool success, const QString& message) {
    if (!success) {
        // Show error message
        showMessage("Move error: " + message, true);
        
        // Play error sound
        audioManager->playSoundEffect(AudioManager::SoundEffect::ERROR);
    }
}

void MPChessClient::onMoveRecommendationsReceived(const QJsonArray& recommendations) {
    // Update move recommendations
    gameManager->setMoveRecommendations(recommendations);
}

void MPChessClient::onMoveRequested(const QString& gameId, const ChessMove& move) {
    // Check if we're in replay mode
    if (replayMode) {
        return;
    }
    
    // Check if it's our turn
    PieceColor currentTurn = gameManager->getCurrentGameState()["currentTurn"].toString() == "white" ? 
        PieceColor::WHITE : PieceColor::BLACK;
    
    if (currentTurn != gameManager->getPlayerColor()) {
        showMessage("It's not your turn", true);
        return;
    }
    
    // Send move to server via game manager
    gameManager->makeMove(move);
}

void MPChessClient::onSquareClicked(const Position& pos) {
    // Handle square click
    // This is used for selecting pieces and making moves
}

void MPChessClient::onResignClicked() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Resignation",
        "Are you sure you want to resign?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Resign the game
        gameManager->resign();
    }
}

void MPChessClient::onDrawOfferClicked() {
    // Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Draw Offer",
        "Are you sure you want to offer a draw?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Offer a draw
        gameManager->offerDraw();
        
        // Show message
        showMessage("Draw offered to opponent");
    }
}

void MPChessClient::onDrawOfferReceived(const QString& offeredBy) {
    // Ask if the player accepts the draw
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Draw Offer",
        QString("Player %1 offers a draw. Do you accept?").arg(offeredBy),
        QMessageBox::Yes | QMessageBox::No
    );
    
    // Respond to the draw offer
    gameManager->respondToDraw(reply == QMessageBox::Yes);
    
    // Show message
    if (reply == QMessageBox::Yes) {
        showMessage("Draw accepted");
    } else {
        showMessage("Draw declined");
    }
}

void MPChessClient::onDrawResponseReceived(bool accepted) {
    // Show message
    if (accepted) {
        showMessage("Draw offer accepted");
    } else {
        showMessage("Draw offer declined");
    }
}

void MPChessClient::onConnectAction() {
    // Show connect dialog
    bool ok;
    QString host = QInputDialog::getText(
        this, "Connect to Server",
        "Enter server address (host:port):",
        QLineEdit::Normal,
        "localhost:5000",
        &ok
    );
    
    if (ok && !host.isEmpty()) {
        // Parse host and port
        QStringList parts = host.split(":");
        QString hostName = parts[0];
        int port = (parts.size() > 1) ? parts[1].toInt() : 5000;
        
        // Connect to server
        connectToServer(hostName, port);
    }
}

void MPChessClient::onDisconnectAction() {
    // Disconnect from server
    disconnectFromServer();
}

void MPChessClient::onSettingsAction() {
    // Show settings dialog
    SettingsDialog dialog(themeManager, audioManager, this);
    
    connect(&dialog, &SettingsDialog::settingsChanged, this, &MPChessClient::updateTheme);
    
    dialog.exec();
}

void MPChessClient::onExitAction() {
    // Close the application
    close();
}

void MPChessClient::onFlipBoardAction() {
    // Flip the board
    boardWidget->setFlipped(!boardWidget->isFlipped());
}

void MPChessClient::onShowAnalysisAction() {
    // Toggle analysis widget visibility
    bool visible = analysisWidget->isVisible();
    analysisWidget->setVisible(!visible);
}

void MPChessClient::onShowChatAction() {
    // Toggle chat visibility
    bool visible = chatDisplay->isVisible() && chatInput->isVisible();
    chatDisplay->setVisible(!visible);
    chatInput->setVisible(!visible);
}

void MPChessClient::onAboutAction() {
    // Show about dialog
    QMessageBox::about(
        this,
        "About Chess Client",
        "Chess Client\n\n"
        "A multiplayer chess client that connects to the MPChessServer.\n\n"
        "Version 1.0.0\n"
        "© 2023 Chess Client Team"
    );
}

void MPChessClient::onHomeTabSelected() {
    mainStack->setCurrentIndex(0);
}

void MPChessClient::onPlayTabSelected() {
    mainStack->setCurrentIndex(1);
}

void MPChessClient::onAnalysisTabSelected() {
    mainStack->setCurrentIndex(2);
}

void MPChessClient::onProfileTabSelected() {
    mainStack->setCurrentIndex(3);
}

void MPChessClient::onLeaderboardTabSelected() {
    mainStack->setCurrentIndex(4);
    
    // Request leaderboard
    networkManager->requestLeaderboard();
}

void MPChessClient::onMatchmakingStatusReceived(const QJsonObject& statusData) {
    // Update matchmaking widget
    matchmakingWidget->setMatchmakingStatus(statusData);
}

void MPChessClient::onRequestMatchmaking(bool join, TimeControlType timeControl) {
    // Request matchmaking
    networkManager->requestMatchmaking(join, timeControl);
}

void MPChessClient::onGameHistoryReceived(const QJsonArray& gameHistory) {
    // Update game history widget
    gameHistoryWidget->setGameHistoryData(gameHistory);
}

void MPChessClient::onGameAnalysisReceived(const QJsonObject& analysis) {
    // Update analysis widget
    analysisWidget->setAnalysisData(analysis);
}

void MPChessClient::onGameSelected(const QString& gameId) {
    // Request game analysis
    networkManager->requestGameAnalysis(gameId);
    
    // Switch to analysis tab
    mainStack->setCurrentIndex(2);
}

void MPChessClient::onRequestGameHistory() {
    // Request game history
    networkManager->requestGameHistory();
}

void MPChessClient::onRequestGameAnalysis(bool stockfish) {
    // Request game analysis
    QJsonObject data;
    data["gameId"] = gameManager->getCurrentGameId();
    data["stockfish"] = stockfish;
    
    networkManager->requestGameAnalysis(gameManager->getCurrentGameId());
}

void MPChessClient::onLeaderboardReceived(const QJsonObject& leaderboard) {
    // Update leaderboard widget
    leaderboardWidget->setLeaderboardData(leaderboard);
    
    // Update player ranks
    if (leaderboard.contains("yourRanks")) {
        leaderboardWidget->setPlayerRanks(leaderboard["yourRanks"].toObject());
    }
}

void MPChessClient::onRequestLeaderboard(bool allPlayers) {
    // Request leaderboard
    networkManager->requestLeaderboard(allPlayers);
}

void MPChessClient::setupUI() {
    // Set window title
    setWindowTitle("Chess Client");
    
    // Set window icon
    setWindowIcon(QIcon(":/icons/app_icon.png"));
    
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Create main stack
    mainStack = new QStackedWidget(centralWidget);
    
    // Create tabs for navigation
    QTabBar* tabBar = new QTabBar(centralWidget);
    tabBar->addTab("Home");
    tabBar->addTab("Play");
    tabBar->addTab("Analysis");
    tabBar->addTab("Profile");
    tabBar->addTab("Leaderboard");
    
    // Connect tab signals
    connect(tabBar, &QTabBar::currentChanged, this, [this](int index) {
        switch (index) {
            case 0: onHomeTabSelected(); break;
            case 1: onPlayTabSelected(); break;
            case 2: onAnalysisTabSelected(); break;
            case 3: onProfileTabSelected(); break;
            case 4: onLeaderboardTabSelected(); break;
        }
    });
    
    // Create home page
    QWidget* homePage = new QWidget();
    QVBoxLayout* homeLayout = new QVBoxLayout(homePage);
    
    QLabel* welcomeLabel = new QLabel("Welcome to Chess Client", homePage);
    QFont welcomeFont = welcomeLabel->font();
    welcomeFont.setPointSize(welcomeFont.pointSize() + 6);
    welcomeFont.setBold(true);
    welcomeLabel->setFont(welcomeFont);
    welcomeLabel->setAlignment(Qt::AlignCenter);
    
    matchmakingWidget = new MatchmakingWidget(homePage);
    
    homeLayout->addWidget(welcomeLabel);
    homeLayout->addWidget(matchmakingWidget);
    homeLayout->addStretch();
    
    // Create game page
    QWidget* gamePage = new QWidget();
    QVBoxLayout* gameLayout = new QVBoxLayout(gamePage);
    
    // Create game UI
    createGameUI();
    
    // Create analysis page
    QWidget* analysisPage = new QWidget();
    QVBoxLayout* analysisLayout = new QVBoxLayout(analysisPage);
    
    gameHistoryWidget = new GameHistoryWidget(analysisPage);
    analysisWidget = new AnalysisWidget(analysisPage);
    
    analysisLayout->addWidget(gameHistoryWidget);
    analysisLayout->addWidget(analysisWidget);
    
    // Create profile page
    QWidget* profilePage = new QWidget();
    QVBoxLayout* profileLayout = new QVBoxLayout(profilePage);
    
    profileWidget = new ProfileWidget(profilePage);
    
    profileLayout->addWidget(profileWidget);
    
    // Create leaderboard page
    QWidget* leaderboardPage = new QWidget();
    QVBoxLayout* leaderboardLayout = new QVBoxLayout(leaderboardPage);
    
    leaderboardWidget = new LeaderboardWidget(leaderboardPage);
    
    leaderboardLayout->addWidget(leaderboardWidget);
    
    // Add pages to stack
    mainStack->addWidget(homePage);
    mainStack->addWidget(gamePage);
    mainStack->addWidget(analysisPage);
    mainStack->addWidget(profilePage);
    mainStack->addWidget(leaderboardPage);
    
    // Add widgets to main layout
    mainLayout->addWidget(tabBar);
    mainLayout->addWidget(mainStack);
    
    // Create menus
    createMenus();
    
    // Create status bar
    createStatusBar();
    
    // Connect signals
    connect(matchmakingWidget, &MatchmakingWidget::requestMatchmaking, this, &MPChessClient::onRequestMatchmaking);
    connect(gameHistoryWidget, &GameHistoryWidget::gameSelected, this, &MPChessClient::onGameSelected);
    connect(gameHistoryWidget, &GameHistoryWidget::requestGameHistory, this, &MPChessClient::onRequestGameHistory);
    connect(analysisWidget, &AnalysisWidget::requestAnalysis, this, &MPChessClient::onRequestGameAnalysis);
    connect(leaderboardWidget, &LeaderboardWidget::requestAllPlayers, this, &MPChessClient::onRequestLeaderboard);
    
    // Set initial size
    resize(1024, 768);
}

void MPChessClient::createMenus() {
    // Create file menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* connectAction = fileMenu->addAction("&Connect to Server...");
    connectAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(connectAction, &QAction::triggered, this, &MPChessClient::onConnectAction);
    
    QAction* disconnectAction = fileMenu->addAction("&Disconnect");
    disconnectAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(disconnectAction, &QAction::triggered, this, &MPChessClient::onDisconnectAction);
    
    fileMenu->addSeparator();
    
    QAction* settingsAction = fileMenu->addAction("&Settings...");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MPChessClient::onSettingsAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Alt+F4"));
    connect(exitAction, &QAction::triggered, this, &MPChessClient::onExitAction);
    
    // Create game menu
    QMenu* gameMenu = menuBar()->addMenu("&Game");
    
    QAction* flipBoardAction = gameMenu->addAction("&Flip Board");
    flipBoardAction->setShortcut(QKeySequence("F"));
    connect(flipBoardAction, &QAction::triggered, this, &MPChessClient::onFlipBoardAction);
    
    QAction* showAnalysisAction = gameMenu->addAction("Show &Analysis");
    showAnalysisAction->setShortcut(QKeySequence("A"));
    showAnalysisAction->setCheckable(true);
    showAnalysisAction->setChecked(true);
    connect(showAnalysisAction, &QAction::triggered, this, &MPChessClient::onShowAnalysisAction);
    
    QAction* showChatAction = gameMenu->addAction("Show &Chat");
    showChatAction->setShortcut(QKeySequence("C"));
    showChatAction->setCheckable(true);
    showChatAction->setChecked(true);
    connect(showChatAction, &QAction::triggered, this, &MPChessClient::onShowChatAction);
    
    // Create help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MPChessClient::onAboutAction);
}

void MPChessClient::createStatusBar() {
    // Create status labels
    connectionStatusLabel = new QLabel("Disconnected");
    connectionStatusLabel->setStyleSheet("color: red;");
    
    gameStatusLabel = new QLabel("No active game");
    
    // Add labels to status bar
    statusBar()->addWidget(connectionStatusLabel);
    statusBar()->addWidget(gameStatusLabel, 1);
}

void MPChessClient::createGameUI() {
    // Create game layout
    QWidget* gameWidget = mainStack->widget(1);
    QVBoxLayout* gameLayout = qobject_cast<QVBoxLayout*>(gameWidget->layout());
    
    // Create main game area
    QSplitter* gameSplitter = new QSplitter(Qt::Horizontal, gameWidget);
    
    // Create board widget
    boardWidget = new ChessBoardWidget(themeManager, audioManager, gameSplitter);
    boardWidget->setMinimumSize(400, 400);
    
    // Create side panel
    QWidget* sidePanel = new QWidget(gameSplitter);
    QVBoxLayout* sidePanelLayout = new QVBoxLayout(sidePanel);
    
    // Create captured pieces widget
    capturedPiecesWidget = new CapturedPiecesWidget(themeManager, sidePanel);
    
    // Create move history widget
    moveHistoryWidget = new MoveHistoryWidget(sidePanel);
    
    // Create game timer widget
    gameTimerWidget = new GameTimerWidget(sidePanel);
    
    // Create game controls
    QHBoxLayout* gameControlsLayout = new QHBoxLayout();
    
    QPushButton* resignButton = new QPushButton("Resign", sidePanel);
    QPushButton* drawButton = new QPushButton("Offer Draw", sidePanel);
    
    gameControlsLayout->addWidget(resignButton);
    gameControlsLayout->addWidget(drawButton);
    
    // Create chat area
    chatDisplay = new QTextEdit(sidePanel);
    chatDisplay->setReadOnly(true);
    
    chatInput = new QLineEdit(sidePanel);
    chatInput->setPlaceholderText("Type a message...");
    
    // Add widgets to side panel
    sidePanelLayout->addWidget(capturedPiecesWidget);
    sidePanelLayout->addWidget(gameTimerWidget);
    sidePanelLayout->addLayout(gameControlsLayout);
    sidePanelLayout->addWidget(moveHistoryWidget);
    sidePanelLayout->addWidget(chatDisplay);
    sidePanelLayout->addWidget(chatInput);
    
    // Set splitter sizes
    gameSplitter->addWidget(boardWidget);
    gameSplitter->addWidget(sidePanel);
    gameSplitter->setStretchFactor(0, 3);
    gameSplitter->setStretchFactor(1, 1);
    
    // Add game splitter to game layout
    gameLayout->addWidget(gameSplitter);
    
    // Connect signals
    connect(boardWidget, &ChessBoardWidget::moveRequested, this, &MPChessClient::onMoveRequested);
    connect(boardWidget, &ChessBoardWidget::squareClicked, this, &MPChessClient::onSquareClicked);
    connect(resignButton, &QPushButton::clicked, this, &MPChessClient::onResignClicked);
    connect(drawButton, &QPushButton::clicked, this, &MPChessClient::onDrawOfferClicked);
}

void MPChessClient::updateBoardFromGameState(const QJsonObject& gameState) {
    // Get board data
    QJsonArray boardArray = gameState["board"].toArray();
    
    // Clear the board
    boardWidget->resetBoard();
    
    // Set pieces
    for (int r = 0; r < 8; ++r) {
        QJsonArray rowArray = boardArray[r].toArray();
        for (int c = 0; c < 8; ++c) {
            QJsonObject pieceObj = rowArray[c].toObject();
            QString type = pieceObj["type"].toString();
            QString color = pieceObj["color"].toString();
            
            if (type != "empty") {
                PieceType pieceType;
                if (type == "pawn") pieceType = PieceType::PAWN;
                else if (type == "knight") pieceType = PieceType::KNIGHT;
                else if (type == "bishop") pieceType = PieceType::BISHOP;
                else if (type == "rook") pieceType = PieceType::ROOK;
                else if (type == "queen") pieceType = PieceType::QUEEN;
                else if (type == "king") pieceType = PieceType::KING;
                else continue;
                
                PieceColor pieceColor = (color == "white") ? PieceColor::WHITE : PieceColor::BLACK;
                
                boardWidget->setPiece(Position(r, c), pieceType, pieceColor);
            }
        }
    }
    
    // Highlight last move if available
    if (gameState.contains("moveHistory") && !gameState["moveHistory"].toArray().isEmpty()) {
        QJsonArray moveHistory = gameState["moveHistory"].toArray();
        QJsonObject lastMove = moveHistory.last().toObject();
        
        QString from = lastMove["from"].toString();
        QString to = lastMove["to"].toString();
        
        Position fromPos = Position::fromAlgebraic(from);
        Position toPos = Position::fromAlgebraic(to);
        
        boardWidget->highlightLastMove(fromPos, toPos);
    }
    
    // Highlight check if in check
    if (gameState["isCheck"].toBool()) {
        // Find the king of the current player
        PieceColor currentTurn = gameState["currentTurn"].toString() == "white" ? 
            PieceColor::WHITE : PieceColor::BLACK;
        
        // In a real implementation, we would need to find the king's position
        // For now, we'll just highlight the king if we find it
        for (int r = 0; r < 8; ++r) {
            QJsonArray rowArray = boardArray[r].toArray();
            for (int c = 0; c < 8; ++c) {
                QJsonObject pieceObj = rowArray[c].toObject();
                QString type = pieceObj["type"].toString();
                QString color = pieceObj["color"].toString();
                
                if (type == "king" && 
                    ((color == "white" && currentTurn == PieceColor::WHITE) ||
                     (color == "black" && currentTurn == PieceColor::BLACK))) {
                    boardWidget->highlightCheck(Position(r, c));
                    break;
                }
            }
        }
    }
}

void MPChessClient::updateCapturedPieces(const QJsonObject& gameState) {
    // Clear captured pieces
    capturedPiecesWidget->clear();
    
    // Get captured pieces
    QJsonArray whiteCaptured = gameState["whiteCaptured"].toArray();
    QJsonArray blackCaptured = gameState["blackCaptured"].toArray();
    
    // Add white captured pieces
    for (const QJsonValue& value : whiteCaptured) {
        QString type = value.toString();
        
        PieceType pieceType;
        if (type == "pawn") pieceType = PieceType::PAWN;
        else if (type == "knight") pieceType = PieceType::KNIGHT;
        else if (type == "bishop") pieceType = PieceType::BISHOP;
        else if (type == "rook") pieceType = PieceType::ROOK;
        else if (type == "queen") pieceType = PieceType::QUEEN;
        else continue;
        
        capturedPiecesWidget->addCapturedPiece(pieceType, PieceColor::WHITE);
    }
    
    // Add black captured pieces
    for (const QJsonValue& value : blackCaptured) {
        QString type = value.toString();
        
        PieceType pieceType;
        if (type == "pawn") pieceType = PieceType::PAWN;
        else if (type == "knight") pieceType = PieceType::KNIGHT;
        else if (type == "bishop") pieceType = PieceType::BISHOP;
        else if (type == "rook") pieceType = PieceType::ROOK;
        else if (type == "queen") pieceType = PieceType::QUEEN;
        else continue;
        
        capturedPiecesWidget->addCapturedPiece(pieceType, PieceColor::BLACK);
    }
}

void MPChessClient::updateMoveHistory(const QJsonObject& gameState) {
    // Clear move history
    moveHistoryWidget->clear();
    
    // Get move history
    QJsonArray moveHistory = gameState["moveHistory"].toArray();
    
    // Add moves to history
    int moveNumber = 1;
    QString whiteMoveNotation;
    QString blackMoveNotation;
    
    for (int i = 0; i < moveHistory.size(); ++i) {
        QJsonObject moveObj = moveHistory[i].toObject();
        
        // In a real implementation, we would convert the move to standard notation
        // For now, we'll just use the algebraic notation
        QString moveNotation = moveObj["from"].toString() + moveObj["to"].toString();
        
        if (i % 2 == 0) {
            // White's move
            whiteMoveNotation = moveNotation;
            
            if (i == moveHistory.size() - 1) {
                // Last move is white's move
                moveHistoryWidget->addMove(moveNumber, whiteMoveNotation);
            }
        } else {
            // Black's move
            blackMoveNotation = moveNotation;
            
            // Add the move pair
            moveHistoryWidget->addMove(moveNumber, whiteMoveNotation, blackMoveNotation);
            
            // Increment move number
            moveNumber++;
            
            // Reset move notations
            whiteMoveNotation.clear();
            blackMoveNotation.clear();
        }
    }
}

void MPChessClient::updateTimers(const QJsonObject& gameState) {
    // Get time data
    qint64 whiteTime = gameState["whiteRemainingTime"].toInt();
    qint64 blackTime = gameState["blackRemainingTime"].toInt();
    QString currentTurn = gameState["currentTurn"].toString();
    
    // Update timers
    gameTimerWidget->setWhiteTime(whiteTime);
    gameTimerWidget->setBlackTime(blackTime);
    gameTimerWidget->setActiveColor(currentTurn == "white" ? PieceColor::WHITE : PieceColor::BLACK);
    
    // Start timer if game is active
    if (gameManager->isGameActive()) {
        gameTimerWidget->start();
    } else {
        gameTimerWidget->stop();
    }
}

void MPChessClient::showLoginDialog() {
    // Create login dialog if it doesn't exist
    if (!loginDialog) {
        loginDialog = new LoginDialog(this);
        connect(loginDialog, &LoginDialog::loginRequested, networkManager, &NetworkManager::authenticate);
    }
    
    // Show the dialog
    loginDialog->exec();
}

void MPChessClient::showMessage(const QString& message, bool error) {
    // Show message in status bar
    statusBar()->showMessage(message, 5000);
    
    // Log the message
    if (error) {
        logger->error(message);
    } else {
        logger->info(message);
    }
}

void MPChessClient::enterReplayMode(const QVector<ChessMove>& moves) {
    replayMode = true;
    currentReplayIndex = -1;
    
    // Disable board interaction
    boardWidget->setInteractive(false);
    
    // Reset the board
    boardWidget->resetBoard();
    boardWidget->setupInitialPosition();
    
    // Enable replay controls
    replaySlider->setEnabled(true);
    replayPrevButton->setEnabled(true);
    replayPlayButton->setEnabled(true);
    replayNextButton->setEnabled(true);
    
    // Set slider range
    replaySlider->setRange(-1, moves.size() - 1);
    replaySlider->setValue(-1);
    
    updateReplayControls();
}

void MPChessClient::exitReplayMode() {
    replayMode = false;
    
    // Enable board interaction if game is active
    boardWidget->setInteractive(gameManager->isGameActive());
    
    // Disable replay controls
    replaySlider->setEnabled(false);
    replayPrevButton->setEnabled(false);
    replayPlayButton->setEnabled(false);
    replayNextButton->setEnabled(false);
    
    // Update the board from current game state
    updateBoardFromGameState(gameManager->getCurrentGameState());
}

void MPChessClient::updateReplayControls() {
    // Update slider
    replaySlider->setValue(currentReplayIndex);
    
    // Update buttons
    replayPrevButton->setEnabled(currentReplayIndex > -1);
    replayNextButton->setEnabled(currentReplayIndex < replaySlider->maximum());
}

void MPChessClient::saveSettings() {
    QSettings settings;
    
    // Save window geometry
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    
    // Save server settings
    // (These would be saved when connecting to a server)
    
    // Theme settings are saved by the ThemeManager
    // Audio settings are saved by the AudioManager
}

void MPChessClient::loadSettings() {
    QSettings settings;
    
    // Load window geometry
    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
    }
    
    if (settings.contains("window/state")) {
        restoreState(settings.value("window/state").toByteArray());
    }
    
    // Load server settings
    // (These would be loaded when showing the connect dialog)
    
    // Theme settings are loaded by the ThemeManager
    // Audio settings are loaded by the AudioManager
}

void MPChessClient::updateTheme() {
    // Apply theme to the application
    setStyleSheet(themeManager->getStyleSheet());
    
    // Update board theme
    boardWidget->updateTheme();
    
    // Update captured pieces widget
    capturedPiecesWidget->updateTheme();
}

// Main function
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application information
    QApplication::setApplicationName("Chess Client");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("Chess Client Team");
    QApplication::setOrganizationDomain("chessclient.example.com");
    
    // Create main window
    MPChessClient window;
    window.show();
    
    // Connect to server
    window.connectToServer("localhost", 5000);
    
    return app.exec();
}