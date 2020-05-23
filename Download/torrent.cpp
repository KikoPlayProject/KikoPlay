#include "torrent.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include <QBrush>
namespace
{
    struct BEncodeItem
    {
        enum Type{BInt,BString,BList,BDict};
        Type type;
        virtual ~BEncodeItem(){}
    };
    struct BEncodeInt : public BEncodeItem
    {
        BEncodeInt(const QByteArray &content, int &pos)
        {
            type=BInt;
			value = 0;
            pos++;
            char numChar=content[pos++];
            bool neg = false;
            if (numChar == '-')
            {
                neg = true;
                numChar=content[pos++];
            }
            while (numChar >= '0' && numChar <= '9')
            {
                value = value * 10 + (numChar - '0');
                numChar=content[pos++];
            }
            if (neg) value = -value;
        }
        qint64 value;
    };
    struct BEncodeStr : public BEncodeItem
    {
        BEncodeStr(const QByteArray &content, int &pos)
        {
            type=BString;
            char numChar;
            numChar=content[pos++];
            length=0;
            while (numChar >= '0' && numChar <= '9')
            {
                length = length * 10 + (numChar - '0');
                numChar=content[pos++];
            }
            value=QString(content.mid(pos,length));
            pos+=length;
        }
        QString value;
        int length;
    };
    struct BEncodeList : public BEncodeItem
    {
        BEncodeList()
        {
            type=BList;
        }
        ~BEncodeList()
        {
            qDeleteAll(items);
        }
        QList<BEncodeItem *> items;
    };
    struct BEncodeDict : public BEncodeItem
    {
        BEncodeDict()
        {
            type=BDict;
        }
        ~BEncodeDict()
        {
            for(auto iter=itemDict.cbegin();iter!=itemDict.cend();iter++)
                delete (*iter).second;
        }
        QList<QPair<QString,BEncodeItem*>> itemDict;
        QString currentKey;
    };
    void appendBEItem(BEncodeItem *parent, BEncodeItem *item)
    {
        if(parent->type==BEncodeItem::BList)
        {
            static_cast<BEncodeList *>(parent)->items.append(item);
        }
        else if(parent->type==BEncodeItem::BDict)
        {
            BEncodeDict *dict=static_cast<BEncodeDict *>(parent);
            if(dict->currentKey.isEmpty())
            {
                if(item->type==BEncodeItem::BString)
                    dict->currentKey=static_cast<BEncodeStr *>(item)->value;
            }
            else
            {
                dict->itemDict.append(QPair<QString,BEncodeItem *>(dict->currentKey,item));
                dict->currentKey=QString();
            }
        }
    }
    void insertFile(BEncodeList *pathList, TorrentFile *root, TorrentFile *fileToInsert)
    {
        TorrentFile *currentPath=root;
        for(BEncodeItem *pathItem:pathList->items)
        {
            bool contains=false;
            for(TorrentFile *path:currentPath->children)
            {
                if(path->name==static_cast<BEncodeStr *>(pathItem)->value)
                {
                    contains=true;
                    currentPath=path;
                    break;
                }
            }
            if(!contains)
            {
                if(pathItem==pathList->items.last())
                {
                    currentPath->children.append(fileToInsert);
                    fileToInsert->parent=currentPath;
                    fileToInsert->name=static_cast<BEncodeStr *>(pathItem)->value;
                    return;
                }
                else
                {
                    TorrentFile *pathFile=new TorrentFile();
                    pathFile->checkStatus=Qt::Checked;
                    currentPath->children.append(pathFile);
                    pathFile->name=static_cast<BEncodeStr *>(pathItem)->value;
                    pathFile->parent=currentPath;
                    currentPath=pathFile;
                }
            }
        }
    }
    void buildFileTree(BEncodeDict *infoDict,TorrentFile *root)
    {
        root->checkStatus=Qt::Checked;
        for(auto iter=infoDict->itemDict.cbegin();iter!=infoDict->itemDict.cend();++iter)
        {
            if((*iter).first=="name")
            {
                if((*iter).second->type==BEncodeItem::BString)
                    root->name=static_cast<BEncodeStr *>((*iter).second)->value;
            }
            else if((*iter).first=="length")
            {
                if((*iter).second->type==BEncodeItem::BInt)
                {
                    root->size=static_cast<BEncodeInt *>((*iter).second)->value;
                    root->index=1;
                }
            }
            else if((*iter).first=="files")
            {
                if((*iter).second->type==BEncodeItem::BList)
                {
                    root->index=-1;
                    BEncodeList *fileList=static_cast<BEncodeList *>((*iter).second);
                    int index=1;
                    for(BEncodeItem *fileItem:fileList->items)
                    {
                        TorrentFile *file=new TorrentFile();
                        file->index=index++;
                        file->checkStatus=Qt::Checked;
                        BEncodeDict *fileDict=static_cast<BEncodeDict *>(fileItem);
                        for(auto iter=fileDict->itemDict.cbegin();iter!=fileDict->itemDict.cend();iter++)
                        {
                            if((*iter).first=="path")
                            {
                                insertFile(static_cast<BEncodeList *>((*iter).second),root,file);
                            }
                            else if((*iter).first=="length")
                            {
                                file->size=static_cast<BEncodeInt *>((*iter).second)->value;
                            }
                        }
                    }
                }
            }
        }
    }
}
TorrentDecoder::TorrentDecoder(const QString &torrentFile)
{
    QFile tf(torrentFile);
    bool ret=tf.open(QIODevice::ReadOnly);
    if(!ret) throw TorrentError(QObject::tr("Open File Failed: %1").arg(torrentFile));
    rawContent=tf.readAll();
    if(rawContent.length()==0) throw TorrentError(QObject::tr("Torrent Content Error"));
    decodeTorrent();
}

