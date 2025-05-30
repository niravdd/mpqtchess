// src/gui/ChessBoardView.cpp
#include "ChessBoardView.h"
#include "../util/ThemeManager.h"
#include <QtGui/QResizeEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>

ChessBoardView::ChessBoardView(QWidget* parent, NetworkClient* networkClient)
    : QGraphicsView(parent)
    , scene_(new QGraphicsScene(this))
    , game_(std::make_unique<ChessGame>())
    , selectedPiece_(nullptr)
    , currentTheme_(Settings::getInstance().getCurrentTheme())
    , animationsEnabled_(Settings::getInstance().getAnimationsEnabled())
    , soundEnabled_(Settings::getInstance().isSoundEnabled())
    , networkClient_(networkClient)
    , selectedSquare_(-1, -1)
    , gameOver_(false)
    , playerColor_(PieceColor::None)
{
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    setupBoard();
    updateBoard();

    if (networkClient_) {
        // Connection status signals
        connect(networkClient_, &NetworkClient::connected,
                this, &ChessBoardView::onConnected);
        connect(networkClient_, &NetworkClient::disconnected,
                this, &ChessBoardView::onDisconnected);
        connect(networkClient_, &NetworkClient::rawDataReceived,
                this, &ChessBoardView::handleNetworkData);
        connect(networkClient_, &NetworkClient::parsedMoveReceived,
                this, &ChessBoardView::handleParsedMove);
        connect(networkClient_, &NetworkClient::errorOccurred,
                this, &ChessBoardView::onNetworkError);
    }

#ifdef QT_DEBUG
    // Enable core dumps or more detailed crash reporting
    signal(SIGSEGV, [](int) {
        qDebug() << "Segmentation fault occurred!";
        abort();
    });
#endif
}

