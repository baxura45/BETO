#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

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

private:
    QLabel* m_healthLabel = nullptr;
    QLabel* m_memoryLabel = nullptr;
    QLabel* m_accountLabel = nullptr;
    QTableWidget* m_screenshots = nullptr;
    QPushButton* m_cleanButton = nullptr;
};
