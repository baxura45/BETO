// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 *  Copyright (C) 2022-2026 Prism Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <QWidget>

#include "tasks/ConcurrentTask.h"

class QLabel;
class QPushButton;
class QTableWidget;
class QTimer;

class DiscoverServersPage : public QWidget {
    Q_OBJECT

   public:
    explicit DiscoverServersPage(QWidget* parent = nullptr);
    ~DiscoverServersPage() override = default;

   public slots:
    void refreshServers();

   private:
    void populateRows();
    void applyRowStatus(int row, bool online, int players, int maxPlayers, int pingMs, const QString& motd);

    QTableWidget* m_table = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QTimer* m_autoRefresh = nullptr;
    ConcurrentTask::Ptr m_queryTask;
};
