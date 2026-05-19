#include "networkpage.h"
#include <QVBoxLayout>
#include <QSettings>
#include <QLabel>
#include <QNetworkInterface>
#include "LANServer/lanserver.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaSpinBox.h"
#include "Common/network.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "globalobjects.h"
#include "Common/qrcodegen.hpp"
#include "globalobjects.h"


#define SETTING_KEY_PROXY_TYPE "Network/ProxyType"
#define SETTING_KEY_PROXY_HOST "Network/ProxyHost"
#define SETTING_KEY_PROXY_PORT "Network/ProxyPort"
#define SETTING_KEY_PROXY_USER "Network/ProxyUser"
#define SETTING_KEY_PROXY_PASSWORD "Network/ProxyPassword"
#define SETTING_KEY_SERVER_AUTOSTART "Server/AutoStart"
#define SETTING_KEY_SERVER_SYNC_PLAYTIME "Server/SyncPlayTime"
#define SETTING_KEY_SERVER_PORT "Server/Port"
#define SETTING_KEY_SERVER_AUTO_DLNA "Server/DLNAAutoStart"


namespace
{
    QPixmap generateQrPixmap(const QString &text, int modulePixelSize = 4)
    {
        using namespace qrcodegen;
        QrCode qr = QrCode::encodeText(text.toUtf8().constData(), QrCode::Ecc::MEDIUM);
        int size = qr.getSize();
        int imageSize = size * modulePixelSize;
        QImage image(imageSize, imageSize, QImage::Format_RGB32);
        image.fill(Qt::white);
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                if (qr.getModule(x, y)) {
                    for (int dy = 0; dy < modulePixelSize; dy++) {
                        for (int dx = 0; dx < modulePixelSize; dx++) {
                            image.setPixel(x * modulePixelSize + dx, y * modulePixelSize + dy, qRgb(0, 0, 0));
                        }
                    }
                }
            }
        }
        return QPixmap::fromImage(image);
    }
}

NetworkPage::NetworkPage(QWidget *parent) : SettingPage(parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    initProxyArea(itemVLayout);
    initLANServerArea(itemVLayout);
    itemVLayout->addStretch(1);
}

void NetworkPage::initProxyArea(QVBoxLayout *vLayout)
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

    vLayout->addWidget(proxyArea);
    vLayout->addWidget(detailArea);

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

void NetworkPage::initLANServerArea(QVBoxLayout *vLayout)
{
    SettingItemArea *serviceArea = new SettingItemArea(tr("LAN Service"), this);

    ElaToggleSwitch *serviceSwitch = new ElaToggleSwitch(this);
    serviceSwitch->setIsToggled(GlobalObjects::lanServer->isStart());
    serviceArea->addItem(tr("Start Server"), serviceSwitch);

    ElaLineEdit *portEdit = new ElaLineEdit(this);
    QIntValidator *portValidator = new QIntValidator(this);
    portValidator->setRange(1, 65535);
    portEdit->setValidator(portValidator);
    portEdit->setText(QString::number(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_PORT, 8000).toUInt()));
    portEdit->setEnabled(!GlobalObjects::lanServer->isStart());
    serviceArea->addItem(tr("Port"), portEdit);

    ElaToggleSwitch *syncUpdateTimeSwitch = new ElaToggleSwitch(this);
    syncUpdateTimeSwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_SYNC_PLAYTIME, true).toBool());
    serviceArea->addItem(tr("Synchronous Playback Time"), syncUpdateTimeSwitch);

    ElaToggleSwitch *startDLNASwitch = new ElaToggleSwitch(this);
    startDLNASwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_AUTO_DLNA, true).toBool());
    startDLNASwitch->setEnabled(serviceSwitch->getIsToggled());
    serviceArea->addItem(tr("Start DLNA Media Server"), startDLNASwitch);

    QLabel *addressTip = new QLabel(this);
    addressTip->setTextInteractionFlags(Qt::TextBrowserInteraction);
    addressTip->setOpenExternalLinks(true);

    QLabel *qrLabel = new QLabel(this);

    auto refreshTips = [=](){
        QStringList tips;
        QStringList qrAddresses;
        tips << QString("<h2>%1</h2>").arg(tr("KikoPlay Server Address"));
        for (QHostAddress &address : QNetworkInterface::allAddresses())
        {
            if (address.protocol() == QAbstractSocket::IPv4Protocol)
            {
                QString addr = address.toString();
                tips << QString("<p style='color: rgb(220,220,220); font-size: 16px;'><a style='color: rgb(96, 208, 252);' href=\"http://%1:%2/\">%1</a></p>").arg(addr, portEdit->text());
                if (address != QHostAddress::LocalHost)
                {
                    qrAddresses << QString("http://%1:%2/").arg(addr, portEdit->text());
                }
            }
        }
        addressTip->setText(tips.join('\n'));
        if (!qrAddresses.isEmpty())
        {
            qrLabel->setPixmap(generateQrPixmap(qrAddresses.join('\n')));
            qrLabel->setToolTip(qrAddresses.join('\n'));
        }
        else
        {
            qrLabel->clear();
        }
    };
    refreshTips();

    QHBoxLayout *addressLayout = new QHBoxLayout;
    addressLayout->setSpacing(16);
    addressLayout->addWidget(addressTip);
    addressLayout->addStretch(1);
    addressLayout->addWidget(qrLabel);

    QWidget *addressContainer = new QWidget(this);
    addressContainer->setObjectName(QStringLiteral("SettingItemAreaContainer"));
    addressContainer->setLayout(addressLayout);
    addressContainer->setVisible(serviceSwitch->getIsToggled());

    vLayout->addWidget(serviceArea);
    vLayout->addWidget(addressContainer);


    QObject::connect(serviceSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        if (checked)
        {
            quint16 port = qBound<quint16>(1, portEdit->text().toUInt(), 65535);
            portEdit->setText(QString::number(port));
            serviceSwitch->setIsToggled(GlobalObjects::lanServer->startServer(port));
            portEdit->setEnabled(!serviceSwitch->getIsToggled());
            startDLNASwitch->setEnabled(true);
            refreshTips();
            addressContainer->show();
            if (serviceSwitch->getIsToggled() && startDLNASwitch->getIsToggled())
            {
                GlobalObjects::lanServer->startDLNA();
            }
        }
        else
        {
            GlobalObjects::lanServer->stopServer();
            GlobalObjects::lanServer->stopDLNA();
            portEdit->setEnabled(true);
            addressContainer->hide();
            startDLNASwitch->setEnabled(false);
        }
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_AUTOSTART, checked);
    });

    QObject::connect(syncUpdateTimeSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_SYNC_PLAYTIME, checked);
    });

    QObject::connect(startDLNASwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        if (checked)
        {
            if (serviceSwitch->getIsToggled())
            {
                GlobalObjects::lanServer->startDLNA();
            }
        }
        else
        {
            GlobalObjects::lanServer->stopDLNA();
        }
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_AUTO_DLNA, checked);
    });

    QObject::connect(portEdit, &QLineEdit::textChanged, this, [=](const QString &port){
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_PORT, port.toUInt());
    });
}
