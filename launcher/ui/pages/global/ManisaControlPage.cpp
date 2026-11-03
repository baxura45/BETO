#include "ManisaControlPage.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCheckBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>
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

    auto* customization = new QHBoxLayout();
    customization->setSpacing(10);

    auto* pvpGroup = new QGroupBox(tr("Hazır PvP Ayarı"), this);
    auto* pvpLayout = new QVBoxLayout(pvpGroup);
    pvpLayout->addWidget(new QLabel(tr("FOV 99 • GUI Ölçeği 2 • Ekranı geri atan titreme kapalı • Otomatik koşma kapalı"), pvpGroup));
    m_pvpPresetButton = new QPushButton(tr("Manisa PvP Ayarını Uygula"), pvpGroup);
    m_pvpPresetButton->setStyleSheet(QStringLiteral("QPushButton{background:#10B981;color:#07110D;font-weight:800;padding:10px;border-radius:8px;}"));
    pvpLayout->addWidget(m_pvpPresetButton);
    connect(m_pvpPresetButton, &QPushButton::clicked, this, &ManisaControlPage::applyPvpPreset);
    customization->addWidget(pvpGroup, 1);

    auto* discordGroup = new QGroupBox(tr("Discord Rich Presence"), this);
    auto* discordLayout = new QVBoxLayout(discordGroup);
    m_discordEnabled = new QCheckBox(tr("Discord durumunu göster"), discordGroup);
    m_discordNickname = new QCheckBox(tr("Minecraft nickini göster"), discordGroup);
    m_discordServer = new QCheckBox(tr("Sunucu adını göster"), discordGroup);
    m_discordPlaytime = new QCheckBox(tr("Oyun süresini göster"), discordGroup);
    m_discordEnabled->setChecked(APPLICATION->settings()->get("DiscordPresenceEnabled").toBool());
    m_discordNickname->setChecked(APPLICATION->settings()->get("DiscordPresenceNickname").toBool());
    m_discordServer->setChecked(APPLICATION->settings()->get("DiscordPresenceServer").toBool());
    m_discordPlaytime->setChecked(APPLICATION->settings()->get("DiscordPresencePlaytime").toBool());
    discordLayout->addWidget(m_discordEnabled);
    discordLayout->addWidget(m_discordNickname);
    discordLayout->addWidget(m_discordServer);
    discordLayout->addWidget(m_discordPlaytime);
    for (auto* checkbox : { m_discordEnabled, m_discordNickname, m_discordServer, m_discordPlaytime }) {
        connect(checkbox, &QCheckBox::toggled, this, &ManisaControlPage::saveDiscordSettings);
    }
    m_discordNickname->setEnabled(m_discordEnabled->isChecked());
    m_discordServer->setEnabled(m_discordEnabled->isChecked());
    m_discordPlaytime->setEnabled(m_discordEnabled->isChecked());
    customization->addWidget(discordGroup, 1);
    root->addLayout(customization);

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

void ManisaControlPage::applyPvpPreset()
{
    const QMap<QString, QString> values = {
        { "fov", "0.65" },
        { "guiScale", "2" },
        { "bobView", "false" },
        { "autoJump", "false" },
        { "pauseOnLostFocus", "false" },
        { "attackIndicator", "2" },
        { "advancedItemTooltips", "false" },
        { "sensitivity", "0.5" }
    };

    int updated = 0;
    QDir instancesDir(APPLICATION->settings()->get("InstanceDir").toString());
    const auto instances = instancesDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& instance : instances) {
        const QString optionsPath = FS::PathCombine(instance.absoluteFilePath(), "minecraft", "options.txt");
        QString content;

        QFile options(optionsPath);
        if (options.exists() && options.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&options);
            while (!stream.atEnd()) {
                const QString line = stream.readLine();
                const int separator = line.indexOf(':');
                if (separator > 0) {
                    const QString key = line.left(separator);
                    if (!values.contains(key)) {
                        content += line + '\n';
                    }
                } else if (!line.isEmpty()) {
                    content += line + '\n';
                }
            }
            options.close();
        }

        if (!options.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            continue;
        }
        QTextStream stream(&options);
        if (!content.isEmpty()) {
            stream << content;
        }
        for (auto it = values.cbegin(); it != values.cend(); ++it) {
            stream << it.key() << ':' << it.value() << '\n';
        }
        options.close();
        updated++;
    }

    APPLICATION->settings()->set("ManisaPvpPresetVersion", 1);
    APPLICATION->settings()->set("ManisaPvpPresetFov", 99);
    APPLICATION->settings()->set("ManisaPvpPresetGuiScale", 2);
    QMessageBox::information(this, tr("Manisa PvP Ayarı"),
                             tr("%1 instance için hazır ayar uygulandı.\nMinecraft bir sonraki açılışta bu ayarları kullanacak.").arg(updated));
}

void ManisaControlPage::saveDiscordSettings()
{
    APPLICATION->settings()->set("DiscordPresenceEnabled", m_discordEnabled->isChecked());
    APPLICATION->settings()->set("DiscordPresenceNickname", m_discordNickname->isChecked());
    APPLICATION->settings()->set("DiscordPresenceServer", m_discordServer->isChecked());
    APPLICATION->settings()->set("DiscordPresencePlaytime", m_discordPlaytime->isChecked());
    m_discordNickname->setEnabled(m_discordEnabled->isChecked());
    m_discordServer->setEnabled(m_discordEnabled->isChecked());
    m_discordPlaytime->setEnabled(m_discordEnabled->isChecked());
}
