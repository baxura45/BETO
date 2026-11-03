// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 */
#pragma once

#include <QWidget>

class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QPushButton;
class QTextEdit;

class ManisaAIChatPage : public QWidget {
    Q_OBJECT

   public:
    explicit ManisaAIChatPage(QWidget* parent = nullptr);

   private slots:
    void sendMessage();
    void handleReply();

   private:
    void appendMessage(const QString& author, const QString& message, bool assistant);
    void setSending(bool sending);

    QTextEdit* m_chat = nullptr;
    QLineEdit* m_input = nullptr;
    QPushButton* m_sendButton = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    QNetworkReply* m_reply = nullptr;
};
