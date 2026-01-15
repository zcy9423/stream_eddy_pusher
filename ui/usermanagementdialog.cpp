#include "usermanagementdialog.h"
#include "../core/usermanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>

// 辅助对话框：编辑用户
class UserEditDialog : public QDialog {
public:
    UserEditDialog(QWidget* parent, const QString& username = "", UserManager::UserRole role = UserManager::Operator) 
        : QDialog(parent) {
        setWindowTitle(username.isEmpty() ? "添加用户" : "编辑用户");
        QVBoxLayout* layout = new QVBoxLayout(this);
        QFormLayout* form = new QFormLayout();
        
        editUser = new QLineEdit(username);
        editUser->setEnabled(username.isEmpty()); // 编辑时不可改名
        
        editPass = new QLineEdit();
        editPass->setEchoMode(QLineEdit::Password);
        editPass->setPlaceholderText(username.isEmpty() ? "必填" : "留空则不修改");
        
        comboRole = new QComboBox();
        comboRole->addItem("管理员", UserManager::Admin);
        comboRole->addItem("操作员", UserManager::Operator);
        // comboRole->addItem("访客", UserManager::Guest); // 通常不添加访客账号
        
        // 设置当前角色
        int idx = comboRole->findData(role);
        if (idx >= 0) comboRole->setCurrentIndex(idx);

        form->addRow("用户名:", editUser);
        form->addRow("密码:", editPass);
        form->addRow("角色:", comboRole);
        
        QDialogButtonBox* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
        
        layout->addLayout(form);
        layout->addWidget(box);
    }
    
    QString getUsername() const { return editUser->text().trimmed(); }
    QString getPassword() const { return editPass->text(); }
    UserManager::UserRole getRole() const { 
        return static_cast<UserManager::UserRole>(comboRole->currentData().toInt()); 
    }

private:
    QLineEdit* editUser;
    QLineEdit* editPass;
    QComboBox* comboRole;
};

UserManagementDialog::UserManagementDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("用户与角色管理");
    resize(500, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 表格
    m_table = new QTableWidget();
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"用户名", "角色"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可直接在表格编辑
    
    mainLayout->addWidget(m_table);

    // 按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton("添加用户");
    m_btnEdit = new QPushButton("修改用户");
    m_btnDelete = new QPushButton("删除用户");
    
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnEdit);
    btnLayout->addWidget(m_btnDelete);
    
    mainLayout->addLayout(btnLayout);

    connect(m_btnAdd, &QPushButton::clicked, this, &UserManagementDialog::onAddUser);
    connect(m_btnEdit, &QPushButton::clicked, this, &UserManagementDialog::onEditUser);
    connect(m_btnDelete, &QPushButton::clicked, this, &UserManagementDialog::onDeleteUser);

    refreshList();
}

void UserManagementDialog::refreshList()
{
    m_table->setRowCount(0);
    QList<UserManager::User> users = UserManager::instance().getAllUsers();
    
    for (const auto& u : users) {
        int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(u.username));
        m_table->setItem(row, 1, new QTableWidgetItem(UserManager::roleName(u.role)));
        // 存储原始角色值以便编辑时恢复
        m_table->item(row, 1)->setData(Qt::UserRole, u.role);
    }
}

void UserManagementDialog::onAddUser()
{
    UserEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString u = dlg.getUsername();
        QString p = dlg.getPassword();
        if (u.isEmpty() || p.isEmpty()) {
            QMessageBox::warning(this, "错误", "用户名和密码不能为空");
            return;
        }
        
        if (UserManager::instance().addUser(u, p, dlg.getRole())) {
            refreshList();
        } else {
            QMessageBox::warning(this, "错误", "添加失败，用户名可能已存在");
        }
    }
}

void UserManagementDialog::onEditUser()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    
    QString username = m_table->item(row, 0)->text();
    UserManager::UserRole role = static_cast<UserManager::UserRole>(m_table->item(row, 1)->data(Qt::UserRole).toInt());
    
    UserEditDialog dlg(this, username, role);
    if (dlg.exec() == QDialog::Accepted) {
        if (UserManager::instance().updateUser(username, dlg.getPassword(), dlg.getRole())) {
            refreshList();
            QMessageBox::information(this, "提示", "修改成功");
        } else {
            QMessageBox::warning(this, "错误", "修改失败");
        }
    }
}

void UserManagementDialog::onDeleteUser()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    
    QString username = m_table->item(row, 0)->text();
    
    if (username == UserManager::instance().currentUser().username) {
        QMessageBox::warning(this, "警告", "不能删除当前登录的账号");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除用户 " + username + " 吗？") == QMessageBox::Yes) {
        if (UserManager::instance().removeUser(username)) {
            refreshList();
        } else {
            QMessageBox::warning(this, "错误", "删除失败（可能是唯一的管理员）");
        }
    }
}
