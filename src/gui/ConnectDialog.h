// src/gui/ConnectDialog.h
#pragma once
#include "../QtCompat.h"
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>

class ConnectDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget* parent = nullptr);
    
    QString getServerAddress() const;
    quint16 getServerPort() const;
    void accept() override; // Override QDialog's accept() method

private slots:
    void validateInput();

private:
    void createUI();
    void loadSettings();
    void saveSettings();

    QLineEdit* addressEdit_;
    QSpinBox* portSpinner_;
    QPushButton* connectButton_;
};