void ChessBoardView::setupBoard()
{
    const int BOARD_SIZE = 8;
    qreal squareSize = 80; // Initial size, will be adjusted in resizeEvent

    const ThemeManager::ThemeConfig& theme = 
        ThemeManager::getInstance().getCurrentTheme();

    // Clear existing board items first
    for (auto item : scene_->items()) {
        if (dynamic_cast<QGraphicsRectItem*>(item) && 
            !dynamic_cast<ChessPieceItem*>(item)) {
            scene_->removeItem(item);
            delete item;
        }
    }

    // Determine board orientation based on player color
    bool isWhiteAtBottom = (playerColor_ == PieceColor::White || playerColor_ == PieceColor::None);
    
    qDebug() << "from ChessBoardView::setupBoard(): Setting up board with orientation: " 
             << (isWhiteAtBottom ? "White at bottom" : "Black at bottom")
             << " - Player color is: " << (playerColor_ == PieceColor::White ? "White" : 
                                          (playerColor_ == PieceColor::Black ? "Black" : "None"));

    // Create squares
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            // Calculate visual position based on player perspective
            int visualRow = isWhiteAtBottom ? row : (BOARD_SIZE - 1 - row);
            int visualCol = isWhiteAtBottom ? col : (BOARD_SIZE - 1 - col);
            
            QGraphicsRectItem* square = scene_->addRect(
                visualCol * squareSize, visualRow * squareSize,
                squareSize, squareSize);
            
            bool isLight = (row + col) % 2 == 0;
            square->setBrush(isLight ? theme.colors.lightSquares 
                                   : theme.colors.darkSquares);
            square->setPen(QPen(theme.colors.border));
            
            // Create highlight overlay
            highlightItems_[row][col] = scene_->addRect(
                visualCol * squareSize, visualRow * squareSize,
                squareSize, squareSize);
            highlightItems_[row][col]->setBrush(::Qt::transparent);
            highlightItems_[row][col]->setPen(::Qt::NoPen);
            highlightItems_[row][col]->setZValue(1);
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
    fitInView(scene_->sceneRect(), ::Qt::KeepAspectRatio);
    
    // Update board squares and pieces
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            // Find the square for this position
            for (auto item : scene_->items(QPointF(col * squareSize + squareSize/2,
                                                 row * squareSize + squareSize/2))) {
                if (QGraphicsRectItem* square = dynamic_cast<QGraphicsRectItem*>(item)) {
                    if (!dynamic_cast<ChessPieceItem*>(square) && square != highlightItems_[row][col]) {
                        square->setRect(col * squareSize, row * squareSize, 
                                      squareSize, squareSize);
                    }
                }
            }
            
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
    // Convert mouse position to board coordinates
    QPointF scenePos = mapToScene(event->pos());
    QPoint boardPos = boardPositionAt(scenePos);

    qDebug() << "Mouse Press: Scene Pos" << scenePos 
             << "Board Pos" << boardPos 
             << "Player Color" << (playerColor_ == PieceColor::White ? "White" : "Black")
             << "Current Turn" << (game_->getCurrentTurn() == PieceColor::White ? "White" : "Black");

    // Validate board position
    if (!game_->isValidPosition(boardPos)) {
        QDebug(QtMsgType::QtWarningMsg) << "Invalid board position: " << boardPos;
        return;
    }
    
    // Get the piece at the clicked position
    auto piece = game_->getPieceAt(boardPos);
    
    // Check if piece exists and it's the current player's turn
    if (piece && piece->getColor() == game_->getCurrentTurn()) {
        // Only allow moving pieces of your own color in network games
        if (networkClient_ && networkClient_->isConnected()) {
            if (piece->getColor() != playerColor_) {
                qDebug() << "Cannot move opponent's pieces in network game";
                return;
            }
        }
        
        // Find the corresponding ChessPieceItem
        for (auto item : scene_->items(scenePos)) {
            if (ChessPieceItem* pieceItem = dynamic_cast<ChessPieceItem*>(item)) {
                selectedPiece_ = pieceItem;
                dragStartPos_ = scenePos;
                selectedSquare_ = boardPos;

                qDebug() << "Piece Selected: " 
                         << (piece->getColor() == PieceColor::White ? "White" : "Black")
                         << "Piece Type: " << pieceTypeToString(piece->getType())
                         << "at position" << boardPos.x() << "," << boardPos.y();
                
                // Bring the piece to front and highlight legal moves
                selectedPiece_->setZValue(2);
                highlightLegalMoves(boardPos);
                break;
            }
        }
    } else {
        qDebug() << "No valid piece to move at position" << boardPos.x() << "," << boardPos.y();
        if (piece) {
            qDebug() << "Piece color:" << (piece->getColor() == PieceColor::White ? "White" : "Black")
                     << "Current turn:" << (game_->getCurrentTurn() == PieceColor::White ? "White" : "Black");
        }
    }
}

void ChessBoardView::mouseMoveEvent(QMouseEvent* event)
{
    if (selectedPiece_) {
        // Map mouse position to scene coordinates
        QPointF scenePos = mapToScene(event->pos());
        
        // Center the piece on the mouse cursor
        QPointF offset(selectedPiece_->boundingRect().width() / 2, 
                       selectedPiece_->boundingRect().height() / 2);
        selectedPiece_->setPos(scenePos - offset);
    }
}

