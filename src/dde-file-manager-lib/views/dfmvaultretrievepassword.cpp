/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     lixiang<lixianga@uniontech.com>
 *
 * Maintainer: zhengyouge<zhengyouge@uniontech.com>
 *             lixiang<lixianga@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "dfmvaultretrievepassword.h"
#include "accessibility/ac-lib-file-manager.h"
#include "vault/operatorcenter.h"
#include "durl.h"

#include <QStringList>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QDateTime>
#include <QLineEdit>

using namespace PolkitQt1;

DWIDGET_USE_NAMESPACE

const QString defaultKeyPath = VAULT_BASE_PATH + QString("/") + RSA_PUB_KEY_FILE_NAME + QString(".key");
const QString PolicyKitRetrievePasswordActionId = "com.deepin.filemanager.vault.VerifyKey.RetrievePassword";

DFMVaultRetrievePassword *DFMVaultRetrievePassword::instance()
{
    static DFMVaultRetrievePassword s_instance;
    return &s_instance;
}

void DFMVaultRetrievePassword::verificationKey()
{
    QString password;
    QString keyPath;
    switch (m_savePathTypeComboBox->currentIndex()) {
    case 0:
    {
        if(QFile::exists(defaultKeyPath)) {
            m_defaultFilePathEdit->setText(QString(DFMVAULT_ROOT) + RSA_PUB_KEY_FILE_NAME + QString(".key"));
            getButton(1)->setEnabled(true);
            keyPath = defaultKeyPath;
        }
        else {
            m_defaultFilePathEdit->setPlaceholderText(tr("Unable to get the key file"));
            m_defaultFilePathEdit->setText("");
            getButton(1)->setEnabled(false);
        }
        break;
    }
    case 1:
        keyPath = m_filePathEdit->text();
        if(!QFile::exists(keyPath)) {
            m_filePathEdit->lineEdit()->setPlaceholderText(tr("Unable to get the key file"));
            m_filePathEdit->setText("");
            getButton(1)->setEnabled(false);
        }
        else {
            getButton(1)->setEnabled(true);
        }
        break;
    }

    if(OperatorCenter::getInstance()->verificationRetrievePassword(keyPath, password)) {
        setResultsPage(password);
    }
    else {
        m_verificationPrompt->setText(tr("Verification failed"));
    }
}

QString DFMVaultRetrievePassword::getUserName()
{
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QString userName = userPath.section("/", -1, -1);
    return userName;
}

void DFMVaultRetrievePassword::onButtonClicked(int index, const QString &text)
{
    switch(index) {
    case 0:
        emit signalReturn();
        break;
    case 1:
        if(text == btnList[3]) {
            setOnButtonClickedClose(true);  //! 按钮1关闭窗口能力打开
        }else {
            setOnButtonClickedClose(false); //! 按钮1关闭窗口能力关闭
            // 用户权限认证(异步授权)
            auto ins = Authority::instance();
            ins->checkAuthorization(PolicyKitRetrievePasswordActionId,
                                    UnixProcessSubject(getpid()),
                                    Authority::AllowUserInteraction);
            connect(ins, &Authority::checkAuthorizationFinished,
                    this, &DFMVaultRetrievePassword::slotCheckAuthorizationFinished);
        }
        break;
    }
}

void DFMVaultRetrievePassword::onComboBoxIndex(int index)
{
    switch (index) {
    case 0:
    {
        m_defaultFilePathEdit->show();
        m_filePathEdit->hide();
        if(QFile::exists(defaultKeyPath)) {
            m_defaultFilePathEdit->setText(QString(DFMVAULT_ROOT) + RSA_PUB_KEY_FILE_NAME + QString(".key"));
            getButton(1)->setEnabled(true);
        }
        else {
            m_defaultFilePathEdit->setPlaceholderText(tr("Unable to get the key file"));
            m_defaultFilePathEdit->setText("");
            getButton(1)->setEnabled(false);
        }
        m_verificationPrompt->setText("");
    }
        break;
    case 1:
        m_defaultFilePathEdit->hide();
        m_filePathEdit->show();
        if(QFile::exists(m_filePathEdit->text()))
            getButton(1)->setEnabled(true);
        else if(!m_filePathEdit->text().isEmpty() && m_filePathEdit->lineEdit()->placeholderText() != QString(tr("Unable to get the key file"))){
            m_filePathEdit->lineEdit()->setPlaceholderText(tr("Unable to get the key file"));
            m_filePathEdit->setText("");
            getButton(1)->setEnabled(false);
        }
        else {
            m_filePathEdit->lineEdit()->setPlaceholderText(tr("Select a path"));
            getButton(1)->setEnabled(false);
        }
        m_verificationPrompt->setText("");
        break;
    }
}

