#include "dlnamediaserver.h"
#include <QSysInfo>
#include "dlnamediaitem.h"
#include "Common/threadtask.h"
#include "Common/logger.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"

DLNAMediaServer::DLNAMediaServer() : UPnPDevice()
{
    server = QString("%1/%2 UPnP/1.1 KikoPlay/%3").arg(QSysInfo::productType(), QSysInfo::productVersion(), GlobalObjects::kikoVersion).toUtf8();
    deviceType = "urn:schemas-upnp-org:device:MediaServer:1";
    friendlyName = "KikoPlay Media Server";
    manufacturer = "KikoPlay Project";
    manufacturerURL = "https://kikoplayproject.github.io/";
    modelDescription = "KikoPlay DLNA Playlist Media Server, by Kikyou";
    modelName = "KikoPlay";
    modelURL = manufacturerURL;
    initContentDirectoryService();
    initConnectionManagerService();
}


void DLNAMediaServer::onAction(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    using Func = void(DLNAMediaServer::*)(UPnPAction &, stefanfrings::HttpRequest &, stefanfrings::HttpResponse &);
    static QMap<QString, Func> actionTable = {
        {"Browse", &DLNAMediaServer::onBrowse},
        {"GetSystemUpdateID", &DLNAMediaServer::onGetSystemUpdateID},
        {"GetSortCapabilities", &DLNAMediaServer::onGetSortCapabilities},
        {"GetSearchCapabilities", &DLNAMediaServer::onGetSearchCapabilities},
        {"GetCurrentConnectionIDs", &DLNAMediaServer::onGetCurrentConnectionIDs},
        {"GetProtocolInfo", &DLNAMediaServer::onGetProtocolInfo},
        {"GetCurrentConnectionInfo", &DLNAMediaServer::onGetCurrentConnectionInfo}
    };
    auto actionFunc = actionTable.value(action.desc.name, nullptr);
    if(!actionFunc)
    {
        response.setStatus(stefanfrings::HttpResponse::BadRequest);
        return;
    }
    response.setHeader("Content-Type", "text/xml; charset=\"utf-8\"");
    response.setHeader("Server", this->server);
    (this->*actionFunc)(action, request, response);
    Logger::logger()->log(Logger::LANServer, QString("[DLNA][%1]OnAction: %2, status: %3").arg(request.getPeerAddress().toString(), action.desc.name, QString::number(action.errCode)));
}

void DLNAMediaServer::initContentDirectoryService()
{
    services["ContentDirectory"] = QSharedPointer<UPnPService>::create();
    UPnPService &contentDirectory = *services["ContentDirectory"];
    contentDirectory.serviceType = "urn:schemas-upnp-org:service:ContentDirectory:1";
    contentDirectory.serviceId = "urn:upnp-org:serviceId:ContentDirectory";
    contentDirectory.SCPDURL = "ContentDirectory/scpd.xml";
    contentDirectory.controlURL = "ContentDirectory/control.xml";
    contentDirectory.eventSubURL = "ContentDirectory/event.xml";

    UPnPStateVariable &svBrowseFlag = contentDirectory.addStateVariable("A_ARG_TYPE_BrowseFlag", "string");
    svBrowseFlag.allowedValues = QStringList{"BrowseMetadata", "BrowseDirectChildren"};
    UPnPStateVariable &svContainerUpdateIDs = contentDirectory.addStateVariable("ContainerUpdateIDs", "string");
    svContainerUpdateIDs.sendEvent = true;
    UPnPStateVariable &svSystemUpdateId = contentDirectory.addStateVariable("SystemUpdateID", "ui4");
    svSystemUpdateId.sendEvent = true;
    svSystemUpdateId.value = "0";
    UPnPStateVariable &svArgCount = contentDirectory.addStateVariable("A_ARG_TYPE_Count", "ui4");
    UPnPStateVariable &svArgSortCriteria = contentDirectory.addStateVariable("A_ARG_TYPE_SortCriteria", "string");
    UPnPStateVariable &svSortCapabilities = contentDirectory.addStateVariable("SortCapabilities", "string");
    UPnPStateVariable &svArgIndex = contentDirectory.addStateVariable("A_ARG_TYPE_Index", "ui4");
    UPnPStateVariable &svArgObjID = contentDirectory.addStateVariable("A_ARG_TYPE_ObjectID", "string");
    UPnPStateVariable &svArgUpdateID = contentDirectory.addStateVariable("A_ARG_TYPE_UpdateID", "ui4");
    UPnPStateVariable &svArgResult = contentDirectory.addStateVariable("A_ARG_TYPE_Result", "string");
    UPnPStateVariable &svSearchCapabilities = contentDirectory.addStateVariable("SearchCapabilities", "string");
    UPnPStateVariable &svFilter = contentDirectory.addStateVariable("A_ARG_TYPE_Filter", "string");

    UPnPActionDesc &adBrowse = contentDirectory.addActionDesc("Browse");
    adBrowse.addArgDesc("ObjectID", svArgObjID, UPnPArgDesc::IN);
    adBrowse.addArgDesc("BrowseFlag", svBrowseFlag, UPnPArgDesc::IN);
    adBrowse.addArgDesc("Filter", svFilter, UPnPArgDesc::IN);
    adBrowse.addArgDesc("StartingIndex", svArgIndex, UPnPArgDesc::IN);
    adBrowse.addArgDesc("RequestedCount", svArgCount, UPnPArgDesc::IN);
    adBrowse.addArgDesc("SortCriteria", svArgSortCriteria, UPnPArgDesc::IN);
    adBrowse.addArgDesc("Result", svArgResult, UPnPArgDesc::OUT);
    adBrowse.addArgDesc("NumberReturned", svArgCount, UPnPArgDesc::OUT);
    adBrowse.addArgDesc("TotalMatches", svArgCount, UPnPArgDesc::OUT);
    adBrowse.addArgDesc("UpdateID", svArgUpdateID, UPnPArgDesc::OUT);

    UPnPActionDesc &adGetSortCaps = contentDirectory.addActionDesc("GetSortCapabilities");
    adGetSortCaps.addArgDesc("SortCaps", svSortCapabilities, UPnPArgDesc::OUT);

    UPnPActionDesc &adGetSystemUpdateID = contentDirectory.addActionDesc("GetSystemUpdateID");
    adGetSystemUpdateID.addArgDesc("Id", svSystemUpdateId, UPnPArgDesc::OUT);

    UPnPActionDesc &adGetSearchCaps = contentDirectory.addActionDesc("GetSearchCapabilities");
    adGetSearchCaps.addArgDesc("SearchCaps", svSearchCapabilities, UPnPArgDesc::OUT);
}

