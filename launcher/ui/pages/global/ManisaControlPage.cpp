#include "ManisaControlPage.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "HardwareInfo.h"
#include "DesktopServices.h"
#include "minecraft/auth/AccountList.h"
#include "settings/SettingsObject.h"

namespace {
QString formatSize(qint64 bytes)
{
    if (bytes >= 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 1) + QStringLiteral(" GB");
    if (bytes >= 1024LL * 1024)
        return QString::number(bytes / (1024.0 * 1024), 'f', 1) + QStringLiteral(" MB");
    return QString::number(bytes / 1024.0, 'f', 1) + QStringLiteral(" KB");
}
}

ManisaControlPage::ManisaControlPage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* title = new QLabel(tr("Manisa Merkezi"), this);
    auto titleFont = title->font();
    titleFont.setPointSize(titleFont.pointSize() + 6);
    titleFont.setBold(true);
    title->setFont(titleFont);
    root->addWidget(title);

    auto* cards = new QGridLayout();
    cards->setSpacing(10);
    m_healthLabel = new QLabel(tr("Instance Sağlığı: İyi"), this);
    m_memoryLabel = new QLabel(tr("Bellek: okunuyor"), this);
    m_accountLabel = new QLabel(tr("Profil: yükleniyor"), this);

    for (auto* card : { m_healthLabel, m_memoryLabel, m_accountLabel }) {
        card->setAlignment(Qt::AlignCenter);
        card->setMinimumHeight(74);
        card->setStyleSheet(QStringLiteral(
            "QLabel { background:#111827; border:1px solid #10B981; border-radius:14px; "
            "padding:14px; color:#E5E7EB; font-size:14px; font-weight:700; }"));
    }
    cards->addWidget(m_healthLabel, 0, 0);
    cards->addWidget(m_memoryLabel, 0, 1);
    cards->addWidget(m_accountLabel, 0, 2);
    root->addLayout(cards);

    auto* actions = new QHBoxLayout();
    m_cleanButton = new QPushButton(tr("Sistemi Hafiflet"), this);
    auto* screenshotsButton = new QPushButton(tr("Ekran Görüntüleri"), this);
    auto* logsButton = new QPushButton(tr("Log Klasörü"), this);
    auto* instancesButton = new QPushButton(tr("Instance Klasörü"), this);
    m_cleanButton->setStyleSheet(QStringLiteral("QPushButton{background:#10B981;color:#07110D;font-weight:800;padding:10px;border-radius:8px;}"));
    connect(m_cleanButton, &QPushButton::clicked, this, &ManisaControlPage::cleanLauncherFiles);
    connect(screenshotsButton, &QPushButton::clicked, this, &ManisaControlPage::openScreenshotFolder);
    connect(logsButton, &QPushButton::clicked, this, &ManisaControlPage::openLogsFolder);
    connect(instancesButton, &QPushButton::clicked, this, &ManisaControlPage::openInstancesFolder);
    actions->addWidget(m_cleanButton);
    actions->addWidget(screenshotsButton);
    actions->addWidget(logsButton);
    actions->addWidget(instancesButton);
    root->addLayout(actions);

    m_screenshots = new QTableWidget(this);
    m_screenshots->setColumnCount(3);
    m_screenshots->setHorizontalHeaderLabels({ tr("Dosya"), tr("Boyut"), tr("Tarih") });
    m_screenshots->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_screenshots->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_screenshots->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_screenshots, 1);

    auto* copyButton = new QPushButton(tr("Seçili Görüntüyü Kopyala"), this);
    connect(copyButton, &QPushButton::clicked, this, &ManisaControlPage::copySelectedScreenshot);
    root->addWidget(copyButton);

    refreshDashboard();
}

void ManisaControlPage::refreshDashboard()
{
    const auto total = HardwareInfo::totalRamMiB();
    const auto available = HardwareInfo::availableRamMiB();
    m_memoryLabel->setText(tr("RAM: %1 GB toplam\n%2 GB boş").arg(total / 1024.0, 0, 'f', 1).arg(available / 1024.0, 0, 'f', 1));
    m_accountLabel->setText(APPLICATION->accounts()->count() > 0
                                ? tr("Profil: %1 profil hazır").arg(APPLICATION->accounts()->count())
                                : tr("Profil: Henüz profil eklenmedi"));
}

void ManisaControlPage::cleanLauncherFiles()
{
    qint64 deletedSize = 0;
    QStringList folders = {
        FS::PathCombine(APPLICATION->dataRoot(), "logs"),
        FS::PathCombine(APPLICATION->dataRoot(), "cache"),
        FS::PathCombine(APPLICATION->root(), "cache")
    };
    for (const auto& folderPath : folders) {
        QDir folder(folderPath);
        if (!folder.exists()) continue;
        const auto files = folder.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto& file : files) {
            deletedSize += file.size();
            QFile::remove(file.absoluteFilePath());
        }
    }
    QMessageBox::information(this, tr("Manisa Temizlik"),
                             tr("%1 temizlendi. Loglar ve geçici önbellek kaldırıldı.").arg(formatSize(deletedSize)));
    refreshDashboard();
}

void ManisaControlPage::openScreenshotFolder()
{
    refreshScreenshots();
    DesktopServices::openPath(APPLICATION->settings()->get("InstanceDir").toString(), true);
}

void ManisaControlPage::openLogsFolder()
{
    DesktopServices::openPath(FS::PathCombine(APPLICATION->dataRoot(), "logs"), true);
}

void ManisaControlPage::openInstancesFolder()
{
    DesktopServices::openPath(APPLICATION->settings()->get("InstanceDir").toString(), true);
}

void ManisaControlPage::refreshScreenshots()
{
    m_screenshots->setRowCount(0);
    QDir instancesDir(APPLICATION->settings()->get("InstanceDir").toString());
    auto files = instancesDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                            QDir::Name);
    QList<QFileInfo> screenshots;
    for (const auto& instance : files) {
        if (!instance.isDir()) continue;
        QDir root(instance.absoluteFilePath());
        QStringList roots = { root.absoluteFilePath(".minecraft/screenshots"),
                              root.absoluteFilePath("screenshots"),
                              root.absoluteFilePath(".minecraft/config") };
        for (const auto& screenshotRoot : roots) {
            QDir dir(screenshotRoot);
            if (!dir.exists()) continue;
            auto found = dir.entryInfoList({ "*.png", "*.jpg", "*.jpeg" }, QDir::Files, QDir::Time);
            screenshots.append(found);
        }
    }
    m_screenshots->setRowCount(screenshots.size());
    for (int row = 0; row < screenshots.size(); ++row) {
        const auto& image = screenshots.at(row);
        m_screenshots->setItem(row, 0, new QTableWidgetItem(image.fileName()));
        m_screenshots->setItem(row, 1, new QTableWidgetItem(formatSize(image.size())));
        m_screenshots->setItem(row, 2, new QTableWidgetItem(image.lastModified().toString("dd.MM.yyyy HH:mm")));
        m_screenshots->item(row, 0)->setData(Qt::UserRole, image.absoluteFilePath());
    }
}

void ManisaControlPage::copySelectedScreenshot()
{
    const auto rows = m_screenshots->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    const auto path = m_screenshots->item(rows.first().row(), 0)->data(Qt::UserRole).toString();
    QImage image(path);
    if (!image.isNull()) {
        QApplication::clipboard()->setImage(image);
        QMessageBox::information(this, tr("Manisa Galeri"), tr("Ekran görüntüsü panoya kopyalandı."));
    }
}