void ChessBoardView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!selectedPiece_)
    {
        return;
    }
    try
    {    
        // Convert start and end positions to board coordinates
        QPoint fromPos = boardPositionAt(dragStartPos_);
        QPoint toPos = boardPositionAt(mapToScene(event->pos()));

        // Extensive validation before move
        if (!game_)
        {
            qDebug() << "from ChessBoardView::mouseReleaseEvent(): Game logic is null!";
            return;
        }

        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Move Attempt: From" << fromPos << "To" << toPos
                 << "Current Player" << ((game_->getCurrentPlayer() == PieceColor::White) ? "White" : "Black");

        // Validate and attempt to make the move
        if (game_->isValidMove(fromPos, toPos, game_->getCurrentPlayer()))
        {
            if (game_->makeMove(fromPos, toPos, game_->getCurrentPlayer()))
            {
                // Update board and generate move notation
                updateBoard();
                QString moveNotation = generateMoveNotation(fromPos, toPos);
                emit moveCompleted(moveNotation);

                qDebug() << "from ChessBoardView::mouseReleaseEvent(): Move Successful: " << moveNotation;
                
                // Store game state information BEFORE network operations
                bool isInCheckmate = false;
                bool isInStalemate = false;
                PieceColor currentTurn = PieceColor::None;
                
                // Save game state that we might need after network operations
                if (game_) {
                    isInCheckmate = game_->isCheckmate(game_->getCurrentTurn());
                    isInStalemate = game_->isStalemate(game_->getCurrentTurn());
                    currentTurn = game_->getCurrentTurn();
                }

                // Send move over network if connected
                if (networkClient_ && networkClient_->isConnected())
                {
                    QString fromSquare = positionToAlgebraic(fromPos);
                    QString toSquare = positionToAlgebraic(toPos);

                    qDebug() << "from ChessBoardView::mouseReleaseEvent(): fromSquare: " << fromSquare << " and toSquare: " << toSquare;

                    try
                    {
                        // Add safety check before sending move
                        if (!fromSquare.isEmpty() && !toSquare.isEmpty())
                        {
                            networkClient_->sendMove(fromSquare, toSquare);
                            qDebug() << "from ChessBoardView::mouseReleaseEvent(): Move sent to server successfully";
                        }
                        else
                        {
                            qDebug() << "from ChessBoardView::mouseReleaseEvent(): Error: Invalid algebraic notation generated";
                        }
                    }
                    catch (const std::exception& e)
                    {
                        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Exception sending move:" << e.what();
                    }
                    catch (...)
                    {
                        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Unknown exception sending move";
                    }
                }

                if(game_)
                {
                    // Check for game-ending conditions
                    if (game_->isCheckmate(game_->getCurrentTurn()))
                    {
                        emit gameOver(tr("Checkmate! %1 wins!")
                            .arg(game_->getCurrentTurn() == PieceColor::White ? 
                                "Black" : "White"));
                    }
                    else if (game_->isStalemate(game_->getCurrentTurn()))
                    {
                        emit gameOver(tr("Stalemate! Game is drawn."));
                    }
                    else
                    {
                        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Not checkmate, not stalemate...";
                    }
                }
                else
                {
                    qDebug() << "from ChessBoardView::mouseReleaseEvent(): WARNING: Game became invalid after network operations!";
                }
            }
            else
            {
                qDebug() << "from ChessBoardView::mouseReleaseEvent(): Move failed in game logic";

                // Invalid move, return piece to original position
                selectedPiece_->setPos(dragStartPos_);
            }
        } else
        {
            qDebug() << "from ChessBoardView::mouseReleaseEvent(): Invalid move validation";

            // Move not valid, return piece to original position
            selectedPiece_->setPos(dragStartPos_);
        }
        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Outer try{}";
    }
    catch (const std::exception& e)
    {
        qDebug() << "Exception in mouseReleaseEvent:" << e.what();
    }
    catch (...)
    {
        qDebug() << "Unknown exception in mouseReleaseEvent";
    }

    qDebug() << "from ChessBoardView::mouseReleaseEvent(): if(selectedPiece_) is valid?";
    // Always reset piece selection and highlights
    if (selectedPiece_)
    {
        qDebug() << "from ChessBoardView::mouseReleaseEvent(): Inside if(selectedPiece_) is valid";
        
        selectedPiece_->setZValue(1);
        selectedPiece_ = nullptr;
    }

    qDebug() << "from ChessBoardView::mouseReleaseEvent(): Before clearHighlights...";
    clearHighlights();
}

void ChessBoardView::updatePlayerStatusLabel()
{
    // Assuming you have a QLabel in your control panel or game interface
    QString playerColorStr = (playerColor_ == PieceColor::White) ? "White" : "Black";
    QString currentTurnStr = (game_->getCurrentTurn() == PieceColor::White) ? "White" : "Black";
    
    QString statusText = QString("You are playing %1 | Current Turn: %2")
        .arg(playerColorStr)
        .arg(currentTurnStr);
    
    // You'll need to connect this to your game control panel
    emit updateStatusLabel(statusText);
}

QString ChessBoardView::positionToAlgebraic(const QPoint& pos) const
{
    if (pos.x() < 0 || pos.x() > 7 || pos.y() < 0 || pos.y() > 7)
        return "";
        
    char file = 'a' + pos.x();
    char rank = '8' - pos.y();  // Assuming 0,0 is top-left (a8)
    
    return QString(file) + QString(rank);
}

