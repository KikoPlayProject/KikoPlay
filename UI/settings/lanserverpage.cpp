#include "lanserverpage.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QNetworkInterface>
#include <QIntValidator>
#include <QFont>
#include <QImage>
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "globalobjects.h"
#include "LANServer/lanserver.h"
#include "Common/qrcodegen.hpp"

namespace {
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

#define SETTING_KEY_SERVER_AUTOSTART "Server/AutoStart"
#define SETTING_KEY_SERVER_SYNC_PLAYTIME "Server/SyncPlayTime"
#define SETTING_KEY_SERVER_PORT "Server/Port"
#define SETTING_KEY_SERVER_AUTO_DLNA "Server/DLNAAutoStart"

LANServerPage::LANServerPage(QWidget *parent) : SettingPage(parent)
{
    SettingItemArea *serviceArea = new SettingItemArea(tr("Service"), this);

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

    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(serviceArea);
    itemVLayout->addWidget(addressContainer);
    itemVLayout->addStretch(1);


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

