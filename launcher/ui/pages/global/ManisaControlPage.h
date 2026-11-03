#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QSpinBox;
class QGroupBox;
class ManisaControlPage : public QWidget
{
    Q_OBJECT
public:
    explicit ManisaControlPage(QWidget* parent = nullptr);

private slots:
    void refreshDashboard();
    void cleanLauncherFiles();
    void openLogsFolder();
    void openInstancesFolder();

private:
    QLabel* m_healthLabel = nullptr;
    QLabel* m_memoryLabel = nullptr;
    QLabel* m_accountLabel = nullptr;
    QPushButton* m_cleanButton = nullptr;
    QSpinBox* m_minRamSpinBox = nullptr;
    QSpinBox* m_maxRamSpinBox = nullptr;
};
