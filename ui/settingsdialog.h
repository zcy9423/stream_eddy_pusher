#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    // 保存并关闭
    void accept() override;

private:
    void initUI();
    void loadSettings();

private:
    // Serial
    QComboBox *m_comboBaud;
    
    // Motion
    QDoubleSpinBox *m_spinMaxSpeed;
    QDoubleSpinBox *m_spinMaxPos;
    QSpinBox *m_spinTimeout;

    // Data
    QLineEdit *m_editDataPath;
    QPushButton *m_btnBrowse;
};

#endif // SETTINGSDIALOG_H
