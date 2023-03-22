#ifndef DLNAMEDIASERVER_H
#define DLNAMEDIASERVER_H

#include "upnpdevice.h"
struct DLNAMediaItem;
class DLNAMediaServer : public UPnPDevice
{
public:
    DLNAMediaServer();

    // UPnPDevice interface
protected:
    virtual void onAction(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    void initContentDirectoryService();
    void initConnectionManagerService();

    void onBrowse(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onGetSystemUpdateID(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onGetSortCapabilities(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onGetSearchCapabilities(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);

    void onGetCurrentConnectionIDs(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onGetProtocolInfo(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onGetCurrentConnectionInfo(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    void writeDIDL(QByteArray &buffer,  const QVector<DLNAMediaItem> &mediaItems, quint32 mask);
    void onBrowseMeta(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void onBrowseDirectChildren(UPnPAction &action, stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void setDefaultActionResponse(UPnPAction &action, stefanfrings::HttpResponse& response);
};

#endif // DLNAMEDIASERVER_H
