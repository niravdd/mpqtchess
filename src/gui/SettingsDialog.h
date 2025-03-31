// src/gui/SettingsDialog.h
#pragma once
#include <QtWidgets/QDialog>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGraphicsRectItem>
#include <QtWidgets/QGraphicsPixmapItem>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void accept() override;
    void updatePreview();

private:
    void createUI();
    void loadSettings();
    void saveSettings();
    QGraphicsScene* previewScene_;
    QGraphicsView* previewView_;
    void setupPreviewBoard();
    void updatePreviewPieces();
    QString getCurrentThemePath() const;

    QComboBox* themeCombo_;
    QCheckBox* soundEnabledCheck_;
    QSpinBox* volumeSpinner_;
    QSpinBox* timeControlSpinner_;
    QCheckBox* autoQueenCheck_;
};