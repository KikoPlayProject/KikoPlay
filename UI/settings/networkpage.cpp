#include "networkpage.h"
#include <QVBoxLayout>
#include <QSettings>
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaSpinBox.h"
#include "Common/network.h"
#include "globalobjects.h"

#define SETTING_KEY_PROXY_TYPE "Network/ProxyType"
#define SETTING_KEY_PROXY_HOST "Network/ProxyHost"
#define SETTING_KEY_PROXY_PORT "Network/ProxyPort"
#define SETTING_KEY_PROXY_USER "Network/ProxyUser"
#define SETTING_KEY_PROXY_PASSWORD "Network/ProxyPassword"

NetworkPage::NetworkPage(QWidget *parent) : SettingPage(parent)
{
    SettingItemArea *proxyArea = new SettingItemArea(tr("Proxy"), this);

    ElaComboBox *proxyTypeCombo = new ElaComboBox(this);
    proxyTypeCombo->addItem(tr("No Proxy"), 0);
    proxyTypeCombo->addItem(tr("System Proxy"), 1);
    proxyTypeCombo->addItem(tr("HTTP Proxy"), 2);
    proxyTypeCombo->addItem(tr("SOCKS5 Proxy"), 3);
    proxyTypeCombo->setCurrentIndex(GlobalObjects::appSetting->value(SETTING_KEY_PROXY_TYPE, 1).toInt());
    proxyArea->addItem(tr("Proxy Type"), proxyTypeCombo);

    SettingItemArea *detailArea = new SettingItemArea(tr("Proxy Server"), this);

    ElaLineEdit *hostEdit = new ElaLineEdit(this);
    hostEdit->setText(GlobalObjects::appSetting->value(SETTING_KEY_PROXY_HOST).toString());
    hostEdit->setPlaceholderText(tr("Host"));
    detailArea->addItem(tr("Host"), hostEdit);

    ElaSpinBox *portSpin = new ElaSpinBox(this);
    portSpin->setRange(0, 65535);
    portSpin->setValue(GlobalObjects::appSetting->value(SETTING_KEY_PROXY_PORT, 0).toInt());
    detailArea->addItem(tr("Port"), portSpin);

    ElaLineEdit *userEdit = new ElaLineEdit(this);
    userEdit->setText(GlobalObjects::appSetting->value(SETTING_KEY_PROXY_USER).toString());
    userEdit->setPlaceholderText(tr("Optional"));
    detailArea->addItem(tr("Username"), userEdit);

    ElaLineEdit *passwordEdit = new ElaLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setText(GlobalObjects::appSetting->value(SETTING_KEY_PROXY_PASSWORD).toString());
    passwordEdit->setPlaceholderText(tr("Optional"));
    detailArea->addItem(tr("Password"), passwordEdit);

    detailArea->setVisible(proxyTypeCombo->currentIndex() >= 2);

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(8);
    vLayout->addWidget(proxyArea);
    vLayout->addWidget(detailArea);
    vLayout->addStretch(1);

    auto applyProxy = [=]() {
        GlobalObjects::appSetting->setValue(SETTING_KEY_PROXY_TYPE, proxyTypeCombo->currentIndex());
        GlobalObjects::appSetting->setValue(SETTING_KEY_PROXY_HOST, hostEdit->text().trimmed());
        GlobalObjects::appSetting->setValue(SETTING_KEY_PROXY_PORT, portSpin->value());
        GlobalObjects::appSetting->setValue(SETTING_KEY_PROXY_USER, userEdit->text().trimmed());
        GlobalObjects::appSetting->setValue(SETTING_KEY_PROXY_PASSWORD, passwordEdit->text());
        Network::applyProxySetting();
    };

    QObject::connect(proxyTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index) {
        detailArea->setVisible(index >= 2);
        applyProxy();
    });

    QObject::connect(hostEdit, &ElaLineEdit::editingFinished, this, applyProxy);
    QObject::connect(portSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int) { applyProxy(); });
    QObject::connect(userEdit, &ElaLineEdit::editingFinished, this, applyProxy);
    QObject::connect(passwordEdit, &ElaLineEdit::editingFinished, this, applyProxy);
}