void DLNAMediaServer::initConnectionManagerService()
{
    services["ConnectionManager"] = QSharedPointer<UPnPService>::create();
    UPnPService &connectionManager = *services["ConnectionManager"];
    connectionManager.serviceType = "urn:schemas-upnp-org:service:ConnectionManager:1";
    connectionManager.serviceId = "urn:upnp-org:serviceId:ConnectionManager";
    connectionManager.SCPDURL = "ConnectionManager/scpd.xml";
    connectionManager.controlURL = "ConnectionManager/control.xml";
    connectionManager.eventSubURL = "ConnectionManager/event.xml";

    UPnPStateVariable &svArgProtocol = connectionManager.addStateVariable("A_ARG_TYPE_ProtocolInfo", "string");
    UPnPStateVariable &svArgConnStatus = connectionManager.addStateVariable("A_ARG_TYPE_ConnectionStatus", "string");
    svArgConnStatus.allowedValues = QStringList({"OK", "ContentFormatMismatch", "InsufficientBandwidth", "UnreliableChannel", "Unknown"});
    UPnPStateVariable &svArgTransID = connectionManager.addStateVariable("A_ARG_TYPE_AVTransportID", "i4");
    UPnPStateVariable &svArgRcsID = connectionManager.addStateVariable("A_ARG_TYPE_RcsID", "i4");
    UPnPStateVariable &svArgConnID = connectionManager.addStateVariable("A_ARG_TYPE_ConnectionID", "i4");
    UPnPStateVariable &svArgConnManager = connectionManager.addStateVariable("A_ARG_TYPE_ConnectionManager", "string");
    UPnPStateVariable &svSrcProtocolInfo = connectionManager.addStateVariable("SourceProtocolInfo", "string");
    svSrcProtocolInfo.sendEvent = true;
    svSrcProtocolInfo.value = "http-get:*:*:*";
    UPnPStateVariable &svSinkProtocolInfo = connectionManager.addStateVariable("SinkProtocolInfo", "string");
    svSinkProtocolInfo.sendEvent = true;
    UPnPStateVariable &svArgDirection = connectionManager.addStateVariable("A_ARG_TYPE_Direction", "string");
    svArgDirection.allowedValues = QStringList({"Input", "Output"});
    UPnPStateVariable &svCurConnIDs = connectionManager.addStateVariable("CurrentConnectionIDs", "string");
    svCurConnIDs.sendEvent = true;
    svCurConnIDs.value = "0";

    UPnPActionDesc &adGetCurConnInfo = connectionManager.addActionDesc("GetCurrentConnectionInfo");
    adGetCurConnInfo.addArgDesc("ConnectionID", svArgConnID, UPnPArgDesc::IN);
    adGetCurConnInfo.addArgDesc("RcsID", svArgRcsID, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("AVTransportID", svArgTransID, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("ProtocolInfo", svArgProtocol, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("PeerConnectionManager", svArgConnManager, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("PeerConnectionID", svArgConnID, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("Direction", svArgDirection, UPnPArgDesc::OUT);
    adGetCurConnInfo.addArgDesc("Status", svArgConnStatus, UPnPArgDesc::OUT);

    UPnPActionDesc &adGetProtocolInfo = connectionManager.addActionDesc("GetProtocolInfo");
    adGetProtocolInfo.addArgDesc("Source", svSrcProtocolInfo, UPnPArgDesc::OUT);
    adGetProtocolInfo.addArgDesc("Sink", svSinkProtocolInfo, UPnPArgDesc::OUT);

    UPnPActionDesc &adGetCurConnIDs = connectionManager.addActionDesc("GetCurrentConnectionIDs");
    adGetCurConnIDs.addArgDesc("Source", svCurConnIDs, UPnPArgDesc::OUT);
}

void DLNAMediaServer::onBrowse(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    const QString &browseFlag = action.argMap["BrowseFlag"]->val;
    if(browseFlag == "BrowseMetadata")
    {
        onBrowseMeta(action, request, response);
    }
    else if(browseFlag == "BrowseDirectChildren")
    {
        onBrowseDirectChildren(action, request, response);
    }
    else
    {
        action.errCode = 402;
        action.errDesc = "Invalid args";
    }
    QByteArray resposeBuffer;
    if(action.errCode != 0)
    {
        response.setStatus(stefanfrings::HttpResponse::InternalServerError, "Internal Server Error");
        action.writeErrorResponse(resposeBuffer);
    }
    else
    {
        action.writeResponse(resposeBuffer);
    }
    response.write(resposeBuffer, true);
}

void DLNAMediaServer::onGetSystemUpdateID(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::onGetSortCapabilities(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::onGetSearchCapabilities(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::onGetCurrentConnectionIDs(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::onGetProtocolInfo(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::onGetCurrentConnectionInfo(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    action.setArg("RcsID", "-1");
    action.setArg("AVTransportID", "-1");
    action.setArg("ProtocolInfo", "http-get:*:*:*");
    action.setArg("PeerConnectionManager", "/");
    action.setArg("PeerConnectionID", "-1");
    action.setArg("Direction", "Output");
    action.setArg("Status", "Unknown");
    setDefaultActionResponse(action, response);
}

void DLNAMediaServer::writeDIDL(QByteArray &buffer, const QVector<DLNAMediaItem> &mediaItems, quint32 mask)
{
    QXmlStreamWriter writer(&buffer);
    writer.writeStartElement("DIDL-Lite");
    writer.writeDefaultNamespace("urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/");
    writer.writeNamespace("http://purl.org/dc/elements/1.1/", "dc");
    writer.writeNamespace("urn:schemas-upnp-org:metadata-1-0/upnp/", "upnp");
    writer.writeNamespace("urn:schemas-dlna-org:metadata-1-0/", "dlna");
    for(const DLNAMediaItem &item : mediaItems)
        item.toDidl(writer, mask);
    writer.writeEndElement();
}

void DLNAMediaServer::onBrowseMeta(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    QString objId = action.argMap["ObjectID"]->val;
    if(objId.isEmpty())
    {
        action.errCode = 402;
        action.errDesc = "Invalid args";
        return;
    }
    DLNAMediaItem mediaItem;
    QString filePath;
    bool pathErr = false;
    QMetaObject::invokeMethod(GlobalObjects::playlist, [&]()->bool{
        const PlayListItem *item = GlobalObjects::playlist->getPathItem(objId);
        if(!item) return true;
        mediaItem.title = item->parent? item->title : "Root";
        mediaItem.objID = objId;
        mediaItem.type = item->children? DLNAMediaItem::Container : DLNAMediaItem::Item;
        mediaItem.childSize = item->children? item->children->size() : 0;
        mediaItem.parentID = "-1";
        if(objId != "0")
        {
            int sepPos = objId.lastIndexOf('_');
            assert(sepPos != -1);
            mediaItem.parentID = objId.mid(0, sepPos);
        }
        filePath = mediaItem.type == DLNAMediaItem::Item? item->path : "";
        return false;
    },Qt::BlockingQueuedConnection, &pathErr);
    if(pathErr)
    {
        action.errCode = 800;
        action.errDesc = "Internal error";
        return;
    }
    if(mediaItem.type == DLNAMediaItem::Item)
    {
        quint16 port = GlobalObjects::appSetting->value("Server/Port", 8000).toUInt();
        mediaItem.setResource(filePath, response.getLocalAddress(), port);
        QFileInfo info(filePath);
        mediaItem.fileSize = info.size();
    }
    QByteArray result;
    QString filter = action.argMap["Filter"]->val;
    writeDIDL(result, {mediaItem}, DLNAMediaItem::getMask(filter));
    action.setArg("Result", result);
    action.setArg("NumberReturned", "1");
    action.setArg("TotalMatches", "1");
    action.setArg("UpdateID", "1");
    Logger::logger()->log(Logger::LANServer, QString("[DLNA][%1]BrowseMeta: %2, filter: %3").arg(request.getPeerAddress().toString(), mediaItem.title, filter));
}

void DLNAMediaServer::onBrowseDirectChildren(UPnPAction &action, stefanfrings::HttpRequest &request, stefanfrings::HttpResponse &response)
{
    Q_UNUSED(request);
    QString objId = action.argMap["ObjectID"]->val;
    if(objId.isEmpty())
    {
        action.errCode = 402;
        action.errDesc = "Invalid args";
        return;
    }
    QVector<DLNAMediaItem> mediaItems;
    QVector<QString> filePaths;
    bool pathErr = false;
    int startIndex = action.argMap["StartingIndex"]->val.toInt();
    int requestCount = action.argMap["RequestedCount"]->val.toInt();
    int totalMatches = 0;
    if(startIndex < 0 || requestCount < 0)
    {
        action.errCode = 402;
        action.errDesc = "Invalid args";
        return;
    }
    QString fullPath;
    QMetaObject::invokeMethod(GlobalObjects::playlist, [&]()->bool{
        const PlayListItem *item = GlobalObjects::playlist->getPathItem(objId);
        if(!item || !item->children) return true;
        QStringList pathStack;
        const PlayListItem *tmpItem = item;
        while(tmpItem)
        {
            pathStack.push_front(tmpItem->title);
            tmpItem = tmpItem->parent;
        }
        fullPath = pathStack.join('/');
        if(objId == "0") fullPath = "/";
        const QVector<PlayListItem *> &children = *item->children;
        totalMatches = children.size();
        if (children.size() == 0 || startIndex >= children.size()) return false;
        totalMatches = children.size() - startIndex;
        int endIndex = requestCount == 0? children.size() : startIndex + requestCount;
        endIndex = qMin(endIndex, children.size());
        mediaItems.reserve(endIndex - startIndex);
        filePaths.reserve(endIndex - startIndex);
        for(int i = startIndex; i < endIndex; ++i)
        {
            mediaItems.append(DLNAMediaItem());
            DLNAMediaItem &curItem = mediaItems.back();
            curItem.title = children[i]->title;
            curItem.objID = objId + "_" + QString::number(i);
            curItem.type = children[i]->children? DLNAMediaItem::Container : DLNAMediaItem::Item;
            curItem.parentID = objId;
            filePaths.append(curItem.type == DLNAMediaItem::Item? children[i]->path : "");
        }
        return false;
    },Qt::BlockingQueuedConnection, &pathErr);
    if(pathErr)
    {
        action.errCode = 800;
        action.errDesc = "Internal error";
        return;
    }
    quint16 port = GlobalObjects::appSetting->value("Server/Port", 8000).toUInt();
    for(int i = 0; i < mediaItems.size(); ++i)
    {
        if(mediaItems[i].type == DLNAMediaItem::Item)
        {
            mediaItems[i].setResource(filePaths[i], response.getLocalAddress(), port);
            QFileInfo info(filePaths[i]);
            mediaItems[i].fileSize = info.size();
        }
    }
    QByteArray result;
    QString filter = action.argMap["Filter"]->val;
    writeDIDL(result, mediaItems, DLNAMediaItem::getMask(filter));
    action.setArg("Result", result);
    action.setArg("NumberReturned", QString::number(mediaItems.size()));
    action.setArg("TotalMatches", QString::number(totalMatches));
    action.setArg("UpdateID", "1");
    Logger::logger()->log(Logger::LANServer, QString("[DLNA][%1]BrowseDirectChildren: %2, filter: %3, start: %4, requset count: %5")
                          .arg(request.getPeerAddress().toString(), fullPath, filter, QString::number(startIndex), QString::number(requestCount)));
}

void DLNAMediaServer::setDefaultActionResponse(UPnPAction &action, stefanfrings::HttpResponse &response)
{
    QByteArray resposeBuffer;
    action.fillStateVariableArg();
    action.writeResponse(resposeBuffer);
    response.write(resposeBuffer, true);
}
