#include "dlnadiscover.h"
#include <QTreeView>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QGridLayout>
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QUrl>
#include <QSettings>
#include <QUdpSocket>
#include "LANServer/dlna/upnpctrlpoint.h"
#include "LANServer/dlna/dlnamediacontroller.h"
#include "Play/Playlist/playlistitem.h"
#include "globalobjects.h"

DLNADiscover::DLNADiscover(const PlayListItem *item, QWidget *parent) : CFramelessDialog(tr("Select DLNA Device"), parent, true)
{
    QPushButton *refreshDevice = new QPushButton(tr("Refresh"), this);
    deviceView = new QTreeView(this);
    deviceView->setRootIsDecorated(false);
    deviceView->setAlternatingRowColors(true);

    QGridLayout *discoverGLayout = new QGridLayout(this);
    discoverGLayout->addWidget(refreshDevice, 0, 0);
    discoverGLayout->addWidget(deviceView, 1, 0, 1, 3);
    discoverGLayout->setRowStretch(1, 1);
    discoverGLayout->setColumnStretch(1, 1);
    discoverGLayout->setContentsMargins(0, 0, 0, 0);

    UPnPCtrlPoint *ctrlPoint = new UPnPCtrlPoint(this);
    deviceView->setModel(ctrlPoint);
    playItemFunc = [=](const QModelIndex &index){
        QSharedPointer<UPnPDevice> device = ctrlPoint->getDevice(index);
        DLNAMediaController controller(device);
        QHostAddress deviceIP = QHostAddress(QUrl(device->getLocation()).host());
        QUdpSocket tmpSocket;
        tmpSocket.connectToHost(deviceIP, 1900);
        QHostAddress httpServerIP = tmpSocket.localAddress();
        /*
        bool findIP = false;
        for(QNetworkInterface interface : QNetworkInterface::allInterfaces())
        {
            if(interface.flags().testFlag(QNetworkInterface::IsUp))
            {
                for(QNetworkAddressEntry entry : interface.addressEntries())
                {
                    if(entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                    {
                        if(entry.ip().isInSubnet(deviceIP, entry.prefixLength()))
                        {
                            httpServerIP = entry.ip();
                            findIP = true;
                            break;
                        }
                    }
                }
                if(findIP) break;
            }
        }
        if(!findIP) return false;
        */
        showBusyState(true);
        refreshDevice->setEnabled(false);
        QString port = QString::number(GlobalObjects::appSetting->value("Server/Port", 8000).toUInt());
        QString pathHash(QCryptographicHash::hash(item->path.toUtf8(),QCryptographicHash::Md5).toHex());
        QString suffix = item->path.mid(item->path.lastIndexOf('.') + 1);
        QString mediaURI = QString("http://%1:%2/media/%3.%4").arg(httpServerIP.toString(), port, pathHash, suffix);
        controller.setAVTransportURI(mediaURI);
        controller.play();
        if(item->playTimeState == PlayListItem::UNFINISH)
        {
            controller.seek(item->playTime);
        }
        deviceName = device->getFriendlyName();
        refreshDevice->setEnabled(true);
        showBusyState(false);
        return true;
    };
    QObject::connect(refreshDevice, &QPushButton::clicked, this, [=](){
        showBusyState(true);
        refreshDevice->setEnabled(false);
        ctrlPoint->search(AVTransportService);
        refreshDevice->setEnabled(true);
        showBusyState(false);
    });
    QObject::connect(deviceView, &QTreeView::doubleClicked, [=](const QModelIndex &index){
        if(playItemFunc(index))
        {
            CFramelessDialog::onAccept();
        }
    });
    setSizeSettingKey("DialogSize/DLNADiscover",QSize(400*logicalDpiX()/96,220*logicalDpiY()/96));
    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/DeviceView"));
    if(!headerState.isNull())
        deviceView->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([this](){
        GlobalObjects::appSetting->setValue("HeaderViewState/DeviceView", deviceView->header()->saveState());
    });
    QTimer::singleShot(0, [=](){
        refreshDevice->clicked();
    });
}

void DLNADiscover::onAccept()
{
    auto selection = deviceView->selectionModel()->selectedRows();
    if(selection.size()==0) return;
    if(playItemFunc(selection.first()))
    {
        CFramelessDialog::onAccept();
    }
}
