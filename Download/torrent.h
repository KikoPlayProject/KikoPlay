#ifndef TORRENTDECODER_H
#define TORRENTDECODER_H
#include "util.h"
#include <QAbstractItemModel>
#include <QColor>
#include <QTreeView>
class TorrentError
{
public:
    TorrentError(const QString &info):errorInfo(info){}
    QString errorInfo;
};
class TorrentDecoder
{
public:
    TorrentDecoder(const QString &torrentFile);
    TorrentDecoder(const QByteArray &torrentContent);
    TorrentFile *root;
    QString infoHash;
    QByteArray rawContent;
private:
    void decodeTorrent();
};
class TorrentFileModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TorrentFileModel(TorrentFile *rootFile, QObject *parent = nullptr);
    ~TorrentFileModel();
    QString getCheckedIndex();
    void checkAll(bool on);
    void checkVideoFiles(bool on);
    qint64 getCheckedFileSize();
    void setNormColor(const QColor &color);
    void setIgnoreColor(const QColor &color);
protected:
    TorrentFile *root;
    QColor normColor{0,0,0}, ignoreColor{200,200,200};
    const QStringList headers={tr("Name"),tr("Format"),tr("Size")};
    void refreshChildrenCheckStatus(const QModelIndex &index);
    void refreshParentCheckStatus(const QModelIndex &index);
signals:
    void checkedIndexChanged();
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return headers.size();}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
};
class CTorrentFileModel : public TorrentFileModel
{
    Q_OBJECT
public:
    explicit CTorrentFileModel(QObject *parent = nullptr);
    void setContent(TorrentFileInfo *infoRoot, const QString &selectIndexes="");
    void updateFileProgress(const QJsonArray &fileArray);
private:
    TorrentFile emptyRoot;
    TorrentFileInfo *info;
public:
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    inline virtual int columnCount(const QModelIndex &) const {return 4;}
};
class TorrentTreeView : public QTreeView
{
    Q_OBJECT
    Q_PROPERTY(QColor ignoreColor READ getIgnoreColor WRITE setIgnoreColor)
    Q_PROPERTY(QColor normColor READ getNormColor WRITE setNormColor)
public:
    using QTreeView::QTreeView;

    QColor getIgnoreColor() const {return ignoreColor;}
    void setIgnoreColor(const QColor& color)
    {
        ignoreColor =  color;
        emit ignoreColorChanged(ignoreColor);
    }
    QColor getNormColor() const {return normColor;}
    void setNormColor(const QColor& color)
    {
        normColor = color;
        emit normColorChanged(normColor);
    }
signals:
    void ignoreColorChanged(const QColor &color);
    void normColorChanged(const QColor &color);
private:
    QColor ignoreColor, normColor;
};
#endif // TORRENTDECODER_H