TorrentDecoder::TorrentDecoder(const QByteArray &torrentContent)
{
    rawContent=torrentContent;
    if(rawContent.length()==0) throw TorrentError(QObject::tr("Torrent Content Error"));
    decodeTorrent();
}

void TorrentDecoder::decodeTorrent()
{
    QStack<BEncodeItem *> contents;
    BEncodeItem *rootBEItem=nullptr;
    int i=0,infoStartPos=-1,infoEndPos=rawContent.length()-1;
    while (i<rawContent.length())
    {
        switch (rawContent[i])
        {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            BEncodeStr *beStr=new BEncodeStr(rawContent,i);
            if(contents.size()==0)
            {
                delete beStr;
                throw TorrentError(QObject::tr("Torrent Content Error"));
            }
            appendBEItem(contents.top(),beStr);
            if(contents.size()==1 && beStr->value=="info")
            {
                infoStartPos=i;
            }
            break;
        }
        case 'd':
        {
            BEncodeDict *beDict=new BEncodeDict();
            if(contents.size()>0)
            {
                appendBEItem(contents.top(),beDict);
            }
            contents.push(beDict);
            i++;
            break;
        }
        case 'l':
        {
            BEncodeList *beList=new BEncodeList();
            if(contents.size()>0)
            {
                appendBEItem(contents.top(),beList);
            }
            contents.push(beList);
            i++;
            break;
        }
        case 'i':
        {
            BEncodeInt *beInt=new BEncodeInt(rawContent,i);
            if(contents.size()==0)
            {
                delete beInt;
                throw TorrentError(QObject::tr("Torrent Content Error"));
            }
            appendBEItem(contents.top(),beInt);
            break;
        }
        case 'e':
        {
            rootBEItem=contents.pop();
            if(contents.size()==1 && contents.top()->type==BEncodeItem::BDict)
            {
                BEncodeDict *dict=static_cast<BEncodeDict *>(contents.top());
                if(dict->itemDict.last().first=="info")
                {
                    infoEndPos=i;
                }
            }
            i++;
            break;
        }
        default:
            break;
        }
    }
    root=new TorrentFile;
    root->size=0;
    root->parent=nullptr;
    if(rootBEItem->type!=BEncodeItem::BDict)
    {
        delete rootBEItem;
        throw TorrentError(QObject::tr("Torrent Content Error"));
    }

    BEncodeDict *rootDict=static_cast<BEncodeDict *>(rootBEItem);
    for(auto iter=rootDict->itemDict.cbegin();iter!=rootDict->itemDict.cend();++iter)
    {
        if((*iter).first=="info")
        {
            BEncodeDict *infoDict=static_cast<BEncodeDict *>((*iter).second);
            buildFileTree(infoDict,root);
            if(root->size>0)//single file
            {
                TorrentFile *tmpRoot=new TorrentFile();
                tmpRoot->parent=nullptr;
                tmpRoot->name=root->name;
                tmpRoot->checkStatus=Qt::Checked;
                tmpRoot->children.append(root);
                root->parent=tmpRoot;
                root=tmpRoot;
            }
            break;
        }
    }

    QByteArray infoField(rawContent.mid(infoStartPos,infoEndPos-infoStartPos+1));
    infoHash=QString(QCryptographicHash::hash(infoField,QCryptographicHash::Sha1).toHex());
    delete rootBEItem;
}

