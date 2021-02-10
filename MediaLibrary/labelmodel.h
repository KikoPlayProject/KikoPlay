#ifndef LABELMODEL_H
#define LABELMODEL_H
#include <QAbstractItemModel>
#include <QtCore>
#include <QColor>
struct TagNode
{
    TagNode(const QString &title="", TagNode *p=nullptr, int count=0, int tp=0):tagTitle(title),animeCount(count),type(tp),
        parent(p),subNodes(nullptr)
    {
        if(p)
        {
            if(!p->subNodes) p->subNodes=new QList<TagNode *>();
            p->subNodes->append(this);
            p->animeCount+=count;
        }
    }
    ~TagNode()
    {
        if(subNodes)
        {
            qDeleteAll(*subNodes);
            delete subNodes;
        }
    }
    QString tagTitle;
    int animeCount;
    int type;
    TagNode *parent;
    QList<TagNode *> *subNodes;
};
class LabelModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum BrushType
    {
        TopLevel,
        ChildLevel
    };

    explicit LabelModel(QObject *parent = nullptr);
    ~LabelModel();
    void refreshLabel();
    void removeTag(const QModelIndex &index);
    void selLabelList(const QModelIndexList &indexes, QStringList &tags, QSet<QString> &times);
    inline const QMap<QString,QSet<QString> > getTags(){return tagMap;}
    void setBrushColor(BrushType type, const QColor &color);
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return 1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
signals:
    void removedTag(const QString &tag);
private:
    TagNode *root;
    QMap<QString,QSet<QString> > tagMap;
    QColor foregroundColor[2];
};
class LabelProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit LabelProxyModel(QObject *parent=nullptr):QSortFilterProxyModel(parent){}
    
public:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

#endif // LABELMODEL_H
