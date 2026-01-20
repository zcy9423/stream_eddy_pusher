#include "logindialog.h"
#include "../core/usermanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("用户登录");
    setFixedSize(360, 220);
    
    // 去掉问号
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(15);

    // 标题
    QLabel *title = new QLabel("系统登录");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    // 输入区
    QGridLayout *formLayout = new QGridLayout();
    
    m_userEdit = new QLineEdit();
    m_userEdit->setPlaceholderText("用户名");
    m_userEdit->setMinimumWidth(200);
    m_userEdit->setMinimumHeight(28);
    // m_userEdit->setText("admin"); // Removed default for security

    m_passEdit = new QLineEdit();
    m_passEdit->setPlaceholderText("密码");
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setMinimumWidth(200);
    m_passEdit->setMinimumHeight(28);
    // m_passEdit->setText("123456"); // Removed default for security

    formLayout->addWidget(new QLabel("账号:"), 0, 0);
    formLayout->addWidget(m_userEdit, 0, 1);
    formLayout->addWidget(new QLabel("密码:"), 1, 0);
    formLayout->addWidget(m_passEdit, 1, 1);
    
    mainLayout->addLayout(formLayout);

    // 按钮区
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnLogin = new QPushButton("登录");
    m_btnLogin->setDefault(true);
    m_btnLogin->setStyleSheet("background-color: #3498db; color: white; border-radius: 4px; padding: 5px;");
    
    m_btnCancel = new QPushButton("取消");
    m_btnCancel->setStyleSheet("background-color: #ecf0f1; color: #333; border-radius: 4px; padding: 5px;");

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnLogin);
    
    mainLayout->addLayout(btnLayout);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void LoginDialog::onLoginClicked()
{
    QString user = m_userEdit->text().trimmed();
    QString pass = m_passEdit->text();

    if (UserManager::instance().login(user, pass)) {
        accept();
    } else {
        // 震动提示或红色边框效果可在此添加
        QMessageBox::warning(this, "登录失败", "用户名或密码错误");
        m_passEdit->clear();
        m_passEdit->setFocus();
    }
}
