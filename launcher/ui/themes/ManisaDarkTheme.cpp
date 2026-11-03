// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 *  Copyright (C) 2022-2026 Prism Launcher Contributors
 *  Copyright (C) 2013-2021 MultiMC Contributors
 */
#include "ManisaDarkTheme.h"

#include <QObject>

QString ManisaDarkTheme::id()
{
    return "manisa-dark";
}

QString ManisaDarkTheme::name()
{
    return QObject::tr("Manisa Dark");
}

QString ManisaDarkTheme::tooltip()
{
    return QObject::tr("OLED-friendly Deep Obsidian theme with Spil Emerald accents (60-30-10).");
}

QString ManisaDarkTheme::appStyleSheet()
{
    return DarkTheme::appStyleSheet() + QStringLiteral(
        "QWidget { font-family: 'Inter', 'Segoe UI', 'Noto Sans', sans-serif; }"
        "QPlainTextEdit, QTextEdit { font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace; }"
        "QPushButton[default=\"true\"], QPushButton#playButton, QPushButton#refreshServersButton {"
        "    background-color: #10B981; color: #0D1117; border: 1px solid #34D399; border-radius: 8px;"
        "    font-weight: 700; padding: 7px 16px;"
        "}"
        "QPushButton[default=\"true\"]:hover, QPushButton#playButton:hover, QPushButton#refreshServersButton:hover {"
        "    background-color: #34D399; border: 1px solid #6EE7B7; color: #0D1117;"
        "}"
        "QTabWidget#homeTabs::pane { border: 1px solid #30363D; border-radius: 10px; background: #0D1117; }"
        "QTabWidget#homeTabs QTabBar::tab { min-width: 120px; padding: 10px 18px; }"
        "QWidget#DiscoverServersPage { background-color: #0D1117; }"
        "QTableWidget#discoverServerTable {"
        "    background-color: #161B22; alternate-background-color: #21262D; border: 1px solid #30363D;"
        "    border-radius: 10px; gridline-color: #30363D; selection-background-color: #21262D;"
        "    selection-color: #F0F6FC;"
        "}"
        "QLabel#playerBadge { color: #10B981; font-weight: 700; font-family: 'JetBrains Mono', 'Cascadia Code', monospace; }"
        "QLabel#offlineBadge { color: #F43F5E; font-weight: 700; }"
        "QLabel#regionBadge { color: #EC4899; font-weight: 600; }"
        "QStatusBar QLabel#legalCopyrightLabel { color: #8B949E; font-size: 11px; }"
    );
}
