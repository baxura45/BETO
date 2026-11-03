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
#include <QComboBox>
#include <QImage>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>

#include "Application.h"
#include "minecraft/launch/MinecraftTarget.h"
#include "settings/SettingsObject.h"
#include "ui/pages/instance/ServerPingTask.h"

namespace {

struct CatalogServer {
    QString name;
    QString address;
    QString region;
    QString type;
    QString description;
};

const QList<CatalogServer>& catalog()
{
    static const QList<CatalogServer> servers = {
        { QStringLiteral("Hypixel"), QStringLiteral("mc.hypixel.net"), QStringLiteral("Global"), QStringLiteral("Mini Oyunlar"), QStringLiteral("SkyBlock, Bed Wars ve parkur gibi topluluk modlarıyla popüler ağ.") },
        { QStringLiteral("CubeCraft"), QStringLiteral("play.cubecraft.net"), QStringLiteral("Global"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Bedrock/Java özellikli hızlı mini oyunlar ve sezonlu etkinlikler.") },
        { QStringLiteral("The Hive"), QStringLiteral("play.hivemc.com"), QStringLiteral("Global"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Bed Wars, Treasure Wars ve arcade tarzı çok oyunculu sunucu.") },
        { QStringLiteral("Minehut"), QStringLiteral("mc.minehut.com"), QStringLiteral("Global"), QStringLiteral("Topluluk"), QStringLiteral("Oyuncu toplulukları için barındırma ve mini oyun ağı.") },
        { QStringLiteral("GommeHD"), QStringLiteral("gommehd.net"), QStringLiteral("Global"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Almanya merkezli BedWars, CityBuild ve mini oyun deneyimleri.") },
        { QStringLiteral("PikaNetwork"), QStringLiteral("play.pika-network.net"), QStringLiteral("Global"), QStringLiteral("Prison / Survival"), QStringLiteral("Prison, SkyBlock ve Survival bölümüne sahip büyük ağ.") },
        { QStringLiteral("Mineplex"), QStringLiteral("mineplex.com"), QStringLiteral("Global"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Arcade oyunları, Bed Wars ve topluluk etkinlikleri.") },
        { QStringLiteral("Wynncraft"), QStringLiteral("play.wynncraft.com"), QStringLiteral("Global"), QStringLiteral("MMORPG"), QStringLiteral("Minecraft üzerinde kapsamlı görev, büyü ve sınıf sistemi sunan RPG ağı.") },
        { QStringLiteral("Complex Gaming"), QStringLiteral("play.complexgaming.net"), QStringLiteral("Global"), QStringLiteral("Survival / SkyBlock"), QStringLiteral("Modlu ve vanilla Survival, SkyBlock, Pixelmon seçenekleri.") },
        { QStringLiteral("DonutSMP"), QStringLiteral("play.donutsmp.net"), QStringLiteral("Global"), QStringLiteral("Hardcore Economy"), QStringLiteral("Ekonomi odaklı rekabetçi Survival deneyimi.") },
        { QStringLiteral("CraftRise"), QStringLiteral("play.craftrise.net"), QStringLiteral("Türkiye"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Türkiye odaklı BedWars, SkyWars ve Survival sunucusu.") },
        { QStringLiteral("SonOyuncu"), QStringLiteral("play.sonoyuncu.net"), QStringLiteral("Türkiye"), QStringLiteral("Mini Oyunlar"), QStringLiteral("Türkiye topluluğuna yönelik SkyBlock ve mini oyunlar.") },
        { QStringLiteral("MadenCraft"), QStringLiteral("mc.madencraft.com"), QStringLiteral("Türkiye"), QStringLiteral("Survival"), QStringLiteral("Hayatta kalma ve ekonomi odaklı Türk Minecraft topluluğu.") },
    };
    return servers;
}

enum Columns { ColName = 0, ColRegion, ColAddress, ColType, ColPlayers, ColPing, ColStatus, ColCount };

}  // namespace

DiscoverServersPage::DiscoverServersPage(QWidget* parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("DiscoverServersPage"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

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
    header->addWidget(m_refreshButton, 0, Qt::AlignVCenter);
    root->addLayout(header);

    auto* controlBar = new QHBoxLayout();
    controlBar->setSpacing(8);

    m_regionFilter = new QComboBox(this);
    m_regionFilter->addItems({ tr("Tüm Bölgeler"), tr("Türkiye"), tr("Global") });
    m_regionFilter->setMinimumWidth(180);
    connect(m_regionFilter, &QComboBox::currentTextChanged, this, [this]() { hideRowsBelowThreshold(); });
    if (QLocale::system().territory() == QLocale::Territory::Turkey) {
        m_regionFilter->setCurrentText(tr("Türkiye"));
    }
    auto* filterLabel = new QLabel(tr("Bölge"), this);
    filterLabel->setStyleSheet(QStringLiteral("color: #8B949E;"));
    controlBar->addWidget(filterLabel);
    controlBar->addWidget(m_regionFilter);
    controlBar->addStretch(1);

    m_statusLabel = new QLabel(tr("Sunucular sorgulanıyor…"), this);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #8B949E;"));
    controlBar->addWidget(m_statusLabel, 0, Qt::AlignRight);
    root->addLayout(controlBar);

    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("discoverServerTable"));
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({ tr("Sunucu"), tr("Bölge"), tr("Adres"), tr("Tür"), tr("Oyuncular"), tr("Ping"), tr("Durum") });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionsClickable(true);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(ColName, 150);
    m_table->setColumnWidth(ColRegion, 88);
    m_table->setColumnWidth(ColAddress, 170);
    m_table->setColumnWidth(ColType, 155);
    m_table->setColumnWidth(ColPlayers, 100);
    m_table->setColumnWidth(ColPing, 78);
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

    m_network = new QNetworkAccessManager(this);
    refreshServers();
}

void DiscoverServersPage::populateRows()
{
    const auto& servers = catalog();
    QList<CatalogServer> combined;
    combined.append(servers);
    for (const auto& dynamicServer : m_dynamicServers) {
        if (dynamicServer.size() >= 3) {
            combined.append({ dynamicServer.at(0), dynamicServer.at(1), dynamicServer.at(2),
                              dynamicServer.size() >= 4 ? dynamicServer.at(3) : tr("Topluluk"),
                              dynamicServer.size() >= 5 ? dynamicServer.at(4) : tr("Oy verme sitelerinden önerilen sunucu.") });
        }
    }

    m_table->setRowCount(combined.size());
    for (int row = 0; row < combined.size(); ++row) {
        const auto& server = combined.at(row);
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
        auto* type = new QTableWidgetItem(server.type);
        type->setToolTip(server.description);

        m_table->setItem(row, ColName, name);
        m_table->setItem(row, ColRegion, region);
        m_table->setItem(row, ColAddress, address);
        m_table->setItem(row, ColType, type);
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
        if (players < 30) {
            m_table->setRowHidden(row, true);
            return;
        }
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
        statusItem->setText(tr("Çevrimiçi"));
        statusItem->setForeground(QBrush(QColor(QStringLiteral("#F0F6FC"))));
        statusItem->setToolTip(motd);
    } else {
        playersItem->setText(QStringLiteral("—"));
        playersItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        pingItem->setText(QStringLiteral("—"));
        pingItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        statusItem->setText(tr("Çevrimdışı / zaman aşımı"));
        statusItem->setForeground(QBrush(QColor(QStringLiteral("#F43F5E"))));
        m_table->setRowHidden(row, true);
    }
    hideRowsBelowThreshold();
}

void DiscoverServersPage::hideRowsBelowThreshold()
{
    if (!m_regionFilter) return;
    const auto region = m_regionFilter->currentText();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        auto* regionItem = m_table->item(row, ColRegion);
        auto* playersItem = m_table->item(row, ColPlayers);
        bool regionMatch = region == tr("Tüm Bölgeler");
        if (!regionMatch && regionItem) {
            regionMatch = regionItem->text() == region;
        }
        bool playerMatch = !playersItem || playersItem->text() == QStringLiteral("…") ||
                           playersItem->text().section('/', 0, 0).trimmed().toInt() >= 30;
        m_table->setRowHidden(row, !regionMatch || !playerMatch);
    }
}

void DiscoverServersPage::refreshServers()
{
    m_statusLabel->setText(tr("Oy verme sitelerinden güncel sunucular ve oyuncu sayıları alınıyor…"));
    m_refreshButton->setEnabled(false);
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto* status = m_table->item(row, ColStatus)) {
            status->setText(tr("Sorgulanıyor"));
            status->setForeground(QBrush(QColor(QStringLiteral("#8B949E"))));
        }
    }

    m_dynamicServers.clear();
    const QStringList sources = {
        QStringLiteral("https://minecraftservers.org/"),
        QStringLiteral("https://topminecraftservers.org/country/TR"),
        QStringLiteral("https://topminecraftservers.org/")
    };

    auto remaining = std::make_shared<int>(sources.size());
    for (const auto& source : sources) {
        QNetworkRequest request{QUrl(source)};
        request.setRawHeader("User-Agent", "ManisaLauncher/12.0");
        auto* reply = m_network->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, remaining]() {
            if (reply->error() == QNetworkReply::NoError) {
                parseVoteCatalog(reply->readAll());
            }
            reply->deleteLater();
            (*remaining)--;
            if (*remaining == 0) {
                populateRows();
                startServerPing();
                m_statusLabel->setText(tr("Oy verme siteleri güncellendi. Sunucular ping ile kontrol ediliyor."));
            }
        });
    }
}

void DiscoverServersPage::parseVoteCatalog(const QByteArray& html)
{
    const QString text = QString::fromUtf8(html);
    QRegularExpression addressPattern(
        QStringLiteral(R"(([a-zA-Z0-9-]+\.)+[a-zA-Z]{2,})"),
        QRegularExpression::CaseInsensitiveOption);

    auto matches = addressPattern.globalMatch(text);
    while (matches.hasNext()) {
        auto match = matches.next();
        QString address = match.captured(0).trimmed();
        QString lower = address.toLower();
        if (lower.contains(QStringLiteral("minecraftservers.org")) ||
            lower.contains(QStringLiteral("minecraft.net")) ||
            lower.startsWith(QStringLiteral("www."))) {
            continue;
        }

        bool duplicate = false;
        for (const auto& candidate : m_dynamicServers) {
            if (!candidate.isEmpty() && candidate.at(1).compare(address, Qt::CaseInsensitive) == 0) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;

        QString name = address;
        if (name.startsWith(QStringLiteral("mc."), Qt::CaseInsensitive) ||
            name.startsWith(QStringLiteral("play."), Qt::CaseInsensitive) ||
            name.startsWith(QStringLiteral("hub."), Qt::CaseInsensitive)) {
            name = name.section('.', 1, 1);
        }
        name.replace('-', ' ');
        if (!name.isEmpty()) name[0] = name[0].toUpper();

        QString region = QStringLiteral("Global");
        if (lower.contains(QStringLiteral("craftavia")) || lower.contains(QStringLiteral("craftrise")) ||
            lower.contains(QStringLiteral("sonoyuncu")) || lower.contains(QStringLiteral("madencraft"))) {
            region = QStringLiteral("Türkiye");
        }

        m_dynamicServers.append({ name, address, region, QStringLiteral("Oy verme sitesi"), QStringLiteral("Anlık ping ve oyuncu sayısı ile önerilen oy topluluk sunucusu.") });
        if (m_dynamicServers.size() >= 30) break;
    }
}

void DiscoverServersPage::startServerPing()
{
    if (m_queryTask) {
        m_queryTask->abort();
        m_queryTask.reset();
    }

    QList<CatalogServer> servers;
    servers.append(catalog());
    for (const auto& dynamicServer : m_dynamicServers) {
        if (dynamicServer.size() >= 3) {
            servers.append({ dynamicServer.at(0), dynamicServer.at(1), dynamicServer.at(2),
                             dynamicServer.size() >= 4 ? dynamicServer.at(3) : tr("Topluluk"),
                             dynamicServer.size() >= 5 ? dynamicServer.at(4) : tr("Oy verme sitesinden öneri.") });
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
