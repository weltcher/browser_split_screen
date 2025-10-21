#include "LoginDialog.h"
#include "DatabaseManager.h"
#include <QApplication>
#include <QScreen>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , m_loginSuccessful(false)
    , m_rememberPassword(false)
{
    setupUI();
    setWindowTitle("Browser Split Screen - 登录");
    setModal(true);
    setFixedSize(400, 300);
    
    // Center the dialog on screen
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

LoginDialog::~LoginDialog()
{
}

void LoginDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    
    // Create tabs
    m_loginTab = new QWidget();
    m_registerTab = new QWidget();
    
    m_tabWidget->addTab(m_loginTab, "登录");
    m_tabWidget->addTab(m_registerTab, "注册");
    
    setupLoginTab();
    setupRegisterTab();
    
    mainLayout->addWidget(m_tabWidget);
    
    // Connect tab change signal
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &LoginDialog::onTabChanged);
    
    // Set default tab
    m_tabWidget->setCurrentIndex(0);
}

void LoginDialog::setupLoginTab()
{
    QVBoxLayout* layout = new QVBoxLayout(m_loginTab);
    
    // Create form group
    QGroupBox* formGroup = new QGroupBox("用户登录", m_loginTab);
    QFormLayout* formLayout = new QFormLayout(formGroup);
    
    // Username field
    m_usernameEdit = new QLineEdit(formGroup);
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setMaxLength(50);
    formLayout->addRow("用户名:", m_usernameEdit);
    
    // Password field
    m_passwordEdit = new QLineEdit(formGroup);
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMaxLength(100);
    formLayout->addRow("密码:", m_passwordEdit);
    
    // Remember password checkbox
    m_rememberCheckBox = new QCheckBox("记住密码", formGroup);
    formLayout->addRow("", m_rememberCheckBox);
    
    layout->addWidget(formGroup);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_loginButton = new QPushButton("登录", m_loginTab);
    m_loginButton->setDefault(true);
    m_loginButton->setMinimumWidth(80);
    buttonLayout->addWidget(m_loginButton);
    
    m_cancelButton = new QPushButton("取消", m_loginTab);
    m_cancelButton->setMinimumWidth(80);
    buttonLayout->addWidget(m_cancelButton);
    
    layout->addLayout(buttonLayout);
    layout->addStretch();
    
    // Connect signals
    connect(m_loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
}

void LoginDialog::setupRegisterTab()
{
    QVBoxLayout* layout = new QVBoxLayout(m_registerTab);
    
    // Create form group
    QGroupBox* formGroup = new QGroupBox("用户注册", m_registerTab);
    QFormLayout* formLayout = new QFormLayout(formGroup);
    
    // Username field
    m_regUsernameEdit = new QLineEdit(formGroup);
    m_regUsernameEdit->setPlaceholderText("请输入用户名");
    m_regUsernameEdit->setMaxLength(50);
    formLayout->addRow("用户名:", m_regUsernameEdit);
    
    // Password field
    m_regPasswordEdit = new QLineEdit(formGroup);
    m_regPasswordEdit->setPlaceholderText("请输入密码");
    m_regPasswordEdit->setEchoMode(QLineEdit::Password);
    m_regPasswordEdit->setMaxLength(100);
    formLayout->addRow("密码:", m_regPasswordEdit);
    
    // Confirm password field
    m_regConfirmPasswordEdit = new QLineEdit(formGroup);
    m_regConfirmPasswordEdit->setPlaceholderText("请再次输入密码");
    m_regConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
    m_regConfirmPasswordEdit->setMaxLength(100);
    formLayout->addRow("确认密码:", m_regConfirmPasswordEdit);
    
    layout->addWidget(formGroup);
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_registerButton = new QPushButton("注册", m_registerTab);
    m_registerButton->setDefault(true);
    m_registerButton->setMinimumWidth(80);
    buttonLayout->addWidget(m_registerButton);
    
    layout->addLayout(buttonLayout);
    layout->addStretch();
    
    // Connect signals
    connect(m_registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(m_regConfirmPasswordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::onLoginClicked()
{
    if (!validateInput()) {
        return;
    }
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    
    if (dbManager->authenticateUser(m_usernameEdit->text(), m_passwordEdit->text())) {
        m_loginSuccessful = true;
        m_username = m_usernameEdit->text();
        m_password = m_passwordEdit->text();
        m_rememberPassword = m_rememberCheckBox->isChecked();
        
        // Save session if remember password is checked
        if (m_rememberPassword) {
            dbManager->saveUserSession(m_username, true);
        }
        
        // showMessage("登录成功", "欢迎使用浏览器分屏工具！", QMessageBox::Information);
        accept();
    } else {
        showMessage("登录失败", "用户名或密码错误，请重试。", QMessageBox::Warning);
        m_passwordEdit->clear();
        m_passwordEdit->setFocus();
    }
}

void LoginDialog::onRegisterClicked()
{
    QString username = m_regUsernameEdit->text().trimmed();
    QString password = m_regPasswordEdit->text();
    QString confirmPassword = m_regConfirmPasswordEdit->text();
    
    // Validate input
    if (username.isEmpty()) {
        showMessage("注册失败", "用户名不能为空。", QMessageBox::Warning);
        m_regUsernameEdit->setFocus();
        return;
    }
    
    if (password.isEmpty()) {
        showMessage("注册失败", "密码不能为空。", QMessageBox::Warning);
        m_regPasswordEdit->setFocus();
        return;
    }
    
    if (password.length() < 6) {
        showMessage("注册失败", "密码长度至少6位。", QMessageBox::Warning);
        m_regPasswordEdit->setFocus();
        return;
    }
    
    if (password != confirmPassword) {
        showMessage("注册失败", "两次输入的密码不一致。", QMessageBox::Warning);
        m_regConfirmPasswordEdit->clear();
        m_regConfirmPasswordEdit->setFocus();
        return;
    }
    
    DatabaseManager* dbManager = DatabaseManager::getInstance();
    
    if (dbManager->createUser(username, password)) {
        showMessage("注册成功", "用户注册成功，请使用新账户登录。", QMessageBox::Information);
        
        // Clear register fields and switch to login tab
        clearFields();
        m_tabWidget->setCurrentIndex(0);
        
        // Fill login fields with new user info
        m_usernameEdit->setText(username);
        m_passwordEdit->setFocus();
    } else {
        showMessage("注册失败", "用户名已存在，请选择其他用户名。", QMessageBox::Warning);
        m_regUsernameEdit->setFocus();
    }
}

void LoginDialog::onCancelClicked()
{
    reject();
}

void LoginDialog::onTabChanged(int index)
{
    clearFields();
    
    if (index == 0) { // Login tab
        m_usernameEdit->setFocus();
    } else { // Register tab
        m_regUsernameEdit->setFocus();
    }
}

void LoginDialog::clearFields()
{
    m_usernameEdit->clear();
    m_passwordEdit->clear();
    m_rememberCheckBox->setChecked(false);
    
    m_regUsernameEdit->clear();
    m_regPasswordEdit->clear();
    m_regConfirmPasswordEdit->clear();
}

bool LoginDialog::validateInput()
{
    if (m_usernameEdit->text().trimmed().isEmpty()) {
        showMessage("登录失败", "请输入用户名。", QMessageBox::Warning);
        m_usernameEdit->setFocus();
        return false;
    }
    
    if (m_passwordEdit->text().isEmpty()) {
        showMessage("登录失败", "请输入密码。", QMessageBox::Warning);
        m_passwordEdit->setFocus();
        return false;
    }
    
    return true;
}

void LoginDialog::showMessage(const QString& title, const QString& message, QMessageBox::Icon icon)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

QString LoginDialog::getUsername() const
{
    return m_username;
}

QString LoginDialog::getPassword() const
{
    return m_password;
}

bool LoginDialog::isRememberPassword() const
{
    return m_rememberPassword;
}

bool LoginDialog::isLoginSuccessful() const
{
    return m_loginSuccessful;
}

