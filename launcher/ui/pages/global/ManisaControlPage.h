#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class QCheckBox;
class QGroupBox;

class ManisaControlPage : public QWidget
{
    Q_OBJECT
public:
    explicit ManisaControlPage(QWidget* parent = nullptr);

private slots:
    void refreshDashboard();
    void cleanLauncherFiles();
    void openScreenshotFolder();
    void openLogsFolder();
    void openInstancesFolder();
    void refreshScreenshots();
    void copySelectedScreenshot();
    void applyPvpPreset();
    void saveDiscordSettings();

private:
    QLabel* m_healthLabel = nullptr;
    QLabel* m_memoryLabel = nullptr;
    QLabel* m_accountLabel = nullptr;
    QTableWidget* m_screenshots = nullptr;
    QPushButton* m_cleanButton = nullptr;
    QPushButton* m_pvpPresetButton = nullptr;
    QCheckBox* m_discordEnabled = nullptr;
    QCheckBox* m_discordNickname = nullptr;
    QCheckBox* m_discordServer = nullptr;
    QCheckBox* m_discordPlaytime = nullptr;
};
