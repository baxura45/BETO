#include "ManisaControlPage.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QHBoxLayout>
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

    auto* ramGroup = new QGroupBox(tr("Minecraft RAM Ayarı"), this);
    auto* ramLayout = new QHBoxLayout(ramGroup);
    ramLayout->addWidget(new QLabel(tr("Başlangıç RAM"), ramGroup));
    m_minRamSpinBox = new QSpinBox(ramGroup);
    m_minRamSpinBox->setRange(256, 32768);
    m_minRamSpinBox->setSingleStep(256);
    m_minRamSpinBox->setSuffix(tr(" MB"));
    m_minRamSpinBox->setValue(APPLICATION->settings()->get("MinMemAlloc").toInt());
    ramLayout->addWidget(m_minRamSpinBox);

    ramLayout->addWidget(new QLabel(tr("Maksimum RAM"), ramGroup));
    m_maxRamSpinBox = new QSpinBox(ramGroup);
    m_maxRamSpinBox->setRange(512, static_cast<int>(HardwareInfo::totalRamMiB()));
    m_maxRamSpinBox->setSingleStep(512);
    m_maxRamSpinBox->setSuffix(tr(" MB"));
    m_maxRamSpinBox->setValue(APPLICATION->settings()->get("MaxMemAlloc").toInt());
    ramLayout->addWidget(m_maxRamSpinBox);
    root->addWidget(ramGroup);

    connect(m_minRamSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        if (value > m_maxRamSpinBox->value()) {
            m_maxRamSpinBox->setValue(value);
        }
        APPLICATION->settings()->set("MinMemAlloc", value);
    });
    connect(m_maxRamSpinBox, &QSpinBox::valueChanged, this, [this](int value) {
        const int minRam = APPLICATION->settings()->get("MinMemAlloc").toInt();
        if (value < minRam) {
            m_maxRamSpinBox->setValue(minRam);
            return;
        }
        APPLICATION->settings()->set("MaxMemAlloc", value);
    });

    auto* actions = new QHBoxLayout();
    m_cleanButton = new QPushButton(tr("Sistemi Hafiflet"), this);
    auto* logsButton = new QPushButton(tr("Log Klasörü"), this);
    auto* instancesButton = new QPushButton(tr("Instance Klasörü"), this);
    m_cleanButton->setStyleSheet(QStringLiteral("QPushButton{background:#10B981;color:#07110D;font-weight:800;padding:10px;border-radius:8px;}"));
    connect(m_cleanButton, &QPushButton::clicked, this, &ManisaControlPage::cleanLauncherFiles);
    connect(logsButton, &QPushButton::clicked, this, &ManisaControlPage::openLogsFolder);
    connect(instancesButton, &QPushButton::clicked, this, &ManisaControlPage::openInstancesFolder);
    actions->addWidget(m_cleanButton);
    actions->addWidget(logsButton);
    actions->addWidget(instancesButton);
    root->addLayout(actions);

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

void ManisaControlPage::openLogsFolder()
{
    DesktopServices::openPath(FS::PathCombine(APPLICATION->dataRoot(), "logs"), true);
}

void ManisaControlPage::openInstancesFolder()
{
    DesktopServices::openPath(APPLICATION->settings()->get("InstanceDir").toString(), true);
}