QPoint ChessBoardView::algebraicToPosition(const QString& algebraic) const
{
    if (algebraic.length() != 2)
        return QPoint(-1, -1);
        
    int x = algebraic[0].toLatin1() - 'a';
    int y = '8' - algebraic[1].toLatin1();  // Assuming 0,0 is top-left (a8)
    
    if (x < 0 || x > 7 || y < 0 || y > 7)
        return QPoint(-1, -1);
        
    return QPoint(x, y);
}

void ChessBoardView::receiveNetworkMove(const QString& fromSquare, const QString& toSquare)
{
    try
    {
        QPoint fromPos = algebraicToPosition(fromSquare);
        QPoint toPos = algebraicToPosition(toSquare);
        
        if (fromPos.x() < 0 || toPos.x() < 0)
        {
            qDebug() << "from ChessBoardView::receiveNetworkMove(): Invalid network move positions: (from) - " << fromSquare << ", (to) - " << toSquare;

            return;  // Invalid positions
        }

        qDebug() << "from ChessBoardView::receiveNetworkMove(): Received opponent move: (from) - " << fromSquare << ", (to) - " << toSquare
                    << "... Translated to: (from) - " << fromPos << ", (to) - " << toPos;

        // Determine current turn
        PieceColor currentColor = game_->getCurrentTurn();
    
        qDebug() << "from ChessBoardView::receiveNetworkMove(): game_->getCurrentTurn(): " << ((game_->getCurrentTurn() == PieceColor::White) ? "White" : "Black")
                    << " - and - game_->getCurrentPlayer(): " << ((game_->getCurrentPlayer() == PieceColor::White) ? "White" : "Black");

        // Execute the move from the network
        if (game_->isValidMove(fromPos, toPos, game_->getCurrentPlayer()))
        {
            if (game_->makeMove(fromPos, toPos, game_->getCurrentPlayer()))
            {
                updateBoard();
                emit moveCompleted(generateMoveNotation(fromPos, toPos));

                // Check game state
                if (game_->isCheckmate(game_->getCurrentTurn()))
                {
                    emit gameOver(tr("Checkmate! %1 wins!")
                        .arg(game_->getCurrentTurn() == PieceColor::White ? "Black" : "White"));
                } else if (game_->isStalemate(game_->getCurrentTurn()))
                {
                    emit gameOver(tr("Stalemate! Game is drawn."));
                }
            }
            else
            {
                qDebug() << "from ChessBoardView::receiveNetworkMove(): Failed to execute opponent's move: (from) - " << fromSquare << ", (to) - " << toSquare;
            }
        }
        else
        {
            qDebug() << "from ChessBoardView::receiveNetworkMove(): Invalid opponent move received: (from) - " << fromSquare << ", (to) - " << toSquare;
        }
    }
    catch (const std::exception& e)
    {
        qDebug() << "Exception in receiveNetworkMove:" << e.what();
    }
    catch (...)
    {
        qDebug() << "Unknown exception in receiveNetworkMove";
    }
}

void ChessBoardView::receiveNetworkMove(int fromCol, int fromRow, int toCol, int toRow)
{
    handleParsedMove(fromCol, fromRow, toCol, toRow);
}

void ChessBoardView::highlightLegalMoves(const QPoint& pos)
{
    clearHighlights();
    auto legalMoves = game_->getLegalMoves(pos);
    
    const ThemeManager::ThemeConfig& theme = 
    ThemeManager::getInstance().getCurrentTheme();

    for (const auto& move : legalMoves) {
        highlightItems_[move.row][move.col]->setBrush(
            theme.colors.highlightMove);
    }
}

QPoint ChessBoardView::boardPositionAt(const QPointF& pos) const
{
    qreal squareSize = scene_->width() / 8;
    int col = static_cast<int>(pos.x() / squareSize);
    int row = static_cast<int>(pos.y() / squareSize);

    qDebug() << "from ChessBoardView::boardPositionAt(): Visual Pos: " << QPoint(col, row)
             << "Player Color: " << (playerColor_ == PieceColor::White ? "White" : 
                                    (playerColor_ == PieceColor::Black ? "Black" : "None"));

    // Adjust for player perspective
    bool isBlackPlayer = (playerColor_ == PieceColor::Black);
    if (isBlackPlayer)
    {
        // For black player, the board is flipped, so we need to invert coordinates
        col = 7 - col;
        row = 7 - row;

        qDebug() << "from ChessBoardView::boardPositionAt(): Logical Pos for Black Orientation: " << QPoint(col, row)
                    << "Player Color: Black";
    }
        
    return QPoint(col, row);
}