TorrentFileModel::TorrentFileModel(TorrentFile *rootFile, QObject *parent):QAbstractItemModel(parent),root(rootFile)
{

}

TorrentFileModel::~TorrentFileModel()
{

}

QString TorrentFileModel::getCheckedIndex()
{
    QList<TorrentFile *> items;
    items.push_back(root);
    QStringList indexes;
    while(!items.empty())
    {
        TorrentFile *currentItem=items.takeFirst();
        for(TorrentFile *child:currentItem->children)
        {
            if(child->children.count()>0)
                items.push_back(child);
            else if(child->index>0 && child->checkStatus==Qt::Checked)
                indexes.append(QString::number(child->index));
        }
    }
    return indexes.join(',');
}

void TorrentFileModel::checkAll(bool on)
{
    root->checkStatus=(on?Qt::Checked:Qt::Unchecked);
    root->setChildrenCheckStatus();
    for(int i=0;i<root->children.count();++i)
    {
        QModelIndex cIndex(index(i,0,QModelIndex()));
        emit dataChanged(cIndex,cIndex.sibling(cIndex.row(),2));
        refreshChildrenCheckStatus(cIndex);
    }
    emit checkedIndexChanged();
}

void TorrentFileModel::checkVideoFiles(bool on)
{
    root->checkStatus=Qt::Unchecked;
    root->setChildrenCheckStatus();
    QList<TorrentFile *> fileList;
    fileList.append(root);
    while(!fileList.isEmpty())
    {
        TorrentFile *file=fileList.takeFirst();
        for(TorrentFile *child:file->children)
        {
            if(child->index>0)
            {
                QFileInfo fi(child->name);
                if(GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fi.suffix().toLower()))
                {

					child->checkStatus=(on?Qt::Checked:Qt::Unchecked);
                    child->setParentCheckStatus();
                }
            }
            else if(child->children.count()>0)
            {
                fileList.append(child);
            }
        }
    }
    for(int i=0;i<root->children.count();++i)
    {
        QModelIndex cIndex(index(i,0,QModelIndex()));
        emit dataChanged(cIndex,cIndex.sibling(cIndex.row(),2));
        refreshChildrenCheckStatus(cIndex);
    }
    emit checkedIndexChanged();
}

qint64 TorrentFileModel::getCheckedFileSize()
{
    QList<TorrentFile *> items;
    items.push_back(root);
    qint64 checkedFileSize=0;
    while(!items.empty())
    {
        TorrentFile *currentItem=items.takeFirst();
        for(TorrentFile *child:currentItem->children)
        {
            if(child->children.count()>0)
                items.push_back(child);
            else if(child->index>0 && child->checkStatus==Qt::Checked)
                checkedFileSize+=child->size;
        }
    }
    return checkedFileSize;
}

void TorrentFileModel::setNormColor(const QColor &color)
{
    beginResetModel();
    normColor = color;
    endResetModel();
}

void TorrentFileModel::setIgnoreColor(const QColor &color)
{
    beginResetModel();
    ignoreColor = color;
    endResetModel();
}

void TorrentFileModel::refreshChildrenCheckStatus(const QModelIndex &index)
{
    QList<QModelIndex> pIndexes;
    pIndexes.append(index);
    while (!pIndexes.isEmpty())
    {
        QModelIndex pIndex(pIndexes.takeFirst());
        TorrentFile *parentItem = static_cast<TorrentFile *>(pIndex.internalPointer());
        for(int i=0;i<parentItem->children.count();++i)
        {
            emit dataChanged(pIndex.child(i,0),pIndex.child(i,2));
            pIndexes.append(pIndex.child(i,0));
        }
    }
}

void TorrentFileModel::refreshParentCheckStatus(const QModelIndex &index)
{
    QModelIndex parentIndex(index.parent());
    while(parentIndex.isValid())
    {
        emit dataChanged(parentIndex,parentIndex);
        parentIndex=parentIndex.parent();
    }
}

