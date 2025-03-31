// src/gui/SettingsDialog.cpp
#include "SettingsDialog.h"
#include "../util/Settings.h"
#include <QtWidgets/QVBoxLayout>     // Layout widget
#include <QtWidgets/QHBoxLayout>     // Layout widget
#include <QtWidgets/QGroupBox>       // Container widget
#include <QtWidgets/QLabel>          // Label widget
#include <QtWidgets/QPushButton>     // Button widget
#include <QtWidgets/QGraphicsItem>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    createUI();
    loadSettings();
}

void SettingsDialog::createUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Theme settings
    QGroupBox* themeGroup = new QGroupBox(tr("Theme"));
    QVBoxLayout* themeLayout = new QVBoxLayout;
    themeCombo_ = new QComboBox;
    themeCombo_->addItems({"Classic", "Modern", "Minimalist"});
    themeLayout->addWidget(new QLabel(tr("Select Theme:")));
    themeLayout->addWidget(themeCombo_);
    
    // Add preview group
    previewView_ = new QGraphicsView;
    previewView_->setFixedSize(240, 240);  // Fixed size for preview
    previewView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewView_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewView_->setRenderHint(QPainter::Antialiasing);
    
    previewScene_ = new QGraphicsScene(this);
    previewView_->setScene(previewScene_);
    
    themeLayout->addWidget(new QLabel(tr("Preview:")));
    themeLayout->addWidget(previewView_);
    themeLayout->addStretch();
    themeGroup->setLayout(themeLayout);

    // Sound settings
    QGroupBox* soundGroup = new QGroupBox(tr("Sound"));
    QVBoxLayout* soundLayout = new QVBoxLayout;
    soundEnabledCheck_ = new QCheckBox(tr("Enable sound effects"));
    volumeSpinner_ = new QSpinBox;
    volumeSpinner_->setRange(0, 100);
    volumeSpinner_->setSuffix("%");
    soundLayout->addWidget(soundEnabledCheck_);
    soundLayout->addWidget(new QLabel(tr("Volume:")));
    soundLayout->addWidget(volumeSpinner_);
    soundLayout->addStretch();
    soundGroup->setLayout(soundLayout);

    // Game settings
    QGroupBox* gameGroup = new QGroupBox(tr("Game"));
    QVBoxLayout* gameLayout = new QVBoxLayout;
    timeControlSpinner_ = new QSpinBox;
    timeControlSpinner_->setRange(1, 120);
    timeControlSpinner_->setSuffix(tr(" minutes"));
    autoQueenCheck_ = new QCheckBox(tr("Auto-queen promotion"));
    gameLayout->addWidget(new QLabel(tr("Time control:")));
    gameLayout->addWidget(timeControlSpinner_);
    gameLayout->addWidget(autoQueenCheck_);
    gameLayout->addStretch();
    gameGroup->setLayout(gameLayout);

    // Create horizontal layout for groups
    QHBoxLayout* groupsLayout = new QHBoxLayout;
    groupsLayout->addWidget(themeGroup);
    
    // Create vertical layout for right side groups
    QVBoxLayout* rightGroupsLayout = new QVBoxLayout;
    rightGroupsLayout->addWidget(soundGroup);
    rightGroupsLayout->addWidget(gameGroup);
    rightGroupsLayout->addStretch();
    
    groupsLayout->addLayout(rightGroupsLayout);
    mainLayout->addLayout(groupsLayout);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    QPushButton* okButton = new QPushButton(tr("OK"));
    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(themeCombo_, &QComboBox::currentTextChanged,
            this, &SettingsDialog::updatePreview);
    connect(soundEnabledCheck_, &QCheckBox::toggled,
            volumeSpinner_, &QSpinBox::setEnabled);

    // Set up initial state
    setupPreviewBoard();
    updatePreview();

    // Set window properties
    setWindowTitle(tr("Settings"));
    setMinimumWidth(500);
    setModal(true);
}

void SettingsDialog::loadSettings()
{
    Settings& settings = Settings::getInstance();
    
    themeCombo_->setCurrentText(settings.getCurrentTheme());
    soundEnabledCheck_->setChecked(settings.isSoundEnabled());
    volumeSpinner_->setValue(settings.getSoundVolume());
    timeControlSpinner_->setValue(settings.getTimeControl());
    autoQueenCheck_->setChecked(settings.isAutoQueen());
    
    volumeSpinner_->setEnabled(settings.isSoundEnabled());
}