void ChessBoardView::updateBoard()
{
    qDebug() << "from ChessBoardView::updateBoard(): Entered with playerColor =" 
             << (playerColor_ == PieceColor::White ? "White" : 
                (playerColor_ == PieceColor::Black ? "Black" : "None"));
    
    // Clear existing pieces
    for (auto item : scene_->items()) {
        if (dynamic_cast<ChessPieceItem*>(item)) {
            scene_->removeItem(item);
            delete item;
        }
    }
    
    // Add pieces from current game state
    qreal squareSize = scene_->width() / 8;
    
    // Determine board orientation based on player color
    bool isBlackPlayer = (playerColor_ == PieceColor::Black);

    // Debug logging
    qDebug() << "from ChessBoardView::updateBoard(): Player Colour" 
             << (playerColor_ == PieceColor::White ? "White" : 
                (playerColor_ == PieceColor::Black ? "Black" : "None"))
             << "... and isBlackPlayer is: " << isBlackPlayer
             << "... and scene width is: " << scene_->width()
             << "... and squareSize is: " << squareSize;

    // Check if game is valid
    if (!game_) {
        qDebug() << "from ChessBoardView::updateBoard(): Game object is NULL!";
        return;
    }
    
    // Log the number of pieces in the game
    int pieceCount = 0;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (game_->getPieceAt(QPoint(col, row))) {
                pieceCount++;
            }
        }
    }
    qDebug() << "from ChessBoardView::updateBoard(): Game has" << pieceCount << "pieces";
    
    // Always place pieces, even if player color is None
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            // Calculate the effective row and column based on player perspective
            int effectiveRow = isBlackPlayer ? (7 - row) : row;
            int effectiveCol = isBlackPlayer ? (7 - col) : col;
            
            // Get the piece from the logical board position
            auto piece = game_->getPieceAt(QPoint(col, row));
            if (piece) {
                auto pieceItem = new ChessPieceItem(piece);
                
                // Position pieces based on player perspective
                pieceItem->setPos(effectiveCol * squareSize, 
                                  effectiveRow * squareSize);
                pieceItem->updateSize(squareSize);
                scene_->addItem(pieceItem);
                
                // Set z-value to ensure pieces are above board squares
                pieceItem->setZValue(1);
                
                qDebug() << "from ChessBoardView::updateBoard(): Added piece" 
                         << pieceTypeToString(piece->getType())
                         << (piece->getColor() == PieceColor::White ? "White" : "Black")
                         << "at position" << col << "," << row
                         << "visual position" << effectiveCol << "," << effectiveRow;
            }
        }
    }

    updatePlayerStatusLabel();
    
    // Force scene update
    scene_->update();
    
    qDebug() << "from ChessBoardView::updateBoard(): Board updated with" << pieceCount << "pieces";
    
    // Log the current turn
    qDebug() << "from ChessBoardView::updateBoard(): Current turn is" 
             << (game_->getCurrentTurn() == PieceColor::White ? "White" : "Black");
}

void ChessBoardView::updateBoardFromGame()
{
    // Clear the board and update it with current game state
    updateBoard();
    
    // Update view based on game state
    if (game_->isCheckmate(game_->getCurrentTurn())) {
        emit gameOver(tr("Checkmate! %1 wins!")
            .arg(game_->getCurrentTurn() == PieceColor::White ? 
                 "Black" : "White"));
        gameOver_ = true;
    } else if (game_->isStalemate(game_->getCurrentTurn())) {
        emit gameOver(tr("Stalemate! Game is drawn."));
        gameOver_ = true;
    }
}

void ChessBoardView::updateTheme()
{
    setupBoard();
    updateBoard();
}

void ChessBoardView::clearHighlights()
{
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (highlightItems_[row][col]) {
                highlightItems_[row][col]->setBrush(::Qt::transparent);
            }
        }
    }
}

