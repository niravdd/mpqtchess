// src/gui/ConnectDialog.cpp
#include "ConnectDialog.h"
#include "../util/Settings.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtCore/QRegularExpression>
#include <QtGui/QRegularExpressionValidator>

ConnectDialog::ConnectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Connect to Game"));
    createUI();
    loadSettings();
}

void ConnectDialog::createUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Server address
    QHBoxLayout* addressLayout = new QHBoxLayout;
    addressEdit_ = new QLineEdit;
    addressEdit_->setPlaceholderText(tr("Server address or IP"));
    // IPv4 validation regex
    QRegularExpression ipRegex(
        "^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}"
        "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\$|^localhost\\$");
    addressEdit_->setValidator(new QRegularExpressionValidator(ipRegex, this));
    addressLayout->addWidget(new QLabel(tr("Address:")));
    addressLayout->addWidget(addressEdit_);

    // Port number
    QHBoxLayout* portLayout = new QHBoxLayout;
    portSpinner_ = new QSpinBox;
    portSpinner_->setRange(1024, 65535);
    portSpinner_->setValue(12345);
    portLayout->addWidget(new QLabel(tr("Port:")));
    portLayout->addWidget(portSpinner_);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    connectButton_ = new QPushButton(tr("Connect"));
    QPushButton* cancelButton = new QPushButton(tr("Cancel"));
    connectButton_->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(connectButton_);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(addressLayout);
    mainLayout->addLayout(portLayout);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(connectButton_, &::QPushButton::clicked, this, &::QDialog::accept);
    connect(cancelButton, &::QPushButton::clicked, this, &::QDialog::reject);
    connect(addressEdit_, &::QLineEdit::textChanged, 
            this, &ConnectDialog::validateInput);
}

void ConnectDialog::validateInput()
{
    bool valid = !addressEdit_->text().isEmpty() && 
                 addressEdit_->hasAcceptableInput();
    connectButton_->setEnabled(valid);
}

void ConnectDialog::loadSettings()
{
    Settings& settings = Settings::getInstance();
    addressEdit_->setText(settings.getLastServer());
    portSpinner_->setValue(settings.getLastPort());
    validateInput();
}

void ConnectDialog::saveSettings()
{
    Settings& settings = Settings::getInstance();
    settings.setLastServer(addressEdit_->text());
    settings.setLastPort(portSpinner_->value());
}

QString ConnectDialog::getServerAddress() const
{
    return addressEdit_->text();
}

quint16 ConnectDialog::getServerPort() const
{
    return static_cast<quint16>(portSpinner_->value());
}

void ConnectDialog::accept()
{
    saveSettings();
    QDialog::accept();
}