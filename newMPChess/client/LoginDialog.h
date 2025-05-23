// LoginDialog.h
#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit LoginDialog(QWidget* parent = nullptr);
    
    QString getUsername() const;
    QString getPassword() const;
    
private slots:
    void validateInput();
    
private:
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QPushButton* loginButton;
    QPushButton* cancelButton;
    QLabel* statusLabel;
};
