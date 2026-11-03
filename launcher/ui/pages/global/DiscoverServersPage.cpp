// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 *  Copyright (C) 2022-2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */
#include "DiscoverServersPage.h"

#include <QApplication>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "Application.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "settings/SettingsObject.h"
#include "ui/pages/instance/ServerPingTask.h"

namespace {

struct CatalogServer {
    QString name;
    QString address;
    QString region;
};

const QList<CatalogServer>& catalog()
{
    static const QList<CatalogServer> servers = {
        { QStringLiteral("Hypixel"), QStringLiteral("mc.hypixel.net"), QStringLiteral("Global") },
        { QStringLiteral("CubeCraft"), QStringLiteral("play.cubecraft.net"), QStringLiteral("Global") },
        { QStringLiteral("The Hive"), QStringLiteral("play.hivemc.com"), QStringLiteral("Global") },
        { QStringLiteral("2b2t"), QStringLiteral("2b2t.org"), QStringLiteral("Global") },
        { QStringLiteral("Minehut"), QStringLiteral("mc.minehut.com"), QStringLiteral("Global") },
        { QStringLiteral("GommeHD"), QStringLiteral("gommehd.net"), QStringLiteral("Global") },
        { QStringLiteral("PikaNetwork"), QStringLiteral("play.pika-network.net"), QStringLiteral("Global") },
        { QStringLiteral("CraftRise"), QStringLiteral("play.craftise.net"), QStringLiteral("Türkiye") },
        { QStringLiteral("SonOyuncu"), QStringLiteral("play.sonoyuncu.network"), QStringLiteral("Türkiye") },
    };
    return servers;
}

enum Columns { ColName = 0, ColRegion, ColAddress, ColPlayers, ColPing, ColStatus, ColCount };

}  // namespace

