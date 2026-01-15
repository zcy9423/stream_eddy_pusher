#ifndef USERMANAGEMENTDIALOG_H
#define USERMANAGEMENTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>

class UserManagementDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UserManagementDialog(QWidget *parent = nullptr);

private slots:
    void onAddUser();
    void onEditUser();
    void onDeleteUser();
    void refreshList();

private:
    QTableWidget *m_table;
    QPushButton *m_btnAdd;
    QPushButton *m_btnEdit;
    QPushButton *m_btnDelete;
};

#endif // USERMANAGEMENTDIALOG_H