void SettingsDialog::saveSettings()
{
    Settings& settings = Settings::getInstance();
    
    settings.setCurrentTheme(themeCombo_->currentText());
    settings.setSoundEnabled(soundEnabledCheck_->isChecked());
    settings.setSoundVolume(volumeSpinner_->value());
    settings.setTimeControl(timeControlSpinner_->value());
    settings.setAutoQueen(autoQueenCheck_->isChecked());
}

void SettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void SettingsDialog::setupPreviewBoard()
{
    const int BOARD_SIZE = 4;  // Using 4x4 for preview instead of full 8x8
    const qreal SQUARE_SIZE = 50.0;
    
    previewScene_->clear();
    previewScene_->setSceneRect(0, 0, BOARD_SIZE * SQUARE_SIZE, BOARD_SIZE * SQUARE_SIZE);
    
    // Create board squares
    for (int row = 0; row < BOARD_SIZE; ++row) {
        for (int col = 0; col < BOARD_SIZE; ++col) {
            QGraphicsRectItem* square = previewScene_->addRect(
                col * SQUARE_SIZE, row * SQUARE_SIZE,
                SQUARE_SIZE, SQUARE_SIZE);
            
            bool isLight = (row + col) % 2 == 0;
            square->setBrush(isLight ? Qt::white : QColor(128, 128, 128));
            square->setPen(Qt::NoPen);
        }
    }
    
    // Fit view to scene
    previewView_->fitInView(previewScene_->sceneRect(), Qt::KeepAspectRatio);
}

void SettingsDialog::updatePreview()
{
    // Load theme colors
    QString themeName = themeCombo_->currentText().toLower();
    QFile themeFile(QString(":/themes/%1.json").arg(themeName));
    
    if (themeFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(themeFile.readAll());
        QJsonObject theme = doc.object();
        
        // Get board colors from theme
        QJsonObject board = theme["board"].toObject();
        QColor lightSquares(board["lightSquares"].toString());
        QColor darkSquares(board["darkSquares"].toString());
        QColor highlightColor(board["highlightMove"].toString());
        
        // Update board squares
        int squareIndex = 0;
        for (QGraphicsItem* item : previewScene_->items()) {
            if (QGraphicsRectItem* square = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                int row = squareIndex / 4;
                int col = squareIndex % 4;
                bool isLight = (row + col) % 2 == 0;
                square->setBrush(isLight ? lightSquares : darkSquares);
                
                // Add highlight example on one square
                if (row == 1 && col == 1) {
                    square->setBrush(highlightColor);
                }
                
                squareIndex++;
            }
        }
        
        // Update pieces
        updatePreviewPieces();
    }
    
    previewView_->update();
}

void SettingsDialog::updatePreviewPieces()
{
    const qreal SQUARE_SIZE = 50.0;
    QString themePath = getCurrentThemePath();
    
    // Clear existing pieces
    for (QGraphicsItem* item : previewScene_->items()) {
        if (dynamic_cast<QGraphicsPixmapItem*>(item) != nullptr) {
            previewScene_->removeItem(item);
            delete item;
        }
    }
    
    // Add sample pieces
    struct PreviewPiece {
        QString file;
        int row;
        int col;
    };
    
    std::vector<PreviewPiece> pieces = {
        {"white_king", 3, 1},
        {"white_queen", 3, 2},
        {"black_rook", 0, 0},
        {"black_knight", 0, 1},
        {"black_bishop", 0, 2},
        {"black_pawn", 1, 3}
    };
    
    for (const auto& piece : pieces) {
        QString imagePath = QString("%1/%2.png").arg(themePath).arg(piece.file);
        QPixmap pixmap(imagePath);
        
        if (!pixmap.isNull()) {
            QGraphicsPixmapItem* pieceItem = previewScene_->addPixmap(
                pixmap.scaled(SQUARE_SIZE, SQUARE_SIZE, 
                            Qt::KeepAspectRatio, 
                            Qt::SmoothTransformation));
            pieceItem->setPos(piece.col * SQUARE_SIZE, piece.row * SQUARE_SIZE);
        }
    }
}

QString SettingsDialog::getCurrentThemePath() const
{
    return QString(":/pieces/%1").arg(themeCombo_->currentText().toLower());
}