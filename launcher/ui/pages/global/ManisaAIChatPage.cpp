// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 */
#include "ManisaAIChatPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QScrollBar>
#include <QTextEdit>
#include <QVBoxLayout>

ManisaAIChatPage::ManisaAIChatPage(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("ManisaAIChatPage"));

    m_network = new QNetworkAccessManager(this);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    m_chat = new QTextEdit(this);
    m_chat->setObjectName(QStringLiteral("manisaAIChatHistory"));
    m_chat->setReadOnly(true);
    m_chat->setPlaceholderText(tr("Manisa AI Minecraft sorularını bekliyor..."));
    m_chat->setStyleSheet(QStringLiteral(
        "QTextEdit {"
        " background-color: #0D1117;"
        " color: #F0F6FC;"
        " border: 1px solid #30363D;"
        " border-radius: 14px;"
        " padding: 14px;"
        " selection-background-color: #10B981;"
        " font-size: 14px;"
        "}"));

    auto* composer = new QFrame(this);
    composer->setObjectName(QStringLiteral("manisaAIComposer"));
    composer->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #161B22; border: 1px solid #30363D; border-radius: 12px; }"));
    auto* composerLayout = new QHBoxLayout(composer);
    composerLayout->setContentsMargins(10, 8, 10, 8);
    composerLayout->setSpacing(10);

    m_input = new QLineEdit(composer);
    m_input->setPlaceholderText(tr("Minecraft hakkında bir soru sor..."));
    m_input->setClearButtonEnabled(true);
    m_input->setStyleSheet(QStringLiteral(
        "QLineEdit { background: transparent; border: none; color: #F0F6FC; padding: 8px; font-size: 14px; }"
        "QLineEdit::placeholder { color: #8B949E; }"));

    m_sendButton = new QPushButton(tr("Gönder"), composer);
    m_sendButton->setObjectName(QStringLiteral("manisaAISendButton"));
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_sendButton->setMinimumWidth(92);
    m_sendButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: #10B981; color: #0D1117; border: none; border-radius: 9px; padding: 10px 16px; font-weight: 700; }"
        "QPushButton:hover { background-color: #34D399; }"
        "QPushButton:pressed { background-color: #059669; }"
        "QPushButton:disabled { background-color: #30363D; color: #8B949E; }"));

    composerLayout->addWidget(m_input, 1);
    composerLayout->addWidget(m_sendButton);

    root->addWidget(m_chat, 1);
    root->addWidget(composer, 0);

    connect(m_sendButton, &QPushButton::clicked, this, &ManisaAIChatPage::sendMessage);
    connect(m_input, &QLineEdit::returnPressed, this, &ManisaAIChatPage::sendMessage);

    appendMessage(tr("Manisa AI"), tr("Merhaba! Mod yükleme, FPS ayarları, sunucu önerileri ve Minecraft sorunları hakkında yardımcı olabilirim."),
                  true);
}

void ManisaAIChatPage::appendMessage(const QString& author, const QString& message, bool assistant)
{
    const QString safeMessage = message.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>"));
    const QString bubbleColor = assistant ? QStringLiteral("#161B22") : QStringLiteral("#103D31");
    const QString authorColor = assistant ? QStringLiteral("#10B981") : QStringLiteral("#34D399");
    const QString alignment = assistant ? QStringLiteral("left") : QStringLiteral("right");

    m_chat->append(QStringLiteral(
        "<div style='text-align:%1;margin:6px 0;'>"
        "<div style='display:inline-block;text-align:left;max-width:75%%;background:%2;"
        "border:1px solid #30363D;border-radius:12px;padding:10px 12px;color:#F0F6FC;'>"
        "<b style='color:%3;'>%4</b><br>%5"
        "</div></div>")
                       .arg(alignment, bubbleColor, authorColor, author.toHtmlEscaped(), safeMessage));

    m_chat->verticalScrollBar()->setValue(m_chat->verticalScrollBar()->maximum());
}

void ManisaAIChatPage::setSending(bool sending)
{
    m_input->setEnabled(!sending);
    m_sendButton->setEnabled(!sending);
    m_sendButton->setText(sending ? tr("Bekleniyor...") : tr("Gönder"));
}

void ManisaAIChatPage::sendMessage()
{
    const QString message = m_input->text().trimmed();
    if (message.isEmpty() || m_reply != nullptr) {
        return;
    }

    appendMessage(tr("Sen"), message, false);
    m_input->clear();
    setSending(true);

    QJsonObject request;
    request[QStringLiteral("model")] = QStringLiteral("qwen2.5-coder:7b");
    request[QStringLiteral("stream")] = false;

    QJsonArray messages;
    QJsonObject system;
    system[QStringLiteral("role")] = QStringLiteral("system");
    system[QStringLiteral("content")] =
        QStringLiteral("Sen Manisa Launcher icin Minecraft uzmanisin. Mod kurulum, Fabric/Forge/NeoForge, FPS, "
                       "sunucu ve Java konularinda kisa, guvenli ve dogru Turkce yardim ver.");
    messages.append(system);

    QJsonObject user;
    user[QStringLiteral("role")] = QStringLiteral("user");
    user[QStringLiteral("content")] = message;
    messages.append(user);
    request[QStringLiteral("messages")] = messages;

    QNetworkRequest networkRequest(QUrl(QStringLiteral("http://127.0.0.1:11434/api/chat")));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    m_reply = m_network->post(networkRequest, QJsonDocument(request).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &ManisaAIChatPage::handleReply);
}

void ManisaAIChatPage::handleReply()
{
    if (!m_reply) {
        return;
    }

    QByteArray body = m_reply->readAll();
    const bool failed = m_reply->error() != QNetworkReply::NoError;
    m_reply->deleteLater();
    m_reply = nullptr;
    setSending(false);

    if (failed) {
        appendMessage(tr("Manisa AI"),
                      tr("Ollama'ya bağlanamadım. Ollama uygulamasının açık olduğundan ve qwen2.5-coder:7b modelinin kurulu olduğundan emin ol."),
                      true);
        m_input->setFocus();
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(body);
    const QString answer = document.object()
                               .value(QStringLiteral("message"))
                               .toObject()
                               .value(QStringLiteral("content"))
                               .toString();
    appendMessage(tr("Manisa AI"),
                  answer.isEmpty() ? tr("Şu anda bir yanıt oluşturamadım. Sorunu biraz daha ayrıntılı olarak yeniden yazabilirsin.")
                                   : answer,
                  true);
    m_input->setFocus();
}