DiscoverServersPage::DiscoverServersPage(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DiscoverServersPage"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* header = new QHBoxLayout();
    auto* titleBox = new QVBoxLayout();
    auto* title = new QLabel(tr("Keşfet / Sunucu Listesi"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    title->setFont(titleFont);
    auto* subtitle = new QLabel(tr("Türkiye ve global Minecraft sunucularının anlık oyuncu sayısı ve ping durumu."), this);
    subtitle->setStyleSheet(QStringLiteral("color: #8B949E;"));
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);

    m_refreshButton = new QPushButton(tr("Yenile"), this);
    m_refreshButton->setObjectName(QStringLiteral("refreshServersButton"));
    m_refreshButton->setDefault(true);
    connect(m_refreshButton, &QPushButton::clicked, this, &DiscoverServersPage::refreshServers);

    header->addLayout(titleBox, 1);
    header->addWidget(m_refreshButton, 0, Qt::AlignTop);
    root->addLayout(header);

    m_statusLabel = new QLabel(tr("Sunucular sorgulanıyor…"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8B949E;"));
    root->addWidget(m_statusLabel);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("discoverServerTable"));
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({ tr("Sunucu"), tr("Bölge"), tr("Adres"), tr("Oyuncular"), tr("Ping"), tr("Durum") });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColRegion, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColPing, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        auto* item = m_table->item(row, ColAddress);
        if (!item) {
            return;
        }
        QApplication::clipboard()->setText(item->text());
        m_statusLabel->setText(tr("Adres panoya kopyalandı: %1").arg(item->text()));
    });
    root->addWidget(m_table, 1);

    auto* hint = new QLabel(tr("Adresi kopyalamak için bir satıra çift tıklayın. Durum, Minecraft Server List Ping (handshake + status JSON) ile alınır."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #8B949E;"));
    root->addWidget(hint);

    populateRows();

    m_autoRefresh = new QTimer(this);
    m_autoRefresh->setInterval(60 * 1000);
    connect(m_autoRefresh, &QTimer::timeout, this, &DiscoverServersPage::refreshServers);
    m_autoRefresh->start();

    refreshServers();
}

void DiscoverServersPage::populateRows()
{
    const auto& servers = catalog();
    m_table->setRowCount(servers.size());
    for (int row = 0; row < servers.size(); ++row) {
        const auto& server = servers.at(row);
        auto* name = new QTableWidgetItem(server.name);
        name->setFont([&] {
            QFont f = font();
            f.setBold(true);
            return f;
        }());
        auto* region = new QTableWidgetItem(server.region);
        region->setForeground(QBrush(QColor(QStringLiteral("#EC4899"))));
        auto* address = new QTableWidgetItem(server.address);
        address->setFont([&] {
            QFont f = font();
            f.setFamily(QStringLiteral("JetBrains Mono"));
            return f;
        }());
        auto* players = new QTableWidgetItem(tr("…"));
        players->setTextAlignment(Qt::AlignCenter);
        auto* ping = new QTableWidgetItem(QStringLiteral("—"));
        ping->setTextAlignment(Qt::AlignCenter);
        auto* status = new QTableWidgetItem(tr("Sorgulanıyor"));
        status->setForeground(QBrush(QColor(QStringLiteral("#8B949E"))));

        m_table->setItem(row, ColName, name);
        m_table->setItem(row, ColRegion, region);
        m_table->setItem(row, ColAddress, address);
        m_table->setItem(row, ColPlayers, players);
        m_table->setItem(row, ColPing, ping);
        m_table->setItem(row, ColStatus, status);
    }
}

void DiscoverServersPage::applyRowStatus(int row, bool online, int players, int maxPlayers, int pingMs, const QString& motd)
{
    if (row < 0 || row >= m_table->rowCount()) {
        return;
    }
    auto* playersItem = m_table->item(row, ColPlayers);
    auto* pingItem = m_table->item(row, ColPing);
    auto* statusItem = m_table->item(row, ColStatus);
    if (!playersItem || !pingItem || !statusItem) {
        return;
    }

    if (online) {
        if (maxPlayers > 0) {
            playersItem->setText(QStringLiteral("%1 / %2").arg(players).arg(maxPlayers));
        } else {
            playersItem->setText(QString::number(players));
        }
        playersItem->setForeground(QBrush(QColor(QStringLiteral("#10B981"))));
        pingItem->setText(tr("%1 ms").arg(pingMs));
        if (pingMs < 80) {
            pingItem->setForeground(QBrush(QColor(QStringLiteral("#10B981"))));
        } else if (pingMs < 180) {
            pingItem->setForeground(QBrush(QColor(QStringLiteral("#F0F6FC"))));
        } else {
            pingItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        }
        statusItem->setText(motd.isEmpty() ? tr("Çevrimiçi") : motd);
        statusItem->setForeground(QBrush(QColor(QStringLiteral("#F0F6FC"))));
        statusItem->setToolTip(motd);
    } else {
        playersItem->setText(QStringLiteral("—"));
        playersItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        pingItem->setText(QStringLiteral("—"));
        pingItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        statusItem->setText(tr("Çevrimdışı / zaman aşımı"));
        statusItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
    }
}

void DiscoverServersPage::refreshServers()
{
    if (m_queryTask) {
        m_queryTask->abort();
        m_queryTask.reset();
    }

    const auto& servers = catalog();
    m_statusLabel->setText(tr("Minecraft Server List Ping gönderiliyor…"));
    m_refreshButton->setEnabled(false);

    for (int row = 0; row < servers.size(); ++row) {
        if (auto* status = m_table->item(row, ColStatus)) {
            status->setText(tr("Sorgulanıyor"));
            status->setForeground(QBrush(QColor(QStringLiteral("#8B949E"))));
        }
    }

    m_queryTask = ConcurrentTask::Ptr(
        new ConcurrentTask("Discover server status", APPLICATION->settings()->get("NumberOfConcurrentTasks").toInt()));

    int row = 0;
    for (const auto& server : servers) {
        const auto target = MinecraftTarget::parse(server.address, false);
        auto* task = new ServerPingTask(target.address, target.port);
        m_queryTask->addTask(Task::Ptr(task));
        connect(task, &Task::finished, this, [this, task, row]() {
            const bool online = task->wasSuccessful() && task->m_outputOnlinePlayers >= 0;
            applyRowStatus(row, online, task->m_outputOnlinePlayers, task->m_outputMaxPlayers, task->m_outputPingMs,
                           task->m_outputDescription);
        });
        ++row;
    }

    connect(m_queryTask.get(), &Task::finished, this, [this]() {
        m_refreshButton->setEnabled(true);
        m_statusLabel->setText(tr("Son güncelleme tamamlandı. Otomatik yenileme 60 saniyede bir."));
        m_queryTask.reset();
    });

    m_queryTask->start();
}
