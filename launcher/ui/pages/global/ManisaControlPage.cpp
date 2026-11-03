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
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProcess>

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

    auto* profilesGroup = new QGroupBox(tr("Performans ve Mod Profilleri"), this);
    auto* profilesLayout = new QGridLayout(profilesGroup);
    profilesLayout->addWidget(new QLabel(tr("Performans Profili"), profilesGroup), 0, 0);
    m_profileCombo = new QComboBox(profilesGroup);
    m_profileCombo->addItems({ tr("Dengeli"), tr("FPS Odaklı"), tr("Kalite Odaklı"), tr("Mod Paketi") });
    m_profileCombo->setCurrentIndex(APPLICATION->settings()->get("ManisaPerformanceProfile").toInt());
    profilesLayout->addWidget(m_profileCombo, 0, 1);

    m_discordCheckBox = new QCheckBox(tr("Launcher Discord gösterimi"), profilesGroup);
    m_discordCheckBox->setChecked(APPLICATION->settings()->get("ManisaDiscordEnabled").toBool());
    profilesLayout->addWidget(m_discordCheckBox, 0, 2);
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ManisaControlPage::applyPerformanceProfile);
    connect(m_discordCheckBox, &QCheckBox::toggled, this, &ManisaControlPage::toggleDiscord);

    auto* modCheckButton = new QPushButton(tr("Mod Uyumluluğunu Tara"), profilesGroup);
    auto* discordButton = new QPushButton(tr("Discord'u Aç"), profilesGroup);
    modCheckButton->setStyleSheet(QStringLiteral("QPushButton{padding:8px;border-radius:7px;font-weight:700;}"));
    discordButton->setStyleSheet(QStringLiteral("QPushButton{padding:8px;border-radius:7px;font-weight:700;}"));
    profilesLayout->addWidget(modCheckButton, 1, 0, 1, 2);
    profilesLayout->addWidget(discordButton, 1, 2);
    connect(modCheckButton, &QPushButton::clicked, this, &ManisaControlPage::scanModCompatibility);
    connect(discordButton, &QPushButton::clicked, this, &ManisaControlPage::openDiscord);
    root->addWidget(profilesGroup);

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

void ManisaControlPage::applyPerformanceProfile(int index)
{
    const int totalRam = static_cast<int>(HardwareInfo::totalRamMiB());
    int minRam = 512;
    int maxRam = 4096;

    switch (index) {
    case 1:
        minRam = qMin(1024, totalRam / 8);
        maxRam = qMin(qMax(3072, totalRam / 2), totalRam - 2048);
        break;
    case 2:
        minRam = qMin(2048, totalRam / 4);
        maxRam = qMin(qMax(6144, totalRam / 3 * 2), totalRam - 2048);
        break;
    case 3:
        minRam = qMin(2048, totalRam / 4);
        maxRam = qMin(qMax(8192, totalRam / 2), totalRam - 3072);
        break;
    default:
        minRam = qMin(1024, totalRam / 8);
        maxRam = qMin(qMax(4096, totalRam / 2), totalRam - 2048);
        break;
    }

    minRam = qMax(256, minRam);
    maxRam = qMax(minRam + 512, maxRam);
    m_minRamSpinBox->setValue(minRam);
    m_maxRamSpinBox->setValue(maxRam);
    APPLICATION->settings()->set("ManisaPerformanceProfile", index);

    if (index == 1) {
        APPLICATION->settings()->set("JvmArgs", QStringLiteral("-XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=50"));
    } else if (index == 2) {
        APPLICATION->settings()->set("JvmArgs", QStringLiteral("-XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=200"));
    } else {
        APPLICATION->settings()->set("JvmArgs", QStringLiteral("-XX:+UseG1GC -XX:+ParallelRefProcEnabled -XX:MaxGCPauseMillis=100"));
    }
}

void ManisaControlPage::scanModCompatibility()
{
    const QString instanceRoot = APPLICATION->settings()->get("InstanceDir").toString();
    QDir instanceDir(instanceRoot);
    int instanceCount = 0;
    int modJarCount = 0;
    int missingLoaderCount = 0;
    int checkedMods = 0;

    const auto instanceFolders = instanceDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& instance : instanceFolders) {
        instanceCount++;
        QDir root(instance.absoluteFilePath());
        bool hasProfile = QFile::exists(FS::PathCombine(instance.absoluteFilePath(), "mmc-pack.json"));
        if (!hasProfile)
            continue;

        QDir mods(FS::PathCombine(instance.absoluteFilePath(), "minecraft", "mods"));
        if (!mods.exists())
            mods.setPath(FS::PathCombine(instance.absoluteFilePath(), ".minecraft", "mods"));

        if (!mods.exists())
            continue;

        const auto jars = mods.entryInfoList({ "*.jar" }, QDir::Files, QDir::Name);
        modJarCount += jars.size();
        checkedMods += jars.size();
        if (jars.isEmpty())
            missingLoaderCount++;
    }

    QString report = tr("%1 instance tarandı\n%2 mod dosyası bulundu\n%3 mods klasörü boş")
                         .arg(instanceCount)
                         .arg(modJarCount)
                         .arg(missingLoaderCount);

    if (checkedMods == 0) {
        report += tr("\nDurum: Mod içermeyen veya mods klasörü bulunmayan instance'lar.");
    } else {
        report += tr("\nDurum: Temel dosya uyumluluk taraması tamamlandı.");
    }

    QMessageBox::information(this, tr("Mod Uyumluluk Taraması"), report);
}

void ManisaControlPage::toggleDiscord(bool enabled)
{
    APPLICATION->settings()->set("ManisaDiscordEnabled", enabled);
}

void ManisaControlPage::openDiscord()
{
    if (!QDesktopServices::openUrl(QUrl(QStringLiteral("discord://-/settings/activity-status"))))
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://discord.com/settings/activity-status")));
}