void DFMVaultRetrievePassword::onBtnSelectFilePath(const QString & path)
{
    m_filePathEdit->setText(path);
    if(!path.isEmpty())
        getButton(1)->setEnabled(true);
}

void DFMVaultRetrievePassword::slotCheckAuthorizationFinished(PolkitQt1::Authority::Result result)
{
    disconnect(Authority::instance(), &Authority::checkAuthorizationFinished,
            this, &DFMVaultRetrievePassword::slotCheckAuthorizationFinished);
    if (isVisible()) {
        if (result == Authority::Yes) {
            verificationKey();
        }
    }
}

DFMVaultRetrievePassword::DFMVaultRetrievePassword(QWidget *parent):DFMVaultPageBase(parent)
{
    setIcon(QIcon::fromTheme("dfm_vault"));
    setFixedWidth(396);

    m_title = new DLabel(tr("Retrieve Password"), this);

    m_savePathTypeComboBox = new QComboBox(this);
    AC_SET_ACCESSIBLE_NAME(m_savePathTypeComboBox, AC_VAULT_SAVE_PATH_TYPE_COMBOBOX);
    m_savePathTypeComboBox->addItem(tr("By key in the default path"));
    m_savePathTypeComboBox->addItem(tr("By key in the specified path"));

    m_filePathEdit = new DFileChooserEdit(this);
    AC_SET_ACCESSIBLE_NAME(m_filePathEdit, AC_VAULT_SAVE_PUBKEY_FILE_EDIT);
    m_filePathEdit->lineEdit()->setPlaceholderText(tr("Select a path"));
    QFileDialog * fileDialog = new QFileDialog(this, QDir::homePath());
    fileDialog->setDirectoryUrl(QDir::homePath());
    fileDialog->setNameFilter(QString("KEY file(*.key)"));
    m_filePathEdit->setFileDialog(fileDialog);
    m_filePathEdit->lineEdit()->setReadOnly(true);
    m_filePathEdit->hide();

    m_defaultFilePathEdit = new QLineEdit(this);
    m_defaultFilePathEdit->setEnabled(false);

    m_verificationPrompt = new DLabel(this);
    QPalette pe;
    pe.setColor(QPalette::WindowText,Qt::red);
    m_verificationPrompt->setPalette(pe);

    m_PasswordRecoveryPage = new QFrame(this);

    m_title1 = new DLabel(m_PasswordRecoveryPage);
    m_title1->setText(tr("Verification Successful"));
    m_passwordMsg = new DLabel(m_PasswordRecoveryPage);
    m_hintMsg = new DLabel(m_PasswordRecoveryPage);
    m_hintMsg->setText(tr("Keep it safe"));

    QHBoxLayout * hlayout = new QHBoxLayout();
    hlayout->setMargin(0);
    hlayout->addStretch(1);
    hlayout->addWidget(m_title1);
    hlayout->addStretch(1);

    QHBoxLayout * hlayout1 = new QHBoxLayout();
    hlayout1->setMargin(0);
    hlayout1->addStretch(1);
    hlayout1->addWidget(m_passwordMsg);
    hlayout1->addStretch(1);

    QHBoxLayout * hlayout2 = new QHBoxLayout();
    hlayout2->setMargin(0);
    hlayout2->addStretch(1);
    hlayout2->addWidget(m_hintMsg);
    hlayout2->addStretch(1);

    QVBoxLayout * vlayout = new QVBoxLayout(m_PasswordRecoveryPage);
    vlayout->setContentsMargins(0, 0, 0, 10);
    vlayout->addLayout(hlayout);
    vlayout->addSpacing(15);
    vlayout->addLayout(hlayout1);
    vlayout->addLayout(hlayout2);
    m_PasswordRecoveryPage->hide();

    // 主视图
    m_selectKeyPage = new QFrame(this);

    // 布局
    QHBoxLayout *play1 = new QHBoxLayout();
    play1->setMargin(0);
    play1->addWidget(m_filePathEdit);
    play1->addWidget(m_defaultFilePathEdit);

    QHBoxLayout *play2 = new QHBoxLayout();
    play2->addStretch(1);
    play2->addWidget(m_title);
    play2->addStretch(1);

    QHBoxLayout *play3 = new QHBoxLayout();
    play3->addStretch(1);
    play3->addWidget(m_verificationPrompt);
    play3->addStretch(1);

    QVBoxLayout *mainLayout = new QVBoxLayout(m_selectKeyPage);
    mainLayout->setContentsMargins(0, 0, 0, 10);
    mainLayout->addLayout(play2);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(m_savePathTypeComboBox);
    mainLayout->addLayout(play1);
    mainLayout->addLayout(play3);

    m_selectKeyPage->setLayout(mainLayout);
    addContent(m_selectKeyPage);
    setSpacing(0);
    // 防止点击按钮后界面隐藏
    setOnButtonClickedClose(false);

    btnList = QStringList({tr("Back","button"), tr("Verify Key","button"),tr("Go to Unlock", "button"), tr("Close", "button")});
    addButton(btnList[0], false);
    addButton(btnList[1], true, ButtonType::ButtonRecommend);

    AC_SET_ACCESSIBLE_NAME(getButton(0), AC_VAULT_RETURN_UNLOCK_PAGE_BTN);
    AC_SET_ACCESSIBLE_NAME(getButton(1), AC_VAULT_VERIFY_PUKEY_FILE_BTN);

    connect(this, &DFMVaultRetrievePassword::buttonClicked, this, &DFMVaultRetrievePassword::onButtonClicked);

    connect(m_savePathTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboBoxIndex(int)));

    connect(m_filePathEdit, &DFileChooserEdit::fileChoosed, this, &DFMVaultRetrievePassword::onBtnSelectFilePath);
}

