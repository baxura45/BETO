// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Manisa Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Manisa Launcher Contributors
 *  Copyright (C) 2022-2026 Prism Launcher Contributors
 *  Copyright (C) 2013-2021 MultiMC Contributors
 */
#include "DarkTheme.h"

#include <QObject>

QString DarkTheme::id()
{
    return "dark";
}

QString DarkTheme::name()
{
    return QObject::tr("Manisa Modern Dark");
}

QPalette DarkTheme::colorScheme()
{
    QPalette palette;
    // 60-30-10 Professional Palette
    // #0D1117 (Deep Obsidian) - Window & Base
    // #161B22 / #21262D (Frosted Steel) - Cards & Buttons
    // #10B981 (Spil Emerald) - Accents & Primary actions
    // #EC4899 (Mesir Crimson) - Highlights
    palette.setColor(QPalette::Window, QColor("#0D1117"));
    palette.setColor(QPalette::WindowText, QColor("#F0F6FC"));
    palette.setColor(QPalette::Base, QColor("#161B22"));
    palette.setColor(QPalette::AlternateBase, QColor("#21262D"));
    palette.setColor(QPalette::ToolTipBase, QColor("#161B22"));
    palette.setColor(QPalette::ToolTipText, QColor("#F0F6FC"));
    palette.setColor(QPalette::Text, QColor("#F0F6FC"));
    palette.setColor(QPalette::Button, QColor("#21262D"));
    palette.setColor(QPalette::ButtonText, QColor("#F0F6FC"));
    palette.setColor(QPalette::BrightText, QColor("#EC4899"));
    palette.setColor(QPalette::Link, QColor("#10B981"));
    palette.setColor(QPalette::Highlight, QColor("#10B981"));
    palette.setColor(QPalette::HighlightedText, QColor("#0D1117"));
    palette.setColor(QPalette::PlaceholderText, QColor("#8B949E"));
    return fadeInactive(palette, fadeAmount(), fadeColor());
}

double DarkTheme::fadeAmount()
{
    return 0.5;
}

QColor DarkTheme::fadeColor()
{
    return QColor("#0D1117");
}

bool DarkTheme::hasStyleSheet()
{
    return true;
}

QString DarkTheme::appStyleSheet()
{
    return QStringLiteral(
        "QMainWindow, QDialog { background-color: #0D1117; color: #F0F6FC; }"
        "QWidget { font-family: 'Segoe UI', 'Inter', sans-serif; }"
        "QPushButton { background-color: #21262D; border: 1px solid #30363D; border-radius: 6px; padding: 6px 14px; color: #F0F6FC; font-weight: 500; }"
        "QPushButton:hover { background-color: #30363D; border: 1px solid #10B981; color: #FFFFFF; }"
        "QPushButton:pressed { background-color: #10B981; color: #0D1117; }"
        "QPushButton:disabled { background-color: #161B22; border-color: #21262D; color: #484F58; }"
        "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QComboBox {"
        "    background-color: #161B22; border: 1px solid #30363D; border-radius: 6px; padding: 5px 8px; color: #F0F6FC; selection-background-color: #10B981; selection-color: #0D1117;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QComboBox:focus { border: 1px solid #10B981; }"
        "QListView, QTreeView, QTableView {"
        "    background-color: #161B22; border: 1px solid #30363D; border-radius: 8px; color: #F0F6FC; outline: none; padding: 4px;"
        "}"
        "QListView::item, QTreeView::item { border-radius: 6px; padding: 4px 8px; margin: 2px; }"
        "QListView::item:selected, QTreeView::item:selected {"
        "    background-color: #21262D; border-left: 3px solid #10B981; color: #FFFFFF;"
        "}"
        "QListView::item:hover, QTreeView::item:hover { background-color: #1C2128; }"
        "QTabBar::tab {"
        "    background: #161B22; color: #8B949E; border: 1px solid #30363D; border-bottom: none; padding: 8px 16px; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px;"
        "}"
        "QTabBar::tab:selected { background: #0D1117; color: #10B981; border-top: 2px solid #10B981; font-weight: bold; }"
        "QTabBar::tab:hover:!selected { background: #21262D; color: #F0F6FC; }"
        "QTabWidget::pane { border: 1px solid #30363D; background-color: #0D1117; border-radius: 6px; }"
        "QHeaderView::section { background-color: #161B22; color: #8B949E; padding: 6px; border: none; border-bottom: 1px solid #30363D; font-weight: bold; }"
        "QScrollBar:vertical { background: #0D1117; width: 10px; margin: 0; border-radius: 5px; }"
        "QScrollBar::handle:vertical { background: #30363D; min-height: 24px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: #10B981; }"
        "QScrollBar:horizontal { background: #0D1117; height: 10px; margin: 0; border-radius: 5px; }"
        "QScrollBar::handle:horizontal { background: #30363D; min-width: 24px; border-radius: 5px; }"
        "QScrollBar::handle:horizontal:hover { background: #10B981; }"
        "QToolTip { color: #F0F6FC; background-color: #161B22; border: 1px solid #10B981; border-radius: 6px; padding: 6px 10px; }"
        "QProgressBar { background-color: #161B22; border: 1px solid #30363D; border-radius: 6px; text-align: center; color: #F0F6FC; height: 16px; }"
        "QProgressBar::chunk { background-color: #10B981; border-radius: 5px; }"
        "QStatusBar { background-color: #161B22; color: #8B949E; border-top: 1px solid #30363D; padding: 2px 8px; }"
        "QMenuBar { background-color: #0D1117; color: #F0F6FC; border-bottom: 1px solid #21262D; }"
        "QMenuBar::item:selected { background-color: #21262D; color: #10B981; border-radius: 4px; }"
        "QMenu { background-color: #161B22; color: #F0F6FC; border: 1px solid #30363D; border-radius: 6px; padding: 4px; }"
        "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: #21262D; color: #10B981; }"
        "QToolBar { background-color: #0D1117; border: none; border-bottom: 1px solid #21262D; spacing: 4px; padding: 4px; }"
        "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 6px; padding: 6px; color: #F0F6FC; }"
        "QToolButton:hover { background-color: #21262D; border: 1px solid #30363D; color: #10B981; }"
        "QToolButton:pressed { background-color: #30363D; }"
        "QPushButton[default=\"true\"] {"
        "    background-color: #10B981; color: #0D1117; border: 1px solid #34D399; border-radius: 8px; font-weight: 700;"
        "}"
        "QPushButton[default=\"true\"]:hover { background-color: #34D399; border: 1px solid #6EE7B7; }"
        "QPlainTextEdit, QTextEdit { font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace; }"
    );
}

QString DarkTheme::tooltip()
{
    return QObject::tr("Manisa Launcher official modern dark theme featuring Deep Obsidian and Spil Emerald.");
}