void ChessBoardView::applySettings()
{
    // Apply settings from the Settings singleton
    Settings& settings = Settings::getInstance();
    
    // Update board settings
    setTheme(settings.getCurrentTheme());
    
    // Update piece scale
    double whiteScale = settings.getThemeScale(PieceColor::White);
    double blackScale = settings.getThemeScale(PieceColor::Black);
    
    // Update all chess pieces with new scales
    for (auto item : scene_->items()) {
        // amazonq-ignore-next-line
        if (ChessPieceItem* pieceItem = dynamic_cast<ChessPieceItem*>(item)) {
            if (pieceItem->getPiece()->getColor() == PieceColor::White) {
                pieceItem->updateSize(whiteScale);
            } else {
                pieceItem->updateSize(blackScale);
            }
        }
    }
    
    // Update animation settings if applicable
    bool animationsEnabled = settings.getAnimationsEnabled();
    setAnimationsEnabled(animationsEnabled);
    
    // Update sound settings if applicable
    bool soundEnabled = settings.isSoundEnabled();
    setSoundEnabled(soundEnabled);
    
    // Refresh board view
    update();
}

bool ChessBoardView::connectToServer(const QString& host, quint16 port)
{
    // Create network client if not already created
    if (!networkClient_) {
        networkClient_ = new NetworkClient(this);
        
        // Connect signals from network client
        connect(networkClient_, &NetworkClient::connected, this, &ChessBoardView::onConnected);
        connect(networkClient_, &NetworkClient::disconnected, this, &ChessBoardView::onDisconnected);
        connect(networkClient_, &NetworkClient::parsedMoveReceived, this, &ChessBoardView::handleParsedMove);
        connect(networkClient_, &NetworkClient::errorOccurred, this, &ChessBoardView::onNetworkError);
    }
    
    // Connect to the specified server
    bool success = networkClient_->connectToServer(host, port);
    
    // Update status
    if (success) {
        emit statusChanged(tr("Connecting to %1:%2...").arg(host).arg(port));
    } else {
        emit statusChanged(tr("Failed to connect to %1:%2").arg(host).arg(port));
    }
    
    return success;
}

bool ChessBoardView::loadGame(const QString& filename)
{
    // Check if file exists
    QFile file(filename);
    if (!file.open(::QIODevice::ReadOnly)) {
        emit statusChanged(tr("Failed to open game file: %1").arg(filename));
        return false;
    }
    
    // Read file content
    QByteArray data = file.readAll();
    file.close();
    
    // Parse game data
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit statusChanged(tr("Invalid game file format"));
        return false;
    }
    
    // Reset current game
    game_ = std::make_unique<ChessGame>();
    
    // Deserialize game state
    if (!game_->fromJson(doc.object())) {
        emit statusChanged(tr("Failed to load game state"));
        game_ = std::make_unique<ChessGame>(); // Create a fresh game
        return false;
    }
    
    // Update board state to match the loaded game
    updateBoardFromGame();
    
    // Update move history
    emit gameLoaded(game_.get());
    
    // Update status
    emit statusChanged(tr("Game loaded from %1").arg(filename));
    return true;
}

bool ChessBoardView::saveGame(const QString& filename)
{
    // Check if there is an active game
    if (!game_) {
        emit statusChanged(tr("No active game to save"));
        return false;
    }
    
    // Serialize game state to JSON
    QJsonObject gameData = game_->toJson();
    
    // Create JSON document
    QJsonDocument doc(gameData);
    
    // Write to file
    QFile file(filename);
    if (!file.open(::QIODevice::WriteOnly)) {
        emit statusChanged(tr("Failed to open file for writing: %1").arg(filename));
        return false;
    }
    
    // Save the data
    file.write(doc.toJson());
    file.close();
    
    // Update status
    emit statusChanged(tr("Game saved to %1").arg(filename));
    return true;
}