void DFMVaultRetrievePassword::setVerificationPage()
{
    if(getContent(0) == m_PasswordRecoveryPage) {
        m_savePathTypeComboBox->setCurrentIndex(0);
        removeContent(m_PasswordRecoveryPage, false);
        m_PasswordRecoveryPage->hide();
        addContent(m_selectKeyPage);
        m_selectKeyPage->show();
        m_filePathEdit->setText(QString(""));
    }
    m_verificationPrompt->setText("");
    setButtonText(0, btnList[0]);
    setButtonText(1, btnList[1]);
}

void DFMVaultRetrievePassword::setResultsPage(QString password)
{
    m_passwordMsg->setText(tr("Vault password: %1").arg(password));
    m_selectKeyPage->hide();
    removeContent(m_selectKeyPage, false);
    addContent(m_PasswordRecoveryPage);
    m_PasswordRecoveryPage->show();
    setButtonText(0, btnList[2]);
    setButtonText(1, btnList[3]);
    AC_SET_ACCESSIBLE_NAME(getButton(1), AC_VAULT_VERIFY_PUKEY_FILE_BTN);
}

void DFMVaultRetrievePassword::showEvent(QShowEvent *event)
{
    if(QFile::exists(defaultKeyPath)) {
        m_defaultFilePathEdit->setText(QString(DFMVAULT_ROOT) + RSA_PUB_KEY_FILE_NAME + QString(".key"));
        getButton(1)->setEnabled(true);
    }
    else {
        m_defaultFilePathEdit->setPlaceholderText(tr("Unable to get the key file"));
        getButton(1)->setEnabled(false);
    }
    m_filePathEdit->setText("");
    setVerificationPage();
    DFMVaultPageBase::showEvent(event);
}