// src/gui/ConnectDialog.h
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class ConnectDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget* parent = nullptr);
    
    QString getAddress() const;
    quint16 getPort() const;

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