void ChessBoardView::setTheme(const QString& theme)
{
    // Check if theme is different from current
    if (currentTheme_ == theme) {
        return;
    }
    
    // Update current theme
    currentTheme_ = theme;
    
    // Load theme from theme manager
    ThemeManager& themeManager = ThemeManager::getInstance();
    if (!themeManager.loadTheme(theme)) {
        emit statusChanged(tr("Failed to load theme: %1").arg(theme));
        return;
    }
    
    // Update board appearance
    setupBoard(); // This should redraw the board with new theme colors
    
    // Update pieces if there are theme-specific piece styles
    for (auto item : scene_->items()) {
        if (ChessPieceItem* pieceItem = dynamic_cast<ChessPieceItem*>(item)) {
            pieceItem->setTheme(theme);
            pieceItem->updateSize(scene_->width()/8);  // Force redraw with new theme
        }
    }
    
    // Emit theme changed signal
    emit themeChanged(theme);
    
    // Refresh view
    update();
}

void ChessBoardView::setAnimationsEnabled(bool enabled)
{
    animationsEnabled_ = enabled;
    // Implementation would depend on how animations are handled
}

void ChessBoardView::setSoundEnabled(bool enabled)
{
    soundEnabled_ = enabled;
    // Implementation would depend on how sound is handled
}

void ChessBoardView::resetGame()
{
    qDebug() << "from ChessBoardView::resetGame(): Entered with playerColor =" 
             << (playerColor_ == PieceColor::White ? "White" : 
                (playerColor_ == PieceColor::Black ? "Black" : "None"));
    
    // Store current player colour
    PieceColor currentPlayer = playerColor_;

    // Clear current selection
    selectedSquare_ = QPoint(-1, -1);
    
    // Remove highlight
    clearHighlights();
    
    // Create a new chess game
    game_ = std::make_unique<ChessGame>();
    
    qDebug() << "from ChessBoardView::resetGame(): Created new game with" << game_->getPieceCount() << "pieces";
    
    // Maintain the player color that was set
    playerColor_ = currentPlayer;
    
    // Setup the board with the correct orientation based on player color
    setupBoard();
    
    // Update board state
    updateBoard();
    
    // Notify that a new game has started
    emit gameLoaded(game_.get());
    
    // Update status
    emit statusChanged(tr("New game started"));
    
    // Reset game state variables
    gameOver_ = false;
    
    // Log the current player color
    qDebug() << "from ChessBoardView::resetGame(): Player color maintained as: " 
             << (playerColor_ == PieceColor::White ? "White" : "Black");
    
    // Refresh view
    update();
    
    qDebug() << "from ChessBoardView::resetGame(): Game reset complete";
}

void ChessBoardView::setPlayerColor(PieceColor color)
{
    if (playerColor_ != color) {
        playerColor_ = color;
        qDebug() << "from ChessBoardView::setPlayerColor(): Player color changed to" << (playerColor_ == PieceColor::White ? "White" : "Black");
        
        // Create a new game with pieces
        game_ = std::make_unique<ChessGame>();
        qDebug() << "from ChessBoardView::setPlayerColor(): Created new game with pieces";
        
        // Setup board with new orientation
        setupBoard();
        
        // Update the board to reflect the new orientation
        updateBoard();
        
        // Log the board state after setting player color
        qDebug() << "from ChessBoardView::setPlayerColor(): Board updated with player color" << (playerColor_ == PieceColor::White ? "White" : "Black");
        qDebug() << "from ChessBoardView::setPlayerColor(): Game has" << game_->getPieceCount() << "pieces on the board";
        
        // Update status
        updatePlayerStatusLabel();
        
        // Emit status change
        emit statusChanged(tr("Game started. You are playing %1")
            .arg(color == PieceColor::White ? "white" : "black"));
    }
}

QString ChessBoardView::getCurrentTheme() const
{
    return currentTheme_;
}

// Network-related slot implementations
void ChessBoardView::onConnected()
{
    emit statusChanged(tr("Connected to server"));
    
    // Notify that we're waiting for color assignment
    emit statusChanged(tr("Connected to server. Waiting for color assignment..."));
    
    // Notify server that player is ready to start
    notifyServerReady();
}

void ChessBoardView::onDisconnected()
{
    emit statusChanged(tr("Disconnected from server"));

    // Handle disconnection gracefully - todo: maybe offer to save the game
    if (!gameOver_) {
        emit gameOver(tr("Connection to server lost. Game ended."));
        gameOver_ = true;
    }
}

