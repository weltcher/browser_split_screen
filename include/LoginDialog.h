#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QWidget>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

    QString getUsername() const;
    QString getPassword() const;
    bool isRememberPassword() const;
    bool isLoginSuccessful() const;

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onCancelClicked();
    void onTabChanged(int index);

private:
    void setupUI();
    void setupLoginTab();
    void setupRegisterTab();
    void clearFields();
    bool validateInput();
    void showMessage(const QString& title, const QString& message, QMessageBox::Icon icon);

    // Login tab widgets
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QCheckBox* m_rememberCheckBox;
    QPushButton* m_loginButton;
    QPushButton* m_cancelButton;

    // Register tab widgets
    QLineEdit* m_regUsernameEdit;
    QLineEdit* m_regPasswordEdit;
    QLineEdit* m_regConfirmPasswordEdit;
    QPushButton* m_registerButton;

    // Common widgets
    QTabWidget* m_tabWidget;
    QWidget* m_loginTab;
    QWidget* m_registerTab;

    // State
    bool m_loginSuccessful;
    QString m_username;
    QString m_password;
    bool m_rememberPassword;
};

#endif // LOGINDIALOG_H