QModelIndex TorrentFileModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    const TorrentFile *parentItem;

    if (parent.isValid())
        parentItem = static_cast<TorrentFile *>(parent.internalPointer());
    else
        parentItem = root;

    TorrentFile *childItem = parentItem->children.value(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TorrentFileModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    TorrentFile *childItem = static_cast<TorrentFile*>(child.internalPointer());
    TorrentFile *parentItem = childItem->parent;

    if (parentItem == root)
        return QModelIndex();
    int row=parentItem->parent->children.indexOf(parentItem);
    return createIndex(row, 0, parentItem);
}

int TorrentFileModel::rowCount(const QModelIndex &parent) const
{
    const TorrentFile *parentItem;
    if (parent.column() > 0)
        return 0;

    if (parent.isValid())
        parentItem = static_cast<TorrentFile*>(parent.internalPointer());
    else
        parentItem = root;
    return parentItem->children.size();
}

QVariant TorrentFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(index.column()==0)
            return item->name;
        else if(index.column()==1 && item->index>0)
            return item->name.mid(item->name.lastIndexOf('.')+1);
        else if(index.column()==2 && item->index>0)
            return formatSize(false,item->size);
        break;
    }
    case Qt::CheckStateRole:
    {
        if(index.column()==0)
            return item->checkStatus;
        break;
    }
    case Qt::ForegroundRole:
    {
        if(item->checkStatus==Qt::Unchecked)
            return ignoreColor;
        else
            return normColor;
        break;
    }
    }
    return QVariant();
}

bool TorrentFileModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
    if(role==Qt::CheckStateRole && index.column()==0)
    {
        item->checkStatus=Qt::CheckState(value.toInt());
        item->setChildrenCheckStatus();
        item->setParentCheckStatus();
        refreshChildrenCheckStatus(index);
        refreshParentCheckStatus(index);
        emit dataChanged(index,index.sibling(index.row(),2));
        emit checkedIndexChanged();
        return true;
    }
    return false;
}

QVariant TorrentFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags TorrentFileModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column()==0)
    {
        return  Qt::ItemIsUserCheckable | defaultFlags;
    }
    return defaultFlags;
}

CTorrentFileModel::CTorrentFileModel(QObject *parent):TorrentFileModel(&emptyRoot,parent),info(nullptr)
{

}

void CTorrentFileModel::setContent(TorrentFileInfo *infoRoot, const QString &selectIndexes)
{
    beginResetModel();
    root=infoRoot?infoRoot->root:&emptyRoot;
	if (infoRoot)
	{
		root->checkStatus = (Qt::Unchecked);
		root->setChildrenCheckStatus();
		QStringList selIndexes(selectIndexes.split(','));
		for (const QString & indexStr : selIndexes)
		{
			int index = indexStr.toInt();
			TorrentFile *tf=infoRoot->indexMap.value(index,nullptr);
			if (tf)
			{
				tf->checkStatus = Qt::Checked;
				tf->setChildrenCheckStatus();
				tf->setParentCheckStatus();
			}
		}
	}
    endResetModel();
    info=infoRoot;
}

void CTorrentFileModel::updateFileProgress(const QJsonArray &fileArray)
{
	if (!info)return;
    for(auto iter=fileArray.begin();iter!=fileArray.end();++iter)
    {
        QJsonObject fileObj=(*iter).toObject();
        int i=fileObj.value("index").toString().toInt();
        qint64 cLength=fileObj.value("completedLength").toString().toLongLong();
        TorrentFile *file=info->indexMap.value(i,nullptr);
        if(file)
        {
            file->completedSize=cLength;
            int row=file->parent->children.indexOf(file);
            QModelIndex fileIndex(createIndex(row,3,file));
            emit dataChanged(fileIndex,fileIndex);
        }
    }
}

QVariant CTorrentFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    TorrentFile *item = static_cast<TorrentFile*>(index.internalPointer());
    if(index.column()==3 && item->index>0 && role==Qt::DisplayRole)
    {
        return QString("%1%").arg(qBound<double>(0,(double)item->completedSize/item->size*100,100),0,'g',3);
    }
    return TorrentFileModel::data(index,role);
}

QVariant CTorrentFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section==3) return tr("Progress");
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}