void ChessBoardView::onNetworkError(const QString& errorMsg)
{
    emit statusChanged(tr("Network error: %1").arg(errorMsg));
}

void ChessBoardView::setNetworkClient(NetworkClient* client)
{
    // Disconnect old client if exists
    if (networkClient_) {
        disconnect(networkClient_, nullptr, this, nullptr);
    }
    
    networkClient_ = client;
    
    // Connect to new client
    if (networkClient_) {
        connect(networkClient_, &NetworkClient::connected,
                this, &ChessBoardView::onConnected);
        connect(networkClient_, &NetworkClient::disconnected,
                this, &ChessBoardView::onDisconnected);
        connect(networkClient_, &NetworkClient::parsedMoveReceived,
                this, &ChessBoardView::handleParsedMove);
        connect(networkClient_, &NetworkClient::colorAssigned,
                this, &ChessBoardView::setPlayerColor);
        connect(networkClient_, &NetworkClient::errorOccurred,
                this, &ChessBoardView::onNetworkError);
    }
}

// New move handling with coordinates
void ChessBoardView::handleParsedMove(int fromCol, int fromRow, int toCol, int toRow)
{
    try
    {
        QPoint fromPos(fromCol, fromRow);
        QPoint toPos(toCol, toRow);
        
        // Add detailed logging
        qDebug() << "from ChessBoardView::handleParsedMove(): Network Move Received:"
                << "From: (" << fromCol << "," << fromRow << ")"
                << "To: (" << toCol << "," << toRow << ")"
                << "Current Player:" << ((game_->getCurrentTurn() == PieceColor::White) ? "White" : "Black");

        qDebug() << "from ChessBoardView::handleParsedMove(): Received Network Move: From" 
                << QPoint(fromCol, fromRow) 
                << "To" 
                << QPoint(toCol, toRow)
                << "Current Player:" << ((game_->getCurrentTurn() == PieceColor::White) ? "White" : "Black");

        // Determine opponent's color
        PieceColor opponentColor = (game_->getCurrentTurn() == PieceColor::White) 
                                    ? PieceColor::Black 
                                    : PieceColor::White;

        // Validate move before execution
        if (game_->isValidMove(fromPos, toPos, opponentColor)) {
            if (game_->makeMove(fromPos, toPos, opponentColor)) {
                updateBoard();
    //          emit moveCompleted(generateMoveNotation({fromCol, fromRow}, {toCol, toRow}));
                emit moveCompleted(generateMoveNotation(fromPos, toPos));

                // Game state checks
                if (game_->isCheckmate(game_->getCurrentTurn())) {
                    emit gameOver(tr("Checkmate! %1 wins!")
                        .arg(game_->getCurrentTurn() == PieceColor::White ? "Black" : "White"));
                } else if (game_->isStalemate(game_->getCurrentTurn())) {
                    emit gameOver(tr("Stalemate! Game is drawn."));
                }
            } else {
                qDebug() << "from ChessBoardView::handleParsedMove(): Network move failed: Invalid move execution";
            }
        } else {
            
            qDebug() << "from ChessBoardView::handleParsedMove(): Network move failed: Invalid move validation";
        }
    }
    catch (const std::exception& e)
    {
        qDebug() << "from ChessBoardView::handleParsedMove(): Exception in handleParsedMove:" << e.what();
    }
    catch (...)
    {
        qDebug() << "from ChessBoardView::handleParsedMove(): Unknown exception in handleParsedMove";
    }
}

void ChessBoardView::handleNetworkData(const QByteArray& data)
{
    // This function is no longer needed as we're handling color assignment directly
    // through the colorAssigned signal in NetworkClient
}

void ChessBoardView::notifyServerReady()
{
    if (networkClient_ && networkClient_->isConnected()) {
        qDebug() << "Notifying server that player is ready to start";
        
        networkClient_->sendReadyStatus();
    
        emit statusChanged(tr("Waiting for opponent..."));
    
        // QJsonObject readyMessage;
        // readyMessage["type"] = "ready";
        
        // QJsonDocument doc(readyMessage);
        // networkClient_->sendData(doc.toJson());
        
        // emit statusChanged(tr("Waiting for game to start..."));
    }
    else
    {
        emit statusChanged(tr("Not connected to server"));
    }
}