#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

private slots:
    void onLoginClicked();

private:
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    QPushButton *m_btnLogin;
    QPushButton *m_btnCancel;
};

#endif // LOGINDIALOG_H
